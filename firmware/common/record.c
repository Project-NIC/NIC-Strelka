#include <string.h>
#include "crc32.h"
#include "record.h"

static void put_le32(uint8_t *at, uint32_t v)
{
	at[0] = (uint8_t)v;
	at[1] = (uint8_t)(v >> 8);
	at[2] = (uint8_t)(v >> 16);
	at[3] = (uint8_t)(v >> 24);
}

static uint32_t get_le32(const uint8_t *at)
{
	return (uint32_t)at[0] | ((uint32_t)at[1] << 8) | ((uint32_t)at[2] << 16) |
	       ((uint32_t)at[3] << 24);
}

void rec_seal(uint8_t block[REC_BLOCK_BYTES])
{
	put_le32(block + REC_CRC_AT, strelka_crc32(block, REC_BODY_BYTES));
}

bool rec_block_ok(const uint8_t block[REC_BLOCK_BYTES])
{
	return strelka_crc32(block, REC_BODY_BYTES) == get_le32(block + REC_CRC_AT);
}

static void put_text(char *to, size_t room, const char *text)
{
	size_t n = text ? strlen(text) : 0;

	memset(to, 0, room);
	memcpy(to, text, n < room ? n : room);
}

void rec_head_block(uint8_t block[REC_BLOCK_BYTES], const struct rec_session *s)
{
	struct rec_head h;

	memset(block, 0, REC_BLOCK_BYTES);
	memset(&h, 0, sizeof(h));
	memcpy(h.mark, "STRK", 4);
	h.version = REC_VERSION;
	h.kind = REC_HEAD;
	h.session = s->session;
	h.belka = s->belka;
	h.chip = s->chip;
	memcpy(h.fw, s->fw, 3);
	memcpy(h.build, s->build, 4);
	h.rate_hz = s->rate_hz;
	h.n = s->n;
	h.acc_g = s->acc_g;
	h.gyro_dps = s->gyro_dps;
	h.first_period = s->first_period;
	h.time_us = s->time_us;
	h.time_period = s->time_period;
	h.tz_min = s->tz_min;
	h.clock = s->clock;
	if (s->cal) {
		memcpy(h.acc_cal, s->cal->acc, sizeof(h.acc_cal));
		memcpy(h.gyro_scale, s->cal->gyro_scale, sizeof(h.gyro_scale));
		memcpy(h.gyro_zero, s->cal->gyro_zero, sizeof(h.gyro_zero));
		h.cal_temp = s->cal->temp;
		h.cal_time = s->cal->time;
	} else {
		/* no calibration: the scale 1,000, the time 0 (RECORD.md, The session's head) */
		for (int i = 0; i < 3; i++) {
			h.gyro_scale[i] = 32768;
		}
	}
	put_text(h.place, sizeof(h.place), s->place);
	put_text(h.name, sizeof(h.name), s->name);
	memcpy(block, &h, sizeof(h));
	rec_seal(block);
}

unsigned rec_table_blocks(size_t length)
{
	return length ? (unsigned)((length + REC_TABLE_TEXT - 1) / REC_TABLE_TEXT) : 1u;
}

void rec_table_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, unsigned part,
		     const uint8_t *text, size_t length)
{
	struct rec_table *t = (struct rec_table *)block;
	size_t at = (size_t)part * REC_TABLE_TEXT;
	size_t n = at < length ? length - at : 0;

	memset(block, 0, REC_BLOCK_BYTES);
	t->kind = REC_TABLE;
	t->part = (uint8_t)part;
	t->session = session;
	t->length = (uint16_t)length;
	if (n > REC_TABLE_TEXT) {
		n = REC_TABLE_TEXT;
	}
	memcpy(t->text, text + at, n);
	rec_seal(block);
}

bool rec_step_missing(const struct strelka_step *s)
{
	for (int i = 0; i < 3; i++) {
		if (s->a[i] != REC_STEP_MISSING || s->g[i] != REC_STEP_MISSING) {
			return false;
		}
	}
	return true;
}

void rec_data_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint32_t number,
		    uint32_t period, uint8_t place, int32_t offset_ns, uint8_t acc_out,
		    uint8_t gyro_out, const struct strelka_step *steps, unsigned n)
{
	struct rec_data *d = (struct rec_data *)block;

	memset(block, 0, REC_BLOCK_BYTES);
	if (n > REC_STEPS) {
		n = REC_STEPS;
	}
	d->kind = REC_DATA;
	d->steps = (uint8_t)n;
	d->session = session;
	d->number = number;
	d->period = period;
	d->place = place;
	d->acc_out = acc_out;
	d->gyro_out = gyro_out;
	d->offset_ns = offset_ns;
	for (unsigned i = 0; i < n; i++) {
		d->step[i] = steps[i];
		if (rec_step_missing(&steps[i])) {
			d->missing++;
		}
	}
	rec_seal(block);
}

uint8_t rec_info_who(unsigned povodok, unsigned syncs)
{
	return (uint8_t)(((povodok & 0xFu) << 4) | (syncs & 0xFu));
}

void rec_info_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint32_t number,
		    uint32_t period, int16_t temp, uint16_t cell_mv,
		    const struct rec_info_entry *entries, unsigned n)
{
	struct rec_info *b = (struct rec_info *)block;

	memset(block, 0, REC_BLOCK_BYTES);
	if (n > REC_INFO_STEPS) {
		n = REC_INFO_STEPS;
	}
	b->kind = REC_INFO;
	b->steps = (uint8_t)n;
	b->session = session;
	b->number = number;
	b->period = period;
	b->temp = temp;
	b->cell_mv = cell_mv;
	memcpy(b->entry, entries, n * sizeof(*entries));
	rec_seal(block);
}

void rec_end_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint8_t why, uint32_t steps,
		   uint32_t blocks, uint32_t last_period, uint32_t dropped)
{
	struct rec_end *e = (struct rec_end *)block;

	memset(block, 0, REC_BLOCK_BYTES);
	e->kind = REC_END;
	e->why = why;
	e->session = session;
	e->steps = steps;
	e->blocks = blocks;
	e->last_period = last_period;
	e->dropped = dropped;
	rec_seal(block);
}
