/* The live view's message to the phone (PHONE.md, The live view): built from the first Povodok's
 * gathering — its own hearing and a PASS — read back entry by entry, and split into two messages
 * when 47 Belkas raw at 100 Hz do not fit in 1 400 B; the bytes written to a file for Python to
 * read with tools/live_view.py, and a few quaternions packed for Python to unpack. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "live_msg.h"
#include "quat.h"
#include "test.h"

static void live_from(struct povodok_link *l, uint8_t belka, uint8_t code, uint8_t item,
		      uint16_t state, int8_t rssi)
{
	uint8_t body[LIVE_BODY_MAX];
	struct frame f = {
		.kind = FRAME_LIVE, .sender = WHO_BELKA, .number = belka, .s0 = S0(code, item),
		.period = l->period, .state = state, .body = body, .body_len = live_body_bytes(code),
	};

	for (unsigned i = 0; i < sizeof(body); i++) {
		body[i] = (uint8_t)(belka * 3 + i);
	}
	povodok_link_heard(l, &f, rssi);
}

static void first_up(struct povodok_link *l, uint8_t channel)
{
	struct povodok_link_cfg cfg = { .number = 0, .address = 0xA1B2C3D4u, .channel = channel };
	struct povodok_sync_out so;
	struct povodok_period_out po;

	povodok_link_init(l, &cfg);
	for (unsigned i = 0; i < LINK_TAKEOVER_PERIODS; i++) {
		povodok_link_period_begin(l, &so);
		povodok_link_period_end(l, LINK_NOISE_UNKNOWN, &po);
	}
	CHECK(l->first);
}

int main(int argc, char **argv)
{
	static struct povodok_link l;
	struct live_msg msgs[LIVE_MSG_MESSAGES];
	struct povodok_sync_out so;
	struct live_msg_head h;
	struct live_msg_entry e;
	const uint8_t *steps;
	size_t steps_len, at;
	FILE *f;

	CHECK(argc == 2);
	f = fopen(argv[1], "w");
	CHECK(f != NULL);

	/* two Belkas heard, one passed by Povodok 2, one taken and silent; the first's SYNC the
	 * head's state; Povodoks 0 and 2 heard */
	first_up(&l, 7);
	povodok_link_record(&l, true);
	povodok_link_live(&l, S0_RAW_100);
	for (unsigned k = 1; k <= 4; k++) {
		povodok_link_take(&l, (uint8_t)k, 0x100u + k);
	}
	povodok_link_period_begin(&l, &so);
	CHECK(so.send && l.sync_state == so.state && l.sync_s0 == so.s0);
	live_from(&l, 1, S0_RAW_100, ITEM_CHANNEL, ITEM_BYTES(17, -92), -60);
	live_from(&l, 3, S0_RAW_100, ITEM_STATE, ITEM_BYTES(BELKA_RECORDING, FAULT_CELL_LOW), -70);
	{
		struct frame sync = { .kind = FRAME_SYNC, .sender = WHO_POVODOK, .number = 2,
				      .s0 = so.s0, .period = l.period,
				      .state = SET_STATE(1, 9, SET_MOVE_NONE, 12) };
		struct strelka_packet p;
		struct frame pass;
		uint8_t body[LIVE_BODY_MAX];

		povodok_link_heard(&l, &sync, -50);
		for (unsigned i = 0; i < sizeof(body); i++) {
			body[i] = (uint8_t)(2 * 3 + i);
		}
		frame_pass_begin(&p, 2, l.period, 0);
		CHECK(frame_pass_add(&p, 2, S0(S0_RAW_100, ITEM_CHANNEL), ITEM_BYTES(5, -95), body));
		CHECK(frame_read(&p, frame_ram_len(&p), &pass));
		povodok_link_heard(&l, &pass, -50);
	}
	CHECK(live_msg_from_link(&l, 7, msgs) == 1);
	CHECK(msgs[0].len == 21 + 3 * (5 + 24));
	CHECK(live_msg_read_head(msgs[0].bytes, msgs[0].len, &h));
	CHECK(h.kind == 1 && h.version == 1 && h.session == 7 && h.period == l.period &&
	      h.state == so.state && h.s0 == S0(S0_RAW_100, ITEM_CHANNEL) && h.povodoks == 0x05 &&
	      h.povodok_state[0] == so.state && h.povodok_state[1] == 0 &&
	      h.povodok_state[2] == SET_STATE(1, 9, SET_MOVE_NONE, 12) && h.belkas == 3);
	at = sizeof(h);
	CHECK(live_msg_next(msgs[0].bytes, msgs[0].len, &at, &e, &steps, &steps_len));
	CHECK(e.belka == 1 && e.s0 == S0(S0_RAW_100, ITEM_CHANNEL) && e.by == 0 &&
	      e.state == ITEM_BYTES(17, -92) && steps_len == 24 && steps[0] == 3 && steps[23] == 26);
	CHECK(live_msg_next(msgs[0].bytes, msgs[0].len, &at, &e, &steps, &steps_len));
	CHECK(e.belka == 2 && e.by == 2 && e.state == ITEM_BYTES(5, -95) && steps[0] == 6);
	CHECK(live_msg_next(msgs[0].bytes, msgs[0].len, &at, &e, &steps, &steps_len));
	CHECK(e.belka == 3 && e.by == 0 && e.state == ITEM_BYTES(BELKA_RECORDING, FAULT_CELL_LOW));
	CHECK(!live_msg_next(msgs[0].bytes, msgs[0].len, &at, &e, &steps, &steps_len) &&
	      at == msgs[0].len);
	fprintf(f, "msg ");
	for (size_t i = 0; i < msgs[0].len; i++) {
		fprintf(f, "%02x", msgs[0].bytes[i]);
	}
	fprintf(f, "\n");
	OK("a period's message: the head, three Belkas, one of them passed");

	/* 47 Belkas raw at 100 Hz fill one message of 1 400 B; 48 need two of the same head */
	{
		struct povodok_period_out po;

		povodok_link_period_end(&l, LINK_NOISE_UNKNOWN, &po);
		for (unsigned k = 1; k <= 48; k++) {
			povodok_link_take(&l, (uint8_t)k, 0x100u + k);
		}
		povodok_link_period_begin(&l, &so);
		for (unsigned k = 1; k <= 47; k++) {
			live_from(&l, (uint8_t)k, S0_RAW_100, ITEM_CHANNEL, 0, -60);
		}
		CHECK(live_msg_from_link(&l, 7, msgs) == 1 && msgs[0].len == 21 + 47 * 29);
		live_from(&l, 48, S0_RAW_100, ITEM_CHANNEL, 0, -60);
		CHECK(live_msg_from_link(&l, 7, msgs) == 2 && msgs[0].len == 21 + 47 * 29 &&
		      msgs[1].len == 21 + 29);
		CHECK(live_msg_read_head(msgs[1].bytes, msgs[1].len, &h) && h.belkas == 1 &&
		      h.period == l.period);
		at = sizeof(h);
		CHECK(live_msg_next(msgs[1].bytes, msgs[1].len, &at, &e, &steps, &steps_len) &&
		      e.belka == 48);
		for (unsigned i = 0; i < 2; i++) {
			fprintf(f, "msg ");
			for (size_t j = 0; j < msgs[i].len; j++) {
				fprintf(f, "%02x", msgs[i].bytes[j]);
			}
			fprintf(f, "\n");
		}
		/* with the live view off, the state alone: 5 B an entry */
		povodok_link_period_end(&l, LINK_NOISE_UNKNOWN, &po);
		povodok_link_live(&l, S0_LIVE_OFF);
		while (l.period % LINK_STATE_EVERY != 0) {
			povodok_link_period_begin(&l, &so);
			povodok_link_period_end(&l, LINK_NOISE_UNKNOWN, &po);
		}
		povodok_link_period_begin(&l, &so);
		live_from(&l, 5, S0_LIVE_OFF, ITEM_CELL_TEMP, ITEM_BYTES(80, 21), -60);
		CHECK(live_msg_from_link(&l, 7, msgs) == 1 && msgs[0].len == 21 + 5);
		fprintf(f, "msg ");
		for (size_t j = 0; j < msgs[0].len; j++) {
			fprintf(f, "%02x", msgs[0].bytes[j]);
		}
		fprintf(f, "\n");
	}
	OK("47 Belkas in one message, the 48th in a second; the state alone in 5 B");

	/* quaternions packed, for Python's unpacking: the smallest three */
	{
		const float qs[][4] = {
			{ 1.0f, 0.0f, 0.0f, 0.0f }, { 0.7071068f, 0.7071068f, 0.0f, 0.0f },
			{ 0.5f, -0.5f, 0.5f, -0.5f }, { 0.1f, 0.2f, -0.3f, 0.927362f },
			{ -0.6f, 0.0f, 0.8f, 0.0f },
		};

		for (unsigned i = 0; i < 5; i++) {
			uint32_t v = quat_pack(qs[i]);
			float back[4];

			quat_unpack(v, back);
			for (int j = 0; j < 4; j++) {
				CHECK(fabsf(fabsf(back[j]) - fabsf(qs[i][j])) < 0.003f);
			}
			fprintf(f, "quat %08x %.7f %.7f %.7f %.7f\n", v, (double)qs[i][0],
				(double)qs[i][1], (double)qs[i][2], (double)qs[i][3]);
		}
	}
	fclose(f);
	printf("LIVE OK\n");
	return 0;
}
