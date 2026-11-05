/* NIC-Strelka — a sensor off the set's clock put onto the set's grid (FIRMWARE.md, Reading the
 * sensor): the LSM6DSV320X, the hoof's, runs on its own clock, since its 320 g channel has no
 * ODR-triggered mode (DS14623 Rev 3, table 21), at an ODR at least 1,9 times the rate on the grid.
 * Every sample comes with its data-ready's stamp, captured in hardware; each point of the grid
 * between two samples takes the straight line between them at its instant, so that the steps
 * after it — common/steps.c — find the samples on the grid as from a steered sensor. Across a gap
 * longer than `max_gap` — samples lost — nothing is drawn: the grid's points there stay missing,
 * marked so by the steps. Plain C, checked on the host (tests/test_resample.c). */
#ifndef STRELKA_RESAMPLE_H
#define STRELKA_RESAMPLE_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

/* a point of the grid: its number from period 0, its values, whether either sample it was drawn
 * between was out of range */
typedef void (*resample_out_t)(int64_t index, const struct strelka_step *s, bool acc_out,
			       bool gyro_out, void *ctx);

struct resample {
	uint32_t period;         /* the grid's sample period, ticks of 16 MHz */
	uint32_t max_gap;        /* the longest between two samples drawn across, ticks */
	resample_out_t out;
	void *ctx;
	bool have;               /* a sample before */
	int64_t at;              /* its time from period 0, ticks */
	struct strelka_step v;
	bool acc_out, gyro_out;
	int64_t next;            /* the next point of the grid to give */
};

void resample_begin(struct resample *r, uint32_t grid_period, uint32_t max_gap,
		    resample_out_t out, void *ctx);
/* a sample at `at` ticks from period 0, in order; an older one or a repeat is left */
void resample_sample(struct resample *r, int64_t at, const struct strelka_step *s, bool acc_out,
		     bool gyro_out);

#endif
