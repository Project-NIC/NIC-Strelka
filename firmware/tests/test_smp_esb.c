/* SMP over ESB on the host (common/smp_esb.c): a request from Povodok to a Belka in fragments and
 * her answer back, each fragment sent again until acknowledged; a fragment lost, a stranger's
 * answer, a request left unanswered past the bridge's 3 s, a full queue. */
#include <string.h>
#include "smp_esb.h"
#include "test.h"

static const uint8_t KEY[KEY_BYTES] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
#define ADDRESS 0xA1B2C3D4u

static struct smp_esb_povodok pv, pv1;
static struct smp_esb_belka b5, b6;
static uint8_t request[600], answer[2000];

/* the air: a period's packets, Povodok's first, then the Belka's, every `lose`th lost */
static int air_from(struct smp_esb_povodok *p, uint32_t period, unsigned lose, unsigned *n,
		    struct smp_esb_belka *b, const uint8_t **got, size_t *got_len, uint32_t *token)
{
	uint8_t pkt[ESB_PAYLOAD_MAX];

	if (smp_esb_povodok_peer(p, period) == 0) {
		return 0;
	}
	for (unsigned i = 0; i < SMP_ESB_PACKETS; i++) {
		size_t len = smp_esb_povodok_next(p, pkt);
		bool lost = lose && ++*n % lose == 0;

		if (len) {
			if (!lost) {
				int r = smp_esb_belka_take(b, p->number, pkt, len);

				smp_esb_povodok_acked(p);
				if (r == 1) {
					CHECK(b->rx.len == sizeof(request) &&
					      memcmp(b->rx.frame, request, sizeof(request)) == 0);
					CHECK(smp_esb_belka_answer(b, answer, sizeof(answer)) == 0);
				}
			}
			continue;
		}
		len = smp_esb_belka_next(b, p->number, pkt);
		if (len && !lost) {
			int r = smp_esb_povodok_take(p, b->number, pkt, len, got, got_len, token);

			smp_esb_belka_acked(b);
			if (r == 1) {
				return 1;
			}
		}
	}
	return 0;
}

static int air(uint32_t period, unsigned lose, unsigned *n, struct smp_esb_belka *b,
	       const uint8_t **got, size_t *got_len, uint32_t *token)
{
	return air_from(&pv, period, lose, n, b, got, got_len, token);
}

int main(void)
{
	const uint8_t *got = NULL;
	size_t got_len = 0;
	uint32_t token = 0, period = 1000;
	unsigned n = 0, periods;

	for (size_t i = 0; i < sizeof(request); i++) {
		request[i] = (uint8_t)(i * 3);
	}
	for (size_t i = 0; i < sizeof(answer); i++) {
		answer[i] = (uint8_t)(i * 7 + 1);
	}
	smp_esb_povodok_init(&pv, KEY, ADDRESS, 0);
	smp_esb_belka_init(&b5, KEY, ADDRESS, 5);
	smp_esb_belka_init(&b6, KEY, ADDRESS, 6);

	/* 600 B out in 3 fragments, 2 000 B back in 9: 12 packets, one period */
	CHECK(smp_esb_povodok_queue(&pv, 5, 0x00030001u, request, sizeof(request), period) == 0);
	CHECK(air(period, 0, &n, &b5, &got, &got_len, &token) == 1);
	CHECK(got_len == sizeof(answer) && memcmp(got, answer, got_len) == 0 && token == 0x00030001u);
	OK("a request out and its answer back in one period, 12 packets");

	/* every fifth packet lost: each fragment sent again until acknowledged, a period or two */
	CHECK(smp_esb_povodok_queue(&pv, 5, 7, request, sizeof(request), ++period) == 0);
	for (periods = 0; periods < 10 && air(period++, 5, &n, &b5, &got, &got_len, &token) != 1;
	     periods++) {
	}
	CHECK(periods < 3 && token == 7 && got_len == sizeof(answer));
	OK("a fifth of the packets lost: sent again, the answer whole within two periods");

	/* a stranger's fragment for the Belka awaited is dropped by its MIC; another Belka's
	 * answer while 5 is asked is not taken */
	{
		uint8_t pkt[ESB_PAYLOAD_MAX];
		size_t len;

		CHECK(smp_esb_povodok_queue(&pv, 5, 8, request, sizeof(request), period) == 0);
		CHECK(smp_esb_povodok_peer(&pv, period) == 5);
		while ((len = smp_esb_povodok_next(&pv, pkt)) > 0) {
			CHECK(smp_esb_belka_take(&b5, 0, pkt, len) >= 0);
			smp_esb_povodok_acked(&pv);
		}
		CHECK(smp_esb_belka_answer(&b6, answer, 100) == 0);
		len = smp_esb_belka_next(&b6, 0, pkt);
		CHECK(smp_esb_povodok_take(&pv, 6, pkt, len, &got, &got_len, &token) == -1);
		CHECK(smp_esb_belka_answer(&b5, answer, 100) == 0);
		len = smp_esb_belka_next(&b5, 0, pkt);
		pkt[len - 1] ^= 1;                              /* its MIC spoilt */
		CHECK(smp_esb_povodok_take(&pv, 5, pkt, len, &got, &got_len, &token) == -1);
		pkt[len - 1] ^= 1;
		CHECK(smp_esb_povodok_take(&pv, 5, pkt, len, &got, &got_len, &token) == 1 &&
		      token == 8 && got_len == 100);
	}
	OK("another Belka's answer and a spoilt MIC not taken");

	/* another Povodok to the same Belka, its count its own and far below the first's: taken, and
	 * the first's again after it */
	smp_esb_povodok_init(&pv1, KEY, ADDRESS, 1);
	CHECK(b5.rx_last[0] > 0 || b5.rx.last_count > 0);
	CHECK(smp_esb_povodok_queue(&pv1, 5, 21, request, sizeof(request), period) == 0);
	CHECK(air_from(&pv1, period++, 0, &n, &b5, &got, &got_len, &token) == 1 && token == 21 &&
	      got_len == sizeof(answer) && memcmp(got, answer, sizeof(answer)) == 0);
	CHECK(smp_esb_povodok_queue(&pv, 5, 22, request, sizeof(request), period) == 0);
	CHECK(air(period++, 0, &n, &b5, &got, &got_len, &token) == 1 && token == 22);
	OK("two Povodoks to one Belka, each with its own count");

	/* left unanswered past the bridge's 3 s, a request is let go; the queue holds four */
	CHECK(smp_esb_povodok_queue(&pv, 6, 9, request, sizeof(request), period) == 0);
	CHECK(smp_esb_povodok_peer(&pv, period) == 6);
	CHECK(smp_esb_povodok_peer(&pv, period + SMP_ESB_HOLD_PERIODS) == 0 && pv.dropped == 1);
	for (unsigned i = 0; i < SMP_ESB_QUEUE; i++) {
		CHECK(smp_esb_povodok_queue(&pv, 5, i, request, 10, period) == 0);
	}
	CHECK(smp_esb_povodok_queue(&pv, 5, 99, request, 10, period) == -1);
	CHECK(smp_esb_povodok_queue(&pv, 5, 99, request, SMP_FRAME_MAX + 1, period) == -1);
	OK("a request let go after 3 s; four held, a fifth refused");
	printf("SMP ESB OK\n");
	return 0;
}
