/* NIC-Strelka — the Belka: it reads its sensor, stamps the samples, stores them and sends them,
 * and nothing else (FIRMWARE.md). This is the skeleton: the board held on, the light, the
 * magnets, the clock and the sensor, the card, USB, and the record's path from a sample to a
 * block on the card — every sample with its own stamp from the interrupt, summed into steps by
 * its number on the set's grid (common/steps.c), the steps composed into blocks
 * (common/session.c), the blocks to the card through its buffer (card.c); the live view's turn
 * as a quaternion from every step (common/orient.c). The sensor's thread owns the record: a stop asked from elsewhere — a magnet, the card's fault, the application — is
 * a request it serves between two samples. Strelka's group 64 (common/strelka_mgmt.c) is
 * registered with MCUmgr and does the Belka's part of it on the settings, the card and the
 * record; what starts and stops the record is the SYNC's state, and the SYNC, the LIVEs and the
 * SMP frames over ESB come by the link, shared/link.h, which is not written: the kit decides it
 * first. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/fs/fs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include "belka_link.h"
#include "card.h"
#include "charger.h"
#include "clock.h"
#include "crypto.h"
#include "gauge.h"
#include "hold.h"
#include "imu.h"
#include "light.h"
#include "magnet.h"
#include "orient.h"
#include "protocol.h"
#include "quat.h"
#include "record.h"
#include "session.h"
#include "steps.h"
#include "strelka_mgmt_zephyr.h"
#include "usb.h"

LOG_MODULE_REGISTER(belka, LOG_LEVEL_INF);

#define SENSOR_PRIO 2                      /* above the writer, below the radio's work */
#define MOUNT "/SD:"
#define TABLE_MAX 4096                     /* the set's table's text, written after the head */

struct set_settings {                      /* "strelka/set": the set this Belka belongs to */
	uint32_t address;
	uint8_t channel;
	uint8_t key[KEY_BYTES];
};

static uint64_t chip_id;
static uint8_t my_number;                  /* 1–63 from the taking, "strelka/n"; 0 a new Belka */
static struct set_settings set;
static struct strelka_cal cal;             /* "strelka/cal"; t 0 none */
static bool cal_loaded;
static struct gauge_learned learned;       /* "strelka/gauge": what the gauge learned of the cell */
static bool learned_loaded;
static uint8_t state = BELKA_WAITING, fault = FAULT_NONE;
static struct imu_config sensor_cfg = { 1000, 16, 2000 };
static uint8_t samples_a_step = 10;        /* N: 1 kHz into 100 Hz */
static struct strelka_session session_cfg = { .s = 0, .rate_hz = 1000, .n = 10, .acc_g = 16,
					      .gyro_dps = 2000, .live = 0, .stop_s = 10 };
static uint8_t table[TABLE_MAX];
static uint32_t table_len;
static uint8_t peer_pub[32];               /* Povodok's X25519 key from 64/8, for 64/9 */
static bool peer_pub_set;
static struct belka_link link;
/* the live view's steps of the period, for the LIVE: the turn as quaternions (common/orient.c)
 * or the raw steps, at the live view's rate, the link taking them at the slot */
static struct orient orient;
static uint8_t live_body[LIVE_BODY_MAX];
static unsigned live_fill;                 /* steps in it, 0–2 */

static enum light standing_light(void);

/* the record's path, the sensor's thread's alone */
static struct steps steps;
static struct session session;
static bool recording;
static uint32_t lagged;                    /* times the thread found more than one stamp waiting */
/* the stamps from the interrupt, one a data-ready, in their order */
K_MSGQ_DEFINE(stamps, sizeof(uint64_t), 64, 4);
/* what others ask of the record: served by the sensor's thread between two samples */
static volatile bool stop_asked, off_asked;
static volatile uint8_t stop_why;

/* ---- the settings ---------------------------------------------------------------------------- */

