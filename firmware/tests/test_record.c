/* The record's blocks and a session composed, as RECORD.md has them: the CRC-32's check value,
 * every block's CRC read back, and two sessions written to the files named on the command line
 * — the first whole, the second with steps 5 and 77 lost — which tools/test_firmware.py holds
 * byte for byte against tools/test_record.py's make_session, Python building the same. */
#include <stdio.h>
#include <string.h>
#include "crc32.h"
#include "session.h"
#include "test.h"

static const char TABLE[] = "druh;kůň\njméno;Bára\nanamnéza;\"Od jara kulhá vpravo vpředu.\n"
			    "Klusána na kruhu.\"\nbelka;cip;misto\n1;0123456789ABCDEF;hlava, nánosník\n"
			    "2;0123456789ABCDF0;kohoutek\n";

static void to_file(const uint8_t block[REC_BLOCK_BYTES], void *ctx)
{
	CHECK(rec_block_ok(block));
	fwrite(block, 1, REC_BLOCK_BYTES, (FILE *)ctx);
}

static void write_session(const char *path, const int *lose, int lost)
{
	FILE *f = fopen(path, "wb");
	struct rec_calibration cal = { { 0 }, { 32768, 32768, 32768 }, { 0 }, 2550, 0 };
	struct rec_session cfg = {
		.session = 7, .belka = 3, .chip = 0x1122334455667788ull, .fw = { 0, 5, 0 },
		.build = { 1, 2, 3, 4 }, .rate_hz = 100, .n = 10, .acc_g = 16, .gyro_dps = 2000,
		.first_period = 1000, .time_us = 0, .time_period = 1000, .tz_min = 120,
		.clock = CLOCK_STEERED, .cal = &cal, .place = "hlava, nánosník",
		.name = "S00007B03.STRK",
	};
	struct session s;

	CHECK(f != NULL);
	session_begin(&s, &cfg, to_file, f);
	session_table(&s, TABLE, strlen(TABLE));
	for (int k = 0; k < 130; k++) {
		int m = k % 300;
		struct strelka_step step = { { (int16_t)m, (int16_t)-m, 16384 },
					     { 0, (int16_t)(100 * m), (int16_t)(100 - 100 * m) } };
		struct session_step_info info = { k % 7 - 3, (int8_t)(-60 - k % 5), (uint8_t)(k % 4),
						  (uint8_t)(1 + k % 4), false, false };

		for (int i = 0; i < lost; i++) {
			if (lose[i] == k) {
				for (int j = 0; j < 3; j++) {
					step.a[j] = step.g[j] = REC_STEP_MISSING;
				}
			}
		}
		session_step(&s, &step, &info, 2512, 3290);
	}
	session_end(&s, END_SYNC_STOP, 0);
	CHECK(s.steps == 130 && s.blocks == 6 && s.last_period == 1000 + 129 / 2);
	fclose(f);
}

int main(int argc, char **argv)
{
	uint8_t block[REC_BLOCK_BYTES];
	struct strelka_step steps[2] = { { { 1, 2, 3 }, { 4, 5, 6 } },
					 { { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING },
					   { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING } } };
	struct rec_data *d = (struct rec_data *)block;

	CHECK(argc == 3);
	CHECK(strelka_crc32("123456789", 9) == 0xCBF43926u);
	CHECK(strelka_crc32_more(strelka_crc32("1234", 4), "56789", 5) == 0xCBF43926u);
	OK("the CRC-32's check value");

	rec_data_block(block, 7, 3, 1001, 0, -70, 1, 0, steps, 2);
	CHECK(rec_block_ok(block) && d->missing == 1 && d->steps == 2 && d->acc_out == 1);
	block[100] ^= 1;
	CHECK(!rec_block_ok(block));
	CHECK(rec_table_blocks(0) == 1 && rec_table_blocks(502) == 1 && rec_table_blocks(503) == 2);
	OK("a data block, its CRC, the table's parts");

	write_session(argv[1], NULL, 0);
	{
		int lose[2] = { 5, 77 };

		write_session(argv[2], lose, 2);
	}
	OK("two sessions composed, for Python to compare");
	printf("RECORD OK\n");
	return 0;
}
