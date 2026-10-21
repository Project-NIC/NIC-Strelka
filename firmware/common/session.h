/* NIC-Strelka — a session's file composed block by block as the steps come (RECORD.md, The
 * file): the head, the set's table, data blocks of 40 steps with an information block after every
 * 120, the end. The composer knows nothing of the card: every finished block goes out through a
 * callback, on a unit into the buffer before the card, on the host into a file. The steps come
 * in their order, k = 0, 1, 2 …, a lost one as REC_STEP_MISSING, from steps.c. */
#ifndef STRELKA_SESSION_H
#define STRELKA_SESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "record.h"

typedef void (*session_out_t)(const uint8_t block[REC_BLOCK_BYTES], void *ctx);

struct session_step_info {  /* what the information block keeps of a step */
	int32_t offset_ns;      /* its data-ready against the grid; the data block takes the
				 * first step's whole, the information block each one's i16 */
	int8_t signal_dbm;      /* the SYNC's; REC_NO_SIGNAL none */
	uint8_t povodok;        /* 0–3, REC_NO_POVODOK none */
	uint8_t syncs;          /* heard in the period, 0–4 */
	bool acc_out;           /* the step out of range */
	bool gyro_out;
};

struct session {
	struct rec_session cfg;
	session_out_t out;
	void *ctx;
	uint32_t steps_a_period;
	uint32_t number;        /* the next data or information block's */
	uint32_t steps;         /* taken so far */
	uint32_t blocks;        /* data and information blocks written */
	uint32_t last_period;
	bool open;
	/* the data block being filled */
	struct strelka_step step[REC_STEPS];
	unsigned fill;
	int32_t first_offset_ns;
	uint8_t acc_out, gyro_out;
	/* the information block being filled */
	struct rec_info_entry info[REC_INFO_STEPS];
	unsigned info_fill;
	int16_t temp;
	uint16_t cell_mv;
	uint8_t block[REC_BLOCK_BYTES];
};

void session_begin(struct session *s, const struct rec_session *cfg, session_out_t out, void *ctx);
void session_table(struct session *s, const char *text, size_t length);
void session_step(struct session *s, const struct strelka_step *step,
		  const struct session_step_info *info, int16_t temp, uint16_t cell_mv);
/* the end: the last data and information blocks, however full, then the end block */
void session_end(struct session *s, uint8_t why, uint32_t dropped);
uint32_t session_period_of(const struct session *s, uint32_t k);   /* step k's period */
uint8_t session_place_of(const struct session *s, uint32_t k);     /* its block's place */

#endif
