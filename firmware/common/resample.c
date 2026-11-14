/* NIC-Strelka — a sensor on its own clock put onto the set's grid (resample.h). */
#include <string.h>
#include "resample.h"

/* the filter's half, from its centre to RESAMPLE_SPAN steps, RESAMPLE_TABLE points a step, 16 384
 * for 1: sinc(0,965 u) × Kaiser(u / 10, β 3,395), u in steps — flat within 1 % to 0,43 of the
 * rate, 40 dB down from 0,57, so that what folds over the rate's half lands above 0,43 alone
 * (Kaiser's formula for 40 dB; the response worked out numerically) */
static const int16_t kernel[RESAMPLE_SPAN * RESAMPLE_TABLE + 1] = {
	16384, 16378, 16359, 16328, 16285, 16230, 16162, 16083, 15991, 15888, 15773, 15646, 15508,
	15359, 15199, 15028, 14847, 14655, 14453, 14242, 14021, 13791, 13553, 13305, 13050, 12787,
	12516, 12238, 11953, 11662, 11365, 11062, 10754, 10441, 10124, 9803, 9478, 9150, 8820, 8487,
	8152, 7815, 7478, 7140, 6801, 6463, 6126, 5789, 5454, 5121, 4789, 4461, 4135, 3813, 3494,
	3180, 2869, 2564, 2264, 1969, 1679, 1396, 1119, 848, 585, 328, 79, -163, -398, -624, -842,
	-1052, -1254, -1447, -1632, -1807, -1974, -2132, -2281, -2421, -2552, -2674, -2787, -2890,
	-2985, -3071, -3147, -3215, -3274, -3324, -3366, -3399, -3424, -3440, -3449, -3449, -3442,
	-3427, -3405, -3376, -3340, -3297, -3247, -3191, -3130, -3062, -2989, -2911, -2828, -2740,
	-2648, -2552, -2451, -2348, -2241, -2131, -2018, -1903, -1785, -1666, -1545, -1423, -1300,
	-1176, -1052, -928, -804, -680, -557, -434, -313, -193, -75, 41, 156, 268, 378, 485, 589,
	690, 788, 882, 974, 1061, 1145, 1224, 1300, 1372, 1439, 1503, 1561, 1616, 1666, 1711, 1752,
	1789, 1820, 1848, 1870, 1888, 1902, 1911, 1916, 1916, 1912, 1904, 1891, 1875, 1855, 1830,
	1802, 1770, 1735, 1696, 1654, 1609, 1561, 1510, 1456, 1400, 1341, 1280, 1217, 1153, 1086,
	1018, 949, 878, 806, 734, 660, 587, 512, 438, 364, 290, 216, 142, 69, -3, -74, -144, -212,
	-280, -346, -410, -473, -534, -592, -649, -704, -756, -806, -853, -898, -941, -980, -1017,
	-1052, -1083, -1112, -1138, -1161, -1181, -1198, -1212, -1223, -1232, -1237, -1240, -1240,
	-1237, -1231, -1223, -1212, -1198, -1182, -1163, -1142, -1119, -1093, -1065, -1035, -1004,
	-970, -934, -897, -858, -818, -776, -733, -689, -644, -598, -551, -503, -455, -406, -357,
	-308, -259, -209, -160, -111, -62, -14, 34, 81, 128, 174, 218, 262, 305, 346, 387, 426, 463,
	499, 534, 567, 598, 628, 655, 681, 706, 728, 748, 767, 783, 798, 811, 821, 830, 837, 842,
	844, 845, 844, 841, 836, 830, 821, 811, 799, 786, 770, 754, 735, 715, 694, 672, 648, 623,
	597, 570, 542, 513, 483, 452, 421, 389, 356, 323, 290, 256, 223, 189, 155, 121, 87, 53, 20,
	-13, -46, -78, -109, -141, -171, -201, -230, -258, -285, -311, -336, -360, -383, -405, -426,
	-446, -464, -481, -497, -512, -525, -537, -547, -556, -564, -571, -576, -579, -582, -583,
	-582, -581, -578, -574, -568, -561, -554, -545, -534, -523, -511, -497, -483, -468, -452,
	-435, -417, -398, -379, -359, -339, -318, -296, -274, -252, -230, -207, -184, -161, -137,
	-114, -91, -67, -44, -21, 2, 24, 46, 68, 90, 111, 131, 151, 170, 189, 207, 225, 241, 257,
	273, 287, 301, 313, 325, 336, 346, 355, 364, 371, 378, 383, 388, 391, 394, 396, 397, 396,
	395, 394, 391, 387, 383, 378, 372, 365, 357, 349, 340, 330, 320, 309, 298, 286, 273, 260,
	247, 233, 219, 205, 190, 175, 160, 144, 129, 113, 97, 82, 66, 50, 35, 19, 4, -11, -26, -41,
	-55, -69, -83, -97, -110, -122, -134, -146, -157, -168, -178, -188, -197, -205, -213, -221,
	-227, -233, -239, -244, -248, -252, -255, -257, -259, -260, -261, -261, -260, -259, -257,
	-255, -252, -249, -245, -240, -235, -230, -224, -218, -211, -204, -196, -188, -180, -172,
	-163, -154, -145, -135, -126, -116, -106, -96, -86, -76, -66, -56, -46, -35, -25, -15, -6,
	4, 14, 23, 32, 41, 50, 59, 67, 75, 83, 90, 97, 104, 110, 116, 122, 128, 132, 137, 141, 145,
	149, 152, 154, 156, 158, 160, 161, 162, 162, 162, 161, 160, 159, 158, 156, 154, 151, 148,
	145, 142, 138, 134, 130, 125, 121, 116, 111, 106, 100, 95, 89, 83, 78, 72, 66, 60, 53, 47,
	41, 35, 29, 23, 17, 11, 5, -1, -6, -12, -17, -23, -28, -33, -38, -42, -47, -51, -55, -59,
	-63, -66, -69, -72, -75, -78, -80, -82, -84, -86, -87, -88, -89, -90, -90, -91, -91, -90,
	-90, -89, -89, -88, -87, -85, -84, -82, -80, -78, -76, -74, -71
};

