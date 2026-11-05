/* NIC-Strelka — Kryska's record raw on its flash, the part that needs no Zephyr (flashrec.h). */
#include <stdio.h>
#include <string.h>
#include "flashrec.h"
#include "record.h"

static uint16_t le16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

bool flashrec_erased(const uint8_t block[REC_BLOCK_BYTES])
{
	for (size_t i = 0; i < REC_BLOCK_BYTES; i++) {
		if (block[i] != 0xFF) {
			return false;
		}
	}
	return true;
}

/* a whole block of session `s`: its CRC holds and its kind is one of the record's */
static bool whole(const uint8_t *b, uint16_t s)
{
	if (!rec_block_ok(b)) {
		return false;
	}
	if (memcmp(b, "STRK", 4) == 0) {
		return b[5] == REC_HEAD && le16(b + 6) == s;
	}
	return (b[0] == REC_DATA || b[0] == REC_INFO || b[0] == REC_TABLE || b[0] == REC_END) &&
	       le16(b + 2) == s;
}

static bool is_head(const uint8_t *b, uint16_t s)
{
	return rec_block_ok(b) && memcmp(b, "STRK", 4) == 0 && b[5] == REC_HEAD &&
	       le16(b + 6) == s;
}

static int read_at(struct flashrec *f, uint32_t i)
{
	return f->read(f->ctx, i, f->block) ? 0 : -1;
}

/* from block `g`, the last table part or the head itself, back to the head of session s; else
 * -1, the guess having missed */
static int back_over_table(struct flashrec *f, uint32_t g, uint16_t s, uint32_t *head)
{
	if (read_at(f, g)) {
		return -1;
	}
	if (is_head(f->block, s)) {
		*head = g;
		return 0;
	}
	if (rec_block_ok(f->block) && f->block[0] == REC_TABLE && le16(f->block + 2) == s &&
	    f->block[1] + 1u <= g) {
		g -= f->block[1] + 1u;
		if (read_at(f, g)) {
			return -1;
		}
		if (is_head(f->block, s)) {
			*head = g;
			return 0;
		}
	}
	return -1;
}

/* the first data or information block after the head at `h`, past the table's parts; the block
 * after the last part when there is no data yet */
static int first_data(struct flashrec *f, uint32_t h, uint16_t s, uint32_t last, uint32_t *fd)
{
	uint32_t i = h + 1;

	while (i <= last) {
		if (read_at(f, i)) {
			return -1;
		}
		if (!(rec_block_ok(f->block) && f->block[0] == REC_TABLE && le16(f->block + 2) == s)) {
			break;
		}
		i++;
	}
	*fd = i;
	return 0;
}

/* the head of the session whose last whole block is `last`: by the end's count, by a data or
 * information block's number, by a table part's number — each guess checked by its mark, its
 * session and its distance, since blocks dropped and counted otherwise make a guess land too
 * early, in an earlier session of the same number even; when it misses, block by block back to
 * the head, stopping at an end block or another session's. -1 for a block that is not a whole
 * block of a session (the power gone in the middle of a page: not erased, its CRC broken), -3
 * for a run of blocks with no head before it, `*skip_to` the run's first, -2 when the memory
 * cannot be read */
static int find_head(struct flashrec *f, uint32_t last, uint32_t *head, uint16_t *s,
		     uint8_t *belka, uint32_t *skip_to)
{
	const uint8_t *b = f->block;
	uint32_t guess, back, fd;

	if (read_at(f, last)) {
		return -2;
	}
	if (!rec_block_ok(b)) {
		return -1;
	}
	if (memcmp(b, "STRK", 4) == 0) {
		if (b[5] != REC_HEAD) {
			return -1;
		}
		*s = le16(b + 6);
		*belka = b[8];
		*head = last;
		return 0;
	}
	*s = le16(b + 2);
	switch (b[0]) {
	case REC_END:
		/* the end counts the data and information blocks given out, the dropped among
		 * them (RECORD.md, The end): the block before the first data block */
		back = le32(b + 8) - le32(b + 16);
		guess = last >= back + 1 ? last - back - 1 : 0;
		break;
	case REC_DATA:
	case REC_INFO:
		back = le32(b + 4);                  /* the block's number, the first 0 */
		guess = last >= back + 1 ? last - back - 1 : 0;
		break;
	case REC_TABLE:
		back = b[1];
		guess = last >= back + 1 ? last - back - 1 : 0;
		break;
	default:
		return -1;
	}
	if (back_over_table(f, guess, *s, head) == 0) {
		/* the data blocks between the head and `last` can be no more than the count
		 * says: more, and the guess fell into an earlier session of the same number */
		if (first_data(f, *head, *s, last, &fd)) {
			return -2;
		}
		if (fd <= last && last - fd <= back) {
			if (read_at(f, *head)) {
				return -2;
			}
			*belka = f->block[8];
			return 0;
		}
	}
	/* the guess missed: back one by one to a head of this session, over its own blocks; an
	 * end block or another session's block before a head means the run has none */
	for (uint32_t i = last; i > 0; i--) {
		if (read_at(f, i - 1)) {
			return -2;
		}
		if (is_head(f->block, *s)) {
			*head = i - 1;
			*belka = f->block[8];
			return 0;
		}
		if (!whole(f->block, *s) || f->block[0] == REC_END) {
			*skip_to = i;
			return -3;
		}
	}
	*skip_to = 0;
	return -3;
}

