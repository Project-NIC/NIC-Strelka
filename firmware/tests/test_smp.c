/* SMP's frames and Strelka's group 64 on the host (COMMANDS.md): the request frames built by
 * Python's cbor2 in tools/test_firmware.py are read from a file, fed through the stream's framing
 * in pieces of every size, and handled by a unit that stands in — the lock first, every command
 * of the group, the codes on a wrong value, a wrong op, an unknown command; the answers go to a
 * file for Python to read back. The values the unit stands in with are fixed here and in Python
 * alike. Beside it, SHA-256 and HMAC-SHA256 over a few strings, for Python's hashlib. */
#include <stdio.h>
#include <string.h>
#include "crypto.h"
#include "strelka_mgmt.h"
#include "test.h"

/* the password's hash, held by the unit: SHA-256 of "strelka1", as Python computes it too */
static uint8_t password_hash[32];
static struct strelka_session got_session;
static struct strelka_cal got_cal;
static struct strelka_join got_join;
static struct strelka_table_piece got_table;
static uint8_t got_table_csv[STRELKA_TABLE_PIECE_MAX];
static unsigned calls[CMD_TABLE + 1];
static bool record_on, search_on;
static uint8_t live_code;
static uint8_t blink_s, card_do, drop_n;
static uint64_t take_chip;
static uint8_t take_n;

static int f_info(struct strelka_info *out, void *ctx)
{
	(void)ctx;
	calls[CMD_INFO]++;
	out->chip = 0x0102030405060708ull;
	out->n = 5;
	out->who = WHO_BELKA;
	out->fw[0] = STRELKA_FW_MAJOR; out->fw[1] = STRELKA_FW_MINOR; out->fw[2] = STRELKA_FW_PATCH;
	memcpy(out->build, "\xde\xad\xbe\xef", 4);
	out->sensor = 0x44;
	out->card_in = true;
	out->card_mb = 30436;
	out->card_free_mb = 29000;
	out->card_fs = 2;
	out->cell_pct = 87;
	out->cell_mv = 3291;
	out->temp_centi = 2512;
	out->state = BELKA_WAITING;
	out->fault = FAULT_NONE;
	out->cal_time = 1760000000u;
	return 0;
}

static int f_session(const struct strelka_session *s, void *ctx)
{
	(void)ctx;
	calls[CMD_SESSION]++;
	got_session = *s;
	return 0;
}

static int f_sessions(uint32_t from, struct strelka_session_entry *list, unsigned max,
		      unsigned *n, bool *more, void *ctx)
{
	(void)ctx;
	calls[CMD_SESSIONS]++;
	CHECK(from == 2 && max == STRELKA_SESSIONS_A_PAGE);
	memset(list, 0, 2 * sizeof(*list));
	list[0].s = 7;
	strcpy(list[0].file, "S00007B05.STRK");
	list[0].steps = 130;
	list[0].blocks = 9;
	list[0].p0 = 1000;
	list[0].end = END_SYNC_STOP;
	list[0].bytes = 4608;
	list[1].s = 8;
	strcpy(list[1].file, "S00008B05.STRK");
	list[1].steps = 1;
	list[1].blocks = 4;
	list[1].p0 = 5000;
	list[1].end = END_UNFINISHED;
	list[1].bytes = 2048;
	*n = 2;
	*more = true;
	return 0;
}

static int f_cal_write(const struct strelka_cal *cal, void *ctx)
{
	(void)ctx;
	calls[CMD_CAL]++;
	got_cal = *cal;
	return 0;
}

static int f_cal_read(struct strelka_cal *cal, void *ctx)
{
	(void)ctx;
	calls[CMD_CAL]++;
	*cal = got_cal;
	return 0;
}

static int f_blink(uint32_t s, void *ctx)
{
	(void)ctx;
	calls[CMD_BLINK]++;
	blink_s = (uint8_t)s;
	return 0;
}

static int f_card(uint8_t what, void *ctx)
{
	(void)ctx;
	calls[CMD_CARD]++;
	card_do = what;
	return SMP_ERR_BUSY;             /* recording: refused */
}

