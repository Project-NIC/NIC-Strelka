/* The ICM-42688-P and the ICM-42686-P share one register map (DS-000347 v1.9, DS-000639
 * v1.0); only the range tables differ. Registers of bank 0 unless said. The sensor's node is the
 * overlay's alias imu, SPIM21 at 8 MHz, mode 3. Carried over from the earlier code; built against nRF Connect SDK v3.4.0, not run on hardware: the kit decides it. */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "imu.h"

LOG_MODULE_REGISTER(imu, LOG_LEVEL_INF);

#define IMU_NODE DT_ALIAS(imu)
static const struct spi_dt_spec bus = SPI_DT_SPEC_GET(IMU_NODE,
	SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA);

/* bank 0 */
#define REG_DEVICE_CONFIG   0x11
#define REG_INT_CONFIG      0x14
#define REG_FIFO_CONFIG     0x16
#define REG_TEMP_DATA1      0x1D
#define REG_INT_STATUS      0x2D
#define REG_FIFO_COUNTH     0x2E
#define REG_FIFO_DATA       0x30
#define REG_INTF_CONFIG0    0x4C
#define REG_INTF_CONFIG1    0x4D
#define REG_PWR_MGMT0       0x4E
#define REG_GYRO_CONFIG0    0x4F
#define REG_ACCEL_CONFIG0   0x50
#define REG_GYRO_CONFIG1    0x51
#define REG_GYRO_ACCEL_CONFIG0 0x52
#define REG_ACCEL_CONFIG1   0x53
#define REG_FIFO_CONFIG1    0x5F
#define REG_INT_CONFIG0     0x63
#define REG_INT_CONFIG1     0x64
#define REG_INT_SOURCE0     0x65
#define REG_WHO_AM_I        0x75
#define REG_BANK_SEL        0x76
/* bank 1 */
#define REG_INTF_CONFIG5    0x7B   /* PIN9_FUNCTION bits 2:1 — 10 is CLKIN */

#define WHOAMI_42688P 0x47
#define WHOAMI_42688V 0xDB   /* the -V, taken as a -P */
#define WHOAMI_42686P 0x44

static uint8_t sensor, whoami;
static struct imu_config current;
static int16_t last_temp;

static int reg_write(uint8_t reg, uint8_t val)
{
	uint8_t tx[2] = { reg & 0x7F, val };
	const struct spi_buf b = { .buf = tx, .len = 2 };
	const struct spi_buf_set s = { .buffers = &b, .count = 1 };
	return spi_write_dt(&bus, &s);
}

static int reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
	uint8_t cmd = reg | 0x80;
	const struct spi_buf tx = { .buf = &cmd, .len = 1 };
	const struct spi_buf rx[2] = { { .buf = NULL, .len = 1 }, { .buf = buf, .len = len } };
	const struct spi_buf_set ts = { .buffers = &tx, .count = 1 };
	const struct spi_buf_set rs = { .buffers = rx, .count = 2 };
	return spi_transceive_dt(&bus, &ts, &rs);
}

static int bank(uint8_t b)
{
	return reg_write(REG_BANK_SEL, b);
}

int imu_init(void)
{
	uint8_t id = 0;

	if (!spi_is_ready_dt(&bus)) {
		return -ENODEV;
	}
	bank(0);
	reg_write(REG_DEVICE_CONFIG, 0x01);   /* soft reset */
	k_msleep(2);
	if (reg_read(REG_WHO_AM_I, &id, 1)) {
		return -EIO;
	}
	whoami = id;
	switch (id) {
	case WHOAMI_42688P:
	case WHOAMI_42688V:
		sensor = IMU_42688P;
		break;
	case WHOAMI_42686P:
		sensor = IMU_42686P;
		break;
	default:
		LOG_ERR("no sensor on the land, WHO_AM_I 0x%02x", id);
		sensor = 0;
		return -ENODEV;
	}
	LOG_INF("sensor %s (0x%02x)", sensor == IMU_42686P ? "ICM-42686-P" : "ICM-42688-P", id);

	/* INT1 push-pull, active high, pulsed */
	reg_write(REG_INT_CONFIG, 0x03);
	/* the pulse 8 µs and no de-assert hold, which the sheet requires at 4 kHz and above and
	 * allows below (INT_TPULSE_DURATION, INT_TDEASSERT_DISABLE); INT_ASYNC_RESET cleared as the
	 * sheet asks (DS-000439, 14, INT_CONFIG1; 12, the use notes) */
	reg_write(REG_INT_CONFIG1, 0x60);
	/* the data-ready on INT1 */
	reg_write(REG_INT_SOURCE0, 0x08);
	/* the reset value: the FIFO count and the data big-endian, as unpack() reads them */
	reg_write(REG_INTF_CONFIG0, 0x30);
	return 0;
}

uint8_t imu_sensor(void)
{
	return sensor;
}

uint8_t imu_whoami(void)
{
	return whoami;
}

uint8_t imu_acc_g_max(void)
{
	return sensor == IMU_42686P ? 32 : 16;
}

uint16_t imu_gyro_dps_max(void)
{
	return sensor == IMU_42686P ? 4000 : 2000;
}

