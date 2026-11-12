/* The whole set on the host: 32 Belkas and 4 Povodoks round an animal, with the radio's losses
 * drawn at random, period by period for some three minutes — the logic of belka_link.c and
 * povodok_link.c against the pages (PRINCIPLE.md, The inside link; FIRMWARE.md, A session). Each
 * Povodok hears the Belkas on its side well, the next side's less, the far ones poorly, and the
 * Povodoks hear each other in the open. What is checked: the first after a second's listening and
 * the others' repeats; every Belka's record from the period after the SYNC; the gathering — what
 * the first holds each period is exactly what it heard plus what reached it in the PASSes, the
 * windows' room counted; the items once a minute, with the live view on and off; the move to the
 * quietest channel when the margin falls, every unit on the new channel, a Belka deaf during the
 * move finding the set on the spare; the takeover when the first dies and the yield when it comes
 * back, the set's time unbroken; the pause after ten seconds of silence and the record taken up;
 * a Belka deaf through two moves finding the set channel by channel; the search with a collision
 * resolved by the next attempt, the found Belkas taken into the set; the stop. */
#include <string.h>
#include "belka_link.h"
#include "povodok_link.h"
#include "live_msg.h"
#include "smp_esb.h"
#include "fetch.h"
#include "fetch_belka.h"
#include "test.h"

#ifdef WORLD_SIM                /* the emulator, tools/world.py: the set as large as asked */
#define NB STRELKA_BELKAS_MAX
static unsigned set_belkas = 32;
#define SET_BELKAS set_belkas
#else
#define NB 37                   /* 32 in the set; 33–37 new, taken during the search */
#define SET_BELKAS 32
#endif
#define NP STRELKA_POVODOKS_MAX
#define CHANNEL0 7
#define ADDRESS 0xA1B2C3D4u

struct world {
	struct belka_link bl[NB + 1];
	struct povodok_link pv[NP];
	struct belka_facts facts[NB + 1];
	uint8_t b_listen[NB + 1], p_listen[NP];
	bool b_on[NB + 1], p_on[NP];
	uint8_t hear_pb[NP][NB + 1];    /* percent, the same both ways */
	uint8_t hear_pp[NP][NP];
	int8_t rssi_pb[NP][NB + 1];
	int8_t noise[STRELKA_CHANNELS];
	bool dead;                      /* the radio silent: nothing reaches anyone */
	uint64_t rng;
	uint32_t period;
	/* this period, for the gathering's oracle */
	bool sent[NB + 1];
	uint8_t sent_code;              /* the live view's code of every LIVE sent, or 0xFF mixed */
	bool heard_by[NP][NB + 1];
	bool miss_reached[NP];
	bool pass_reached[NP][NP];
	int first_j;                    /* whose MISS went out; -1 none, -2 more than one */
	unsigned lives_sent, syncs_sent, heres, here_collisions, pass_overflow;
	struct belka_period_out bo[NB + 1];
	struct povodok_period_out po[NP];
	struct belka_live_out lv[NB + 1];
	struct povodok_sync_out so[NP];
	/* the record, as the Belkas report it */
	bool recording[NB + 1], paused[NB + 1];
	unsigned starts, stops, pauses, resumes;
	uint32_t first_period[NB + 1];
	unsigned gathered_checks;
	unsigned passes_sent, net_checks;
	/* what the emulator tells, per Belka: periods its LIVE went out, reached the first itself,
	 * reached it by a PASS, and held by it at the period's end */
	unsigned st_sent[NB + 1], st_direct[NB + 1], st_passed[NB + 1], st_held[NB + 1];
	/* SMP over ESB (common/smp_esb.c): the first asking the numbers heard and not taken (H4);
	 * the download through every Povodok joined to the phone, each given by 64/21 the same
	 * Belkas and going round them, those that answer on its channel its own (common/fetch.c) —
	 * while smp_on, the set neither recording nor showing its live view */
	bool smp_on;
	struct smp_esb_povodok sp[NP];
	struct smp_esb_belka sb[NB + 1];
	struct fetch fe[NP];
	bool probing[NB + 1];
	uint8_t probe_misses[NB + 1];
	uint32_t probe_first[NB + 1];
	uint8_t asking[NP];             /* the Belka each Povodok's request is for, 0 none */
	uint8_t fails[NP];              /* the attempts lost running: ESB gives up at 4 */
	uint32_t file_len[NB + 1];      /* each Belka's file of the session */
	unsigned admitted, smp_packets, smp_exchanges;
	uint32_t smp_credit_us[NP];     /* the air's time on ESB not yet a whole packet */
	uint64_t smp_bytes;
};

