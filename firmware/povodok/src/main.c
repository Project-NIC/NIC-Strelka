/* NIC-Strelka — Povodok: it commands the set, gathers it and talks to the phone (FIRMWARE.md).
 * The skeleton with the phone's side on it: the PMIC and the power, the light, the magnets, USB, the
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
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_sd.h>
#include "ble_prov.h"
#include "clock.h"
#include "crypto.h"
#include "gauge.h"
#include "light.h"
#include "pmic.h"
#include "power.h"
#include "link.h"
#include "live_msg.h"
#include "live_udp.h"
#include "magnet.h"
#include "phone_net.h"
#include "povodok_link.h"
#include "protocol.h"
#include "smp_tcp.h"
#include "store.h"
#include "strelka_mgmt_zephyr.h"
#include "usb.h"

LOG_MODULE_REGISTER(povodok, LOG_LEVEL_INF);

#define TABLE_MAX 4096                     /* the set's table's text held for the Belkas */

static uint64_t chip_id;
/* the set, the number 0–3 (0 the first by design), the password's hash and the gauge's
 * state: store.h's, kept across power-ons */
static uint8_t password_hash[32];          /* the kept one, or the factory's */

/* found on the network by name, as printers are (PHONE.md, The link): the service _strelka._tcp
 * on 1337, the instance "Strelka A1B2C3D4 P1" — the set's address and the Povodok's number from 1,
 * as written on its box — so that the application finds every Povodok of its set, the network's
 * way of gathering among them, without searching the addresses */
static char sd_instance[40] = "Strelka";
/* the service mode, from every start until the application's 64/20: Bluetooth LE on for the
 * network, the set's setting allowed; out of it, Bluetooth LE off and every setting refused with 6,
 * until the unit is switched off and on (FIRMWARE.md, The service mode) */
static bool service = true;
DNS_SD_REGISTER_TCP_SERVICE(strelka_sd, sd_instance, "_strelka", "local", DNS_SD_EMPTY_TXT,
			    SMP_PORT);
static struct povodok_link link;
static struct strelka_session session;     /* 64/1, to go to every Belka by the link */
static uint8_t table[TABLE_MAX];
static uint32_t table_len;
static bool measuring;                     /* the SYNC's state, bit 15: the record runs */

/* ---- the light's blink and the delayed deeds ------------------------------------------------ */

/* the standing pattern: Povodok's own, green and red in turn, the cell low off the cable; on the
 * cable the PMIC's only while it charges or its charger is at fault (FIRMWARE.md, The magnets and
 * the light) */
