/* NIC-Strelka — the live view's message to the phone, a message a period over UDP (PHONE.md, The
 * live view): the head of 21 B, then an entry of 5 B and its steps for each Belka the first
 * Povodok holds this period; 1 400 B a message at most, the rest in a second of the same head,
 * the phone joining them by the period. Built from the first Povodok's gathering
 * (povodok_link.h) and read back by the tests and the tools. Plain C11. */
#ifndef STRELKA_LIVE_MSG_H
#define STRELKA_LIVE_MSG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "povodok_link.h"
#include "protocol.h"

#define LIVE_MSG_MESSAGES 2        /* a period's gathering goes in one or two */

struct live_msg {
	uint8_t bytes[UDP_MAX_BYTES];
	size_t len;
};

/* the head written, no entries yet */
void live_msg_begin(struct live_msg *m, uint16_t session, uint32_t period, uint16_t state,
		    uint8_t s0, uint8_t povodoks, const uint16_t povodok_state[STRELKA_POVODOKS_MAX]);
/* an entry added; false when it does not fit in UDP_MAX_BYTES — a second message takes it */
bool live_msg_add(struct live_msg *m, uint8_t belka, uint8_t s0, uint8_t by, uint16_t state,
		  const uint8_t *steps, size_t steps_len);
/* this period's gathering from the first Povodok's link, before its period's end: the messages
 * filled, their number returned, 0 when the link is not the first's */
unsigned live_msg_from_link(const struct povodok_link *l, uint16_t session,
			    struct live_msg out[LIVE_MSG_MESSAGES]);

/* reading: the head, then the entries one by one, *at from sizeof(struct live_msg_head) */
bool live_msg_read_head(const uint8_t *bytes, size_t len, struct live_msg_head *h);
bool live_msg_next(const uint8_t *bytes, size_t len, size_t *at, struct live_msg_entry *e,
		   const uint8_t **steps, size_t *steps_len);

#endif
