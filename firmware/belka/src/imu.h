/* NIC-Strelka — the motion sensor: an ICM-42688-P or an ICM-42686-P on SPI, one land, told by
 * WHO_AM_I; CLKIN from the processor; the data registers at every data-ready up to some 2 kHz,
 * the FIFO in bursts above until the RISC-V coprocessor takes the raw rates (FIRMWARE.md, The
 * sensor and its clock; Reading the sensor). Built against nRF Connect SDK v3.4.0, not run on
 * hardware: the kit decides it. */
#ifndef STRELKA_IMU_H
#define STRELKA_IMU_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define IMU_42688P 1
#define IMU_42686P 2

struct imu_config {
	uint16_t rate_hz;      /* the sensor's rate: the rate into memory times N */
	uint8_t acc_g;         /* ±2, 4, 8, 16; 32 on the ICM-42686-P */
	uint16_t gyro_dps;     /* ±250 … 2000; 4000 on the ICM-42686-P */
};

int imu_init(void);                       /* finds the chip; -ENODEV if none answers */
uint8_t imu_sensor(void);                 /* IMU_42688P or IMU_42686P, 0 if none */
uint8_t imu_whoami(void);                 /* 0x47, 0x44 or 0xDB, for the application (64/0) */
uint8_t imu_acc_g_max(void);              /* 16 or 32 */
uint16_t imu_gyro_dps_max(void);          /* 2000 or 4000 */
int imu_configure(const struct imu_config *cfg); /* ranges, rate, filters, CLKIN mode, INT1 */
int imu_start(void);                      /* the gyro and the accelerometer on, low-noise */
int imu_stop(void);
int imu_read_step(struct strelka_step *step, bool *acc_out, bool *gyro_out);
int16_t imu_last_temp(void);              /* TEMP_DATA raw from the last read */
int16_t imu_temp_centi(void);             /* the same in 0,01 °C: TEMP_DATA / 132,48 + 25 */
int imu_read_fifo(struct strelka_step *steps, int max_steps); /* the steps read */
bool imu_rate_ok(uint16_t rate_hz);
bool imu_ranges_ok(uint8_t acc_g, uint16_t gyro_dps);

#endif
