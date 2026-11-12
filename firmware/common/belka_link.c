#include <string.h>
#include "belka_link.h"

void belka_link_init(struct belka_link *l, const struct belka_link_cfg *cfg)
{
	memset(l, 0, sizeof(*l));
	l->cfg = *cfg;
	if (l->cfg.stop_s == 0) {
		l->cfg.stop_s = LINK_STOP_S_DEFAULT;
	}
	if (l->cfg.off_s == 0) {
		l->cfg.off_s = LINK_OFF_S_DEFAULT;
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
	l->root_known = false;
	l->no_sync_periods = 0;
	l->spare_known = false;
	l->move_pending = false;
}

static void sync_heard(struct belka_link *l, const struct frame *f, int8_t rssi)
{
	uint8_t move = SET_MOVE(f->state);

	/* two firsts, Povodoks that do not hear each other: the lower first's time alone, a higher
	 * one's SYNC nothing to this Belka until five seconds have passed without the lower */
	bool lately = l->root_known &&
		      (f->period - l->root_period < LINK_TAKEOVER_LOST_PERIODS ||
		       l->root_period - f->period < LINK_TAKEOVER_LOST_PERIODS);

	if (lately && f->root > l->root_best) {
		return;
	}
	if (!lately || f->root <= l->root_best) {
		l->root_known = true;
		l->root_best = f->root;
		l->root_period = f->period;
	}
	if (l->syncs_heard == 0 || f->root < l->taken_root) {
		/* the first heard in the period is taken: its time, and its content — the other
		 * Povodoks repeat the first's a period later, for those that missed it, and two
		 * standing up together may count differently for a period; one of a lower first
		 * heard after it takes its place */
		l->period = f->period;
		l->period_known = true;
		l->taken_root = f->root;
		l->taken_povodok = f->number;
		l->taken_signal = rssi;
		l->live_code = l->cfg.aside ? S0_LIVE_OFF : S0_HIGH(f->s0);
		l->item_asked = S0_LOW(f->s0);
	}
	if ((f->state & SET_RECORD) && !l->cfg.aside) {
		l->run_seen = true;
		l->stop_count = 0;
	} else if (l->recording && l->stop_count < LINK_STOP_SYNCS) {
		l->stop_count++;
	}
	if (f->number < STRELKA_POVODOKS_MAX) {
		uint8_t k = f->number;

		l->pov_seen[k] = true;
		l->pov_period[k] = f->period;
		l->pov_rssi[k] = rssi;
		l->pov_downloads[k] = (f->state & SET_DOWNLOADS) != 0;
		if ((f->state & SET_ESB) && move == SET_MOVE_NONE) {
			l->pov_esb_seen[k] = true;   /* its channel on ESB in the channel's field */
			l->pov_esb_period[k] = f->period;
			l->pov_esb_channel[k] = SET_CHANNEL(f->state);
		}
	}
	if (f->state & SET_ESB) {
		l->esb_pending = true;           /* a frame waits for one of us: on ESB 10 s on */
		belka_link_spoken(l);
	}
	if (move == SET_MOVE_TWO || move == SET_MOVE_NEXT ||
	    (move == SET_MOVE_NONE && !(f->state & SET_ESB) && l->syncs_heard == 0 &&
	     !l->move_pending)) {
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

/* nothing recorded and the live view off: the set's SYNCs in the state's period alone */
static bool at_rest(const struct belka_link *l)
{
	return !l->recording && l->live_code == S0_LIVE_OFF;
}

/* the set's time known and not lost: a SYNC or a CALL with a period outside a small window of
 * the own count is a replay or a stranger's, the signature stopping forgery alone */
static bool period_plausible(const struct belka_link *l, uint32_t period)
{
	if (!l->period_known || l->no_sync_periods >= LINK_LOST_PERIODS) {
		return true;
	}
	return period + LINK_PERIOD_WINDOW >= l->period && period <= l->period + LINK_PERIOD_WINDOW;
}

void belka_link_heard(struct belka_link *l, const struct frame *f, int8_t rssi)
{
	if (f->sender != WHO_POVODOK || l->asleep || l->esb) {
		return;                          /* the Povodoks alone, and only when listening */
	}
	if ((f->kind == FRAME_SYNC || f->kind == FRAME_CALL) && !period_plausible(l, f->period)) {
		return;
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

/* the Povodok this Belka takes on ESB, said in item 8: of those joined to the phone heard in the
 * last second, the ones within LINK_SPLIT_DB of the strongest, in turn by the Belka's number,
 * every fourth number shifted by one, so that Belkas numbered leg by leg spread too — on the
 * table, every one hearing every Belka, they share the set evenly; none joined, the lowest heard */
static uint8_t esb_povodok(const struct belka_link *l)
{
	uint8_t near[STRELKA_POVODOKS_MAX], n = 0, lowest = 0xFF;
	int best = INT8_MIN;

	for (uint8_t k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		bool fresh = l->pov_seen[k] && l->period - l->pov_period[k] < LINK_PERIODS_A_S;

		if (fresh && lowest == 0xFF) {
			lowest = k;
		}
		if (fresh && l->pov_downloads[k] && l->pov_rssi[k] > best) {
			best = l->pov_rssi[k];
		}
	}
	for (uint8_t k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		if (l->pov_seen[k] && l->period - l->pov_period[k] < LINK_PERIODS_A_S &&
		    l->pov_downloads[k] && l->pov_rssi[k] >= best - LINK_SPLIT_DB) {
			near[n++] = k;
		}
	}
	return n ? near[(l->cfg.number + l->cfg.number / 4) % n] : lowest == 0xFF ? 0 : lowest;
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
			if (l->signal_ring_heard[i] == BELKA_RING_HEARD) {
				sum += l->signal_ring[i];
				n++;
			} else if (l->signal_ring_heard[i] == BELKA_RING_MISSED) {
				missed++;
			}                        /* asleep: not listened, not missed */
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
		return ITEM_BYTES(l->measure_channel | esb_povodok(l) << 6, l->measured_noise);
	default:
		return 0;
	}
}

void belka_link_slot(struct belka_link *l, const struct belka_facts *facts,
		     struct belka_live_out *live)
{
	uint8_t item;

	memset(live, 0, sizeof(*live));
	if (l->cfg.number == 0 || !l->period_known || l->paused || l->esb) {
		return;                          /* a new Belka sends HEREs alone; paused, nothing */
	}
	/* in every period with the live view on, once a second with it off */
	if (l->live_code == S0_LIVE_OFF && l->period % LINK_STATE_EVERY != 0) {
		return;
	}
	item = l->trouble ? ITEM_STATE : l->item_asked;
	live->send = true;
	live->s0 = S0(l->live_code, item);
	if (facts->live_high_g && (l->live_code == S0_RAW_50 || l->live_code == S0_RAW_100)) {
		live->s0 = S0(l->live_code == S0_RAW_50 ? S0_RAW_50_HG : S0_RAW_100_HG, item);
	}
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

	/* a period on ESB: no SYNC listened, nothing counted, the packets for it keeping it on; 10 s
	 * of quiet return it to the SYNCs, listened in every period until one is heard */
	if (l->esb) {
		l->signal_ring[l->ring_at] = 0;
		l->signal_ring_heard[l->ring_at] = BELKA_RING_ASLEEP;
		l->ring_at = (uint8_t)((l->ring_at + 1) % LINK_PERIODS_A_S);
		l->period++;
		l->esb = (int32_t)(l->period - l->spoken_until) < 0;
		l->try_channel = l->channel;
		out->esb = l->esb;
		out->listen_channel = l->esb ? l->esb_channel : l->channel;
		out->state = l->state;
		out->measure_channel = l->measure_channel;
		return;
	}

	/* a period asleep: nothing listened, so nothing learnt and nothing counted; awake again in
	 * the state's period, once a second */
	if (l->asleep) {
		l->signal_ring[l->ring_at] = 0;
		l->signal_ring_heard[l->ring_at] = BELKA_RING_ASLEEP;
		l->ring_at = (uint8_t)((l->ring_at + 1) % LINK_PERIODS_A_S);
		l->period++;                     /* now the next period's number */
		l->asleep = l->period % LINK_STATE_EVERY != 0;
		out->asleep = l->asleep;
		out->listen_channel = l->try_channel;
		out->state = l->state;
		out->measure_channel = l->measure_channel;
		return;
	}

	/* at rest — nothing recorded, the live view off — the SYNCs come in the state's period alone,
	 * and in a move's (PRINCIPLE.md, The inside link): a period between, unheard, is no SYNC
	 * missed, and one missed is a second's */
	bool rest = l->period_known && at_rest(l) && !l->move_pending;
	bool expected = !rest || l->period % LINK_STATE_EVERY == 0;
	bool quiet = !heard && !expected;

	/* this period's record of the SYNCs */
	if (heard) {
		out->taken_povodok = l->taken_povodok;
		out->signal_dbm = l->taken_signal;
	}
	out->syncs_heard = l->syncs_heard;
	l->signal_ring[l->ring_at] = heard ? l->taken_signal : 0;
	l->signal_ring_heard[l->ring_at] = heard ? BELKA_RING_HEARD :
					   quiet ? BELKA_RING_ASLEEP : BELKA_RING_MISSED;
	l->ring_at = (uint8_t)((l->ring_at + 1) % LINK_PERIODS_A_S);
	if (heard) {
		l->no_sync_periods = 0;
		l->rest_missed = 0;
	} else if (!quiet) {
		l->no_sync_periods += rest ? LINK_STATE_EVERY : 1;
		if (rest && l->rest_missed < UINT8_MAX) {
			l->rest_missed++;
		}
	}

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

	/* every Povodok gone: waiting, the unit switches itself off after the setting's seconds;
	 * paused, after three times as long, its end written */
	if (!l->recording && l->no_sync_periods >= (uint32_t)l->cfg.off_s * LINK_PERIODS_A_S) {
		out->off = true;
	} else if (l->recording && l->paused &&
		   l->no_sync_periods >= 3u * l->cfg.off_s * LINK_PERIODS_A_S) {
		l->recording = false;
		l->paused = false;
		out->record_stop = true;
		out->stop_why = END_SILENT;
		out->off = true;
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
	if (quiet) {
		/* at rest between the state's periods: listened where it was */
	} else if (rest && !heard) {
		/* at rest, the state's SYNCs missed running: the set's own again, then the spare, then
		 * the set's other channels in turn, a state's period each, the own every other */
		if (l->rest_missed < LINK_REST_SPARE ||
		    (l->rest_missed >= LINK_REST_LOST && l->rest_missed % 2 == 1)) {
			l->try_channel = l->channel;
		} else if (l->rest_missed < LINK_REST_LOST) {
			l->try_channel = l->spare_known ? l->spare : l->channel;
			l->sweep = l->channel;
		} else {
			l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
			if (l->sweep == l->channel) {
				l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
			}
			l->try_channel = l->sweep;
		}
	} else if (!l->period_known && !heard) {
		/* the set's time not known: its own channel a second, then a second of the set's other
		 * channels, a period each, and so on — a set at rest, its SYNC once a second, heard on
		 * its own within two */
		if (l->no_sync_periods < LINK_LOST_PERIODS ||
		    (l->no_sync_periods / LINK_PERIODS_A_S) % 2 == 0) {
			l->try_channel = l->channel;
		} else {
			l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
			if (l->sweep == l->channel) {
				l->sweep = (uint8_t)((l->sweep + 1) % STRELKA_SET_CHANNELS);
			}
			l->try_channel = l->sweep;
		}
	} else if (heard || l->no_sync_periods < LINK_ALONE_PERIODS || l->no_sync_periods % 2 == 1) {
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
	out->measure_channel = (uint8_t)((l->cfg.number + l->period + 1) % LINK_MEASURED_CHANNELS);
	l->measure_channel = out->measure_channel;
	l->measured_noise = LINK_NOISE_UNKNOWN;
	l->syncs_heard = 0;
	l->taken_povodok = REC_NO_POVODOK;
	l->taken_signal = REC_NO_SIGNAL;
	l->run_seen = false;
	if (l->period_known) {
		l->period++;
	}

	/* on ESB from the next period, a SYNC's bit 14 heard, on the channel of the Povodok it takes
	 * there, as that one's SYNC says it — the first's at once, another's from its repeat a period
	 * later; a period with no SYNC, the SYNCs paused, on the last known or the set's */
	if (l->esb_pending) {
		uint8_t k = esb_povodok(l);
		bool known = l->pov_esb_seen[k] &&
			     l->period - 1 - l->pov_esb_period[k] < LINK_PERIODS_A_S;

		if (l->recording || l->live_code != S0_LIVE_OFF ||
		    (int32_t)(l->period - l->spoken_until) >= 0) {
			l->esb_pending = false;  /* said no more */
		} else if (known || (!heard && expected)) {
			l->esb_pending = false;
			l->esb = true;
			l->esb_povodok = k;
			l->esb_channel = known ? l->pov_esb_channel[k] : l->channel;
			out->listen_channel = l->esb_channel;
		}
	}
	out->esb = l->esb;
	if (l->warm_left) {
		l->warm_left--;
	}
	/* asleep from the next period: the SYNC just heard, or at rest the state's missed, the set's
	 * SYNCs silent till the next — nothing recorded, the live view off, no move said */
	l->asleep = l->cfg.sleep && l->period_known && (heard || (rest && !quiet)) &&
		    !l->recording && !l->esb && !l->esb_pending &&
		    l->live_code == S0_LIVE_OFF && !l->move_pending && facts->unit_state == 0 &&
		    l->warm_left == 0 &&
		    l->period % LINK_STATE_EVERY != 0 &&      /* l->period now the next one's */
		    (int32_t)(l->period - l->spoken_until) >= 0;
	out->asleep = l->asleep;
}

uint32_t belka_link_period(const struct belka_link *l)
{
	return l->period;
}

void belka_link_warm(struct belka_link *l)
{
	l->warm_left = LINK_WARM_PERIODS;
	l->asleep = false;
}

void belka_link_spoken(struct belka_link *l)
{
	l->spoken_until = l->period + LINK_SPOKEN_PERIODS;
	l->asleep = false;
}
