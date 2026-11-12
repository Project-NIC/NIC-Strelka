#include <string.h>
#include "povodok_link.h"

void povodok_link_init(struct povodok_link *l, const struct povodok_link_cfg *cfg)
{
	memset(l, 0, sizeof(*l));
	l->cfg = *cfg;
	l->first_number = cfg->number;
	l->spare = (uint8_t)((cfg->channel + 1) % STRELKA_SET_CHANNELS);
	l->try_channel = cfg->channel;
	l->measure_next = (uint8_t)((10u * cfg->number) % LINK_MEASURED_CHANNELS);
	l->measured_noise = LINK_NOISE_UNKNOWN;
	l->flight_first = 0xFF;
	l->items_turn = 0;
	l->move_asked = 0xFF;
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

void povodok_link_units(struct povodok_link *l, const uint8_t *units)
{
	for (unsigned i = 0; i < UNITS_BYTES; i++) {
		l->aside[i] = units ? (uint8_t)~units[i] : 0;
	}
}

void povodok_link_awake(struct povodok_link *l, bool on)
{
	l->awake = on;
	if (on) {
		l->esb_until = l->tick + LINK_SPOKEN_PERIODS;
	}
}

void povodok_link_traffic(struct povodok_link *l)
{
	l->esb_until = l->tick + LINK_SPOKEN_PERIODS;
}

void povodok_link_downloads(struct povodok_link *l, bool on)
{
	l->downloads = on;
}

uint8_t povodok_link_esb_povodok(const struct povodok_link *l, uint8_t belka)
{
	const struct povodok_belka *b = &l->belka[belka];

	return b->item_known[ITEM_CHANNEL] ? ITEM_ESB_POVODOK(b->item[ITEM_CHANNEL]) : 0xFF;
}

/* the channel this Povodok takes Belkas on over ESB: the first, the set's own; another joined to
 * the phone, the quietest it measured of its quarter of the set's channels — its number, then
 * every fourth — never the set's, so that no two meet; one not joined, the set's, silent there */
static uint8_t esb_channel_of(const struct povodok_link *l)
{
	uint8_t best = l->cfg.channel;
	int quiet = INT8_MAX;

	if (l->first || !l->downloads) {
		return l->cfg.channel;
	}
	for (uint8_t c = l->cfg.number; c < STRELKA_SET_CHANNELS; c += STRELKA_POVODOKS_MAX) {
		int8_t noise = l->noise_povodok[l->cfg.number][c];
		int n = noise == LINK_NOISE_UNKNOWN ? 0 : noise;   /* unmeasured: last */

		if (c != l->cfg.channel && n < quiet) {
			quiet = n;
			best = c;
		}
	}
	return best;
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

void povodok_link_gather(struct povodok_link *l, bool net)
{
	l->net = net;
}

int povodok_link_move(struct povodok_link *l, uint8_t channel)
{
	if (!l->first || channel >= STRELKA_SET_CHANNELS || channel == l->cfg.channel ||
	    l->move_period) {
		return -1;
	}
	l->move_asked = channel;
	return 0;
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
		l->noise_belka[ITEM_MEASURED(f->state) % STRELKA_CHANNELS] = (int8_t)ITEM_LOW(f->state);
	}
}

static uint8_t first_now(const struct povodok_link *l);

static void sync_heard(struct povodok_link *l, const struct frame *f)
{
	uint8_t k = f->number;
	uint8_t move = SET_MOVE(f->state);

	l->povodok_known[k] = true;
	l->povodok_heard_tick[k] = l->tick;
	l->povodok_state[k] = f->state;
	if (l->esb_home && k < l->cfg.number) {
		l->esb = l->esb_home = false;   /* the first's SYNCs back */
		l->announcing = false;
		l->announced = 0;
	}
	if (f->state & SET_ESB) {
		l->esb_seen = true;              /* a frame waiting somewhere: ESB announced */
	}
	if (move != SET_MEASUREMENT) {
		l->margin_povodok[k] = (uint8_t)SET_MARGIN(f->state);
	} else {
		l->noise_povodok[k][SET_CHANNEL(f->state)] = (int8_t)SET_NOISE_DBM(f->state);
	}
	if (l->content_taken && (k >= l->cfg.number || f->root >= l->taken_root)) {
		return;                          /* taken already, and this one's first no lower */
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
		l->taken_root = f->root;
		l->root = f->root;
		l->first_number = k;
		l->content_fresh = true;
		/* the flight, said for the Povodok numbered period % 4, taken from the first alone —
		 * the lowest number heard within the takeover's time, not a second one heard in a
		 * period the first's SYNC was missed */
		if (f->period % STRELKA_POVODOKS_MAX == l->cfg.number && k == first_now(l)) {
			l->flight_own = f->flight;
			l->flight_first = k;
		}
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
	} else if (move == SET_MOVE_NONE && !(f->state & SET_ESB)) {
		l->spare = SET_CHANNEL(f->state);   /* with bit 14, the sender's channel on ESB */
	}
}

void povodok_link_heard(struct povodok_link *l, const struct frame *f, int8_t rssi)
{
	switch (f->kind) {
	case FRAME_SYNC:
		/* past its listening second and following a lower number: a SYNC with a period
		 * outside the window of its own count is a replay or a stranger's */
		if (l->tick > LINK_PERIODS_A_S && l->no_sync_periods < LINK_LOST_PERIODS &&
		    !(f->period + LINK_PERIOD_WINDOW >= l->period &&
		      f->period <= l->period + LINK_PERIOD_WINDOW)) {
			break;
		}
		sync_heard(l, f);
		break;
	case FRAME_LIVE:
		if (f->number >= 1 && f->number <= STRELKA_BELKAS_MAX) {
			/* a LIVE on the set's address from a number not taken: no LIVE is signed, so
			 * it is not let in on that alone, but asked over SMP, and taken when its
			 * answer passes the MIC under the set's key (povodok_link_unknown) */
			if (!l->belka[f->number].taken) {
				l->belka[f->number].unknown = true;
				break;
			}
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

			if (item.belka < 1 || item.belka > STRELKA_BELKAS_MAX) {
				continue;
			}
			if (!b->taken) {
				b->unknown = true;       /* heard by another: to be asked, as a LIVE */
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
static bool at_rest(const struct povodok_link *l);

static uint16_t state_now(const struct povodok_link *l)
{
	uint16_t awake = (l->awake || l->announcing ? SET_ESB : 0u) |
			 (l->downloads ? SET_DOWNLOADS : 0u);
	uint8_t move = SET_MOVE_NONE;

	if (l->move_period) {
		if (l->move_period == l->period + 2) {
			move = SET_MOVE_TWO;
		} else if (l->move_period == l->period + 1) {
			move = SET_MOVE_NEXT;
		}
	}
	if (move != SET_MOVE_NONE) {
		return (uint16_t)(awake | SET_STATE(l->record, l->margin, move, l->spare));
	}
	if (l->awake || l->announcing) {
		/* ESB said: the channel's field this Povodok's channel there, for its Belkas */
		return (uint16_t)(awake | SET_STATE(l->record, l->margin, SET_MOVE_NONE,
						    l->announcing ? l->esb_channel : esb_channel_of(l)));
	}
	if ((at_rest(l) ? l->period / LINK_STATE_EVERY : l->period) % 2 == 1 &&
	    l->measured_noise != LINK_NOISE_UNKNOWN) {
		int steps = (l->measured_noise - LINK_NOISE_FLOOR) / 2;

		steps = steps < 0 ? 0 : steps > 31 ? 31 : steps;
		return (uint16_t)(awake | SET_STATE(l->record, steps, SET_MEASUREMENT, l->measured_channel));
	}
	return (uint16_t)(awake | SET_STATE(l->record, l->margin, SET_MOVE_NONE, l->spare));
}

/* on ESB the first leaves it for two of the state's periods a minute and sends its SYNC there,
 * so that a unit back on the SYNCs hears the set (link_rules.h) */
static bool esb_sync_period(const struct povodok_link *l, uint32_t period)
{
	return l->first && period % LINK_STATE_EVERY == 0 &&
	       period % LINK_ESB_SYNC_PERIODS < LINK_ESB_SYNCS * LINK_STATE_EVERY;
}

static bool belkas_send_this_period(const struct povodok_link *l)
{
	return l->live_code != S0_LIVE_OFF || l->period % LINK_STATE_EVERY == 0;
}

/* the set at rest — nothing recorded, the live view off, no search: its SYNCs in the state's period
 * alone, once a second, and in the periods of a move (PRINCIPLE.md, The inside link) */
static bool at_rest(const struct povodok_link *l)
{
	return !l->record && l->live_code == S0_LIVE_OFF && !l->searching;
}

static uint32_t takeover_periods(const struct povodok_link *l)
{
	return at_rest(l) ? LINK_TAKEOVER_REST_PERIODS : LINK_TAKEOVER_PERIODS;
}

/* a lower number heard within the takeover's time */
static bool first_heard(const struct povodok_link *l)
{
	return first_now(l) != 0xFF;
}

/* the first as this Povodok hears it: the lowest number heard within the takeover's time, 0xFF
 * none */
static uint8_t first_now(const struct povodok_link *l)
{
	for (unsigned k = 0; k < l->cfg.number; k++) {
		if (l->povodok_known[k] && l->tick - l->povodok_heard_tick[k] < takeover_periods(l)) {
			return (uint8_t)k;
		}
	}
	return 0xFF;
}

static bool sync_this_period(const struct povodok_link *l)
{
	return !at_rest(l) || l->period % LINK_STATE_EVERY == 0 || l->move_period;
}

/* the flight the first says for Povodok k this period: the mean once a second's stamps are in,
 * half the twice, rounded, within the who's four bits; else the last said */
static uint8_t flight_out(struct povodok_link *l, uint8_t k)
{
	int32_t ticks;

	if (k == l->cfg.number) {
		return 0;
	}
	if (l->flight_n[k] >= LINK_FLIGHT_FIRST) {
		ticks = l->flight_twice[k] < 0 ? 0 : (l->flight_twice[k] + 65536) / 131072;
		if (ticks > (int32_t)FLIGHT_TICKS_MAX) {
			ticks = (int32_t)FLIGHT_TICKS_MAX;
		}
		l->flight_sent[k] = (uint8_t)ticks;
	}
	return l->flight_sent[k];
}

void povodok_link_stamp(struct povodok_link *l, uint8_t k, int32_t late_ticks)
{
	int32_t twice;

	if (!l->first || k >= STRELKA_POVODOKS_MAX || k == l->cfg.number ||
	    late_ticks > LINK_FLIGHT_LATE_MAX || late_ticks < -LINK_FLIGHT_LATE_MAX) {
		return;
	}
	/* late by twice the flight, less what k takes off already, which this one sent it; in
	 * 1/65536 tick, so that a stamp a tick off still moves the mean of 256 */
	twice = (late_ticks + l->flight_sent[k]) * 65536;
	if (l->flight_n[k] < LINK_FLIGHT_AVERAGE) {
		l->flight_n[k]++;
	}
	l->flight_twice[k] += (twice - l->flight_twice[k]) / (int32_t)l->flight_n[k];
}

uint8_t povodok_link_flight(const struct povodok_link *l)
{
	/* the first's flight while the one that said it is the first still */
	return l->first || l->flight_first != first_now(l) ? 0 : l->flight_own;
}

void povodok_link_period_begin(struct povodok_link *l, struct povodok_sync_out *out)
{
	bool fresh = l->content_fresh;   /* the last period's content: this period's or no one's */

	l->content_fresh = false;
	memset(out, 0, sizeof(*out));
	if (l->esb && !esb_sync_period(l, l->period)) {
		return;                          /* the set on ESB: no SYNC but the minute's */
	}
	if (l->searching && l->period % LINK_STATE_EVERY != 0) {
		out->send = true;
		out->call = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = S0(S0_CALL, l->attempt);
		return;
	}
	if (l->first) {
		/* the items' schedule, counted in the periods the Belkas send in: item 8, the
		 * channels', every other one, items 1–7 in turn between (PRINCIPLE.md) */
		if (belkas_send_this_period(l)) {
			l->item = l->items_turn % 2 == 0
				? ITEM_CHANNEL
				: (uint8_t)(ITEM_FIRST + (l->items_turn / 2) % (ITEM_OUT - ITEM_FIRST + 1));
			l->items_turn++;
		}
		if (!sync_this_period(l)) {
			return;                  /* at rest: the state's period alone */
		}
		out->send = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = S0(l->live_code, l->item);
		out->flight = flight_out(l, (uint8_t)(l->period % STRELKA_POVODOKS_MAX));
		out->root = l->cfg.number;
		l->sync_state = out->state;
		l->sync_s0 = out->s0;
		l->povodok_state[l->cfg.number] = out->state;
	} else if (fresh || (at_rest(l) && l->period % LINK_STATE_EVERY == 0 && first_heard(l))) {
		/* the first's content of the last period, in this Povodok's own slot and period; at
		 * rest, in the state's period, its last heard */
		out->send = true;
		out->period = l->period;
		out->state = state_now(l);
		out->s0 = l->content_s0;
		out->repeat = true;
		out->root = l->root;
		l->sync_state = out->state;
		l->sync_s0 = out->s0;
		l->povodok_state[l->cfg.number] = out->state;
	}
}

bool povodok_link_miss(struct povodok_link *l, uint8_t missed[MISS_BODY_BYTES])
{
	if (!l->first || l->net) {
		return false;            /* gathered by the network: no MISS, and so no PASS */
	}
	memset(l->missed, 0, sizeof(l->missed));
	for (unsigned n = 1; n <= STRELKA_BELKAS_MAX; n++) {
		bool aside = l->aside[n / 8] & (1u << (n % 8));

		if (l->belka[n].taken && !l->belka[n].heard &&
		    (!aside || l->period % LINK_STATE_EVERY == 0)) {
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

/* a move is worth it: the set's own channel measured, and the spare's loudest measurement under
 * its loudest by LINK_MOVE_QUIETER_DB — a margin low because the units are far finds the two
 * alike and moves nothing */
static bool spare_quieter(const struct povodok_link *l, int8_t spare_noise)
{
	int8_t own = l->noise_belka[l->cfg.channel];

	for (unsigned k = 0; k < STRELKA_POVODOKS_MAX; k++) {
		if (l->noise_povodok[k][l->cfg.channel] > own) {
			own = l->noise_povodok[k][l->cfg.channel];
		}
	}
	return own != LINK_NOISE_UNKNOWN && spare_noise != INT8_MAX &&
	       spare_noise + LINK_MOVE_QUIETER_DB <= own;
}

/* the first's map: the spare the quietest channel anyone measured, neither the current nor the
 * search's; the move when the margin at any Povodok heard within a second has stood low for a
 * second */
static void decide_channels(struct povodok_link *l)
{
	int8_t best = INT8_MAX;
	uint8_t best_ch = l->spare;
	uint8_t worst_margin = l->margin;

	if (best_ch == l->cfg.channel) {
		/* right after a move the spare is the channel the set is on: the next one until
		 * a measurement names a better */
		best_ch = (uint8_t)((l->cfg.channel + 1) % STRELKA_SET_CHANNELS);
	}

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
		    l->tick - l->povodok_heard_tick[k] < takeover_periods(l) &&
		    l->margin_povodok[k] < worst_margin) {
			worst_margin = l->margin_povodok[k];
		}
	}
	l->low_margin_periods = worst_margin < LINK_MOVE_MARGIN ? l->low_margin_periods + 1 : 0;
	if (!l->move_period && l->move_asked != 0xFF &&
	    (!at_rest(l) || (l->period + 1) % LINK_STATE_EVERY == 0)) {
		/* the application's channel, said and taken as a move of the first's own */
		l->spare = l->move_asked;
		l->move_asked = 0xFF;
		l->move_period = l->period + 3;
		l->low_margin_periods = 0;
	} else if (!l->move_period && l->low_margin_periods >= LINK_MOVE_PERIODS &&
		   spare_quieter(l, best) &&
		   (!at_rest(l) || (l->period + 1) % LINK_STATE_EVERY == 0)) {
		/* said as 2 in the next SYNC, then as 1 — at rest, the next the state's period's, so
		 * that the Belkas asleep hear it and wake for the move */
		l->move_period = l->period + 3;
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
	l->measure_next = (uint8_t)((l->measure_next + 1) % LINK_MEASURED_CHANNELS);
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

	/* the set given over to ESB: a frame waiting here or bit 14 heard, said in the SYNCs for
	 * two of the state's periods, so that every Belka asleep hears it, no move pending; then no
	 * SYNC until 10 s pass with no frame and no packet, and back as at the start, a second's
	 * listening before taking over */
	if (l->awake || l->esb_seen) {
		if (l->esb_seen) {
			l->esb_until = l->tick + LINK_SPOKEN_PERIODS;
		}
		if (!l->announcing) {
			l->esb_channel = esb_channel_of(l);
		}
		l->announcing = true;
	}
	l->esb_seen = false;
	if (l->announcing && !l->esb && !l->move_period && l->period % LINK_STATE_EVERY == 0 &&
	    ++l->announced >= LINK_ESB_ANNOUNCE) {
		l->esb = true;
	}
	if (l->announcing && (int32_t)(l->tick - l->esb_until) >= 0) {
		if (l->esb && !l->esb_home && l->esb_channel != l->cfg.channel) {
			/* its own channel quiet: back on the set's, silent while the first may still
			 * be downloading there, until its SYNC, 10 minutes at most */
			l->esb_home = true;
			l->esb_until = l->tick + LINK_ESB_HOME_PERIODS;
		} else {
			l->announcing = false;
			l->announced = 0;
			l->esb_home = false;
			if (l->esb) {
				l->esb = false;
				for (unsigned k = 0; k < l->cfg.number; k++) {
					l->povodok_heard_tick[k] = l->tick;
				}
			}
		}
	}
	out->esb = l->esb && !esb_sync_period(l, l->period + 1);

	/* the first: the lowest number heard within a second; none for a second, this one — at
	 * the start too, after a second's listening; one that has heard a lower number waits five
	 * seconds without any, a first hidden a moment not being a first gone; not while
	 * searching, hearing no one */
	if (!l->searching && !l->esb) {
		uint8_t lowest = 0xFF;

		for (unsigned k = 0; k < l->cfg.number; k++) {
			if (l->povodok_known[k] &&
			    l->tick - l->povodok_heard_tick[k] < LINK_TAKEOVER_LOST_PERIODS) {
				lowest = (uint8_t)k;
				break;
			}
		}
		if (lowest == 0xFF) {
			if (!l->first && l->tick + 1 >= takeover_periods(l)) {
				l->first = true;
				l->first_number = l->cfg.number;
				memset(l->flight_twice, 0, sizeof(l->flight_twice));
				memset(l->flight_n, 0, sizeof(l->flight_n));
				memset(l->flight_sent, 0, sizeof(l->flight_sent));
				l->try_channel = l->cfg.channel;
				out->took_over = true;
			}
		} else if (l->first) {
			l->first = false;
			out->yielded = true;
		}
	}
	/* a lower number's SYNC missed — at rest, in the state's period alone, the SYNCs silent
	 * between */
	if (l->content_taken || l->esb) {
		l->no_sync_periods = 0;
	} else if (sync_this_period(l)) {
		l->no_sync_periods++;
	}

	/* the move, every unit in the same period; then the first's map, a new spare named */
	if (l->move_period && l->period + 1 >= l->move_period) {
		l->cfg.channel = l->spare;
		l->move_period = 0;
		l->low_margin_periods = 0;
		out->moved = true;
	}
	if (l->first && !l->announcing) {
		decide_channels(l);              /* no move while ESB is said or on */
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
	/* searching: the search's channel, but the set's own in the state's period, its SYNC's */
	out->listen_channel = l->searching && (l->period + 1) % LINK_STATE_EVERY != 0
			      ? STRELKA_SEARCH_CHANNEL : l->try_channel;
	if (l->esb && !l->esb_home) {
		out->listen_channel = l->esb_channel;   /* on ESB, on its own channel */
	}
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
		if (l->povodok_known[k] && l->tick - l->povodok_heard_tick[k] < takeover_periods(l)) {
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

unsigned povodok_link_unknown(struct povodok_link *l, uint8_t numbers[STRELKA_BELKAS_MAX])
{
	unsigned n = 0;

	for (unsigned k = 1; k <= STRELKA_BELKAS_MAX; k++) {
		if (l->belka[k].unknown && !l->belka[k].taken) {
			numbers[n++] = (uint8_t)k;
		}
		l->belka[k].unknown = false;
	}
	return n;
}
