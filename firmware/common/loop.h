/* NIC-Strelka — the clock loop that steers the sensor's CLKIN to the set's grid (PRINCIPLE.md,
 * The clock loop; FIRMWARE.md, The sensor and its clock). Once a period it is given where the
 * last data-ready fell against the grid, in ticks of the 16 MHz clock, and answers with the whole
 * ticks to add to CLKIN's periods, one a period: a PI loop at 0,1 Hz, damping 0,707, after a first
 * second that measures the crystals' difference by a line through the SYNCs. The timers, the
 * stamps and the pins are shared/clock.c's; this is the arithmetic alone, so that the host can
 * run it against a simulated crystal (tests/test_loop.c). */
#ifndef STRELKA_LOOP_H
#define STRELKA_LOOP_H

#include <stdbool.h>
#include <stdint.h>

#define LOOP_CLOCK_HZ       16000000u
#define LOOP_TICK_NS        62.5
#define LOOP_CLKIN_HALF     250u       /* 32 kHz: 500 ticks a CLKIN period */
#define LOOP_PERIOD_TICKS   (20000u * 16u)
#define LOOP_HZ             0.1        /* the loop's natural frequency; calc/params.py */
#define LOOP_DAMPING        0.707
#define LOOP_ACQUIRE        50         /* the first second measures the frequency */
#define LOOP_LOCK_TICKS     16         /* locked: within a microsecond */
#define LOOP_LOCK_PERIODS   50         /* for a second */

struct loop {
	bool steered;           /* false: the fallback, CLKIN straight, nothing asked */
	uint32_t sample_ticks;  /* the sensor's sample period: 16e6 / its rate */
	double kp, ki, integ, carry;
	int acquire_n;
	double acquire_t[LOOP_ACQUIRE], acquire_e[LOOP_ACQUIRE];
	double acquire_last, acquire_turns;
	int locked_run;
};

void loop_init(struct loop *l, bool steered, uint32_t sample_ticks);
/* the offset of a stamp against the grid, within ± half a sample */
int64_t loop_centre(int64_t offset_ticks, uint32_t sample_ticks);
/* once a period, heard or not: the ticks to add to CLKIN's periods, signed, 0 when none */
int32_t loop_period(struct loop *l, bool heard, int64_t offset_ticks, uint32_t period);
bool loop_locked(const struct loop *l);

#endif