static int settings_set(const char *key, size_t len, settings_read_cb read_cb, void *arg)
{
	if (strcmp(key, "n") == 0 && len == sizeof(my_number)) {
		return read_cb(arg, &my_number, len) < 0 ? -EIO : 0;
	}
	if (strcmp(key, "set") == 0 && len == sizeof(set)) {
		return read_cb(arg, &set, len) < 0 ? -EIO : 0;
	}
	if (strcmp(key, "cal") == 0 && len == sizeof(cal)) {
		cal_loaded = read_cb(arg, &cal, len) == (ssize_t)len;
		return cal_loaded ? 0 : -EIO;
	}
	if (strcmp(key, "gauge") == 0 && len == sizeof(learned)) {
		learned_loaded = read_cb(arg, &learned, len) == (ssize_t)len;
		return learned_loaded ? 0 : -EIO;
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(strelka, "strelka", NULL, settings_set, NULL, NULL);

/* ---- the record ------------------------------------------------------------------------------ */

static void to_card(const uint8_t block[REC_BLOCK_BYTES], void *ctx)
{
	ARG_UNUSED(ctx);
	card_put(block);
}

/* the live view: every step turns the orientation on; at the live view's rate — every
 * (rate / live Hz)th step — a quaternion or the raw step goes into the period's LIVE body */
static void live_step(uint32_t k, const struct strelka_step *step)
{
	uint8_t code = session_cfg.live;
	unsigned live_hz = (code == S0_QUAT_100 || code == S0_RAW_100) ? 100u : 50u;
	unsigned every = (sensor_cfg.rate_hz / samples_a_step) / live_hz;
	unsigned steps_a_period = live_steps(code);

	orient_step(&orient, step, (float)samples_a_step / sensor_cfg.rate_hz);
	if (code == S0_LIVE_OFF || every == 0 || k % every != 0) {
		return;
	}
	if (live_fill >= steps_a_period) {
		live_fill = 0;                   /* the period's body taken by the link, or left */
	}
	if (code == S0_RAW_50 || code == S0_RAW_100) {
		memcpy(live_body + live_fill * STEP_BYTES, step, STEP_BYTES);
	} else {
		float q[4];
		uint32_t packed;

		orient_quat(&orient, q);
		packed = quat_pack(q);
		memcpy(live_body + live_fill * live_step_bytes(code), &packed, sizeof(packed));
	}
	live_fill++;
}

/* a step done: into the session with what the clock says of its first sample; the SYNC's signal
 * and its Povodok come from the link, none here yet; a step marked lost has no stamp */
static void step_out(uint32_t k, const struct strelka_step *step, bool acc_out, bool gyro_out,
		     uint64_t first_stamp, void *ctx)
{
	struct session_step_info info = {
		.offset_ns = first_stamp ? (int32_t)(clock_grid_offset_ticks(first_stamp) *
						     CLOCK_TICK_NS) : 0,
		.signal_dbm = REC_NO_SIGNAL,
		.povodok = REC_NO_POVODOK,
		.syncs = 0,
		.acc_out = acc_out,
		.gyro_out = gyro_out,
	};

	ARG_UNUSED(ctx);
	session_step(&session, step, &info, imu_temp_centi(), gauge_mv());
	live_step(k, step);
}

/* the record begins at the first sample of `first_period`, the period after the first SYNC heard
 * saying it runs; the head says what the session command, 64/1, the calibration and the taking
 * gave. The link calls it, from the sensor's thread's side, so it waits unused here */
static __unused int record_start(uint32_t first_period)
{
	struct rec_calibration rec_cal = {
		.temp = cal.temp, .time = cal.t,
	};
	struct rec_session cfg = {
		.session = session_cfg.s, .belka = my_number, .chip = chip_id,
		.fw = { STRELKA_FW_MAJOR, STRELKA_FW_MINOR, STRELKA_FW_PATCH },
		.rate_hz = sensor_cfg.rate_hz / samples_a_step, .n = samples_a_step,
		.acc_g = sensor_cfg.acc_g, .gyro_dps = sensor_cfg.gyro_dps,
		.first_period = first_period, .time_us = session_cfg.t_us,
		.time_period = session_cfg.p_given ? session_cfg.p : first_period,
		.tz_min = session_cfg.tz_min, .clock = CLOCK_STEERED,
		.cal = cal_loaded ? &rec_cal : NULL, .place = "", .name = "",
	};
	char name[32];

	memcpy(rec_cal.acc, cal.acc, sizeof(rec_cal.acc));
	memcpy(rec_cal.gyro_scale, cal.gscale, sizeof(rec_cal.gyro_scale));
	memcpy(rec_cal.gyro_zero, cal.gzero, sizeof(rec_cal.gyro_zero));
	if (recording || !card_present()) {
		return -1;
	}
	usb_allow(false);                /* the card is the record's now, not the host's */
	if (card_mount()) {
		fault = card_fault();
		usb_allow(true);
		return -1;
	}
	snprintf(name, sizeof(name), "S%05uB%02u.STRK", session_cfg.s, my_number);
	cfg.name = name;
	if (card_open(session_cfg.s, my_number)) {
		fault = card_fault();
		usb_allow(true);
		return -1;
	}
	session_begin(&session, &cfg, to_card, NULL);
	if (table_len) {
		session_table(&session, (const char *)table, table_len);
	}
	steps_begin(&steps, sensor_cfg.rate_hz, samples_a_step, first_period, step_out, NULL);
	recording = true;
	state = BELKA_RECORDING;
	light_set(LIGHT_GREEN_FAST);
	return 0;
}

/* the sensor's thread's own: the end written, the card told to close */
static void record_end(uint8_t why)
{
	if (!recording) {
		return;
	}
	steps_end(&steps);
	session_end(&session, why, card_dropped());
	card_close();
	recording = false;
	state = BELKA_WAITING;
	light_set(standing_light());
	usb_allow(true);
}

/* from anywhere else: a request the sensor's thread serves */
static void record_stop(uint8_t why)
{
	stop_why = why;
	stop_asked = true;
}

/* the interrupt: the edge's stamp, captured in hardware, into the queue; the thread reads the
 * registers after — the newest sample, so that a thread lagging a sample behind pairs a stamp
 * with the sample after it, which the sensor's FIFO alone would prevent (FIRMWARE.md, Reading
 * the sensor); counted as `lagged`, and seen in the record as a repeated index dropped */
static void on_data_ready(uint64_t stamp)
{
	if (k_msgq_put(&stamps, &stamp, K_NO_WAIT)) {
		lagged++;
	}
}

static void sensor_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		struct strelka_step sample;
		uint64_t stamp;
		bool acc_out, gyro_out;
		int64_t index;

		if (k_msgq_get(&stamps, &stamp, K_MSEC(100)) == 0) {
			if (k_msgq_num_used_get(&stamps) > 0) {
				lagged++;
			}
			if (imu_read_step(&sample, &acc_out, &gyro_out)) {
				fault = FAULT_SENSOR_SILENT;
			} else {
				index = clock_sample_index(stamp);
				if (recording && index >= 0) {
					steps_sample(&steps, index, stamp, &sample, acc_out,
						     gyro_out);
				}
				/* the live view's step, as quaternions or raw, goes to the link */
			}
		}
		/* what others asked meanwhile, served between two samples */
		if (recording && card_fault() != FAULT_NONE) {
			fault = card_fault();
			record_end(fault == FAULT_CARD_FULL ? END_CARD_FULL : END_FAULT);
		}
		if (stop_asked) {
			stop_asked = false;
			record_end(stop_why);
		}
		if (off_asked && !card_busy()) {
			off_asked = false;
			light_set(LIGHT_OFF);
			hold_release();          /* the board dark as the magnet leaves */
		}
	}
}