static int f_off(void *ctx)
{
	(void)ctx;
	calls[CMD_OFF]++;
	return 0;
}

static int f_forget(void *ctx)
{
	(void)ctx;
	calls[CMD_FORGET]++;
	return 0;
}

static int f_key(const uint8_t pub[32], uint8_t pub_out[32], void *ctx)
{
	(void)ctx;
	calls[CMD_KEY]++;
	for (int i = 0; i < 32; i++) {
		CHECK(pub[i] == (uint8_t)(0x40 + i));
		pub_out[i] = (uint8_t)(0x80 + i);
	}
	return 0;
}

static int f_join(const struct strelka_join *j, void *ctx)
{
	(void)ctx;
	calls[CMD_JOIN]++;
	got_join = *j;
	return 0;
}

static int f_record(bool run, void *ctx)
{
	(void)ctx;
	calls[CMD_RECORD]++;
	record_on = run;
	return 0;
}

static int f_live(uint8_t code, void *ctx)
{
	(void)ctx;
	calls[CMD_LIVE]++;
	live_code = code;
	return 0;
}

static int f_search(bool run, void *ctx)
{
	(void)ctx;
	calls[CMD_SEARCH]++;
	search_on = run;
	return 0;
}

static int f_found(struct strelka_found *list, unsigned max, unsigned *n, void *ctx)
{
	(void)ctx;
	calls[CMD_SEARCH]++;
	CHECK(max == STRELKA_FOUND_MAX);
	list[0].chip = 0x1122334455667788ull;
	list[0].rssi = -67;
	list[0].ago_s = 3;
	list[1].chip = 0xA1A2A3A4A5A6A7A8ull;
	list[1].rssi = -80;
	list[1].ago_s = 12;
	*n = 2;
	return 0;
}

static int f_take(uint64_t chip, uint8_t n, void *ctx)
{
	(void)ctx;
	calls[CMD_TAKE]++;
	take_chip = chip;
	take_n = n;
	return n == 33 ? SMP_ERR_NOENT : 0;   /* 33: not heard */
}

static int f_set(struct strelka_set *out, void *ctx)
{
	(void)ctx;
	calls[CMD_SET]++;
	out->n = 2;
	out->belka[0] = (struct strelka_set_belka){ 1, 0x0101, 0, 0, BELKA_RECORDING, 0 };
	out->belka[1] = (struct strelka_set_belka){ 2, 0x0202, 3, 2, BELKA_RECORDING_ALONE,
						    FAULT_CELL_LOW };
	out->povodoks = 0x05;
	out->ch = 23;
	out->spare = 7;
	return 0;
}

static int f_drop(uint8_t n, void *ctx)
{
	(void)ctx;
	calls[CMD_DROP]++;
	drop_n = n;
	return n == 9 ? STRELKA_RC(STRELKA_ERR_UNREACHABLE) : 0;
}

static int f_password_hash(uint8_t hash[32], void *ctx)
{
	(void)ctx;
	memcpy(hash, password_hash, 32);
	return 0;
}

static int f_password_set(const uint8_t hash[32], void *ctx)
{
	(void)ctx;
	calls[CMD_PASSWORD]++;
	memcpy(password_hash, hash, 32);
	return 0;
}

static int f_table(const struct strelka_table_piece *p, void *ctx)
{
	(void)ctx;
	calls[CMD_TABLE]++;
	got_table = *p;
	memcpy(got_table_csv, p->csv, p->csv_len);
	return 0;
}

static const struct strelka_ops ops = {
	.info = f_info, .session = f_session, .sessions = f_sessions, .cal_write = f_cal_write,
	.cal_read = f_cal_read, .blink = f_blink, .card = f_card, .off = f_off, .forget = f_forget,
	.key = f_key, .join = f_join, .record = f_record, .live = f_live, .search = f_search,
	.found = f_found, .take = f_take, .set = f_set, .drop = f_drop,
	.password_hash = f_password_hash, .password_set = f_password_set, .table = f_table,
};

void test_random_reset(void);           /* tests/sha256_soft.c: the nonces the same again */

