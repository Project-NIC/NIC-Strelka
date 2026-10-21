/* A Belka's side of the inside link, period by period against scripted SYNCs (PRINCIPLE.md, The
 * inside link; FIRMWARE.md, A session): the record from the period after the first SYNC saying it
 * runs, stopped after five saying it does not, paused after the setting's seconds without any and
 * taken up again; the LIVE in every period with the live view on and once a second with it off,
 * trouble first; the item's values; the move counted down and every Belka moving in the same
 * period; the set looked for on the spare, then on every channel; a new Belka's window after a
 * CALL. */
#include <string.h>
#include "belka_link.h"
#include "test.h"

static struct belka_link l;
static struct belka_facts facts = {
	.cell_pct = 85, .temp_c = -3, .cell_mv = 3290, .card_free_pct = 90, .dropped = 2,
	.offset_ns = -7, .acc_out = 4, .gyro_out = 1,
};
static struct belka_live_out live;
static struct belka_period_out out;

static void sync(uint8_t povodok, uint32_t period, bool record, uint8_t code, uint8_t item,
		 uint8_t move, uint8_t channel, int8_t rssi)
{
	struct frame f = {
		.kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = povodok, .s0 = S0(code, item),
		.period = period, .state = SET_STATE(record, 20, move, channel),
	};

	belka_link_heard(&l, &f, rssi);
}

/* a period: the SYNCs given, then the slot and the end */
static void period_end(void)
{
	belka_link_slot(&l, &facts, &live);
	belka_link_period_end(&l, &facts, &out);
}

