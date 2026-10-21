/* The samples summed into steps by their numbers on the grid (FIRMWARE.md, Reading the sensor):
 * two Belkas whose sensors began at different moments make the same steps; a sample lost leaves
 * its step the mean of the rest; a step with no sample is marked; nothing before the start. */
#include <string.h>
#include "steps.h"
#include "test.h"

static struct strelka_step got[16];
static bool got_acc[16];
static uint64_t got_stamp[16];
static int got_n;

static void out(uint32_t k, const struct strelka_step *step, bool acc_out, bool gyro_out,
		uint64_t first_stamp, void *ctx)
{
	(void)gyro_out; (void)ctx;
	CHECK(k == (uint32_t)got_n && got_n < 16);
	got[got_n] = *step;
	got_acc[got_n] = acc_out;
	got_stamp[got_n] = first_stamp;
	got_n++;
}

/* the sensor at 1 kHz, N 10: a sample's value its number modulo 1000 */
static void feed(struct steps *s, int64_t from, int64_t to, const int64_t *skip, int skips)
{
	for (int64_t i = from; i < to; i++) {
		struct strelka_step smp;
		bool lost = false;

		for (int j = 0; j < skips; j++) {
			lost |= skip[j] == i;
		}
		if (lost) {
			continue;
		}
		for (int a = 0; a < 3; a++) {
			smp.a[a] = (int16_t)(i % 1000);
			smp.g[a] = (int16_t)(-(i % 1000));
		}
		steps_sample(s, i, (uint64_t)i * 16000u, &smp, i == 20017, false);
	}
}

int main(void)
{
	struct steps s;
	int64_t skip[] = { 20015, 20020, 20021, 20022, 20023, 20024, 20025, 20026, 20027, 20028,
			   20029 };

	/* Belka A began early, its samples from 19 995; the record from period 1 000, sample
	 * 20 000: steps 0 (20 000–20 009), 1 (one sample lost), 2 (none: marked), 3, 4 (partial,
	 * flushed at the end) */
	steps_begin(&s, 1000, 10, 1000, out, NULL);
	CHECK(s.first == 20000 && s.a_period == 20);
	feed(&s, 19995, 20045, skip, 11);
	steps_end(&s);
	CHECK(got_n == 5);
	CHECK(got[0].a[0] == 4 && got[0].g[1] == -4);            /* the mean of 0…9 */
	CHECK(got[1].a[0] == 14 && got_acc[1]);                   /* 10…19 less 15: 130/9 → 14 */
	CHECK(got[2].a[0] == REC_STEP_MISSING && got[2].g[2] == REC_STEP_MISSING);
	CHECK(got[3].a[0] == 34);
	CHECK(got[4].a[0] == 42);                                 /* 40…44, flushed */
	CHECK(got_stamp[0] == 20000ull * 16000 && got_stamp[1] == 20010ull * 16000 &&
	      got_stamp[2] == 0 && got_stamp[3] == 20030ull * 16000);
	OK("Belka A: the steps, one short, one missing, the last flushed, each with its stamp");

	/* a lagging reader repeats the newest sample: the repeat is dropped, the step whole */
	got_n = 0;
	steps_begin(&s, 1000, 10, 1000, out, NULL);
	feed(&s, 20000, 20009, NULL, 0);
	{
		struct strelka_step smp = { { 5, 5, 5 }, { -5, -5, -5 } };

		steps_sample(&s, 20005, 20005ull * 16000, &smp, false, false);   /* again */
		steps_sample(&s, 20003, 20003ull * 16000, &smp, false, false);   /* older */
	}
	CHECK(got_n == 0);
	feed(&s, 20009, 20010, NULL, 0);
	CHECK(got_n == 1 && got[0].a[0] == 4);
	OK("a repeated or an older sample dropped");

	/* Belka B began late, at 20 003: step 0 the mean of 20 003–20 009, the same numbering */
	got_n = 0;
	steps_begin(&s, 1000, 10, 1000, out, NULL);
	feed(&s, 20003, 20020, NULL, 0);
	steps_end(&s);
	CHECK(got_n == 2 && got[0].a[0] == 6 && got[1].a[0] == 14);
	OK("Belka B: the same step numbers from a later start");

	/* raw: N 1 at 4 kHz, a_period 80, every sample its own step */
	got_n = 0;
	steps_begin(&s, 4000, 1, 5, out, NULL);
	CHECK(s.first == 400);
	feed(&s, 398, 405, NULL, 0);
	CHECK(got_n == 5 && got[0].a[0] == 400 && got[4].a[0] == 404);
	OK("raw steps, nothing before the start");
	printf("STEPS OK\n");
	return 0;
}
