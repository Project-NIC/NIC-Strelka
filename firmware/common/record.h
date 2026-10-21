/* NIC-Strelka — the record's blocks, built (RECORD.md). Each builder fills one block of 512 B:
 * the fields, zeros to byte 508, the CRC-32 after. tools/strelka_record.py builds the same bytes
 * in Python; the host test holds the two to each other. */
#ifndef STRELKA_RECORD_H
#define STRELKA_RECORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

struct rec_calibration {
	int16_t acc[6];         /* raw readings, x up and down, y, z */
	uint16_t gyro_scale[3]; /* 32 768 for 1,000 */
	int16_t gyro_zero[3];
	int16_t temp;           /* 0,01 °C */
	uint32_t time;          /* s since 1970; 0 none */
};

struct rec_session {        /* what the head says of the session */
	uint16_t session;
	uint8_t belka;
	uint64_t chip;
	uint8_t fw[3];
	uint8_t build[4];
	uint16_t rate_hz;       /* into memory */
	uint8_t n;
	uint8_t acc_g;
	uint16_t gyro_dps;
	uint32_t first_period;
	uint64_t time_us;
	uint32_t time_period;
	int16_t tz_min;
	uint8_t clock;          /* enum rec_clock */
	const struct rec_calibration *cal;   /* NULL none */
	const char *place;      /* UTF-8, 32 B at most kept */
	const char *name;
};

void rec_seal(uint8_t block[REC_BLOCK_BYTES]);              /* the CRC-32 over the first 508 */
bool rec_block_ok(const uint8_t block[REC_BLOCK_BYTES]);
void rec_head_block(uint8_t block[REC_BLOCK_BYTES], const struct rec_session *s);
/* the set's table: how many blocks a text of `length` bytes takes, and its part i */
unsigned rec_table_blocks(size_t length);
void rec_table_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, unsigned part,
		     const uint8_t *text, size_t length);
/* a data block: the fields, then the steps given; a lost step has its six values
 * REC_STEP_MISSING and is counted */
void rec_data_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint32_t number,
		    uint32_t period, uint8_t place, int32_t offset_ns, uint8_t acc_out,
		    uint8_t gyro_out, const struct strelka_step *steps, unsigned n);
void rec_info_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint32_t number,
		    uint32_t period, int16_t temp, uint16_t cell_mv,
		    const struct rec_info_entry *entries, unsigned n);
uint8_t rec_info_who(unsigned povodok, unsigned syncs);      /* povodok 0–3, REC_NO_POVODOK none */
void rec_end_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint8_t why, uint32_t steps,
		   uint32_t blocks, uint32_t last_period, uint32_t dropped);
bool rec_step_missing(const struct strelka_step *s);

#endif
