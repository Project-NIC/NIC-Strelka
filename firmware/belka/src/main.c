/* NIC-Strelka — the Belka: it reads its sensor, stamps the samples, stores them and sends them,
 * and nothing else (FIRMWARE.md). This is the skeleton: the PMIC and the power, the light, the magnets,
 * the clock and the sensor, the card, USB, and the record's path from a sample to a block on the
 * card — every sample with its own stamp, captured at its data-ready, from the FLPR's ring
 * (sensor_flpr.c) or, in the fallback, the interrupt, summed into steps by its number on the
 * set's grid (common/steps.c), the steps composed into blocks (common/session.c), the blocks
 * to the card through its buffer (card.c); the live view's turn as a quaternion from every step
 * (common/orient.c). The sensor's thread owns the record: a stop asked from elsewhere — a magnet,
 * the card's fault, the application — is a request it serves between two samples. Strelka's group
 * 64 (common/strelka_mgmt.c) is registered with MCUmgr and does the Belka's part of it on the
 * settings, the card and the record; what starts and stops the record is the SYNC's state, and the
 * SYNC, the LIVEs and the SMP frames over ESB come by the link, shared/link.h, which is not
 * written: the kit decides it first. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include "belka_link.h"
#include "card.h"
#include "clock.h"
#include "crypto.h"
#include "gauge.h"
#include "imu.h"
#include "light.h"
#include "magnet.h"
#include "orient.h"
#include "pmic.h"
#include "power.h"
#include "protocol.h"
#include "quat.h"
#include "record.h"
#include "resample.h"
#include "session.h"
#include "setfile.h"
#include "steps.h"
#include "store.h"
#include "strelka_mgmt_zephyr.h"
#include "usb.h"

LOG_MODULE_REGISTER(belka, LOG_LEVEL_INF);

#define SENSOR_PRIO 2                      /* above the writer, below the radio's work */
#define MOUNT "/SD:"
#define TABLE_MAX 4096                     /* the set's table's text, written after the head */

static uint64_t chip_id;
/* the set, the number 1–63 (0 a new Belka), the calibration and the gauge's state:
 * store.h's, kept across power-ons */
static uint8_t state = BELKA_WAITING, fault = FAULT_NONE;
static struct imu_config sensor_cfg = { 1000, 16, 2000 };
static uint8_t samples_a_step = 10;        /* N: 1 kHz into 100 Hz */
static struct strelka_session session_cfg = { .s = 0, .rate_hz = 1000, .n = 10, .acc_g = 16,
					      .gyro_dps = 2000, .live = 0, .stop_s = 10, .off_s = 600 };
static bool settings_refused;              /* the last 64/1 refused: no record until a good one */
static uint8_t table[TABLE_MAX];
static uint32_t table_len;
static uint8_t peer_pub[32];               /* Povodok's X25519 key from 64/8, for 64/9 */
static bool peer_pub_set;
static struct strelka_x25519 taking_pair;  /* the Belka's own for the taking, 64/8 to 64/9 */
static struct belka_link link;
/* the live view's steps of the period, for the LIVE: the turn as quaternions (common/orient.c)
 * or the raw steps, at the live view's rate, the link taking them at the slot */
static struct orient orient;
static uint8_t live_body[LIVE_BODY_MAX];
static unsigned live_fill;                 /* steps in it, 0–2 */

static enum light standing_light(void);

/* the record's path, the sensor's thread's alone */
static struct steps steps;
static bool disk_opened;                   /* the disk opened for writing until the cable leaves */
static struct resample resampler;          /* the hoof's sensor's samples onto the grid */
static void grid_point(int64_t index, const struct strelka_step *s, bool acc_out, bool gyro_out,
		       void *ctx);