/* an older session of the pair, listed or not: the listed ones of the pair, all newer, count it */
static void count_older(struct flashrec *f, uint16_t session, uint8_t belka)
{
	for (unsigned k = 0; k < f->n; k++) {
		if (f->list[k].session == session && f->list[k].belka == belka) {
			f->list[k].again++;
		}
	}
}

/* the entries listed newest first turned oldest first; `again` was counted on the way */
static void settle(struct flashrec *f)
{
	for (unsigned i = 0, j = f->n; i + 1 < j; i++, j--) {
		struct flashrec_session t = f->list[i];

		f->list[i] = f->list[j - 1];
		f->list[j - 1] = t;
	}
}

int flashrec_scan(struct flashrec *f, flashrec_read_t read, void *ctx, uint32_t blocks)
{
	uint32_t lo, hi, i;

	f->read = read;
	f->ctx = ctx;
	f->blocks = blocks;
	f->n = 0;
	f->more = false;
	f->next = 0;
	if (blocks == 0) {
		return 0;
	}
	/* the first erased block by halving: the blocks are written in one run from 0 */
	if (read_at(f, 0)) {
		return -1;
	}
	if (flashrec_erased(f->block)) {
		return 0;
	}
	lo = 0;                                  /* written */
	hi = blocks;                             /* erased, or the memory's end */
	while (hi - lo > 1) {
		uint32_t mid = lo + (hi - lo) / 2;

		if (read_at(f, mid)) {
			return -1;
		}
		if (flashrec_erased(f->block)) {
			hi = mid;
		} else {
			lo = mid;
		}
	}
	f->next = hi;
	/* the sessions backwards from there; past the list's cap the walk goes on all the same,
	 * so that the listed ones' `again` counts every older session of their number */
	i = f->next;
	while (i > 0) {
		uint32_t head, skip_to;
		uint16_t s;
		uint8_t belka;
		int rc = find_head(f, i - 1, &head, &s, &belka, &skip_to);

		if (rc == -2) {
			return -1;
		}
		if (rc == -1) {
			i--;                         /* a block of no session: passed over */
			continue;
		}
		if (rc == -3) {
			i = skip_to;                 /* a run with no head: passed over whole */
			continue;
		}
		count_older(f, s, belka);
		if (f->n == FLASHREC_SESSIONS_MAX) {
			f->more = true;
		} else {
			f->list[f->n].first = head;
			f->list[f->n].blocks = i - head;
			f->list[f->n].session = s;
			f->list[f->n].belka = belka;
			f->list[f->n].again = 0;
			f->n++;
		}
		i = head;
	}
	settle(f);
	return 0;
}

void flashrec_add(struct flashrec *f, uint16_t session, uint8_t belka, uint32_t first,
		  uint32_t blocks)
{
	uint16_t again = 0;

	if (first + blocks > f->next) {
		f->next = first + blocks;
	}
	/* the name after the last listed of its number, never a recount: stable while the list
	 * holds one of the pair (a pair whose every listed session has left the list starts at
	 * the name the next scan gives it) */
	for (unsigned k = 0; k < f->n; k++) {
		if (f->list[k].session == session && f->list[k].belka == belka &&
		    f->list[k].again + 1u > again) {
			again = (uint16_t)(f->list[k].again + 1u);
		}
	}
	if (f->n == FLASHREC_SESSIONS_MAX) {
		/* the oldest leaves the list, unreachable until the memory is erased */
		memmove(f->list, f->list + 1, (FLASHREC_SESSIONS_MAX - 1) * sizeof(f->list[0]));
		f->n--;
		f->more = true;
	}
	f->list[f->n].first = first;
	f->list[f->n].blocks = blocks;
	f->list[f->n].session = session;
	f->list[f->n].belka = belka;
	f->list[f->n].again = again;
	f->n++;
}

void flashrec_name(const struct flashrec_session *s, char out[FLASHREC_NAME_MAX])
{
	if (s->again == 0) {
		snprintf(out, FLASHREC_NAME_MAX, "S%05uB%02u.STRK", s->session, s->belka);
	} else {
		snprintf(out, FLASHREC_NAME_MAX, "S%05uB%02u_%u.STRK", s->session, s->belka,
			 s->again + 1u);
	}
}

/* digits at *p, at most `max` of them, into *v; false when there is none */
static bool digits(const char **p, unsigned max, unsigned *v)
{
	unsigned n = 0;

	*v = 0;
	while (n < max && **p >= '0' && **p <= '9') {
		*v = *v * 10 + (unsigned)(**p - '0');
		(*p)++;
		n++;
	}
	return n > 0;
}

int flashrec_find(const struct flashrec *f, const char *name)
{
	const char *p = name;
	unsigned session, belka, again = 0, k;

	if (*p++ != 'S' || !digits(&p, 5, &session) || *p++ != 'B' || !digits(&p, 2, &belka)) {
		return -1;
	}
	if (*p == '_') {
		p++;
		if (!digits(&p, 5, &k) || k < 2) {
			return -1;
		}
		again = k - 1;
	}
	if (strcmp(p, ".STRK") != 0 || session > 0xFFFF || belka > 0xFF) {
		return -1;
	}
	for (unsigned i = 0; i < f->n; i++) {
		if (f->list[i].session == session && f->list[i].belka == belka &&
		    f->list[i].again == again) {
			return (int)i;
		}
	}
	return -1;
}