K_THREAD_DEFINE(sensor, 2048, sensor_thread, NULL, NULL, NULL, SENSOR_PRIO, 0, 0);

/* ---- the light's blink and the delayed deeds ------------------------------------------------ */

/* the standing pattern (FIRMWARE.md, The magnets and the light): measuring, a fault, the cell
 * low, the charger's on the cable, else waiting in the set */
static enum light standing_light(void)
{
	if (recording) {
		return LIGHT_GREEN_FAST;
	}
	if (fault && fault != FAULT_CELL_LOW) {
		return LIGHT_RED_STEADY;
	}
	switch (charger_state()) {
	case CHARGER_CHARGING:
		return gauge_pct() < 20 ? LIGHT_RED_BLINK : LIGHT_GREEN_BLINK;
	case CHARGER_DONE:
		return LIGHT_GREEN_STEADY;
	case CHARGER_FAULT:
		return LIGHT_ORANGE_STEADY;
	default:
		break;
	}
	return gauge_pct() < 20 && gauge_present() ? LIGHT_RED_BLINK : LIGHT_GREEN_SLOW;
}

static void blink_over(struct k_work *w)
{
	ARG_UNUSED(w);
	light_set(standing_light());
}
static K_WORK_DELAYABLE_DEFINE(blink_work, blink_over);

