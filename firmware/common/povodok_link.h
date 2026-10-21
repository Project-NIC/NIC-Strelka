/* NIC-Strelka — a Povodok's side of the inside link, the logic alone (PRINCIPLE.md, The inside
 * link): the first Povodok's SYNC, its MISS after the slots and the gathering of every Belka's
 * LIVE from its own hearing and the others' PASSes; the other Povodoks' repeats of its SYNC and
 * their PASSes; the map of the channels and the move; the takeover when the first falls silent;
 * the search for new Belkas. The radio and the timers are the unit's; this takes frames already
 * read and checked and says what to send. Plain C, run on the host against a simulated set
 * (tests/test_world.c). */
#ifndef STRELKA_POVODOK_LINK_H
#define STRELKA_POVODOK_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "frame.h"
#include "link_rules.h"

struct povodok_link_cfg {
	uint8_t number;         /* 0–3; the lowest present is the first */
	uint32_t address;
	uint8_t channel;        /* the set's, 0–38 */
};

struct povodok_belka {      /* what a Povodok knows of a Belka */
	bool taken;
	uint64_t chip;
	uint32_t last_period;   /* its last LIVE, heard or passed */
	uint8_t by;             /* 0 heard by this Povodok, 1–3 passed by that Povodok (COMMANDS.md) */
	uint8_t s0;
	uint16_t state;
	uint8_t steps[LIVE_BODY_MAX];
	uint8_t steps_len;
	int8_t rssi;            /* as this Povodok heard it last, REC_NO_SIGNAL none */
	bool heard, passed;     /* this period */
	uint16_t item[ITEM_LAST + 1];
	bool item_known[ITEM_LAST + 1];
};

struct povodok_sync_out {   /* the SYNC this period, or the CALL */
	bool send;
	bool call;
	uint32_t period;
	uint16_t state;
	uint8_t s0;
};

struct povodok_pass_item {
	uint8_t belka, s0;
	uint16_t state;
	const uint8_t *steps;
	uint8_t steps_len;
};

struct povodok_period_out {
	uint8_t listen_channel; /* the set's, or the search's while searching */
	uint8_t measure_channel;
	bool moved;             /* the set moved to a new channel at this period's end */
	bool took_over, yielded;
};

struct povodok_link {
	struct povodok_link_cfg cfg;
	bool first;             /* the first now: the lowest number heard within a second */
	uint8_t first_number;   /* the lowest number's SYNC heard this period, else own */
	uint32_t period;        /* the set's time: own count, the first's when heard */
	uint32_t tick;          /* periods since the start, this Povodok's own */
	/* the set's state, the first's own or as heard from it */
	bool record;
	uint8_t live_code;
	uint8_t item;           /* the item asked this period */
	uint32_t items_countdown;
	uint8_t items_row;      /* 0 none; 1–7 the item of the row */
	uint8_t spare;
	uint32_t move_period;   /* the period the set moves at; 0 none */
	/* this period's SYNC as this Povodok sent it: the set's state and S0, for the live view */
	uint16_t sync_state;
	uint8_t sync_s0;
	/* the other Povodoks */
	uint32_t povodok_heard_tick[STRELKA_POVODOKS_MAX];
	bool povodok_known[STRELKA_POVODOKS_MAX];
	uint16_t povodok_state[STRELKA_POVODOKS_MAX];   /* each one's last SYNC's state */
	bool content_taken;     /* this period: a lower number's SYNC repeated in the next */
	bool time_taken;        /* this period: the set's time from a higher number's SYNC */
	bool content_fresh;
	uint8_t content_s0;
	uint32_t no_sync_periods;   /* without a lower number's SYNC */
	/* the Belkas */
	struct povodok_belka belka[STRELKA_BELKAS_MAX + 1];
	uint8_t missed[MISS_BODY_BYTES];   /* the first's MISS this period; or the one heard */
	bool miss_heard;
	/* the channels: the noise each measurer saw last on each channel */
	int8_t noise_povodok[STRELKA_POVODOKS_MAX][STRELKA_CHANNELS];
	int8_t noise_belka[STRELKA_CHANNELS];
	uint8_t measure_next;   /* this Povodok's turn through the 40 */
	uint8_t measured_channel;
	int8_t measured_noise;  /* for the next odd period's SYNC */
	int16_t rssi_sum;       /* the Belkas heard this period */
	uint16_t rssi_n;
	uint8_t margin;         /* own, 0–31 by 2 dB */
	uint8_t margin_povodok[STRELKA_POVODOKS_MAX];
	uint32_t low_margin_periods;
	uint8_t try_channel;    /* the channel listened on this period */
	/* the search */
	bool searching;
	uint8_t attempt;
	uint32_t round_start;
	struct { uint64_t chip; int8_t rssi; uint32_t period; } found[32];
	uint8_t found_n;
};

void povodok_link_init(struct povodok_link *l, const struct povodok_link_cfg *cfg);
/* the application's word */
void povodok_link_record(struct povodok_link *l, bool run);
void povodok_link_live(struct povodok_link *l, uint8_t code);
void povodok_link_search(struct povodok_link *l, bool on);
void povodok_link_take(struct povodok_link *l, uint8_t number, uint64_t chip);
void povodok_link_drop(struct povodok_link *l, uint8_t number);
/* a frame heard this period, read and checked by the caller: a SYNC, a LIVE, a MISS, a PASS, a
 * HERE */
void povodok_link_heard(struct povodok_link *l, const struct frame *f, int8_t rssi);
/* the period's start: the SYNC to send in this Povodok's slot (a CALL while searching) */
void povodok_link_period_begin(struct povodok_link *l, struct povodok_sync_out *out);
/* after the slots: the first's MISS (true, the bits in `missed`), or the items this Povodok
 * passes in its window, those the MISS names, heard this period and not passed before it */
bool povodok_link_miss(struct povodok_link *l, uint8_t missed[MISS_BODY_BYTES]);
unsigned povodok_link_pass(struct povodok_link *l, struct povodok_pass_item *items, unsigned max);
/* the period's end, with the noise measured on the channel asked */
void povodok_link_period_end(struct povodok_link *l, int8_t measured_noise,
			     struct povodok_period_out *out);
/* the Povodoks heard within a second, a bit each, this one's own among them (PHONE.md) */
uint8_t povodok_link_povodoks(const struct povodok_link *l);
/* the gathering: the Belkas with a LIVE this period, for the phone (PHONE.md) */
unsigned povodok_link_gathered(const struct povodok_link *l, uint8_t numbers[STRELKA_BELKAS_MAX]);

#endif
