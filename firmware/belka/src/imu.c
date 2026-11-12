/* The ICM-42688-P and the ICM-42686-P share one register map (DS-000347 v1.9, DS-000639
 * v1.0); only the range tables differ. Registers of bank 0 unless said. ST's LSM6DSV16X on the
 * same land has its own (DS13510 Rev 4), its ODR-triggered mode set by AN5763 Rev 4, 3.3; ST's
 * LSM6DSV320X the same map with a high-g channel beside (DS14623 Rev 3). The
 * sensor's node is the overlay's alias imu, SPIM21 at 8 MHz, mode 3, which both take. Built
 * against nRF Connect SDK v3.4.0, not run on hardware: the kit decides it. */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "imu.h"
#include "imu_tables.h"

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

/* the LSM6DSV16X (DS13510 Rev 4, the register map; its outputs little-endian) */
#define LSM_IF_CFG          0x03
#define LSM_ODR_TRIG_CFG    0x06
#define LSM_INT1_CTRL       0x0D
#define LSM_WHO_AM_I        0x0F
#define LSM_CTRL1           0x10   /* the accelerometer: OP_MODE_XL 6:4, ODR_XL 3:0 */
#define LSM_CTRL2           0x11   /* the gyro: OP_MODE_G 6:4, ODR_G 3:0 */
#define LSM_CTRL3           0x12   /* BDU, IF_INC, SW_RESET */
#define LSM_CTRL4           0x13   /* DRDY_PULSED bit 1, INT2_IN_LH bit 0 */
#define LSM_CTRL6           0x15   /* LPF1_G_BW 6:4, FS_G 3:0 */
#define LSM_CTRL7           0x16   /* LPF1_G_EN bit 0 (DS14623 Rev 3, table 67) */
#define LSM_CTRL8           0x17   /* HP_LPF2_XL_BW 7:5, FS_XL 1:0 */
#define LSM_CTRL9           0x18   /* LPF2_XL_EN bit 3 (table 72) */
#define LSM_OUT_TEMP_L      0x20
#define LSM_ODR_TRIGGERED   0x30   /* OP_MODE 011 (tables 51, 54) */
/* the LSM6DSV320X's besides (DS14623 Rev 3) */
#define LSM_CTRL1_XL_HG     0x4E   /* XL_HG_REGOUT_EN 7, ODR_XL_HG 5:3, FS_XL_HG 2:0 (table 147) */
#define LSM_HG_REGOUT_EN    0x80
#define LSM_HIGH_PERF       0x00   /* OP_MODE 000, which the high-g channel asks (6.1.2) */

static uint8_t sensor, whoami;
static bool running;
static uint8_t lsm_odrsel;
static bool lsm_own;                       /* an LSM on its own clock: 2 kHz and below */
static uint8_t lsm_odr, lsm320_hg;         /* ODR_XL and ODR_G; CTRL1_XL_HG, 0 the high-g off */
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

static int lsm_init(void)
{
	uint8_t c3 = 0x01;

	/* SW_RESET, cleared by the chip when done (table 57) */
	reg_write(LSM_CTRL3, 0x01);
	for (int i = 0; i < 10 && (c3 & 0x01); i++) {
		k_msleep(1);
		reg_read(LSM_CTRL3, &c3, 1);
	}
	/* INT1 and INT2 push-pull, active high, SPI of four wires, I2C and I3C off (table 30) */
	reg_write(LSM_IF_CFG, 0x01);
	/* the accelerometer's data-ready on INT1 (table 46), both sensors at one ODR in this mode */
	reg_write(LSM_INT1_CTRL, 0x01);
	return 0;
}

