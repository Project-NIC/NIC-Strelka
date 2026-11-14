/* The calibration from six faces (cal.h), tools/strelka_cal.py's numbers to the last */
#include "cal.h"

#include <math.h>
#include <string.h>

#define STILL_G   0.02
#define STILL_DPS 2.0
#define ONE_AXIS  0.9
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* the faces in their order: z up, z down, x up, x down, y up, y down */
static const uint8_t face_axis[CAL_FACES] = { 2, 2, 0, 0, 1, 1 };
static const int8_t face_sign[CAL_FACES] = { 1, -1, 1, -1, 1, -1 };

static double round_half_up(double x)
{
	return floor(x + 0.5);
}

void cal_begin(struct cal_run *c, uint16_t acc_fs, uint16_t gyro_fs)
{
	memset(c, 0, sizeof(*c));
	c->acc_fs = acc_fs;
	c->gyro_fs = gyro_fs;
}

int cal_ask(struct cal_run *c, uint8_t face)
{
	if (c->measuring || face != c->taken || face >= CAL_FACES) {
		return -1;
	}
	c->measuring = true;
	c->face = face;
	memset(&c->cur, 0, sizeof(c->cur));
	memset(c->hg_cur, 0, sizeof(c->hg_cur));
	return 0;
}

void cal_sample(struct cal_run *c, const int16_t s[6], const int16_t *hg, uint64_t stamp)
{
	if (c->samples++ == 0) {
		c->first_stamp = stamp;
	}
	c->last_stamp = stamp;
	if (c->measuring) {
		for (unsigned i = 0; i < 6; i++) {
			c->cur.sum[i] += s[i];
			c->cur.sq[i] += (int64_t)s[i] * s[i];
		}
		for (unsigned i = 0; hg != NULL && i < 3; i++) {
			c->hg_cur[i] += hg[i];       /* read at once with the rest, the same faces */
		}
		c->hg = c->hg || hg != NULL;
		c->cur.n++;
	} else if (c->taken >= 1 && c->taken < CAL_FACES) {
		for (unsigned i = 0; i < 3; i++) {
			c->run_sum[i] += s[3 + i];
		}
		c->run_n++;
	}
}

/* still: no axis spreading, by its standard deviation; the face asked: its axis carrying most of
 * the gravity, with its sign */
static enum cal_got judge(const struct cal_run *c)
{
	double per_g = 32768.0 / c->acc_fs, per_dps = 32768.0 / c->gyro_fs, m[3];

	for (unsigned i = 0; i < 6; i++) {
		double mean = (double)c->cur.sum[i] / c->cur.n;
		double var = (double)c->cur.sq[i] / c->cur.n - mean * mean;

		if (sqrt(var > 0 ? var : 0) > (i < 3 ? STILL_G * per_g : STILL_DPS * per_dps)) {
			return CAL_MOVED;
		}
		if (i < 3) {
			m[i] = mean / per_g;
		}
	}
	unsigned axis = face_axis[c->face];

	if (m[axis] * face_sign[c->face] < 0.8) {
		return CAL_OTHER_FACE;
	}
	for (unsigned i = 0; i < 3; i++) {
		if (i != axis && fabs(m[i]) > 0.3) {
			return CAL_OTHER_FACE;
		}
	}
	return CAL_TAKEN;
}

int cal_finish(struct cal_run *c)
{
	if (!c->measuring || c->cur.n == 0) {
		c->measuring = false;
		return -1;
	}
	c->measuring = false;
	enum cal_got got = judge(c);

	if (got == CAL_TAKEN) {
		memcpy(c->face_sum[c->taken], c->cur.sum, sizeof(c->cur.sum));
		memcpy(c->hg_face_sum[c->taken], c->hg_cur, sizeof(c->hg_cur));
		c->face_n[c->taken] = c->cur.n;
		if (c->taken >= 1) {
			memcpy(c->turn_sum[c->taken - 1], c->run_sum, sizeof(c->run_sum));
			c->turn_n[c->taken - 1] = c->run_n;
		}
		memset(c->run_sum, 0, sizeof(c->run_sum));
		c->run_n = 0;
		c->taken++;
	} else if (c->taken >= 1) {
		/* a face not taken lies between the faces: its gyro is the turn's */
		for (unsigned i = 0; i < 3; i++) {
			c->run_sum[i] += c->cur.sum[3 + i];
		}
		c->run_n += c->cur.n;
	}
	return got;
}

double cal_rate_hz(const struct cal_run *c, uint32_t clock_hz)
{
	if (c->samples < 2 || c->last_stamp <= c->first_stamp) {
		return 0;
	}
	return (double)(c->samples - 1) * clock_hz / (double)(c->last_stamp - c->first_stamp);
}

