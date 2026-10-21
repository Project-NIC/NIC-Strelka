/* A Povodok's side of the inside link against scripted frames (PRINCIPLE.md, The inside link):
 * the first after a second's listening, its SYNC's state in even and odd periods, the items once
 * a minute in a row — with the live view on and off; the MISS and the gathering; another
 * Povodok's repeat a period later, its PASS of what the MISS names and it heard, leaving out what
 * an earlier window passed; the takeover after a second and the yield; the move decided from the
 * margin and said twice; the search's CALLs and the HEREs found. */
#include <string.h>
#include "povodok_link.h"
#include "test.h"

static struct povodok_sync_out so;
static struct povodok_period_out po;
static uint8_t missed[MISS_BODY_BYTES];
static struct povodok_pass_item items[STRELKA_BELKAS_MAX];

static void sync_from(struct povodok_link *l, uint8_t povodok, uint32_t period, bool record,
		      uint8_t code, uint8_t item, uint8_t margin, uint8_t move, uint8_t channel)
{
	struct frame f = {
		.kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = povodok, .s0 = S0(code, item),
		.period = period, .state = SET_STATE(record, margin, move, channel),
	};

	povodok_link_heard(l, &f, -50);
}

static void live_from(struct povodok_link *l, uint8_t belka, uint8_t code, uint8_t item,
		      uint32_t period, uint16_t state, int8_t rssi)
{
	uint8_t body[LIVE_BODY_MAX];
	struct frame f = {
		.kind = FRAME_LIVE, .sender = WHO_BELKA, .number = belka, .s0 = S0(code, item),
		.period = period, .state = state, .body = body, .body_len = live_body_bytes(code),
	};

	memset(body, belka, sizeof(body));
	povodok_link_heard(l, &f, rssi);
}

static void miss_from(struct povodok_link *l, uint8_t povodok, uint32_t period,
		      const uint8_t bits[MISS_BODY_BYTES])
{
	struct frame f = {
		.kind = FRAME_MISS, .sender = WHO_POVODOK, .number = povodok, .s0 = S0(S0_MISS, 0),
		.period = period, .body = bits, .body_len = MISS_BODY_BYTES,
	};

	povodok_link_heard(l, &f, -50);
}

/* a PASS from another Povodok, built with the packet's own builder and read back */
static void pass_from(struct povodok_link *l, uint8_t povodok, uint32_t period, unsigned n,
		      const struct povodok_pass_item *its)
{
	struct strelka_packet p;
	struct frame f;

	frame_pass_begin(&p, povodok, period, 0);
	for (unsigned i = 0; i < n; i++) {
		CHECK(frame_pass_add(&p, its[i].belka, its[i].s0, its[i].state, its[i].steps));
	}
	CHECK(frame_read(&p, frame_ram_len(&p), &f) && f.kind == FRAME_PASS);
	povodok_link_heard(l, &f, -50);
}

static void run_silent(struct povodok_link *l, unsigned periods, int8_t noise)
{
	for (unsigned i = 0; i < periods; i++) {
		povodok_link_period_begin(l, &so);
		CHECK(!so.send);
		povodok_link_period_end(l, noise, &po);
	}
}

