/* NIC-Strelka — SMP over ESB between Povodok and a Belka (COMMANDS.md, The frame over ESB;
 * PRINCIPLE.md, The download; FIRMWARE.md, The power): a frame in fragments signed by the set's
 * key (frag.h), while the set neither records nor shows its live view, the set given over to ESB
 * while a request waits (povodok_link_awake): the SYNC's bit 14 for two of the state's periods,
 * then the SYNCs paused and the radio ESB's whole, SMP_ESB_PACKET_US a packet, every unit kept on
 * ESB while packets come to it (belka_link_spoken, povodok_link_traffic), the Povodok going round
 * its Belkas a frame from each (fetch.h); ESB as Nordic made it, its acknowledgements and repeats.
 * Povodok keeps the requests bridged to the Belkas in a short queue and speaks to one Belka at a
 * time: its request out fragment by fragment, then the Belka's answer back. A fragment goes again
 * until the radio acknowledges it, as ESB repeats it; a request not answered within
 * SMP_ESB_HOLD_PERIODS is let go, the port having answered 4 by then (smp_tcp.h). The radio's
 * turns themselves are the link's (shared/link.h): this is the logic above them, plain C, held by
 * tests/test_world.c and tools/world.py. */
#ifndef STRELKA_SMP_ESB_H
#define STRELKA_SMP_ESB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "frag.h"
#include "link_rules.h"

#define SMP_ESB_PACKET_US  1188u   /* a packet of 252 B, its acknowledgement and the turns (calc:
				    * download-pace) */
#define SMP_ESB_PACKETS    (STRELKA_PERIOD_US / SMP_ESB_PACKET_US)   /* whole in a period: 16 */
#define SMP_ESB_QUEUE         4u   /* requests waiting in Povodok */
#define SMP_ESB_HOLD_PERIODS (3u * LINK_PERIODS_A_S)   /* the bridge's 3 s (SMP_TCP_BRIDGE_MS) */

/* one frame going out in fragments, each sent until acknowledged */
struct smp_esb_out {
	uint8_t frame[SMP_FRAME_MAX];
	size_t len;
	struct frag_tx tx;
	uint8_t pkt[ESB_PAYLOAD_MAX];
	size_t pkt_len;                 /* the fragment on the air; 0 none */
	bool busy;                      /* a frame not yet out whole */
};

struct smp_esb_request {
	uint8_t to;                     /* the Belka's number */
	uint32_t token;                 /* the bridge's, back with the answer */
	uint32_t since;                 /* the period it was queued in */
	size_t len;
	uint8_t frame[SMP_FRAME_MAX];
};

struct smp_esb_povodok {
	uint8_t key[KEY_BYTES];
	uint32_t address;
	uint8_t number;                 /* this Povodok's, 0–3 */
	struct smp_esb_request q[SMP_ESB_QUEUE];
	unsigned qn;
	bool started;                   /* the head request on the air */
	bool answering;                 /* the head out whole: its answer awaited */
	bool reached;                   /* a packet of the head acknowledged */
	struct smp_esb_out out;
	struct frag_rx rx;
	uint32_t tx_count[STRELKA_BELKAS_MAX + 1];   /* a direction and a Belka: COMMANDS.md */
	uint32_t rx_last[STRELKA_BELKAS_MAX + 1];
	bool rx_any[STRELKA_BELKAS_MAX + 1];
	unsigned dropped;               /* requests let go unanswered */
};

void smp_esb_povodok_init(struct smp_esb_povodok *p, const uint8_t key[KEY_BYTES],
			  uint32_t address, uint8_t number);
/* a request bridged to the Belka numbered `to`: 0, or -1 when the queue is full or the frame too
 * long */
int smp_esb_povodok_queue(struct smp_esb_povodok *p, uint8_t to, uint32_t token,
			  const uint8_t *frame, size_t len, uint32_t period);
/* the Belka spoken to now, 0 none: the link sends its packets to her once the set is on ESB,
 * having said povodok_link_awake while a request waits (FIRMWARE.md, The power) */
uint8_t smp_esb_povodok_peer(struct smp_esb_povodok *p, uint32_t period);
/* the fragment Povodok sends next, its length; 0 when it has none now (idle, or awaiting the
 * answer) */
size_t smp_esb_povodok_next(struct smp_esb_povodok *p, uint8_t out[ESB_PAYLOAD_MAX]);
void smp_esb_povodok_acked(struct smp_esb_povodok *p);       /* the radio acknowledged it */
/* the radio gave a packet up, ESB's repeats spent: the exchange at the head let go now, the Belka
 * not on this channel — called only while none of the exchange's packets has reached her
 * (smp_esb_povodok_reached) and she has never answered this Povodok; else the packet goes again */
void smp_esb_povodok_failed(struct smp_esb_povodok *p);
/* a packet of the exchange at the head acknowledged by her */
bool smp_esb_povodok_reached(const struct smp_esb_povodok *p);
/* a fragment from the Belka numbered `from`: 1 and the answer whole in *frame, *len, its token in
 * *token, the exchange over; 0 taken, not whole yet; -1 dropped */
int smp_esb_povodok_take(struct smp_esb_povodok *p, uint8_t from, const uint8_t *payload,
			 size_t len, const uint8_t **frame, size_t *frame_len, uint32_t *token);

struct smp_esb_belka {
	uint8_t key[KEY_BYTES];
	uint32_t address;
	uint8_t number;                 /* this Belka's, 1–63 */
	struct frag_rx rx;
	uint8_t rx_from;                /* the Povodok rx is taking from */
	uint32_t rx_last[STRELKA_POVODOKS_MAX];   /* each Povodok counts its own: the last taken */
	bool rx_any[STRELKA_POVODOKS_MAX];
	struct smp_esb_out out;
	uint32_t tx_count;              /* one for all the Povodoks, each holding the last it took */
};

void smp_esb_belka_init(struct smp_esb_belka *b, const uint8_t key[KEY_BYTES], uint32_t address,
			uint8_t number);
/* a fragment from the Povodok numbered `povodok`: 1 and the request whole in b->rx.frame,
 * b->rx.len; 0 taken; -1 dropped. Any fragment taken keeps the Belka awake
 * (belka_link_spoken) */
int smp_esb_belka_take(struct smp_esb_belka *b, uint8_t povodok, const uint8_t *payload,
		       size_t len);
int smp_esb_belka_answer(struct smp_esb_belka *b, const uint8_t *frame, size_t len);
size_t smp_esb_belka_next(struct smp_esb_belka *b, uint8_t povodok,
			  uint8_t out[ESB_PAYLOAD_MAX]);
void smp_esb_belka_acked(struct smp_esb_belka *b);

#endif
