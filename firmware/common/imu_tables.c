/* NIC-Strelka — the sensors' tables (imu_tables.h). */
#include <stddef.h>
#include "imu_tables.h"

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* The LSM6DSV16X's rates with its reference of 40 ms: ODRsel the nearest of AN5763's table 15
 * within ±33 % of the rate, and ODR_TRIG_N_ODR half the samples in 40 ms, the register counting
 * in twos; each above the table's least samples for its ODRsel. Up to 4 kHz, the most the
 * ODR-triggered mode reaches (DT0155 Rev 2). */
static const struct { uint16_t hz; uint8_t odrsel; uint8_t n_odr; } lsm_rates[] = {
	{ 200, 0x7, 4 },     /* ODRsel 200 Hz,   8 samples, the least 8 */
	{ 500, 0x8, 10 },    /* ODRsel 400 Hz,  20 samples, the least 16 */
	{ 1000, 0x9, 20 },   /* ODRsel 800 Hz,  40 samples, the least 32 */
	{ 2000, 0xA, 40 },   /* ODRsel 1600 Hz, 80 samples, the least 64 */
	{ 4000, 0xB, 80 },   /* ODRsel 3200 Hz, 160 samples, the least 128 */
};

/* Every sensor runs on its own clock at its fastest, its own filters off, and its samples are
 * filtered onto the grid by the main core (common/resample.c): an LSM at 7 680 Hz, an ICM at
 * 32 kHz, each sample read as it comes; on a grid of 200, 500, 1 000 or 2 000 Hz, the LSM6DSV16X's
 * 4 000 too, an ICM's on any of its rates. The LSMs' ODRs in high-performance mode, ODR_XL and
 * ODR_G, the same on both (DS13510 Rev 4, table 52; DS14623 Rev 3, table 54), and the LSM6DSV320X's
 * high-g ODR_XL_HG (table 149); the table keeps the slower ones, should a unit come to spare its
 * cell */
static const uint16_t own_grids[] = { 200, 500, 1000, 2000 };
static const struct { uint16_t hz; uint8_t odr; uint8_t odr_hg; } lsm_odrs[] = {
	{ 480, 0x8, 0x3 }, { 960, 0x9, 0x4 }, { 1920, 0xA, 0x5 }, { 3840, 0xB, 0x6 },
	{ 7680, 0xC, 0x7 },
};
#define LSM_FASTEST (COUNT(lsm_odrs) - 1)
#define ICM_FASTEST_HZ 32000

static bool own_grid(uint16_t hz)
{
	for (size_t i = 0; i < COUNT(own_grids); i++) {
		if (own_grids[i] == hz) {
			return true;
		}
	}
	return false;
}


/* the LSMs' accelerometer steps in µg a step, the low-g channel and the LSM6DSV320X's high-g one
 * (DS13510 Rev 4 and DS14623 Rev 3, table 3), and the high-g's FS_XL_HG (DS14623, table 150) */
static const struct { uint16_t g; uint16_t step_ug; int8_t hg_code; } lsm_acc[] = {
	{ 2, 61, -1 }, { 4, 122, -1 }, { 8, 244, -1 }, { 16, 488, -1 },
	{ 32, 976, 0 }, { 64, 1952, 1 }, { 128, 3904, 2 }, { 256, 7808, 3 }, { 320, 10417, 4 },
};

/* the LSMs' gyro ranges, the LSM6DSV16X's FS_G codes and their steps in µdps (DS13510 Rev 4,
 * tables 3 and 63; the LSM6DSV320X's the same steps from 250 °/s, DS14623 Rev 3, table 3) */
static const struct { uint16_t dps; uint8_t code; uint32_t step_udps; } lsm_gyro[] = {
	{ 125, 0x0, 4375 }, { 250, 0x1, 8750 }, { 500, 0x2, 17500 },
	{ 1000, 0x3, 35000 }, { 2000, 0x4, 70000 }, { 4000, 0xC, 140000 },
};

uint8_t imu_kind(uint8_t whoami)
{
	switch (whoami) {
	case WHOAMI_42688P:
	case WHOAMI_42688V:
		return IMU_42688P;
	case WHOAMI_42686P:
		return IMU_42686P;
	case WHOAMI_LSM6DSV16X:
		return IMU_LSM6DSV16X;
	case WHOAMI_LSM6DSV320X:
		return IMU_LSM6DSV320X;
	default:
		return 0;
	}
}

bool imu_is_lsm(uint8_t kind)
{
	return kind == IMU_LSM6DSV16X || kind == IMU_LSM6DSV320X;
}

uint16_t imu_acc_g_max_of(uint8_t kind)
{
	return kind == IMU_LSM6DSV320X ? 320 : kind == IMU_42686P ? 32 : 16;
}

uint16_t imu_gyro_dps_max_of(uint8_t kind)
{
	return kind == IMU_42686P || imu_is_lsm(kind) ? 4000 : 2000;
}

