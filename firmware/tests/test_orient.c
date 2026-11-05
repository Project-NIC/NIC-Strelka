/* The Belka's turn as a quaternion against synthetic motion (FIRMWARE.md, The principle): a
 * sensor lying still and tilted takes its tilt from gravity at once; a steady turn about an axis
 * integrates to the angle turned; a gyro with a bias, held still, is pulled back to gravity's tilt
 * by the complementary filter and stays there; a rocking motion is followed within a few degrees;
 * the heading, with no magnetometer, is left where the gyro says. The sensor is modelled as the
 * ICM-42686-P at ±2 000 °/s and ±16 g, 100 steps a second. */
#include <math.h>
#include <string.h>
#include "orient.h"
#include "quat.h"
#include "test.h"

#define RATE 100.0f
#define DT (1.0f / RATE)
#define G_RAW 2048.0f                   /* 1 g at ±16 g */
#define DPS_RAW (32768.0f / 2000.0f)    /* raw a degree a second at ±2 000 °/s */
#define DEG 0.017453292f

/* the world's up as the sensor sees it for a turn q, and the step a sensor at rest would read */
static void sensor_at(const float q[4], const float w_dps[3], struct strelka_step *s)
{
	float w = q[0], x = q[1], y = q[2], z = q[3];
	float up[3] = { 2 * (x * z - w * y), 2 * (y * z + w * x), w * w - x * x - y * y + z * z };

	for (int i = 0; i < 3; i++) {
		s->a[i] = (int16_t)lroundf(up[i] * G_RAW);
		s->g[i] = (int16_t)lroundf(w_dps[i] * DPS_RAW);
	}
}

static float angle_between(const float a[4], const float b[4])
{
	float d = fabsf(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);

	return 2.0f * acosf(d > 1.0f ? 1.0f : d) / DEG;
}

static float tilt_error(const struct orient *o, const float truth[4])
{
	float up[3], want[3], w = truth[0], x = truth[1], y = truth[2], z = truth[3], d;

	orient_up(o, up);
	want[0] = 2 * (x * z - w * y);
	want[1] = 2 * (y * z + w * x);
	want[2] = w * w - x * x - y * y + z * z;
	d = up[0] * want[0] + up[1] * want[1] + up[2] * want[2];
	return acosf(d > 1.0f ? 1.0f : d < -1.0f ? -1.0f : d) / DEG;
}

/* q ⊗ a turn of angle about the sensor's axis */
static void turn(float q[4], const float axis[3], float angle)
{
	float h = angle / 2, s = sinf(h), r[4] = { cosf(h), axis[0] * s, axis[1] * s, axis[2] * s };
	float out[4] = {
		q[0] * r[0] - q[1] * r[1] - q[2] * r[2] - q[3] * r[3],
		q[0] * r[1] + q[1] * r[0] + q[2] * r[3] - q[3] * r[2],
		q[0] * r[2] - q[1] * r[3] + q[2] * r[0] + q[3] * r[1],
		q[0] * r[3] + q[1] * r[2] - q[2] * r[1] + q[3] * r[0],
	};

	memcpy(q, out, sizeof(out));
}

