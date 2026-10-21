/* NIC-Strelka — Povodok: it commands the set, gathers it and talks to the phone (FIRMWARE.md).
 * The skeleton with the phone's side on it: the board held on, the light, the magnets, USB, the
 * clock for the set's grid; SMP over TCP on 1337 and 1337 + n with the lock on every connection
 * (shared/smp_tcp.c) and Strelka's group 64 (common/strelka_mgmt.c), its commands acting on the
 * link's logic (common/povodok_link.c) and on the settings kept across power-ons. The radio
 * itself — the SYNCs on the air, ESB to the Belkas — is shared/link.h, not written: the kit
 * decides it first, so what goes to the Belkas (the session, the table, the taking) waits here,
 * a Belka's port answers "out of reach", and the live view's messages (shared/live_udp.c) wait
 * for the link to call povodok_period_done. The phone's way in is shared/phone_net.c — the USB
 * network adapter with DHCP for the phone, the Wi-Fi joined with the name and password that
 * Bluetooth LE brought in the first minutes (shared/ble_prov.c). Built against nRF Connect SDK
 * v3.4.0, not run on hardware. */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include "ble_prov.h"
#include "charger.h"
#include "clock.h"
#include "crypto.h"
#include "gauge.h"
#include "hold.h"
#include "light.h"
#include "link.h"
#include "live_msg.h"
#include "live_udp.h"
#include "magnet.h"
#include "phone_net.h"
#include "povodok_link.h"
#include "protocol.h"
#include "smp_tcp.h"
#include "strelka_mgmt_zephyr.h"
#include "usb.h"

LOG_MODULE_REGISTER(povodok, LOG_LEVEL_INF);

#define TABLE_MAX 4096                     /* the set's table's text held for the Belkas */

struct set_settings {                      /* "strelka/set": the set this unit belongs to */
	uint32_t address;
	uint8_t channel;
	uint8_t key[KEY_BYTES];
};

static uint64_t chip_id;
static uint8_t my_number;                  /* 0–3, "strelka/n"; 0 the first by design */
static struct set_settings set = { .address = 0, .channel = 0 };
static uint8_t password_hash[32];          /* "strelka/pw": the application's, SHA-256 */
static bool password_loaded;
static struct gauge_learned learned;       /* "strelka/gauge" */
static bool learned_loaded;
static struct povodok_link link;
static struct strelka_session session;     /* 64/1, to go to every Belka by the link */
static uint8_t table[TABLE_MAX];
static uint32_t table_len;
static bool measuring;                     /* the SYNC's state, bit 15: the record runs */

/* ---- the settings ---------------------------------------------------------------------------- */

