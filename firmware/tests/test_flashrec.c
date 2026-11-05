/* NIC-Strelka — Kryska's record raw on its flash (common/flashrec.c), on the host over a
 * simulated memory: the first erased block found by halving in an empty, a part-written and a
 * full memory; the sessions found backwards — closed ones by their end, one the power cut off by
 * its last data block, one of the head alone, one with blocks dropped and one whose end counts
 * them wrong; a page the power cut in the middle passed over; the same number again named _2; a
 * session added after the scan; the list's cap. */
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "flashrec.h"
#include "record.h"

#define BLOCKS 4096u                     /* 2 MB of a memory */

static uint8_t memory[BLOCKS][REC_BLOCK_BYTES];
static struct flashrec f;
static bool fail_reads;

static bool reader(void *ctx, uint32_t index, uint8_t out[REC_BLOCK_BYTES])
{
	(void)ctx;
	if (fail_reads || index >= BLOCKS) {
		return false;
	}
	memcpy(out, memory[index], REC_BLOCK_BYTES);
	return true;
}

static void erase_all(void)
{
	memset(memory, 0xFF, sizeof(memory));
}

static void put(uint32_t at, const uint8_t block[REC_BLOCK_BYTES])
{
	CHECK(at < BLOCKS);
	memcpy(memory[at], block, REC_BLOCK_BYTES);
}

/* a session written from block `at`: the head, `parts` of a table, `data` data and information
 * blocks numbered from 0 with `dropped` of them left out of the memory, the end when `ended`,
 * with `end_dropped` in its count; the blocks written returned */
static uint32_t write_session(uint32_t at, uint16_t session, uint8_t belka, unsigned parts,
			      unsigned data, unsigned dropped, bool ended, uint32_t end_dropped)
{
	static uint8_t block[REC_BLOCK_BYTES];
	static char text[3 * REC_TABLE_TEXT];
	struct rec_session cfg = { .session = session, .belka = belka, .rate_hz = 400, .n = 1,
				   .acc_fs = 16, .gyro_dps = 2000, .first_period = 1000,
				   .place = "", .name = "" };
	struct strelka_step step = { { 1, 2, 3 }, { 4, 5, 6 } };
	uint32_t i = at;

	rec_head_block(block, &cfg);
	put(i++, block);
	memset(text, 'a', sizeof(text));
	for (unsigned p = 0; p < parts; p++) {
		rec_table_block(block, session, p, (const uint8_t *)text,
				parts * REC_TABLE_TEXT - 7);
		put(i++, block);
	}
	for (unsigned k = 0; k < data; k++) {
		if (k % 4 == 3) {
			rec_info_block(block, session, k, 1000 + k, 2500, 3300, NULL, 0);
		} else {
			rec_data_block(block, session, k, 1000 + k, 0, 0, 0, 0, &step, 1);
		}
		if (dropped && k == 1) {
			k += dropped - 1;        /* the numbers go on, the blocks are not there */
			continue;
		}
		put(i++, block);
	}
	if (ended) {
		rec_end_block(block, session, END_SYNC_STOP, data * 40, data, 1000 + data,
			      end_dropped);
		put(i++, block);
	}
	return i - at;
}

static void check_entry(unsigned i, uint16_t session, uint8_t belka, uint32_t first,
			uint32_t blocks, uint16_t again, const char *name)
{
	char got[FLASHREC_NAME_MAX];

	CHECK(i < f.n);
	CHECK(f.list[i].session == session && f.list[i].belka == belka);
	CHECK(f.list[i].first == first && f.list[i].blocks == blocks);
	CHECK(f.list[i].again == again);
	flashrec_name(&f.list[i], got);
	CHECK(strcmp(got, name) == 0);
	CHECK(flashrec_find(&f, name) == (int)i);
}

static void test_empty(void)
{
	erase_all();
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.next == 0 && f.n == 0 && !f.more);
	CHECK(flashrec_scan(&f, reader, NULL, 0) == 0 && f.next == 0);
	OK("an empty memory: the next session at block 0, none listed");
}

