/* The record's blocks and a session composed, as RECORD.md has them: the CRC-32's check value,
 * every block's CRC read back, and five sessions written to the files named on the command line
 * — whole, with steps 5 and 77 lost, at 4 kHz with a table of two blocks, without a table, and
 * keeping the high-g channel — which tools/test_firmware.py holds byte for byte against
 * tools/strelka_record.py's composer. */
#include <stdio.h>
#include <string.h>
#include "crc32.h"
#include "session.h"
#include "test.h"

static const char TABLE[] = "kind;horse\nname;Bára\nnote;\"Od jara kulhá vpravo vpředu.\n"
			    "Klusána na kruhu.\"\nbelka;chip;code;place;kind\n"
			    "3;1122334455667788;HD;hlava, nánosník;Z\n2;0123456789ABCDF0;WI;kohoutek;K\n";

static void to_file(const uint8_t block[REC_BLOCK_BYTES], void *ctx)
{
	CHECK(rec_block_ok(block));
	fwrite(block, 1, REC_BLOCK_BYTES, (FILE *)ctx);
}

/* a session of `steps` steps composed into memory, the high-g channel kept or not, and every block
 * after the head and the table held to the counts a reader checks a file's own blocks by: each
 * number and the end's count at least what its place gives (record.h, rec_count_at) */
static uint8_t mem[64][REC_BLOCK_BYTES];
static unsigned mem_n;

static void to_mem(const uint8_t block[REC_BLOCK_BYTES], void *ctx)
{
	(void)ctx;
	CHECK(mem_n < 64);
	memcpy(mem[mem_n++], block, REC_BLOCK_BYTES);
}

static void check_counts(int steps, bool hg)
{
	struct rec_imu imu = { .acc_range = 16, .acc_step_ng = 488000, .gyro_range = 2000,
			       .gyro_step_ndps = 70000000, .hg_range = hg ? 320 : 0,
			       .hg_step_ng = 10417000 };
	struct rec_group groups[REC_GROUPS_MAX];
	struct rec_session cfg = { .session = 9, .belka = 1, .rate_hz = 100, .n = 1,
				   .first_period = 500, .sensor = "LSM6DSV320X", .groups = groups,
				   .hg = hg };
	struct session s;
	unsigned ends = 0;

	cfg.n_groups = rec_imu_groups(groups, &imu);
	mem_n = 0;
	session_begin(&s, &cfg, to_mem, NULL);
	session_table(&s, "belka;chip\n1;0\n", strlen("belka;chip\n1;0\n"));
	for (int k = 0; k < steps; k++) {
		struct strelka_step step = { { 1, 2, 3 }, { 4, 5, 6 } };
		struct session_step_info info = { 0, -60, 0, 1, false, false };
		int16_t high[3] = { 7, 8, 9 };

		session_step(&s, &step, hg ? high : NULL, &info, 2500, 3300);
	}
	session_end(&s, END_SYNC_STOP, 0);
	for (unsigned i = 2; i < mem_n; i++) {               /* past the head and the table */
		uint64_t at = i - 2;
		uint32_t v = (uint32_t)mem[i][4] | (uint32_t)mem[i][5] << 8 |
			     (uint32_t)mem[i][6] << 16 | (uint32_t)mem[i][7] << 24;
		uint32_t blocks = (uint32_t)mem[i][8] | (uint32_t)mem[i][9] << 8 |
				  (uint32_t)mem[i][10] << 16 | (uint32_t)mem[i][11] << 24;

		switch (mem[i][0]) {
		case REC_DATA:
		case REC_INFO:
			CHECK(v >= rec_count_at(hg, at));
			break;
		case REC_HG:
			CHECK(hg && v >= rec_count_hg(at));
			break;
		case REC_END:
			CHECK(blocks >= rec_count_end(hg, at) && i == mem_n - 1);
			ends++;
			break;
		default:
			CHECK(false);
		}
	}
	CHECK(ends == 1);
}

/* the table four times over, 584 B: two blocks of the table */
static char long_table[4 * sizeof(TABLE)];