int main(void)
{
	struct belka_link_cfg cfg = { .number = 5, .chip = 0x0102030405060708ull,
				      .address = 0xA1B2C3D4u, .channel = 7 };
	uint32_t p = 1000;

	belka_link_init(&l, &cfg);
	CHECK(l.cfg.stop_s == LINK_STOP_S_DEFAULT);

	/* no SYNC yet: no period, no LIVE, waiting on the set's channel */
	period_end();
	CHECK(!live.send && out.state == BELKA_WAITING && out.listen_channel == 7 &&
	      !out.record_start && out.taken_povodok == REC_NO_POVODOK &&
	      out.signal_dbm == REC_NO_SIGNAL);

	/* the first SYNC, the live view off, in a period that is a multiple of 50: the time taken,
	 * the LIVE sent, and it says the state first, whatever is asked */
	sync(0, p, false, S0_LIVE_OFF, ITEM_CHANNEL, SET_MOVE_NONE, 12, -61);
	period_end();
	CHECK(belka_link_period(&l) == p + 1);
	CHECK(live.send && live.s0 == S0(S0_LIVE_OFF, ITEM_STATE) &&
	      live.state == ITEM_BYTES(BELKA_WAITING, FAULT_NONE));
	CHECK(out.taken_povodok == 0 && out.signal_dbm == -61 && out.syncs_heard == 1);
	CHECK(!out.record_start && out.state == BELKA_WAITING);
	CHECK(out.measure_channel == (5 + p + 1) % STRELKA_CHANNELS);
	p++;
	/* the live view off: no LIVE until the next multiple of 50 */
	for (; p % LINK_STATE_EVERY != 0; p++) {
		sync(0, p, false, S0_LIVE_OFF, ITEM_CHANNEL, SET_MOVE_NONE, 12, -61);
		period_end();
		CHECK(!live.send);
	}
	sync(0, p, false, S0_LIVE_OFF, ITEM_CHANNEL, SET_MOVE_NONE, 12, -61);
	belka_link_measured(&l, -93);
	period_end();
	CHECK(live.send && live.s0 == S0(S0_LIVE_OFF, ITEM_CHANNEL) &&
	      ITEM_HIGH(live.state) == (5 + p) % STRELKA_CHANNELS &&
	      (int8_t)ITEM_LOW(live.state) == -93);
	p++;
	OK("the time from the first SYNC, the state first, then once a second with the live view off");

	/* the record runs: from the period after the first SYNC saying so; the state changes, so
	 * the next LIVE says it; with the live view on, a LIVE in every period */
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -61);
	sync(1, p, false, S0_LIVE_OFF, ITEM_CHANNEL, SET_MOVE_NONE, 12, -70);  /* a period behind */
	period_end();
	CHECK(out.record_start && out.first_period == p + 1 && out.state == BELKA_RECORDING);
	CHECK(out.taken_povodok == 0 && out.syncs_heard == 2);
	CHECK(live.send && S0_LOW(live.s0) == ITEM_CHANNEL);   /* the LIVE before the state's change */
	p++;
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -61);
	period_end();
	CHECK(live.send && live.s0 == S0(S0_QUAT_50, ITEM_STATE) &&
	      live.state == ITEM_BYTES(BELKA_RECORDING, FAULT_NONE));
	CHECK(!out.record_start && out.state == BELKA_RECORDING);
	p++;
	/* the items asked, their values */
	static const uint8_t items[] = { ITEM_CELL_TEMP, ITEM_CELL_MV, ITEM_CARD, ITEM_OFFSET,
					 ITEM_STATE, ITEM_OUT };
	static const uint16_t want[] = { ITEM_BYTES(85, -3), 3290, ITEM_BYTES(90, 2),
					 (uint16_t)-7, ITEM_BYTES(BELKA_RECORDING, 0),
					 ITEM_BYTES(4, 1) };
	for (unsigned i = 0; i < sizeof(items); i++, p++) {
		sync(0, p, true, S0_QUAT_50, items[i], SET_MOVE_NONE, 12, -61);
		period_end();
		CHECK(live.send && live.s0 == S0(S0_QUAT_50, items[i]) && live.state == want[i]);
	}
	/* item 4: the SYNCs' mean signal and the periods without one in the last second */
	for (unsigned i = 0; i < LINK_PERIODS_A_S; i++, p++) {
		if (i % 10 != 9) {
			sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, i < 25 ? -60 : -70);
		}
		period_end();
	}
	sync(0, p, true, S0_QUAT_50, ITEM_SYNCS, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(live.send && S0_LOW(live.s0) == ITEM_SYNCS);
	CHECK((int8_t)ITEM_HIGH(live.state) <= -64 && (int8_t)ITEM_HIGH(live.state) >= -66 &&
	      ITEM_LOW(live.state) == 5);
	p++;
	/* a fault appears: the next LIVE says the state, whatever is asked */
	facts.fault = FAULT_CELL_LOW;
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(S0_LOW(live.s0) == ITEM_CHANNEL);
	p++;
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(live.s0 == S0(S0_QUAT_50, ITEM_STATE) &&
	      live.state == ITEM_BYTES(BELKA_RECORDING, FAULT_CELL_LOW));
	facts.fault = FAULT_NONE;
	p++;
	OK("the record from the period after the SYNC, the items' values, trouble first");

	/* the stop: five SYNCs saying the record does not run — four Povodoks' in one period and
	 * one more; one saying it runs in between starts the count again */
	for (unsigned k = 0; k < 4; k++) {
		sync((uint8_t)k, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	}
	period_end();
	CHECK(!out.record_stop && out.state == BELKA_RECORDING);
	p++;
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	sync(1, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(!out.record_stop);                         /* one counted, the second Povodok's */
	p++;
	for (unsigned i = 0; i < 3; i++, p++) {
		sync(0, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
		period_end();
		CHECK(!out.record_stop);
	}
	sync(0, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(out.record_stop && out.stop_why == END_SYNC_STOP && out.state == BELKA_WAITING);
	CHECK(live.send);                                /* the LIVE goes on, waiting */
	p++;
	sync(0, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(!out.record_stop && !out.record_start);
	p++;
	OK("the stop after five SYNCs, the count started again by one saying it runs");

	/* the pause: the record runs, then no SYNC — on its own clock after two periods, the spare
	 * tried every other period, after a second each channel a period, after ten seconds the
	 * pause and the LIVE stopped; a SYNC heard where it listens takes the record up again */
	sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	period_end();
	CHECK(out.record_start && out.first_period == p + 1);
	p++;
	{
		uint32_t silent = 0, pause_at = 0;
		uint8_t rr = 0;

		while (!out.pause) {
			period_end();
			silent++;
			p++;
			if (silent < LINK_ALONE_PERIODS) {
				CHECK(out.state == BELKA_RECORDING && out.listen_channel == 7);
			} else if (silent < LINK_LOST_PERIODS) {
				CHECK(out.state == BELKA_RECORDING_ALONE);
				CHECK(out.listen_channel == (silent % 2 == 0 ? 12 : 7));
			} else {
				/* the set's own channel every other period, the others in turn */
				CHECK(out.pause || out.state == BELKA_RECORDING_SEARCHING);
				if (silent % 2 == 1) {
					CHECK(out.listen_channel == 7);
				} else {
					uint8_t next = (uint8_t)((rr + 1) % STRELKA_SET_CHANNELS);

					next = next == 7 ? 8 : next;
					CHECK(out.listen_channel != 7 && (!rr || out.listen_channel == next));
					rr = out.listen_channel;
				}
			}
			CHECK(live.send == !out.pause || silent >= LINK_PERIODS_A_S * 10);
			CHECK(!out.record_stop);
		}
		pause_at = silent;
		CHECK(pause_at == LINK_STOP_S_DEFAULT * LINK_PERIODS_A_S);
		CHECK(out.state == BELKA_PAUSED);
		period_end();
		CHECK(!live.send && out.state == BELKA_PAUSED && !out.pause);
		CHECK(out.listen_channel == 7);                  /* listening on, the set's own */
		p++;
		/* the set found on the channel listened to — here a sweep channel: the set's now */
		belka_link_period_end(&l, &facts, &out);
		CHECK(out.listen_channel == rr + 1 || out.listen_channel == 8);
		rr = out.listen_channel;
		p++;
		sync(2, p + 3, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -75);
		period_end();
		CHECK(out.resume && !out.record_start && out.state == BELKA_RECORDING);
		CHECK(l.cfg.channel == rr && out.listen_channel == rr);
		CHECK(belka_link_period(&l) == p + 4);  /* the time from any SYNC */
		p += 4;
		CHECK(out.taken_povodok == 2 && out.signal_dbm == -75);
	}
	OK("alone, the spare, every channel, the pause after ten seconds, the record taken up");

	/* a SYNC saying the record does not run, while paused: the stop, the end written */
	for (unsigned i = 0; i < LINK_STOP_S_DEFAULT * LINK_PERIODS_A_S; i++, p++) {
		period_end();
	}
	CHECK(l.paused);
	for (unsigned i = 0; i < LINK_STOP_SYNCS; i++) {
		sync(0, p, false, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 12, -60);
	}
	period_end();
	CHECK(out.record_stop && out.stop_why == END_SYNC_STOP && !out.pause &&
	      out.state == BELKA_WAITING);
	p++;
	OK("paused, then the SYNCs say stop: the end");

	/* the move: said as 2 in period A and 1 in A + 1, every Belka on the new channel from
	 * A + 2 — one that heard both, one that heard the first alone, one the second alone */
	{
		uint8_t ch = l.cfg.channel, spare = (uint8_t)((ch + 5) % STRELKA_SET_CHANNELS);
		struct belka_link b1, b2;
		struct belka_period_out o1, o2;

		sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, spare, -60);
		period_end();
		p++;
		b1 = l;
		b2 = l;
		sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_TWO, spare, -60);
		period_end();
		CHECK(out.listen_channel == ch);
		belka_link_slot(&b1, &facts, &live);
		belka_link_period_end(&b1, &facts, &o1);
		{
			struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 1,
					   .s0 = S0(S0_QUAT_50, ITEM_CHANNEL), .period = p,
					   .state = SET_STATE(1, 20, SET_MOVE_TWO, spare) };
			belka_link_heard(&b2, &f, -60);
		}
		belka_link_slot(&b2, &facts, &live);
		belka_link_period_end(&b2, &facts, &o2);
		p++;
		sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NEXT, spare, -60);
		period_end();
		CHECK(out.listen_channel == spare && l.cfg.channel == spare);
		{
			struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 3,
					   .s0 = S0(S0_QUAT_50, ITEM_CHANNEL), .period = p,
					   .state = SET_STATE(1, 20, SET_MOVE_NEXT, spare) };
			belka_link_heard(&b1, &f, -60);
		}
		belka_link_slot(&b1, &facts, &live);
		belka_link_period_end(&b1, &facts, &o1);
		CHECK(o1.listen_channel == spare);
		belka_link_slot(&b2, &facts, &live);
		belka_link_period_end(&b2, &facts, &o2);   /* heard nothing in A + 1 */
		CHECK(o2.listen_channel == spare);
		p++;
		/* the new channel's SYNC heard: a new spare named; the Belka that heard neither
		 * finds the set on the spare at its second miss */
		b1 = l;
		belka_link_slot(&b1, &facts, &live);
		belka_link_period_end(&b1, &facts, &o1);     /* A + 2: nothing heard, 1 miss */
		CHECK(o1.listen_channel == spare && o1.state == BELKA_RECORDING);
		belka_link_period_end(&b1, &facts, &o1);     /* A + 3: 2 misses, the spare — which
							      * is the channel it is on already */
		CHECK(o1.listen_channel == spare && o1.state == BELKA_RECORDING_ALONE);
		sync(0, p, true, S0_QUAT_50, ITEM_CHANNEL, SET_MOVE_NONE, 3, -60);
		period_end();
		CHECK(l.spare == 3 && out.listen_channel == spare && !l.move_pending);
		p++;
	}
	/* a repeater's SYNC in the same period carries the last period's reading: it neither moves
	 * the time nor undoes a move pending; the time is the taken SYNC's */
	{
		struct belka_link b = l;
		struct belka_period_out o;
		uint8_t ch = b.cfg.channel, spare2 = (uint8_t)((ch + 9) % STRELKA_SET_CHANNELS);
		uint32_t q = belka_link_period(&b);
		struct frame first = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 0,
				       .s0 = S0(S0_QUAT_50, ITEM_CHANNEL), .period = q,
				       .state = SET_STATE(1, 20, SET_MOVE_TWO, spare2) };
		struct frame stale = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 1,
				       .s0 = S0(S0_QUAT_50, ITEM_CHANNEL), .period = q + 200,
				       .state = SET_STATE(1, 20, SET_MOVE_NONE, ch) };

		belka_link_heard(&b, &first, -60);
		belka_link_heard(&b, &stale, -60);
		CHECK(belka_link_period(&b) == q && b.spare == spare2 && b.move_pending);
		belka_link_slot(&b, &facts, &live);
		belka_link_period_end(&b, &facts, &o);
		CHECK(o.taken_povodok == 0 && o.syncs_heard == 2 && belka_link_period(&b) == q + 1);
		belka_link_period_end(&b, &facts, &o);     /* nothing heard: the move all the same */
		CHECK(o.listen_channel == spare2 && b.cfg.channel == spare2);
	}
	OK("the move said twice, every Belka on the new channel in the same period");

	/* a new Belka: CALLs alone, its HERE's window in the period its draw names */
	{
		struct belka_link n;
		struct belka_link_cfg ncfg = { .number = 0, .chip = 0x1122334455667788ull };
		uint16_t draw = search_draw(ncfg.chip, 3);
		uint32_t round = 475u * 20u;
		struct frame call = { .kind = FRAME_CALL, .sender = WHO_POVODOK, .number = 1,
				      .s0 = S0(S0_CALL, 3), .period = round + SEARCH_PERIOD(draw),
				      .state = SET_STATE(1, 20, SET_MOVE_NONE, 7) };
		struct frame other = call;

		belka_link_init(&n, &ncfg);
		belka_link_period_end(&n, &facts, &out);
		CHECK(out.listen_channel == STRELKA_SEARCH_CHANNEL && belka_link_here(&n) == -1);
		other.period = round + (SEARCH_PERIOD(draw) + 1) % SEARCH_PERIODS;
		belka_link_heard(&n, &other, -70);
		CHECK(belka_link_here(&n) == -1);
		belka_link_slot(&n, &facts, &live);
		CHECK(!live.send);
		belka_link_period_end(&n, &facts, &out);
		belka_link_heard(&n, &call, -70);
		CHECK(belka_link_here(&n) == (int16_t)SEARCH_WINDOW(draw));
		other.s0 = S0(S0_CALL, 4);                /* another attempt: another draw */
		other.period = call.period;
		belka_link_heard(&n, &other, -70);
		CHECK(search_draw(ncfg.chip, 4) / SEARCH_WINDOWS == SEARCH_PERIOD(draw) ||
		      belka_link_here(&n) == -1);
		belka_link_period_end(&n, &facts, &out);
		CHECK(belka_link_here(&n) == -1);
		/* taken: its number, the set's address and channel */
		belka_link_join(&n, 40, 0xA1B2C3D4u, 7);
		belka_link_period_end(&n, &facts, &out);
		CHECK(out.listen_channel == 7 && !live.send);
		{
			struct frame f = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 0,
					   .s0 = S0(S0_QUAT_50, ITEM_CHANNEL), .period = 5000,
					   .state = SET_STATE(0, 20, SET_MOVE_NONE, 12) };
			belka_link_heard(&n, &f, -60);
		}
		belka_link_slot(&n, &facts, &live);
		CHECK(live.send && live.s0 == S0(S0_QUAT_50, ITEM_STATE));
		belka_link_period_end(&n, &facts, &out);
		CHECK(belka_link_period(&n) == 5001 && out.measure_channel == (40 + 5001) % 40);
	}
	OK("a new Belka's HERE in its window, then taken into the set");

	printf("BELKA LINK OK\n");
	return 0;
}