static void test_sessions(void)
{
	uint32_t at = 0, a, b, c, d, e;

	erase_all();
	a = write_session(at, 7, 3, 2, 9, 0, true, 0);          /* closed, a table of two */
	at += a;
	b = write_session(at, 7, 3, 0, 5, 0, true, 0);          /* the same number again */
	at += b;
	c = write_session(at, 8, 3, 1, 11, 2, true, 2);         /* two blocks dropped, counted */
	at += c;
	d = write_session(at, 9, 3, 1, 11, 2, true, 0);         /* dropped and not counted */
	at += d;
	e = write_session(at, 10, 3, 1, 6, 0, false, 0);        /* the power gone: no end */
	at += e;
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.next == at && f.n == 5 && !f.more);
	check_entry(0, 7, 3, 0, a, 0, "S00007B03.STRK");
	check_entry(1, 7, 3, a, b, 1, "S00007B03_2.STRK");
	check_entry(2, 8, 3, a + b, c, 0, "S00008B03.STRK");
	check_entry(3, 9, 3, a + b + c, d, 0, "S00009B03.STRK");
	check_entry(4, 10, 3, a + b + c + d, e, 0, "S00010B03.STRK");
	CHECK(flashrec_find(&f, "S00007B03_3.STRK") == -1);
	CHECK(flashrec_find(&f, "S00007B03_1.STRK") == -1);
	CHECK(flashrec_find(&f, "S00011B03.STRK") == -1);
	CHECK(flashrec_find(&f, "STRELKA.SET") == -1);
	CHECK(flashrec_find(&f, "S00007B03.STRK.bak") == -1);
	OK("five sessions found backwards: by the end, dropped blocks counted or not, by the "
	   "last data block; the same number again named _2");

	/* a page the power cut in the middle: the first 256 B of a block written, the rest
	 * erased — not erased, its CRC broken; passed over, the next session after it */
	memset(memory[at], 0, 256);
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.next == at + 1 && f.n == 5);
	check_entry(4, 10, 3, a + b + c + d, e, 0, "S00010B03.STRK");
	OK("a block the power left half written passed over, the next session after it");

	/* a session written after the scan and added: listed, next moved */
	CHECK(write_session(f.next, 7, 3, 0, 2, 0, true, 0) == 4);
	flashrec_add(&f, 7, 3, f.next, 4);
	CHECK(f.n == 6 && f.next == at + 1 + 4);
	check_entry(5, 7, 3, at + 1, 4, 2, "S00007B03_3.STRK");
	OK("a session added after the scan, the third of its number named _3");

	/* the power gone right after the head */
	at = f.next;
	write_session(at, 11, 4, 0, 0, 0, false, 0);
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.n == 7 && f.next == at + 1);
	check_entry(6, 11, 4, at, 1, 0, "S00011B04.STRK");
	OK("a session of the head alone");

	fail_reads = true;
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == -1);
	fail_reads = false;
	OK("a memory that cannot be read said so");
}

/* two sessions of one number back to back, the later with blocks dropped and not counted: the
 * guess at its head falls into the earlier one and must not be taken */
static void test_same_number(void)
{
	uint32_t a, b;

	erase_all();
	a = write_session(0, 7, 3, 0, 3, 0, true, 0);
	b = write_session(a, 7, 3, 1, 10, 5, false, 0);       /* no end, five blocks gone */
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.n == 2 && f.next == a + b);
	check_entry(0, 7, 3, 0, a, 0, "S00007B03.STRK");
	check_entry(1, 7, 3, a, b, 1, "S00007B03_2.STRK");
	b = write_session(a + b, 7, 3, 1, 10, 5, true, 0);    /* an end counting none dropped */
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.n == 3);
	check_entry(2, 7, 3, f.list[1].first + f.list[1].blocks, b, 2, "S00007B03_3.STRK");
	OK("two sessions of one number back to back kept apart, the later's blocks dropped");

	/* a run of data blocks with no head before them, after an end: passed over, not merged
	 * into the session before */
	erase_all();
	a = write_session(0, 8, 3, 0, 2, 0, true, 0);
	b = write_session(a, 8, 3, 0, 4, 0, false, 0);
	memset(memory[a], 0, 256);                             /* the head half written */
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.n == 1 && f.next == a + b);
	check_entry(0, 8, 3, 0, a, 0, "S00008B03.STRK");
	OK("a run with no head passed over whole");
}

static void test_full(void)
{
	uint32_t at = 0;
	unsigned n = 0;

	erase_all();
	while (at + 3 <= BLOCKS) {
		at += write_session(at, (uint16_t)(100 + n % 50), 1, 0, 1, 0, true, 0);
		n++;
	}
	CHECK(at == BLOCKS - 1);
	write_session(at, 1, 1, 0, 0, 0, false, 0);              /* the last block a head */
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.next == BLOCKS);
	CHECK(f.n == FLASHREC_SESSIONS_MAX && f.more);
	CHECK(f.list[f.n - 1].session == 1 && f.list[f.n - 1].first == BLOCKS - 1);
	CHECK(f.list[0].first == BLOCKS - 1 - 3 * (FLASHREC_SESSIONS_MAX - 1));
	flashrec_add(&f, 2, 1, f.next, 1);                       /* no room: listed all the same */
	CHECK(f.n == FLASHREC_SESSIONS_MAX && f.list[f.n - 1].session == 2 && f.more);
	OK("a full memory: the newest listed up to the cap, the oldest left out");

	/* one number throughout: the names count from the memory's beginning, and a session
	 * added after the cap takes the next name, never one in the list */
	erase_all();
	at = 0;
	for (n = 0; n < FLASHREC_SESSIONS_MAX + 6; n++) {
		at += write_session(at, 1, 1, 0, 0, 0, true, 0);      /* two blocks each */
	}
	CHECK(flashrec_scan(&f, reader, NULL, BLOCKS) == 0);
	CHECK(f.n == FLASHREC_SESSIONS_MAX && f.more);
	CHECK(f.list[0].again == 6 && f.list[f.n - 1].again == FLASHREC_SESSIONS_MAX + 5);
	CHECK(flashrec_find(&f, "S00001B01_7.STRK") == 0);
	CHECK(flashrec_find(&f, "S00001B01.STRK") == -1);        /* the first six are not listed */
	flashrec_add(&f, 1, 1, f.next, 2);
	CHECK(f.list[f.n - 1].again == FLASHREC_SESSIONS_MAX + 6);
	CHECK(flashrec_find(&f, "S00001B01_1031.STRK") == (int)f.n - 1);
	OK("the names stable past the cap");
}

int main(void)
{
	test_empty();
	test_sessions();
	test_same_number();
	test_full();
	return 0;
}