static void reboot_now(struct k_work *w)
{
	ARG_UNUSED(w);
	sys_reboot(SYS_REBOOT_COLD);
}
static K_WORK_DELAYABLE_DEFINE(reboot_work, reboot_now);

/* ---- Strelka's group 64, the Belka's side (COMMANDS.md) ------------------------------------- */

static int op_info(struct strelka_info *out, void *ctx)
{
	ARG_UNUSED(ctx);
	out->chip = chip_id;
	out->n = my_number;
	out->who = WHO_BELKA;
	out->fw[0] = STRELKA_FW_MAJOR;
	out->fw[1] = STRELKA_FW_MINOR;
	out->fw[2] = STRELKA_FW_PATCH;
	out->sensor = imu_whoami();
	out->card_in = card_present();
	out->card_free_mb = card_free_mb();
	out->card_fs = card_present() ? 2 : 0;   /* the card's size and its format: card.c, to come */
	out->cell_pct = gauge_pct();
	out->cell_mv = gauge_mv();
	out->temp_centi = imu_temp_centi();
	out->state = state;
	out->fault = fault;
	out->cal_time = cal_loaded ? cal.t : 0;
	return 0;
}

static int op_session(const struct strelka_session *s, void *ctx)
{
	ARG_UNUSED(ctx);
	if (recording) {
		return SMP_ERR_BAD_STATE;
	}
	if (!imu_rate_ok(s->rate_hz) || !imu_ranges_ok(s->acc_g, s->gyro_dps)) {
		return SMP_ERR_INVAL;
	}
	session_cfg = *s;
	sensor_cfg.rate_hz = s->rate_hz;
	sensor_cfg.acc_g = s->acc_g;
	sensor_cfg.gyro_dps = s->gyro_dps;
	samples_a_step = s->n;
	link.cfg.stop_s = s->stop_s;
	imu_configure(&sensor_cfg);
	clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
	orient_init(&orient, sensor_cfg.gyro_dps, cal_loaded ? cal.gscale : NULL,
		    cal_loaded ? cal.gzero : NULL, 0, 0);
	live_fill = 0;
	return 0;
}

