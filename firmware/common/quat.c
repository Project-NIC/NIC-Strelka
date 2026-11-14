#include <math.h>
#include "quat.h"

#define SCALE 511.0f
#define ROOT_HALF 0.70710678f

uint32_t quat_pack(const float q[4])
{
	int big = 0;
	float sign;
	uint32_t v;

	for (int i = 1; i < 4; i++) {
		if (fabsf(q[i]) > fabsf(q[big])) {
			big = i;
		}
	}
	sign = q[big] < 0 ? -1.0f : 1.0f;
	v = (uint32_t)big << 30;
	for (int i = 0, shift = 20; i < 4; i++) {
		if (i == big) {
			continue;
		}
		float c = q[i] * sign / ROOT_HALF;
		long n = lroundf(c * SCALE);

		if (n > 511) {
			n = 511;
		} else if (n < -511) {
			n = -511;
		}
		v |= ((uint32_t)n & 0x3FFu) << shift;
		shift -= 10;
	}
	return v;
}

void quat_unpack(uint32_t v, float q[4])
{
	int big = (int)(v >> 30);
	float sum = 0;

	for (int i = 0, shift = 20; i < 4; i++) {
		if (i == big) {
			continue;
		}
		int32_t n = (int32_t)((v >> shift) & 0x3FFu);

		if (n & 0x200) {
			n -= 0x400;
		}
		q[i] = (float)n / SCALE * ROOT_HALF;
		sum += q[i] * q[i];
		shift -= 10;
	}
	q[big] = sum < 1.0f ? sqrtf(1.0f - sum) : 0.0f;
}
