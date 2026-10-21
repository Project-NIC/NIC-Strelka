/* NIC-Strelka — SMP's frames as they go over TCP and over ESB (COMMANDS.md): the header of 8 B,
 * big-endian as Zephyr writes it; a stream cut into frames, each its header and `length` bytes,
 * nothing between; the lock on a connection; an answer with Zephyr's error code alone. Plain
 * C11 on zcbor, compiled on the host for the tests and into both units. */
#ifndef STRELKA_SMP_H
#define STRELKA_SMP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

#define SMP_HDR_BYTES    8u
#define SMP_NONCE_BYTES  16u
#define SMP_MAC_BYTES    32u

struct smp_head {           /* the header read */
	uint8_t op;             /* SMP_OP_READ … SMP_OP_WRITE_RSP */
	uint8_t version;        /* 1 */
	uint8_t flags;
	uint16_t len;           /* the CBOR body's */
	uint16_t group;
	uint8_t seq;
	uint8_t id;
};

void smp_head_read(const uint8_t bytes[SMP_HDR_BYTES], struct smp_head *h);
void smp_head_write(uint8_t bytes[SMP_HDR_BYTES], const struct smp_head *h);
/* the answer's header to a request's: the op one up, the length given */
void smp_head_answer(const struct smp_head *req, uint16_t len, struct smp_head *rsp);

/* a TCP stream into frames: fed what arrived, it calls `on_frame` with each whole frame, header
 * first; false when a header says more than SMP_FRAME_MAX, the connection to be closed. The
 * callback answers whether to go on: false, and the bytes after the frame are kept for
 * smp_stream_resume, so that a transport handles one request at a time on a connection and
 * reads no further until its answer has gone */
#define SMP_STREAM_REST 1024u         /* what a read may bring at most beyond one frame */
struct smp_stream {
	uint8_t buf[SMP_HDR_BYTES + SMP_FRAME_MAX];
	size_t have;
	uint8_t rest[SMP_STREAM_REST];
	size_t rest_len;
};
typedef bool (*smp_frame_fn)(const uint8_t *frame, size_t len, void *ctx);
void smp_stream_init(struct smp_stream *s);
bool smp_stream_feed(struct smp_stream *s, const uint8_t *data, size_t n, smp_frame_fn on_frame,
		     void *ctx);
/* the bytes kept when a callback said stop, fed again; false as feed's */
bool smp_stream_resume(struct smp_stream *s, smp_frame_fn on_frame, void *ctx);
bool smp_stream_has_rest(const struct smp_stream *s);

/* the lock, one a connection (COMMANDS.md, 64/16): until the application has answered the nonce
 * with its MAC, every frame but 64/16's answers 11, access denied */
struct smp_lock {
	bool open;
	bool nonce_set;         /* a nonce read and not yet answered, a pair begun with it */
	uint8_t nonce[SMP_NONCE_BYTES];
};
void smp_lock_init(struct smp_lock *l);
/* true when the frame may go on to its handler: the lock open, no lock, or the request 64/16 */
bool smp_lock_passes(const struct smp_lock *l, const struct smp_head *h);
/* the MAC the application owes: HMAC-SHA256 with the password's SHA-256 as the key over the nonce
 * followed by the secret agreed by X25519 for this connection, its SHA-256 of 32 B; true when
 * `mac` is it, in constant time, the nonce spent either way — so that whoever listens cannot try
 * passwords on what he heard */
bool smp_lock_check(struct smp_lock *l, const uint8_t hash[32], const uint8_t secret[32],
		    const uint8_t mac[SMP_MAC_BYTES]);

/* an answer with Zephyr's code alone, {"rc": rc} — SMP version 2 gives the standard codes bare
 * and a group's own in {"err": {"group", "rc"}}; the frame's bytes written, 0 when `cap` is short */
size_t smp_error_frame(uint8_t *out, size_t cap, const struct smp_head *req, int rc);
/* an answer with a group's own code, {"err": {"group": group, "rc": rc}} */
size_t smp_group_error_frame(uint8_t *out, size_t cap, const struct smp_head *req, uint16_t group,
			     uint16_t rc);

#endif