static struct session session;
static bool recording;
static uint32_t lagged;                    /* times the thread found more than one stamp waiting */
#if !defined(CONFIG_STRELKA_SENSOR_FLPR)
/* the stamps from the interrupt, one a data-ready, in their order */
K_MSGQ_DEFINE(stamps, sizeof(uint64_t), 64, 4);
#endif
/* what others ask of the record: served by the sensor's thread between two samples */
static volatile bool stop_asked, off_asked, go_off;
static volatile uint8_t stop_why;

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
 * gave. The link calls it, from the sensor's thread's side, so it waits unused here; -EAGAIN
 * while the last file is still closing, to be asked again the next period */
static __unused int record_start(uint32_t first_period)
{
	struct rec_calibration rec_cal = {
		.temp = store.cal.temp, .time = store.cal.t,
	};
	uint8_t kind = imu_sensor();
	/* the sensor described whole, so that a reader needs no table (RECORD.md) */
	struct rec_sensor sensor = {
		.id = imu_whoami(), .name = imu_name_of(kind), .sheet = imu_sheet_of(kind),
		.acc_channel = imu_acc_channel_of(kind, sensor_cfg.acc_g),
		.acc_range = sensor_cfg.acc_g,
		.acc_step_ng = imu_acc_step_ng_of(kind, sensor_cfg.acc_g),
		.gyro_range = sensor_cfg.gyro_dps,
		.gyro_step_ndps = imu_gyro_step_ndps_of(kind, sensor_cfg.gyro_dps),
		.sensor_hz = imu_sensor_hz(sensor_cfg.rate_hz),
	};
	struct rec_session cfg = {
		.session = session_cfg.s, .belka = store.number, .chip = chip_id,
		.fw = { STRELKA_FW_MAJOR, STRELKA_FW_MINOR, STRELKA_FW_PATCH },
		.rate_hz = sensor_cfg.rate_hz / samples_a_step, .n = samples_a_step,
		.acc_fs = imu_acc_full_scale(sensor_cfg.acc_g),
		.gyro_dps = imu_gyro_full_scale(sensor_cfg.gyro_dps),
		.first_period = first_period, .time_us = session_cfg.t_us,
		.time_period = session_cfg.p_given ? session_cfg.p : first_period,
		.tz_min = session_cfg.tz_min,
		.clock = imu_on_grid() ? CLOCK_STEERED : CLOCK_RESAMPLED,
		.cal = store.cal_loaded ? &rec_cal : NULL, .sensor = &sensor, .place = "", .name = "",
	};
	char name[32];

	memcpy(rec_cal.acc, store.cal.acc, sizeof(rec_cal.acc));
	memcpy(rec_cal.gyro_scale, store.cal.gscale, sizeof(rec_cal.gyro_scale));
	memcpy(rec_cal.gyro_zero, store.cal.gzero, sizeof(rec_cal.gyro_zero));
	if (recording || settings_refused) {
		return -1;
	}
	if (!card_present()) {
		fault = FAULT_NO_CARD;   /* the memory asked of, and none there */
		return -1;
	}
	if (card_busy()) {
		return -EAGAIN;          /* the last file still closing: no fault, the link asks
					  * again the next period */
	}
	usb_allow(false);                /* the card is the record's now, not the host's */
	if (card_mount()) {
		fault = card_fault();
		usb_allow(true);
		return -1;
	}
	snprintf(name, sizeof(name), "S%05uB%02u.STRK", session_cfg.s, store.number);
	cfg.name = name;
	if (card_open(session_cfg.s, store.number, cfg.rate_hz)) {
		fault = card_fault();
		usb_allow(true);
		return -1;
	}
	session_begin(&session, &cfg, to_card, NULL);
	if (table_len) {
		session_table(&session, (const char *)table, table_len);
	}
	steps_begin(&steps, sensor_cfg.rate_hz, samples_a_step, first_period, step_out, NULL);
	/* across a gap of two and a half of the sensor's own periods at most: a lost sample bridged,
	 * two not */
	resample_begin(&resampler, CLOCK_HZ / sensor_cfg.rate_hz,
		       CLOCK_HZ * 5u / (2u * imu_sensor_hz(sensor_cfg.rate_hz)), grid_point, NULL);
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

#if !defined(CONFIG_STRELKA_SENSOR_FLPR)
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
#endif

/* a sample with its stamp into the session's steps, on the set's grid */
/* a point of the grid drawn between two samples of a sensor off the set's clock */
static void grid_point(int64_t index, const struct strelka_step *s, bool acc_out, bool gyro_out,
		       void *ctx)
{
	ARG_UNUSED(ctx);
	steps_sample(&steps, index, clock_grid_stamp(index), s, acc_out, gyro_out);
}

/* the set given by the cable (common/setfile.h): STRELKA.SET on the card, written by the
 * application or by hand while the host had the disk, taken as the Belka has its card back —
 * read, kept, its bytes overwritten and the file deleted, so that the key stays nowhere but in the
 * settings; the light green 2 s taken, red 2 s refused (FIRMWARE.md, What a unit keeps) */
static void take_set_file(void)
{
	static char text[SETFILE_MAX + 1];      /* a byte more: a longer file is refused whole,
						 * never taken cut at a line's middle */
	struct fs_file_t f;
	struct setfile sf;
	ssize_t n;
	bool ok;

	fs_file_t_init(&f);
	if (fs_open(&f, MOUNT "/" SETFILE_NAME, FS_O_RDWR)) {
		return;                          /* none given */
	}
	n = fs_read(&f, text, sizeof(text));
	ok = n > 0 && setfile_parse(text, (size_t)n, &sf) == 0;
	memset(text, 0, sizeof(text));
	if (n > 0 && fs_seek(&f, 0, FS_SEEK_SET) == 0) {
		fs_write(&f, text, (size_t)n);   /* the key's bytes gone from the card */
		fs_sync(&f);
	}
	fs_close(&f);
	fs_unlink(MOUNT "/" SETFILE_NAME);
	if (ok) {
		struct store_set set = { .address = sf.address, .channel = sf.channel };

		memcpy(set.key, sf.key, sizeof(set.key));
		ok = store_join(&set, sf.number) == 0;
		if (ok) {
			belka_link_join(&link, store.number, store.set.address, store.set.channel);
		}
		memset(&set, 0, sizeof(set));
	}
	memset(&sf, 0, sizeof(sf));
	LOG_INF("the set file %s", ok ? "taken" : "refused");
	light_flash(ok ? COLOUR_GREEN : COLOUR_RED, 2000);
}

static void sample_in(uint64_t stamp, const struct strelka_step *sample, bool acc_out,
		      bool gyro_out)
{
	if (!recording) {
		return;
	}
	if (imu_on_grid()) {
		int64_t index = clock_sample_index(stamp);

		if (index >= 0) {
			steps_sample(&steps, index, stamp, sample, acc_out, gyro_out);
		}
	} else if (clock_origin_known()) {
		/* the hoof's sensor on its own clock: onto the grid by its stamps (resample.h) */
		resample_sample(&resampler, clock_since_origin(stamp), sample, acc_out, gyro_out);
	}
	/* the live view's step, as quaternions or raw, goes to the link */
}

/* what others asked meanwhile, served between two samples */
static void between(void)
{
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
		go_off = true;           /* main's thread takes it, on its own larger stack */
	}
}