static int settings_set(const char *key, size_t len, settings_read_cb read_cb, void *arg)
{
	if (strcmp(key, "n") == 0 && len == sizeof(my_number)) {
		return read_cb(arg, &my_number, len) < 0 ? -EIO : 0;
	}
	if (strcmp(key, "set") == 0 && len == sizeof(set)) {
		return read_cb(arg, &set, len) < 0 ? -EIO : 0;
	}
	if (strcmp(key, "pw") == 0 && len == sizeof(password_hash)) {
		password_loaded = read_cb(arg, password_hash, len) == (ssize_t)len;
		return password_loaded ? 0 : -EIO;
	}
	if (strcmp(key, "gauge") == 0 && len == sizeof(learned)) {
		learned_loaded = read_cb(arg, &learned, len) == (ssize_t)len;
		return learned_loaded ? 0 : -EIO;
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(strelka, "strelka", NULL, settings_set, NULL, NULL);

/* ---- the light's blink and the delayed deeds ------------------------------------------------ */

/* the standing pattern: Povodok's own, or the charger's on the cable (FIRMWARE.md, The magnets
 * and the light) */
static enum light standing_light(void)
{
	switch (charger_state()) {
	case CHARGER_CHARGING:
		return gauge_pct() < 20 ? LIGHT_RED_BLINK : LIGHT_GREEN_BLINK;
	case CHARGER_FAULT:
		return LIGHT_ORANGE_STEADY;
	default:
		return LIGHT_GREEN_RED;
	}
}

static void blink_over(struct k_work *w)
{
	ARG_UNUSED(w);
	light_set(standing_light());
}
static K_WORK_DELAYABLE_DEFINE(blink_work, blink_over);

static void off_now(struct k_work *w)
{
	ARG_UNUSED(w);
	light_set(LIGHT_OFF);
	hold_release();
}
static K_WORK_DELAYABLE_DEFINE(off_work, off_now);

static void reboot_now(struct k_work *w)
{
	ARG_UNUSED(w);
	sys_reboot(SYS_REBOOT_COLD);
}
static K_WORK_DELAYABLE_DEFINE(reboot_work, reboot_now);

/* ---- the record's state, from the magnets and from the application --------------------------- */

static void set_record(bool run)
{
	if (run == measuring) {
		return;
	}
	measuring = run;
	povodok_link_record(&link, run);
	light_flash(run ? COLOUR_GREEN : COLOUR_RED, 2000);
}

/* ---- the live view to the phone, a message a period (PHONE.md) ------------------------------ */

/* the link calls it after the PASSes of a period, the gathering whole; one message or two */
void povodok_period_done(void)
{
	static struct live_msg msgs[LIVE_MSG_MESSAGES];
	unsigned n = live_msg_from_link(&link, session.s, msgs);

	for (unsigned i = 0; i < n; i++) {
		live_udp_send(msgs[i].bytes, msgs[i].len);
	}
}

/* ---- Strelka's group 64, Povodok's side (COMMANDS.md) --------------------------------------- */

static int op_info(struct strelka_info *out, void *ctx)
{
	ARG_UNUSED(ctx);
	out->chip = chip_id;
	out->n = my_number;
	out->who = WHO_POVODOK;
	out->fw[0] = STRELKA_FW_MAJOR;
	out->fw[1] = STRELKA_FW_MINOR;
	out->fw[2] = STRELKA_FW_PATCH;
	out->cell_pct = gauge_pct();
	out->cell_mv = gauge_mv();
	out->temp_centi = gauge_temp_centi();
	out->state = measuring ? BELKA_RECORDING : BELKA_WAITING;
	return 0;
}

static int op_session(const struct strelka_session *s, void *ctx)
{
	ARG_UNUSED(ctx);
	if (measuring) {
		return SMP_ERR_BAD_STATE;        /* not while the set records */
	}
	session = *s;
	povodok_link_live(&link, s->live);
	/* with the period of the instant t, to every Belka by the link: to come */
	return 0;
}

static int op_blink(uint32_t seconds, void *ctx)
{
	ARG_UNUSED(ctx);
	if (seconds == 0) {
		k_work_cancel_delayable(&blink_work);
		light_set(standing_light());
		return 0;
	}
	light_set(LIGHT_ORANGE_FAST);
	k_work_reschedule(&blink_work, K_SECONDS(seconds));
	return 0;
}

static int op_off(void *ctx)
{
	ARG_UNUSED(ctx);
	/* recording, the set is stopped first — five SYNCs saying so — then off; the answer leaves
	 * before the board goes dark */
	set_record(false);
	k_work_reschedule(&off_work, K_MSEC(500));
	return 0;
}

static int op_forget(void *ctx)
{
	ARG_UNUSED(ctx);
	settings_delete("strelka/set");
	settings_delete("strelka/n");
	settings_delete("strelka/gauge");
	settings_delete("strelka/pw");   /* the factory's password again */
	phone_net_forget();              /* the network's name and password */
	k_work_reschedule(&reboot_work, K_MSEC(500));
	return 0;
}

static int op_record(bool run, void *ctx)
{
	ARG_UNUSED(ctx);
	set_record(run);
	return 0;
}

static int op_live(uint8_t code, void *ctx)
{
	ARG_UNUSED(ctx);
	session.live = code;
	povodok_link_live(&link, code);
	return 0;
}

static int op_search(bool run, void *ctx)
{
	ARG_UNUSED(ctx);
	if (run && measuring) {
		return SMP_ERR_BAD_STATE;        /* searching, the first sends CALLs, no SYNCs */
	}
	povodok_link_search(&link, run);
	return 0;
}

static int op_found(struct strelka_found *list, unsigned max, unsigned *n, void *ctx)
{
	ARG_UNUSED(ctx);
	*n = 0;
	for (unsigned i = 0; i < link.found_n && *n < max; i++) {
		list[*n].chip = link.found[i].chip;
		list[*n].rssi = link.found[i].rssi;
		list[*n].ago_s = (link.period - link.found[i].period) / STRELKA_PERIODS_A_S;
		(*n)++;
	}
	return 0;
}

static int op_take(uint64_t chip, uint8_t n, void *ctx)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(n);
	for (unsigned i = 0; i < link.found_n; i++) {
		if (link.found[i].chip == chip) {
			/* heard: the key agreed and the set given by SMP over ESB, 64/8 and 64/9 —
			 * the link's, to come; until then the Belka is out of reach */
			return STRELKA_RC(STRELKA_ERR_UNREACHABLE);
		}
	}
	return SMP_ERR_NOENT;
}

static int op_set(struct strelka_set *out, void *ctx)
{
	ARG_UNUSED(ctx);
	out->n = 0;
	for (unsigned k = 1; k <= STRELKA_BELKAS_MAX; k++) {
		const struct povodok_belka *b = &link.belka[k];
		struct strelka_set_belka *o = &out->belka[out->n];

		if (!b->taken) {
			continue;
		}
		o->n = (uint8_t)k;
		o->chip = b->chip;
		o->ago = link.period > b->last_period ? link.period - b->last_period : 0;
		o->by = b->by;
		if (b->item_known[ITEM_STATE]) {
			o->state = ITEM_HIGH(b->item[ITEM_STATE]);
			o->fault = ITEM_LOW(b->item[ITEM_STATE]);
		}
		out->n++;
	}
	out->povodoks = (uint8_t)(1u << my_number);
	for (unsigned k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		if (link.povodok_known[k] && link.tick - link.povodok_heard_tick[k] < LINK_PERIODS_A_S) {
			out->povodoks |= (uint8_t)(1u << k);
		}
	}
	out->ch = link.cfg.channel;
	out->spare = link.spare;
	return 0;
}

static int op_drop(uint8_t n, void *ctx)
{
	ARG_UNUSED(ctx);
	if (!link.belka[n].taken) {
		return SMP_ERR_NOENT;
	}
	povodok_link_drop(&link, n);
	return 0;
}

static int op_password_hash(uint8_t hash[32], void *ctx)
{
	ARG_UNUSED(ctx);
	memcpy(hash, password_hash, 32);
	return 0;
}

static int op_password_set(const uint8_t hash[32], void *ctx)
{
	ARG_UNUSED(ctx);
	memcpy(password_hash, hash, 32);
	return settings_save_one("strelka/pw", password_hash, 32) ? SMP_ERR_UNKNOWN : 0;
}

static int op_table(const struct strelka_table_piece *p, void *ctx)
{
	ARG_UNUSED(ctx);
	if (p->len > TABLE_MAX) {
		return SMP_ERR_TOO_LARGE;
	}
	if (p->off == 0) {
		memset(table, 0, sizeof(table));
		table_len = p->len;
	}
	memcpy(table + p->off, p->csv, p->csv_len);
	/* whole, to every Belka by the link: to come */
	return 0;
}

static const struct strelka_ops ops = {
	.info = op_info, .session = op_session, .blink = op_blink, .off = op_off,
	.forget = op_forget, .record = op_record, .live = op_live, .search = op_search,
	.found = op_found, .take = op_take, .set = op_set, .drop = op_drop,
	.password_hash = op_password_hash, .password_set = op_password_set, .table = op_table,
};

/* ---- the magnets ---------------------------------------------------------------------------- */

static void on_magnet(enum magnet_event e)
{
	switch (e) {
	case MAGNET_A_SHORT:
		light_show_cell(gauge_pct());
		break;
	case MAGNET_A_OFF:
		op_off(NULL);
		break;
	case MAGNET_B_START:
		set_record(true);
		break;
	case MAGNET_B_STOP:
		set_record(false);
		break;
	case MAGNET_BOTH_10S:
		light_flash(COLOUR_ORANGE, 2000);
		break;
	case MAGNET_FACTORY:
		op_forget(NULL);
		break;
	}
}

int main(void)
{
	const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
	struct wdt_timeout_cfg wdt_cfg = { .window.max = 5000, .flags = WDT_FLAG_RESET_SOC };
	struct povodok_link_cfg link_cfg;
	int wdt_ch;

	hold_init();
	light_init();
	hwinfo_get_device_id((uint8_t *)&chip_id, sizeof(chip_id));
	LOG_INF("Povodok, firmware %u.%u.%u, chip %016llx", STRELKA_FW_MAJOR, STRELKA_FW_MINOR,
		STRELKA_FW_PATCH, (unsigned long long)chip_id);
	settings_subsys_init();
	settings_load();
	if (!password_loaded) {
		/* the factory settings' password, the build's own, until the application changes it */
		strelka_sha256((const uint8_t *)CONFIG_STRELKA_FACTORY_PASSWORD,
			       strlen(CONFIG_STRELKA_FACTORY_PASSWORD), password_hash);
	}
	link_cfg = (struct povodok_link_cfg){ .number = my_number, .address = set.address,
					      .channel = set.channel };
	povodok_link_init(&link, &link_cfg);
	clock_init();                    /* the first Povodok's timer is the set's grid */
	charger_init();
	gauge_init(learned_loaded ? &learned : NULL);
	usb_init();                      /* up when the stack sees VBUS: on the cable, at once */
	phone_net_init();                /* the lead's network with DHCP for the phone; the Wi-Fi kept */
	ble_prov_init();                 /* the network's name and password, the first minutes */
	strelka_mgmt_zephyr_init(&ops, NULL);
	smp_tcp_init();                  /* the phone's side: 1337, and 1337 + n as Belkas are taken */
	live_udp_init();                 /* the live view to the phone, from povodok_period_done */
	magnet_init(on_magnet);
	light_set(standing_light());
	/* the watchdog last, after the start */
	wdt_ch = wdt_install_timeout(wdt, &wdt_cfg);
	wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);

	/* Povodok on its cable starts as by magnet A and runs on; off the cable it lets HOLD go
	 * 5 min after the cable left, unless the cable has come back (FIRMWARE.md, Povodok and its
	 * cable): the charger's /PG says. The link's periods — povodok_link_period_begin and _end
	 * on the radio's timer — are shared/link.h's, to come */
	for (unsigned off_cable_s = 0;;) {
		bool taken[STRELKA_BELKAS_MAX + 1];
		struct gauge_learned fresh;

		wdt_feed(wdt, wdt_ch);
		for (int j = 0; j < 10; j++) {
			k_sleep(K_MSEC(100));
			charger_service();
		}
		if (gauge_service(&fresh)) {
			learned = fresh;
			learned_loaded = true;
			settings_save_one("strelka/gauge", &learned, sizeof(learned));
		}
		off_cable_s = charger_powered() ? 0 : off_cable_s + 1;
		if (off_cable_s == 5 * 60 || (gauge_empty() && !charger_powered())) {
			op_off(NULL);
		}
		light_set(standing_light());
		for (unsigned n = 0; n <= STRELKA_BELKAS_MAX; n++) {
			taken[n] = link.belka[n].taken;
		}
		smp_tcp_belka_ports(taken);
	}
	return 0;
}
