/* NIC-Strelka — the hoof's sensor off the set's clock put onto the grid (common/resample.c), on the
 * host: the LSM6DSV320X at its 7 680 Hz running 1 % fast and 1 % slow, the grid at 4 kHz; a ramp
 * drawn exactly, a sine within what a straight line between samples keeps, every point of the
 * grid given once and in order; three samples lost and the points between left missing; samples
 * from before period 0; a sample out of range marking the points drawn from it. */
#include <math.h>
#include <string.h>
#include "test.h"
#include "resample.h"

#define TICKS_HZ  16000000.0
#define GRID      4000u                     /* ticks: 4 kHz */
#define MAX_GAP   5000u                     /* some 2,4 of the sensor's periods */
#define PI        3.14159265358979323846

struct got {
	int64_t first, last, count;
	int64_t holes;                           /* points skipped */
	int max_ramp_err, max_sine_err;
	int out_marked;
	int64_t skip_from, skip_to;              /* the points expected missing */
};

static double sine_at(double t)
{
	return 10000.0 * sin(2 * PI * 37.0 * t);
}

static double ramp_at(double t)
{
	return -20000.0 + 30000.0 * t;          /* within an i16 over a second */
}

static void take(int64_t index, const struct strelka_step *s, bool acc_out, bool gyro_out,
		 void *ctx)
{
	struct got *g = ctx;
	double t = (double)index * GRID / TICKS_HZ;
	int e_ramp = abs(s->g[0] - (int)lround(ramp_at(t)));
	int e_sine = abs(s->a[0] - (int)lround(sine_at(t)));

	(void)gyro_out;
	if (g->count == 0) {
		g->first = index;
	} else {
		CHECK(index > g->last);             /* in order, never twice */
		g->holes += index - g->last - 1;
	}
	CHECK(index >= 0);
	CHECK(!(index >= g->skip_from && index <= g->skip_to));
	g->last = index;
	g->count++;
	g->max_ramp_err = e_ramp > g->max_ramp_err ? e_ramp : g->max_ramp_err;
	g->max_sine_err = e_sine > g->max_sine_err ? e_sine : g->max_sine_err;
	g->out_marked += acc_out;
}

/* a second of the sensor at 7 680 Hz times (1 + error), from `start` ticks after period 0; the
 * samples numbered `lose_from`… for `lose_n` lost, sample `clip` out of range */
static void run(struct got *g, double error, double start, int lose_from, int lose_n, int clip)
{
	struct resample r;
	double dt = TICKS_HZ / (7680.0 * (1.0 + error));

	memset(g, 0, sizeof(*g));
	g->skip_from = g->skip_to = -1;
	resample_begin(&r, GRID, MAX_GAP, take, g);
	for (int k = 0; k < 7680; k++) {
		double at = start + k * dt;
		double t = at / TICKS_HZ;
		struct strelka_step s = { 0 };

		if (k >= lose_from && k < lose_from + lose_n) {
			continue;
		}
		s.a[0] = (int16_t)lround(sine_at(t));
		s.g[0] = (int16_t)lround(ramp_at(t));
		resample_sample(&r, (int64_t)llround(at), &s, k == clip, false);
	}
}

static void test_steady(void)
{
	static const double errors[] = { 0.0, 0.01, -0.01 };

	for (unsigned i = 0; i < sizeof(errors) / sizeof(errors[0]); i++) {
		struct got g;

		run(&g, errors[i], 1234.0, -1, 0, -1);
		CHECK(g.holes == 0);
		/* the points within the sensor's second, its 7 680 samples taking 1 / (1 + error) s */
		CHECK(fabs((double)g.count - 4000.0 / (1.0 + errors[i])) <= 2.0);
		CHECK(g.max_ramp_err <= 1);                   /* a line drawn exactly */
		CHECK(g.max_sine_err <= 3);                   /* A ω² h² / 8 ≈ 1,2, and the rounding */
	}
	OK("the sensor 1 % fast or slow onto a 4 kHz grid: every point once, in order, the values held");
}

static void test_gap(void)
{
	struct got g;
	double dt = TICKS_HZ / 7680.0;
	double start = 1234.0;
	int64_t last_before = (int64_t)llround(start + 999 * dt);
	int64_t first_after = (int64_t)llround(start + 1003 * dt);

	/* the points between the last sample before the gap and the first after are not drawn */
	run(&g, 0.0, start, 1000, 3, -1);
	CHECK(g.holes > 0);
	CHECK(g.holes <= (first_after - last_before) / GRID + 1);
	CHECK(g.max_ramp_err <= 1 && g.max_sine_err <= 3);
	OK("three samples lost: the grid's points between left missing, none drawn across");
}

static void test_before_zero_and_clip(void)
{
	struct got g;

	run(&g, 0.0, -50000.0, -1, 0, 4000);
	CHECK(g.first == 0);                         /* nothing before period 0 */
	CHECK(g.holes == 0);
	CHECK(g.out_marked >= 1 && g.out_marked <= 2);   /* the points drawn from the clipped one */
	OK("samples from before period 0 left; a sample out of range marking the points beside it");
}

int main(void)
{
	test_steady();
	test_gap();
	test_before_zero_and_clip();
	return 0;
}
