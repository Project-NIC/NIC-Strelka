#include <string.h>
#include "povodok_link.h"

void povodok_link_init(struct povodok_link *l, const struct povodok_link_cfg *cfg)
{
	memset(l, 0, sizeof(*l));
	l->cfg = *cfg;
	l->first_number = cfg->number;
	l->spare = (uint8_t)((cfg->channel + 1) % STRELKA_SET_CHANNELS);
	l->try_channel = cfg->channel;
	l->measure_next = (uint8_t)((10u * cfg->number) % STRELKA_CHANNELS);
	l->measured_noise = LINK_NOISE_UNKNOWN;
	l->items_countdown = LINK_ITEMS_EVERY;
	l->item = ITEM_CHANNEL;
	l->margin = 31;                  /* no Belka heard yet: no trouble known */
	memset(l->noise_povodok, LINK_NOISE_UNKNOWN, sizeof(l->noise_povodok));
	memset(l->noise_belka, LINK_NOISE_UNKNOWN, sizeof(l->noise_belka));
	for (unsigned i = 0; i < STRELKA_POVODOKS_MAX; i++) {
		l->margin_povodok[i] = 31;
	}
	for (unsigned i = 0; i <= STRELKA_BELKAS_MAX; i++) {
		l->belka[i].rssi = REC_NO_SIGNAL;
	}
}

void povodok_link_record(struct povodok_link *l, bool run)
{
	l->record = run;
}

void povodok_link_live(struct povodok_link *l, uint8_t code)
{
	l->live_code = code;
}

void povodok_link_search(struct povodok_link *l, bool on)
{
	if (on && !l->searching) {
		l->attempt = 0;
		l->round_start = l->period;
		l->found_n = 0;
	}
	l->searching = on;
}

void povodok_link_take(struct povodok_link *l, uint8_t number, uint64_t chip)
{
	if (number >= 1 && number <= STRELKA_BELKAS_MAX) {
		memset(&l->belka[number], 0, sizeof(l->belka[number]));
		l->belka[number].taken = true;
		l->belka[number].chip = chip;
		l->belka[number].rssi = REC_NO_SIGNAL;
	}
}

void povodok_link_drop(struct povodok_link *l, uint8_t number)
{
	if (number >= 1 && number <= STRELKA_BELKAS_MAX) {
		l->belka[number].taken = false;
	}
}

/* a Belka's LIVE into the first's keeping: heard here (by 0) or passed by Povodok `by`; its item 8,
 * a channel measured, into the map either way (PRINCIPLE.md, The channels) */
static void live_into(struct povodok_link *l, struct povodok_belka *b, const struct frame *f,
		      bool heard_here, uint8_t by, int8_t rssi)
{
	uint8_t item = S0_LOW(f->s0);

	b->last_period = f->period;
	b->by = heard_here ? 0 : by;
	b->s0 = f->s0;
	b->state = f->state;
	b->steps_len = (uint8_t)(f->body_len > LIVE_BODY_MAX ? LIVE_BODY_MAX : f->body_len);
	memcpy(b->steps, f->body, b->steps_len);
	if (heard_here) {
		b->rssi = rssi;
		b->heard = true;
	} else {
		b->passed = true;
	}
	if (item >= ITEM_FIRST && item <= ITEM_LAST) {
		b->item[item] = f->state;
		b->item_known[item] = true;
	}
	if (item == ITEM_CHANNEL) {
		l->noise_belka[ITEM_HIGH(f->state) % STRELKA_CHANNELS] = (int8_t)ITEM_LOW(f->state);
	}
}

