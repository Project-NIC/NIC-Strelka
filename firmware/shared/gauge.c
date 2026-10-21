#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include "gauge.h"

LOG_MODULE_REGISTER(gauge, LOG_LEVEL_INF);

/* the registers (the MAX17055 data sheet, rev. 0, the register map; UG6365) */
#define REG_STATUS     0x00
#define REG_REPSOC     0x06
#define REG_TEMP       0x08
#define REG_VCELL      0x09
#define REG_MIXSOC     0x0D
#define REG_MIXCAP     0x0F
#define REG_FULLCAPREP 0x10
#define REG_CYCLES     0x17
#define REG_DESIGNCAP  0x18
#define REG_ICHGTERM   0x1E
#define REG_FULLCAPNOM 0x23
#define REG_RCOMP0     0x38
#define REG_TEMPCO     0x39
#define REG_VEMPTY     0x3A
#define REG_FSTAT      0x3D
#define REG_DQACC      0x45
#define REG_DPACC      0x46
#define REG_SOFTWAKE   0x60
#define REG_HIBCFG     0xBA
#define REG_MODELCFG   0xDB
#define STATUS_POR     0x0002
#define FSTAT_DNR      0x0001
#define MODELCFG_REFRESH 0x8000
#define CYCLES_BIT6    0x0040

/* the values for the LFP103042 behind the BQ25170 (FIRMWARE.md, The charger and the supply) */
#define DESIGNCAP      0x0B40         /* 720 mAh at 0,25 mAh a step over 20 mΩ */
#define ICHGTERM       0x0180         /* 30 mA */
#define VEMPTY         0x9B53         /* VE 3,10 V, VR 3,32 V */
#define MODELCFG       0x8060         /* Refresh, ModelID 6, LiFePO4 */
#define DPACC_EZ       0x0563         /* dQAcc × 44 138 ÷ DesignCap */
#define DPACC_RESTORE  0x0C80         /* the guide, 4.9 */

static const struct i2c_dt_spec bus = I2C_DT_SPEC_GET(DT_NODELABEL(max17055));
static bool present;
static uint16_t repsoc, vcell, temp, cycles_seen;
static bool cycles_known;

static int rd(uint8_t reg, uint16_t *v)
{
	uint8_t b[2];
	int rc = i2c_write_read_dt(&bus, &reg, 1, b, 2);

	if (rc == 0) {
		*v = (uint16_t)(b[0] | (b[1] << 8));   /* little-endian on the bus */
	}
	return rc;
}

static int wr(uint8_t reg, uint16_t v)
{
	uint8_t b[3] = { reg, (uint8_t)v, (uint8_t)(v >> 8) };

	return i2c_write_dt(&bus, b, sizeof(b));
}

/* the guide's WriteAndVerifyRegister: written, read back, three tries */
static int wr_verify(uint8_t reg, uint16_t v)
{
	for (int i = 0; i < 3; i++) {
		uint16_t back;

		if (wr(reg, v) == 0) {
			k_sleep(K_MSEC(1));
			if (rd(reg, &back) == 0 && back == v) {
				return 0;
			}
		}
	}
	return -EIO;
}

static int wait_dnr_clear(void)
{
	for (int i = 0; i < 100; i++) {           /* a second at most */
		uint16_t fstat;

		if (rd(REG_FSTAT, &fstat) == 0 && !(fstat & FSTAT_DNR)) {
			return 0;
		}
		k_sleep(K_MSEC(10));
	}
	return -ETIMEDOUT;
}

/* the EZ configuration, the guide's steps 2 and 3.1: the six registers in their order, the
 * gauge out of hibernation meanwhile, the model's refresh awaited */
static int ez_config(void)
{
	uint16_t hibcfg;
	int rc = 0;

	rc |= wr(REG_DESIGNCAP, DESIGNCAP);
	rc |= wr(REG_DQACC, DESIGNCAP / 32);
	rc |= wr(REG_ICHGTERM, ICHGTERM);
	rc |= wr(REG_VEMPTY, VEMPTY);
	rc |= rd(REG_HIBCFG, &hibcfg);
	rc |= wr(REG_SOFTWAKE, 0x90);
	rc |= wr(REG_HIBCFG, 0x0);
	rc |= wr(REG_SOFTWAKE, 0x0);
	rc |= wr(REG_DPACC, DPACC_EZ);
	rc |= wr(REG_MODELCFG, MODELCFG);
	for (int i = 0; i < 200 && rc == 0; i++) {   /* two seconds at most */
		uint16_t modelcfg;

		if (rd(REG_MODELCFG, &modelcfg) == 0 && !(modelcfg & MODELCFG_REFRESH)) {
			break;
		}
		k_sleep(K_MSEC(10));
	}
	rc |= wr(REG_HIBCFG, hibcfg);
	return rc ? -EIO : 0;
}