/* the records on the card: every *.STRK, its head's and its end's words */
static int op_sessions(uint32_t from, struct strelka_session_entry *list, unsigned max,
		       unsigned *n, bool *more, void *ctx)
{
	struct fs_dir_t dir;
	struct fs_dirent ent;
	uint32_t index = 0;
	static uint8_t block[REC_BLOCK_BYTES];

	ARG_UNUSED(ctx);
	*n = 0;
	*more = false;
	if (!card_mounted()) {
		return SMP_ERR_BAD_STATE;        /* the host has the disk, or no card */
	}
	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, MOUNT)) {
		return SMP_ERR_UNKNOWN;
	}
	while (fs_readdir(&dir, &ent) == 0 && ent.name[0]) {
		size_t len = strlen(ent.name);
		char path[sizeof(MOUNT) + MAX_FILE_NAME + 1];
		struct fs_file_t f;
		struct rec_head head;
		struct rec_end end;
		struct strelka_session_entry *e;

		if (ent.type != FS_DIR_ENTRY_FILE || len < 6 || len > STRELKA_FILE_NAME_MAX ||
		    strcmp(ent.name + len - 5, ".STRK") != 0) {
			continue;
		}
		if (index++ < from) {
			continue;
		}
		if (*n >= max) {
			*more = true;
			break;
		}
		snprintf(path, sizeof(path), MOUNT "/%s", ent.name);
		fs_file_t_init(&f);
		if (fs_open(&f, path, FS_O_READ)) {
			continue;
		}
		e = &list[*n];
		memset(e, 0, sizeof(*e));
		strcpy(e->file, ent.name);
		e->bytes = (uint32_t)ent.size;
		if (fs_read(&f, block, sizeof(block)) == (ssize_t)sizeof(block) && rec_block_ok(block)) {
			memcpy(&head, block, sizeof(head));
			e->s = head.session;
			e->p0 = head.first_period;
		}
		if (ent.size >= 2 * REC_BLOCK_BYTES &&
		    fs_seek(&f, -(off_t)REC_BLOCK_BYTES, FS_SEEK_END) == 0 &&
		    fs_read(&f, block, sizeof(block)) == (ssize_t)sizeof(block) &&
		    rec_block_ok(block) && block[0] == REC_END) {
			memcpy(&end, block, sizeof(end));
			e->steps = end.steps;
			e->blocks = end.blocks;
			e->end = end.why;
		} else {
			e->end = END_UNFINISHED;
			e->blocks = (uint32_t)(ent.size / REC_BLOCK_BYTES);
		}
		fs_close(&f);
		(*n)++;
	}
	fs_closedir(&dir);
	return 0;
}

static int op_cal_write(const struct strelka_cal *c, void *ctx)
{
	ARG_UNUSED(ctx);
	cal = *c;
	cal_loaded = true;
	orient_init(&orient, sensor_cfg.gyro_dps, cal.gscale, cal.gzero, 0, 0);
	return settings_save_one("strelka/cal", &cal, sizeof(cal)) ? SMP_ERR_UNKNOWN : 0;
}

static int op_cal_read(struct strelka_cal *c, void *ctx)
{
	ARG_UNUSED(ctx);
	*c = cal;
	return 0;
}

static int op_blink(uint32_t seconds, void *ctx)
{
	ARG_UNUSED(ctx);
	if (seconds == 0) {
		k_work_cancel_delayable(&blink_work);
		blink_over(NULL);
		return 0;
	}
	light_set(LIGHT_ORANGE_FAST);
	k_work_reschedule(&blink_work, K_SECONDS(seconds));
	return 0;
}

/* 1 every session erased, 2 the card formatted; refused while the Belka records */
static int op_card(uint8_t what, void *ctx)
{
	struct fs_dir_t dir;
	struct fs_dirent ent;
	int rc = 0;

	ARG_UNUSED(ctx);
	if (recording || card_busy()) {
		return SMP_ERR_BUSY;
	}
	if (!card_present()) {
		return SMP_ERR_BAD_STATE;
	}
	if (what == 2) {
		state = BELKA_ERASING;
		card_unmount();
		rc = fs_mkfs(FS_FATFS, (uintptr_t)"SD:", NULL, 0);   /* FatFs wants the colon */
		card_mount();
		state = BELKA_WAITING;
		return rc ? SMP_ERR_UNKNOWN : 0;
	}
	if (!card_mounted()) {
		return SMP_ERR_BAD_STATE;
	}
	state = BELKA_ERASING;
	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, MOUNT) == 0) {
		while (fs_readdir(&dir, &ent) == 0 && ent.name[0]) {
			size_t len = strlen(ent.name);
			char path[sizeof(MOUNT) + MAX_FILE_NAME + 1];

			if (ent.type == FS_DIR_ENTRY_FILE && len > 5 &&
			    strcmp(ent.name + len - 5, ".STRK") == 0) {
				snprintf(path, sizeof(path), MOUNT "/%s", ent.name);
				if (fs_unlink(path)) {
					rc = SMP_ERR_UNKNOWN;
				}
			}
		}
		fs_closedir(&dir);
	}
	state = BELKA_WAITING;
	return rc;
}