static void sync_heard(struct povodok_link *l, const struct frame *f)
{
	uint8_t k = f->number;
	uint8_t move = SET_MOVE(f->state);

	l->povodok_known[k] = true;
	l->povodok_heard_tick[k] = l->tick;
	l->povodok_state[k] = f->state;
	if (move != SET_MEASUREMENT) {
		l->margin_povodok[k] = (uint8_t)SET_MARGIN(f->state);
	} else {
		l->noise_povodok[k][SET_CHANNEL(f->state)] = (int8_t)SET_NOISE_DBM(f->state);
	}
	if (l->content_taken) {
		return;
	}
	if (k >= l->cfg.number) {
		/* a higher number standing as the first while this one listens its second: the set's
		 * time and state taken, to take over with them, nothing repeated — a lower number's
		 * SYNC means the first is there */
		if (l->first || l->time_taken) {
			return;
		}
		l->time_taken = true;
	} else {
		/* the first SYNC of a lower number in the period — the lowest's, the slots being in
		 * the numbers' order: the set's time and its content, repeated in this Povodok's own
		 * slot a period later; the first itself takes it too, to repeat it once it yields */
		l->content_taken = true;
		l->first_number = k;
		l->content_fresh = true;
		l->content_s0 = f->s0;
		l->cfg.channel = l->try_channel;    /* heard where listened, the spare perhaps */
	}
	l->period = f->period;
	l->record = (f->state & SET_RECORD) != 0;
	l->live_code = S0_HIGH(f->s0);
	l->item = S0_LOW(f->s0);
	if (move == SET_MOVE_TWO) {
		l->move_period = f->period + 2;
		l->spare = SET_CHANNEL(f->state);
	} else if (move == SET_MOVE_NEXT) {
		l->move_period = f->period + 1;
		l->spare = SET_CHANNEL(f->state);
	} else if (move == SET_MOVE_NONE) {
		l->spare = SET_CHANNEL(f->state);
	}
}

void povodok_link_heard(struct povodok_link *l, const struct frame *f, int8_t rssi)
{
	switch (f->kind) {
	case FRAME_SYNC:
		sync_heard(l, f);
		break;
	case FRAME_LIVE:
		if (f->number <= STRELKA_BELKAS_MAX && l->belka[f->number].taken) {
			live_into(l, &l->belka[f->number], f, true, 0, rssi);
			l->rssi_sum += rssi;
			l->rssi_n++;
		}
		break;
	case FRAME_MISS:
		if (!l->first) {
			memcpy(l->missed, f->body, MISS_BODY_BYTES);
			l->miss_heard = true;
		}
		break;
	case FRAME_PASS: {
		size_t at = 0;
		struct pass_item item;

		while (frame_pass_next(f, &at, &item)) {
			struct povodok_belka *b = &l->belka[item.belka];

			if (!b->taken) {
				continue;
			}
			if (b->heard || b->passed) {
				b->passed = true;        /* passed already: not again in a later window */
				continue;
			}
			struct frame live = {
				.kind = FRAME_LIVE, .sender = WHO_BELKA, .number = item.belka,
				.s0 = item.s0, .period = f->period, .state = item.state,
				.body = item.steps, .body_len = item.steps_len,
			};
			live_into(l, b, &live, false, f->number, REC_NO_SIGNAL);
		}
		break;
	}
	case FRAME_HERE:
		if (l->searching && f->body_len >= 8) {
			uint64_t chip = 0;

			for (int i = 7; i >= 0; i--) {
				chip = (chip << 8) | f->body[i];
			}
			for (unsigned i = 0; i < l->found_n; i++) {
				if (l->found[i].chip == chip) {
					l->found[i].rssi = rssi;  /* heard again: the signal fresh, the
								   * period the first hearing's */
					return;
				}
			}
			if (l->found_n < 32) {
				l->found[l->found_n].chip = chip;
				l->found[l->found_n].rssi = rssi;
				l->found[l->found_n].period = l->period;
				l->found_n++;
			}
		}
		break;
	default:
		break;
	}
}

/* the set's state as this Povodok's SYNC carries it: in an even period the spare and the margin,
 * in an odd one the measurement, during a move the move */
static uint16_t state_now(const struct povodok_link *l)
{
	uint8_t move = SET_MOVE_NONE;

	if (l->move_period) {
		if (l->move_period == l->period + 2) {
			move = SET_MOVE_TWO;
		} else if (l->move_period == l->period + 1) {
			move = SET_MOVE_NEXT;
		}
	}
	if (move != SET_MOVE_NONE) {
		return SET_STATE(l->record, l->margin, move, l->spare);
	}
	if (l->period % 2 == 1 && l->measured_noise != LINK_NOISE_UNKNOWN) {
		int steps = (l->measured_noise - LINK_NOISE_FLOOR) / 2;

		steps = steps < 0 ? 0 : steps > 31 ? 31 : steps;
		return SET_STATE(l->record, steps, SET_MEASUREMENT, l->measured_channel);
	}
	return SET_STATE(l->record, l->margin, SET_MOVE_NONE, l->spare);
}

static bool belkas_send_this_period(const struct povodok_link *l)
{
	return l->live_code != S0_LIVE_OFF || l->period % LINK_STATE_EVERY == 0;
}