int imu_init(void)
{
	uint8_t id = 0;

	if (!spi_is_ready_dt(&bus)) {
		return -ENODEV;
	}
	/* read before anything is written: 0x75 is the ICM's WHO_AM_I in bank 0, where it wakes,
	 * and the LSM6DSV16X's Z_OFS_USR, 0x00 after reset; then 0x0F, the LSM's WHO_AM_I */
	if (reg_read(REG_WHO_AM_I, &id, 1)) {
		return -EIO;
	}
	if (id != WHOAMI_42686P && id != WHOAMI_42688P && id != WHOAMI_42688V) {
		if (reg_read(LSM_WHO_AM_I, &id, 1)) {
			return -EIO;
		}
		if (id != WHOAMI_LSM6DSV16X && id != WHOAMI_LSM6DSV320X) {
			LOG_ERR("no sensor on the land, 0x%02x at 0x0F", id);
			sensor = 0;
			return -ENODEV;
		}
		whoami = id;
		sensor = imu_kind(id);
		LOG_INF("sensor %s (0x%02x)",
			sensor == IMU_LSM6DSV320X ? "LSM6DSV320X" : "LSM6DSV16X", id);
		return lsm_init();
	}
	bank(0);
	reg_write(REG_DEVICE_CONFIG, 0x01);   /* soft reset */
	k_msleep(2);
	if (reg_read(REG_WHO_AM_I, &id, 1)) {
		return -EIO;
	}
	whoami = id;
	sensor = imu_kind(id);
	if (sensor != IMU_42688P && sensor != IMU_42686P) {
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

uint16_t imu_acc_g_max(void)
{
	return imu_acc_g_max_of(sensor);
}

bool imu_on_grid(void)
{
	return imu_on_grid_of(sensor, current.rate_hz);
}

uint16_t imu_sensor_hz(uint16_t rate_hz, uint8_t n)
{
	return imu_sensor_hz_of(sensor, rate_hz, n);
}

uint16_t imu_acc_full_scale(uint16_t acc_g)
{
	return imu_acc_full_scale_of(sensor, acc_g);
}

uint16_t imu_gyro_dps_max(void)
{
	return imu_gyro_dps_max_of(sensor);
}

uint16_t imu_gyro_full_scale(uint16_t gyro_dps)
{
	return imu_gyro_full_scale_of(sensor, gyro_dps);
}

bool imu_rate_ok(uint16_t rate_hz)
{
	return imu_rate_ok_of(sensor, rate_hz);
}

bool imu_ranges_ok(uint16_t acc_g, uint16_t gyro_dps)
{
	return imu_ranges_ok_of(sensor, acc_g, gyro_dps);
}

uint16_t imu_band_hz(uint16_t rate_hz)
{
	/* the filter's band at ODR/4 (GYRO_ACCEL_CONFIG0 = 0x11); the sheet's table 15 */
	return rate_hz / 4;
}

/* AN5763 Rev 4, 3.3, its order: both sensors in power-down, the samples a reference, both in
 * the ODR-triggered mode, the reference's rising edge; then, the reference running, ODRsel —
 * here in imu_start, or at once when the sensor already runs and its reference with it */
static int lsm_own_configure(const struct imu_config *cfg);

static int lsm_configure(const struct imu_config *cfg)
{
	uint8_t n_odr;

	if (cfg->rate_hz <= IMU_OWN_MOST_HZ) {
		return lsm_own_configure(cfg);
	}
	int r = imu_lsm_rate(cfg->rate_hz, &lsm_odrsel, &n_odr);
	int g = imu_lsm_gyro_code(cfg->gyro_dps), a = imu_lsm_acc_code(cfg->acc_g);

	if (r < 0 || g < 0 || a < 0) {
		return -EINVAL;
	}
	current = *cfg;
	lsm_own = false;
	reg_write(LSM_CTRL1, 0x00);
	reg_write(LSM_CTRL2, 0x00);
	reg_write(LSM_ODR_TRIG_CFG, n_odr);
	reg_write(LSM_CTRL1, LSM_ODR_TRIGGERED);
	reg_write(LSM_CTRL2, LSM_ODR_TRIGGERED);
	/* the data-ready pulsed, the reference taken on its rising edge (table 59; DT0155 Rev 2) */
	reg_write(LSM_CTRL4, 0x03);
	reg_write(LSM_CTRL6, (uint8_t)g);          /* LPF1's band left at its reset value */
	reg_write(LSM_CTRL8, (uint8_t)a);          /* LPF2 off: the band ODR/2 (table 69) */
	if (running) {
		reg_write(LSM_CTRL1, LSM_ODR_TRIGGERED | lsm_odrsel);
		reg_write(LSM_CTRL2, LSM_ODR_TRIGGERED | lsm_odrsel);
	}
	return 0;
}

/* an LSM on its own clock, at 2 kHz and below — the LSM6DSV320X always, its high-g channel taking
 * neither the ODR-triggered mode nor the reference (DS14623 Rev 3, 6.1.2 and table 21): in
 * high-performance mode at its ODR some ten times the rate into memory, LPF1 and LPF2 off, the band
 * its own ODR/2, the filter the main core's (common/resample.c). On the LSM6DSV320X a range above
 * ±16 g the high-g channel's, the low-g one kept at ±16 g, and the high-g's data from
 * UI_OUTX_L_A_OIS_HG, 0x34 (6.2.1); with both kept (cfg->hg) the low-g at its range and the high-g
 * at the other, read in one burst. Both in power-down until imu_start */
static int lsm_own_configure(const struct imu_config *cfg)
{
	bool is320 = sensor == IMU_LSM6DSV320X;
	uint8_t odr_hg;
	uint16_t hg_g = cfg->hg ? cfg->hg : cfg->acc_g;
	int r = imu_lsm_own(cfg->rate_hz, cfg->n, &lsm_odr, &odr_hg, NULL);
	int g = is320 ? imu_lsm320_gyro_code(cfg->gyro_dps) : imu_lsm_gyro_code(cfg->gyro_dps);
	int a = cfg->acc_g <= 16 ? imu_lsm_acc_code(cfg->acc_g) : imu_lsm_acc_code(16);
	int hg = !is320 || hg_g <= 16 ? 0 : imu_lsm320_hg_code(hg_g);

	if (r < 0 || g < 0 || a < 0 || hg < 0 || (cfg->hg && cfg->acc_g > 16) ||
	    (!is320 && (cfg->hg || cfg->acc_g > 16))) {
		return -EINVAL;
	}
	current = *cfg;
	lsm_own = true;
	lsm320_hg = !is320 || hg_g <= 16 ? 0 :
		    (uint8_t)(LSM_HG_REGOUT_EN | (odr_hg << 3) | (uint8_t)hg);
	if (is320) {
		reg_write(LSM_CTRL1_XL_HG, 0x00);
	}
	reg_write(LSM_CTRL1, 0x00);
	reg_write(LSM_CTRL2, 0x00);
	reg_write(LSM_CTRL4, 0x02);               /* the data-ready pulsed; pin 9 left alone */
	reg_write(LSM_CTRL6, (uint8_t)g);         /* FS_G (the LSM6DSV320X's with its bit 3) */
	reg_write(LSM_CTRL7, 0x00);               /* LPF1 off */
	reg_write(LSM_CTRL8, (uint8_t)a);         /* FS_XL */
	reg_write(LSM_CTRL9, 0x00);               /* LPF2 off: the band ODR/2 (table 71) */
	if (running) {
		reg_write(LSM_CTRL1, LSM_HIGH_PERF | lsm_odr);
		reg_write(LSM_CTRL2, LSM_HIGH_PERF | lsm_odr);
		if (is320) {
			reg_write(LSM_CTRL1_XL_HG, lsm320_hg);
		}
	}
	return 0;
}

static int icm_configure(const struct imu_config *cfg)
{
	bool own = cfg->rate_hz <= IMU_OWN_MOST_HZ;
	int odr = imu_icm_odr_code(own ? imu_icm_own_hz(cfg->rate_hz, cfg->n) : cfg->rate_hz);
	uint8_t c1;
	/* ±16 … ±2 g on the -42688-P, ±32 … ±2 g on the -42686-P (DS-000639, ACCEL_FS_SEL) */
	int acc = imu_icm_fs_code(cfg->acc_g, imu_acc_g_max(), sensor == IMU_42686P ? 5 : 4);
	int gyro = imu_icm_fs_code(cfg->gyro_dps, imu_gyro_dps_max(), 8);

	if (odr < 0 || acc < 0 || gyro < 0) {
		return -EINVAL;
	}
	current = *cfg;
	bank(0);
	reg_write(REG_GYRO_CONFIG0, (uint8_t)((gyro << 5) | odr));
	reg_write(REG_ACCEL_CONFIG0, (uint8_t)((acc << 5) | odr));
	/* on its own clock, 2 kHz and below: both UI filters at their widest, ODR/2, the filter the
	 * main core's (common/resample.c); steered, the raw rates: at ODR/4 (14.40, reset 0x11) */
	reg_write(REG_GYRO_ACCEL_CONFIG0, own ? 0x00 : 0x11);
	/* RTC_MODE, bit 2 of INTF_CONFIG1, its other bits as they are — 0x51 after reset on the
	 * -42686-P, 0x91 on the -42688-P (14.35) — and PIN9_FUNCTION in bank 1's INTF_CONFIG5:
	 * CLKIN (10) when steered, INT2 (00) and left alone on its own clock */
	if (reg_read(REG_INTF_CONFIG1, &c1, 1) == 0) {
		reg_write(REG_INTF_CONFIG1, own ? (uint8_t)(c1 & ~0x04) : (uint8_t)(c1 | 0x04));
	}
	bank(1);
	reg_write(REG_INTF_CONFIG5, own ? 0x00 : 0x04);
	bank(0);
	/* the FIFO: stream, a packet of accel + gyro + temp, for the rates above 2 kHz; bypassed
	 * below, each sample read as it comes */
	reg_write(REG_FIFO_CONFIG, own ? 0x00 : 0x40);
	reg_write(REG_FIFO_CONFIG1, 0x07);
	return 0;
}

int imu_configure(const struct imu_config *cfg)
{
	if (sensor == IMU_LSM6DSV320X) {
		return lsm_own_configure(cfg);
	}
	return sensor == IMU_LSM6DSV16X ? lsm_configure(cfg) : icm_configure(cfg);
}

int imu_start(void)
{
	if (imu_is_lsm(sensor) && lsm_own) {
		running = true;
		reg_write(LSM_CTRL1, LSM_HIGH_PERF | lsm_odr);
		reg_write(LSM_CTRL2, LSM_HIGH_PERF | lsm_odr);
		return sensor == IMU_LSM6DSV320X ? reg_write(LSM_CTRL1_XL_HG, lsm320_hg) : 0;
	}
	if (sensor == IMU_LSM6DSV16X) {
		/* ODRsel, the reference already running: locked within 4 references (AN5763, 3.3) */
		running = true;
		reg_write(LSM_CTRL1, LSM_ODR_TRIGGERED | lsm_odrsel);
		return reg_write(LSM_CTRL2, LSM_ODR_TRIGGERED | lsm_odrsel);
	}
	running = true;
	/* gyro and accelerometer in low-noise mode; the gyro needs 30 ms before its data, and is
	 * kept on 45 ms at least (DS-000347, table 1 and 14.36) */
	bank(0);
	int r = reg_write(REG_PWR_MGMT0, 0x0F);
	k_msleep(45);
	return r;
}

int imu_stop(void)
{
	running = false;
	if (imu_is_lsm(sensor) && lsm_own) {
		if (sensor == IMU_LSM6DSV320X) {
			reg_write(LSM_CTRL1_XL_HG, 0x00);
		}
		reg_write(LSM_CTRL1, 0x00);
		return reg_write(LSM_CTRL2, 0x00);
	}
	if (sensor == IMU_LSM6DSV16X) {
		reg_write(LSM_CTRL1, LSM_ODR_TRIGGERED);    /* the ODR-triggered mode, powered down */
		return reg_write(LSM_CTRL2, LSM_ODR_TRIGGERED);
	}
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

/* an LSM's 14 B from OUT_TEMP_L: the temperature, the gyro's x y z, the accelerometer's, each
 * little-endian (DS13510 Rev 4, the register map); on the LSM6DSV320X with its high-g channel on,
 * 26 B in one burst, the high-g's x y z the last 6, from 0x34, in the low-g's place (DS14623 Rev
 * 3, the register map) */
static int lsm_read_step(struct strelka_step *step, int16_t *high, bool *acc_out, bool *gyro_out)
{
	uint8_t raw[26];
	bool hg = sensor == IMU_LSM6DSV320X && lsm320_hg != 0;
	bool both = hg && current.hg != 0;       /* the low-g in its place, the high-g beside */
	int at = hg && !both ? 20 : 8;
	int r = reg_read(LSM_OUT_TEMP_L, raw, hg ? 26 : 14);

	if (r) {
		return r;
	}
	last_temp = (int16_t)(raw[0] | (raw[1] << 8));
	*acc_out = *gyro_out = false;
	for (int i = 0; i < 3; i++) {
		step->g[i] = (int16_t)(raw[2 + 2 * i] | (raw[3 + 2 * i] << 8));
		step->a[i] = (int16_t)(raw[at + 2 * i] | (raw[at + 1 + 2 * i] << 8));
		if (step->a[i] == INT16_MAX || step->a[i] == INT16_MIN) {
			*acc_out = true;
		}
		if (step->g[i] == INT16_MAX || step->g[i] == INT16_MIN) {
			*gyro_out = true;
		}
		if (high != NULL) {
			high[i] = both ? (int16_t)(raw[20 + 2 * i] | (raw[21 + 2 * i] << 8)) : 0;
		}
	}
	return 0;
}

static int icm_read_step(struct strelka_step *step, bool *acc_out, bool *gyro_out)
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

int imu_read_step(struct strelka_step *step, int16_t *hg, bool *acc_out, bool *gyro_out)
{
	if (imu_is_lsm(sensor)) {
		return lsm_read_step(step, hg, acc_out, gyro_out);
	}
	if (hg != NULL) {
		hg[0] = hg[1] = hg[2] = 0;           /* an ICM has no high-g channel */
	}
	return icm_read_step(step, acc_out, gyro_out);
}

int16_t imu_last_temp(void)
{
	return last_temp;
}

int16_t imu_temp_centi(void)
{
	return imu_temp_centi_of(sensor, last_temp);
}

static int icm_read_fifo(struct strelka_step *steps, int max_steps)
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

int imu_read_fifo(struct strelka_step *steps, int max_steps)
{
	if (imu_is_lsm(sensor)) {
		return -ENOTSUP;    /* their rates, 7,68 kHz at most, are read from the registers */
	}
	return icm_read_fifo(steps, max_steps);
}
