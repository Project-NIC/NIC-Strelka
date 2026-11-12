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

/* a period of two Povodoks hearing each other: the SYNCs, then the ends */
static void two(struct povodok_link *p, struct povodok_sync_out s[2], struct povodok_period_out o[2])
{
	for (unsigned j = 0; j < 2; j++) {
		povodok_link_period_begin(&p[j], &s[j]);
		if (s[j].send) {
			struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK,
					   .number = p[j].cfg.number, .s0 = s[j].s0,
					   .period = s[j].period, .state = s[j].state };

			povodok_link_heard(&p[1 - j], &f, -50);
		}
	}
	for (unsigned j = 0; j < 2; j++) {
		povodok_link_period_end(&p[j], LINK_NOISE_UNKNOWN, &o[j]);
	}
}

/* a period of `l` hearing one SYNC, from `from`, its who carrying `flight` */
static void heard_flight(struct povodok_link *l, uint8_t from, uint32_t period, uint8_t flight)
{
	struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = from,
			   .flight = flight, .s0 = S0(S0_LIVE_OFF, 0), .period = period,
			   .state = SET_STATE(true, 20, SET_MOVE_NONE, 12) };

	povodok_link_period_begin(l, &so);
	povodok_link_heard(l, &f, -50);
	povodok_link_period_end(l, LINK_NOISE_UNKNOWN, &po);
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

	/* the first: three seconds' listening at rest, then its SYNC in the state's period alone,
	 * once a second — the spare and the margin, and every other second the channel measured in
	 * the period before */
	povodok_link_init(&a, &cfg);
	for (unsigned i = 0; i < LINK_TAKEOVER_REST_PERIODS; i++) {
		povodok_link_period_begin(&a, &so);
		CHECK(!so.send && !a.first);
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
		CHECK(po.took_over == (i + 1 == LINK_TAKEOVER_REST_PERIODS));
		CHECK(po.listen_channel == 7);
	}
	CHECK(a.first && a.period == 150 && po.measure_channel == 150 % LINK_MEASURED_CHANNELS);
	while (a.period < 250) {
		povodok_link_period_begin(&a, &so);
		CHECK(so.send == (a.period % LINK_STATE_EVERY == 0));
		if (so.send) {
			CHECK(!so.call && (so.s0 >> 4) == S0_LIVE_OFF &&
			      (so.state & SET_RECORD) == 0 && SET_MARGIN(so.state) == 31 &&
			      SET_MOVE(so.state) == SET_MOVE_NONE && SET_CHANNEL(so.state) == 8);
		}
		povodok_link_period_end(&a, a.period == 249 ? -88 : LINK_NOISE_UNKNOWN, &po);
	}
	povodok_link_period_begin(&a, &so);
	CHECK(so.send && so.period == 250 && SET_MOVE(so.state) == SET_MEASUREMENT &&
	      SET_CHANNEL(so.state) == 249 % LINK_MEASURED_CHANNELS && SET_NOISE_DBM(so.state) == -88);
	povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	OK("the first after three seconds at rest, its SYNC once a second, the measurement every other");

	/* the items: item 8, the channels', every other period the Belkas send in, 1–7 in turn
	 * between — every period with the live view on, once a second with it off */
	povodok_link_record(&a, true);
	povodok_link_live(&a, S0_QUAT_50);
	{
		uint8_t seen[28], last = 0;

		for (unsigned i = 0; i < 28; i++) {
			povodok_link_period_begin(&a, &so);
			CHECK(so.send && (so.s0 >> 4) == S0_QUAT_50 && (so.state & SET_RECORD) &&
			      !(so.state & SET_ESB));
			seen[i] = S0_LOW(so.s0);
			povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
		}
		for (unsigned i = 0; i < 28; i++) {
			if (seen[i] == ITEM_CHANNEL) {
				CHECK(i + 1 == 28 || seen[i + 1] != ITEM_CHANNEL);
				continue;
			}
			CHECK(seen[i] >= ITEM_FIRST && seen[i] <= ITEM_OUT);
			CHECK(last == 0 || seen[i] == last % ITEM_OUT + 1);
			last = seen[i];
		}
		CHECK(last != 0);
	}
	povodok_link_live(&a, S0_LIVE_OFF);
	while (a.period % LINK_STATE_EVERY != 1) {
		povodok_link_period_begin(&a, &so);
		povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
	}
	{
		bool had[ITEM_LAST + 1] = { false };

		for (unsigned sec = 0; sec < 14; sec++) {
			uint8_t item = 0;

			for (unsigned i = 0; i < LINK_STATE_EVERY; i++) {
				povodok_link_period_begin(&a, &so);
				if (i == LINK_STATE_EVERY - 1) {
					item = S0_LOW(so.s0);          /* the Belkas' period */
				}
				povodok_link_period_end(&a, LINK_NOISE_UNKNOWN, &po);
			}
			had[item] = true;
		}
		for (unsigned item = ITEM_FIRST; item <= ITEM_LAST; item++) {
			CHECK(had[item]);
		}
	}
	OK("the items: item 8 every other time the Belkas send, 1–7 in turn between");

	/* the MISS and the gathering: three Belkas taken, two heard, one of them saying a channel's
	 * noise; a fourth, not taken, heard on the set's address and listed to be asked, not let in
	 * (the review's H4); the margin from the signals heard */
	povodok_link_live(&a, S0_QUAT_50);
	povodok_link_take(&a, 1, 0x11);
	povodok_link_take(&a, 2, 0x22);
	povodok_link_take(&a, 3, 0x33);
	povodok_link_period_begin(&a, &so);
	live_from(&a, 1, S0_QUAT_50, ITEM_CHANNEL, so.period, ITEM_BYTES(20, -100), -60);
	live_from(&a, 3, S0_QUAT_50, ITEM_CELL_MV, so.period, 3300, -64);
	live_from(&a, 4, S0_QUAT_50, ITEM_CELL_MV, so.period, 3300, -64);
	CHECK(!a.belka[4].taken && a.belka[4].unknown);
	CHECK(povodok_link_miss(&a, missed) && miss_has(missed, 2) && !miss_has(missed, 1) &&
	      !miss_has(missed, 3) && !miss_has(missed, 4));
	/* 2 not among the session's units: missed in the state's periods alone, as at rest */
	povodok_link_units(&a, (const uint8_t[UNITS_BYTES]){ 0x0A });
	CHECK(a.period % LINK_STATE_EVERY != 0 && povodok_link_miss(&a, missed) &&
	      !miss_has(missed, 2));
	povodok_link_units(&a, NULL);
	CHECK(povodok_link_miss(&a, missed) && miss_has(missed, 2));
	CHECK(povodok_link_pass(&a, items, STRELKA_BELKAS_MAX) == 0);   /* the first passes none */
	{
		uint8_t numbers[STRELKA_BELKAS_MAX];

		CHECK(povodok_link_gathered(&a, numbers) == 2 && numbers[0] == 1 && numbers[1] == 3);
	}
	CHECK(a.belka[1].heard && a.belka[1].rssi == -60 && a.belka[1].steps_len == 4 &&
	      a.belka[1].steps[0] == 1 && a.belka[1].item_known[ITEM_CHANNEL]);
	CHECK(a.belka[3].item_known[ITEM_CELL_MV] && a.belka[3].item[ITEM_CELL_MV] == 3300 &&
	      !a.belka[3].item_known[ITEM_CHANNEL]);
	CHECK(a.noise_belka[20] == -100 && !a.belka[4].taken && !a.belka[4].heard);
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
	/* a Povodok numbered 3 hears 1, stood up as a first of its own, and then 2, repeating the
	 * first's: the lower first's SYNC takes the place of the one taken, and its own goes out
	 * as one repeating, the first named in it */
	{
		struct povodok_link r;
		struct povodok_link_cfg c3 = cfg;
		struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK,
				   .s0 = S0(S0_QUAT_50, ITEM_CARD),
				   .state = SET_STATE(true, 20, SET_MOVE_NONE, 12) };

		c3.number = 3;
		povodok_link_init(&r, &c3);
		povodok_link_period_begin(&r, &so);
		f.number = 1, f.root = 1, f.period = 2001;
		povodok_link_heard(&r, &f, -50);
		CHECK(r.period == 2001 && r.root == 1);
		f.number = 2, f.root = 0, f.period = 2000;
		povodok_link_heard(&r, &f, -50);
		CHECK(r.period == 2000 && r.root == 0 && r.first_number == 2);
		f.number = 1, f.root = 1, f.period = 2001;   /* no lower: not taken back */
		povodok_link_heard(&r, &f, -50);
		CHECK(r.period == 2000 && r.root == 0);
		povodok_link_period_end(&r, LINK_NOISE_UNKNOWN, &po);
		povodok_link_period_begin(&r, &so);
		CHECK(so.send && so.repeat && so.root == 0 && so.period == 2001 && so.flight == 0);
	}
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
	 * the second Povodok the first after five seconds, a first hidden a moment not being a first
	 * gone, with the set's time and the record going on;
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
		CHECK(silent == LINK_TAKEOVER_LOST_PERIODS && b.first && po.listen_channel == 7);
		CHECK(b.period == 1000 + LINK_TAKEOVER_LOST_PERIODS + 1);
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
		run_silent(&d, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
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
		/* the margin under five from the first LIVEs at period 150, after three seconds'
		 * listening at rest, the channel's noise taken for the floor until measured; fifty
		 * periods low, the decision at the end of the fiftieth, said in the next SYNC */
		CHECK(decided == LINK_TAKEOVER_REST_PERIODS + LINK_MOVE_PERIODS);
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

	/* the units far, the band quiet: the margin low and every channel alike — no move, a spare
	 * no quieter than the set's own buying nothing; nor before the set's own is measured */
	{
		struct povodok_link d;

		cfg.number = 0;
		povodok_link_init(&d, &cfg);
		povodok_link_take(&d, 1, 1);
		run_silent(&d, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
		povodok_link_record(&d, true);
		povodok_link_live(&d, S0_QUAT_50);
		for (unsigned i = 0; i < 6 * LINK_PERIODS_A_S; i++) {
			povodok_link_period_begin(&d, &so);
			CHECK(SET_MOVE(so.state) != SET_MOVE_TWO && SET_MOVE(so.state) != SET_MOVE_NEXT);
			live_from(&d, 1, S0_QUAT_50, ITEM_CHANNEL, so.period, 0, -93);
			povodok_link_period_end(&d, (int8_t)(-98 + (int)(po.measure_channel % 3)), &po);
			CHECK(!po.moved);
		}
		CHECK(d.margin < LINK_MOVE_MARGIN && d.cfg.channel == 7 &&
		      d.low_margin_periods >= LINK_MOVE_PERIODS);
	}
	OK("no move for the distance alone, the spare no quieter than the set's own channel");

	/* a Povodok not the first tells a measurement in every other SYNC: all of the set's 39
	 * channels all the same, their count odd, and never the search's */
	{
		struct povodok_link d;
		bool told[STRELKA_CHANNELS] = { false };
		unsigned n = 0;

		cfg.number = 2;
		povodok_link_init(&d, &cfg);
		for (uint32_t period = 4000; period < 4000 + 4 * LINK_MEASURED_CHANNELS; period++) {
			povodok_link_period_begin(&d, &so);
			if (so.send && SET_MOVE(so.state) == SET_MEASUREMENT) {
				told[SET_CHANNEL(so.state)] = true;
			}
			sync_from(&d, 0, period, true, S0_QUAT_50, ITEM_CHANNEL, 20, SET_MOVE_NONE, 12);
			povodok_link_period_end(&d, -90, &po);
		}
		for (unsigned ch = 0; ch < STRELKA_CHANNELS; ch++) {
			n += told[ch];
		}
		CHECK(n == LINK_MEASURED_CHANNELS && !told[STRELKA_SEARCH_CHANNEL]);
	}
	OK("every one of the set's 39 channels told in every other SYNC");

	/* on ESB for a long download: once a minute the first leaves it for two of the state's
	 * periods and sends its SYNC, bit 14 in it, for a unit back on the SYNCs */
	{
		struct povodok_link d;
		unsigned syncs = 0;

		cfg.number = 0;
		povodok_link_init(&d, &cfg);
		run_silent(&d, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
		while (d.period < LINK_ESB_SYNC_PERIODS + 3 * LINK_STATE_EVERY) {
			bool was_esb = d.esb;

			povodok_link_awake(&d, true);
			povodok_link_period_begin(&d, &so);
			if (was_esb && so.send) {
				CHECK((so.period == LINK_ESB_SYNC_PERIODS ||
				       so.period == LINK_ESB_SYNC_PERIODS + LINK_STATE_EVERY) &&
				      !so.call && (so.state & SET_ESB));
				syncs++;
			}
			povodok_link_period_end(&d, LINK_NOISE_UNKNOWN, &po);
			if (d.esb) {
				CHECK(po.esb == !(d.period == LINK_ESB_SYNC_PERIODS ||
						  d.period == LINK_ESB_SYNC_PERIODS + LINK_STATE_EVERY));
			}
		}
		CHECK(d.esb && d.first && syncs == LINK_ESB_SYNCS);
	}
	OK("on ESB the first's SYNC twice a minute, the rest ESB's whole");

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
			if ((3001 + i) % LINK_STATE_EVERY == 0) {
				CHECK(!so.call);     /* the state's period: the set's SYNC's, no CALL */
				povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
				CHECK(po.listen_channel == STRELKA_SEARCH_CHANNEL && !po.took_over);
				continue;
			}
			CHECK(so.send && so.call && S0_HIGH(so.s0) == S0_CALL &&
			      S0_LOW(so.s0) == i / SEARCH_PERIODS && so.period == 3001 + i);
			if (i == 3 || i == 600) {
				here.body = chip;
				here.body_len = 8;
				here.period = so.period;
				povodok_link_heard(&s, &here, i == 3 ? -70 : -65);
			}
			povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
			CHECK(po.listen_channel == ((3002 + i) % LINK_STATE_EVERY == 0
						    ? s.try_channel : STRELKA_SEARCH_CHANNEL) &&
			      !po.took_over);
		}
		CHECK(s.found_n == 1 && s.found[0].chip == 0x0102030405060708ull &&
		      s.found[0].rssi == -65 && s.found[0].period == 3004 && !s.first);
		povodok_link_search(&s, false);
		povodok_link_period_begin(&s, &so);
		CHECK(!so.send);
		povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
		CHECK(po.listen_channel == 7 || po.listen_channel == 12);

		/* the first searching: in the state's period its SYNC on the set's channel, the set's
		 * own units not left without one; a CALL in every other */
		cfg.number = 0;
		povodok_link_init(&s, &cfg);
		run_silent(&s, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
		CHECK(s.first);
		povodok_link_search(&s, true);
		for (unsigned i = 0; i < 2 * LINK_STATE_EVERY; i++) {
			bool states = s.period % LINK_STATE_EVERY == 0;

			povodok_link_period_begin(&s, &so);
			CHECK(so.send && so.call == !states);
			CHECK(states ? S0_HIGH(so.s0) == S0_LIVE_OFF : S0_HIGH(so.s0) == S0_CALL);
			povodok_link_period_end(&s, LINK_NOISE_UNKNOWN, &po);
			CHECK(po.listen_channel == (s.period % LINK_STATE_EVERY == 0
						    ? 7 : STRELKA_SEARCH_CHANNEL));
		}
		povodok_link_search(&s, false);
	}
	OK("the search's CALLs, the attempt by the round, the HEREs found");

	/* a replayed SYNC: a Povodok following the first drops a period far from its count */
	{
		struct povodok_link r;

		cfg.number = 1;
		povodok_link_init(&r, &cfg);
		for (unsigned i = 0; i < 2 * LINK_PERIODS_A_S; i++) {
			povodok_link_period_begin(&r, &so);
			sync_from(&r, 0, 7000 + i, false, S0_LIVE_OFF, ITEM_CHANNEL, 20, SET_MOVE_NONE,
				  7);
			povodok_link_period_end(&r, LINK_NOISE_UNKNOWN, &po);
		}
		CHECK(r.period == 7000 + 2 * LINK_PERIODS_A_S);
		povodok_link_period_begin(&r, &so);
		sync_from(&r, 0, 6500, true, S0_QUAT_50, ITEM_CHANNEL, 20, SET_MOVE_NONE, 7);
		CHECK(r.period == 7000 + 2 * LINK_PERIODS_A_S && !r.record);
		sync_from(&r, 0, 7000 + 2 * LINK_PERIODS_A_S, false, S0_LIVE_OFF, ITEM_CHANNEL, 20,
			  SET_MOVE_NONE, 7);
		CHECK(r.content_taken);
		povodok_link_period_end(&r, LINK_NOISE_UNKNOWN, &po);
	}
	OK("a replayed SYNC far from the count dropped by a Povodok that follows");

	/* a PASS or a LIVE from a number not taken lets it in on nothing — no LIVE is signed — but
	 * lists it to be asked over SMP (the review's H4); HEREs from more than 32 chips keep the
	 * first 32; with no channel measured the spare is never the set's own channel */
	{
		struct povodok_link q;
		struct povodok_pass_item it = { .belka = 9, .s0 = S0(S0_LIVE_OFF, ITEM_STATE),
						.state = ITEM_BYTES(4, 0), .steps = NULL };
		struct frame here = { .kind = FRAME_HERE, .sender = WHO_BELKA, .number = 0,
				      .s0 = S0(S0_HERE, 0), .body_len = 8 };
		uint8_t chip[8] = { 0 };

		cfg.number = 0;
		povodok_link_init(&q, &cfg);
		uint8_t unknown[STRELKA_BELKAS_MAX];

		CHECK(!q.belka[9].taken);
		povodok_link_period_begin(&q, &so);
		pass_from(&q, 1, q.period, 1, &it);
		live_from(&q, 11, S0_LIVE_OFF, ITEM_STATE, q.period, 0, -60);
		povodok_link_period_end(&q, LINK_NOISE_UNKNOWN, &po);
		CHECK(!q.belka[9].taken && !q.belka[11].taken);
		CHECK(povodok_link_unknown(&q, unknown) == 2 && unknown[0] == 9 && unknown[1] == 11);
		CHECK(povodok_link_unknown(&q, unknown) == 0);     /* each once */
		povodok_link_take(&q, 9, 0);                        /* her answer passed the MIC */
		CHECK(q.belka[9].taken);

		povodok_link_search(&q, true);
		for (unsigned i = 0; i < 40; i++) {
			povodok_link_period_begin(&q, &so);
			chip[0] = (uint8_t)(i + 1);
			here.body = chip;
			here.period = so.period;
			povodok_link_heard(&q, &here, -60);
			povodok_link_period_end(&q, LINK_NOISE_UNKNOWN, &po);
		}
		CHECK(q.found_n == 32);
		povodok_link_search(&q, false);
		for (unsigned i = 0; i < 3 * LINK_PERIODS_A_S; i++) {
			povodok_link_period_begin(&q, &so);
			povodok_link_period_end(&q, LINK_NOISE_UNKNOWN, &po);
		}
		CHECK(q.spare != q.cfg.channel && q.spare < STRELKA_SET_CHANNELS);
	}
	OK("an unknown number listed to be asked, not let in; 32 HEREs at most; the spare unmeasured");

	/* the set given over to ESB, at rest: a frame waiting in the first, bit 14 in its SYNCs and
	 * the other's, once a second, for two of the state's periods; then no SYNC from either,
	 * nothing taken over, while frames wait and packets come; 10 s with none, the first's SYNC
	 * back at the next state's period, the other following it */
	{
		struct povodok_link p[2];
		struct povodok_sync_out s[2];
		struct povodok_period_out o[2];
		struct povodok_link_cfg c1 = cfg;
		unsigned i, n = 0, states = 0;
		bool home = false;

		c1.number = 1;
		povodok_link_init(&p[0], &cfg);
		povodok_link_init(&p[1], &c1);
		povodok_link_downloads(&p[0], true);
		povodok_link_downloads(&p[1], true);   /* its own channel: 1, of 1, 5, 9 … */
		for (i = 0; i <= 4 * LINK_PERIODS_A_S; i++) {
			two(p, s, o);                  /* at rest: the SYNCs once a second */
		}
		CHECK(p[0].first && !p[1].first && s[0].send && s[1].send);
		while (!o[0].esb) {
			povodok_link_awake(&p[0], true);
			two(p, s, o);
			CHECK(o[1].esb == o[0].esb && s[1].send == s[0].send);
			if (s[0].send) {               /* the state's period alone */
				CHECK(s[0].period % LINK_STATE_EVERY == 0 && (s[0].state & SET_ESB) &&
				      (s[0].state & SET_DOWNLOADS) && SET_CHANNEL(s[0].state) == 7 &&
				      (s[1].state & SET_DOWNLOADS));
				CHECK(states == 0 ||
				      ((s[1].state & SET_ESB) && SET_CHANNEL(s[1].state) == 1));
				states++;
			}
			CHECK(++n <= 2 * LINK_PERIODS_A_S);
		}
		CHECK(states == LINK_ESB_ANNOUNCE);
		for (i = 0; i < 20 * LINK_PERIODS_A_S; i++) {
			povodok_link_awake(&p[0], true);
			povodok_link_traffic(&p[1]);
			two(p, s, o);
			CHECK(!s[0].send && !s[1].send && o[0].esb && o[1].esb && !o[1].took_over &&
			      o[0].listen_channel == 7 && o[1].listen_channel == 1);
		}
		povodok_link_awake(&p[0], false);
		for (n = 0;; n++) {
			two(p, s, o);
			if (s[0].send) {
				break;
			}
			CHECK(!s[1].send && n < 2 * LINK_SPOKEN_PERIODS);
			home |= p[1].esb_home && o[1].listen_channel == 7;   /* back, silent */
		}
		/* 10 s on, at rest, the first's SYNC at the next state's period */
		CHECK(n >= LINK_SPOKEN_PERIODS && n < LINK_SPOKEN_PERIODS + LINK_STATE_EVERY &&
		      s[0].period % LINK_STATE_EVERY == 0 && !(s[0].state & SET_ESB) && p[0].first &&
		      home && !p[1].esb && o[1].listen_channel == 7);
		for (i = 0; i < 2 * LINK_PERIODS_A_S; i++) {
			two(p, s, o);
			CHECK(!o[1].took_over && !p[1].first);
		}
		CHECK(s[1].send && !(s[1].state & SET_ESB) && !p[1].esb);
	}
	OK("the set given over to ESB: bit 14 said for two of the state's periods, each Povodok its channel; back after 10 s");
	printf("POVODOK LINK OK\n");
	/* the move at the application's word, 64/24: the first alone, a channel of the set's but its
	 * own, said in two SYNCs and taken by every unit in the same period */
	{
		struct povodok_link e, f;
		bool moved = false;

		cfg.number = 0;
		cfg.channel = 7;
		povodok_link_init(&f, &cfg);
		CHECK(povodok_link_move(&f, 30) == -1);              /* not the first yet */
		povodok_link_init(&e, &cfg);
		povodok_link_take(&e, 1, 1);
		run_silent(&e, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
		povodok_link_record(&e, true);
		CHECK(e.first && povodok_link_move(&e, 39) == -1 && povodok_link_move(&e, 7) == -1);
		CHECK(povodok_link_move(&e, 30) == 0);
		for (unsigned i = 0; i < 6 && !moved; i++) {
			povodok_link_period_begin(&e, &so);
			povodok_link_period_end(&e, LINK_NOISE_UNKNOWN, &po);
			moved = po.moved;
		}
		CHECK(moved && e.cfg.channel == 30 && po.listen_channel == 30);
		OK("the move at the application's word: the first's alone, every unit in one period");
	}

	/* the SYNC's flight: the first stamps Povodok 2's SYNCs, late by twice the flight less what 2
	 * takes off, jittered a tick either way, and says the mean in the periods numbered 2 mod 4
	 * once a second's stamps are in; a stamp 3 µs off and its own are left out; Povodok 2's stand
	 * moved, the mean follows it. Povodok 2 takes the flight from those periods and from the first
	 * alone: a second Povodok's SYNC in a period the first's was missed leaves it, the first gone
	 * a second clears it */
	{
		struct povodok_link g, h;
		const int32_t flight = 3, moved = 5;     /* ticks: 187,5 and 312,5 ns, 56 and 94 m */
		uint8_t said = 0;

		cfg.number = 0;
		cfg.channel = 7;
		povodok_link_init(&g, &cfg);
		run_silent(&g, LINK_TAKEOVER_REST_PERIODS, LINK_NOISE_UNKNOWN);
		povodok_link_record(&g, true);
		CHECK(g.first && povodok_link_flight(&g) == 0);
		for (unsigned i = 0; i < 2400; i++) {
			int32_t now = i < 400 ? flight : moved;

			povodok_link_period_begin(&g, &so);
			CHECK(so.send);
			if (so.period % STRELKA_POVODOKS_MAX != 2) {
				CHECK(so.flight == 0);
			} else {
				if (i < 400) {
					CHECK(so.flight == (i >= LINK_FLIGHT_FIRST ? flight : 0));
				}
				said = so.flight;
			}
			povodok_link_stamp(&g, 2, 2 * now - said + (int32_t)(i % 3) - 1);
			povodok_link_stamp(&g, 1, 1000);
			povodok_link_stamp(&g, 0, 5);
			povodok_link_period_end(&g, LINK_NOISE_UNKNOWN, &po);
			if (i == 399) {
				CHECK(said == flight);
			}
		}
		CHECK(said == moved && g.flight_n[1] == 0 && g.flight_n[0] == 0);

		cfg.number = 2;
		povodok_link_init(&h, &cfg);
		heard_flight(&h, 0, 1001, 0);
		CHECK(povodok_link_flight(&h) == 0);
		heard_flight(&h, 0, 1002, 3);                /* 2 mod 4: its own */
		CHECK(povodok_link_flight(&h) == 3);
		heard_flight(&h, 0, 1003, 5);                /* Povodok 3's */
		heard_flight(&h, 1, 1006, 0);                /* the first missed, the second heard */
		CHECK(!h.first && povodok_link_flight(&h) == 3);
		for (uint32_t period = 1007; period < 1007 + LINK_TAKEOVER_REST_PERIODS; period++) {
			heard_flight(&h, 1, period, 0);
		}
		CHECK(!h.first && povodok_link_flight(&h) == 0);
		OK("the flight measured by the first, followed, said in its periods, taken by the one it is for");
	}
	return 0;
}
