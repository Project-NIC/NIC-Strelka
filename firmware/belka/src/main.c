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
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt.h>
#include "belka_link.h"
#include "cal.h"
#include "card.h"
#include "clock.h"
#include "crypto.h"
#include "gauge.h"
#include "imu.h"
#include "imu_tables.h"
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
static struct imu_config sensor_cfg = { 1000, 16, 2000, 0, 10 };
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
static struct cal_gain live_gain;          /* the calibration on a step, common/cal.h */
static struct cal_gain live_gain_hg;       /* the same with the high-g channel's three */
/* both channels kept: this period's LIVE body the high-g's, the low-g having clipped in it — the
 * link sends it as code 3 or 6 (common/belka_link.h, live_high_g) */
static bool live_high_g;
static bool live_clip_pending;  /* the low-g clipped in a step between the live view's own */
static struct strelka_step live_raw[LIVE_STEPS_MAX];     /* the period's raw steps, and */
static int16_t live_raw_hg[LIVE_STEPS_MAX][3];           /* their high-g threes */
static uint8_t live_body[LIVE_BODY_MAX];
static unsigned live_fill;                 /* steps in it, 0–2 */

static enum light standing_light(void);

/* the record's path, the sensor's thread's alone */
static struct steps steps;
static bool disk_opened;                   /* the disk opened for writing until the cable leaves */
static struct resample resampler;          /* the hoof's sensor's samples onto the grid */
static void grid_point(int64_t index, const struct strelka_step *s, const int16_t *hg,
		       bool acc_out, bool gyro_out, void *ctx);
static struct session session;
static bool recording;
static void cal_drop(void);
static void cal_feed(const struct strelka_step *s, const int16_t *hg, uint64_t stamp);
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

/* the live view's calibration and orientation set again — the session's ranges, the calibration
 * kept: whenever either changes (PRINCIPLE.md, The LIVE) */
static void live_init(void)
{
	uint8_t kind = imu_sensor();

	cal_gain_init(&live_gain, store.cal_loaded ? store.cal.acc : NULL, store.cal.gscale,
		      store.cal.gzero, store.cal.range, sensor_cfg.acc_g, sensor_cfg.gyro_dps,
		      imu_acc_step_ng_of(kind, sensor_cfg.acc_g));
	if (sensor_cfg.hg) {
		/* the high-g channel's three by its own readings, range and step, the gyro as ever */
		uint16_t range[2] = { store.cal.hg_range, store.cal.range[1] };

		cal_gain_init(&live_gain_hg, store.cal_loaded ? store.cal.hg : NULL, store.cal.gscale,
			      store.cal.gzero, range, sensor_cfg.hg, sensor_cfg.gyro_dps,
			      imu_acc_step_ng_of(kind, sensor_cfg.hg));
	}
	orient_init(&orient, imu_gyro_full_scale(sensor_cfg.gyro_dps), 0, 0);
}

/* the live view's step: the samples' mean over its own period, 10 or 20 ms, taken as they come, so
 * that the live view waits for no filter — the record's waits ten steps (common/resample.c); every
 * such step calibrated turns the orientation on, and a quaternion or the calibrated step goes into
 * the period's LIVE body; the record keeps its own steps. acc_out: a sample of the step out of the
 * accelerometer's range, the step a mean that a clipped peak need not leave at the range's end */