bool imu_on_grid_of(uint8_t kind, uint16_t rate_hz)
{
	return kind != IMU_LSM6DSV320X && rate_hz > IMU_OWN_MOST_HZ;
}

int imu_icm_odr_code(uint16_t hz)
{
	/* DS-000347, 14.37 and 14.38; the same in DS-000639 */
	switch (hz) {
	case 32000: return 0x1;
	case 16000: return 0x2;
	case 8000: return 0x3;
	case 4000: return 0x4;
	case 2000: return 0x5;
	case 1000: return 0x6;
	case 200: return 0x7;
	case 100: return 0x8;
	case 50: return 0x9;
	case 25: return 0xA;
	case 500: return 0xF;
	default: return -1;
	}
}

int imu_icm_fs_code(uint16_t want, uint16_t top, int steps)
{
	/* bits 7:5: the ICM-42688-P's table starts at ±16 g / ±2000 °/s, the ICM-42686-P's at
	 * ±32 g / ±4000 °/s, each step below halving the range */
	uint16_t r = top;

	for (int i = 0; i < steps; i++, r /= 2) {
		if (r == want) {
			return i;
		}
	}
	return -1;
}

int imu_lsm_rate(uint16_t hz, uint8_t *odrsel, uint8_t *n_odr)
{
	for (size_t i = 0; i < COUNT(lsm_rates); i++) {
		if (lsm_rates[i].hz == hz) {
			if (odrsel) {
				*odrsel = lsm_rates[i].odrsel;
			}
			if (n_odr) {
				*n_odr = lsm_rates[i].n_odr;
			}
			return (int)i;
		}
	}
	return -1;
}

static int lsm_gyro_index(uint16_t dps)
{
	for (size_t i = 0; i < COUNT(lsm_gyro); i++) {
		if (lsm_gyro[i].dps == dps) {
			return (int)i;
		}
	}
	return -1;
}

int imu_lsm_gyro_code(uint16_t dps)
{
	int i = lsm_gyro_index(dps);

	return i < 0 ? -1 : lsm_gyro[i].code;
}

int imu_lsm_acc_code(uint16_t g)
{
	/* FS_XL: ±2, 4, 8, 16 g as 00 … 11 (DS13510 Rev 4, table 68; DS14623 Rev 3, CTRL8) */
	switch (g) {
	case 2: return 0x0;
	case 4: return 0x1;
	case 8: return 0x2;
	case 16: return 0x3;
	default: return -1;
	}
}

int imu_lsm_own(uint16_t hz, uint8_t n, uint8_t *odr, uint8_t *odr_hg, uint16_t *sensor_hz)
{
	size_t k;

	(void)n;
	if (!own_grid(hz) && hz != 4000) {
		return -1;
	}
	k = LSM_FASTEST;
	if (odr) {
		*odr = lsm_odrs[k].odr;
	}
	if (odr_hg) {
		*odr_hg = lsm_odrs[k].odr_hg;
	}
	if (sensor_hz) {
		*sensor_hz = lsm_odrs[k].hz;
	}
	return (int)k;
}

uint16_t imu_icm_own_hz(uint16_t hz, uint8_t n)
{
	(void)hz;
	(void)n;
	return ICM_FASTEST_HZ;
}

int imu_lsm320_gyro_code(uint16_t dps)
{
	/* FS_G 001 … 101 for ±250 … ±4 000 °/s, bit 3 to be 1 (DS14623 Rev 3, table 64) */
	int i = lsm_gyro_index(dps);

	return i < 1 ? -1 : 0x08 | i;
}

static int lsm_acc_index(uint16_t g)
{
	for (size_t i = 0; i < COUNT(lsm_acc); i++) {
		if (lsm_acc[i].g == g) {
			return (int)i;
		}
	}
	return -1;
}

int imu_lsm320_hg_code(uint16_t g)
{
	int i = lsm_acc_index(g);

	return i < 0 ? -1 : lsm_acc[i].hg_code;
}

bool imu_rate_ok_of(uint8_t kind, uint16_t rate_hz)
{
	if (rate_hz <= IMU_OWN_MOST_HZ) {
		/* on its own clock: an LSM on the grids above, the LSM6DSV16X's 4 kHz too, the
		 * LSM6DSV320X's 2 kHz the most; an ICM on any of its rates */
		if (kind == IMU_LSM6DSV16X) {
			return own_grid(rate_hz) || rate_hz == 4000;
		}
		return kind == IMU_LSM6DSV320X ? own_grid(rate_hz) : imu_icm_odr_code(rate_hz) >= 0;
	}
	if (kind == IMU_LSM6DSV16X) {
		return imu_lsm_rate(rate_hz, NULL, NULL) >= 0;   /* 4 kHz, steered */
	}
	return kind != IMU_LSM6DSV320X && imu_icm_odr_code(rate_hz) >= 0;
}

