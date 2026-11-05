/* NIC-Strelka — the inside link's six packets built and read (PRINCIPLE.md, The inside link).
 * A packet is built in the radio's own RAM layout, struct strelka_packet; the builders return the
 * bytes the radio takes from RAM, who to the body's end. The radio's address, channel and timing
 * are the link's, not here. */
#ifndef STRELKA_FRAME_H
#define STRELKA_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "frag.h"
#include "protocol.h"

enum frame_kind {
	FRAME_SYNC,
	FRAME_LIVE,
	FRAME_MISS,
	FRAME_PASS,
	FRAME_CALL,
	FRAME_HERE,
};

struct frame {              /* a packet read */
	enum frame_kind kind;
	uint8_t sender;         /* WHO_POVODOK or WHO_BELKA */
	uint8_t number;
	uint8_t s0;
	uint32_t period;
	uint16_t state;
	const uint8_t *body;
	size_t body_len;
};

/* the live view's code: its steps a period and their bytes */
unsigned live_steps(uint8_t code);         /* 0, 1 or 2 */
unsigned live_step_bytes(uint8_t code);    /* 0, 4 a quaternion, 12 a raw step */
unsigned live_body_bytes(uint8_t code);
bool live_code_ok(uint8_t code);           /* 0, 1, 2, 4, 5 */

size_t frame_ram_len(const struct strelka_packet *p);   /* who to the body's end */

/* the builders; `steps` are live_steps(code) quaternions or raw steps, as the code says */
size_t frame_sync(struct strelka_packet *p, uint8_t povodok, uint8_t live, uint8_t item,
		  uint32_t period, uint16_t state, const uint8_t sig[SYNC_BODY_BYTES]);
size_t frame_call(struct strelka_packet *p, uint8_t povodok, uint8_t attempt, uint32_t period,
		  uint16_t state, const uint8_t sig[SYNC_BODY_BYTES]);
size_t frame_live(struct strelka_packet *p, uint8_t belka, uint8_t live, uint8_t item,
		  uint32_t period, uint16_t state, const void *steps);
/* the MISS is the first Povodok's, whoever stands in as the first (PRINCIPLE.md) */
size_t frame_miss(struct strelka_packet *p, uint8_t povodok, uint32_t period, uint16_t state,
		  const uint8_t missed[MISS_BODY_BYTES]);
size_t frame_pass_begin(struct strelka_packet *p, uint8_t povodok, uint32_t period,
			uint16_t state);
bool frame_pass_add(struct strelka_packet *p, uint8_t belka, uint8_t s0, uint16_t state,
		    const void *steps);    /* false: no room left */
size_t frame_here(struct strelka_packet *p, uint64_t chip, uint32_t period);

/* the SYNC's signature: AES-CCM's tag with the set's key over the packet's head in RAM, the nonce
 * the fragment's with the period for the count, S0 for the index and no receiver; written into the
 * body, or checked */
void frame_sync_sign(struct strelka_packet *p, const uint8_t key[KEY_BYTES], uint32_t address);
bool frame_sync_check(const struct strelka_packet *p, const uint8_t key[KEY_BYTES],
		      uint32_t address);

/* reading: false when the packet is malformed — a length out of bounds, a body of the wrong
 * size for its kind, a sender's bits unused */
bool frame_read(const struct strelka_packet *p, size_t ram_len, struct frame *f);

struct pass_item {
	uint8_t belka;
	uint8_t s0;
	uint16_t state;
	const uint8_t *steps;
	size_t steps_len;
};
/* the items of a PASS one by one: *at from 0; false when none is left or the rest is malformed */
bool frame_pass_next(const struct frame *f, size_t *at, struct pass_item *item);

/* the MISS's bits */
void miss_set(uint8_t missed[MISS_BODY_BYTES], unsigned belka);
bool miss_has(const uint8_t missed[MISS_BODY_BYTES], unsigned belka);

/* the search: the number a new Belka draws from its chip's ID and the CALL's attempt — the low 16
 * bits of SplitMix64's number attempt + 1 from the ID as the seed (PRINCIPLE.md, The search) */
uint16_t search_draw(uint64_t chip, uint8_t attempt);
#define SEARCH_PERIOD(draw)  ((draw) / SEARCH_WINDOWS)
#define SEARCH_WINDOW(draw)  ((draw) % SEARCH_WINDOWS)

#endif
