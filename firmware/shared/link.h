/* NIC-Strelka — the inside link on the radio: Strelka's frame and ESB taking turns within the
 * period (PRINCIPLE.md, The inside link; FIRMWARE.md, The inside link). No code yet: ESB on the
 * nRF54LM20A and its turns with a frame of one's own are the first thing tried on the development
 * kit (STATUS.md), and what the kit shows writes this file. The shape it takes is here, so that
 * the rest of the firmware is written against it. What the link says and answers, period by
 * period, is written and checked already, in common/belka_link.c and common/povodok_link.c: this
 * file is to carry their frames over the radio and feed them what it hears. */
#ifndef STRELKA_LINK_H
#define STRELKA_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "frame.h"

enum link_role {
	LINK_BELKA,
	LINK_POVODOK,
};

struct link_callbacks {
	/* a Belka: a SYNC heard, its END stamped (clock_last_sync_end), from Povodok k */
	void (*sync)(const struct frame *f, uint64_t end_local);
	/* the first Povodok: a LIVE heard in its slot, or passed by another Povodok */
	void (*live)(const struct frame *f, uint8_t by);
	/* a Povodok searching: a HERE in its window */
	void (*here)(uint64_t chip, int8_t rssi);
	/* an SMP frame reassembled from ESB, to the MCUmgr transport */
	void (*smp)(const uint8_t *frame, size_t len, uint8_t from);
};

int link_init(enum link_role role, const struct link_callbacks *cb);
/* the set: its address, key and channel, and the unit's own number */
int link_join(uint32_t address, const uint8_t key[16], uint8_t channel, uint8_t number);
/* a Belka: the LIVE for the next period, made ready before its slot */
void link_live(uint8_t live, uint8_t item, uint16_t state, const void *steps);
/* the first Povodok: the next SYNC's content; the others repeat it a period later */
void link_sync(uint8_t live, uint8_t item, uint16_t set_state);
/* an SMP frame to a Belka or to Povodok, in fragments over ESB, in the period's room */
int link_smp_send(uint8_t to, const uint8_t *frame, size_t len);
/* the search for new Belkas, Povodok's CALLs on 2480 MHz */
void link_search(bool on);
/* the channel a unit measures this period, and the noise read on it */
int8_t link_noise_dbm(uint8_t channel);
/* Povodok's main defines it; the link calls it after the PASSes of each period, the gathering
 * whole: the live view's messages go to the phone from it */
void povodok_period_done(void);

#endif
