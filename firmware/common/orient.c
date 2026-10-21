#include <math.h>
#include <string.h>
#include "orient.h"

#define DEG_TO_RAD 0.017453292f

void orient_init(struct orient *o, uint16_t gyro_dps, const uint16_t gyro_scale[3],
		 const int16_t gyro_zero[3], float kp, float ki)
{
	memset(o, 0, sizeof(*o));
	o->q[0] = 1.0f;
	o->kp = kp > 0 ? kp : ORIENT_KP_DEFAULT;
	o->ki = ki > 0 ? ki : ORIENT_KI_DEFAULT;
	/* the sensor's 16 bits over its range; the calibration's scale, one mean of three, on top */
	o->gyro_scale = (float)gyro_dps / 32768.0f * DEG_TO_RAD;
	if (gyro_scale) {
		float mean = ((float)gyro_scale[0] + gyro_scale[1] + gyro_scale[2]) / 3.0f;

		o->gyro_scale *= mean / 32768.0f;
	}
	if (gyro_zero) {
		for (int i = 0; i < 3; i++) {
			o->gyro_zero[i] = gyro_zero[i];
		}
	}
}

/* the body's z axis in the world, the vertical, as the sensor sees it: R^T · (0, 0, 1) */
static void up_from(const float q[4], float up[3])
{
	float w = q[0], x = q[1], y = q[2], z = q[3];

	up[0] = 2.0f * (x * z - w * y);
	up[1] = 2.0f * (y * z + w * x);
	up[2] = w * w - x * x - y * y + z * z;
}

static void normalize(float q[4])
{
	float n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);

	if (n > 0) {
		for (int i = 0; i < 4; i++) {
			q[i] /= n;
		}
	}
}

void orient_step(struct orient *o, const struct strelka_step *step, float dt)
{
	float w[3], a[3], an, up[3], e[3] = { 0, 0, 0 };
	float *q = o->q, dq[4];

	for (int i = 0; i < 3; i++) {
		w[i] = ((float)step->g[i] - o->gyro_zero[i]) * o->gyro_scale;
		a[i] = step->a[i];
	}
	an = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	if (an > 0) {
		for (int i = 0; i < 3; i++) {
			a[i] /= an;
		}
		if (!o->settled) {
			/* the first step: the tilt from gravity alone, the heading zero — the
			 * quaternion turning the measured up onto (0, 0, 1) */
			float c = a[2], s = sqrtf(a[0] * a[0] + a[1] * a[1]);

			if (s > 1e-6f) {
				float half = atan2f(s, c) / 2.0f;
				float ax = a[1] / s, ay = -a[0] / s;

				q[0] = cosf(half);
				q[1] = ax * sinf(half);
				q[2] = ay * sinf(half);
				q[3] = 0;
			} else {
				q[0] = c >= 0 ? 1.0f : 0.0f;
				q[1] = c >= 0 ? 0.0f : 1.0f;
				q[2] = 0;
				q[3] = 0;
			}
			o->settled = true;
			return;
		}
		/* the error between the measured up and the estimated one, in the sensor's frame */
		up_from(q, up);
		e[0] = a[1] * up[2] - a[2] * up[1];
		e[1] = a[2] * up[0] - a[0] * up[2];
		e[2] = a[0] * up[1] - a[1] * up[0];
	}
	for (int i = 0; i < 3; i++) {
		o->bias[i] -= o->ki * e[i] * dt;  /* what the pull keeps correcting is the bias */
		w[i] += o->kp * e[i] - o->bias[i];
	}
	/* q += 0,5 · q ⊗ (0, w) · dt */
	dq[0] = 0.5f * (-q[1] * w[0] - q[2] * w[1] - q[3] * w[2]);
	dq[1] = 0.5f * (q[0] * w[0] + q[2] * w[2] - q[3] * w[1]);
	dq[2] = 0.5f * (q[0] * w[1] - q[1] * w[2] + q[3] * w[0]);
	dq[3] = 0.5f * (q[0] * w[2] + q[1] * w[1] - q[2] * w[0]);
	for (int i = 0; i < 4; i++) {
		q[i] += dq[i] * dt;
	}
	normalize(q);
}

void orient_quat(const struct orient *o, float q[4])
{
	memcpy(q, o->q, sizeof(o->q));
}

void orient_up(const struct orient *o, float up[3])
{
	up_from(o->q, up);
}
