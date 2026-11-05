#include <string.h>
#include "session.h"

static int16_t clip16(int32_t v)
{
	return (int16_t)(v > INT16_MAX ? INT16_MAX : v < INT16_MIN ? INT16_MIN : v);
}

void session_begin(struct session *s, const struct rec_session *cfg, session_out_t out, void *ctx)
{
	memset(s, 0, sizeof(*s));
	s->cfg = *cfg;
	s->out = out;
	s->ctx = ctx;
	/* 1 at 50 Hz, 640 at 32 kHz; a rate under 50 Hz is none of the design's */
	s->steps_a_period = cfg->rate_hz / STRELKA_PERIODS_A_S;
	if (s->steps_a_period == 0) {
		s->steps_a_period = 1;
	}
	s->last_period = cfg->first_period;
	s->open = true;
	rec_head_block(s->block, &s->cfg);
	s->out(s->block, s->ctx);
}

void session_table(struct session *s, const char *text, size_t length)
{
	unsigned parts;

	if (length > UINT16_MAX) {
		length = UINT16_MAX;             /* the block's length field; COMMANDS.md bounds it */
	}
	parts = rec_table_blocks(length);

	for (unsigned i = 0; i < parts; i++) {
		rec_table_block(s->block, s->cfg.session, i, (const uint8_t *)text, length);
		s->out(s->block, s->ctx);
	}
}

uint32_t session_period_of(const struct session *s, uint32_t k)
{
	return s->cfg.first_period + k / s->steps_a_period;
}

uint8_t session_place_of(const struct session *s, uint32_t k)
{
	return (uint8_t)((k % s->steps_a_period) / REC_STEPS);
}

static void data_out(struct session *s)
{
	uint32_t first = s->steps - s->fill;        /* the block's first step */

	rec_data_block(s->block, s->cfg.session, s->number++, session_period_of(s, first),
		       session_place_of(s, first), s->first_offset_ns, s->acc_out, s->gyro_out,
		       s->step, s->fill);
	s->out(s->block, s->ctx);
	s->blocks++;
	s->fill = 0;
	s->acc_out = s->gyro_out = 0;
}

static void info_out(struct session *s)
{
	uint32_t first = s->steps - s->info_fill;

	rec_info_block(s->block, s->cfg.session, s->number++, session_period_of(s, first), s->temp,
		       s->cell_mv, s->info, s->info_fill);
	s->out(s->block, s->ctx);
	s->blocks++;
	s->info_fill = 0;
}

void session_step(struct session *s, const struct strelka_step *step,
		  const struct session_step_info *info, int16_t temp, uint16_t cell_mv)
{
	struct rec_info_entry *e;

	if (!s->open) {
		return;
	}
	if (s->fill == 0) {
		s->first_offset_ns = info->offset_ns;
	}
	s->step[s->fill++] = *step;
	if (info->acc_out && s->acc_out < UINT8_MAX) {
		s->acc_out++;
	}
	if (info->gyro_out && s->gyro_out < UINT8_MAX) {
		s->gyro_out++;
	}
	e = &s->info[s->info_fill++];
	e->offset_ns = clip16(info->offset_ns);
	e->signal_dbm = info->signal_dbm;
	e->who = rec_info_who(info->povodok, info->syncs);
	s->temp = temp;
	s->cell_mv = cell_mv;
	s->last_period = session_period_of(s, s->steps);
	s->steps++;
	if (s->fill == REC_STEPS) {
		data_out(s);
	}
	if (s->info_fill == REC_INFO_STEPS) {
		info_out(s);
	}
}

void session_end(struct session *s, uint8_t why, uint32_t dropped)
{
	if (!s->open) {
		return;
	}
	if (s->fill) {
		data_out(s);
	}
	if (s->info_fill) {
		info_out(s);
	}
	rec_end_block(s->block, s->cfg.session, why, s->steps, s->blocks, s->last_period, dropped);
	s->out(s->block, s->ctx);
	s->open = false;
}
