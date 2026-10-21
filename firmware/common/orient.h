/* NIC-Strelka — the Belka's one computation: its turn as a quaternion for the live view
 * (FIRMWARE.md, The principle; PRINCIPLE.md, The LIVE). The gyro integrated step by step, its
 * tilt pulled toward gravity by the accelerometer with a small gain and the gyro's bias learned
 * from what the pull keeps correcting — Mahony's explicit complementary filter (Mahony, Hamel and
 * Pflimlin, "Nonlinear complementary filters on the special orthogonal group", IEEE Transactions
 * on Automatic Control 53(5), 2008); no magnetometer, so the heading is where the gyro says and
 * wanders freely. The calibration's gyro zero and scale taken off when given. Plain C11 with
 * floats, run on the host against synthetic motion (tests/test_orient.c). */
#ifndef STRELKA_ORIENT_H
#define STRELKA_ORIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define ORIENT_KP_DEFAULT 0.5f  /* 1/s: the tilt's error halved in some 1,4 s */
#define ORIENT_KI_DEFAULT 0.05f /* 1/s²: the gyro's bias learned over some 20 s */

struct orient {
	float q[4];             /* w, x, y, z: the sensor's frame turned into the world's */
	float kp, ki;           /* the tilt's pull toward gravity, and the bias's learning */
	float bias[3];          /* the gyro's bias learned, rad/s */
	float gyro_scale;       /* raw to rad/s */
	float gyro_zero[3];     /* raw, taken off */
	bool settled;           /* the first steps align to gravity at once */
};

/* gyro_dps the range, ±2 000 or ±4 000; the calibration's scale (32 768 for 1,000) and zero, or
 * NULL for none; kp and ki 0 for the defaults */
void orient_init(struct orient *o, uint16_t gyro_dps, const uint16_t gyro_scale[3],
		 const int16_t gyro_zero[3], float kp, float ki);
/* a step of dt seconds: the gyro's turn integrated, the tilt corrected toward the accelerometer */
void orient_step(struct orient *o, const struct strelka_step *step, float dt);
/* the turn now, as the live view packs it (quat.h) */
void orient_quat(const struct orient *o, float q[4]);
/* the world's vertical as the sensor sees it, for the tests and the calibration: unit vector */
void orient_up(const struct orient *o, float up[3]);

#endif
