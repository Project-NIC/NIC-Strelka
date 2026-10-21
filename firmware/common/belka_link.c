#include <string.h>
#include "belka_link.h"

void belka_link_init(struct belka_link *l, const struct belka_link_cfg *cfg)
{
	memset(l, 0, sizeof(*l));
	l->cfg = *cfg;
	if (l->cfg.stop_s == 0) {
		l->cfg.stop_s = LINK_STOP_S_DEFAULT;
	}
	l->channel = cfg->number ? cfg->channel : STRELKA_SEARCH_CHANNEL;
	l->try_channel = l->channel;
	l->taken_povodok = REC_NO_POVODOK;
	l->taken_signal = REC_NO_SIGNAL;
	l->here_window = -1;
	l->measured_noise = LINK_NOISE_UNKNOWN;
	l->state = BELKA_WAITING;
	l->sent_state = 0xFF;            /* the first LIVE says the state, whatever is asked */
}

void belka_link_join(struct belka_link *l, uint8_t number, uint32_t address, uint8_t channel)
{
	l->cfg.number = number;
	l->cfg.address = address;
	l->cfg.channel = channel;
	l->channel = channel;
	l->try_channel = channel;
	l->period_known = false;
	l->no_sync_periods = 0;
	l->spare_known = false;
	l->move_pending = false;
}

static void sync_heard(struct belka_link *l, const struct frame *f, int8_t rssi)
{
	uint8_t move = SET_MOVE(f->state);

	if (l->syncs_heard == 0) {
		/* the first heard in the period is taken: its time, and its content — the other
		 * Povodoks repeat the first's a period later, for those that missed it, and two
		 * standing up together may count differently for a period */
		l->period = f->period;
		l->period_known = true;
		l->taken_povodok = f->number;
		l->taken_signal = rssi;
		l->live_code = S0_HIGH(f->s0);
		l->item_asked = S0_LOW(f->s0);
	}
	if (f->state & SET_RECORD) {
		l->run_seen = true;
		l->stop_count = 0;
	} else if (l->recording && l->stop_count < LINK_STOP_SYNCS) {
		l->stop_count++;
	}
	if (move == SET_MOVE_TWO || move == SET_MOVE_NEXT ||
	    (move == SET_MOVE_NONE && l->syncs_heard == 0 && !l->move_pending)) {
		/* the spare: from a move said by anyone, or from the taken SYNC's even reading —
		 * a repeater's SYNC carries the last period's and must not undo a move pending */
		l->spare = SET_CHANNEL(f->state);
		l->spare_known = true;
	}
	if (l->syncs_heard < 4) {
		l->syncs_heard++;
	}
	if (move == SET_MOVE_TWO) {
		l->move_period = f->period + 2;
		l->move_pending = true;
	} else if (move == SET_MOVE_NEXT) {
		l->move_period = f->period + 1;
		l->move_pending = true;
	}
}

static void call_heard(struct belka_link *l, const struct frame *f)
{
	uint16_t draw = search_draw(l->cfg.chip, S0_LOW(f->s0));

	l->period = f->period;
	l->period_known = true;
	l->here_window = f->period % LINK_SEARCH_ROUND == SEARCH_PERIOD(draw) ?
			 (int16_t)SEARCH_WINDOW(draw) : -1;
}

void belka_link_heard(struct belka_link *l, const struct frame *f, int8_t rssi)
{
	if (f->sender != WHO_POVODOK) {
		return;                          /* a Belka listens to the Povodoks alone */
	}
	if (l->cfg.number == 0) {
		if (f->kind == FRAME_CALL) {
			call_heard(l, f);
		}
		return;                          /* new: the search's channel, CALLs alone */
	}
	if (f->kind == FRAME_SYNC) {
		sync_heard(l, f, rssi);
	}
}

int16_t belka_link_here(const struct belka_link *l)
{
	return l->cfg.number == 0 ? l->here_window : -1;
}

void belka_link_measured(struct belka_link *l, int8_t noise_dbm)
{
	l->measured_noise = noise_dbm;
}

static uint16_t item_value(struct belka_link *l, uint8_t item, const struct belka_facts *facts)
{
	switch (item) {
	case ITEM_CELL_TEMP:
		return ITEM_BYTES(facts->cell_pct, facts->temp_c);
	case ITEM_CELL_MV:
		return facts->cell_mv;
	case ITEM_CARD:
		return ITEM_BYTES(facts->card_free_pct, facts->dropped);
	case ITEM_SYNCS: {
		int sum = 0, n = 0, missed = 0;

		for (unsigned i = 0; i < LINK_PERIODS_A_S; i++) {
			if (l->signal_ring_heard[i]) {
				sum += l->signal_ring[i];
				n++;
			} else {
				missed++;
			}
		}
		return ITEM_BYTES(n ? sum / n : REC_NO_SIGNAL, missed);
	}
	case ITEM_OFFSET:
		return (uint16_t)facts->offset_ns;
	case ITEM_STATE:
		l->sent_state = l->state;
		l->sent_fault = facts->fault;
		l->trouble = false;
		return ITEM_BYTES(l->state, facts->fault);
	case ITEM_OUT:
		return ITEM_BYTES(facts->acc_out, facts->gyro_out);
	case ITEM_CHANNEL:
		return ITEM_BYTES(l->measure_channel, l->measured_noise);
	default:
		return 0;
	}
}