static void sensor_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		struct strelka_step sample;
		uint64_t stamp;
		bool acc_out, gyro_out;

#if defined(CONFIG_STRELKA_SENSOR_FLPR)
		/* the FLPR reads every sample into the ring: emptied every millisecond, up to 32 a
		 * time at 32 kHz; what it lost or passed counted as lagged (FIRMWARE.md, The sensor
		 * on the FLPR) */
		k_msleep(1);
		while (imu_ring_pop(&sample, &stamp, &acc_out, &gyro_out)) {
			sample_in(stamp, &sample, acc_out, gyro_out);
		}
		lagged = imu_dropped();
#else
		if (k_msgq_get(&stamps, &stamp, K_MSEC(100)) == 0) {
			if (k_msgq_num_used_get(&stamps) > 0) {
				lagged++;
			}
			if (imu_read_step(&sample, &acc_out, &gyro_out)) {
				fault = FAULT_SENSOR_SILENT;
			} else {
				sample_in(stamp, &sample, acc_out, gyro_out);
			}
		}
#endif
		between();
	}
}

K_THREAD_DEFINE(sensor, 2048, sensor_thread, NULL, NULL, NULL, SENSOR_PRIO, 0, 0);

/* ---- the light's blink and the delayed deeds ------------------------------------------------ */