static struct smp_lock lock;
static FILE *answers;
static unsigned frames;
static uint8_t all_answers[65536];
static size_t all_answers_len;

static bool on_frame(const uint8_t *frame, size_t len, void *ctx)
{
	uint8_t out[SMP_HDR_BYTES + SMP_FRAME_MAX];
	size_t n = strelka_mgmt_frame(frame, len, out, sizeof(out), &ops, &lock, NULL);

	(void)ctx;
	CHECK(n >= SMP_HDR_BYTES + 1);
	frames++;
	if (answers) {
		fwrite(out, 1, n, answers);
	}
	CHECK(all_answers_len + n <= sizeof(all_answers));
	memcpy(all_answers + all_answers_len, out, n);
	all_answers_len += n;
	return true;
}

/* a transport taking one frame at a time: stop after each, the rest kept and resumed */
static bool one_at_a_time(const uint8_t *frame, size_t len, void *ctx)
{
	on_frame(frame, len, ctx);
	return false;
}

int main(int argc, char **argv)
{
	static uint8_t requests[65536];
	size_t n;
	FILE *f;
	struct smp_stream stream;

	CHECK(argc == 4);
	f = fopen(argv[1], "rb");
	CHECK(f != NULL);
	n = fread(requests, 1, sizeof(requests), f);
	fclose(f);
	CHECK(n > 0);
	strelka_sha256((const uint8_t *)"strelka1", 8, password_hash);

	/* the frames one by one, the answers kept */
	answers = fopen(argv[2], "wb");
	CHECK(answers != NULL);
	smp_stream_init(&stream);
	CHECK(smp_stream_feed(&stream, requests, n, on_frame, NULL));
	fclose(answers);
	answers = NULL;
	CHECK(stream.have == 0 && frames == 33);

	/* what the unit was told */
	CHECK(calls[CMD_INFO] == 1 && calls[CMD_SESSION] == 1 && calls[CMD_SESSIONS] == 1 &&
	      calls[CMD_CAL] == 2 && calls[CMD_BLINK] == 1 && calls[CMD_CARD] == 1 &&
	      calls[CMD_OFF] == 1 && calls[CMD_FORGET] == 1 && calls[CMD_KEY] == 1 &&
	      calls[CMD_JOIN] == 1 && calls[CMD_RECORD] == 1 && calls[CMD_LIVE] == 1 &&
	      calls[CMD_SEARCH] == 2 && calls[CMD_TAKE] == 2 && calls[CMD_SET] == 1 &&
	      calls[CMD_DROP] == 2 && calls[CMD_PASSWORD] == 1 && calls[CMD_TABLE] == 1);
	CHECK(got_session.s == 7 && got_session.rate_hz == 1000 && got_session.n == 10 &&
	      got_session.acc_g == 16 && got_session.gyro_dps == 2000 &&
	      got_session.live == S0_QUAT_50 && got_session.stop_s == 15 &&
	      got_session.t_us == 1760000000000000ull && got_session.tz_min == 120 &&
	      got_session.p_given && got_session.p == 123456);
	CHECK(got_cal.acc[0] == 16380 && got_cal.acc[1] == -16390 && got_cal.acc[5] == -16400 &&
	      got_cal.gscale[0] == 32768 && got_cal.gscale[2] == 32700 && got_cal.gzero[1] == -12 &&
	      got_cal.temp == 2550 && got_cal.t == 1759999000u);
	CHECK(blink_s == 5 && card_do == 1 && record_on && live_code == S0_RAW_50 && search_on &&
	      take_chip == 0x0807060504030201ull && take_n == 33 && drop_n == 9);
	CHECK(got_join.n == 12 && got_join.ch == 7 && memcmp(got_join.addr, "\xd4\xc3\xb2\xa1", 4) == 0 &&
	      got_join.key[0] == 1 && got_join.key[15] == 16 && got_join.nonce[12] == 0x6c &&
	      got_join.mic[3] == 0xff);
	CHECK(got_table.off == 10 && got_table.len == 40 && got_table.csv_len == 11 &&
	      memcmp(got_table_csv, "druh;k\xc5\xaf\xc5\x88\n", 11) == 0);
	CHECK(lock.open);
	{
		uint8_t want[32];

		strelka_sha256((const uint8_t *)"novy-klic-2026", 14, want);
		CHECK(memcmp(password_hash, want, 32) == 0);   /* 64/17 changed it */
	}
	OK("every command of group 64 handled, the values Python sent");

	/* the same bytes in pieces of every size give the same answers */
	for (size_t piece = 1; piece <= 2100; piece = piece < 20 ? piece + 1 : piece * 3) {
		uint8_t first[65536];
		size_t first_len = all_answers_len;

		memcpy(first, all_answers, first_len);
		all_answers_len = 0;
		frames = 0;
		smp_lock_init(&lock);
		test_random_reset();
		strelka_sha256((const uint8_t *)"strelka1", 8, password_hash);
		memset(calls, 0, sizeof(calls));
		smp_stream_init(&stream);
		for (size_t at = 0; at < n; at += piece) {
			CHECK(smp_stream_feed(&stream, requests + at, at + piece <= n ? piece : n - at,
					      on_frame, NULL));
		}
		CHECK(frames == 33 && all_answers_len == first_len &&
		      memcmp(first, all_answers, first_len) == 0);
	}
	OK("the stream cut anywhere, the same answers");

	/* fed whole, the callback stopping after each frame: the rest kept and resumed, the same
	 * answers once more */
	{
		uint8_t first[65536];
		size_t first_len = all_answers_len;

		memcpy(first, all_answers, first_len);
		all_answers_len = 0;
		frames = 0;
		smp_lock_init(&lock);
		test_random_reset();
		strelka_sha256((const uint8_t *)"strelka1", 8, password_hash);
		memset(calls, 0, sizeof(calls));
		smp_stream_init(&stream);
		for (size_t at = 0; at < n; at += 700) {
			CHECK(smp_stream_feed(&stream, requests + at, at + 700 <= n ? 700 : n - at,
					      one_at_a_time, NULL));
			while (smp_stream_has_rest(&stream)) {
				CHECK(smp_stream_resume(&stream, one_at_a_time, NULL));
			}
		}
		CHECK(frames == 33 && all_answers_len == first_len &&
		      memcmp(first, all_answers, first_len) == 0);
	}
	OK("one frame at a time, the rest kept and resumed");

	/* a header saying more than a frame holds closes the connection */
	{
		uint8_t bad[SMP_HDR_BYTES] = { 0x0A, 0, 0x08, 0x01, 0, 64, 0, 0 };   /* 2 049 B */

		smp_stream_init(&stream);
		CHECK(!smp_stream_feed(&stream, bad, sizeof(bad), on_frame, NULL));
		bad[2] = 0x08;
		bad[3] = 0x00;                   /* 2 048 B: taken, waited for */
		CHECK(smp_stream_feed(&stream, bad, sizeof(bad), on_frame, NULL) && stream.have == 8);
	}
	OK("a frame too long refused");

	/* the hashes for Python */
	f = fopen(argv[3], "w");
	CHECK(f != NULL);
	{
		static const char *texts[] = { "", "abc", "strelka1", "The quick brown fox jumps over "
					       "the lazy dog, sixty-four bytes at least, yes." };
		uint8_t h[32], m[32];

		for (unsigned i = 0; i < 4; i++) {
			CHECK(strelka_sha256((const uint8_t *)texts[i], strlen(texts[i]), h) == 0);
			CHECK(strelka_hmac_sha256((const uint8_t *)"key", 3, (const uint8_t *)texts[i],
						  strlen(texts[i]), m) == 0);
			fprintf(f, "%s|", texts[i]);
			for (int j = 0; j < 32; j++) {
				fprintf(f, "%02x", h[j]);
			}
			fprintf(f, " ");
			for (int j = 0; j < 32; j++) {
				fprintf(f, "%02x", m[j]);
			}
			fprintf(f, "\n");
		}
	}
	fclose(f);
	printf("SMP OK\n");
	return 0;
}
