/* The calibration on a step, common/cal.h's cal_gain, as the Belka puts the live view's steps
 * (PRINCIPLE.md, The LIVE): a file of lines — "i 1 acc×6 gscale×3 gzero×3 range×2 acc_range
 * gyro_range acc_step_ng", or "i 0" for none; "s" and a step's six values — each step answered
 * by its six calibrated, for tools/test_firmware.py to hold against tools/strelka_record.py's
 * live_step. Before the file, the cases a reader can check by eye: a gyro reading 0,7 % high
 * brought down, not up; a step lost left lost; the ends held. */
#include <stdio.h>
#include "cal.h"
#include "test.h"

int main(int argc, char **argv)
{
	struct cal_gain g;
	struct strelka_step in = { { 0, 0, 0 }, { 1000, -1000, 0 } }, out;
	int16_t acc[6] = { 2048, -2048, 2048, -2048, 2048, -2048 };
	uint16_t gscale[3] = { 33000, 33000, 32768 };   /* the gyro sees 33 000 / 32 768 of a turn */
	int16_t gzero[3] = { 0, 0, 0 };
	char line[256];
	FILE *f;

	cal_gain_init(&g, acc, gscale, gzero, NULL, 16, 2000, 488281);
	cal_gain_step(&g, &in, &out);
	CHECK(out.g[0] == 993 && out.g[1] == -993);       /* 1 000 × 32 768 / 33 000 */
	in = (struct strelka_step){ { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING },
				    { REC_STEP_MISSING, REC_STEP_MISSING, REC_STEP_MISSING } };
	cal_gain_step(&g, &in, &out);
	CHECK(out.a[0] == REC_STEP_MISSING && out.g[2] == REC_STEP_MISSING);
	in = (struct strelka_step){ { 32767, -32767, 0 }, { 32767, 0, 0 } };
	cal_gain_init(&g, acc, (const uint16_t[3]){ 32000, 32768, 32768 },
		      (const int16_t[3]){ -200, 0, 0 }, NULL, 16, 2000, 488281);
	cal_gain_step(&g, &in, &out);
	CHECK(out.g[0] == 32767);
	cal_gain_init(&g, NULL, NULL, NULL, NULL, 16, 2000, 488281);
	cal_gain_step(&g, &in, &out);
	CHECK(out.a[1] == -32767 && out.g[0] == 32767);   /* none: a step as it is */
	OK("a gyro's scale brought down, a step lost kept, the ends held, none left alone");

	CHECK(argc == 2);
	f = fopen(argv[1], "r");
	CHECK(f != NULL);
	while (fgets(line, sizeof(line), f)) {
		int v[20], n;

		if (line[0] == 'i') {
			n = sscanf(line + 1, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				   &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9],
				   &v[10], &v[11], &v[12], &v[13], &v[14], &v[15], &v[16], &v[17]);
			if (n >= 1 && v[0] == 0) {
				cal_gain_init(&g, NULL, NULL, NULL, NULL, 16, 2000, 488281);
				continue;
			}
			CHECK(n == 18);
			int16_t a[6], z[3];
			uint16_t sc[3], r[2];

			for (int i = 0; i < 6; i++) {
				a[i] = (int16_t)v[1 + i];
			}
			for (int i = 0; i < 3; i++) {
				sc[i] = (uint16_t)v[7 + i];
				z[i] = (int16_t)v[10 + i];
			}
			r[0] = (uint16_t)v[13];
			r[1] = (uint16_t)v[14];
			cal_gain_init(&g, a, sc, z, r, (uint16_t)v[15], (uint16_t)v[16], (uint32_t)v[17]);
		} else if (line[0] == 's') {
			CHECK(sscanf(line + 1, "%d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4],
				     &v[5]) == 6);
			in = (struct strelka_step){ { (int16_t)v[0], (int16_t)v[1], (int16_t)v[2] },
						    { (int16_t)v[3], (int16_t)v[4], (int16_t)v[5] } };
			cal_gain_step(&g, &in, &out);
			printf("%d %d %d %d %d %d\n", out.a[0], out.a[1], out.a[2], out.g[0], out.g[1],
			       out.g[2]);
		}
	}
	fclose(f);
	return 0;
}