/* the standing pattern (FIRMWARE.md, The magnets and the light): measuring, a fault, the
 * PMIC's own on the cable, the cell low, else waiting in the set */
static enum light standing_light(void)
{
	if (recording) {
		return LIGHT_GREEN_FAST;
	}
	if (fault && fault != FAULT_CELL_LOW) {
		return LIGHT_RED_STEADY;
	}
	if (pmic_vbus()) {
		return LIGHT_CHARGER;    /* green while it charges, red for its fault: the PMIC's */
	}
	return gauge_pct() < 20 && gauge_present() ? LIGHT_RED_BLINK : LIGHT_GREEN_SLOW;
}

static void blink_over(struct k_work *w)
{
	ARG_UNUSED(w);
	light_set(standing_light());
}
static K_WORK_DELAYABLE_DEFINE(blink_work, blink_over);

/* the restart after the factory settings, or after a set given: the unit stays on (power.h) */
static bool factory_restart;

static void reboot_now(struct k_work *w)
{
	ARG_UNUSED(w);
	power_restart(factory_restart);
}
static K_WORK_DELAYABLE_DEFINE(reboot_work, reboot_now);

/* ---- Strelka's group 64, the Belka's side (COMMANDS.md) ------------------------------------- */

static int op_info(struct strelka_info *out, void *ctx)
{
	ARG_UNUSED(ctx);
	out->chip = chip_id;
	out->n = store.number;
	out->who = WHO_BELKA;
	out->fw[0] = STRELKA_FW_MAJOR;
	out->fw[1] = STRELKA_FW_MINOR;
	out->fw[2] = STRELKA_FW_PATCH;
	out->sensor = imu_whoami();
	out->card_in = card_present();
	out->card_free_mb = card_free_mb();
	out->card_fs = card_fs();
	out->cell_pct = gauge_pct();
	out->cell_mv = gauge_mv();
	out->temp_centi = imu_temp_centi();
	out->state = state;
	out->fault = fault;
	out->cal_time = store.cal_loaded ? store.cal.t : 0;
	return 0;
}

