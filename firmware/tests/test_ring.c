/* NIC-Strelka — the ring between the FLPR and the main core, and the sensors' tables, on the host:
 * the head's bytes where FIRMWARE.md puts them, a ring filled past full and emptied, the
 * temperature's half degrees, the stamp's 64 bits across TIMER22's wrap; the chips by WHO_AM_I,
 * the LSM6DSV16X's rates by AN5763's table 15 and its gyro's full scale by its step; the
 * LSM6DSV320X's own rates, ranges and codes by DS14623. */
#include <string.h>
#include "test.h"
#include "sensor_ring.h"
#include "imu_tables.h"

static struct ring r;

static void test_layout(void)
{
	uint8_t *b = (uint8_t *)&r.head;

	memset(&r, 0, sizeof(r));
	r.head.rate_hz = 0x1234;
	r.head.written = 0xA1B2C3D4;
	r.head.read = 0x0A0B0C0D;
	r.head.lost = 0x5566;
	r.head.missed = 0x7788;
	r.head.temp_half = -2;
	r.head.acc_g = 320;
	CHECK(b[6] == 0x34 && b[7] == 0x12);                     /* little-endian, both cores */
	CHECK(b[10] == 0x66 && b[11] == 0x55);
	CHECK(b[12] == 0xD4 && b[15] == 0xA1);
	CHECK(b[16] == 0x0D && b[19] == 0x0A);
	CHECK(b[20] == 0x88 && b[21] == 0x77);
	CHECK(b[5] == 0xFE);
	CHECK(b[22] == 0x40 && b[23] == 0x01 && b[8] == 0);
	CHECK(sizeof(struct ring) == 32 + 24 * RING_N);
	OK("the head's bytes");
}

static void test_fill(void)
{
	struct ring_sample s = { 0 }, out;

	memset(&r, 0, sizeof(r));
	for (uint32_t i = 0; i < RING_N + 5; i++) {
		s.stamp = i;
		s.a[0] = (int16_t)i;
		ring_push(&r, &s);
	}
	CHECK(r.head.written == RING_N && r.head.lost == 5);
	for (uint32_t i = 0; i < RING_N; i++) {
		CHECK(ring_pop(&r, &out) == 1 && out.stamp == i && out.a[0] == (int16_t)i);
	}
	CHECK(ring_pop(&r, &out) == 0);
	/* the counts run on past 2^32 by their difference alone */
	r.head.written = r.head.read = 0xFFFFFFFE;
	for (int i = 0; i < 4; i++) {
		s.stamp = 100 + i;
		CHECK(ring_push(&r, &s) == 0);
	}
	for (int i = 0; i < 4; i++) {
		CHECK(ring_pop(&r, &out) == 1 && out.stamp == (uint32_t)(100 + i));
	}
	CHECK(r.head.read == 2);
	OK("the ring filled past full and emptied");
}

static void test_temp_and_stamp(void)
{
	CHECK(ring_temp_half(2500) == 0 && ring_temp_centi(0) == 2500);
	CHECK(ring_temp_half(3725) == 24 && ring_temp_centi(24) == 3700);  /* ½ °C steps */
	CHECK(ring_temp_half(-4000) == -128 && ring_temp_half(20000) == 127);
	CHECK(ring_extend(0x100000010ull, 0x00000005u) == 0x100000005ull);
	CHECK(ring_extend(0x100000010ull, 0xFFFFFFF0u) == 0x0FFFFFFF0ull);  /* before the wrap */
	OK("the temperature's half degrees and the stamp's 64 bits");
}

