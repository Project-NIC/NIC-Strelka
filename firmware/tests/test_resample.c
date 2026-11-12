/* NIC-Strelka — a sensor on its own clock filtered onto the set's grid (common/resample.c), on the
 * host: 100 Hz into memory, N 10, and 2 kHz from an LSM at 7 680 Hz, 2 and 32 kHz from an ICM at
 * 32 kHz, the sensor 1 % fast and 1 % slow; a constant drawn exactly, a sine at a tenth of the rate
 * within 0,2 %, at 0,42 within 1 %, one at 0,6 held under −40 dB; each step's N points given once, in order, the same;
 * a lost sample hardly felt; a gap's steps left missing; the end drawn by the flush; a sample out
 * of range marking the step beside it. */
#include <math.h>
#include <string.h>
#include "test.h"
#include "resample.h"

#define TICKS_HZ  16000000.0
#define PI        3.14159265358979323846

struct got {
	int n;
	int64_t last, count, steps;
	int64_t step_first;                     /* the current step's first point */
	int16_t step_val;
	double f, amp;                          /* the signal: amp sin(2π f t), or amp for f 0 */
	double grid_s;                          /* the grid's period, s */
	double err_max;                         /* the worst |got − want| at the step's centre */
	double t_lo, t_hi;                      /* the error looked at between, the filter whole */
	int64_t miss_from, miss_to;             /* steps whose points must not come */
	int marked;
	bool same;                              /* each step's N points the same */
	bool zero;                              /* above the filter's band: 0 wanted */
};

static double signal(const struct got *g, double t)
{
	return g->f == 0 ? g->amp : g->amp * sin(2 * PI * g->f * t);
}

static void take(int64_t index, const struct strelka_step *s, const int16_t *hg, bool acc_out,
		 bool gyro_out, void *ctx)
{
	struct got *g = ctx;
	int64_t k = index / g->n;

	(void)hg;
	(void)gyro_out;
	CHECK(index > g->last);                 /* each once and in order */
	g->last = index;
	g->count++;
	CHECK(!(k >= g->miss_from && k <= g->miss_to));
	if (index % g->n == 0) {
		double c = ((double)index + (g->n - 1) / 2.0) * g->grid_s;
		double e = fabs(s->a[0] - (g->zero ? 0 : signal(g, c)));

		g->steps++;
		g->step_first = index;
		g->step_val = s->a[0];
		if (e > g->err_max && !acc_out && c > g->t_lo && c < g->t_hi) {
			g->err_max = e;
		}
	} else if (s->a[0] != g->step_val || index - g->step_first >= g->n) {
		g->same = false;
	}
	if (acc_out) {
		g->marked++;
	}
}

/* one run: `seconds` of the sensor at `odr` Hz, `drift` off, onto a grid of `grid` Hz, N `n`;
 * the samples numbered `lose_from`..`lose_to` lost, sample `out_at` out of range */
static struct got run(double odr, double drift, unsigned grid, int n, double f, double amp,
		      double seconds, int lose_from, int lose_to, int out_at)
{
	static struct resample r;
	struct got g;
	double dt = TICKS_HZ / (odr * (1.0 + drift));
	uint32_t period = (uint32_t)(TICKS_HZ / grid);

	memset(&g, 0, sizeof(g));
	g.n = n;
	g.last = -1;
	g.f = f;
	g.amp = amp;
	g.grid_s = period / TICKS_HZ;
	g.miss_from = g.miss_to = -1;
	g.same = true;
	g.zero = f > 0.57 * grid / n;
	g.t_lo = 0.15;
	g.t_hi = seconds - 0.15;
	resample_begin(&r, period, (uint8_t)n, 0, (uint32_t)(TICKS_HZ / odr), (uint32_t)(dt * 2.5), take,
		       &g);
	for (int i = 0; i < (int)(odr * seconds); i++) {
		double t = (i * dt + 0.37 * dt) / TICKS_HZ;
		struct strelka_step s;

		if (i >= lose_from && i <= lose_to) {
			continue;
		}
		memset(&s, 0, sizeof(s));
		s.a[0] = (int16_t)lround(signal(&g, t));
		resample_sample(&r, (int64_t)llround(t * TICKS_HZ), &s, NULL, i == out_at, false);
	}
	resample_flush(&r);
	return g;
}

