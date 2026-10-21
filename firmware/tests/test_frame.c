/* The inside link's packets built and read back (PRINCIPLE.md, The inside link): each kind, its
 * size on the air as the page has it, a PASS filled to its last byte, the SYNC's signature made
 * and checked, the set's and a Belka's state, the search's draw, a turn in 4 B. */
#include <math.h>
#include <string.h>
#include "frame.h"
#include "quat.h"
#include "timetable.h"
#include "test.h"

int ccm_soft_self_test(void);

int main(void)
{
	struct strelka_packet p;
	struct frame f;
	uint8_t key[KEY_BYTES] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	uint8_t sig[4] = { 0 }, missed[8] = { 0 };
	struct strelka_step raw[2] = { { { 1, 2, 3 }, { 4, 5, 6 } }, { { 7, 8, 9 }, { 10, 11, 12 } } };
	uint32_t quats[2] = { 0x11111111u, 0x22222222u };
	size_t n, at;
	struct pass_item item;
	uint16_t state;

	CHECK(ccm_soft_self_test() == 0);

	/* a SYNC: 22 B on the air, its signature checked, and a change caught */
	state = SET_STATE(1, 17, SET_MOVE_TWO, 12);
	n = frame_sync(&p, 2, S0_RAW_100, ITEM_STATE, 123456, state, sig);
	frame_sync_sign(&p, key, 0xA1B2C3D4u);
	CHECK(n == 9 + 4 && STRELKA_AIR_BYTES(n - 9) == 22);
	CHECK(frame_read(&p, n, &f) && f.kind == FRAME_SYNC && f.sender == WHO_POVODOK &&
	      f.number == 2 && f.period == 123456 && f.body_len == 4);
	CHECK(S0_HIGH(f.s0) == S0_RAW_100 && S0_LOW(f.s0) == ITEM_STATE);
	CHECK((f.state & SET_RECORD) && SET_MARGIN(f.state) == 17 && SET_MOVE(f.state) == 2 &&
	      SET_CHANNEL(f.state) == 12);
	CHECK(frame_sync_check(&p, key, 0xA1B2C3D4u));
	p.period++;
	CHECK(!frame_sync_check(&p, key, 0xA1B2C3D4u));
	p.period--;
	CHECK(!frame_sync_check(&p, key, 0xA1B2C3D5u));
	CHECK(!frame_read(&p, n + 1, &f) && !frame_read(&p, n - 1, &f));
	OK("a SYNC, its signature, the set's state");

	/* the measurement's reading of the state */
	state = SET_STATE(0, (-64 + 100) / 2, SET_MEASUREMENT, 39);
	CHECK(SET_NOISE_DBM(state) == -64 && SET_MOVE(state) == 3 && SET_CHANNEL(state) == 39);

	/* a LIVE at each live view: 18, 22, 26, 30, 42 B on the air */
	n = frame_live(&p, 63, S0_LIVE_OFF, ITEM_CELL_TEMP, 7, ITEM_BYTES(85, -3), NULL);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 18 && frame_read(&p, n, &f) && f.kind == FRAME_LIVE &&
	      f.number == 63 && f.body_len == 0);
	CHECK(ITEM_HIGH(f.state) == 85 && (int8_t)ITEM_LOW(f.state) == -3);
	n = frame_live(&p, 1, S0_QUAT_50, 0, 7, 0, quats);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 22 && frame_read(&p, n, &f) && f.body_len == 4);
	n = frame_live(&p, 1, S0_QUAT_100, 0, 7, 0, quats);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 26);
	n = frame_live(&p, 1, S0_RAW_50, 0, 7, 0, raw);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 30);
	n = frame_live(&p, 1, S0_RAW_100, 0, 7, 0, raw);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 42 && n == 9 + 24 && frame_read(&p, n, &f) &&
	      memcmp(f.body, raw, 24) == 0);
	p.who = WHO(WHO_BELKA, 0);
	CHECK(!frame_read(&p, n, &f));           /* a Belka numbered 0 sends HEREs alone */
	p.who = 0x40 | 5;
	CHECK(!frame_read(&p, n, &f));           /* the sender 1 is free */
	OK("a LIVE at every live view");

	/* a MISS and a HERE */
	miss_set(missed, 1);
	miss_set(missed, 63);
	miss_set(missed, 64);
	CHECK(miss_has(missed, 1) && miss_has(missed, 63) && !miss_has(missed, 2) &&
	      !miss_has(missed, 0) && !miss_has(missed, 64));
	n = frame_miss(&p, 2, 9, 0, missed);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 26 && frame_read(&p, n, &f) && f.kind == FRAME_MISS &&
	      f.number == 2 && miss_has(f.body, 63));
	n = frame_here(&p, 0x0102030405060708ull, 9);
	CHECK(STRELKA_AIR_BYTES(n - 9) == 26 && frame_read(&p, n, &f) && f.kind == FRAME_HERE &&
	      f.body[0] == 8 && f.body[7] == 1);
	n = frame_call(&p, 0, 15, 9, 0, sig);
	CHECK(frame_read(&p, n, &f) && f.kind == FRAME_CALL && S0_LOW(f.s0) == 15);
	OK("a MISS, a HERE, a CALL");

	/* a PASS filled: 8 Belkas at raw 100 Hz, 28 B each, in a body of 249; 20 as quaternions at
	 * 100 Hz, 12 B each */
	n = frame_pass_begin(&p, 1, 9, 0);
	for (int i = 1; i <= 8; i++) {
		CHECK(frame_pass_add(&p, (uint8_t)i, S0(S0_RAW_100, ITEM_CELL_MV), 3300, raw));
	}
	CHECK(!frame_pass_add(&p, 9, S0(S0_RAW_100, 0), 0, raw));
	CHECK(frame_pass_add(&p, 9, S0(S0_LIVE_OFF, ITEM_STATE), ITEM_BYTES(4, 0), NULL));
	n = frame_ram_len(&p);
	CHECK(n == 9 + 8 * 28 + 4 && STRELKA_AIR_BYTES(n - 9) <= 267);
	CHECK(frame_read(&p, n, &f) && f.kind == FRAME_PASS);
	at = 0;
	for (int i = 1; i <= 9; i++) {
		CHECK(frame_pass_next(&f, &at, &item) && item.belka == i);
		CHECK(i == 9 ? item.steps_len == 0 : (item.steps_len == 24 && item.state == 3300));
	}
	CHECK(!frame_pass_next(&f, &at, &item));
	n = frame_pass_begin(&p, 3, 9, 0);
	for (int i = 1; i <= 20; i++) {
		CHECK(frame_pass_add(&p, (uint8_t)i, S0(S0_QUAT_100, 0), 0, quats));
	}
	CHECK(!frame_pass_add(&p, 21, S0(S0_QUAT_100, 0), 0, quats));
	OK("a PASS, 8 raw or 20 quaternion Belkas a packet");

	/* the search's draw: SplitMix64's numbers as Python gives them for this ID; two IDs that
	 * draw one number at one attempt draw apart at the next */
	CHECK(search_draw(0x0102030405060708ull, 0) == 0xD409 &&
	      search_draw(0x0102030405060708ull, 1) == 0x6571 &&
	      search_draw(0x0102030405060708ull, 15) == 0xE0B7);
	{
		uint64_t a = 0x124, b;
		unsigned apart = 0, pairs = 0;

		for (b = 0x9000; pairs < 8; b++) {
			if (search_draw(b, 0) == search_draw(a, 0)) {
				pairs++;
				apart += search_draw(b, 1) != search_draw(a, 1);
			}
		}
		CHECK(apart >= 7);
	}
	CHECK(SEARCH_PERIOD(65535) < SEARCH_PERIODS && SEARCH_WINDOW(65535) < SEARCH_WINDOWS);
	OK("the search's draw");

	/* the timetable, as PRINCIPLE.md computes it */
	CHECK(TT_SYNC_AT_US(0) == 30 && TT_SYNC_AT_US(3) == 444 && TT_SLOT_AT_US(1) == 582 &&
	      TT_SLOT_AT_US(63) == 582 + 62 * 204 && TT_PASS_AT_US(3) + TT_PASS_WINDOW_US <=
	      TT_MEASURE_AT_US);
	OK("the period's timetable");

	/* a turn in 4 B: a few quaternions packed and unpacked within 0,003 a component */
	{
		const float q[][4] = { { 1, 0, 0, 0 }, { 0.5f, 0.5f, 0.5f, 0.5f },
				       { -0.7f, 0.1f, 0.7f, 0.1f }, { 0.2f, -0.9f, 0.3f, 0.1f } };

		for (int i = 0; i < 4; i++) {
			float in[4], out[4], norm = 0, worst = 0;

			for (int j = 0; j < 4; j++) {
				norm += q[i][j] * q[i][j];
			}
			norm = sqrtf(norm);
			for (int j = 0; j < 4; j++) {
				in[j] = q[i][j] / norm;
			}
			quat_unpack(quat_pack(in), out);
			/* q and −q are one turn: compared with the sign that fits */
			float sign = (in[0] * out[0] + in[1] * out[1] + in[2] * out[2] +
				      in[3] * out[3]) < 0 ? -1.0f : 1.0f;

			for (int j = 0; j < 4; j++) {
				float d = fabsf(in[j] - sign * out[j]);

				worst = d > worst ? d : worst;
			}
			CHECK(worst < 0.003f);
		}
	}
	OK("a turn in 4 B");
	printf("FRAME OK\n");
	return 0;
}