static int op_off(void *ctx)
{
	ARG_UNUSED(ctx);
	record_stop(END_APPLICATION);
	off_asked = true;                /* once the record is closed and the card done */
	return 0;
}

static int op_forget(void *ctx)
{
	ARG_UNUSED(ctx);
	settings_delete("strelka/set");
	settings_delete("strelka/n");
	settings_delete("strelka/cal");
	settings_delete("strelka/gauge");  /* a new cell on J2 starts clean; the records stay */
	k_work_reschedule(&reboot_work, K_MSEC(500));
	return 0;
}

/* the taking, 64/8: Povodok's public key kept, a fresh pair's public key answered */
static int op_key(const uint8_t pub[32], uint8_t pub_out[32], void *ctx)
{
	ARG_UNUSED(ctx);
	if (strelka_x25519_begin(pub_out)) {
		return SMP_ERR_UNKNOWN;
	}
	memcpy(peer_pub, pub, 32);
	peer_pub_set = true;
	return 0;
}

/* the taking, 64/9: the set's key under AES-CCM with the agreed key, then the number, the
 * address and the channel kept and taken */
static int op_join(const struct strelka_join *j, void *ctx)
{
	uint8_t agreed[32];              /* the taking's key its first 16 B */
	struct set_settings fresh;

	ARG_UNUSED(ctx);
	if (!peer_pub_set) {
		return SMP_ERR_BAD_STATE;
	}
	if (strelka_x25519_agree(peer_pub, agreed) ||
	    strelka_ccm_decrypt(agreed, j->nonce, NULL, 0, j->key, KEY_BYTES, fresh.key, j->mic)) {
		return SMP_ERR_ACCESS;
	}
	fresh.address = ((uint32_t)j->addr[0]) | ((uint32_t)j->addr[1] << 8) |
			((uint32_t)j->addr[2] << 16) | ((uint32_t)j->addr[3] << 24);
	fresh.channel = j->ch;
	set = fresh;
	my_number = j->n;
	peer_pub_set = false;
	if (settings_save_one("strelka/set", &set, sizeof(set)) ||
	    settings_save_one("strelka/n", &my_number, sizeof(my_number))) {
		return SMP_ERR_UNKNOWN;
	}
	belka_link_join(&link, my_number, set.address, set.channel);
	return 0;
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
	return 0;
}

static const struct strelka_ops ops = {
	.info = op_info, .session = op_session, .sessions = op_sessions, .cal_write = op_cal_write,
	.cal_read = op_cal_read, .blink = op_blink, .card = op_card, .off = op_off,
	.forget = op_forget, .key = op_key, .join = op_join, .table = op_table,
};

/* ---- the magnets ---------------------------------------------------------------------------- */

static void on_magnet(enum magnet_event e)
{
	switch (e) {
	case MAGNET_A_SHORT:
		light_show_cell(gauge_pct());
		break;
	case MAGNET_A_OFF:
		record_stop(END_MAGNET_A);
		off_asked = true;        /* once the record is closed and the card done */
		break;
	case MAGNET_BOTH_10S:
		light_flash(COLOUR_ORANGE, 2000);
		break;
	case MAGNET_FACTORY:
		op_forget(NULL);         /* the records kept, the unit started again */
		break;
	default:
		break;                   /* magnet B alone does nothing on a Belka */
	}
}