int main(void)
{
	struct povodok_link a, b, c;
	struct povodok_link_cfg cfg = { .number = 0, .address = 0xA1B2C3D4u, .channel = 7 };

	/* the first: a second's listening, then its SYNC from period 50 — in an even period the
	 * spare and the margin, in an odd one the channel measured at the last period's end */
	povodok_link_init(&a, &cfg);
	for (unsigned i = 0; i < LINK_TAKEOVER_PERIODS; i++) {
		povodok_link_period_begin(&a, &so);
		CHECK(!so.send && !a.first);
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
		CHECK(po.took_over == (i + 1 == LINK_TAKEOVER_PERIODS));
		CHECK(po.listen_channel == 7);
	}
	CHECK(a.first && a.period == 50);
	povodok_link_period_begin(&a, &so);
	CHECK(so.send && !so.call && so.period == 50 && so.s0 == S0(S0_LIVE_OFF, ITEM_CHANNEL));
	CHECK((so.state & SET_RECORD) == 0 && SET_MARGIN(so.state) == 31 &&
	      SET_MOVE(so.state) == SET_MOVE_NONE && SET_CHANNEL(so.state) == 8);
	CHECK(po.measure_channel == 10);           /* ten times its number, 50 periods on */
	povodok_link_period_end(&a, -88, &po);
	povodok_link_period_begin(&a, &so);
	CHECK(so.send && so.period == 51 && SET_MOVE(so.state) == SET_MEASUREMENT &&
	      SET_CHANNEL(so.state) == 10 && SET_NOISE_DBM(so.state) == -88);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	povodok_link_period_begin(&a, &so);
	CHECK(so.period == 52 && SET_MOVE(so.state) == SET_MOVE_NONE);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	OK("the first after a second's listening, its even and odd SYNCs");

	/* the items: once a minute 1–7 in a row, item 8 otherwise; the live view off, the row goes
	 * on in the periods the Belkas send in alone */
	povodok_link_record(&a, true);
	povodok_link_live(&a, S0_QUAT_50);
	while (a.period < 3050) {
		povodok_link_period_begin(&a, &so);
		CHECK(so.send && S0_LOW(so.s0) == ITEM_CHANNEL && so.s0 == S0(S0_QUAT_50, 8) &&
		      (so.state & SET_RECORD));
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	}
	for (unsigned item = ITEM_FIRST; item <= ITEM_OUT; item++) {
		povodok_link_period_begin(&a, &so);
		CHECK(S0_LOW(so.s0) == item && so.period == 3050 + item - 1);
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	}
	povodok_link_period_begin(&a, &so);
	CHECK(S0_LOW(so.s0) == ITEM_CHANNEL);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	povodok_link_live(&a, S0_LIVE_OFF);
	while (a.period < 6050) {
		povodok_link_period_begin(&a, &so);
		CHECK(S0_LOW(so.s0) == ITEM_CHANNEL);
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	}
	for (unsigned item = ITEM_FIRST; item <= ITEM_OUT; item++) {
		for (unsigned i = 0; i < LINK_STATE_EVERY; i++) {
			povodok_link_period_begin(&a, &so);
			CHECK(S0_LOW(so.s0) == item && so.period == 6050 + (item - 1) * 50 + i);
			povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
		}
	}
	povodok_link_period_begin(&a, &so);
	CHECK(S0_LOW(so.s0) == ITEM_CHANNEL && so.period == 6400);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	OK("the items once a minute, in the Belkas' sending periods");

	/* the MISS and the gathering: three Belkas taken, two heard, one of them saying a channel's
	 * noise; a Belka not taken ignored; the margin from the signals heard */
	povodok_link_live(&a, S0_QUAT_50);
	povodok_link_take(&a, 1, 0x11);
	povodok_link_take(&a, 2, 0x22);
	povodok_link_take(&a, 3, 0x33);
	povodok_link_period_begin(&a, &so);
	live_from(&a, 1, S0_QUAT_50, ITEM_CHANNEL, so.period, ITEM_BYTES(20, -100), -60);
	live_from(&a, 3, S0_QUAT_50, ITEM_CELL_MV, so.period, 3300, -64);
	live_from(&a, 4, S0_QUAT_50, ITEM_CELL_MV, so.period, 3300, -64);
	CHECK(povodok_link_miss(&a, missed) && miss_has(missed, 2) && !miss_has(missed, 1) &&
	      !miss_has(missed, 3) && !miss_has(missed, 4));
	CHECK(povodok_link_pass(&a, items, STRELKA_BELKAS_MAX) == 0);   /* the first passes none */
	{
		uint8_t numbers[STRELKA_BELKAS_MAX];

		CHECK(povodok_link_gathered(&a, numbers) == 2 && numbers[0] == 1 && numbers[1] == 3);
	}
	CHECK(a.belka[1].heard && a.belka[1].rssi == -60 && a.belka[1].steps_len == 4 &&
	      a.belka[1].steps[0] == 1 && a.belka[1].item_known[ITEM_CHANNEL]);
	CHECK(a.belka[3].item_known[ITEM_CELL_MV] && a.belka[3].item[ITEM_CELL_MV] == 3300 &&
	      !a.belka[3].item_known[ITEM_CHANNEL]);
	CHECK(a.noise_belka[20] == -100 && !a.belka[4].taken);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	CHECK(a.margin == 19 && !a.belka[1].heard);      /* (-62 + 100) / 2, the floor unmeasured */
	povodok_link_period_begin(&a, &so);
	CHECK(SET_MARGIN(so.state) == 19 && SET_CHANNEL(so.state) == 20);  /* the quietest known */
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	OK("the MISS, the gathering, the margin, the spare from the Belkas' measurements");

	/* another Povodok: the first's content repeated a period later with its own margin; the
	 * MISS heard, it passes what it heard of the missed; a third leaves out what the second
	 * passed */
	cfg.number = 1;
	povodok_link_init(&b, &cfg);
	cfg.number = 2;
	povodok_link_init(&c, &cfg);
	for (unsigned k = 1; k <= 6; k++) {
		povodok_link_take(&b, (uint8_t)k, k);
		povodok_link_take(&c, (uint8_t)k, k);
	}
	povodok_link_period_begin(&b, &so);
	CHECK(!so.send);
	sync_from(&b, 0, 1000, true, S0_QUAT_50, ITEM_CARD, 20, SET_MOVE_NONE, 12);
	CHECK(b.period == 1000 && b.record && b.spare == 12 && b.live_code == S0_QUAT_50);
	live_from(&b, 2, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(90, 0), -70);
	live_from(&b, 5, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(91, 0), -71);
	live_from(&b, 6, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(92, 0), -72);
	CHECK(!povodok_link_miss(&b, missed));
	memset(missed, 0, sizeof(missed));
	miss_set(missed, 2);
	miss_set(missed, 4);
	miss_set(missed, 5);
	miss_from(&b, 0, 1000, missed);
	CHECK(povodok_link_pass(&b, items, STRELKA_BELKAS_MAX) == 2 && items[0].belka == 2 &&
	      items[1].belka == 5 && items[0].state == ITEM_BYTES(90, 0) &&
	      items[0].steps_len == 4 && items[0].steps[0] == 2);
	/* the third heard 2, 4 and 6, and the second's PASS */
	povodok_link_period_begin(&c, &so);
	sync_from(&c, 0, 1000, true, S0_QUAT_50, ITEM_CARD, 20, SET_MOVE_NONE, 12);
	sync_from(&c, 1, 1000, true, S0_QUAT_50, ITEM_CARD, 25, SET_MOVE_NONE, 12);
	live_from(&c, 2, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(90, 0), -80);
	live_from(&c, 4, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(93, 0), -80);
	live_from(&c, 6, S0_QUAT_50, ITEM_CARD, 1000, ITEM_BYTES(92, 0), -80);
	miss_from(&c, 0, 1000, missed);
	pass_from(&c, 1, 1000, 2, items);
	CHECK(povodok_link_pass(&c, items, STRELKA_BELKAS_MAX) == 1 && items[0].belka == 4);
	CHECK(c.belka[2].heard && c.belka[2].passed && c.belka[5].passed && !c.belka[5].heard &&
	      c.belka[5].by == 1 && c.margin_povodok[1] == 25);
	/* the first takes the PASSes: by whom, and nothing twice */
	povodok_link_period_begin(&a, &so);
	live_from(&a, 1, S0_QUAT_50, ITEM_CARD, so.period, ITEM_BYTES(89, 0), -60);
	CHECK(povodok_link_miss(&a, missed) && miss_has(missed, 2) && miss_has(missed, 3));
	items[0].belka = 2;
	items[0].s0 = S0(S0_QUAT_50, ITEM_CARD);
	items[0].state = ITEM_BYTES(90, 0);
	items[0].steps = b.belka[2].steps;
	items[1].belka = 1;                       /* heard by the first itself: ignored */
	items[1].s0 = S0(S0_QUAT_50, ITEM_CARD);
	items[1].state = 0;
	items[1].steps = b.belka[2].steps;
	pass_from(&a, 1, so.period, 2, items);
	items[0].belka = 2;                       /* passed already: ignored */
	items[0].state = 1;
	items[1].belka = 3;
	items[1].state = ITEM_BYTES(70, 1);
	pass_from(&a, 2, so.period, 2, items);
	{
		uint8_t numbers[STRELKA_BELKAS_MAX];

		CHECK(povodok_link_gathered(&a, numbers) == 3);
	}
	CHECK(a.belka[2].passed && a.belka[2].by == 1 && a.belka[2].state == ITEM_BYTES(90, 0) &&
	      a.belka[2].rssi == REC_NO_SIGNAL && a.belka[3].by == 2 && a.belka[3].item[ITEM_CARD] ==
	      ITEM_BYTES(70, 1) && a.belka[1].state == ITEM_BYTES(89, 0) && a.belka[1].by == 0);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	/* the repeat: the second's SYNC in the next period, the first's S0, its own margin */
	povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
	CHECK(b.period == 1001 && !po.took_over && po.listen_channel == 7);
	povodok_link_period_begin(&b, &so);
	CHECK(so.send && so.period == 1001 && so.s0 == S0(S0_QUAT_50, ITEM_CARD) &&
	      (so.state & SET_RECORD) && SET_MARGIN(so.state) == b.margin &&
	      SET_CHANNEL(so.state) == 12 && b.margin == (-71 + 100) / 2);
	povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
	povodok_link_period_begin(&b, &so);
	CHECK(!so.send);                          /* nothing heard: nothing to repeat */
	povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
	OK("the repeat a period later, the PASSes by the windows, nothing twice");

	/* the takeover: the first silent — the spare tried every other period from the second miss,
	 * the second Povodok the first after a second, with the set's time and the record going on;
	 * the first heard again, it yields */
	{
		uint32_t silent = 2;              /* two periods without the first's SYNC so far */

		while (!po.took_over) {
			povodok_link_period_begin(&b, &so);
			CHECK(!so.send);
			povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
			silent++;
			if (!po.took_over) {
				CHECK(po.listen_channel == (silent % 2 == 0 ? 12 : 7));
			}
		}
		CHECK(silent == LINK_TAKEOVER_PERIODS && b.first && po.listen_channel == 7);
		CHECK(b.period == 1000 + LINK_TAKEOVER_PERIODS + 1);
		povodok_link_period_begin(&b, &so);
		CHECK(so.send && so.period == b.period && (so.state & SET_RECORD) &&
		      so.s0 == S0(S0_QUAT_50, ITEM_CHANNEL) && SET_CHANNEL(so.state) == 12);
		CHECK(povodok_link_miss(&b, missed));
		povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
		povodok_link_period_begin(&b, &so);
		sync_from(&b, 0, 2000, false, S0_RAW_50, ITEM_SYNCS, 20, SET_MOVE_NONE, 13);
		CHECK(!povodok_link_miss(&b, missed) || b.first);
		povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
		CHECK(po.yielded && !b.first && b.period == 2001);
		povodok_link_period_begin(&b, &so);
		CHECK(so.send && so.s0 == S0(S0_RAW_50, ITEM_SYNCS) && !(so.state & SET_RECORD) &&
		      SET_CHANNEL(so.state) == 13);
		povodok_link_period_end(&b, LINK_NOISE_UNKNOWN, &po);
	}
	OK("the takeover after a second, the yield");

	/* the move: the margin under five steps for a second — the first says it in two SYNCs,
	 * the spare the quietest channel measured, and moves; the margin on the new channel */
	{
		struct povodok_link d;
		uint32_t decided = 0, moved = 0;
		int8_t noise[STRELKA_CHANNELS];

		for (unsigned ch = 0; ch < STRELKA_CHANNELS; ch++) {
			noise[ch] = (int8_t)(-95 + (int)(ch * 7 % 11));
		}
		noise[7] = -80;                   /* the set's channel, loud */
		noise[20] = -100;                 /* the quietest */
		cfg.number = 0;
		povodok_link_init(&d, &cfg);
		povodok_link_take(&d, 1, 1);
		povodok_link_take(&d, 2, 2);
		run_silent(&d, LINK_TAKEOVER_PERIODS, LINK_NOISE_UNKNOWN);
		povodok_link_record(&d, true);
		povodok_link_live(&d, S0_QUAT_50);
		for (unsigned i = 0; i < 200 && !moved; i++) {
			povodok_link_period_begin(&d, &so);
			CHECK(so.send);
			if (SET_MOVE(so.state) == SET_MOVE_TWO) {
				CHECK(!decided && SET_CHANNEL(so.state) == 20);
				decided = so.period;
			} else if (SET_MOVE(so.state) == SET_MOVE_NEXT) {
				CHECK(decided == so.period - 1 && SET_CHANNEL(so.state) == 20);
			} else if (decided) {
				CHECK(so.period >= decided + 2);
			}
			live_from(&d, 1, S0_QUAT_50, ITEM_CHANNEL, so.period, 0, -90);
			live_from(&d, 2, S0_QUAT_50, ITEM_CHANNEL, so.period, 0, -92);
			povodok_link_period_end(&d, noise[po.measure_channel], &po);
			if (po.moved) {
				moved = d.period;
			}
		}
		CHECK(decided && moved == decided + 2 && d.cfg.channel == 20 && po.listen_channel == 20);
		CHECK(d.margin == 0);
		/* the margin under five from the first LIVEs at period 50, the channel's noise taken
		 * for the floor until measured; fifty periods low, the decision at the end of the
		 * fiftieth, said in the next SYNC */
		CHECK(decided == 50 + LINK_MOVE_PERIODS);
		povodok_link_period_begin(&d, &so);
		CHECK(SET_MOVE(so.state) != SET_MOVE_TWO && SET_MOVE(so.state) != SET_MOVE_NEXT);
		CHECK(SET_MOVE(so.state) == SET_MEASUREMENT || SET_CHANNEL(so.state) != 20);
		povodok_link_period_end(&d, noise[po.measure_channel], &po);
		for (unsigned i = 0; i < 100; i++) {
			povodok_link_period_begin(&d, &so);
			CHECK(SET_MOVE(so.state) != SET_MOVE_TWO);
			live_from(&d, 1, S0_QUAT_50, ITEM_CHANNEL, so.period, 0, -90);
			povodok_link_period_end(&d, noise[po.measure_channel], &po);
		}
		CHECK(d.margin == 5 && d.cfg.channel == 20);   /* (-90 + 100) / 2: not under five */
	}
	OK("the move from the margin, said twice, to the quietest channel");

	/* the search: CALLs with the attempt, on the search's channel, the attempt up each round;
	 * the HEREs found once each; no takeover while searching */
	{
		struct povodok_link s;
		struct frame here = { .kind = FRAME_HERE, .sender = WHO_BELKA, .number = 0,
				      .s0 = S0(S0_HERE, 0) };
		uint8_t chip[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };

		cfg.number = 1;
		povodok_link_init(&s, &cfg);
		povodok_link_period_begin(&s, &so);
		sync_from(&s, 0, 3000, true, S0_QUAT_50, ITEM_CHANNEL, 20, SET_MOVE_NONE, 12);
		povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
		povodok_link_search(&s, true);
		for (unsigned i = 0; i < 2 * SEARCH_PERIODS + 5; i++) {
			povodok_link_period_begin(&s, &so);
			CHECK(so.send && so.call && S0_HIGH(so.s0) == S0_CALL &&
			      S0_LOW(so.s0) == i / SEARCH_PERIODS && so.period == 3001 + i);
			if (i == 3 || i == 600) {
				here.body = chip;
				here.body_len = 8;
				here.period = so.period;
				povodok_link_heard(&s, &here, i == 3 ? -70 : -65);
			}
			povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
			CHECK(po.listen_channel == STRELKA_SEARCH_CHANNEL && !po.took_over);
		}
		CHECK(s.found_n == 1 && s.found[0].chip == 0x0102030405060708ull &&
		      s.found[0].rssi == -65 && s.found[0].period == 3004 && !s.first);
		povodok_link_search(&s, false);
		povodok_link_period_begin(&s, &so);
		CHECK(!so.send);
		povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
		CHECK(po.listen_channel == 7 || po.listen_channel == 12);
	}
	OK("the search's CALLs, the attempt by the round, the HEREs found");

	printf("POVODOK LINK OK\n");
	return 0;
}