static void test_tables(void)
{
	uint8_t odrsel, n_odr;

	CHECK(imu_kind(0x70) == IMU_LSM6DSV16X && imu_kind(0x44) == IMU_42686P);
	CHECK(imu_kind(0x47) == IMU_42688P && imu_kind(0xDB) == IMU_42688P && imu_kind(0) == 0);
	CHECK(imu_lsm_rate(1000, &odrsel, &n_odr) >= 0 && odrsel == 0x9 && n_odr == 20);
	CHECK(imu_lsm_rate(4000, &odrsel, &n_odr) >= 0 && odrsel == 0xB && n_odr == 80);
	CHECK(imu_lsm_rate(100, NULL, NULL) < 0 && imu_lsm_rate(8000, NULL, NULL) < 0);
	CHECK(imu_rate_ok_of(IMU_LSM6DSV16X, 4000) && !imu_rate_ok_of(IMU_LSM6DSV16X, 32000));
	CHECK(imu_rate_ok_of(IMU_42686P, 32000) && imu_rate_ok_of(IMU_42686P, 100));
	CHECK(imu_ranges_ok_of(IMU_LSM6DSV16X, 16, 4000) && !imu_ranges_ok_of(IMU_LSM6DSV16X, 32, 4000));
	CHECK(imu_ranges_ok_of(IMU_42686P, 32, 4000) && !imu_ranges_ok_of(IMU_42688P, 16, 4000));
	CHECK(imu_gyro_full_scale_of(IMU_LSM6DSV16X, 4000) == 4588);
	CHECK(imu_gyro_full_scale_of(IMU_LSM6DSV16X, 2000) == 2294);
	CHECK(imu_gyro_full_scale_of(IMU_42686P, 4000) == 4000);
	CHECK(imu_temp_centi_of(IMU_LSM6DSV16X, 256) == 2600);
	CHECK(imu_temp_centi_of(IMU_42686P, 1325) == 3500);   /* 132,48 a degree */
	/* every sensor on its own clock at its fastest, at every rate; the LSM6DSV320X 2 kHz at most */
	CHECK(imu_kind(0x73) == IMU_LSM6DSV320X && imu_is_lsm(IMU_LSM6DSV320X));
	CHECK(!imu_on_grid_of(IMU_LSM6DSV320X, 2000) && !imu_on_grid_of(IMU_LSM6DSV16X, 4000) &&
	      !imu_on_grid_of(IMU_42686P, 32000));
	CHECK(imu_rate_ok_of(IMU_LSM6DSV320X, 2000) && !imu_rate_ok_of(IMU_LSM6DSV320X, 4000));
	CHECK(imu_rate_ok_of(IMU_LSM6DSV16X, 4000) && !imu_rate_ok_of(IMU_LSM6DSV16X, 100));
	CHECK(imu_sensor_hz_of(IMU_LSM6DSV320X, 500, 10) == 7680);
	CHECK(imu_sensor_hz_of(IMU_LSM6DSV16X, 4000, 1) == 7680);
	CHECK(imu_sensor_hz_of(IMU_42686P, 1000, 10) == 32000 && imu_sensor_hz_of(IMU_42686P, 32000, 1) == 32000);
	{
		uint8_t odr, odr_hg;

		CHECK(imu_lsm_own(1000, 10, &odr, &odr_hg, NULL) >= 0 && odr == 0xC && odr_hg == 0x7);
		CHECK(imu_lsm_own(8000, 1, NULL, NULL, NULL) < 0 && imu_lsm_own(100, 1, NULL, NULL, NULL) < 0);
	}
	CHECK(imu_ranges_ok_of(IMU_LSM6DSV320X, 320, 4000) && imu_ranges_ok_of(IMU_LSM6DSV320X, 16, 250));
	CHECK(!imu_ranges_ok_of(IMU_LSM6DSV320X, 320, 125) && !imu_ranges_ok_of(IMU_LSM6DSV320X, 48, 4000));
	CHECK(!imu_ranges_ok_of(IMU_LSM6DSV16X, 32, 4000));
	/* the finest ranges every sensor has, the ICMs' gyro in whole degrees alone */
	CHECK(imu_ranges_ok_of(IMU_LSM6DSV16X, 2, 125) && imu_ranges_ok_of(IMU_LSM6DSV320X, 2, 250));
	CHECK(imu_ranges_ok_of(IMU_42688P, 2, 125) && imu_ranges_ok_of(IMU_42686P, 2, 125));
	CHECK(!imu_ranges_ok_of(IMU_42688P, 2, 62) && !imu_ranges_ok_of(IMU_42686P, 4, 31));
	CHECK(imu_acc_full_scale_of(IMU_LSM6DSV320X, 320) == 341);   /* 10,417 mg a step */
	CHECK(imu_acc_full_scale_of(IMU_LSM6DSV320X, 256) == 256);
	CHECK(imu_acc_full_scale_of(IMU_LSM6DSV16X, 16) == 16 && imu_acc_full_scale_of(IMU_42686P, 32) == 32);
	CHECK(imu_lsm320_gyro_code(250) == 0x09 && imu_lsm320_gyro_code(4000) == 0x0D);
	CHECK(imu_lsm320_gyro_code(125) < 0);
	CHECK(imu_lsm320_hg_code(320) == 4 && imu_lsm320_hg_code(32) == 0 && imu_lsm320_hg_code(16) < 0);
	CHECK(imu_gyro_full_scale_of(IMU_LSM6DSV320X, 4000) == 4588);
	CHECK(imu_acc_g_max_of(IMU_LSM6DSV320X) == 320);
	/* the sensor described for the head: name, sheet, channel, steps */
	CHECK(strcmp(imu_name_of(IMU_LSM6DSV320X), "LSM6DSV320X") == 0);
	CHECK(strcmp(imu_sheet_of(IMU_42686P), "DS-000639 v1.0") == 0);
	CHECK(imu_acc_channel_of(IMU_LSM6DSV320X, 320) == 1 && imu_acc_channel_of(IMU_LSM6DSV320X, 16) == 0);
	CHECK(imu_acc_step_ng_of(IMU_LSM6DSV320X, 320) == 10417000);
	CHECK(imu_acc_step_ng_of(IMU_42686P, 32) == 976563);       /* 32 g / 32 768 */
	CHECK(imu_gyro_step_ndps_of(IMU_LSM6DSV16X, 4000) == 140000000);
	CHECK(imu_gyro_step_ndps_of(IMU_42686P, 4000) == 122070313);
	OK("the sensors' tables");
}

int main(void)
{
	test_layout();
	test_fill();
	test_temp_and_stamp();
	test_tables();
	return 0;
}
