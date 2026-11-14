/* NIC-Strelka — the calibration from six faces (common/cal.c), on the host: a script from
 * tools/test_firmware.py, the unit of tools/test_cal.py with a face asked too early, one moved and
 * one out of turn among them, run as the Belka runs it; what it answers and computes printed for
 * Python to hold against tools/strelka_cal.py.
 *
 * The script, a line a deed: "b acc_fs gyro_fs" begun, "a face" asked, "s v0 … v5" a sample at
 * 100 Hz, "f" the face measured, "c" the calibration computed. */
#include <stdio.h>
#include <string.h>
#include "test.h"
#include "cal.h"

#define CLOCK_HZ 1000000u

int main(int argc, char **argv)
{
	static struct cal_run c;
	char line[256];
	uint64_t stamp = 0;
	FILE *f;

	CHECK(argc == 2);
	f = fopen(argv[1], "r");
	CHECK(f != NULL);
	while (fgets(line, sizeof(line), f)) {
		int v[9];

		if (line[0] == 'b') {
			CHECK(sscanf(line + 1, "%d %d", &v[0], &v[1]) == 2);
			cal_begin(&c, (uint16_t)v[0], (uint16_t)v[1]);
		} else if (line[0] == 'a') {
			CHECK(sscanf(line + 1, "%d", &v[0]) == 1);
			printf("ask %d\n", cal_ask(&c, (uint8_t)v[0]));
		} else if (line[0] == 's') {
			int16_t s[6], hg[3];
			int got = sscanf(line + 1, "%d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2],
					 &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]);

			CHECK(got == 6 || got == 9);         /* with the high-g channel's three */
			for (unsigned i = 0; i < 6; i++) {
				s[i] = (int16_t)v[i];
			}
			for (unsigned i = 0; i < 3; i++) {
				hg[i] = (int16_t)v[6 + i];
			}
			cal_sample(&c, s, got == 9 ? hg : NULL, stamp);
			stamp += CLOCK_HZ / 100u;
		} else if (line[0] == 'f') {
			printf("got %d\n", cal_finish(&c));
		} else if (line[0] == 'c') {
			struct cal_result r;

			CHECK(c.taken == CAL_FACES);
			cal_compute(&c, cal_rate_hz(&c, CLOCK_HZ), &r);
			printf("acc %d %d %d %d %d %d gscale %u %u %u gzero %d %d %d", r.acc[0],
			       r.acc[1], r.acc[2], r.acc[3], r.acc[4], r.acc[5], r.gscale[0],
			       r.gscale[1], r.gscale[2], r.gzero[0], r.gzero[1], r.gzero[2]);
			if (c.hg) {
				printf(" hg %d %d %d %d %d %d", r.hg[0], r.hg[1], r.hg[2], r.hg[3],
				       r.hg[4], r.hg[5]);
			}
			printf("\n");
		}
	}
	fclose(f);
	return 0;
}
