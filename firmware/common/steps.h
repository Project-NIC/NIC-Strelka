/* NIC-Strelka — the samples summed into the record's steps by their numbers on the set's grid
 * (FIRMWARE.md, Reading the sensor). Step k sums the N samples numbered from the first sample
 * of the period the record began in, plus k × N, so that every Belka's step k is the same N
 * instants; a sample lost leaves its step the mean of the rest; a step lost is given with its
 * six values REC_STEP_MISSING, so that the record keeps its count. A sum is stored as its mean,
 * in the sensor's 16 bits. */
#ifndef STRELKA_STEPS_H
#define STRELKA_STEPS_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

/* a step done: its number, its values, whether a sample was out of range, and the stamp of its
 * first sample, 0 for a step marked lost */
typedef void (*steps_out_t)(uint32_t k, const struct strelka_step *step, bool acc_out,
			    bool gyro_out, uint64_t first_stamp, void *ctx);

struct steps {
	uint32_t n;             /* the samples a step */
	uint32_t a_period;      /* the sensor's samples in a period */
	int64_t first;          /* the first sample's number: the first period × a_period */
	int64_t next_k;         /* the step expected next */
	int64_t sum_k;          /* the step being summed; -1 none */
	int64_t last_index;     /* the last sample taken: a repeat or an older one is dropped */
	uint64_t first_stamp;   /* the stamp of the step's first sample */
	int32_t sum[6];
	uint32_t sum_n;
	bool acc_out, gyro_out;
	steps_out_t out;
	void *ctx;
};

/* the record begins at the first sample of `first_period`; sensor_hz is the sensor's rate, the
 * rate into memory times N */
void steps_begin(struct steps *s, uint32_t sensor_hz, uint32_t n, uint32_t first_period,
		 steps_out_t out, void *ctx);
/* a sample by its number on the grid, from period 0, and its stamp; the steps come out in order,
 * the lost ones marked; a sample before the start, one already taken or one older than the last
 * is left — the stamps come in order, so a repeated index is a lagging reader's repeat */
void steps_sample(struct steps *s, int64_t index, uint64_t stamp, const struct strelka_step *sample,
		  bool acc_out, bool gyro_out);
/* the record's end: the step being summed goes out, whatever it holds, and nothing after */
void steps_end(struct steps *s);

#endif
