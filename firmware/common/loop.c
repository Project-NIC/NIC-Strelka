#include <math.h>
#include <string.h>
#include "loop.h"

#define LOOP_PI 3.14159265358979323846

void loop_init(struct loop *l, bool steered, uint32_t sample_ticks)
{
	double ts = 20000e-6;
	double wn = 2 * LOOP_PI * LOOP_HZ;

	memset(l, 0, sizeof(*l));
	l->steered = steered;
	l->sample_ticks = sample_ticks ? sample_ticks : 16000u;
	l->kp = 2 * LOOP_DAMPING * wn * ts;
	l->ki = (wn * ts) * (wn * ts);
}

int64_t loop_centre(int64_t offset_ticks, uint32_t sample_ticks)
{
	int64_t off = offset_ticks % (int64_t)sample_ticks;

	if (off < 0) {
		off += sample_ticks;
	}
	if (off > (int64_t)sample_ticks / 2) {
		off -= sample_ticks;
	}
	return off;
}

/* whole ticks go out, the rest carries to the next period */
static int32_t whole(struct loop *l, double want)
{
	int32_t ticks = (int32_t)lround(want + l->carry);

	l->carry += want - ticks;
	return ticks;
}

int32_t loop_period(struct loop *l, bool heard, int64_t offset_ticks, uint32_t period)
{
	double meas = heard ? (double)loop_centre(offset_ticks, l->sample_ticks) : 0;
	double want;

	if (!l->steered) {
		return 0;               /* the fallback: the stamps say where the samples fell */
	}
	if (l->acquire_n < LOOP_ACQUIRE) {
		if (heard) {
			/* the offset is read modulo a sample and, at a high rate, the crystals'
			 * difference turns it over within the first second: unwrapped */
			double half = (double)l->sample_ticks / 2;

			if (l->acquire_n > 0 && meas - l->acquire_last > half) {
				l->acquire_turns -= l->sample_ticks;
			} else if (l->acquire_n > 0 && meas - l->acquire_last < -half) {
				l->acquire_turns += l->sample_ticks;
			}
			l->acquire_last = meas;
			l->acquire_t[l->acquire_n] = (double)period;
			l->acquire_e[l->acquire_n] = meas + l->acquire_turns;
			l->acquire_n++;
		}
		if (l->acquire_n < LOOP_ACQUIRE) {
			return 0;
		}
		/* the line through the first second's SYNCs: the frequency, and one phase step */
		double mt = 0, me = 0, sxy = 0, sxx = 0, slope, now_off;

		for (int i = 0; i < l->acquire_n; i++) {
			mt += l->acquire_t[i];
			me += l->acquire_e[i];
		}
		mt /= l->acquire_n;
		me /= l->acquire_n;
		for (int i = 0; i < l->acquire_n; i++) {
			sxy += (l->acquire_t[i] - mt) * (l->acquire_e[i] - me);
			sxx += (l->acquire_t[i] - mt) * (l->acquire_t[i] - mt);
		}
		slope = sxx > 0 ? sxy / sxx : 0;        /* ticks a period */
		/* the offset now, back within half a sample: a whole sample's shift is none */
		now_off = fmod(me + slope * ((double)period - mt), (double)l->sample_ticks);
		if (now_off > (double)l->sample_ticks / 2) {
			now_off -= l->sample_ticks;
		} else if (now_off < -(double)l->sample_ticks / 2) {
			now_off += l->sample_ticks;
		}
		l->integ = -slope;
		return whole(l, -now_off);
	}
	if (heard) {
		l->integ += -l->ki * meas;
		want = -l->kp * meas + l->integ;
		l->locked_run = (meas > -LOOP_LOCK_TICKS && meas < LOOP_LOCK_TICKS) ?
				l->locked_run + 1 : 0;
	} else {
		want = l->integ;        /* no SYNC: the frequency held */
	}
	return whole(l, want);
}

bool loop_locked(const struct loop *l)
{
	return !l->steered || l->locked_run >= LOOP_LOCK_PERIODS;
}
