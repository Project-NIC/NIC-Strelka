/* NIC-Strelka — the sensors' tables, one home for both cores: the chips by WHO_AM_I, their rates,
 * ranges and codes, the gyro's full scale and the temperature by each chip's own scale
 * (FIRMWARE.md, The sensor and its clock; PARTS.md, The motion sensor). Plain C, checked on the
 * host (tests/test_imu_tables.c); belka/src/imu.c sets the chip by them, on the FLPR or, the
 * fallback, on the main core, and belka/src/sensor_flpr.c answers the main core by them. */
#ifndef STRELKA_IMU_TABLES_H
#define STRELKA_IMU_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#define IMU_42688P          1
#define IMU_42686P          2
#define IMU_LSM6DSV16X      3
#define IMU_LSM6DSV320X     4      /* the hoof's: off the set's clock, its samples put onto the grid */

#define WHOAMI_42688P       0x47   /* DS-000347, 14.58 */
#define WHOAMI_42688V       0xDB   /* the -V, taken as a -P (DS-000439, 14.58) */
#define WHOAMI_42686P       0x44   /* DS-000639, 14.58 */
#define WHOAMI_LSM6DSV16X   0x70   /* DS13510 Rev 4, 9.13 */
#define WHOAMI_LSM6DSV320X  0x73   /* DS14623 Rev 3, the register map */

uint8_t imu_kind(uint8_t whoami);                    /* IMU_…, 0 for none */
bool imu_is_lsm(uint8_t kind);                       /* ST's: the LSM6DSV16X or the LSM6DSV320X */
uint16_t imu_acc_g_max_of(uint8_t kind);             /* 16, 32 or 320 */
uint16_t imu_gyro_dps_max_of(uint8_t kind);          /* 2000 or 4000 */
bool imu_rate_ok_of(uint8_t kind, uint16_t rate_hz);
bool imu_ranges_ok_of(uint8_t kind, uint16_t acc_g, uint16_t gyro_dps);
/* every sensor runs on its own clock at its fastest, at every rate, its samples filtered onto the
 * grid by common/resample.c; pin 9 is wired on every board and the steered way kept in the code: a
 * rate above IMU_OWN_MOST_HZ would be steered there again — the LSM6DSV16X's reference or an ICM's
 * CLKIN — never the LSM6DSV320X's, its 320 g channel having no ODR-triggered mode (DS14623 Rev 3,
 * table 21), 2 kHz its most */
#define IMU_OWN_MOST_HZ  32000
bool imu_on_grid_of(uint8_t kind, uint16_t rate_hz);
/* the sensor's own rate for a rate on the grid whose steps hold n samples: the same, or the
 * LSM6DSV320X's ODR, some ten times the rate into memory */
uint16_t imu_sensor_hz_of(uint8_t kind, uint16_t rate_hz, uint8_t n);
/* what 32 768 of the gyro's steps make, ±°/s: the range on an ICM, wider on the LSMs */
uint16_t imu_gyro_full_scale_of(uint8_t kind, uint16_t gyro_dps);
/* what 32 768 of the accelerometer's steps make, ±g: the range, or the LSM6DSV320X's 341 at its
 * ±320 g (DS14623 Rev 3, table 3) */
uint16_t imu_acc_full_scale_of(uint8_t kind, uint16_t acc_g);
/* the sensor described for the record's head (RECORD.md), so that a reader needs no table: its
 * name and its sheet; the channel a range takes, 1 the LSM6DSV320X's high-g; the steps, the
 * accelerometer's in nano-g and the gyro's in nano-degrees a second, from each sheet */
const char *imu_name_of(uint8_t kind);
const char *imu_sheet_of(uint8_t kind);
uint8_t imu_acc_channel_of(uint8_t kind, uint16_t acc_g);
uint32_t imu_acc_step_ng_of(uint8_t kind, uint16_t acc_g);
uint32_t imu_gyro_step_ndps_of(uint8_t kind, uint16_t gyro_dps);
/* the temperature register in 0,01 °C, by the chip's own scale */
int16_t imu_temp_centi_of(uint8_t kind, int16_t raw);

/* the ICMs' codes: GYRO_CONFIG0 / ACCEL_CONFIG0 bits 3:0, and the full-scale steps below the top */
int imu_icm_odr_code(uint16_t hz);
int imu_icm_fs_code(uint16_t want, uint16_t top, int steps);
/* the LSM6DSV16X's: its rate's ODRsel and ODR_TRIG_N_ODR, its gyro's FS_G, its FS_XL */
int imu_lsm_rate(uint16_t hz, uint8_t *odrsel, uint8_t *n_odr);
int imu_lsm_gyro_code(uint16_t dps);
int imu_lsm_acc_code(uint16_t g);
/* the LSMs on their own clock, on a grid of 200 to 2 000 Hz, the LSM6DSV16X's 4 000 too: ODR_XL /
 * ODR_G, the LSM6DSV320X's ODR_XL_HG, and the sensor's own rate, its fastest, 7 680 Hz; −1 for a
 * grid it has not. An ICM's own rate, its fastest, 32 kHz.
 * The LSM6DSV320X's gyro CTRL6 (FS_G with the bit that must be 1); its high-g FS_XL_HG */
int imu_lsm_own(uint16_t hz, uint8_t n, uint8_t *odr, uint8_t *odr_hg, uint16_t *sensor_hz);
uint16_t imu_icm_own_hz(uint16_t hz, uint8_t n);
int imu_lsm320_gyro_code(uint16_t dps);
int imu_lsm320_hg_code(uint16_t g);

#endif