static enum light standing_light(void)
{
	switch (pmic_charge()) {
	case PMIC_CHARGING:
	case PMIC_CHARGE_FAULT:
		return LIGHT_CHARGER;
	case PMIC_OFF_CABLE:
		return gauge_pct() < 20 && gauge_present() ? LIGHT_RED_BLINK : LIGHT_GREEN_RED;
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

static volatile bool go_off;

static void off_now(struct k_work *w)
{
	ARG_UNUSED(w);
	go_off = true;           /* main's thread takes it, off the system work queue */
}
static K_WORK_DELAYABLE_DEFINE(off_work, off_now);

/* the restart after the factory settings, or after a set given: the unit stays on (power.h) */
static bool factory_restart;

static void reboot_now(struct k_work *w)
{
	ARG_UNUSED(w);
	power_restart(factory_restart);
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
	unsigned n;

	if (service) {
		return;                          /* the live view is the work's, not the service's */
	}
	n = live_msg_from_link(&link, session.s, msgs);

	for (unsigned i = 0; i < n; i++) {
		live_udp_send(msgs[i].bytes, msgs[i].len);
	}
}

/* ---- Strelka's group 64, Povodok's side (COMMANDS.md) --------------------------------------- */

static int op_info(struct strelka_info *out, void *ctx)
{
	ARG_UNUSED(ctx);
	out->chip = chip_id;
	out->n = store.number;
	out->who = WHO_POVODOK;
	out->label = store.label;
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
	povodok_link_live(&link, service ? S0_LIVE_OFF : s->live);   /* on at 64/20 */
	/* the three ways of gathering (PRINCIPLE.md): by the set's radio, or by the network, every
	 * Povodok sending the application what it heard itself */
	povodok_link_gather(&link, s->net);
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

/* 64/19: the set given to this Povodok by the application, its key opened under the write's own
 * lock (common/strelka_mgmt.c): kept, and the unit started again on it */
static int op_give(const struct strelka_give *g, void *ctx)
{
	struct store_set set = {
		.address = (uint32_t)g->addr[0] | (uint32_t)g->addr[1] << 8 |
			   (uint32_t)g->addr[2] << 16 | (uint32_t)g->addr[3] << 24,
		.channel = g->ch,
	};
	int rc;

	ARG_UNUSED(ctx);
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
	if (measuring) {
		return SMP_ERR_BAD_STATE;        /* not while the set records */
	}
	memcpy(set.key, g->key, sizeof(set.key));
	rc = store_join(&set, g->n);
	if (rc == 0) {
		rc = store_label(g->label);
	}
	memset(&set, 0, sizeof(set));
	if (rc) {
		return SMP_ERR_UNKNOWN;
	}
	k_work_reschedule(&reboot_work, K_MSEC(500));
	return 0;
}

static int op_forget(void *ctx)
{
	ARG_UNUSED(ctx);
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
	store_forget();                  /* the factory's password again */
	phone_net_forget();              /* the network's name and password */
	factory_restart = true;
	k_work_reschedule(&reboot_work, K_MSEC(2000));   /* after the orange's 2 s */
	return 0;
}

static int op_record(bool run, void *ctx)
{
	ARG_UNUSED(ctx);
	if (run && service) {
		return SMP_ERR_BAD_STATE;        /* the record is the work's: 64/20 first */
	}
	set_record(run);
	return 0;
}

static int op_live(uint8_t code, void *ctx)
{
	ARG_UNUSED(ctx);
	if (code != S0_LIVE_OFF && service) {
		return SMP_ERR_BAD_STATE;        /* the live view is the work's: 64/20 first */
	}
	session.live = code;
	povodok_link_live(&link, code);
	return 0;
}

static int op_search(bool run, void *ctx)
{
	ARG_UNUSED(ctx);
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
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
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
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
		o->rc_known = false;             /* 64/1's answers come with the frames over ESB */
		out->n++;
	}
	out->povodoks = (uint8_t)(1u << store.number);
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
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
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
	if (!service) {
		return SMP_ERR_BAD_STATE;        /* a setting: the service mode's alone */
	}
	memcpy(password_hash, hash, 32);
	return store_pw(hash) ? SMP_ERR_UNKNOWN : 0;
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

/* 64/20: out of the service mode — Bluetooth LE off by main's loop, the settings refused, the
 * Belkas' setting commands not bridged — until switched off and on */
static int op_work(void *ctx)
{
	ARG_UNUSED(ctx);
	if (service) {
		service = false;
		smp_tcp_service(false);
		ble_prov_stop_soon();
		povodok_link_live(&link, session.live);   /* the session's live view, if it asks one */
		LOG_INF("out of the service mode: at work");
	}
	return 0;
}

static const struct strelka_ops ops = {
	.info = op_info, .session = op_session, .blink = op_blink, .off = op_off,
	.forget = op_forget, .record = op_record, .live = op_live, .search = op_search,
	.found = op_found, .take = op_take, .set = op_set, .drop = op_drop,
	.password_hash = op_password_hash, .password_set = op_password_set, .table = op_table,
	.give = op_give, .work = op_work,
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
	case MAGNET_BOTH_8S:
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

	/* the PMIC, the settings kept and the gauge first: then whether Povodok is switched on —
	 * magnet A, or the cable — or switched off on the cable, which idles in power_start, or a
	 * magnet brushed past, which goes back into ship mode (FIRMWARE.md, The PMIC) */
	pmic_init();
	store_load();
	gauge_init(store.gauge_len ? store.gauge : NULL, store.gauge_len);
	light_init();
	power_start(true);
	hwinfo_get_device_id((uint8_t *)&chip_id, sizeof(chip_id));
	LOG_INF("Povodok, firmware %u.%u.%u, chip %016llx", STRELKA_FW_MAJOR, STRELKA_FW_MINOR,
		STRELKA_FW_PATCH, (unsigned long long)chip_id);
	if (store.pw_loaded) {
		memcpy(password_hash, store.pw, sizeof(password_hash));
	} else {
		/* the factory settings' password, the build's own, until the application changes it */
		strelka_sha256((const uint8_t *)CONFIG_STRELKA_FACTORY_PASSWORD,
			       strlen(CONFIG_STRELKA_FACTORY_PASSWORD), password_hash);
	}
	/* the name as a person reads it: the set's own number if it has one, the Povodok 1–4 —
	 * 0–3 inside — and the set's address, unique where two sets share a network */
	if (store.label) {
		snprintf(sd_instance, sizeof(sd_instance), "Strelka %u P%u %08X", store.label,
			 store.number + 1u, (unsigned int)store.set.address);
	} else {
		snprintf(sd_instance, sizeof(sd_instance), "Strelka P%u %08X", store.number + 1u,
			 (unsigned int)store.set.address);
	}
	link_cfg = (struct povodok_link_cfg){ .number = store.number,
					      .address = store.set.address,
					      .channel = store.set.channel };
	povodok_link_init(&link, &link_cfg);
	clock_init();                    /* the first Povodok's timer is the set's grid */
	usb_init();                      /* up when the stack sees VBUS: on the cable, at once */
	phone_net_init();                /* the lead's network with DHCP for the phone; the Wi-Fi kept */
	ble_prov_init();                 /* the network's name and password, the service mode */
	strelka_mgmt_zephyr_init(&ops, NULL);
	smp_tcp_init();                  /* the phone's side: 1337, and 1337 + n as Belkas are taken */
	live_udp_init();                 /* the live view to the phone, from povodok_period_done */
	magnet_init(on_magnet);
	light_set(standing_light());
	/* the watchdog last, after the start */
	wdt_ch = wdt_install_timeout(wdt, &wdt_cfg);
	wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	power_watchdog_taken();          /* fed by the loop from here; until now by power.c */

	/* Povodok on its cable starts as by magnet A and runs on; off the cable it goes off 5 min
	 * after the cable left, unless the cable has come back (FIRMWARE.md, The PMIC): the PMIC's
	 * VBUS says. The link's periods — povodok_link_period_begin and _end
	 * on the radio's timer — are shared/link.h's, to come */
	bool cable_seen = false;

	for (unsigned off_cable_s = 0;;) {
		bool taken[STRELKA_BELKAS_MAX + 1];
		static unsigned s;

		wdt_feed(wdt, wdt_ch);
		k_sleep(K_SECONDS(1));
		if (smp_tcp_joined()) {
			ble_prov_stop_soon();    /* a phone joined: the network given, the radio the set's */
		}
		ble_prov_poll();
		if (go_off) {
			power_off();     /* ship mode; on the cable, idle dark until it leaves */
		}
		gauge_service();
		if (++s % 3600 == 0) {
			power_keep();        /* the gauge's state once an hour */
		}
		/* the set's channel kept as the first moves it, so that a Povodok switched on later
		 * finds the set where it went (store.h) */
		store_channel(link.cfg.channel);
		/* the 5 min count only once a cable has been seen in this run: a Povodok switched on
		 * by magnet A off the cable runs on; no PMIC answering, the development kit's case,
		 * never off by itself */
		if (pmic_vbus() || !pmic_present()) {
			cable_seen = cable_seen || pmic_present();
			off_cable_s = 0;
		} else if (cable_seen) {
			off_cable_s++;
		}
		if (off_cable_s == 5 * 60 || (gauge_empty() && !pmic_vbus())) {
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