static int op_session(const struct strelka_session *s, void *ctx)
{
	ARG_UNUSED(ctx);
	if (recording) {
		return SMP_ERR_BAD_STATE;
	}
	if (!imu_rate_ok(s->rate_hz) || !imu_ranges_ok(s->acc_g, s->gyro_dps)) {
		/* refused: the Belka does not record at the SYNC's word and says so in its state,
		 * since Povodok answers the application before the Belkas do (COMMANDS.md, 64/1) */
		settings_refused = true;
		fault = FAULT_SETTINGS_REFUSED;
		return SMP_ERR_INVAL;
	}
	settings_refused = false;
	if (fault == FAULT_SETTINGS_REFUSED) {
		fault = FAULT_NONE;
	}
	session_cfg = *s;
	sensor_cfg.rate_hz = s->rate_hz;
	sensor_cfg.acc_g = s->acc_g;
	sensor_cfg.gyro_dps = s->gyro_dps;
	samples_a_step = s->n;
	link.cfg.stop_s = s->stop_s;
	link.cfg.off_s = s->off_s;
	imu_configure(&sensor_cfg);
	clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
	orient_init(&orient, imu_gyro_full_scale(sensor_cfg.gyro_dps),
		    store.cal_loaded ? store.cal.gscale : NULL,
		    store.cal_loaded ? store.cal.gzero : NULL, 0, 0);
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
	static char recording[32];
	uint64_t recorded = 0;
	bool is_recording = card_recording(recording, sizeof(recording), &recorded);

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
		bool live_file = is_recording && strcmp(ent.name, recording) == 0;

		if (live_file) {
			/* the file being written: its size is the room taken, its own count the
			 * record so far, and no end yet — the room's last block never read for one */
			ent.size = recorded;
			e->bytes = (uint32_t)recorded;
		}
		if (fs_read(&f, block, sizeof(block)) == (ssize_t)sizeof(block) && rec_block_ok(block)) {
			memcpy(&head, block, sizeof(head));
			e->s = head.session;
			e->p0 = head.first_period;
		}
		if (!live_file && ent.size >= 2 * REC_BLOCK_BYTES &&
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
	int rc = store_cal(c);

	orient_init(&orient, imu_gyro_full_scale(sensor_cfg.gyro_dps), store.cal.gscale,
		    store.cal.gzero, 0, 0);
	return rc ? SMP_ERR_UNKNOWN : 0;
}

static int op_cal_read(struct strelka_cal *c, void *ctx)
{
	ARG_UNUSED(ctx);
	*c = store.cal;
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

/* 1 every session erased, 2 the card formatted, 3 the disk opened for writing on this cable;
 * refused while the Belka records. Each comes only right after the lock written again on the
 * application's connection, its password given again (common/smp.c) */
static int op_card(uint8_t what, void *ctx)
{
	int rc;

	ARG_UNUSED(ctx);
	if (recording || card_busy()) {
		return SMP_ERR_BUSY;
	}
	if (!card_present()) {
		return SMP_ERR_BAD_STATE;
	}
	if (what == 3) {
		/* the disk opened for writing on the cable, until it leaves: the application's word
		 * (FIRMWARE.md, USB) */
		if (!usb_vbus()) {
			return SMP_ERR_BAD_STATE;
		}
		disk_opened = true;
		usb_disk_protect(false);
		light_flash(COLOUR_ORANGE, 2000);
		return 0;
	}
	state = BELKA_ERASING;
	rc = card_erase(what);
	if (rc) {
		state = BELKA_WAITING;
		return rc == -EBUSY ? SMP_ERR_BUSY : rc == -ENODEV ? SMP_ERR_BAD_STATE :
		       rc == -EINVAL ? SMP_ERR_INVAL : SMP_ERR_UNKNOWN;
	}
	if (!card_busy()) {
		state = BELKA_WAITING;           /* done at once; Kryska's runs on, the loop sees */
	}
	return 0;
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
	store_forget();                  /* a new cell on J2 starts clean; the records stay */
	factory_restart = true;
	k_work_reschedule(&reboot_work, K_MSEC(2000));   /* after the orange's 2 s */
	return 0;
}

/* the taking, 64/8: Povodok's public key kept, a fresh pair's public key answered */
static int op_key(const uint8_t pub[32], uint8_t pub_out[32], void *ctx)
{
	ARG_UNUSED(ctx);
	if (strelka_x25519_begin(&taking_pair, pub_out)) {
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
	struct store_set fresh;

	ARG_UNUSED(ctx);
	if (!peer_pub_set) {
		return SMP_ERR_BAD_STATE;
	}
	if (strelka_x25519_agree(&taking_pair, peer_pub, agreed) ||
	    strelka_ccm_decrypt(agreed, j->nonce, NULL, 0, j->key, KEY_BYTES, fresh.key, j->mic)) {
		return SMP_ERR_ACCESS;
	}
	fresh.address = ((uint32_t)j->addr[0]) | ((uint32_t)j->addr[1] << 8) |
			((uint32_t)j->addr[2] << 16) | ((uint32_t)j->addr[3] << 24);
	fresh.channel = j->ch;
	peer_pub_set = false;
	if (store_join(&fresh, j->n)) {
		return SMP_ERR_UNKNOWN;
	}
	belka_link_join(&link, store.number, store.set.address, store.set.channel);
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
	case MAGNET_B_START:
		/* magnet B held 3 s on the cable: the disk opened for writing until the cable leaves,
		 * the hand on the unit its key (FIRMWARE.md, USB) */
		if (usb_vbus() && !recording) {
			disk_opened = true;
			usb_disk_protect(false);
			light_flash(COLOUR_ORANGE, 2000);
		}
		break;
	case MAGNET_A_SHORT:
		light_show_cell(gauge_pct());
		break;
	case MAGNET_A_OFF:
		record_stop(END_MAGNET_A);
		off_asked = true;        /* once the record is closed and the card done */
		break;
	case MAGNET_BOTH_8S:
		light_flash(COLOUR_ORANGE, 2000);
		break;
	case MAGNET_FACTORY:
		op_forget(NULL);         /* the records kept, the unit started again */
		break;
	default:
		break;                   /* magnet B otherwise does nothing on a Belka */
	}
}

int main(void)
{
	const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
	struct wdt_timeout_cfg wdt_cfg = { .window.max = 5000, .flags = WDT_FLAG_RESET_SOC };
	struct belka_link_cfg link_cfg;
	int wdt_ch;

	/* the PMIC, the settings kept and the gauge first: then whether the unit is switched on —
	 * magnet A still held since it took the PMIC out of ship mode — or woken by the cable alone,
	 * which idles in power_start, or a magnet brushed past, which goes back into ship mode
	 * (FIRMWARE.md, The PMIC; The magnets and the light) */
	if (pmic_init()) {
		fault = FAULT_PMIC_SILENT;
	}
	store_load();                    /* the set's address, key and channel, the number, the
					  * calibration, the gauge's state */
	gauge_init(store.gauge_len ? store.gauge : NULL, store.gauge_len);
	light_init();
	power_start(false);
	magnet_init(on_magnet);
	hwinfo_get_device_id((uint8_t *)&chip_id, sizeof(chip_id));
	LOG_INF("Belka, firmware %u.%u.%u, chip %016llx", STRELKA_FW_MAJOR, STRELKA_FW_MINOR,
		STRELKA_FW_PATCH, (unsigned long long)chip_id);
	link_cfg = (struct belka_link_cfg){ .number = store.number, .chip = chip_id,
					    .address = store.set.address,
					    .channel = store.set.channel,
					    .stop_s = session_cfg.stop_s, .sleep = true };
	belka_link_init(&link, &link_cfg);
	orient_init(&orient, imu_gyro_full_scale(sensor_cfg.gyro_dps),
		    store.cal_loaded ? store.cal.gscale : NULL,
		    store.cal_loaded ? store.cal.gzero : NULL, 0, 0);
	clock_init();
#if defined(CONFIG_STRELKA_SENSOR_FLPR)
	clock_data_ready_irq(false);    /* the capture stays; the FLPR watches it */
#else
	clock_on_data_ready(on_data_ready);
#endif
	if (imu_init()) {
		fault = FAULT_SENSOR_SILENT;
	} else {
		/* the ICM: RTC_MODE and PIN9 set first, then CLKIN driven; the LSM6DSV16X: its
		 * ODR-triggered mode set first, then the reference, then ODRsel (AN5763, 3.3) */
		imu_configure(&sensor_cfg);
		clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
		if (imu_on_grid()) {
			clock_pin9_reference(imu_sensor() == IMU_LSM6DSV16X);
			clock_clkin_start(true);
		} else {
			clock_off_grid();    /* the LSM6DSV320X: pin 9 left alone */
		}
		imu_start();
		/* the gyro's full scale known once the chip is */
		orient_init(&orient, imu_gyro_full_scale(sensor_cfg.gyro_dps),
			    store.cal_loaded ? store.cal.gscale : NULL, store.cal_loaded ? store.cal.gzero : NULL, 0, 0);
	}
	if (card_init()) {
		fault = card_fault();
	} else if (card_mounted()) {
		take_set_file();                 /* given while the Belka was off */
	}
	/* the disk read alone on the cable while the Belka is in a set: its records safe from the
	 * host; a new Belka's open, for its STRELKA.SET; Kryska's set file over SMP the same */
	usb_disk_protect(store.set.address != 0);
	card_set_file_allowed(store.set.address == 0);
	usb_init();                      /* up when the stack sees VBUS */
	strelka_mgmt_zephyr_init(&ops, NULL);   /* reached over ESB through Povodok: the link's */
	light_set(standing_light());

	/* the watchdog last, after the card's identification and the rest of the start */
	wdt_ch = wdt_install_timeout(wdt, &wdt_cfg);
	wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	power_watchdog_taken();          /* fed by the loop from here; until now by power.c */

	for (int i = 0;; i++) {
		wdt_feed(wdt, wdt_ch);
		k_sleep(K_SECONDS(1));
		if (go_off) {
			power_off();     /* ship mode, dark as the magnet leaves; on the cable, idle */
		}
		/* the cell: its state once a second, the gauge's kept once an hour, the empty cell
		 * closing the record and switching the unit off (FIRMWARE.md, The PMIC) */
		gauge_service();
		if (i % 3600 == 3599) {
			power_keep();
		}
		/* the set's channel kept as the link moves it, so that a Belka switched on later
		 * finds the set where it went (store.h) */
		store_channel(link.channel);
		if (gauge_present() && gauge_pct() < 20 && fault == FAULT_NONE) {
			fault = FAULT_CELL_LOW;
		} else if (fault == FAULT_CELL_LOW && gauge_pct() >= 20) {
			fault = FAULT_NONE;
		}
		if (gauge_empty() && !pmic_vbus()) {
			record_stop(END_CELL_LOW);
			off_asked = true;
		}
		light_set(standing_light());
		/* the disk's protection: in a set, read alone unless opened on this cable */
		if (!usb_vbus()) {
			disk_opened = false;
		}
		usb_disk_protect(store.set.address != 0 && !disk_opened);
		/* Kryska's STRELKA.SET over SMP on the serial port, where a disk would be open; given,
		 * taken as from a disk; its erase runs on in its writer */
		card_set_file_allowed(store.set.address == 0 || disk_opened);
		if (card_set_file_given()) {
			take_set_file();
		}
		if (state == BELKA_ERASING && !card_busy()) {
			state = BELKA_WAITING;
		}
		/* on the cable and not recording, the host has the disk and the file system is
		 * let go; off the cable, or recording, it is the Belka's */
		if (!recording && usb_vbus() && card_mounted() && !card_busy()) {
			card_unmount();
		} else if (!usb_vbus() && !card_mounted() && card_present()) {
			if (card_mount() == 0) {
				take_set_file();         /* given on the cable just left */
			}
		}
		if (i % 10 == 0) {
			LOG_INF("state %u, fault %u, %u blocks dropped, lagged %u", state, fault,
				card_dropped(), lagged);
		}
		/* the link, when written, drives record_start and record_stop from the SYNC's
		 * state through belka_link_period_end, and the LIVE's items from `state` and
		 * `fault`; asleep, the sensor stopped, the card's supply off and the radio and the
		 * crystal held only for the state's period; off, the board let go (FIRMWARE.md,
		 * The power) */
	}
	return 0;
}