int main(void)
{
	struct orient o;
	struct strelka_step s;
	float q[4], truth[4], zero[3] = { 0, 0, 0 };
	const float xaxis[3] = { 1, 0, 0 }, yaxis[3] = { 0, 1, 0 }, zaxis[3] = { 0, 0, 1 };

	/* still and tilted 30° about x: the tilt from gravity at the first step */
	orient_init(&o, 2000, NULL, NULL, 0, 0);
	truth[0] = 1; truth[1] = truth[2] = truth[3] = 0;
	turn(truth, xaxis, 30 * DEG);
	sensor_at(truth, zero, &s);
	orient_step(&o, &s, DT);
	CHECK(tilt_error(&o, truth) < 0.5f);
	for (int i = 0; i < 100; i++) {
		orient_step(&o, &s, DT);
	}
	CHECK(tilt_error(&o, truth) < 0.5f);
	OK("the tilt from gravity, at once and held");

	/* a steady turn of 90°/s about y for a second: 90° integrated, the heading free */
	orient_init(&o, 2000, NULL, NULL, 0, 0);
	truth[0] = 1; truth[1] = truth[2] = truth[3] = 0;
	sensor_at(truth, zero, &s);
	orient_step(&o, &s, DT);
	for (int i = 0; i < 100; i++) {
		float w[3] = { 0, 90, 0 };

		turn(truth, yaxis, 90 * DEG * DT);
		sensor_at(truth, w, &s);
		orient_step(&o, &s, DT);
	}
	orient_quat(&o, q);
	CHECK(angle_between(q, truth) < 2.0f && tilt_error(&o, truth) < 2.0f);
	OK("a turn of 90° integrated within 2°");

	/* a gyro bias of 2 °/s on x, the sensor still: the tilt drifts no further than the pull
	 * lets it, under 4° at ten seconds, and the bias learned takes it away — under 0,5° at
	 * thirty seconds, the bias known within 0,1 °/s */
	orient_init(&o, 2000, NULL, NULL, 0, 0);
	truth[0] = 1; truth[1] = truth[2] = truth[3] = 0;
	for (int i = 0; i < 1000; i++) {
		float w[3] = { 2, 0, 0 };

		sensor_at(truth, w, &s);
		orient_step(&o, &s, DT);
	}
	CHECK(tilt_error(&o, truth) < 4.0f);
	for (int i = 0; i < 2000; i++) {
		float w[3] = { 2, 0, 0 };

		sensor_at(truth, w, &s);
		orient_step(&o, &s, DT);
	}
	CHECK(tilt_error(&o, truth) < 0.5f && fabsf(o.bias[0] / DEG - 2.0f) < 0.1f);
	/* the calibration's zero takes the bias off: no error left */
	{
		int16_t gzero[3] = { (int16_t)lroundf(2 * DPS_RAW), 0, 0 };
		uint16_t gscale[3] = { 32768, 32768, 32768 };

		orient_init(&o, 2000, gscale, gzero, 0, 0);
		for (int i = 0; i < 1000; i++) {
			float w[3] = { 2, 0, 0 };

			sensor_at(truth, w, &s);
			orient_step(&o, &s, DT);
		}
		CHECK(tilt_error(&o, truth) < 0.3f);
	}
	OK("a gyro's bias held by gravity and learned, or taken off by the calibration");

	/* a rocking of ±20° at 1 Hz about x, a trot's cadence, with a bias of 1 °/s: followed within
	 * 3° throughout the second ten seconds — the accelerometer sees gravity alone here, the
	 * body's own acceleration left out */
	orient_init(&o, 2000, NULL, NULL, 0, 0);
	{
		float worst = 0, angle = 0, last = 0;

		for (int i = 0; i < 2000; i++) {
			float t = i * DT;
			float w[3];

			angle = 20 * DEG * sinf(2 * 3.14159265f * t);
			w[0] = (angle - last) / DT / DEG + 1.0f;
			last = angle;
			truth[0] = cosf(angle / 2); truth[1] = sinf(angle / 2);
			truth[2] = truth[3] = 0;
			sensor_at(truth, w, &s);
			orient_step(&o, &s, DT);
			if (i >= 1000) {
				float err = tilt_error(&o, truth);

				worst = err > worst ? err : worst;
			}
		}
		CHECK(worst < 3.0f);
	}
	OK("a rocking at 1 Hz followed within 3°");

	/* the packing's step is below the filter's error: a turn packed and unpacked within 0,3° */
	orient_quat(&o, q);
	{
		float back[4];

		quat_unpack(quat_pack(q), back);
		CHECK(angle_between(q, back) < 0.3f);
	}
	/* a turn about z, the heading: integrated as well, nothing pulling it */
	orient_init(&o, 2000, NULL, NULL, 0, 0);
	truth[0] = 1; truth[1] = truth[2] = truth[3] = 0;
	sensor_at(truth, zero, &s);
	orient_step(&o, &s, DT);
	for (int i = 0; i < 200; i++) {
		float w[3] = { 0, 0, 45 };

		turn(truth, zaxis, 45 * DEG * DT);
		sensor_at(truth, w, &s);
		orient_step(&o, &s, DT);
	}
	orient_quat(&o, q);
	CHECK(angle_between(q, truth) < 2.0f);
	OK("the heading from the gyro alone");

	printf("ORIENT OK\n");
	return 0;
}
