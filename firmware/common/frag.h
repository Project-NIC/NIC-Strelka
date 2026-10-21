/* NIC-Strelka — an SMP frame in fragments over ESB, each signed by the set's key, a count in it
 * against a replay (COMMANDS.md, The frame over ESB): byte 0 the fragment's kind and index,
 * 1–4 the count, then up to 243 B of the frame, the last 4 the MIC, AES-CCM's tag over every
 * byte before it, the nonce the set's address, the sender's who, the count, the index, the
 * receiver's who and two zeros — the receiver in it, so that a fragment meant for one Belka is
 * nothing to another. One count a direction and a Belka, from 0 at the taking: the sender's
 * `count` is kept by the caller across frames and starts, the receiver drops a fragment whose
 * count is not above the last taken, or under the floor it was given at its start. */
#ifndef STRELKA_FRAG_H
#define STRELKA_FRAG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "crypto.h"
#include "protocol.h"

struct frag_keys {
	uint8_t key[KEY_BYTES];  /* the set's */
	uint32_t address;        /* the set's */
	uint8_t sender;          /* the who bytes in the nonce: on sending one's own and the
				  * other's, on receiving the other's and one's own */
	uint8_t receiver;
};

void frag_nonce(uint8_t nonce[NONCE_BYTES], uint32_t address, uint8_t sender, uint8_t receiver,
		uint32_t count, uint8_t index);

struct frag_tx {
	const uint8_t *frame;
	size_t len;
	size_t at;
	uint8_t index;
};

void frag_tx_begin(struct frag_tx *t, const uint8_t *frame, size_t len);   /* len ≤ 2 048 */
/* the next fragment into out: its length, or 0 when the frame has all gone; the count goes up
 * by one for each fragment made */
size_t frag_tx_next(struct frag_tx *t, const struct frag_keys *k, uint32_t *count,
		    uint8_t out[ESB_PAYLOAD_MAX]);

struct frag_rx {
	uint8_t frame[SMP_FRAME_MAX];
	size_t len;
	uint8_t expect;          /* the index expected next */
	bool open;               /* a frame begun */
	bool any;                /* a count taken already */
	uint32_t last_count;
	uint32_t floor;          /* no count under it is taken: the one saved before the last
				  * start, plus the saving's step */
};

enum frag_result {
	FRAG_NONE = 0,           /* taken, the frame not yet whole */
	FRAG_FRAME = 1,          /* a whole frame in frame[0 .. len) */
	FRAG_DROPPED = -1,       /* a wrong MIC, an old count, an index out of order, too long */
};

/* floor: 0 at the taking; after a start the count saved last plus FRAG_COUNT_SAVE_STEP, since
 * the sender may have gone that far unsaved */
#define FRAG_COUNT_SAVE_STEP 1024u
void frag_rx_begin(struct frag_rx *r, uint32_t floor);
enum frag_result frag_rx_take(struct frag_rx *r, const struct frag_keys *k, const uint8_t *payload,
			      size_t len);

#endif
