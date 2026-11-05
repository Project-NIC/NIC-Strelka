#include <string.h>
#include "steps.h"

static const struct strelka_step missing = {
	{ REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING },
	{ REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING },
};

void steps_begin(struct steps *s, uint32_t sensor_hz, uint32_t n, uint32_t first_period,
		 steps_out_t out, void *ctx)
{
	memset(s, 0, sizeof(*s));
	s->n = n ? n : 1;
	s->a_period = sensor_hz / STRELKA_PERIODS_A_S;
	s->first = (int64_t)first_period * s->a_period;
	s->sum_k = -1;
	s->last_index = -1;
	s->out = out;
	s->ctx = ctx;
}

static void step_done(struct steps *s)
{
	struct strelka_step step;

	for (int i = 0; i < 3; i++) {
		step.a[i] = (int16_t)(s->sum[i] / (int32_t)s->sum_n);
		step.g[i] = (int16_t)(s->sum[3 + i] / (int32_t)s->sum_n);
	}
	s->out((uint32_t)s->sum_k, &step, s->acc_out, s->gyro_out, s->first_stamp, s->ctx);
	s->next_k = s->sum_k + 1;
	s->sum_k = -1;
	s->sum_n = 0;
	s->acc_out = s->gyro_out = false;
	memset(s->sum, 0, sizeof(s->sum));
}

void steps_sample(struct steps *s, int64_t index, uint64_t stamp, const struct strelka_step *sample,
		  bool acc_out, bool gyro_out)
{
	int64_t k;

	if (index < s->first || index <= s->last_index) {
		return;                          /* before the start, or had already */
	}
	s->last_index = index;
	k = (index - s->first) / (int64_t)s->n;
	if (k < s->next_k || (s->sum_k >= 0 && k < s->sum_k)) {
		return;                          /* a step had already: not twice */
	}
	if (s->sum_k >= 0 && k != s->sum_k) {
		step_done(s);                    /* a step left short: the mean of what came */
	}
	while (s->next_k < k) {                  /* steps with no sample at all: marked */
		s->out((uint32_t)s->next_k, &missing, false, false, 0, s->ctx);
		s->next_k++;
	}
	if (s->sum_n == 0) {
		s->first_stamp = stamp;
	}
	s->sum_k = k;
	for (int i = 0; i < 3; i++) {
		s->sum[i] += sample->a[i];
		s->sum[3 + i] += sample->g[i];
	}
	s->acc_out |= acc_out;
	s->gyro_out |= gyro_out;
	if (++s->sum_n == s->n) {
		step_done(s);
	}
}

void steps_end(struct steps *s)
{
	if (s->sum_k >= 0) {
		step_done(s);
	}
}
