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
	unsigned n = s->n_groups < REC_GROUPS_MAX ? s->n_groups : REC_GROUPS_MAX;

	memset(block, 0, REC_BLOCK_BYTES);
	memset(&h, 0, sizeof(h));
	memcpy(h.mark, "STRK", 4);
	h.version = REC_VERSION;
	h.kind = REC_HEAD;
	h.session = s->session;
	h.belka = s->belka;
	h.chip = s->chip;
	memcpy(h.fw, s->fw, 3);
	h.rate_hz = s->rate_hz;
	h.n = s->n;
	h.clock = s->clock;
	h.time_us = s->time_us;
	h.time_period = s->time_period;
	h.tz_q = (int8_t)(s->tz_min / 15);          /* every zone a whole quarter hour */
	put_text(h.sensor, sizeof(h.sensor), s->sensor);
	h.cal_temp = s->cal_temp;
	h.cal_time = s->cal_time;
	h.groups = (uint8_t)n;
	memcpy(block, &h, sizeof(h));
	for (unsigned i = 0; i < n; i++) {
		memcpy(block + REC_GROUPS_AT + i * sizeof(struct rec_group), &s->groups[i],
		       sizeof(struct rec_group));
	}
	rec_seal(block);
}

static void group(struct rec_group *g, uint8_t kind, uint8_t first, uint8_t count, uint8_t type,
		  const char *name, const char *unit, uint32_t step, uint16_t range)
{
	memset(g, 0, sizeof(*g));
	g->kind = kind;
	g->first = first;
	g->count = count;
	g->type = type;
	put_text(g->name, sizeof(g->name), name);
	put_text(g->unit, sizeof(g->unit), unit);
	g->step = step;
	g->range = range;
}

unsigned rec_imu_groups(struct rec_group g[REC_GROUPS_MAX], const struct rec_imu *imu)
{
	unsigned n = 0;

	group(&g[n], REC_DATA, 0, 3, REC_TYPE_I16, imu->high_g ? "ACC_HG" : "ACC", "g",
	      imu->acc_step_ng, imu->acc_range);
	if (imu->cal) {
		g[n].cal_range = imu->cal->range[0];
		memcpy(g[n].cal, imu->cal->acc, sizeof(g[n].cal));
	}
	n++;
	group(&g[n], REC_DATA, 3, 3, REC_TYPE_I16, "GYRO", "dps", imu->gyro_step_ndps,
	      imu->gyro_range);
	for (int i = 0; i < 3; i++) {
		g[n].cal[i] = imu->cal ? (uint16_t)imu->cal->gyro_zero[i] : 0;
		g[n].cal[3 + i] = imu->cal ? imu->cal->gyro_scale[i] : 32768;
	}
	g[n].cal_range = imu->cal ? imu->cal->range[1] : 0;
	n++;
	group(&g[n++], REC_INFO, 0, 1, REC_TYPE_I16, "OFFSET", "ns", 1000000000u, 0);
	group(&g[n++], REC_INFO, 1, 1, REC_TYPE_I8, "SIGNAL", "dBm", 1000000000u, 0);
	group(&g[n++], REC_INFO, 2, 1, REC_TYPE_U8, "RADIO", "", 0, 0);
	if (imu->hg_range) {
		group(&g[n], REC_HG, 0, 3, REC_TYPE_I16, "ACC_HG", "g", imu->hg_step_ng,
		      imu->hg_range);
		if (imu->cal) {
			g[n].cal_range = imu->cal->hg_range;
			memcpy(g[n].cal, imu->cal->hg, sizeof(g[n].cal));
		}
		n++;
	}
	return n;
}

unsigned rec_table_blocks(size_t length)
{
	return length ? (unsigned)((length + REC_TABLE_TEXT - 1) / REC_TABLE_TEXT) : 1u;
}

uint64_t rec_count_at(bool hg, uint64_t at)
{
	return hg ? at / 6 * 4 + (at % 6 < 4 ? at % 6 : 4) : at;
}

uint64_t rec_count_end(bool hg, uint64_t at)
{
	uint64_t r = at % 6;

	/* a last cycle cut short: its one or two data blocks and its information block numbered,
	 * one or two high-g blocks after them */
	return hg ? at / 6 * 4 + (r == 0 ? 0 : r - 1 < 3 ? r - 1 : 3) : at;
}

uint64_t rec_count_hg(uint64_t at)
{
	return at / 6 * 4;
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
	if (n) {
		memcpy(b->entry, entries, n * sizeof(*entries));
	}
	rec_seal(block);
}

void rec_hg_block(uint8_t block[REC_BLOCK_BYTES], uint16_t session, uint32_t number,
		  uint32_t period, uint8_t place, uint8_t half, const int16_t (*steps)[3], unsigned n)
{
	struct rec_hg *b = (struct rec_hg *)block;
	unsigned out = 0;

	memset(block, 0, REC_BLOCK_BYTES);
	if (n > REC_HG_STEPS) {
		n = REC_HG_STEPS;
	}
	b->kind = REC_HG;
	b->steps = (uint8_t)n;
	b->session = session;
	b->number = number;
	b->period = period;
	b->place = place;
	b->half = half;
	for (unsigned i = 0; i < n; i++) {
		bool lost = true, clipped = false;

		for (int j = 0; j < 3; j++) {
			b->step[i][j] = steps[i][j];
			lost = lost && steps[i][j] == REC_STEP_MISSING;
			clipped = clipped || steps[i][j] == INT16_MAX || steps[i][j] == INT16_MIN;
		}
		out += !lost && clipped;
	}
	b->out = (uint8_t)(out > UINT8_MAX ? UINT8_MAX : out);
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