static struct resample_in *at_(struct resample *r, uint32_t i)
{
	return &r->in[(r->head + i) % RESAMPLE_MAX];
}

void resample_begin(struct resample *r, uint32_t grid_period, uint8_t n, int64_t first,
		    uint32_t sensor_period, uint32_t max_gap, resample_out_t out, void *ctx)
{
	memset(r, 0, sizeof(*r));
	r->grid = grid_period;
	r->n = n ? n : 1;
	r->step = grid_period * r->n;
	r->first = first;
	r->period = sensor_period ? sensor_period : 1;
	r->max_gap = max_gap;
	if ((uint64_t)r->period * 16u <= r->step) {
		/* 16 samples a step or more: summed into bins first, a bin the unit kept */
		r->binw = r->step / RESAMPLE_BINS;
		r->period = r->binw;
		r->max_gap = r->binw * 5u / 2u;
	}
	r->out = out;
	r->ctx = ctx;
}

/* step k's centre, ticks from period 0: the middle of its N points */
static int64_t centre(const struct resample *r, int64_t k)
{
	return (r->first + k * r->n) * (int64_t)r->grid + ((int64_t)(r->n - 1) * r->grid) / 2;
}

/* the filter's weight at |dt| ticks from the centre, 0 beyond its span */
static int32_t weight(const struct resample *r, int64_t dt)
{
	int64_t pos = (dt < 0 ? -dt : dt) * RESAMPLE_TABLE;
	int64_t i = pos / r->step;

	if (i >= RESAMPLE_SPAN * RESAMPLE_TABLE) {
		return 0;
	}
	return kernel[i] + (int32_t)(((int64_t)(kernel[i + 1] - kernel[i]) * (pos % r->step)) /
				     r->step);
}