void cal_compute(const struct cal_run *c, double rate_hz, struct cal_result *out)
{
	double means[CAL_FACES][6], zero[3], per_g[3], gravity[CAL_FACES][3];
	double est_sum[3] = { 0 };
	unsigned est_n[3] = { 0 };

	for (unsigned f = 0; f < CAL_FACES; f++) {
		for (unsigned i = 0; i < 6; i++) {
			means[f][i] = (double)c->face_sum[f][i] / c->face_n[f];
		}
		out->acc[2 * face_axis[f] + (face_sign[f] > 0 ? 0 : 1)] =
			(int16_t)round_half_up(means[f][face_axis[f]]);
		/* the high-g channel's the same way: its axis's mean up and down */
		out->hg[2 * face_axis[f] + (face_sign[f] > 0 ? 0 : 1)] = c->hg ?
			(int16_t)round_half_up((double)c->hg_face_sum[f][face_axis[f]] / c->face_n[f]) : 0;
	}
	for (unsigned i = 0; i < 3; i++) {
		double s = 0;

		for (unsigned f = 0; f < CAL_FACES; f++) {
			s += means[f][3 + i];
		}
		out->gzero[i] = (int16_t)round_half_up(s / CAL_FACES);
		zero[i] = (out->acc[2 * i] + out->acc[2 * i + 1]) / 2.0;
		per_g[i] = (out->acc[2 * i] - out->acc[2 * i + 1]) / 2.0;
		if (per_g[i] == 0) {
			per_g[i] = 1.0;
		}
	}
	for (unsigned f = 0; f < CAL_FACES; f++) {
		for (unsigned i = 0; i < 3; i++) {
			gravity[f][i] = (means[f][i] - zero[i]) / per_g[i];
		}
	}
	double step_dps = c->gyro_fs / 32768.0;

	for (unsigned k = 0; k + 1 < CAL_FACES; k++) {
		double theta[3], norm, ab = 0, aa = 0, bb = 0;
		const double *a = gravity[k], *b = gravity[k + 1];

		for (unsigned i = 0; i < 3; i++) {
			theta[i] = (double)(c->turn_sum[k][i] - (int64_t)c->turn_n[k] * out->gzero[i]) /
				   rate_hz * step_dps;
		}
		norm = sqrt(theta[0] * theta[0] + theta[1] * theta[1] + theta[2] * theta[2]);
		for (unsigned i = 0; i < 3; i++) {
			ab += a[i] * b[i];
			aa += a[i] * a[i];
			bb += b[i] * b[i];
		}
		double cosv = ab / sqrt(aa * bb);
		double angle = acos(cosv < -1.0 ? -1.0 : cosv > 1.0 ? 1.0 : cosv) * (180.0 / M_PI);

		if (norm == 0 || angle < 45) {
			continue;
		}
		for (unsigned i = 0; i < 3; i++) {
			if (fabs(theta[i]) >= ONE_AXIS * norm) {
				est_sum[i] += fabs(theta[i]) / angle;
				est_n[i]++;
			}
		}
	}
	for (unsigned i = 0; i < 3; i++) {
		out->gscale[i] = est_n[i] ? (uint16_t)round_half_up(est_sum[i] / est_n[i] * 32768) :
					    32768;
	}
}

void cal_gain_init(struct cal_gain *g, const int16_t acc[6], const uint16_t gscale[3],
		   const int16_t gzero[3], const uint16_t range[2], uint16_t acc_range,
		   uint16_t gyro_range, uint32_t acc_step_ng)
{
	for (unsigned i = 0; i < 6; i++) {
		g->zero[i] = 0.0f;
		g->gain[i] = 1.0f;
	}
	if (acc == NULL) {
		return;
	}
	/* made at other ranges: the readings by the nominal steps (RECORD.md) */
	double ka = range != NULL && range[0] && acc_range ? (double)range[0] / acc_range : 1.0;
	double kg = range != NULL && range[1] && gyro_range ? (double)range[1] / gyro_range : 1.0;
	double step_g = acc_step_ng * 1e-9;

	for (unsigned i = 0; i < 3; i++) {
		double up = acc[2 * i] * ka, down = acc[2 * i + 1] * ka;
		double per_g = (up - down) / 2;
		double scale = gscale[i] / 32768.0;

		if (per_g == 0) {
			per_g = 1.0;
		}
		if (scale == 0) {
			scale = 1.0;
		}
		g->zero[i] = (float)((up + down) / 2);
		g->gain[i] = step_g > 0 ? (float)(1.0 / (per_g * step_g)) : 1.0f;
		g->zero[3 + i] = (float)(gzero[i] * kg);
		g->gain[3 + i] = (float)(1.0 / scale);
	}
}

void cal_gain_step(const struct cal_gain *g, const struct strelka_step *in,
		   struct strelka_step *out)
{
	const int16_t v[6] = { in->a[0], in->a[1], in->a[2], in->g[0], in->g[1], in->g[2] };
	int16_t r[6];
	bool lost = true;

	for (unsigned i = 0; i < 6; i++) {
		lost = lost && v[i] == REC_STEP_MISSING;
	}
	if (lost) {
		*out = *in;
		return;
	}
	for (unsigned i = 0; i < 6; i++) {
		float x = ((float)v[i] - g->zero[i]) * g->gain[i];
		float y = floorf(x + 0.5f);

		r[i] = (int16_t)(y > 32767.0f ? 32767 : y < -32767.0f ? -32767 : (int)y);
	}
	for (unsigned i = 0; i < 3; i++) {
		out->a[i] = r[i];
		out->g[i] = r[3 + i];
	}
}