int main(void)
{
	struct got g;

	/* a constant, exactly: 100 Hz from 7 680 Hz in bins, 2 kHz from 7 680 Hz sample by sample,
	 * 2 kHz from 32 kHz in bins of two, 32 kHz from 32 kHz; the sensor 1 % fast and slow */
	for (int d = -1; d <= 1; d += 2) {
		g = run(7680, 0.01 * d, 1000, 10, 0, 12345, 3, -1, -1, -1);
		CHECK(g.err_max == 0 && g.same && g.steps >= 295 && g.count == g.steps * 10);
		g = run(7680, 0.01 * d, 2000, 1, 0, -12345, 1, -1, -1, -1);
		CHECK(g.err_max == 0 && g.steps >= 1975);     /* 1 s of the sensor's, ±1 % */
		g = run(32000, 0.01 * d, 2000, 1, 0, 777, 1, -1, -1, -1);
		CHECK(g.err_max == 0 && g.steps >= 1975);
		g = run(32000, 0.01 * d, 32000, 1, 0, -777, 0.2, -1, -1, -1);
		CHECK(g.err_max == 0 && g.steps >= 6300);
	}
	OK("a constant drawn exactly from 7 680 and 32 000 Hz, binned or not, the sensor 1 % off");

	/* a sine at a tenth of the rate into memory: within 0,2 % of its amplitude */
	g = run(7680, 0.01, 1000, 10, 10, 10000, 3, -1, -1, -1);
	CHECK(g.err_max < 20 && g.same);
	g = run(7680, -0.01, 2000, 1, 200, 10000, 1, -1, -1, -1);
	CHECK(g.err_max < 20);
	g = run(32000, 0.01, 2000, 1, 200, 10000, 1, -1, -1, -1);
	CHECK(g.err_max < 20);
	OK("a sine at a tenth of the rate within 0,2 %, at 100 Hz and 2 kHz");

	/* at 0,42 of the rate, near the band's edge: within 1 % */
	g = run(7680, 0.01, 1000, 10, 42, 10000, 3, -1, -1, -1);
	CHECK(g.err_max < 100);
	g = run(32000, 0.01, 2000, 1, 840, 10000, 1, -1, -1, -1);
	CHECK(g.err_max < 100);
	OK("at 0,42 of the rate within 1 %");

	/* above the rate's half, 0,6 of it: held under -40 dB, its amplitude 10 000 */
	g = run(7680, 0.01, 1000, 10, 60, 10000, 3, -1, -1, -1);
	CHECK(g.err_max < 100);
	g = run(7680, 0.01, 2000, 1, 1200, 10000, 1, -1, -1, -1);
	CHECK(g.err_max < 100);
	g = run(32000, 0.01, 2000, 1, 1200, 10000, 1, -1, -1, -1);
	CHECK(g.err_max < 100);
	OK("a sine at 0,6 of the rate held under -40 dB, no alias drawn");

	/* a sample lost: hardly felt; five lost from 7 680 at 2 kHz, or 100 from 7 680 at 100 Hz:
	 * the steps whose centre falls in the gap missing */
	g = run(7680, 0.01, 2000, 1, 200, 10000, 1, 3000, 3000, -1);
	CHECK(g.err_max < 40 && g.steps >= 1975);
	g = run(7680, 0.0, 2000, 1, 0, 1000, 1, 3000, 3004, -1);
	CHECK(g.err_max == 0 && g.steps < 1999 && g.steps >= 1990);
	g = run(7680, 0.0, 1000, 10, 0, 1000, 3, 10000, 10099, -1);
	CHECK(g.err_max == 0 && g.steps < 300 && g.steps >= 297);
	OK("a sample lost hardly felt; a gap leaves its steps missing");

	/* a sample out of range marks the step beside it */
	g = run(7680, 0.0, 2000, 1, 0, 1000, 1, -1, -1, 4000);
	CHECK(g.marked == 1 || g.marked == 2);
	OK("a sample out of range marks the step beside it");
	return 0;
}
