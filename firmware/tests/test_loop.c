/* The clock loop against a simulated crystal (PRINCIPLE.md, The clock loop): a Belka whose crystal
 * runs some tens of ppm off the first Povodok's, its CLKIN lengthened or shortened a tick at a
 * time as the loop asks, one tick a CLKIN period, its samples falling where CLKIN puts them. At
 * 1, 8 and 32 kHz, the crystal fast and slow, the loop locks within half a minute and holds the
 * samples within a microsecond of the grid over the last ten seconds; a tenth of the SYNCs lost
 * changes nothing of it. */
#include <math.h>
#include <stdlib.h>
#include "loop.h"
#include "test.h"

static void run(uint32_t rate_hz, double ppm, double phase, int lost_in)
{
	struct loop l;
	double T = 16e6 / rate_hz;              /* a sample in local ticks, nominal */
	double L0 = 1000.0, s_next, s_last = 0, prev_L;
	int periods = 3000, locked_at = -1;
	int32_t pending = 0;
	double worst = 0;
	unsigned seed = 12345;

	loop_init(&l, true, (uint32_t)T);
	s_next = L0 + phase * T;
	prev_L = L0;
	for (int p = 1; p <= periods; p++) {
		/* the SYNC's end on the local timer: the set's grid seen through this crystal */
		double L = (p * (double)LOOP_PERIOD_TICKS) * (1 + ppm * 1e-6) + L0;
		int elapsed = (int)((L - prev_L) / (2 * LOOP_CLKIN_HALF));
		int32_t used = pending > 0 ? (pending < elapsed ? pending : elapsed)
					   : (-pending < elapsed ? pending : -elapsed);
		double origin = L - p * (double)LOOP_PERIOD_TICKS;
		bool heard;

		/* the ticks the loop asked for, one a CLKIN period: every later sample moved */
		pending -= used;
		s_next += used;
		while (s_next <= L) {
			s_last = s_next;
			s_next += T;
		}
		prev_L = L;
		seed = seed * 1103515245u + 12345u;
		heard = lost_in == 0 || (int)((seed >> 16) % (unsigned)lost_in) != 0;
		pending += loop_period(&l, heard, (int64_t)llround(s_last - origin), (uint32_t)p);
		if (locked_at < 0 && loop_locked(&l)) {
			locked_at = p;
		}
		if (p > periods - 500) {
			double off = fmod(s_last - origin, T);

			if (off > T / 2) {
				off -= T;
			}
			if (off < -T / 2) {
				off += T;
			}
			worst = fabs(off) > worst ? fabs(off) : worst;
		}
	}
	printf("ok %5u Hz, %+5.1f ppm%s: locked after %4.1f s, the last 10 s within %.2f us\n",
	       rate_hz, ppm, lost_in ? ", a SYNC in 10 lost" : "", locked_at * 0.02, worst / 16);
	CHECK(locked_at > 0 && locked_at <= 1500);
	CHECK(worst < 16);
}

int main(void)
{
	uint32_t rates[] = { 1000, 8000, 32000 };
	double ppms[] = { 40, -35, 12 };
	struct loop l;

	CHECK(loop_centre(15999, 16000) == -1 && loop_centre(-1, 16000) == -1 &&
	      loop_centre(8000, 16000) == 8000 && loop_centre(8001, 16000) == -7999);
	for (int r = 0; r < 3; r++) {
		for (int q = 0; q < 3; q++) {
			run(rates[r], ppms[q], 0.37 + 0.2 * q, 0);
		}
	}
	run(1000, 40, 0.5, 10);
	run(32000, -35, 0.5, 10);
	/* the fallback asks nothing and is "locked" at once */
	loop_init(&l, false, 16000);
	CHECK(loop_period(&l, true, 5000, 1) == 0 && loop_locked(&l));
	printf("LOOP OK\n");
	return 0;
}
