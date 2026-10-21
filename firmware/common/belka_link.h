/* NIC-Strelka — a Belka's side of the inside link, the logic alone: what the SYNCs say and what
 * the Belka answers, period by period (PRINCIPLE.md, The inside link; FIRMWARE.md, A session).
 * The radio, the timers and the stamps are the unit's; this takes the frames already read and
 * checked, says before the Belka's slot what its LIVE carries, and at each period's end what to
 * do in the next: where to listen, what to measure, and whether the record starts, pauses, goes
 * on or stops. Plain C, run on the host against a simulated set (tests/test_world.c). */
#ifndef STRELKA_BELKA_LINK_H
#define STRELKA_BELKA_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "frame.h"
#include "link_rules.h"

struct belka_link_cfg {
	uint8_t number;         /* 1–63; 0 a new Belka, on the search's channel and address */
	uint64_t chip;          /* for the search's draw */
	uint32_t address;       /* the set's */
	uint8_t channel;        /* the set's, 0–38 */
	uint16_t stop_s;        /* seconds without a SYNC that pause the record; 0 the default */
};

struct belka_facts {        /* what the unit knows of itself, for the state's items */
	uint8_t cell_pct;
	int8_t temp_c;
	uint16_t cell_mv;
	uint8_t card_free_pct;
	uint8_t dropped;
	int16_t offset_ns;
	uint8_t unit_state;     /* BELKA_CALIBRATING, GIVING, TAKING_FIRMWARE, ERASING win over
				 * the link's own; else 0 */
	uint8_t fault;
	uint8_t acc_out, gyro_out;
};

struct belka_live_out {     /* the LIVE of this period's slot */
	bool send;
	uint8_t s0;             /* the live view's code, the item */
	uint16_t state;         /* the item's 2 B */
};

struct belka_period_out {   /* the next period, and what this one was */
	uint8_t listen_channel;
	uint8_t measure_channel;
	bool record_start;      /* from first_period */
	uint32_t first_period;
	bool record_stop;
	uint8_t stop_why;
	bool pause, resume;
	uint8_t state;          /* enum belka_state, the Belka's own */
	/* this period, for the record's information block */
	uint8_t taken_povodok;  /* whose SYNC was taken, REC_NO_POVODOK none */
	uint8_t syncs_heard;
	int8_t signal_dbm;      /* REC_NO_SIGNAL none */
};

struct belka_link {
	struct belka_link_cfg cfg;
	uint32_t period;        /* the set's period, as last heard, counted on since */
	bool period_known;
	/* this period */
	uint8_t syncs_heard, taken_povodok;
	int8_t taken_signal;
	bool run_seen;          /* a SYNC said the record runs */
	int16_t here_window;
	/* the last second */
	int8_t signal_ring[LINK_PERIODS_A_S];
	bool signal_ring_heard[LINK_PERIODS_A_S];
	uint8_t ring_at;
	uint32_t no_sync_periods;
	/* the set's state as heard */
	uint8_t live_code, item_asked;
	uint8_t spare;
	bool spare_known;
	uint32_t move_period;   /* the period the set moves at */
	bool move_pending;
	/* the record */
	bool recording, paused;
	uint8_t stop_count;     /* SYNCs heard saying it does not run, since one saying it does */
	/* the state's items */
	bool trouble;
	uint8_t sent_state, sent_fault;
	/* the channels */
	uint8_t channel;
	uint8_t try_channel;    /* the channel listened on this period */
	uint8_t sweep;          /* lost: the last of the set's channels tried */
	uint8_t measure_channel;
	int8_t measured_noise;
	uint8_t state;
};

void belka_link_init(struct belka_link *l, const struct belka_link_cfg *cfg);
/* the taking: the Belka's number, the set's address and channel, by SMP (COMMANDS.md) */
void belka_link_join(struct belka_link *l, uint8_t number, uint32_t address, uint8_t channel);
/* a frame heard in this period, read and its signature checked by the caller */
void belka_link_heard(struct belka_link *l, const struct frame *f, int8_t rssi);
/* after the SYNCs' slots, before the Belka's own: its LIVE */
void belka_link_slot(struct belka_link *l, const struct belka_facts *facts,
		     struct belka_live_out *live);
/* a new Belka, after the CALL's slot: the window of its HERE in this period, or -1 */
int16_t belka_link_here(const struct belka_link *l);
/* the noise measured on the channel asked for this period */
void belka_link_measured(struct belka_link *l, int8_t noise_dbm);
/* the period's end: what the Belka does next */
void belka_link_period_end(struct belka_link *l, const struct belka_facts *facts,
			   struct belka_period_out *out);
uint32_t belka_link_period(const struct belka_link *l);

#endif
