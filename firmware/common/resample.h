/* NIC-Strelka — a sensor on its own clock put onto the set's grid (FIRMWARE.md, The sensor and its
 * clock): every sensor runs on its own clock at its fastest, its own filters off, and every sample
 * comes with its data-ready's stamp, captured in hardware. Where a step holds 16 samples or more,
 * they are first summed into bins of an eighth of a step, each bin its samples' mean at their mean
 * instant. Each step of the record is then drawn by a low-pass filter at its centre, from the bins
 * or samples within ten steps on either side, each weighted by its true distance — a windowed
 * sinc, Kaiser's window for 40 dB, flat within 1 % to 0,43 of the rate into memory and 40 dB down
 * from 0,57, so that what folds over its half lands above 0,43 alone — and the weights' sum
 * divided out, so that a sensor's clock off by a per cent, a sample lost or a sample late changes
 * the weights and not the answer's scale; the step's N points on the grid given the same value.
 * A sample lost, the gap up to `max_gap`, is put back first on the straight line between its
 * neighbours, so that the filter's weights stay whole. A step whose centre falls in a gap longer
 * than `max_gap` is not drawn: its points stay missing,
 * marked so by the steps (common/steps.c). Integers alone; checked on the host
 * (tests/test_resample.c). */
#ifndef STRELKA_RESAMPLE_H
#define STRELKA_RESAMPLE_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define RESAMPLE_MAX      512    /* kept: 20 steps of up to 16 samples or 8 bins a step, and room */
#define RESAMPLE_SPAN     10     /* the filter's half-width, in steps */
#define RESAMPLE_BINS     8      /* bins a step, where a step holds 16 samples or more */
#define RESAMPLE_TABLE    64     /* its table's points a step */

/* a point of the grid: its number from period 0, its values, whether a sample beside it was out of
 * range */
typedef void (*resample_out_t)(int64_t index, const struct strelka_step *s, const int16_t *hg,
			       bool acc_out, bool gyro_out, void *ctx);

struct resample_in {
	int64_t at;              /* ticks from period 0 */
	int16_t v[9];            /* a, g, and the high-g's three */
	uint8_t flags;           /* 1 acc out, 2 gyro out, 4 the high-g's kept */
	uint8_t n;               /* the samples it stands for: 1, or a bin's count */
};

struct resample {
	uint32_t grid;           /* the grid's period, ticks of 16 MHz */
	uint32_t step;           /* a step's, N of them */
	uint8_t n;
	int64_t first;           /* the first step's first point, its number on the grid */
	uint32_t period;         /* between two kept: the sensor's own period, or a bin's */
	uint32_t binw;           /* a bin's width, ticks; 0 none, each sample kept as it comes */
	int64_t bin;             /* the open bin's number */
	int64_t bin_at;          /* its samples' instants summed */
	int32_t bin_sum[9];
	uint16_t bin_n;
	uint8_t bin_flags;
	bool any_in;
	int64_t last_in;         /* the last sample taken, so that an older one or a repeat is left */
	uint32_t max_gap;        /* the longest between two samples a step's centre may fall in */
	resample_out_t out;
	void *ctx;
	struct resample_in in[RESAMPLE_MAX];
	uint32_t head, count;    /* the oldest kept, and how many */
	bool begun;
	int64_t next;            /* the next step to draw, from the first */
};

void resample_begin(struct resample *r, uint32_t grid_period, uint8_t n, int64_t first,
		    uint32_t sensor_period, uint32_t max_gap, resample_out_t out, void *ctx);
/* a sample at `at` ticks from period 0, in order; an older one or a repeat is left. The steps
 * whose filter has its samples on both sides are drawn as it comes. */
void resample_sample(struct resample *r, int64_t at, const struct strelka_step *s,
		     const int16_t *hg, bool acc_out, bool gyro_out);
/* at the record's end: the steps up to the last sample drawn from what there is */
void resample_flush(struct resample *r);

#endif
