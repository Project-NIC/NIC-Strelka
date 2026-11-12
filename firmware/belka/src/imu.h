/* NIC-Strelka — the motion sensor on SPI, one land for ST's LSM6DSV16X, the classic build's, ST's
 * LSM6DSV320X, the hoof's, and TDK's ICM-42686-P or -42688-P, told by WHO_AM_I; pin 9 CLKIN for
 * the ICM, the ODR-triggered mode's reference for the LSM6DSV16X, nothing for the LSM6DSV320X,
 * off the set's clock; the data registers at every data-ready, on the FLPR or, the fallback, the
 * main core (FIRMWARE.md, The sensor and its clock; Reading the sensor). Built against nRF
 * Connect SDK v3.4.0, not run on hardware: the kit decides it. */
#ifndef STRELKA_IMU_H
#define STRELKA_IMU_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
#include "imu_tables.h"   /* IMU_42688P, IMU_42686P, IMU_LSM6DSV16X, IMU_LSM6DSV320X */

struct imu_config {
	uint16_t rate_hz;      /* the rate on the grid: the rate into memory times N */
	uint16_t acc_g;        /* ±2, 4, 8, 16; 32 on the ICM-42686-P; 32 to 320 on the LSM6DSV320X,
				* its high-g channel */
	uint16_t gyro_dps;     /* ±250 … 2000; 4000 on the ICM-42686-P and the LSMs */
	uint16_t hg;           /* both channels on the LSM6DSV320X: the high-g's ±g, 32–320, the
				* low-g's then acc_g, ±16 g at most; 0 none */
	uint8_t n;             /* the samples a step, N: the LSM6DSV320X's own rate and its filters
				* follow the rate into memory, rate_hz / N; 0 taken as 1 */
};

int imu_init(void);                       /* finds the chip; -ENODEV if none answers */
uint8_t imu_sensor(void);                 /* IMU_…, 0 if none */
uint8_t imu_whoami(void);                 /* 0x47, 0x44, 0xDB, 0x70 or 0x73, for the application */
uint16_t imu_acc_g_max(void);             /* 16, 32 or 320 */
bool imu_on_grid(void);                   /* false: the samples put onto the grid (resample.h) */
/* the sensor's own rate for a rate on the grid whose steps hold n samples */
uint16_t imu_sensor_hz(uint16_t rate_hz, uint8_t n);
/* the accelerometer's full scale its 32 768 steps make (RECORD.md, the head's bytes 143-144) */
uint16_t imu_acc_full_scale(uint16_t acc_g);
uint16_t imu_gyro_dps_max(void);          /* 2000 or 4000 */
/* the gyro's full scale its 32 768 steps make, for the record's head and the orientation: the
 * range on an ICM, wider on the LSM6DSV16X (RECORD.md, the head's bytes 28-29) */
uint16_t imu_gyro_full_scale(uint16_t gyro_dps);
int imu_configure(const struct imu_config *cfg); /* ranges, rate, filters, CLKIN mode, INT1 */
int imu_start(void);                      /* the gyro and the accelerometer on, low-noise */
int imu_stop(void);
int imu_read_step(struct strelka_step *step, int16_t *hg, bool *acc_out, bool *gyro_out);
int16_t imu_last_temp(void);              /* TEMP_DATA raw from the last read */
int16_t imu_temp_centi(void);             /* the same in 0,01 °C, by the chip's own scale */
int imu_read_fifo(struct strelka_step *steps, int max_steps); /* the steps read */
bool imu_rate_ok(uint16_t rate_hz);
bool imu_ranges_ok(uint16_t acc_g, uint16_t gyro_dps);

/* while the FLPR reads the sensor (CONFIG_STRELKA_SENSOR_FLPR, src/sensor_flpr.c): the next sample
 * out of the ring with its stamp, 1 if there was one; and the samples the FLPR lost or passed */
/* hg: the high-g channel's three where both are kept; NULL to leave them */
int imu_ring_pop(struct strelka_step *step, int16_t *hg, uint64_t *stamp, bool *acc_out,
		 bool *gyro_out);
uint32_t imu_dropped(void);

#endif