void belka_link_slot(struct belka_link *l, const struct belka_facts *facts,
		     struct belka_live_out *live)
{
	uint8_t item;

	memset(live, 0, sizeof(*live));
	if (l->cfg.number == 0 || !l->period_known || l->paused) {
		return;                          /* a new Belka sends HEREs alone; paused, nothing */
	}
	/* in every period with the live view on, once a second with it off */
	if (l->live_code == S0_LIVE_OFF && l->period % LINK_STATE_EVERY != 0) {
		return;
	}
	item = l->trouble ? ITEM_STATE : l->item_asked;
	live->send = true;
	live->s0 = S0(l->live_code, item);
	live->state = item_value(l, item, facts);
}

static uint8_t own_state(const struct belka_link *l, const struct belka_facts *facts)
{
	if (facts->unit_state >= BELKA_CALIBRATING) {
		return facts->unit_state;        /* calibrating, giving, taking a firmware, erasing */
	}
	if (l->paused) {
		return BELKA_PAUSED;
	}
	if (!l->recording) {
		return BELKA_WAITING;
	}
	if (l->no_sync_periods >= LINK_LOST_PERIODS) {
		return BELKA_RECORDING_SEARCHING;
	}
	if (l->no_sync_periods >= LINK_ALONE_PERIODS) {
		return BELKA_RECORDING_ALONE;
	}
	return BELKA_RECORDING;
}

void belka_link_period_end(struct belka_link *l, const struct belka_facts *facts,
			   struct belka_period_out *out)
{
	bool heard = l->syncs_heard > 0;

	memset(out, 0, sizeof(*out));
	out->taken_povodok = REC_NO_POVODOK;
	out->signal_dbm = REC_NO_SIGNAL;
	if (l->cfg.number == 0) {
		/* a new Belka: the search's channel, a HERE when its draw says so */
		out->listen_channel = STRELKA_SEARCH_CHANNEL;
		out->state = BELKA_WAITING;
		l->here_window = -1;
		if (l->period_known) {
			l->period++;
		}
		return;
	}

	/* this period's record of the SYNCs */
	if (heard) {
		out->taken_povodok = l->taken_povodok;
		out->signal_dbm = l->taken_signal;
	}
	out->syncs_heard = l->syncs_heard;
	l->signal_ring[l->ring_at] = heard ? l->taken_signal : 0;
	l->signal_ring_heard[l->ring_at] = heard;
	l->ring_at = (uint8_t)((l->ring_at + 1) % LINK_PERIODS_A_S);
	l->no_sync_periods = heard ? 0 : l->no_sync_periods + 1;

	/* the record: from the period after the first SYNC saying it runs; stopped after five
	 * SYNCs saying it does not; paused after the setting's seconds without any */
	if (l->run_seen) {
		if (!l->recording) {
			l->recording = true;
			out->record_start = true;
			out->first_period = l->period + 1;
		} else if (l->paused) {
			l->paused = false;
			out->resume = true;
		}
	} else if (l->recording && l->stop_count >= LINK_STOP_SYNCS) {
		l->recording = false;
		l->paused = false;
		l->stop_count = 0;
		out->record_stop = true;
		out->stop_why = END_SYNC_STOP;
	} else if (l->recording && !l->paused &&
		   l->no_sync_periods >= (uint32_t)l->cfg.stop_s * LINK_PERIODS_A_S) {
		l->paused = true;
		out->pause = true;
	}

	/* the channel: a move counted down, every Belka moving in the same period; a SYNC heard on
	 * a channel tried makes it the set's; none heard, every other period on the set's own —
	 * where another Povodok may stand up as the first — and between, the spare from the
	 * second miss, and from a second on each of the set's other channels in turn */
	if (heard) {
		l->channel = l->try_channel;
	}
	if (l->move_pending && l->period + 1 >= l->move_period) {
		l->channel = l->spare;
		l->move_pending = false;
	}
	l->cfg.channel = l->channel;
	if (heard || l->no_sync_periods < LINK_ALONE_PERIODS || l->no_sync_periods % 2 == 1) {
		l->try_channel = l->channel;
		if (l->no_sync_periods < LINK_LOST_PERIODS) {
			l->sweep = l->channel;   /* a sweep to come starts from the set's own */
		}
	} else if (l->no_sync_periods >= LINK_LOST_PERIODS) {
		l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
		if (l->sweep == l->channel) {
			l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
		}
		l->try_channel = l->sweep;
	} else if (l->spare_known) {
		l->try_channel = l->spare;
	} else {
		l->try_channel = l->channel;
	}
	out->listen_channel = l->try_channel;

	/* the state, and the trouble that puts it in the next LIVE */
	l->state = own_state(l, facts);
	if (l->state != l->sent_state || facts->fault != l->sent_fault) {
		l->trouble = true;
	}
	out->state = l->state;

	/* the next period: its channel to measure, the Belka's number and the period's */
	out->measure_channel = (uint8_t)((l->cfg.number + l->period + 1) % STRELKA_CHANNELS);
	l->measure_channel = out->measure_channel;
	l->measured_noise = LINK_NOISE_UNKNOWN;
	l->syncs_heard = 0;
	l->taken_povodok = REC_NO_POVODOK;
	l->taken_signal = REC_NO_SIGNAL;
	l->run_seen = false;
	if (l->period_known) {
		l->period++;
	}
}

uint32_t belka_link_period(const struct belka_link *l)
{
	return l->period;
}
