/* NIC-Strelka — Kryska's other way, its record raw on NOR flash (chapters/kryska-memory.md): the
 * sessions' blocks of RECORD.md one after another from block 0 into memory erased beforehand, no
 * file system. What needs no Zephyr, checked on the host (tests/test_flashrec.c): where the
 * written run ends, the first erased block, found by halving; the sessions found backwards from
 * there, each by its end block, or by its last data block when the power went before the end;
 * the list kept as sessions are added; a session's name, S00007B03.STRK as the card's, _2 and on
 * for the same number and Belka again, since nothing here is overwritten. The reader of blocks is
 * the caller's. */
#ifndef STRELKA_FLASHREC_H
#define STRELKA_FLASHREC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

#define FLASHREC_SESSIONS_MAX 1024u  /* listed, the newest; 128 MB over 1 024 is 23 s at 400 Hz */
#define FLASHREC_NAME_MAX     24u    /* "S00007B03_1024.STRK", room for any again */

/* true with block `index` read into `out`; false when the memory cannot be read */
typedef bool (*flashrec_read_t)(void *ctx, uint32_t index, uint8_t out[REC_BLOCK_BYTES]);

struct flashrec_session {
	uint32_t first;         /* the head's block */
	uint32_t blocks;        /* the run: the head to the end, or to the last block written */
	uint16_t session;
	uint8_t belka;
	uint16_t again;         /* 0 the first of its number and Belka in the memory, 1 the next */
};

struct flashrec {
	flashrec_read_t read;
	void *ctx;
	uint32_t blocks;        /* the memory's */
	uint32_t next;          /* the first erased block: where the next session begins */
	unsigned n;             /* sessions listed, the oldest first */
	bool more;              /* older sessions beyond FLASHREC_SESSIONS_MAX, not listed */
	uint8_t block[REC_BLOCK_BYTES];    /* the reader's */
	struct flashrec_session list[FLASHREC_SESSIONS_MAX];
};

bool flashrec_erased(const uint8_t block[REC_BLOCK_BYTES]);   /* every byte FFh */
/* the memory read: `next` by halving, the sessions backwards from it; -1 when a read fails */
int flashrec_scan(struct flashrec *f, flashrec_read_t read, void *ctx, uint32_t blocks);
/* a session just written, its head at `first`, `blocks` long: listed, `next` moved */
void flashrec_add(struct flashrec *f, uint16_t session, uint8_t belka, uint32_t first,
		  uint32_t blocks);
void flashrec_name(const struct flashrec_session *s, char out[FLASHREC_NAME_MAX]);
int flashrec_find(const struct flashrec *f, const char *name);   /* its index, or -1 */

#endif