static const uint8_t SET_KEY[KEY_BYTES] = { 7, 7, 7, 7, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static bool world_sleep;            /* the Belkas allowed to sleep: --sleep */

static struct world W;

static uint64_t next_rng(struct world *w)
{
	w->rng ^= w->rng >> 12;
	w->rng ^= w->rng << 25;
	w->rng ^= w->rng >> 27;
	return w->rng * 0x2545F4914F6CDD1Dull;
}

static bool chance(struct world *w, unsigned pct)
{
	return !w->dead && (unsigned)(next_rng(w) >> 33) % 100 < pct;
}

static int8_t jitter(struct world *w, int8_t noise)
{
	return (int8_t)(noise + (int)((next_rng(w) >> 33) % 3) - 1);
}

static void povodok_sends(struct world *w, int j, const struct frame *f, bool reached[NP])
{
	uint8_t ch = w->p_listen[j];

	for (unsigned k = 1; k <= NB; k++) {
		if (w->b_on[k] && w->b_listen[k] == ch && !w->bl[k].asleep && !w->bl[k].esb &&
		    chance(w, w->hear_pb[j][k])) {
			belka_link_heard(&w->bl[k], f, w->rssi_pb[j][k]);
		}
	}
	for (int j2 = 0; j2 < (int)NP; j2++) {
		bool r = j2 != j && w->p_on[j2] && w->p_listen[j2] == ch &&
			 (!w->pv[j2].esb || w->pv[j2].esb_home) && chance(w, w->hear_pp[j][j2]);

		if (reached) {
			reached[j2] = r;
		}
		if (r) {
			povodok_link_heard(&w->pv[j2], f, -50);
		}
	}
}

static void pass_window(struct world *w, int j)
{
	struct povodok_pass_item items[STRELKA_BELKAS_MAX];
	unsigned n = povodok_link_pass(&w->pv[j], items, STRELKA_BELKAS_MAX), at = 0;
	struct strelka_packet p;
	struct frame f;

	for (int j2 = 0; j2 < (int)NP; j2++) {
		w->pass_reached[j][j2] = j2 != j && w->p_on[j2] &&
					 w->p_listen[j2] == w->p_listen[j] &&
					 chance(w, w->hear_pp[j][j2]);
	}
	for (unsigned packet = 0; packet < 2 && at < n; packet++) {
		frame_pass_begin(&p, (uint8_t)j, w->pv[j].period, 0);
		while (at < n && frame_pass_add(&p, items[at].belka, items[at].s0, items[at].state,
						items[at].steps)) {
			at++;
		}
		CHECK(frame_read(&p, frame_ram_len(&p), &f) && f.kind == FRAME_PASS);
		w->passes_sent++;
		for (int j2 = 0; j2 < (int)NP; j2++) {
			if (w->pass_reached[j][j2]) {
				povodok_link_heard(&w->pv[j2], &f, -50);
			}
		}
	}
	if (at < n) {
		w->pass_overflow += n - at;
	}
}

/* what the first must hold this period: what it heard, and what the others heard of the missed
 * and passed in their windows — each window the first `cap` by number of what the MISS names,
 * heard there and not passed in an earlier window it heard */
static void check_gathered(struct world *w)
{
	int j0 = w->first_j;
	bool passes[NP][NB + 1], expected[NB + 1], got[NB + 1];
	uint8_t numbers[STRELKA_BELKAS_MAX];
	unsigned cap, n;

	if (j0 < 0 || w->sent_code == 0xFF) {
		return;
	}
	cap = 2 * (STRELKA_BODY_MAX / (PASS_EACH_BYTES + live_body_bytes(w->sent_code)));
	memset(passes, 0, sizeof(passes));
	for (int j = 0; j < (int)NP; j++) {
		unsigned taken_here = 0;

		if (j == j0 || !w->p_on[j] || !w->miss_reached[j]) {
			continue;
		}
		for (unsigned k = 1; k <= NB && taken_here < cap; k++) {
			bool earlier = false;

			if (!w->pv[j0].belka[k].taken || w->heard_by[j0][k] || !w->heard_by[j][k]) {
				continue;
			}
			for (int j1 = 0; j1 < j; j1++) {
				if (passes[j1][k] && w->pass_reached[j1][j]) {
					earlier = true;
				}
			}
			if (!earlier) {
				passes[j][k] = true;
				taken_here++;
			}
		}
	}
	for (unsigned k = 1; k <= NB; k++) {
		expected[k] = w->pv[j0].belka[k].taken && w->heard_by[j0][k];
		for (int j = 0; j < (int)NP; j++) {
			if (passes[j][k] && w->pass_reached[j][j0]) {
				expected[k] = true;
			}
		}
	}
	memset(got, 0, sizeof(got));
	n = povodok_link_gathered(&w->pv[j0], numbers);
	for (unsigned i = 0; i < n; i++) {
		got[numbers[i]] = true;
	}
	for (unsigned k = 1; k <= NB; k++) {
		w->st_sent[k] += w->sent[k];
		w->st_direct[k] += w->sent[k] && w->heard_by[j0][k];
		w->st_passed[k] += got[k] && !w->heard_by[j0][k];
		w->st_held[k] += got[k];
		if (got[k] != expected[k]) {
			printf("period %u: Belka %u %s by the first, %s expected\n", w->period, k,
			       got[k] ? "held" : "not held", expected[k] ? "but" : "none");

			CHECK(got[k] == expected[k]);
		}
		if (got[k]) {
			CHECK(w->pv[j0].belka[k].steps_len == live_body_bytes(w->sent_code) &&
			      (w->pv[j0].belka[k].steps_len == 0 ||
			       w->pv[j0].belka[k].steps[0] == k));
		}
	}
	w->gathered_checks++;
}

/* gathered by the network: no MISS went out, and every Povodok's message holds exactly what it
 * heard itself, each entry under its own number, 0 the first's */
static void check_net(struct world *w)
{
	if (!w->pv[0].net) {
		return;
	}
	CHECK(w->first_j == -1);
	for (int j = 0; j < (int)NP; j++) {
		struct live_msg msgs[LIVE_MSG_MESSAGES];
		bool got[NB + 1];
		unsigned n;

		if (!w->p_on[j]) {
			continue;
		}
		memset(got, 0, sizeof(got));
		n = live_msg_from_link(&w->pv[j], 1, msgs);
		for (unsigned m = 0; m < n; m++) {
			struct live_msg_head h;
			struct live_msg_entry e;
			const uint8_t *steps;
			size_t steps_len, at = sizeof(h);

			CHECK(live_msg_read_head(msgs[m].bytes, msgs[m].len, &h));
			while (live_msg_next(msgs[m].bytes, msgs[m].len, &at, &e, &steps, &steps_len)) {
				CHECK(e.belka >= 1 && e.belka <= NB && !got[e.belka]);
				CHECK(e.by == (w->pv[j].first ? 0 : j));
				got[e.belka] = true;
			}
		}
		for (unsigned k = 1; k <= NB; k++) {
			CHECK(got[k] == (w->pv[j].belka[k].taken && w->heard_by[j][k]));
		}
	}
	w->net_checks++;
}

/* a Belka's MCUmgr, as far as the emulator needs it: 'P' asks 64/0, answered in 80 B; 'D' a
 * piece of the record, answered with as much of it as is left, up to a frame's 2 048 B */
static void belka_answers(struct world *w, unsigned k)
{
	static uint8_t frame[SMP_FRAME_MAX], data[SMP_FRAME_MAX];
	char name[FETCH_NAME_MAX];
	uint32_t off;
	uint8_t seq;
	size_t n = 80;

	if (fetch_belka_request(w->sb[k].rx.frame, w->sb[k].rx.len, name, sizeof(name), &off,
				&seq)) {
		uint32_t left = off < w->file_len[k] ? w->file_len[k] - off : 0;
		size_t chunk = left < FETCH_BELKA_DATA(SMP_FRAME_MAX) ? left :
								      FETCH_BELKA_DATA(SMP_FRAME_MAX);

		memset(data, (int)k, chunk);
		n = fetch_belka_answer(frame, sizeof(frame), seq, 0, off, data, chunk, w->file_len[k]);
		CHECK(n > 0);
	} else {
		memset(frame, (int)k, n);   /* the 64/0 asked of a number heard (H4) */
	}
	CHECK(smp_esb_belka_answer(&w->sb[k], frame, n) == 0);
}

/* the requests, one a Povodok at a time: each number heard and not taken asked by the first for
 * 64/0 (H4); then the download's next, as each Povodok goes round its Belkas (fetch_next) */
static void world_smp_queue(struct world *w)
{
	static uint8_t frame[SMP_FRAME_MAX];
	uint8_t unknown[STRELKA_BELKAS_MAX], req[16];
	unsigned n = povodok_link_unknown(&w->pv[0], unknown);

	for (unsigned i = 0; i < n; i++) {
		w->probing[unknown[i]] = true;
	}
	for (int j = 0; j < (int)NP; j++) {
		uint8_t k;
		size_t len;

		if (w->asking[j] && w->sp[j].qn == 0) {   /* let go unanswered */
			uint8_t k = w->asking[j];

			if (j == 0 && w->probing[k]) {
				/* as the download's round: given up after three and two seconds, asked
				 * again once heard again */
				if (w->probe_misses[k] == 0) {
					w->probe_first[k] = w->period;
				}
				if (w->probe_misses[k] < UINT8_MAX) {
					w->probe_misses[k]++;
				}
				if (w->probe_misses[k] >= FETCH_TRIES &&
				    w->period - w->probe_first[k] >= FETCH_ELSEWHERE_PERIODS) {
					w->probing[k] = false;
					w->probe_misses[k] = 0;
				}
			}
			fetch_let_go(&w->fe[j], k, w->period);
			w->asking[j] = 0;
		}
		if (!w->p_on[j] || w->asking[j]) {
			continue;
		}
		for (unsigned i = 1; j == 0 && i <= NB; i++) {
			if (w->probing[i]) {
				memset(req, 0, sizeof(req));
				req[0] = 'P';
				CHECK(smp_esb_povodok_queue(&w->sp[0], (uint8_t)i, i, req, sizeof(req),
							    w->period) == 0);
				w->asking[0] = (uint8_t)i;
				break;
			}
		}
		if (w->asking[j] || !w->fe[j].on) {
			continue;
		}
		len = fetch_next(&w->fe[j], &k, frame, sizeof(frame));
		if (len) {
			CHECK(smp_esb_povodok_queue(&w->sp[j], k, FETCH_TOKEN | k, frame, len,
						    w->period) == 0);
			w->asking[j] = k;
		}
	}
}

/* the channel a Povodok is heard on: its own on ESB, the set's back from it */
static uint8_t esb_channel_now(const struct world *w, int j)
{
	return w->pv[j].esb_home ? w->pv[j].cfg.channel : w->pv[j].esb_channel;
}

/* the period's SMP over ESB: a request waiting gives the set over to ESB after two of the state's
 * periods (povodok_link_awake); then every Povodok on ESB on its own channel, the packets one after
 * another, each delivered as that Povodok hears the Belka there, sent again until it is — each
 * unit kept on ESB by the packets it takes */
static void world_smp(struct world *w)
{
	uint8_t pkt[ESB_PAYLOAD_MAX];

	if (!w->smp_on) {
		return;
	}
	world_smp_queue(w);
	for (int j = 0; j < (int)NP; j++) {
		uint8_t ch = esb_channel_now(w, j);
		unsigned left;

		if (!w->p_on[j] || !w->pv[j].esb || w->pv[j].esb_home) {
			continue;
		}
		w->smp_credit_us[j] += STRELKA_PERIOD_US;
		left = w->smp_credit_us[j] / SMP_ESB_PACKET_US;
		w->smp_credit_us[j] -= left * SMP_ESB_PACKET_US;
		while (left) {
			uint8_t k;
			size_t len;
			bool ok;
			const uint8_t *got;
			size_t got_len;
			uint32_t token;

			if (w->sp[j].qn == 0) {
				world_smp_queue(w);      /* the next request at once */
			}
			k = smp_esb_povodok_peer(&w->sp[j], w->period);
			if (k == 0) {
				break;
			}
			left--;
			w->smp_packets++;
			ok = w->b_on[k] && w->bl[k].esb && w->bl[k].esb_channel == ch &&
			     chance(w, w->hear_pb[j][k]);
			if (ok) {
				belka_link_spoken(&w->bl[k]);   /* a fragment, or the ack of hers */
				w->fails[j] = 0;
			} else if (++w->fails[j] >= 4) {
				w->fails[j] = 0;         /* ESB's repeats spent: given up */
				if (!smp_esb_povodok_reached(&w->sp[j]) &&
				    !fetch_here(&w->fe[j], k)) {
					smp_esb_povodok_failed(&w->sp[j]);   /* not on its channel */
					continue;
				}
			}
			len = smp_esb_povodok_next(&w->sp[j], pkt);
			if (len) {
				if (ok) {
					int r = smp_esb_belka_take(&w->sb[k], (uint8_t)j, pkt, len);

					smp_esb_povodok_acked(&w->sp[j]);
					if (r == 1) {
						belka_answers(w, k);
					}
				}
				continue;
			}
			len = smp_esb_belka_next(&w->sb[k], (uint8_t)j, pkt);
			if (len == 0) {
				w->smp_packets--;
				break;           /* nothing on either side: wait */
			}
			if (!ok) {
				continue;        /* lost: sent again, as ESB repeats it */
			}
			smp_esb_belka_acked(&w->sb[k]);
			if (smp_esb_povodok_take(&w->sp[j], k, pkt, len, &got, &got_len, &token) == 1) {
				w->smp_exchanges++;
				w->asking[j] = 0;
				if (token & FETCH_TOKEN) {
					uint32_t off;
					const uint8_t *data;
					size_t data_len;

					CHECK(token == (FETCH_TOKEN | k));
					if (fetch_answer(&w->fe[j], k, got, got_len, &off, &data,
							 &data_len) == 1) {
						CHECK(data_len == 0 || data[0] == k);
						w->smp_bytes += data_len;
					}
					continue;
				}
				CHECK(token == k && got[0] == k);
				if (w->probing[k]) {   /* its answer passed the MIC: taken (H4) */
					w->probing[k] = false;
					w->probe_misses[k] = 0;
					for (int j2 = 0; j2 < (int)NP; j2++) {
						povodok_link_take(&w->pv[j2], k, 0x100u + k);
					}
					w->admitted++;
				}
			}
		}
	}
	world_smp_queue(w);
	for (int j = 0; j < (int)NP; j++) {
		if (w->p_on[j]) {
			povodok_link_awake(&w->pv[j], w->sp[j].qn != 0);   /* the set on ESB */
		}
	}
}

static void world_period(struct world *w)
{
	struct frame f;
	uint8_t sig[SYNC_BODY_BYTES] = { 0 }, missed[MISS_BODY_BYTES], body[LIVE_BODY_MAX];
	int16_t win[NB + 1];

	memset(w->sent, 0, sizeof(w->sent));
	memset(w->heard_by, 0, sizeof(w->heard_by));
	memset(w->miss_reached, 0, sizeof(w->miss_reached));
	memset(w->pass_reached, 0, sizeof(w->pass_reached));
	w->first_j = -1;
	w->sent_code = 0;
	w->lives_sent = 0;
	w->syncs_sent = 0;

	/* the SYNCs' slots, a CALL from a searching Povodok */
	for (int j = 0; j < (int)NP; j++) {
		if (!w->p_on[j]) {
			continue;
		}
		povodok_link_period_begin(&w->pv[j], &w->so[j]);
		if (w->so[j].send) {
			f = (struct frame){ .kind = w->so[j].call ? FRAME_CALL : FRAME_SYNC,
					    .root = w->so[j].root,
					    .sender = WHO_POVODOK, .number = (uint8_t)j,
					    .s0 = w->so[j].s0, .period = w->so[j].period,
					    .state = w->so[j].state, .body = sig,
					    .body_len = SYNC_BODY_BYTES };
			povodok_sends(w, j, &f, NULL);
			w->syncs_sent++;
		}
	}
	/* the Belkas' slots */
	for (unsigned k = 1; k <= NB; k++) {
		if (!w->b_on[k]) {
			continue;
		}
		belka_link_slot(&w->bl[k], &w->facts[k], &w->lv[k]);
		if (!w->lv[k].send) {
			continue;
		}
		CHECK(w->bl[k].cfg.number == k);
		w->sent[k] = true;
		w->lives_sent++;
		if (w->lives_sent == 1) {
			w->sent_code = S0_HIGH(w->lv[k].s0);
		} else if (w->sent_code != S0_HIGH(w->lv[k].s0)) {
			w->sent_code = 0xFF;
		}
		memset(body, (int)k, sizeof(body));
		f = (struct frame){ .kind = FRAME_LIVE, .sender = WHO_BELKA, .number = (uint8_t)k,
				    .s0 = w->lv[k].s0, .period = w->bl[k].period,
				    .state = w->lv[k].state, .body = body,
				    .body_len = live_body_bytes(S0_HIGH(w->lv[k].s0)) };
		for (int j = 0; j < (int)NP; j++) {
			if (w->p_on[j] && w->p_listen[j] == w->b_listen[k] && !w->pv[j].esb &&
			    chance(w, w->hear_pb[j][k])) {
				w->heard_by[j][k] = true;
				povodok_link_heard(&w->pv[j], &f, w->rssi_pb[j][k]);
			}
		}
	}
	/* the MISS */
	for (int j = 0; j < (int)NP; j++) {
		if (w->p_on[j] && povodok_link_miss(&w->pv[j], missed)) {
			bool reached[NP];

			w->first_j = w->first_j == -1 ? j : -2;
			f = (struct frame){ .kind = FRAME_MISS, .sender = WHO_POVODOK,
					    .number = (uint8_t)j, .s0 = S0(S0_MISS, 0),
					    .period = w->pv[j].period, .body = missed,
					    .body_len = MISS_BODY_BYTES };
			povodok_sends(w, j, &f, reached);
			for (int j2 = 0; j2 < (int)NP; j2++) {
				w->miss_reached[j2] = reached[j2];
			}
		}
	}
	/* the PASS windows, by the numbers */
	for (int j = 0; j < (int)NP; j++) {
		if (w->p_on[j]) {
			pass_window(w, j);
		}
	}
	/* the HEREs of new Belkas, two in one window colliding */
	for (unsigned k = 1; k <= NB; k++) {
		win[k] = w->b_on[k] ? belka_link_here(&w->bl[k]) : -1;
	}
	for (unsigned k = 1; k <= NB; k++) {
		unsigned same = 0;
		uint8_t chip[HERE_BODY_BYTES];

		if (win[k] < 0) {
			continue;
		}
		for (unsigned k2 = 1; k2 <= NB; k2++) {
			same += win[k2] == win[k];
		}
		if (same > 1) {
			w->here_collisions++;
			continue;
		}
		w->heres++;
		for (unsigned i = 0; i < HERE_BODY_BYTES; i++) {
			chip[i] = (uint8_t)(w->bl[k].cfg.chip >> (8 * i));
		}
		f = (struct frame){ .kind = FRAME_HERE, .sender = WHO_BELKA, .number = 0,
				    .s0 = S0(S0_HERE, 0), .period = w->bl[k].period, .body = chip,
				    .body_len = HERE_BODY_BYTES };
		for (int j = 0; j < (int)NP; j++) {
			if (w->p_on[j] && w->p_listen[j] == STRELKA_SEARCH_CHANNEL &&
			    chance(w, w->hear_pb[j][k])) {
				povodok_link_heard(&w->pv[j], &f, w->rssi_pb[j][k]);
			}
		}
	}
	/* the gathering, held against what went over the air */
	check_gathered(w);
	check_net(w);
	world_smp(w);
	/* the measurements and the periods' ends */
	for (unsigned k = 1; k <= NB; k++) {
		if (!w->b_on[k]) {
			continue;
		}
		belka_link_measured(&w->bl[k], jitter(w, w->noise[w->bl[k].measure_channel]));
		belka_link_period_end(&w->bl[k], &w->facts[k], &w->bo[k]);
		w->b_listen[k] = w->bo[k].listen_channel;
		if (w->bo[k].record_start) {
			CHECK(!w->recording[k]);
			w->recording[k] = true;
			w->first_period[k] = w->bo[k].first_period;
			w->starts++;
		}
		if (w->bo[k].record_stop) {
			CHECK(w->recording[k] && w->bo[k].stop_why == END_SYNC_STOP);
			w->recording[k] = false;
			w->paused[k] = false;
			w->stops++;
		}
		if (w->bo[k].pause) {
			CHECK(w->recording[k] && !w->paused[k]);
			w->paused[k] = true;
			w->pauses++;
		}
		if (w->bo[k].resume) {
			CHECK(w->recording[k] && w->paused[k]);
			w->paused[k] = false;
			w->resumes++;
		}
	}
	for (int j = 0; j < (int)NP; j++) {
		if (!w->p_on[j]) {
			continue;
		}
		povodok_link_period_end(&w->pv[j], jitter(w, w->noise[w->pv[j].measure_next]),
					&w->po[j]);
		w->p_listen[j] = w->po[j].listen_channel;
	}
	w->period++;
}

static void run(struct world *w, unsigned periods)
{
	for (unsigned i = 0; i < periods; i++) {
		world_period(w);
	}
}

__attribute__((unused)) static unsigned count_first(const struct world *w)
{
	unsigned n = 0;

	for (int j = 0; j < (int)NP; j++) {
		n += w->p_on[j] && w->pv[j].first;
	}
	return n;
}

static unsigned belkas_in_state(const struct world *w, uint8_t state)
{
	unsigned n = 0;

	for (unsigned k = 1; k <= SET_BELKAS; k++) {
		n += w->bo[k].state == state;
	}
	return n;
}

__attribute__((unused)) static unsigned belkas_on_channel(const struct world *w, uint8_t ch)
{
	unsigned n = 0;

	for (unsigned k = 1; k <= SET_BELKAS; k++) {
		n += w->b_listen[k] == ch;
	}
	return n;
}

static void povodok_boot(struct world *w, int j, uint8_t channel)
{
	struct povodok_link_cfg cfg = { .number = (uint8_t)j, .address = ADDRESS,
					.channel = channel };

	povodok_link_init(&w->pv[j], &cfg);
	for (unsigned k = 1; k <= SET_BELKAS; k++) {
		povodok_link_take(&w->pv[j], (uint8_t)k, 0x100u + k);
	}
	w->p_on[j] = true;
	w->p_listen[j] = channel;
}

static void world_init(struct world *w)
{
	memset(w, 0, sizeof(*w));
	w->rng = 0x9E3779B97F4A7C15ull;
	for (unsigned ch = 0; ch < STRELKA_CHANNELS; ch++) {
		w->noise[ch] = (int8_t)(-95 + (int)(ch * 7 % 11));
	}
	w->noise[23] = -100;                     /* the quietest channel */
	for (int j = 0; j < (int)NP; j++) {
		smp_esb_povodok_init(&w->sp[j], SET_KEY, ADDRESS, (uint8_t)j);
	}
	for (unsigned k = 1; k <= NB; k++) {
		smp_esb_belka_init(&w->sb[k], SET_KEY, ADDRESS, (uint8_t)k);
	}
	for (int j = 0; j < (int)NP; j++) {
		for (unsigned k = 1; k <= NB; k++) {
			unsigned side = k % NP, away = ((unsigned)j + NP - side) % NP;

			w->hear_pb[j][k] = (uint8_t)(away == 0 ? 95 : away == 1 ? 70 : 30);
			w->rssi_pb[j][k] = (int8_t)(-55 - 5 * (int)away - (int)(k % 3));
		}
		for (int j2 = 0; j2 < (int)NP; j2++) {
			w->hear_pp[j][j2] = 95;
		}
		povodok_boot(w, j, CHANNEL0);
	}
	for (unsigned k = 1; k <= NB; k++) {
		struct belka_link_cfg cfg = { .number = k <= SET_BELKAS ? (uint8_t)k : 0,
					      .chip = 0x100u + k, .address = ADDRESS,
					      .channel = CHANNEL0, .sleep = world_sleep };

		belka_link_init(&w->bl[k], &cfg);
		w->b_on[k] = k <= SET_BELKAS;
		w->b_listen[k] = k <= SET_BELKAS ? CHANNEL0 : STRELKA_SEARCH_CHANNEL;
		w->facts[k] = (struct belka_facts){ .cell_pct = (uint8_t)(50 + k), .temp_c = 20,
						    .cell_mv = (uint16_t)(3200 + k),
						    .card_free_pct = 90, .offset_ns = (int16_t)(3 * k),
						    .acc_out = (uint8_t)k };
	}
}

#ifndef WORLD_SIM
int main(void)
{
	struct world *w = &W;
	uint32_t at;

	world_init(w);

	/* three seconds' listening at rest — all four switched on in one period, so all four stand
	 * up at once and three yield on the first SYNC; then, at rest, Povodok 0's SYNC and the
	 * others' in the state's period alone */
	run(w, LINK_TAKEOVER_REST_PERIODS);
	CHECK(w->syncs_sent == 0 && w->po[0].took_over && count_first(w) == NP && w->pv[0].first);
	run(w, 1);
	CHECK(w->syncs_sent == NP && w->so[0].send && w->so[0].period == 150 && count_first(w) == 1);
	run(w, 49);
	CHECK(w->syncs_sent == 0 && count_first(w) == 1);
	run(w, 1);
	CHECK(w->syncs_sent == NP && w->so[0].period == 200 && count_first(w) == 1);
	run(w, 200);                             /* four seconds more, a SYNC each */
	for (unsigned k = 1; k <= SET_BELKAS; k++) {
		CHECK(w->bl[k].period_known && w->bl[k].period == w->pv[0].period);
		CHECK(w->bo[k].state == BELKA_WAITING && w->b_listen[k] == CHANNEL0);
	}
	for (int j = 1; j < (int)NP; j++) {
		CHECK(!w->pv[j].first && w->pv[j].period == w->pv[0].period &&
		      w->pv[j].record == false && w->pv[j].spare == w->pv[0].spare);
	}
	CHECK(w->starts == 0 && w->period == 401);
	OK("the first after three seconds at rest, the SYNCs once a second, every Belka on its time");

	/* the start: every Belka records from the period after the SYNC it heard */
	povodok_link_record(&w->pv[0], true);
	run(w, 10);
	CHECK(w->starts == SET_BELKAS);
	for (unsigned k = 1; k <= SET_BELKAS; k++) {
		CHECK(w->recording[k] && w->first_period[k] >= 402 && w->first_period[k] <= 406);
	}
	CHECK(belkas_in_state(w, BELKA_RECORDING) >= SET_BELKAS - 1);
	OK("the record from the period after the start's SYNC, in every Belka");

	/* the live view: the gathering held against the air for a thousand periods */
	povodok_link_live(&w->pv[0], S0_QUAT_50);
	run(w, 100);
	CHECK(w->lives_sent == SET_BELKAS && w->sent_code == S0_QUAT_50);
	at = w->gathered_checks;
	run(w, 1000);
	CHECK(w->gathered_checks == at + 1000 && w->pass_overflow == 0);
	{
		uint8_t numbers[STRELKA_BELKAS_MAX];
		unsigned n = povodok_link_gathered(&w->pv[0], numbers);

		CHECK(n == 0);                   /* after the period's end, the next begins empty */
	}
	OK("the gathering, exactly what the air carried, a thousand periods");

	/* the items: once a minute in a row; Belka 1 hears Povodok 0 and is heard without fail */
	w->hear_pb[0][1] = 100;
	run(w, 3400 - w->period);
	for (unsigned item = ITEM_FIRST; item <= ITEM_LAST; item++) {
		CHECK(w->pv[0].belka[1].item_known[item]);
	}
	CHECK(w->pv[0].belka[1].item[ITEM_CELL_TEMP] == ITEM_BYTES(51, 20) &&
	      w->pv[0].belka[1].item[ITEM_CELL_MV] == 3201 &&
	      w->pv[0].belka[1].item[ITEM_CARD] == ITEM_BYTES(90, 0) &&
	      w->pv[0].belka[1].item[ITEM_SYNCS] == ITEM_BYTES(w->rssi_pb[0][1], 0) &&
	      w->pv[0].belka[1].item[ITEM_OFFSET] == 3 &&
	      w->pv[0].belka[1].item[ITEM_STATE] == ITEM_BYTES(BELKA_RECORDING, FAULT_NONE) &&
	      w->pv[0].belka[1].item[ITEM_OUT] == ITEM_BYTES(1, 0));
	{
		unsigned known = 0;

		for (unsigned k = 1; k <= SET_BELKAS; k++) {
			for (unsigned item = ITEM_FIRST; item <= ITEM_LAST; item++) {
				known += w->pv[0].belka[k].item_known[item];
				if (w->pv[0].belka[k].item_known[ITEM_CELL_MV]) {
					CHECK(w->pv[0].belka[k].item[ITEM_CELL_MV] == 3200 + k);
				}
			}
		}
		CHECK(known >= SET_BELKAS * ITEM_LAST * 9 / 10);
	}
	OK("every Belka's items once a minute, the values the Belkas' own");

	/* the move: the set's channel jammed, the margin falls, the first says the move twice and
	 * every unit is on the quietest channel; Belka 7 deaf through it finds the set on the spare */
	w->noise[CHANNEL0] = -66;
	{
		uint32_t decided = 0, moved = 0;
		uint8_t deaf[NP];

		for (unsigned i = 0; i < 200 && !moved; i++) {
			run(w, 1);
			if (!decided && w->pv[0].move_period) {
				decided = w->pv[0].move_period;   /* the first period on the new channel */
				for (int j = 0; j < (int)NP; j++) {
					deaf[j] = w->hear_pb[j][7];
					w->hear_pb[j][7] = 0;
				}
			}
			if (decided && w->period - 1 == decided - 2) {
				CHECK(SET_MOVE(w->so[0].state) == SET_MOVE_TWO &&
				      SET_CHANNEL(w->so[0].state) == 23 &&
				      SET_MOVE(w->so[3].state) == SET_MOVE_TWO);
			}
			if (decided && w->period - 1 == decided - 1) {
				CHECK(SET_MOVE(w->so[0].state) == SET_MOVE_NEXT &&
				      SET_MOVE(w->so[1].state) == SET_MOVE_NEXT);
			}
			if (w->po[0].moved) {
				moved = w->period - 1;
			}
		}
		CHECK(decided && moved == decided - 1 && w->pv[0].cfg.channel == 23);
		CHECK(decided < 3500 + 100);
		run(w, 3);
		CHECK(belkas_on_channel(w, 23) == SET_BELKAS - 1 && w->b_listen[7] != 23);
		for (int j = 0; j < (int)NP; j++) {
			CHECK(w->p_listen[j] == 23 && w->pv[j].cfg.channel == 23);
			w->hear_pb[j][7] = deaf[j];
		}
		run(w, 6);
		CHECK(w->bl[7].cfg.channel == 23 && w->b_listen[7] == 23);
		run(w, 3);
		CHECK(w->bo[7].state == BELKA_RECORDING && w->recording[7]);
		CHECK(w->pv[0].spare != 23 && w->pv[0].spare != STRELKA_SEARCH_CHANNEL);
	}
	run(w, 300);
	CHECK(w->pass_overflow == 0 && count_first(w) == 1);
	OK("the move said twice, every unit on the quietest channel, a deaf Belka finding it");

	/* the live view off: a LIVE once a second alone, the items' row in those periods */
	povodok_link_live(&w->pv[0], S0_LIVE_OFF);
	run(w, 3);
	memset(w->pv[0].belka[1].item_known, 0, sizeof(w->pv[0].belka[1].item_known));
	for (unsigned i = 0; i < 200; i++) {
		run(w, 1);
		if ((w->period - 1) % LINK_STATE_EVERY == 0) {
			CHECK(w->lives_sent == SET_BELKAS);
		} else {
			CHECK(w->lives_sent == 0);
		}
	}
	run(w, 6550 - w->period);              /* the row from 6150, the first up at 150 */
	for (unsigned item = ITEM_FIRST; item <= ITEM_LAST; item++) {
		CHECK(w->pv[0].belka[1].item_known[item]);
	}
	OK("the live view off: the state once a second, the items in those periods");

	/* raw steps at 100 Hz: the windows hold sixteen a Povodok, the oracle counts the room */
	povodok_link_live(&w->pv[0], S0_RAW_100);
	run(w, 3);
	at = w->gathered_checks;
	run(w, 200);
	CHECK(w->gathered_checks == at + 200);
	OK("raw steps at 100 Hz, the PASS windows' room counted");

	/* the first dies: Povodok 1 takes over after a second, the record goes on; the first comes
	 * back, listens a second on the set's time, takes over, and Povodok 1 yields */
	w->p_on[0] = false;
	{
		unsigned stops = w->stops;

		run(w, LINK_TAKEOVER_PERIODS - 1);
		/* on their own clocks; one that lost the last SYNC before is already looking */
		CHECK(count_first(w) == 0 && belkas_in_state(w, BELKA_RECORDING_ALONE) +
		      belkas_in_state(w, BELKA_RECORDING_SEARCHING) == SET_BELKAS &&
		      belkas_in_state(w, BELKA_RECORDING_SEARCHING) <= 2);
		/* silent five seconds, a first hidden a moment not being a first gone; then the
		 * three left stand up together, and the two higher yield on Povodok 1's first SYNC */
		run(w, LINK_TAKEOVER_LOST_PERIODS - LINK_TAKEOVER_PERIODS);
		CHECK(count_first(w) == 0 && w->pauses == 0);
		run(w, 1);
		CHECK(w->po[1].took_over && w->pv[1].first && count_first(w) == NP - 1);
		run(w, 1);
		CHECK(w->so[1].send && (w->so[1].state & SET_RECORD) &&
		      S0_HIGH(w->so[1].s0) == S0_RAW_100 && count_first(w) == 1 && w->pv[1].first);
		run(w, 10);
		CHECK(belkas_in_state(w, BELKA_RECORDING) >= SET_BELKAS - 1 && w->stops == stops &&
		      w->pauses == 0);
		at = w->gathered_checks;
		run(w, 300);
		CHECK(w->gathered_checks == at + 300 && w->first_j == 1);
		povodok_boot(w, 0, 23);          /* the channel it saved at the move */
		for (unsigned i = 0; i < LINK_TAKEOVER_PERIODS - 1; i++) {
			run(w, 1);                   /* listening its second on the set's time */
			CHECK(!w->so[0].send && w->pv[1].first && count_first(w) == 1);
		}
		CHECK(w->pv[0].period == w->pv[1].period);
		run(w, 1);
		CHECK(w->po[0].took_over && w->pv[0].first);
		run(w, 1);
		CHECK(w->so[0].send && w->so[1].send && w->so[0].period == w->so[1].period &&
		      (w->so[0].state & SET_RECORD) && w->po[1].yielded && count_first(w) == 1);
		for (unsigned k = 1; k <= SET_BELKAS; k++) {
			CHECK(w->bl[k].period == w->pv[0].period && w->recording[k]);
		}
		CHECK(w->stops == stops && w->pauses == 0);
	}
	OK("the takeover after a second, the yield, the set's time unbroken");

	/* the radio dead for eleven seconds: every Belka pauses at ten, every Povodok stands in;
	 * the radio back, one first again and every record taken up */
	w->dead = true;
	{
		uint32_t last_heard = w->period - 1;

		run(w, LINK_STOP_S_DEFAULT * LINK_PERIODS_A_S - 1);
		/* Belka 1 heard the last SYNC for certain; one that lost it is a period ahead */
		CHECK(!w->paused[1] && w->pauses <= 2 && belkas_in_state(w, BELKA_RECORDING_SEARCHING) +
		      belkas_in_state(w, BELKA_PAUSED) == SET_BELKAS);
		run(w, 1);
		CHECK(w->paused[1] && w->bo[1].pause &&
		      w->period - 1 == last_heard + LINK_STOP_S_DEFAULT * LINK_PERIODS_A_S);
		run(w, 3);
		CHECK(w->pauses == SET_BELKAS && belkas_in_state(w, BELKA_PAUSED) == SET_BELKAS);
		CHECK(count_first(w) == NP);
		run(w, 50);
		w->dead = false;
		run(w, 1);
		CHECK(w->syncs_sent == NP && w->so[0].period == w->so[3].period);
		run(w, 1);
		CHECK(count_first(w) == 1 && w->pv[0].first);
		run(w, 3);
		CHECK(w->resumes == SET_BELKAS && w->stops == 0 &&
		      belkas_in_state(w, BELKA_RECORDING) == SET_BELKAS);
		for (unsigned k = 1; k <= SET_BELKAS; k++) {
			CHECK(w->b_listen[k] == 23 && w->bl[k].period == w->pv[0].period);
		}
	}
	OK("the pause at ten seconds of silence, the record taken up by the first SYNC");

	/* Belka 9 deaf through two moves: the spare it knew is stale, so it listens a period on
	 * each channel and finds the set */
	{
		uint8_t deaf[NP], ch1, ch2;
		uint32_t moved = 0;
		unsigned over0 = w->pass_overflow;   /* raw steps' windows may have run out above */

		for (int j = 0; j < (int)NP; j++) {
			deaf[j] = w->hear_pb[j][9];
			w->hear_pb[j][9] = 0;
		}
		w->noise[CHANNEL0] = -90;
		w->noise[23] = -66;
		for (unsigned i = 0; i < 200 && !moved; i++) {
			run(w, 1);
			if (w->po[0].moved) {
				moved = w->period;
			}
		}
		ch1 = w->pv[0].cfg.channel;
		CHECK(moved && ch1 != 23 && w->noise[ch1] == -95);
		run(w, 20);
		w->noise[ch1] = -66;
		moved = 0;
		for (unsigned i = 0; i < 200 && !moved; i++) {
			run(w, 1);
			if (w->po[0].moved) {
				moved = w->period;
			}
		}
		ch2 = w->pv[0].cfg.channel;
		CHECK(moved && ch2 != ch1 && ch2 != 23);
		run(w, 10);
		CHECK(belkas_on_channel(w, ch2) == SET_BELKAS - 1 && w->b_listen[9] != ch2);
		CHECK(w->bo[9].state == BELKA_RECORDING_SEARCHING);
		for (int j = 0; j < (int)NP; j++) {
			w->hear_pb[j][9] = deaf[j];
		}
		/* every other period on the set's own, between each other channel in turn */
		for (unsigned i = 0; i < 2 * STRELKA_SET_CHANNELS + 2 && w->bl[9].cfg.channel != ch2;
		     i++) {
			run(w, 1);
		}
		CHECK(w->bl[9].cfg.channel == ch2 && w->recording[9] && !w->paused[9]);
		run(w, 3);
		CHECK(w->bo[9].state == BELKA_RECORDING && belkas_on_channel(w, ch2) == SET_BELKAS);
		run(w, 100);
		CHECK(w->pass_overflow == over0 && count_first(w) == 1);
	}
	OK("a Belka deaf through two moves finds the set channel by channel");

	/* the search: five new Belkas, two drawing the same number at attempt 0; Povodok 2 calls on
	 * the search's channel; all five found in two rounds and taken into the set */
	{
		uint32_t start;
		uint64_t c36 = w->bl[36].cfg.chip, c37;

		for (c37 = 0x9000; ; c37++) {
			if (search_draw(c37, 0) == search_draw(c36, 0) &&
			    search_draw(c37, 1) != search_draw(c36, 1)) {
				break;
			}
		}
		w->bl[37].cfg.chip = c37;
		for (unsigned k = 33; k <= 37; k++) {
			for (unsigned k2 = 33; k2 < k; k2++) {
				CHECK((search_draw(w->bl[k].cfg.chip, 0) ==
				       search_draw(w->bl[k2].cfg.chip, 0)) == (k == 37 && k2 == 36));
				CHECK(search_draw(w->bl[k].cfg.chip, 1) !=
				      search_draw(w->bl[k2].cfg.chip, 1));
			}
			w->b_on[k] = true;
			for (int j = 0; j < (int)NP; j++) {
				w->hear_pb[j][k] = 100;
			}
		}
		povodok_link_search(&w->pv[2], true);
		start = w->period;
		run(w, 1);
		CHECK(w->so[2].call && w->p_listen[2] == STRELKA_SEARCH_CHANNEL && count_first(w) == 1);
		run(w, SEARCH_PERIODS - 1);
		CHECK(w->pv[2].found_n == 3 && w->here_collisions == 2 && w->heres == 3);
		run(w, SEARCH_PERIODS);              /* the two that collided, apart at attempt 1 */
		CHECK(w->pv[2].found_n == 5 && w->here_collisions == 2 && w->heres == 8);
		for (unsigned i = 0; i < 5; i++) {
			uint64_t chip = w->pv[2].found[i].chip;
			uint32_t p = w->pv[2].found[i].period;
			uint8_t attempt = (uint8_t)((p - start) / SEARCH_PERIODS);

			CHECK(p % SEARCH_PERIODS == SEARCH_PERIOD(search_draw(chip, attempt)));
			CHECK(attempt == ((chip == c36 || chip == c37) ? 1 : 0));
		}
		CHECK(!w->pv[2].first && w->pv[0].first);
		povodok_link_search(&w->pv[2], false);
		for (unsigned k = 33; k <= 37; k++) {
			for (int j = 0; j < (int)NP; j++) {
				povodok_link_take(&w->pv[j], (uint8_t)k, w->bl[k].cfg.chip);
			}
			belka_link_join(&w->bl[k], (uint8_t)k, ADDRESS, w->pv[0].cfg.channel);
			w->b_listen[k] = w->pv[0].cfg.channel;
		}
		run(w, 10);
		CHECK(w->starts == SET_BELKAS + 5 && w->lives_sent == NB && w->p_listen[2] ==
		      w->pv[0].cfg.channel);
		at = w->gathered_checks;
		run(w, 100);
		CHECK(w->gathered_checks == at + 100);
	}
	OK("the search: a collision resolved by the next attempt, five Belkas taken into the set");

	/* the stop */
	povodok_link_record(&w->pv[0], false);
	run(w, 6);
	CHECK(w->stops == NB && belkas_in_state(w, BELKA_WAITING) == SET_BELKAS);
	for (unsigned k = 1; k <= NB; k++) {
		CHECK(!w->recording[k]);
	}
	run(w, 50);
	CHECK(w->stops == NB && w->starts == NB && w->lives_sent == NB);
	OK("the stop in every Belka, the LIVEs going on");

	/* the third way: gathered by the network, every Povodok on one 5 GHz network with the
	 * application — no MISS, no PASS, each Povodok's own message; then back to the radio's */
	{
		unsigned passes = w->passes_sent, nets = w->net_checks;

		for (int j = 0; j < (int)NP; j++) {
			povodok_link_gather(&w->pv[j], true);
		}
		run(w, 500);
		CHECK(w->net_checks == nets + 500 && w->passes_sent == passes);
		for (int j = 0; j < (int)NP; j++) {
			povodok_link_gather(&w->pv[j], false);
		}
		at = w->gathered_checks;
		run(w, 100);
		CHECK(w->gathered_checks == at + 100 && w->passes_sent > passes);
	}
	OK("gathered by the network: no MISS, no PASS, each Povodok's message what it heard; and back");

	printf("WORLD OK: %u periods, %u gatherings held against the air\n", w->period,
	       w->gathered_checks);
	return 0;
}
#endif

#ifdef WORLD_SIM
/* The emulator: the same world, sized and run as asked, telling what a set never shows — each
 * Belka's LIVEs sent, heard by the first itself, passed to it, held at the period's end; the PASS
 * windows' overflow; the first Povodok, its channel and margin, second by second.
 *
 *     world --belkas 63 --povodoks 4 --seconds 60 --live 5 --near 95 --side 70 --far 30 -v
 *     world --belkas 32 --cabled 8 --sleep --seconds 10 --download 200
 *
 * --cabled N: the set's last N Belkas given it by the cable, taken by no Povodok, asked over SMP
 * and taken once their answer passes the MIC (H4); --download KB: after the record, that much of
 * every Belka's record fetched over SMP, the pace told; --sleep: the Belkas asleep where they may
 * be, woken by a fragment in their state's period and kept awake 10 s. */
#include <stdlib.h>

static unsigned arg(int argc, char **argv, const char *name, unsigned dflt)
{
	for (int i = 1; i + 1 < argc; i++) {
		if (strcmp(argv[i], name) == 0) {
			return (unsigned)strtoul(argv[i + 1], NULL, 0);
		}
	}
	return dflt;
}

static bool flag(int argc, char **argv, const char *name)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], name) == 0) {
			return true;
		}
	}
	return false;
}