/* the ODR codes of GYRO_CONFIG0 / ACCEL_CONFIG0 bits 3:0 (DS-000347, 14.37, 14.38) */
static int odr_code(uint16_t hz)
{
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

/* the full-scale codes, bits 7:5: the ICM-42688-P's table starts at ±16 g / ±2000 °/s, the
 * ICM-42686-P's at ±32 g / ±4000 °/s, each step below halving the range */
static int fs_code(uint16_t want, uint16_t top, int steps)
{
	uint16_t r = top;

	for (int i = 0; i < steps; i++, r /= 2) {
		if (r == want) {
			return i;
		}
	}
	return -1;
}

bool imu_rate_ok(uint16_t rate_hz)
{
	return odr_code(rate_hz) >= 0;
}

bool imu_ranges_ok(uint8_t acc_g, uint16_t gyro_dps)
{
	return fs_code(acc_g, imu_acc_g_max(), sensor == IMU_42686P ? 5 : 4) >= 0 &&
	       fs_code(gyro_dps, imu_gyro_dps_max(), 8) >= 0;
}

uint16_t imu_band_hz(uint16_t rate_hz)
{
	/* the filter's band at ODR/4 (GYRO_ACCEL_CONFIG0 = 0x11); the sheet's table 15 */
	return rate_hz / 4;
}

int imu_configure(const struct imu_config *cfg)
{
	int odr = odr_code(cfg->rate_hz);
	/* ±16 … ±2 g on the -42688-P, ±32 … ±2 g on the -42686-P (DS-000639, ACCEL_FS_SEL) */
	int acc = fs_code(cfg->acc_g, imu_acc_g_max(), sensor == IMU_42686P ? 5 : 4);
	int gyro = fs_code(cfg->gyro_dps, imu_gyro_dps_max(), 8);

	if (odr < 0 || acc < 0 || gyro < 0) {
		return -EINVAL;
	}
	current = *cfg;
	bank(0);
	reg_write(REG_GYRO_CONFIG0, (uint8_t)((gyro << 5) | odr));
	reg_write(REG_ACCEL_CONFIG0, (uint8_t)((acc << 5) | odr));
	reg_write(REG_GYRO_ACCEL_CONFIG0, 0x11);     /* both filters at ODR/4 */
	/* CLKIN: RTC_MODE in INTF_CONFIG1, PIN9_FUNCTION = 10 in bank 1's INTF_CONFIG5 */
	reg_write(REG_INTF_CONFIG1, 0x95);           /* RTC_MODE = 1, the rest default */
	bank(1);
	reg_write(REG_INTF_CONFIG5, 0x04);
	bank(0);
	/* the FIFO: stream, a packet of accel + gyro + temp, for the rates above 2 kHz */
	reg_write(REG_FIFO_CONFIG, 0x40);
	reg_write(REG_FIFO_CONFIG1, 0x07);
	return 0;
}

int imu_start(void)
{
	/* gyro and accelerometer in low-noise mode; the gyro needs 30 ms before its data, and is
	 * kept on 45 ms at least (DS-000347, table 1 and 14.36) */
	bank(0);
	int r = reg_write(REG_PWR_MGMT0, 0x0F);
	k_msleep(45);
	return r;
}

int imu_stop(void)
{
	bank(0);
	return reg_write(REG_PWR_MGMT0, 0x00);
}

static void unpack(const uint8_t *p, struct strelka_step *step, bool *acc_clip, bool *gyro_clip)
{
	for (int i = 0; i < 3; i++) {
		step->a[i] = (int16_t)((p[2 * i] << 8) | p[2 * i + 1]);
		step->g[i] = (int16_t)((p[6 + 2 * i] << 8) | p[6 + 2 * i + 1]);
	}
	*acc_clip = *gyro_clip = false;
	for (int i = 0; i < 3; i++) {
		if (step->a[i] == INT16_MAX || step->a[i] == INT16_MIN) {
			*acc_clip = true;
		}
		if (step->g[i] == INT16_MAX || step->g[i] == INT16_MIN) {
			*gyro_clip = true;
		}
	}
}

int imu_read_step(struct strelka_step *step, bool *acc_out, bool *gyro_out)
{
	/* 14 B from TEMP_DATA1 through the gyro's z: temperature 2 B, then ax ay az gx gy gz */
	uint8_t raw[14];
	int r = reg_read(REG_TEMP_DATA1, raw, sizeof(raw));

	if (r) {
		return r;
	}
	last_temp = (int16_t)((raw[0] << 8) | raw[1]);
	unpack(raw + 2, step, acc_out, gyro_out);
	return 0;
}

int16_t imu_last_temp(void)
{
	return last_temp;
}

int16_t imu_temp_centi(void)
{
	/* °C = TEMP_DATA / 132,48 + 25 (DS-000347, 14.6), in hundredths */
	return (int16_t)(2500 + (int32_t)last_temp * 10000 / 13248);
}

int imu_read_fifo(struct strelka_step *steps, int max_steps)
{
	/* a packet of 16 B: header, accel 6, gyro 6, temp 1, timestamp 2 (DS-000347, 6.1, packet 3) */
	uint8_t cnt[2];
	int r = reg_read(REG_FIFO_COUNTH, cnt, 2);
	int bytes, packets, n = 0;
	static uint8_t buf[2048];

	if (r) {
		return r;
	}
	bytes = (cnt[0] << 8) | cnt[1];
	packets = bytes / 16;
	if (packets > max_steps) {
		packets = max_steps;
	}
	if (packets == 0) {
		return 0;
	}
	r = reg_read(REG_FIFO_DATA, buf, (size_t)packets * 16);
	if (r) {
		return r;
	}
	for (int i = 0; i < packets; i++) {
		const uint8_t *p = buf + i * 16;
		bool ac, gc;

		if ((p[0] & 0x60) != 0x60) {   /* a packet without both sensors: skip */
			continue;
		}
		unpack(p + 1, &steps[n++], &ac, &gc);
		/* the packet's temperature, one byte: °C = FIFO_TEMP_DATA / 2,07 + 25, against the
		 * register's TEMP_DATA / 132,48 + 25 (DS-000347, 6.1 and 14.6): 64 times it */
		last_temp = (int16_t)((int8_t)p[13] * 64);
	}
	return n;
}