static void live_step(const struct strelka_step *raw, const int16_t *hg, bool acc_out)
{
	uint8_t code = session_cfg.live;
	unsigned live_hz = (code == S0_QUAT_100 || code == S0_RAW_100) ? 100u : 50u;
	unsigned steps_a_period = live_steps(code);
	struct strelka_step cal;
	const struct strelka_step *step = &cal;

	/* both channels kept: the low-g clipped in this step, or in one between the live view's */
	bool clip = hg != NULL && raw->a[0] != REC_STEP_MISSING &&
		    (acc_out || raw->a[0] == INT16_MAX || raw->a[0] == INT16_MIN ||
		     raw->a[1] == INT16_MAX || raw->a[1] == INT16_MIN ||
		     raw->a[2] == INT16_MAX || raw->a[2] == INT16_MIN);

	cal_gain_step(&live_gain, raw, &cal);
	if (cal.a[0] != REC_STEP_MISSING) {      /* a step lost turns nothing */
		orient_step(&orient, step, 1.0f / live_hz);
	}
	if (code == S0_LIVE_OFF) {
		live_clip_pending = live_clip_pending || clip;
		return;
	}
	if (live_fill >= steps_a_period) {
		live_fill = 0;                   /* the period's body taken by the link, or left */
		live_high_g = false;
	}
	if (code == S0_RAW_50 || code == S0_RAW_100) {
		bool clipped = clip || live_clip_pending;

		live_clip_pending = false;
		live_raw[live_fill] = *raw;
		for (int i = 0; i < 3; i++) {
			live_raw_hg[live_fill][i] = hg ? hg[i] : REC_STEP_MISSING;
		}
		/* both channels kept and the low-g clipped: the whole period from the high-g, its
		 * steps already in the body taken again from theirs, the link's code 3 or 6 */
		if (hg != NULL && clipped && !live_high_g) {
			live_high_g = true;
			for (unsigned j = 0; j < live_fill; j++) {
				struct strelka_step h = {
					{ live_raw_hg[j][0], live_raw_hg[j][1], live_raw_hg[j][2] },
					{ live_raw[j].g[0], live_raw[j].g[1], live_raw[j].g[2] } };
				struct strelka_step c;

				cal_gain_step(&live_gain_hg, &h, &c);
				memcpy(live_body + j * STEP_BYTES, &c, STEP_BYTES);
			}
		}
		if (live_high_g && hg != NULL) {
			struct strelka_step h = { { hg[0], hg[1], hg[2] },
						  { raw->g[0], raw->g[1], raw->g[2] } };

			cal_gain_step(&live_gain_hg, &h, &cal);
		}
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
static void step_out(uint32_t k, const struct strelka_step *step, const int16_t *hg, bool acc_out,
		     bool gyro_out, uint64_t first_stamp, void *ctx)
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
	ARG_UNUSED(k);
	session_step(&session, step, sensor_cfg.hg ? hg : NULL, &info, imu_temp_centi(), gauge_mv());
}

/* the live view's own steps, from the samples as they come: their mean over each 10 or 20 ms of
 * the set's time, by their stamps */
static struct {
	int64_t index;
	int32_t sum[9];
	uint16_t n;
	bool acc_out, hg;
} live_acc;

static void live_sample(int64_t at, const struct strelka_step *s, const int16_t *hg, bool acc_out)
{
	uint8_t code = session_cfg.live;
	int64_t per = (code == S0_QUAT_100 || code == S0_RAW_100) ? CLOCK_HZ / 100 : CLOCK_HZ / 50;
	int64_t idx = at >= 0 ? at / per : -1;

	if (code == S0_LIVE_OFF || idx < 0) {
		return;
	}
	if (live_acc.n && idx != live_acc.index) {
		struct strelka_step m;
		int16_t mh[3];
		int32_t n = live_acc.n;

		for (int j = 0; j < 3; j++) {
			m.a[j] = (int16_t)(live_acc.sum[j] / n);
			m.g[j] = (int16_t)(live_acc.sum[3 + j] / n);
			mh[j] = (int16_t)(live_acc.sum[6 + j] / n);
		}
		live_step(&m, live_acc.hg ? mh : NULL, live_acc.acc_out);
		live_acc.n = 0;
	}
	if (!live_acc.n) {
		memset(&live_acc, 0, sizeof(live_acc));
		live_acc.index = idx;
		live_acc.hg = hg != NULL;
	}
	for (int j = 0; j < 3; j++) {
		live_acc.sum[j] += s->a[j];
		live_acc.sum[3 + j] += s->g[j];
		live_acc.sum[6 + j] += hg ? hg[j] : 0;
	}
	live_acc.acc_out = live_acc.acc_out || acc_out;
	live_acc.hg = live_acc.hg && hg != NULL;
	live_acc.n++;
}

/* the record begins at the first sample of `first_period`, the period after the first SYNC heard
 * saying it runs; the head says what the session command, 64/1, the calibration and the taking
 * gave. The link calls it, from the sensor's thread's side, so it waits unused here; -EAGAIN
 * while the last file is still closing, to be asked again the next period */
static __unused int record_start(uint32_t first_period)
{
	static struct rec_group groups[REC_GROUPS_MAX];
	struct rec_calibration rec_cal = { .hg_range = store.cal.hg_range };
	uint8_t kind = imu_sensor();
	/* the sensor's groups, its steps the sheet's, so that a reader needs no table (RECORD.md,
	 * The groups); both channels kept, the high-g in its own blocks */
	struct rec_imu imu = {
		.acc_range = sensor_cfg.acc_g,
		.acc_step_ng = imu_acc_step_ng_of(kind, sensor_cfg.acc_g),
		.high_g = imu_acc_channel_of(kind, sensor_cfg.acc_g) != 0,
		.gyro_range = sensor_cfg.gyro_dps,
		.gyro_step_ndps = imu_gyro_step_ndps_of(kind, sensor_cfg.gyro_dps),
		.hg_range = sensor_cfg.hg,
		.hg_step_ng = sensor_cfg.hg ? imu_acc_step_ng_of(kind, sensor_cfg.hg) : 0,
		.cal = store.cal_loaded ? &rec_cal : NULL,
	};
	struct rec_session cfg = {
		.session = session_cfg.s, .belka = store.number, .chip = chip_id,
		.fw = { STRELKA_FW_MAJOR, STRELKA_FW_MINOR, STRELKA_FW_PATCH },
		.rate_hz = sensor_cfg.rate_hz / samples_a_step, .n = samples_a_step,
		.first_period = first_period, .time_us = session_cfg.t_us,
		.time_period = session_cfg.p_given ? session_cfg.p : first_period,
		.tz_min = session_cfg.tz_min,
		.clock = imu_on_grid() ? CLOCK_STEERED : CLOCK_RESAMPLED,
		.sensor = imu_name_of(kind),
		.cal_temp = store.cal_loaded ? store.cal.temp : 0,
		.cal_time = store.cal_loaded ? store.cal.t : 0,
		.groups = groups, .hg = sensor_cfg.hg != 0,
	};

	memcpy(rec_cal.acc, store.cal.acc, sizeof(rec_cal.acc));
	memcpy(rec_cal.gyro_scale, store.cal.gscale, sizeof(rec_cal.gyro_scale));
	memcpy(rec_cal.gyro_zero, store.cal.gzero, sizeof(rec_cal.gyro_zero));
	memcpy(rec_cal.range, store.cal.range, sizeof(rec_cal.range));
	memcpy(rec_cal.hg, store.cal.hg, sizeof(rec_cal.hg));
	cfg.n_groups = rec_imu_groups(groups, &imu);
	if (recording || settings_refused) {
		return -1;
	}
	cal_drop();                      /* a calibration under way is over */
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
	/* the sensor on its own clock, 2 kHz and below: each step filtered at its centre from the
	 * samples by their stamps; a lost sample put back on its line, a gap of more than two and a
	 * half of the sensor's own periods left missing (common/resample.c) */
	{
		uint32_t own = CLOCK_HZ / imu_sensor_hz(sensor_cfg.rate_hz, sensor_cfg.n);

		resample_begin(&resampler, CLOCK_HZ / sensor_cfg.rate_hz, samples_a_step,
			       (int64_t)first_period * (sensor_cfg.rate_hz / STRELKA_PERIODS_A_S), own,
			       own * 5u / 2u, grid_point, NULL);
	}
	recording = true;
	state = BELKA_RECORDING;
	light_set(LIGHT_GREEN_FAST);
	if (IS_ENABLED(CONFIG_STRELKA_BUCK2_PWM_RECORDING)) {
		(void)pmic_buck2_pwm(true);      /* Kryska's buck quiet while it records */
	}
	return 0;
}

/* the sensor's thread's own: the end written, the card told to close */
static void record_end(uint8_t why)
{
	if (!recording) {
		return;
	}
	if (!imu_on_grid()) {
		resample_flush(&resampler);      /* the last steps from what there is */
	}
	steps_end(&steps);
	session_end(&session, why, card_dropped());
	card_close();
	recording = false;
	if (IS_ENABLED(CONFIG_STRELKA_BUCK2_PWM_RECORDING)) {
		(void)pmic_buck2_pwm(false);
	}
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

/* the sensor's clock as its rate asks (imu_tables.h): steered by pin 9 above 2 kHz — the
 * LSM6DSV16X's reference or an ICM's CLKIN, the sensor set first (AN5763, 3.3) — or its own at
 * 2 kHz and below, pin 9 left alone and the samples filtered onto the grid; changed only when the
 * way changes, so that a steered loop keeps its lock */
static void sensor_clock(bool first)
{
	static bool steered;
	bool want = imu_on_grid();

	if (!first && want == steered) {
		return;
	}
	steered = want;
	if (want) {
		clock_pin9_reference(imu_sensor() == IMU_LSM6DSV16X);
		clock_clkin_start(true);
	} else {
		if (!first) {
			clock_clkin_stop();
		}
		clock_off_grid();
	}
}

/* a sample with its stamp into the session's steps, on the set's grid */
/* a point of the grid drawn between two samples of a sensor off the set's clock */
static void grid_point(int64_t index, const struct strelka_step *s, const int16_t *hg,
		       bool acc_out, bool gyro_out, void *ctx)
{
	ARG_UNUSED(ctx);
	steps_sample(&steps, index, clock_grid_stamp(index), s, hg, acc_out, gyro_out);
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

/* a sample with its stamp; hg its high-g three, both channels kept, else NULL */
static void sample_in(uint64_t stamp, const struct strelka_step *sample, const int16_t *hg,
		      bool acc_out, bool gyro_out)
{
	if (!recording) {
		cal_feed(sample, hg, stamp);     /* the calibration's, 64/22, at rest */
		return;
	}
	if (clock_origin_known()) {
		live_sample(clock_since_origin(stamp), sample, hg, acc_out);
	}
	if (imu_on_grid()) {
		int64_t index = clock_sample_index(stamp);

		if (index >= 0) {
			steps_sample(&steps, index, stamp, sample, hg, acc_out, gyro_out);
		}
	} else if (clock_origin_known()) {
		/* the hoof's sensor on its own clock: onto the grid by its stamps (resample.h) */
		resample_sample(&resampler, clock_since_origin(stamp), sample, hg, acc_out,
				gyro_out);
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
		int16_t hg[3];
		uint64_t stamp;
		bool acc_out, gyro_out;

#if defined(CONFIG_STRELKA_SENSOR_FLPR)
		/* the FLPR reads every sample into the ring: emptied every millisecond, up to 32 a
		 * time at 32 kHz; what it lost or passed counted as lagged (FIRMWARE.md, The sensor
		 * on the FLPR) */
		k_msleep(1);
		while (imu_ring_pop(&sample, hg, &stamp, &acc_out, &gyro_out)) {
			sample_in(stamp, &sample, sensor_cfg.hg ? hg : NULL, acc_out, gyro_out);
		}
		lagged = imu_dropped();
#else
		if (k_msgq_get(&stamps, &stamp, K_MSEC(100)) == 0) {
			if (k_msgq_num_used_get(&stamps) > 0) {
				lagged++;
			}
			if (imu_read_step(&sample, hg, &acc_out, &gyro_out)) {
				fault = FAULT_SENSOR_SILENT;
			} else {
				sample_in(stamp, &sample, sensor_cfg.hg ? hg : NULL, acc_out, gyro_out);
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

/* 8/0, a sector the card cannot read: zeros in its place, to the sector's end, and the download
 * goes on, the reader dropping the block by its CRC (FIRMWARE.md, A session; the patch in
 * patches/) */
ssize_t fs_mgmt_download_read_failed(uint64_t off, uint64_t len, uint8_t *data, size_t max)
{
	size_t n = REC_BLOCK_BYTES - (size_t)(off % REC_BLOCK_BYTES);

	if (off >= len) {
		return -EIO;
	}
	n = MIN(n, max);
	n = (size_t)MIN((uint64_t)n, len - off);
	memset(data, 0, n);
	return (ssize_t)n;
}

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
	/* Kryska by its build; Zvjozdočka by its ICM */
	out->kind = IS_ENABLED(CONFIG_STRELKA_KRYSKA) ? KIND_KRYSKA :
		    (out->sensor == WHOAMI_42686P || out->sensor == WHOAMI_42688P) ? KIND_ZVJOZDOCKA :
		    KIND_BELKA;
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
	if (!imu_rate_ok(s->rate_hz) || !imu_ranges_ok(s->acc_g, s->gyro_dps) ||
	    (s->hg && imu_sensor() == IMU_LSM6DSV320X &&
	     (!imu_ranges_ok(s->hg, s->gyro_dps) || s->rate_hz > card_both_most_hz()))) {
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
	sensor_cfg.hg = imu_sensor() == IMU_LSM6DSV320X ? s->hg : 0;   /* both: the hoof's chip */
	samples_a_step = s->n;
	sensor_cfg.n = s->n;
	link.cfg.stop_s = s->stop_s;
	link.cfg.off_s = s->off_s;
	/* not among the session's units: no record, no LIVE, as at rest (COMMANDS.md, 64/1) */
	link.cfg.aside = s->units_given &&
			 !(s->units[store.number / 8] & (1u << (store.number % 8)));
	belka_link_warm(&link);          /* awake ten minutes: the warm-up before the faces */
	imu_configure(&sensor_cfg);
	clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
	sensor_clock(false);
	live_init();
	live_fill = 0;
	live_acc.n = 0;
	live_high_g = live_clip_pending = false;
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
			off_t first = REC_BLOCK_BYTES;

			memcpy(&head, block, sizeof(head));
			e->s = head.session;
			/* the first period from the first data block, after the table's parts: the
			 * head keeps none since version 4 */
			if (fs_read(&f, block, sizeof(block)) == (ssize_t)sizeof(block) &&
			    rec_block_ok(block) && block[0] == REC_TABLE) {
				first += (off_t)rec_table_blocks(sys_get_le16(block + 4)) *
					 REC_BLOCK_BYTES;
			}
			if (fs_seek(&f, first, FS_SEEK_SET) == 0 &&
			    fs_read(&f, block, sizeof(block)) == (ssize_t)sizeof(block) &&
			    rec_block_ok(block) && block[0] == REC_DATA) {
				e->p0 = sys_get_le32(block + 8);
			}
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

	live_init();
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
	if (recording) {
		return SMP_ERR_BAD_STATE;        /* not while it records */
	}
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

/* ---- the calibration from six faces (FIRMWARE.md, Calibration; COMMANDS.md, 64/22) ---------- */

#define CAL_FACE_S   5                      /* a face's samples */
#define CAL_LEFT_MS  (10 * 60 * 1000)       /* a calibration left so long is dropped */

static struct cal_run cal_run;              /* common/cal.h, under cal_lock */
static struct k_spinlock cal_lock;
static bool cal_on;
static int64_t cal_last_ms;
static int32_t cal_temp_sum;                /* the faces taken, their temperatures */
static struct strelka_calibrate cal_q0;     /* face 0's ranges and time */
#define CAL_MEASURING 3                     /* 64/22's `got` before the face's 5 s */
static uint8_t cal_face, cal_got = CAL_MEASURING;
static bool cal_done;                       /* the sixth taken, cal_made kept */
static struct strelka_cal cal_made;

static void cal_drop(void)
{
	k_spinlock_key_t key = k_spin_lock(&cal_lock);

	cal_on = false;
	k_spin_unlock(&cal_lock, key);
}

/* every sample at rest, from the sensor's thread: into the face measured or the turn */
static void cal_feed(const struct strelka_step *s, const int16_t *hg, uint64_t stamp)
{
	k_spinlock_key_t key = k_spin_lock(&cal_lock);

	if (cal_on) {
		const int16_t v[6] = { s->a[0], s->a[1], s->a[2], s->g[0], s->g[1], s->g[2] };

		cal_sample(&cal_run, v, hg, stamp);  /* both channels: the high-g's at once */
	}
	k_spin_unlock(&cal_lock, key);
}

/* a face asked: its samples from now; the answer read after 5 s (op_calibrated) */
static int op_calibrate(const struct strelka_calibrate *q, void *ctx)
{
	ARG_UNUSED(ctx);
	k_spinlock_key_t key;
	int rc;

	if (recording) {
		return SMP_ERR_BAD_STATE;
	}
	if (q->face == 0) {
		if (!imu_ranges_ok(q->acc_g, q->gyro_dps) ||
		    (q->hg && (imu_sensor() != IMU_LSM6DSV320X ||
			       !imu_ranges_ok(q->hg, q->gyro_dps)))) {
			return SMP_ERR_INVAL;
		}
		if (sensor_cfg.acc_g != q->acc_g || sensor_cfg.gyro_dps != q->gyro_dps ||
		    sensor_cfg.hg != q->hg) {
			sensor_cfg.acc_g = q->acc_g;
			sensor_cfg.gyro_dps = q->gyro_dps;
			sensor_cfg.hg = q->hg;   /* both: the high-g's samples beside, the same faces */
			imu_configure(&sensor_cfg);
			clock_set_sample_period(CLOCK_HZ / sensor_cfg.rate_hz);
			live_init();
		}
		cal_q0 = *q;
		cal_temp_sum = 0;
		cal_done = false;
		key = k_spin_lock(&cal_lock);
		cal_begin(&cal_run, imu_acc_full_scale(q->acc_g), imu_gyro_full_scale(q->gyro_dps));
		cal_on = true;
		k_spin_unlock(&cal_lock, key);
	} else if (!cal_on || k_uptime_get() - cal_last_ms > CAL_LEFT_MS) {
		cal_drop();
		return SMP_ERR_INVAL;
	}
	key = k_spin_lock(&cal_lock);
	rc = cal_ask(&cal_run, q->face);
	k_spin_unlock(&cal_lock, key);
	if (rc) {
		return SMP_ERR_INVAL;                /* out of turn, or one measured still */
	}
	cal_face = q->face;
	cal_got = CAL_MEASURING;
	cal_last_ms = k_uptime_get();
	return 0;
}

/* the face's answer: measuring still before its 5 s; after the sixth taken, the calibration
 * computed and kept */
static int op_calibrated(uint8_t *face, uint8_t *got, bool *done, struct strelka_cal *out,
			 void *ctx)
{
	ARG_UNUSED(ctx);
	k_spinlock_key_t key;
	struct cal_result r;
	int rc;

	*face = cal_face;
	if (cal_got == CAL_MEASURING) {
		if (k_uptime_get() - cal_last_ms < CAL_FACE_S * 1000) {
			*got = CAL_MEASURING;
			return 0;
		}
		key = k_spin_lock(&cal_lock);
		rc = cal_on ? cal_finish(&cal_run) : -1;
		k_spin_unlock(&cal_lock, key);
		if (rc < 0) {
			cal_drop();
			return SMP_ERR_BAD_STATE;    /* no sample, or a session begun meanwhile */
		}
		cal_got = (uint8_t)rc;
		cal_last_ms = k_uptime_get();
		if (cal_got == CAL_TAKEN) {
			cal_temp_sum += imu_temp_centi();
		}
		if (cal_got == CAL_TAKEN && cal_run.taken == CAL_FACES) {
			cal_drop();
			cal_compute(&cal_run, cal_rate_hz(&cal_run, CLOCK_HZ), &r);
			memset(&cal_made, 0, sizeof(cal_made));
			memcpy(cal_made.acc, r.acc, sizeof(cal_made.acc));
			memcpy(cal_made.gscale, r.gscale, sizeof(cal_made.gscale));
			memcpy(cal_made.gzero, r.gzero, sizeof(cal_made.gzero));
			cal_made.temp = (int16_t)(cal_temp_sum / (int32_t)CAL_FACES);
			cal_made.t = cal_q0.t;
			cal_made.range[0] = cal_q0.acc_g;
			cal_made.range[1] = cal_q0.gyro_dps;
			if (cal_q0.hg) {
				memcpy(cal_made.hg, r.hg, sizeof(cal_made.hg));
				cal_made.hg_range = cal_q0.hg;
			}
			if (op_cal_write(&cal_made, NULL)) {
				return SMP_ERR_UNKNOWN;
			}
			cal_done = true;
		}
	}
	*got = cal_got;
	*done = cal_done && cal_face == CAL_FACES - 1 && cal_got == CAL_TAKEN;
	*out = cal_made;
	return 0;
}

static const struct strelka_ops ops = {
	.info = op_info, .session = op_session, .sessions = op_sessions, .cal_write = op_cal_write,
	.cal_read = op_cal_read, .blink = op_blink, .card = op_card, .off = op_off,
	.forget = op_forget, .key = op_key, .join = op_join, .table = op_table,
	.calibrate = op_calibrate, .calibrated = op_calibrated,
};

/* ---- the magnets ---------------------------------------------------------------------------- */

/* the factory settings by the magnets: 5 s of orange, fast, that the hand sees the hold taken,
 * then forgotten and started again at once */
static void factory_now(struct k_work *w)
{
	ARG_UNUSED(w);
	store_forget();
	factory_restart = true;
	k_work_reschedule(&reboot_work, K_NO_WAIT);
}
static K_WORK_DELAYABLE_DEFINE(factory_work, factory_now);

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
	case MAGNET_FACTORY:
		if (recording) {
			light_flash(COLOUR_RED, 2000);  /* not while it records */
			break;
		}
		light_set(LIGHT_ORANGE_FAST);   /* the records kept, the unit started again */
		k_work_reschedule(&factory_work, K_SECONDS(5));
		break;
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
	live_init();
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
		sensor_clock(true);
		imu_start();
		/* the gyro's full scale known once the chip is */
		live_init();
	}
	if (card_init()) {
		fault = card_fault();
	} else if (card_mounted()) {
		take_set_file();                 /* given while the Belka was off */
	}
	/* the disk read alone on the cable while the Belka is in a set: its records safe from the
	 * host; a new Belka's open, for its STRELKA.SET; the NOR way's set file over SMP the same */
	usb_disk_protect(store.set.address != 0);
	card_set_file_allowed(store.set.address == 0);
	usb_init();                      /* up when the stack sees VBUS */
	strelka_mgmt_zephyr_init(&ops, NULL);   /* reached over ESB through Povodok: the link's */
	light_set(standing_light());

	/* the watchdog last, after the card's identification and the rest of the start */
	wdt_ch = wdt_install_timeout(wdt, &wdt_cfg);
	wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	power_watchdog_taken();          /* fed by the loop from here; until now by power.c */
	/* a new image runs as a test until it confirms itself: the start whole and the watchdog
	 * running, it is kept; one that hangs before is taken back by MCUboot at the next reset
	 * (FIRMWARE.md, The firmware's own updates) */
	if (!boot_is_img_confirmed()) {
		boot_write_img_confirmed();
	}

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
		/* the NOR way's STRELKA.SET over SMP on the serial port, where a disk would be open; given,
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