/* the learned values written back after a POR, the guide's 4.5–4.11 */
static int restore(const struct gauge_learned *k)
{
	uint16_t mixsoc;
	int rc = 0;

	rc |= wr_verify(REG_RCOMP0, k->rcomp0);
	rc |= wr_verify(REG_TEMPCO, k->tempco);
	rc |= wr_verify(REG_FULLCAPNOM, k->fullcapnom);
	k_sleep(K_MSEC(350));
	rc |= rd(REG_MIXSOC, &mixsoc);
	rc |= wr_verify(REG_MIXCAP, (uint16_t)(((uint32_t)mixsoc * k->fullcapnom) / 25600u));
	rc |= wr_verify(REG_FULLCAPREP, k->fullcaprep);
	rc |= wr_verify(REG_DPACC, DPACC_RESTORE);
	rc |= wr_verify(REG_DQACC, k->fullcapnom / 16);
	k_sleep(K_MSEC(350));
	rc |= wr_verify(REG_CYCLES, k->cycles);
	return rc ? -EIO : 0;
}

static void read_all(void)
{
	(void)rd(REG_REPSOC, &repsoc);
	(void)rd(REG_VCELL, &vcell);
	(void)rd(REG_TEMP, &temp);
}

int gauge_init(const struct gauge_learned *kept)
{
	uint16_t status;

	if (!i2c_is_ready_dt(&bus) || rd(REG_STATUS, &status) != 0) {
		LOG_WRN("the gauge does not answer");
		return -ENODEV;
	}
	present = true;
	if (status & STATUS_POR) {
		/* its settings lost, the cell joined: set, and the learned values written back */
		if (wait_dnr_clear() || ez_config()) {
			LOG_ERR("the gauge's EZ configuration failed");
			return -EIO;
		}
		if (kept != NULL && kept->fullcapnom != 0 && restore(kept)) {
			LOG_WRN("the learned values did not take");
		}
		if (rd(REG_STATUS, &status) == 0) {
			(void)wr_verify(REG_STATUS, status & ~STATUS_POR);
		}
		LOG_INF("the gauge set for the cell");
	}
	read_all();
	if (rd(REG_CYCLES, &cycles_seen) == 0) {
		cycles_known = true;
	}
	return 0;
}

bool gauge_present(void)
{
	return present;
}

bool gauge_service(struct gauge_learned *out)
{
	uint16_t cycles;

	if (!present) {
		return false;
	}
	read_all();
	if (rd(REG_CYCLES, &cycles) != 0) {
		return false;
	}
	if (cycles_known && ((cycles ^ cycles_seen) & CYCLES_BIT6)) {
		/* every 64 % of the cell's charge passed: what the gauge learned, to keep */
		cycles_seen = cycles;
		out->cycles = cycles;
		return rd(REG_RCOMP0, &out->rcomp0) == 0 && rd(REG_TEMPCO, &out->tempco) == 0 &&
		       rd(REG_FULLCAPREP, &out->fullcaprep) == 0 &&
		       rd(REG_FULLCAPNOM, &out->fullcapnom) == 0;
	}
	cycles_seen = cycles;
	cycles_known = true;
	return false;
}

uint8_t gauge_pct(void)
{
	unsigned pct = repsoc >> 8;      /* 1/256 % a step; the upper byte whole per cents */

	return present ? (uint8_t)(pct > 100 ? 100 : pct) : 0;
}

uint16_t gauge_mv(void)
{
	return present ? (uint16_t)(((uint32_t)vcell * 78125u) / 1000000u) : 0;  /* 78,125 µV */
}

int16_t gauge_temp_centi(void)
{
	return present ? (int16_t)(((int32_t)(int16_t)temp * 100) / 256) : 0;  /* 1/256 °C */
}

bool gauge_empty(void)
{
	return present && repsoc < 256;  /* under 1 %: 0 % reported */
}
