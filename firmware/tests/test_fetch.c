/* The download as Povodok runs it (common/fetch.c): the Belka furthest behind asked first, a frame
 * each, every answer taken at its offset; one whose file is missing done at her error; a stale
 * answer dropped; one silent let go three times and left out; a download taken up again from the
 * bytes given. */
#include <stdio.h>
#include <string.h>
#include "fetch.h"
#include "fetch_belka.h"
#include "test.h"

static uint8_t file5[5000], file6[2100];

/* Belka n answers the request in frame: her file, or rc 3 when she has none */
static size_t belka(uint8_t n, const uint8_t *frame, size_t len, uint8_t *out)
{
	char name[FETCH_NAME_MAX], want[FETCH_NAME_MAX];
	uint32_t off;
	uint8_t seq;
	const uint8_t *file = n == 5 ? file5 : file6;
	size_t size = n == 5 ? sizeof(file5) : sizeof(file6), chunk;

	CHECK(fetch_belka_request(frame, len, name, sizeof(name), &off, &seq));
	snprintf(want, sizeof(want), FETCH_NAME, 8u, n);
	CHECK(strcmp(name, want) == 0);
	if (n == 9) {
		return fetch_belka_answer(out, SMP_FRAME_MAX, seq, 3, 0, NULL, 0, 0);
	}
	chunk = size - off < FETCH_BELKA_DATA(SMP_FRAME_MAX) ? size - off :
							     FETCH_BELKA_DATA(SMP_FRAME_MAX);
	return fetch_belka_answer(out, SMP_FRAME_MAX, seq, 0, off, file + off, chunk,
				  (uint32_t)size);
}

int main(void)
{
	static uint8_t req[SMP_FRAME_MAX], rsp[SMP_FRAME_MAX], got5[5000], got6[2100];
	const uint8_t three[] = { 5, 6, 9 };
	struct fetch f;
	uint8_t n, order[16];
	size_t len, rlen, data_len;
	const uint8_t *data;
	uint32_t off, from[2];
	unsigned k = 0;

	for (size_t i = 0; i < sizeof(file5); i++) {
		file5[i] = (uint8_t)(i * 7 + 1);
	}
	for (size_t i = 0; i < sizeof(file6); i++) {
		file6[i] = (uint8_t)(i * 3 + 2);
	}

	/* in turn, a frame each: 5, 6, 9, 5, 6, 5 — 9 done at her error, 6 out after two */
	fetch_begin(&f, 8, three, NULL, 3);
	while ((len = fetch_next(&f, &n, req, sizeof(req))) != 0) {
		CHECK(k < sizeof(order));
		order[k++] = n;
		rlen = belka(n, req, len, rsp);
		if (n == 9) {
			CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == -1);
			continue;
		}
		CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == 1);
		CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == 0);   /* twice: stale */
		memcpy((n == 5 ? got5 : got6) + off, data, data_len);
	}
	CHECK(k == 6 && order[0] == 5 && order[1] == 6 && order[2] == 9 && order[3] == 5 &&
	      order[4] == 6 && order[5] == 5);
	CHECK(fetch_done(&f) && fetch_end_of(&f, 5) == FETCH_WHOLE &&
	      fetch_end_of(&f, 6) == FETCH_WHOLE && fetch_end_of(&f, 9) == FETCH_REFUSED);
	CHECK(memcmp(got5, file5, sizeof(file5)) == 0 && memcmp(got6, file6, sizeof(file6)) == 0);
	OK("three Belkas in turn, a frame each, the files whole; a missing file done at its error");

	/* one silent from the start: asked again in her turns, left to another Povodok after three */
	fetch_begin(&f, 8, three, NULL, 2);
	for (k = 0; (len = fetch_next(&f, &n, req, sizeof(req))) != 0; k++) {
		if (n == 6) {
			fetch_let_go(&f, 6, 50 * k);   /* a round a second here */
			continue;
		}
		rlen = belka(n, req, len, rsp);
		CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == 1);
	}
	CHECK(fetch_end_of(&f, 6) == FETCH_ELSEWHERE && fetch_end_of(&f, 5) == FETCH_WHOLE &&
	      k == 3 + FETCH_TRIES);
	/* three misses within a second: not yet given up, a Belka coming over late */
	fetch_begin(&f, 8, three + 1, NULL, 1);
	for (k = 0; k < 2 * FETCH_TRIES && fetch_next(&f, &n, req, sizeof(req)); k++) {
		fetch_let_go(&f, 6, k);
	}
	CHECK(fetch_end_of(&f, 6) == FETCH_RUNNING);
	fetch_let_go(&f, 6, FETCH_ELSEWHERE_PERIODS);
	CHECK(fetch_end_of(&f, 6) == FETCH_ELSEWHERE);
	OK("a Belka never answering: with another Povodok after three requests and two seconds");

	/* one answering, then silent: kept in the round, gone after thirty */
	fetch_begin(&f, 8, three, NULL, 1);
	len = fetch_next(&f, &n, req, sizeof(req));
	rlen = belka(n, req, len, rsp);
	CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == 1);
	for (k = 0; (len = fetch_next(&f, &n, req, sizeof(req))) != 0; k++) {
		fetch_let_go(&f, n, k);
	}
	CHECK(k == FETCH_GONE && fetch_end_of(&f, 5) == FETCH_GONE_END);
	OK("a Belka answering, then silent: kept, gone after thirty requests");

	/* taken up again from the bytes given: the length comes with offset 0 alone, so each ends
	 * at an answer with no data */
	from[0] = 4000;
	from[1] = 2000;
	fetch_begin(&f, 8, three, from, 2);
	for (k = 0; (len = fetch_next(&f, &n, req, sizeof(req))) != 0; k++) {
		const uint8_t who[] = { 6, 6, 5, 5 };              /* the furthest behind first */
		const uint32_t at[] = { 2000, 2100, 4000, 5000 };

		rlen = belka(n, req, len, rsp);
		CHECK(fetch_answer(&f, n, rsp, rlen, &off, &data, &data_len) == 1);
		CHECK(k < 4 && n == who[k] && off == at[k] && (data_len == 0) == (k % 2 == 1));
	}
	CHECK(k == 4 && fetch_done(&f));
	OK("a download taken up again from the bytes given");
	printf("FETCH OK\n");
	return 0;
}