static void write_session(const char *path, const int *lose, int lost, uint32_t rate, uint8_t n,
			  const char *table, bool hg)
{
	FILE *f = fopen(path, "wb");
	struct rec_calibration cal = { { 0 }, { 32768, 32768, 32768 }, { 0 }, { 16, 2000 },
				       { 96, -96, 97, -95, 98, -94 }, 320 };
	struct rec_imu imu = {
		.acc_range = 16, .acc_step_ng = 488000, .gyro_range = 2000,
		.gyro_step_ndps = 70000000, .hg_range = hg ? 320 : 0, .hg_step_ng = 10417000,
		.cal = &cal,
	};
	struct rec_group groups[REC_GROUPS_MAX];
	struct rec_session cfg = {
		.session = 7, .belka = 3, .chip = 0x1122334455667788ull, .fw = { 0, 5, 0 },
		.rate_hz = rate, .n = n, .first_period = 1000, .time_us = 0, .time_period = 1000,
		.tz_min = 120, .clock = CLOCK_STEERED, .sensor = "LSM6DSV320X", .cal_temp = 2550,
		.cal_time = 0, .groups = groups, .hg = hg,
	};
	struct session s;

	cfg.n_groups = rec_imu_groups(groups, &imu);

	CHECK(f != NULL);
	session_begin(&s, &cfg, to_file, f);
	if (table != NULL) {
		session_table(&s, table, strlen(table));
	}
	for (int k = 0; k < 130; k++) {
		int m = k % 300;
		struct strelka_step step = { { (int16_t)m, (int16_t)-m, 16384 },
					     { 0, (int16_t)(100 * m), (int16_t)(100 - 100 * m) } };
		struct session_step_info info = { k % 7 - 3, (int8_t)(-60 - k % 5), (uint8_t)(k % 4),
						  (uint8_t)(1 + k % 4), false, false };

		int16_t high[3] = { (int16_t)m, (int16_t)(2 * m), 96 };

		for (int i = 0; i < lost; i++) {
			if (lose[i] == k) {
				for (int j = 0; j < 3; j++) {
					step.a[j] = step.g[j] = high[j] = REC_STEP_MISSING;
				}
			}
		}
		session_step(&s, &step, hg ? high : NULL, &info, 2512, 3290);
	}
	session_end(&s, END_SYNC_STOP, 0);
	CHECK(s.steps == 130 && s.last_period == 1000 + 129 / (rate / 50));
	CHECK(rate != 100 || s.blocks == 6);
	fclose(f);
}

int main(int argc, char **argv)
{
	uint8_t block[REC_BLOCK_BYTES];
	struct strelka_step steps[2] = { { { 1, 2, 3 }, { 4, 5, 6 } },
					 { { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING },
					   { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING } } };
	struct rec_data *d = (struct rec_data *)block;

	CHECK(argc == 6);
	CHECK(strelka_crc32("123456789", 9) == 0xCBF43926u);
	CHECK(strelka_crc32_more(strelka_crc32("1234", 4), "56789", 5) == 0xCBF43926u);
	OK("the CRC-32's check value");

	rec_data_block(block, 7, 3, 1001, 0, -70, 1, 0, steps, 2);
	CHECK(rec_block_ok(block) && d->missing == 1 && d->steps == 2 && d->acc_out == 1);
	block[100] ^= 1;
	CHECK(!rec_block_ok(block));
	CHECK(rec_table_blocks(0) == 1 && rec_table_blocks(502) == 1 && rec_table_blocks(503) == 2);
	OK("a data block, its CRC, the table's parts");

	/* every length from 1 to 400 steps, the last cycle cut short anywhere: D I, D D I, D I H, D D
	 * I H, D D I H H and the cycle whole */
	for (int steps = 1; steps <= 400; steps++) {
		check_counts(steps, false);
		check_counts(steps, true);
	}
	OK("a reader's counts held by every block of sessions of 1 to 400 steps, the high-g kept or "
	   "not, the end of a last cycle cut short its own");

	write_session(argv[1], NULL, 0, 100, 10, TABLE, false);
	{
		int lose[2] = { 5, 77 };

		write_session(argv[2], lose, 2, 100, 10, TABLE, false);
		write_session(argv[5], lose, 2, 100, 10, TABLE, true);
	}
	for (int i = 0; i < 4; i++) {
		strcat(long_table, TABLE);
	}
	write_session(argv[3], NULL, 0, 4000, 1, long_table, false);
	write_session(argv[4], NULL, 0, 100, 10, NULL, false);
	OK("five sessions composed for Python: 100 Hz whole and with steps lost, 4 kHz with N 1 "
	   "and a table of two blocks, one without a table, and one keeping the high-g channel");
	printf("RECORD OK\n");
	return 0;
}
