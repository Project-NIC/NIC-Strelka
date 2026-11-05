/* NIC-Strelka — a sensor off the set's clock put onto the set's grid (resample.h). */
#include <string.h>
#include "resample.h"

/* the first point of the grid after `at`, and never before period 0 */
static int64_t first_after(int64_t at, uint32_t period)
{
	if (at < 0) {
		return 0;
	}
	return at / (int64_t)period + 1;
}

/* a + (b − a) · num / den, rounded to the nearest */
static int16_t line(int16_t a, int16_t b, int64_t num, int64_t den)
{
	int64_t d = ((int64_t)b - a) * num * 2;

	d = d >= 0 ? (d + den) / (2 * den) : -((-d + den) / (2 * den));
	return (int16_t)(a + d);
}

void resample_begin(struct resample *r, uint32_t grid_period, uint32_t max_gap,
		    resample_out_t out, void *ctx)
{
	memset(r, 0, sizeof(*r));
	r->period = grid_period;
	r->max_gap = max_gap;
	r->out = out;
	r->ctx = ctx;
}

void resample_sample(struct resample *r, int64_t at, const struct strelka_step *s, bool acc_out,
		     bool gyro_out)
{
	if (r->have && at <= r->at) {
		return;
	}
	if (!r->have || at - r->at > (int64_t)r->max_gap) {
		/* the first, or after a gap: the grid takes up after it */
		r->next = first_after(at, r->period);
	} else {
		int64_t den = at - r->at;

		for (int64_t t = r->next * (int64_t)r->period; t <= at;
		     r->next++, t += r->period) {
			struct strelka_step p;
			int64_t num = t - r->at;

			for (int i = 0; i < 3; i++) {
				p.a[i] = line(r->v.a[i], s->a[i], num, den);
				p.g[i] = line(r->v.g[i], s->g[i], num, den);
			}
			r->out(r->next, &p, r->acc_out || acc_out, r->gyro_out || gyro_out, r->ctx);
		}
	}
	r->have = true;
	r->at = at;
	r->v = *s;
	r->acc_out = acc_out;
	r->gyro_out = gyro_out;
}