uint16_t imu_sensor_hz_of(uint8_t kind, uint16_t rate_hz, uint8_t n)
{
	uint16_t hz = rate_hz;

	if (rate_hz > IMU_OWN_MOST_HZ) {
		return rate_hz;                                   /* steered, on the grid */
	}
	if (imu_is_lsm(kind)) {
		(void)imu_lsm_own(rate_hz, n, NULL, NULL, &hz);
		return hz;
	}
	return imu_icm_own_hz(rate_hz, n);
}

bool imu_ranges_ok_of(uint8_t kind, uint16_t acc_g, uint16_t gyro_dps)
{
	if (kind == IMU_LSM6DSV16X) {
		return imu_lsm_acc_code(acc_g) >= 0 && lsm_gyro_index(gyro_dps) >= 0;
	}
	if (kind == IMU_LSM6DSV320X) {
		return lsm_acc_index(acc_g) >= 0 && imu_lsm320_gyro_code(gyro_dps) >= 0;
	}
	/* the ICMs' gyro down to ±125 °/s: their ±62,5, ±31,25 and ±15,625 are no whole degrees */
	return imu_icm_fs_code(acc_g, imu_acc_g_max_of(kind), kind == IMU_42686P ? 5 : 4) >= 0 &&
	       gyro_dps >= 125 && imu_icm_fs_code(gyro_dps, imu_gyro_dps_max_of(kind), 8) >= 0;
}

uint16_t imu_gyro_full_scale_of(uint8_t kind, uint16_t gyro_dps)
{
	int i = lsm_gyro_index(gyro_dps);

	if (!imu_is_lsm(kind) || i < 0) {
		return gyro_dps;
	}
	/* the step times 32 768, rounded to the degree a second */
	return (uint16_t)(((uint64_t)lsm_gyro[i].step_udps * 32768u + 500000u) / 1000000u);
}

uint16_t imu_acc_full_scale_of(uint8_t kind, uint16_t acc_g)
{
	int i = lsm_acc_index(acc_g);

	if (!imu_is_lsm(kind) || i < 0) {
		return acc_g;
	}
	/* the step times 32 768, rounded to the g */
	return (uint16_t)(((uint32_t)lsm_acc[i].step_ug * 32768u + 500000u) / 1000000u);
}

const char *imu_name_of(uint8_t kind)
{
	switch (kind) {
	case IMU_42688P: return "ICM-42688-P";
	case IMU_42686P: return "ICM-42686-P";
	case IMU_LSM6DSV16X: return "LSM6DSV16X";
	case IMU_LSM6DSV320X: return "LSM6DSV320X";
	default: return "";
	}
}

const char *imu_sheet_of(uint8_t kind)
{
	switch (kind) {
	case IMU_42688P: return "DS-000347 v1.9";
	case IMU_42686P: return "DS-000639 v1.0";
	case IMU_LSM6DSV16X: return "DS13510 Rev 4";
	case IMU_LSM6DSV320X: return "DS14623 Rev 3";
	default: return "";
	}
}

uint8_t imu_acc_channel_of(uint8_t kind, uint16_t acc_g)
{
	return kind == IMU_LSM6DSV320X && acc_g > 16 ? 1 : 0;
}

uint32_t imu_acc_step_ng_of(uint8_t kind, uint16_t acc_g)
{
	int i = lsm_acc_index(acc_g);

	if (imu_is_lsm(kind) && i >= 0) {
		return (uint32_t)lsm_acc[i].step_ug * 1000u;     /* the sheet's table 3 */
	}
	/* an ICM's: the range / 32 768 (DS-000639, table 2; DS-000347, table 2) */
	return (uint32_t)(((uint64_t)acc_g * 1000000000u + 16384u) / 32768u);
}

uint32_t imu_gyro_step_ndps_of(uint8_t kind, uint16_t gyro_dps)
{
	int i = lsm_gyro_index(gyro_dps);

	if (imu_is_lsm(kind) && i >= 0) {
		return lsm_gyro[i].step_udps * 1000u;            /* the sheet's table 3 */
	}
	/* an ICM's: the range / 32 768 (DS-000639, table 1; DS-000347, table 1) */
	return (uint32_t)(((uint64_t)gyro_dps * 1000000000u + 16384u) / 32768u);
}

int16_t imu_temp_centi_of(uint8_t kind, int16_t raw)
{
	if (imu_is_lsm(kind)) {
		/* 256 a degree, 0 at 25 °C (DS13510 Rev 4, table 4 and its note 3; DS14623 Rev 3,
		 * table 5 and its note 3) */
		return (int16_t)(2500 + (int32_t)raw * 100 / 256);
	}
	/* TEMP_DATA / 132,48 + 25 (DS-000347, 14.6) */
	return (int16_t)(2500 + (int32_t)raw * 10000 / 13248);
}