void povodok_link_period_begin(struct povodok_link *l, struct povodok_sync_out *out)
{
	bool fresh = l->content_fresh;   /* the last period's content: this period's or no one's */

	l->content_fresh = false;
	memset(out, 0, sizeof(*out));
	if (l->searching) {
		out->send = true;
		out->call = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = S0(S0_CALL, l->attempt);
		return;
	}
	if (l->first) {
		/* the items' schedule, counted in the periods the Belkas send in */
		if (belkas_send_this_period(l)) {
			if (l->items_row) {
				l->item = l->items_row;
				l->items_row = l->items_row == ITEM_OUT ? 0 : l->items_row + 1;
			} else if (l->items_countdown == 0) {
				l->item = ITEM_FIRST;
				l->items_row = ITEM_FIRST + 1;
				l->items_countdown = LINK_ITEMS_EVERY;
			} else {
				l->item = ITEM_CHANNEL;
			}
		}
		if (l->items_countdown) {
			l->items_countdown--;
		}
		out->send = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = S0(l->live_code, l->item);
		l->sync_state = out->state;
		l->sync_s0 = out->s0;
		l->povodok_state[l->cfg.number] = out->state;
	} else if (fresh) {
		/* the first's content of the last period, in this Povodok's own slot and period */
		out->send = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = l->content_s0;
		l->sync_state = out->state;
		l->sync_s0 = out->s0;
		l->povodok_state[l->cfg.number] = out->state;
	}
}

bool povodok_link_miss(struct povodok_link *l, uint8_t missed[MISS_BODY_BYTES])
{
	if (!l->first) {
		return false;
	}
	memset(l->missed, 0, sizeof(l->missed));
	for (unsigned n = 1; n <= STRELKA_BELKAS_MAX; n++) {
		if (l->belka[n].taken && !l->belka[n].heard) {
			miss_set(l->missed, n);
		}
	}
	memcpy(missed, l->missed, MISS_BODY_BYTES);
	return true;
}

unsigned povodok_link_pass(struct povodok_link *l, struct povodok_pass_item *items, unsigned max)
{
	unsigned n = 0;

	if (l->first || !l->miss_heard) {
		return 0;
	}
	for (unsigned k = 1; k <= STRELKA_BELKAS_MAX && n < max; k++) {
		struct povodok_belka *b = &l->belka[k];

		if (miss_has(l->missed, k) && b->heard && !b->passed) {
			items[n].belka = (uint8_t)k;
			items[n].s0 = b->s0;
			items[n].state = b->state;
			items[n].steps = b->steps;
			items[n].steps_len = b->steps_len;
			n++;
		}
	}
	return n;
}

/* the first's map: the spare the quietest channel anyone measured, neither the current nor the
 * search's; the move when the margin at any Povodok heard within a second has stood low for a
 * second */
static void decide_channels(struct povodok_link *l)
{
	int8_t best = INT8_MAX;
	uint8_t best_ch = l->spare;
	uint8_t worst_margin = l->margin;

	for (unsigned c = 0; c < STRELKA_SET_CHANNELS; c++) {
		int8_t loud = LINK_NOISE_UNKNOWN;

		if (c == l->cfg.channel) {
			continue;
		}
		for (unsigned k = 0; k < STRELKA_POVODOKS_MAX; k++) {
			if (l->noise_povodok[k][c] > loud) {
				loud = l->noise_povodok[k][c];
			}
		}
		if (l->noise_belka[c] > loud) {
			loud = l->noise_belka[c];
		}
		if (loud != LINK_NOISE_UNKNOWN && loud < best) {
			best = loud;
			best_ch = (uint8_t)c;
		}
	}
	if (!l->move_period) {
		l->spare = best_ch;
	}
	for (unsigned k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		if (k != l->cfg.number && l->povodok_known[k] &&
		    l->tick - l->povodok_heard_tick[k] < LINK_PERIODS_A_S &&
		    l->margin_povodok[k] < worst_margin) {
			worst_margin = l->margin_povodok[k];
		}
	}
	l->low_margin_periods = worst_margin < LINK_MOVE_MARGIN ? l->low_margin_periods + 1 : 0;
	if (!l->move_period && l->low_margin_periods >= LINK_MOVE_PERIODS) {
		l->move_period = l->period + 3;  /* said as 2 in the next SYNC, then as 1 */
		l->low_margin_periods = 0;
	}
}