int main(void)
{
	const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
	struct wdt_timeout_cfg wdt_cfg = { .window.max = 5000, .flags = WDT_FLAG_RESET_SOC };
	struct belka_link_cfg link_cfg;
	int wdt_ch;

	hold_init();                     /* the board held on; MCUboot had it first */
	light_init();
	magnet_init(on_magnet);
	/* the board started in hardware when magnet A came; if it has gone by now, it was a
	 * magnet brushed past: off again (FIRMWARE.md, The magnets and the light) */
	if (!magnet_a_held()) {
		hold_release();
	}
	hwinfo_get_device_id((uint8_t *)&chip_id, sizeof(chip_id));
	LOG_INF("Belka, firmware %u.%u.%u, chip %016llx", STRELKA_FW_MAJOR, STRELKA_FW_MINOR,
		STRELKA_FW_PATCH, (unsigned long long)chip_id);
	settings_subsys_init();
	settings_load();                 /* the set's address, key and channel, the number, the
					  * calibration; the gauge's learned values to come */
	link_cfg = (struct belka_link_cfg){ .number = my_number, .chip = chip_id,
					    .address = set.address, .channel = set.channel,
					    .stop_s = session_cfg.stop_s };
	belka_link_init(&link, &link_cfg);
	orient_init(&orient, sensor_cfg.gyro_dps, cal_loaded ? cal.gscale : NULL,
		    cal_loaded ? cal.gzero : NULL, 0, 0);
	clock_init();
	clock_on_data_ready(on_data_ready);
	if (imu_init()) {
		fault = FAULT_SENSOR_SILENT;
	} else {
		imu_configure(&sensor_cfg);     /* RTC_MODE and PIN9 set first: then CLKIN driven */
		clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
		clock_clkin_start(true);
		imu_start();
	}
	if (card_init()) {
		fault = card_fault();
	}
	charger_init();
	if (gauge_init(learned_loaded ? &learned : NULL)) {
		fault = fault ? fault : FAULT_GAUGE_SILENT;
	}
	usb_init();                      /* up when the stack sees VBUS */
	strelka_mgmt_zephyr_init(&ops, NULL);   /* reached over ESB through Povodok: the link's */
	light_set(standing_light());

	/* the watchdog last, after the card's identification and the rest of the start */
	wdt_ch = wdt_install_timeout(wdt, &wdt_cfg);
	wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);

	for (int i = 0;; i++) {
		struct gauge_learned fresh;

		wdt_feed(wdt, wdt_ch);
		for (int j = 0; j < 10; j++) {
			k_sleep(K_MSEC(100));
			charger_service();   /* ten samples a second tell the blink of a fault */
		}
		/* the cell: its state once a second, what the gauge learned kept, the empty cell
		 * closing the record and switching the board off (FIRMWARE.md, The cell empty) */
		if (gauge_service(&fresh)) {
			learned = fresh;
			learned_loaded = true;
			settings_save_one("strelka/gauge", &learned, sizeof(learned));
		}
		if (gauge_present() && gauge_pct() < 20 && fault == FAULT_NONE) {
			fault = FAULT_CELL_LOW;
		} else if (fault == FAULT_CELL_LOW && gauge_pct() >= 20) {
			fault = FAULT_NONE;
		}
		if (gauge_empty() && !charger_powered()) {
			record_stop(END_CELL_LOW);
			off_asked = true;
		}
		light_set(standing_light());
		/* on the cable and not recording, the host has the disk and the file system is
		 * let go; off the cable, or recording, it is the Belka's */
		if (!recording && usb_vbus() && card_mounted() && !card_busy()) {
			card_unmount();
		} else if (!usb_vbus() && !card_mounted() && card_present()) {
			card_mount();
		}
		if (i % 10 == 0) {
			LOG_INF("state %u, fault %u, %u blocks dropped, lagged %u", state, fault,
				card_dropped(), lagged);
		}
		/* the link, when written, drives record_start and record_stop from the SYNC's
		 * state through belka_link_period_end, and the LIVE's items from `state` and
		 * `fault` */
	}
	return 0;
}