int main(int argc, char **argv)
{
	struct world *w = &W;
	unsigned povodoks = arg(argc, argv, "--povodoks", NP), seconds = arg(argc, argv, "--seconds", 60);
	unsigned live = arg(argc, argv, "--live", S0_QUAT_50), near = arg(argc, argv, "--near", 95);
	unsigned side = arg(argc, argv, "--side", 70), far = arg(argc, argv, "--far", 30);
	bool verbose = flag(argc, argv, "-v");
	unsigned cabled = arg(argc, argv, "--cabled", 0), download = arg(argc, argv, "--download", 0);
	unsigned joined = arg(argc, argv, "--joined", 1);
	unsigned overflow0, t;

	set_belkas = arg(argc, argv, "--belkas", 32);
	if (set_belkas < 1 || set_belkas > NB || povodoks < 1 || povodoks > NP ||
	    !live_session_code_ok((uint8_t)live) || joined < 1 || joined > povodoks) {
		printf("--belkas 1-%u, --povodoks 1-%u, --joined 1 to the Povodoks, --live 0, 1, 2, "
		       "4 or 5\n", (unsigned)NB, (unsigned)NP);
		return 2;
	}
	world_sleep = flag(argc, argv, "--sleep");
	world_init(w);
	w->rng ^= arg(argc, argv, "--seed", 0);
	for (int j = 0; j < (int)NP; j++) {
		for (unsigned k = 1; k <= NB; k++) {
			unsigned away = ((unsigned)j + NP - k % NP) % NP;

			w->hear_pb[j][k] = (uint8_t)(away == 0 ? near : away == 2 ? far : side);
		}
		w->p_on[j] = (unsigned)j < povodoks;
	}
	if (cabled > set_belkas) {
		cabled = set_belkas;
	}
	for (unsigned k = set_belkas - cabled + 1; k <= set_belkas; k++) {
		for (int j = 0; j < (int)NP; j++) {
			povodok_link_drop(&w->pv[j], (uint8_t)k);   /* given the set by the cable */
		}
	}
	w->smp_on = true;                                /* the service mode: SMP allowed */
	run(w, 2 * LINK_TAKEOVER_PERIODS);              /* the first stands up, the time known */
	for (t = 0; w->admitted < cabled && t < 60 * LINK_PERIODS_A_S; t++) {
		run(w, 1);
	}
	if (cabled) {
		printf("%u Belkas given the set by the cable, %u taken after their answer, in %.1f s;"
		       " %u SMP packets\n", cabled, w->admitted, t / (double)LINK_PERIODS_A_S,
		       w->smp_packets);
	}
	w->smp_on = false;                               /* at work */
	povodok_link_record(&w->pv[0], true);
	povodok_link_live(&w->pv[0], (uint8_t)live);
	run(w, LINK_PERIODS_A_S);
	memset(w->st_sent, 0, sizeof(w->st_sent));
	memset(w->st_direct, 0, sizeof(w->st_direct));
	memset(w->st_passed, 0, sizeof(w->st_passed));
	memset(w->st_held, 0, sizeof(w->st_held));
	overflow0 = w->pass_overflow;
	printf("%u Belkas, %u Povodoks, live view %u, heard %u/%u/%u %%, %u s\n", set_belkas,
	       povodoks, live, near, side, far, seconds);
	for (unsigned s = 0; s < seconds; s++) {
		unsigned held0 = 0, held1 = 0;

		for (unsigned k = 1; k <= set_belkas; k++) {
			held0 += w->st_held[k];
		}
		run(w, LINK_PERIODS_A_S);
		for (unsigned k = 1; k <= set_belkas; k++) {
			held1 += w->st_held[k];
		}
		if (verbose) {
			int j0 = -1;

			for (int j = 0; j < (int)NP; j++) {
				if (w->p_on[j] && w->pv[j].first) {
					j0 = j;
				}
			}
			printf("t %3u s  first %d  channel %2u  recording %2u  held %5.1f %%  "
			       "PASS overflow %u\n", s + 1, j0, j0 >= 0 ? w->p_listen[j0] : 0,
			       belkas_in_state(w, BELKA_RECORDING),
			       100.0 * (held1 - held0) / (set_belkas * (double)LINK_PERIODS_A_S),
			       w->pass_overflow - overflow0);
		}
	}
	printf("\n Belka   sent  heard itself  passed  held by the first\n");
	for (unsigned k = 1; k <= set_belkas; k++) {
		unsigned n = w->st_sent[k] ? w->st_sent[k] : 1;

		printf(" %5u %6u %10.1f %% %6.1f %% %8.1f %%\n", k, w->st_sent[k],
		       100.0 * w->st_direct[k] / n, 100.0 * w->st_passed[k] / n,
		       100.0 * w->st_held[k] / n);
	}
	printf("\nPASS overflow: %u LIVEs the windows had no room for\n", w->pass_overflow - overflow0);
	if (download) {
		static uint8_t every[STRELKA_BELKAS_MAX];
		unsigned whole[NP] = { 0 }, left_out = 0;
		bool done = false;

		povodok_link_record(&w->pv[0], false);
		povodok_link_live(&w->pv[0], S0_LIVE_OFF);
		for (unsigned j = 0; j < joined; j++) {
			povodok_link_downloads(&w->pv[j], true);   /* joined to the phone */
		}
		run(w, 2 * LINK_PERIODS_A_S);            /* item 8 says each one's Povodok */
		/* the phone: 64/21 to every Povodok joined, the same Belkas */
		for (unsigned k = 1; k <= set_belkas; k++) {
			w->file_len[k] = download * 1024u;
			every[k - 1] = (uint8_t)k;
		}
		for (unsigned j = 0; j < joined; j++) {
			fetch_begin(&w->fe[j], 8, every, NULL, set_belkas);
		}
		w->smp_on = true;
		w->smp_bytes = 0;
		w->smp_packets = 0;
		for (t = 0; !done && t < 3600 * LINK_PERIODS_A_S; t++) {
			run(w, 1);
			done = true;
			for (unsigned j = 0; j < joined; j++) {
				done = done && (!w->fe[j].on || fetch_done(&w->fe[j]));
			}
			if (verbose && t % LINK_PERIODS_A_S == LINK_PERIODS_A_S - 1) {
				unsigned pe = 0, be = 0;

				for (int j = 0; j < (int)NP; j++) {
					pe += w->p_on[j] && w->pv[j].esb;
				}
				for (unsigned k = 1; k <= set_belkas; k++) {
					be += w->bl[k].esb;
				}
				printf("t %3u s  on ESB: %u Povodoks, %2u Belkas  %6.0f kB\n",
				       (t + 1) / LINK_PERIODS_A_S, pe, be, w->smp_bytes / 1000.0);
			}
		}
		for (unsigned k = 1; k <= set_belkas; k++) {
			bool got = false;

			for (unsigned j = 0; j < joined; j++) {
				if (fetch_end_of(&w->fe[j], (uint8_t)k) == FETCH_WHOLE) {
					got = true;
					whole[j]++;
				}
			}
			left_out += !got;
		}
		if (verbose) {
			for (unsigned j = 0; j < joined; j++) {
				printf("Povodok %u: %u Belkas\n", j, whole[j]);
			}
		}
		printf("\ndownload through %u Povodoks: %.0f kB of %u Belkas in %.1f s, %.1f kB/s; %u "
		       "packets, %u Belkas left out\n", joined, w->smp_bytes / 1000.0, set_belkas,
		       t / (double)LINK_PERIODS_A_S,
		       w->smp_bytes / 1000.0 / (t / (double)LINK_PERIODS_A_S), w->smp_packets,
		       left_out);
	}
	return 0;
}
#endif
