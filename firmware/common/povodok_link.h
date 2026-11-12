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
	bool unknown;           /* heard on the set's address, not taken: to be asked by SMP, and
				 * taken once its answer passes the MIC under the set's key */
};

struct povodok_sync_out {   /* the SYNC this period, or the CALL */
	bool send;
	bool call;
	uint32_t period;
	uint16_t state;
	uint8_t s0;
	uint8_t flight;         /* the who's bits 5–2, frame_sync's: the first's alone */
	bool repeat;            /* not the first: frame_sync_repeat's, with `root` */
	uint8_t root;           /* the first this Povodok's time comes from; its own, the first */
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
	bool esb;               /* the next period ESB's whole: no SYNC, no slot listened */
};

struct povodok_link {
	struct povodok_link_cfg cfg;
	bool first;             /* the first now: no lower number heard — for a second at the start, for
				 * five seconds once one was heard (link_rules.h) */
	bool net;               /* the set gathered by the network: no MISS, so no PASS, and every
				 * Povodok sends the application what it heard itself (PRINCIPLE.md,
				 * The three ways of gathering) */
	uint8_t first_number;   /* the lowest number's SYNC heard this period, else own */
	uint32_t period;        /* the set's time: own count, the first's when heard */
	uint32_t tick;          /* periods since the start, this Povodok's own */
	/* the set's state, the first's own or as heard from it */
	bool record;
	bool awake;             /* a frame waits in this Povodok for a Belka */
	/* the set given over to ESB for SMP: bit 14 in the SYNCs for LINK_ESB_ANNOUNCE of the
	 * state's periods, then no SYNC until 10 s with no frame and no packet */
	bool esb_seen;          /* this period: a SYNC with bit 14 heard */
	bool announcing;
	uint8_t announced;
	bool esb;
	uint32_t esb_until;     /* the tick ESB ends at, frames and packets putting it off */
	uint8_t esb_channel;    /* this Povodok's on ESB, fixed as ESB is first said */
	bool esb_home;          /* back on the set's channel from its own, silent until the first's
				 * SYNC, LINK_ESB_HOME_PERIODS at most */
	bool downloads;         /* joined to the phone: Belkas taken on ESB (the SYNC's bit 13) */
	uint8_t live_code;
	uint8_t aside[UNITS_BYTES];  /* the Belkas not among the session's units: heard in the
				      * state's periods alone, as at rest */
	uint8_t item;           /* the item asked this period */
	uint32_t items_turn;    /* item 8 every other turn, items 1–7 in turn between */
	uint8_t spare;
	uint32_t move_period;   /* the period the set moves at; 0 none */
	uint8_t move_asked;     /* a channel the application asked to move to, 64/24; 0xFF none */
	/* this period's SYNC as this Povodok sent it: the set's state and S0, for the live view */
	uint16_t sync_state;
	uint8_t sync_s0;
	/* the other Povodoks */
	uint32_t povodok_heard_tick[STRELKA_POVODOKS_MAX];
	bool povodok_known[STRELKA_POVODOKS_MAX];
	uint16_t povodok_state[STRELKA_POVODOKS_MAX];   /* each one's last SYNC's state */
	bool content_taken;     /* this period: a lower number's SYNC repeated in the next */
	uint8_t taken_root;     /* that SYNC's first: one of a lower first heard after it in the
				 * period is taken in its place */
	uint8_t root;           /* the first this Povodok's time comes from, as last taken */
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
	/* the SYNC's flight (PRINCIPLE.md, The clock loop): on the first, each other Povodok's SYNC
	 * late by twice the flight less what it takes off, the mean in 1/65536 tick, and the flight
	 * sent; on the others, the flight to take off and the first it came from */
	int32_t flight_twice[STRELKA_POVODOKS_MAX];
	uint16_t flight_n[STRELKA_POVODOKS_MAX];
	uint8_t flight_sent[STRELKA_POVODOKS_MAX];
	uint8_t flight_own;
	uint8_t flight_first;
};

void povodok_link_init(struct povodok_link *l, const struct povodok_link_cfg *cfg);
/* the application's word */
void povodok_link_record(struct povodok_link *l, bool run);
void povodok_link_live(struct povodok_link *l, uint8_t code);
/* the session's units, 64/1's; NULL every unit */
void povodok_link_units(struct povodok_link *l, const uint8_t *units);
/* a frame waiting for a Belka, said every period while one does: the set given over to ESB */
void povodok_link_awake(struct povodok_link *l, bool on);
/* a packet on ESB taken for this Povodok: ESB 10 s on */
void povodok_link_traffic(struct povodok_link *l);
/* joined to the phone or not: whether this Povodok takes Belkas on ESB, on a channel of its own */
void povodok_link_downloads(struct povodok_link *l, bool on);
/* the Povodok a Belka takes on ESB, from her item 8 (PRINCIPLE.md, The inside link); 0xFF unknown */
uint8_t povodok_link_esb_povodok(const struct povodok_link *l, uint8_t belka);
void povodok_link_search(struct povodok_link *l, bool on);
void povodok_link_take(struct povodok_link *l, uint8_t number, uint64_t chip);
/* the numbers heard on the set's address and not taken, each once: Povodok asks each for 64/0 over
 * SMP and takes her when the answer passes the MIC (COMMANDS.md, 64/13) */
unsigned povodok_link_unknown(struct povodok_link *l, uint8_t numbers[STRELKA_BELKAS_MAX]);
/* the set moved to [channel] at the application's word, 64/24, as the first moves it on its own:
 * said in two SYNCs, every unit moving in the same period; the first alone, 0–38 but its own,
 * no move under way; -1 refused */
int povodok_link_move(struct povodok_link *l, uint8_t channel);
void povodok_link_drop(struct povodok_link *l, uint8_t number);
/* the gathering: false by the set's radio, the MISS and the PASSes; true by the network, every
 * Povodok on one 5 GHz network with the application (64/1, `net`) */
void povodok_link_gather(struct povodok_link *l, bool net);
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
/* the SYNC's flight (PRINCIPLE.md, The clock loop). On the first, the link stamps the other
 * Povodoks' SYNCs in their slots, TIMER22 through DPPI as for its own grid, and gives each here:
 * the ticks its END came after the END expected on this Povodok's grid, signed. The first sends
 * the mean in its SYNC's who, povodok_sync_out.flight, once a second's stamps are in, the last
 * mean until a new one */
#define LINK_FLIGHT_FIRST     50      /* a second of SYNCs before the first flight is sent */
#define LINK_FLIGHT_AVERAGE   256     /* the mean of the first 256, then 1/256 a stamp */
#define LINK_FLIGHT_LATE_MAX  48      /* 3 µs: a stamp further off left out, a Povodok not yet
				       * on the first's time */
void povodok_link_stamp(struct povodok_link *l, uint8_t povodok, int32_t late_ticks);
/* on a Povodok not the first: the ticks the link takes off the first's SYNC's END before
 * clock_sync(), so that this one's own SYNC goes out as from the first's place; 0 on the first,
 * and until a first has sent one */
uint8_t povodok_link_flight(const struct povodok_link *l);
/* the Povodoks heard within a second, a bit each, this one's own among them (PHONE.md) */
uint8_t povodok_link_povodoks(const struct povodok_link *l);
/* the gathering: the Belkas with a LIVE this period, for the phone (PHONE.md) */
unsigned povodok_link_gathered(const struct povodok_link *l, uint8_t numbers[STRELKA_BELKAS_MAX]);

#endif
