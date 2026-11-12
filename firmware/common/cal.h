/* The calibration from six faces, computed in the Belka (FIRMWARE.md, Calibration; COMMANDS.md,
 * 64/22): each face's samples summed while the unit lies still, the gyro summed between the faces,
 * the turns; after the sixth the head's raw readings. tools/strelka_cal.py is the reference, the
 * same numbers to the last (tools/test_firmware.py). No Zephyr here: the caller feeds the samples
 * and holds the lock. */
#ifndef STRELKA_CAL_H
#define STRELKA_CAL_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define CAL_FACES 6u

enum cal_got {              /* 64/22's `got` */
	CAL_TAKEN = 0,
	CAL_MOVED = 1,
	CAL_OTHER_FACE = 2,
};

struct cal_sums {           /* the six axes' sums, their squares' and how many */
	int64_t sum[6];
	int64_t sq[6];
	uint32_t n;
};

struct cal_run {
	uint16_t acc_fs, gyro_fs;    /* what 32 768 steps make, g and °/s */
	uint8_t taken;               /* faces taken, 0–6 */
	bool measuring;
	uint8_t face;                /* the face being measured */
	struct cal_sums cur;
	int64_t hg_cur[3];           /* the high-g channel's on the face, where it is kept */
	int64_t face_sum[CAL_FACES][6];
	int64_t hg_face_sum[CAL_FACES][3];
	bool hg;                     /* a sample with the high-g channel came */
	uint32_t face_n[CAL_FACES];
	int64_t turn_sum[CAL_FACES - 1][3];  /* the gyro between face k and k + 1 */
	uint32_t turn_n[CAL_FACES - 1];
	int64_t run_sum[3];          /* since the last face taken */
	uint32_t run_n;
	uint64_t first_stamp, last_stamp;    /* every sample's, for the rate */
	uint32_t samples;
};

struct cal_result {         /* the head's fields (RECORD.md), 64/3's */
	int16_t acc[6];
	uint16_t gscale[3];
	int16_t gzero[3];
	int16_t hg[6];              /* the high-g channel's six readings; zeros none kept */
};

/* a calibration started again, the sensor's full scales its own */
void cal_begin(struct cal_run *c, uint16_t acc_fs, uint16_t gyro_fs);

/* a face asked: 0 or the next; -1 out of turn, nothing changed */
int cal_ask(struct cal_run *c, uint8_t face);

/* a sample, raw, x y z then the gyro's; stamp in the caller's clock */
void cal_sample(struct cal_run *c, const int16_t s[6], const int16_t *hg, uint64_t stamp);

/* the face asked measured: taken, or why not; a face not taken counts in the turn. -1 with no
 * sample */
int cal_finish(struct cal_run *c);

/* the samples' rate by their stamps, clock_hz the stamps' ticks a second */
double cal_rate_hz(const struct cal_run *c, uint32_t clock_hz);

/* after the sixth face: the raw readings, the samples at rate_hz */
void cal_compute(const struct cal_run *c, double rate_hz, struct cal_result *out);

/* The calibration as a zero and a gain an axis, the accelerometer's x, y, z then the gyro's: a
 * step put into the sensor's own steps as an ideal sensor would give it — the live view's, sent
 * raw or turned into a quaternion (PRINCIPLE.md, The LIVE). The accelerometer by its six readings,
 * the gyro by its zero and its scale; a calibration made at other ranges than the session's taken
 * by the nominal steps. tools/strelka_record.py's live_gain and live_step, float for float. */
struct cal_gain {
	float zero[6];
	float gain[6];
};

/* acc NULL: no calibration, a step as it is; acc_range and gyro_range the session's as set,
 * acc_step_ng the accelerometer's own step, range the ranges the calibration was made at */
void cal_gain_init(struct cal_gain *g, const int16_t acc[6], const uint16_t gscale[3],
		   const int16_t gzero[3], const uint16_t range[2], uint16_t acc_range,
		   uint16_t gyro_range, uint32_t acc_step_ng);

/* a step calibrated, each value rounded half up and held within ±32 767; a step lost, every value
 * REC_STEP_MISSING, left as it is */
void cal_gain_step(const struct cal_gain *g, const struct strelka_step *in,
		   struct strelka_step *out);

#endif