void povodok_link_period_end(struct povodok_link *l, int8_t measured_noise,
			     struct povodok_period_out *out)
{
	memset(out, 0, sizeof(*out));
	/* the measurement of this period, for the next odd SYNC and the map */
	l->measured_channel = l->measure_next;
	l->measured_noise = measured_noise;
	if (measured_noise != LINK_NOISE_UNKNOWN) {
		l->noise_povodok[l->cfg.number][l->measured_channel] = measured_noise;
	}
	l->measure_next = (uint8_t)((l->measure_next + 1) % STRELKA_CHANNELS);
	/* the margin: the Belkas' signals over the noise on the current channel; none heard, the
	 * last stands */
	if (l->rssi_n) {
		int8_t noise = l->noise_povodok[l->cfg.number][l->cfg.channel];
		int mean = l->rssi_sum / (int)l->rssi_n;
		int steps = (mean - (noise == LINK_NOISE_UNKNOWN ? LINK_NOISE_FLOOR : noise)) / 2;

		l->margin = (uint8_t)(steps < 0 ? 0 : steps > 31 ? 31 : steps);
	}
	l->rssi_sum = 0;
	l->rssi_n = 0;

	/* the first: the lowest number heard within a second; none for a second, this one — at
	 * the start too, after a second's listening; not while searching, hearing no one */
	if (!l->searching) {
		uint8_t lowest = 0xFF;

		for (unsigned k = 0; k < l->cfg.number; k++) {
			if (l->povodok_known[k] &&
			    l->tick - l->povodok_heard_tick[k] < LINK_TAKEOVER_PERIODS) {
				lowest = (uint8_t)k;
				break;
			}
		}
		if (lowest == 0xFF) {
			if (!l->first && l->tick + 1 >= LINK_TAKEOVER_PERIODS) {
				l->first = true;
				l->first_number = l->cfg.number;
				l->try_channel = l->cfg.channel;
				out->took_over = true;
			}
		} else if (l->first) {
			l->first = false;
			out->yielded = true;
		}
	}
	l->no_sync_periods = l->content_taken ? 0 : l->no_sync_periods + 1;

	/* the move, every unit in the same period; then the first's map, a new spare named */
	if (l->move_period && l->period + 1 >= l->move_period) {
		l->cfg.channel = l->spare;
		l->move_period = 0;
		l->low_margin_periods = 0;
		out->moved = true;
	}
	if (l->first) {
		decide_channels(l);
	}
	if (l->searching && l->period + 1 - l->round_start >= LINK_SEARCH_ROUND) {
		l->attempt = (uint8_t)((l->attempt + 1) & 0x0F);  /* the next period a new round */
		l->round_start = l->period + 1;
	}
	/* where to listen next: the first on the set's channel; another, hearing no SYNC from a
	 * lower number, the spare every other period from the second miss, until it takes over */
	if (l->first || l->cfg.number == 0 || l->content_taken ||
	    l->no_sync_periods < LINK_ALONE_PERIODS || l->no_sync_periods % 2 == 1) {
		l->try_channel = l->cfg.channel;
	} else {
		l->try_channel = l->spare;
	}
	out->listen_channel = l->searching ? STRELKA_SEARCH_CHANNEL : l->try_channel;
	out->measure_channel = l->measure_next;

	/* this period's hearing closed */
	for (unsigned n = 1; n <= STRELKA_BELKAS_MAX; n++) {
		l->belka[n].heard = false;
		l->belka[n].passed = false;
	}
	l->miss_heard = false;
	l->content_taken = false;
	l->time_taken = false;
	l->tick++;
	l->period++;
}

uint8_t povodok_link_povodoks(const struct povodok_link *l)
{
	uint8_t bits = (uint8_t)(1u << l->cfg.number);

	for (unsigned k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		if (l->povodok_known[k] && l->tick - l->povodok_heard_tick[k] < LINK_PERIODS_A_S) {
			bits |= (uint8_t)(1u << k);
		}
	}
	return bits;
}

unsigned povodok_link_gathered(const struct povodok_link *l, uint8_t numbers[STRELKA_BELKAS_MAX])
{
	unsigned n = 0;

	for (unsigned k = 1; k <= STRELKA_BELKAS_MAX; k++) {
		if (l->belka[k].taken && (l->belka[k].heard || l->belka[k].passed)) {
			numbers[n++] = (uint8_t)k;
		}
	}
	return n;
}