static int16_t clamp16(int64_t v)
{
	return (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
}

/* step k drawn from the samples kept, or left missing where its centre falls in a gap */
static void draw(struct resample *r, int64_t k)
{
	int64_t c = centre(r, k);
	struct resample_in *before = NULL, *after = NULL;
	int64_t sum[9] = { 0 }, wsum = 0;

	for (uint32_t i = 0; i < r->count; i++) {
		struct resample_in *p = at_(r, i);

		if (p->at <= c) {
			before = p;
		} else if (!after) {
			after = p;
		}
	}
	if (!before || (after && after->at - before->at > (int64_t)r->max_gap)) {
		return;
	}
	if (!after) {
		after = before;
	}
	for (uint32_t i = 0; i < r->count; i++) {
		struct resample_in *p = at_(r, i);
		int32_t w = weight(r, p->at - c);

		if (w == 0) {
			continue;
		}
		/* a bin weighs as the samples it holds, so that bins of two and of three stand for
		 * what was measured */
		w *= p->n;
		wsum += w;
		for (int j = 0; j < 9; j++) {
			sum[j] += (int64_t)w * p->v[j];
		}
	}
	if (wsum <= 0) {
		return;
	}

	struct strelka_step s;
	int16_t hg[3];
	bool has_hg = (before->flags & 4) && (after->flags & 4);
	uint8_t flags = before->flags | after->flags;

	for (int j = 0; j < 3; j++) {
		int64_t half = wsum / 2;

		s.a[j] = clamp16((sum[j] + (sum[j] >= 0 ? half : -half)) / wsum);
		s.g[j] = clamp16((sum[3 + j] + (sum[3 + j] >= 0 ? half : -half)) / wsum);
		hg[j] = clamp16((sum[6 + j] + (sum[6 + j] >= 0 ? half : -half)) / wsum);
	}
	for (int j = 0; j < r->n; j++) {
		r->out(r->first + k * r->n + j, &s, has_hg ? hg : NULL, flags & 1, flags & 2,
		       r->ctx);
	}
}

/* the steps whose filter is whole by `until`, drawn; the samples no step wants any more let go */
static void draw_until(struct resample *r, int64_t until, bool all)
{
	int64_t span = (int64_t)RESAMPLE_SPAN * r->step;

	while (r->count && (all ? centre(r, r->next) <= until
				: centre(r, r->next) + span <= until)) {
		draw(r, r->next);
		r->next++;
		while (r->count && at_(r, 0)->at < centre(r, r->next) - span &&
		       r->count > 1 && at_(r, 1)->at <= centre(r, r->next)) {
			r->head = (r->head + 1) % RESAMPLE_MAX;
			r->count--;
		}
	}
}

static void keep(struct resample *r, const struct resample_in *in)
{
	if (r->count == RESAMPLE_MAX) {
		r->head = (r->head + 1) % RESAMPLE_MAX;     /* full: the oldest let go */
		r->count--;
	}
	*at_(r, r->count) = *in;
	r->count++;
}

static void take(struct resample *r, const struct resample_in *in);

/* the open bin closed: its samples' mean at their mean instant */
static void bin_close(struct resample *r)
{
	struct resample_in p;
	int32_t n = r->bin_n;

	if (!n) {
		return;
	}
	p.at = r->bin_at / n;
	for (int j = 0; j < 9; j++) {
		int32_t s = r->bin_sum[j];

		p.v[j] = (int16_t)((s >= 0 ? s + n / 2 : s - n / 2) / n);
	}
	p.flags = r->bin_flags;
	p.n = (uint8_t)(n > 255 ? 255 : n);
	r->bin_n = 0;
	take(r, &p);
}

void resample_sample(struct resample *r, int64_t at, const struct strelka_step *s,
		     const int16_t *hg, bool acc_out, bool gyro_out)
{
	struct resample_in p;

	p.at = at;
	for (int j = 0; j < 3; j++) {
		p.v[j] = s->a[j];
		p.v[3 + j] = s->g[j];
		p.v[6 + j] = hg ? hg[j] : 0;
	}
	p.flags = (uint8_t)((acc_out ? 1 : 0) | (gyro_out ? 2 : 0) | (hg ? 4 : 0));
	p.n = 1;
	if (r->any_in && at <= r->last_in) {
		return;
	}
	r->any_in = true;
	r->last_in = at;
	if (!r->binw) {
		take(r, &p);
		return;
	}
	int64_t b = at >= 0 ? at / r->binw : -((-at + r->binw - 1) / r->binw);

	if (r->bin_n && b != r->bin) {
		bin_close(r);
	}
	if (!r->bin_n) {
		r->bin = b;
		r->bin_at = 0;
		memset(r->bin_sum, 0, sizeof(r->bin_sum));
		r->bin_flags = 4;
	}
	r->bin_at += at;
	for (int j = 0; j < 9; j++) {
		r->bin_sum[j] += p.v[j];
	}
	r->bin_flags = (uint8_t)((r->bin_flags | (p.flags & 3)) & (p.flags | 3));
	r->bin_n++;
}

/* a sample or a bin kept, in order: the lost ones within max_gap put back on their line first */
static void take(struct resample *r, const struct resample_in *in)
{
	struct resample_in p = *in;
	int64_t at = p.at;

	if (r->count && at <= at_(r, r->count - 1)->at) {
		return;
	}
	if (!r->begun) {
		/* the steps whose centre comes before the first sample stay missing */
		int64_t c0 = centre(r, 0);

		r->begun = true;
		r->next = at > c0 ? (at - c0 + r->step - 1) / r->step : 0;
	}
	if (r->count) {
		/* samples lost within max_gap put back on the line between their neighbours */
		const struct resample_in last = *at_(r, r->count - 1);
		int64_t gap = at - last.at;
		/* a bin's instant wanders by up to half a bin with the samples it caught: lost only
		 * beyond 1,75 of the period */
		int64_t lost = (gap * 4 - 3 * (int64_t)r->period) / (4 * (int64_t)r->period);

		if (gap <= (int64_t)r->max_gap && lost > 0) {
			for (int64_t m = 1; m <= lost; m++) {
				struct resample_in q;

				q.at = last.at + gap * m / (lost + 1);
				for (int j = 0; j < 9; j++) {
					q.v[j] = (int16_t)(last.v[j] + ((int32_t)p.v[j] - last.v[j]) *
								       m / (lost + 1));
				}
				q.flags = (uint8_t)((last.flags | p.flags) & 3) |
					  (uint8_t)(last.flags & p.flags & 4);
				q.n = (uint8_t)((last.n + p.n + 1) / 2);
				keep(r, &q);
			}
		}
	}
	keep(r, &p);
	draw_until(r, at, false);
}

void resample_flush(struct resample *r)
{
	bin_close(r);
	if (r->count) {
		draw_until(r, at_(r, r->count - 1)->at, true);
	}
}
