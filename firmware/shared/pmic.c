/* NIC-Strelka — the PMIC (pmic.h). The registers named are the nPM1300's product specification
 * v1.1's: SHPHLDSTATUS 0x0B05 (7.4.2.6), LEDDRVnMODESEL 0x0A00 + n and LEDDRVnSET/CLR 0x0A03 +
 * 2n (6.5.2), BUCK2PWMSET and BUCK2PWMCLR 0x0406 and 0x0407 (6.3.8.7–8), the MAIN block's events, their CLR at +1 and INTENCLR at +3 of each group (7.7.1),
 * the charger's status and error through the SDK's sensor (6.2.14). The processor's soft reset
 * does not reset the PMIC: what the last run left enabled and pending is cleared at the start,
 * so that a stale event does not hold PMIC_INT high and hide every edge after it. The LEDs are
 * written here through the MFD rather than by the SDK's LED driver, which keeps the modes it is
 * given at its start; the PMIC's reset modes are the ones wanted until the firmware takes the
 * light. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include "pmic.h"

LOG_MODULE_REGISTER(pmic, LOG_LEVEL_INF);

#define SHIP_BASE          0x0Bu
#define SHIP_SHPHLDSTATUS  0x05u
#define LED_BASE           0x0Au
#define BUCK_BASE          0x04u
#define BUCK2_PWMSET       0x06u
#define BUCK2_PWMCLR       0x07u
#define LED_MODE           0x00u
#define LED_SET            0x03u
#define LED_CLR            0x04u
#define LED_MODE_ERROR     0u
#define LED_MODE_CHARGING  1u
#define LED_MODE_HOST      2u
/* BCHGCHARGESTATUS (6.2.14): bit 1 complete, 2 trickle, 3 constant current, 4 constant voltage */
#define CHG_COMPLETE       BIT(1)
#define CHG_RUNNING        (BIT(2) | BIT(3) | BIT(4))

static const struct device *const mfd = DEVICE_DT_GET(DT_NODELABEL(pmic));
static const struct device *const charger = DEVICE_DT_GET(DT_NODELABEL(pmic_charger));
static const struct device *const regulators = DEVICE_DT_GET(DT_NODELABEL(pmic_regulators));
static bool ready;
static K_MUTEX_DEFINE(lock);

#define MAIN_BASE 0x00u
/* the MAIN block's event groups: ADC, BCHARGER0–2, SHPHLD, VBUSIN0–1, GPIO (7.7.1) */
static const uint8_t event_groups[] = { 0x02, 0x06, 0x0A, 0x0E, 0x12, 0x16, 0x1A, 0x22 };

int pmic_init(void)
{
	/* the USB input's limit of the devicetree applied now: the driver writes it as the startup
	 * limit alone, VBUSINILIMSTARTUP, for the next plug (6.1.8) */
	const struct sensor_value limit = {
		.val1 = DT_PROP(DT_NODELABEL(pmic_charger), vbus_limit_microamp) / 1000000,
		.val2 = DT_PROP(DT_NODELABEL(pmic_charger), vbus_limit_microamp) % 1000000,
	};

	ready = device_is_ready(mfd) && device_is_ready(charger);
	if (!ready) {
		LOG_ERR("the PMIC silent");
		return -ENODEV;
	}
	for (size_t g = 0; g < ARRAY_SIZE(event_groups); g++) {
		mfd_npm13xx_reg_write(mfd, MAIN_BASE, event_groups[g] + 3u, 0xFFu);   /* INTENCLR */
		mfd_npm13xx_reg_write(mfd, MAIN_BASE, event_groups[g] + 1u, 0xFFu);   /* CLR */
	}
	if (sensor_attr_set(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_CONFIGURATION, &limit) != 0) {
		LOG_WRN("the USB input's limit not raised");
	}
	return 0;
}

bool pmic_present(void)
{
	return ready;
}

const struct device *pmic_charger(void)
{
	return charger;
}

int pmic_listen(struct gpio_callback *cb, gpio_callback_handler_t handler, uint32_t events)
{
	if (!ready) {
		return -ENODEV;
	}
	gpio_init_callback(cb, handler, events);
	return mfd_npm13xx_add_callback(mfd, cb);
}

bool pmic_vbus(void)
{
	struct sensor_value v = { 0 };

	if (!ready || sensor_attr_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM13XX_CHARGER_VBUS_STATUS,
				      (enum sensor_attribute)SENSOR_ATTR_NPM13XX_CHARGER_VBUS_PRESENT,
				      &v) != 0) {
		return false;
	}
	return v.val1 != 0;
}

bool pmic_magnet_a_held(void)
{
	uint8_t status = 1u;

	if (!ready || mfd_npm13xx_reg_read(mfd, SHIP_BASE, SHIP_SHPHLDSTATUS, &status) != 0) {
		return false;
	}
	return (status & 1u) == 0u;
}

static float fl(const struct sensor_value *v)
{
	return (float)v->val1 + (float)v->val2 / 1000000.0f;
}

int pmic_measure(float *v, float *i, float *t, int32_t *status, int32_t *error)
{
	struct sensor_value x;
	int rc;

	if (!ready) {
		return -ENODEV;
	}
	k_mutex_lock(&lock, K_FOREVER);
	rc = sensor_sample_fetch(charger);
	if (rc == 0) {
		sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &x);
		*v = fl(&x);
		sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &x);
		*i = fl(&x);
		sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &x);
		*t = fl(&x);
		sensor_channel_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM13XX_CHARGER_STATUS, &x);
		*status = x.val1;
		sensor_channel_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM13XX_CHARGER_ERROR, &x);
		*error = x.val1;
	}
	k_mutex_unlock(&lock);
	return rc;
}

enum pmic_charge pmic_charge(void)
{
	float v, i, t;
	int32_t status, error;

	if (!pmic_vbus()) {
		return PMIC_OFF_CABLE;
	}
	if (pmic_measure(&v, &i, &t, &status, &error) != 0) {
		return PMIC_CHARGED;
	}
	if (error != 0) {
		return PMIC_CHARGE_FAULT;
	}
	return (status & CHG_RUNNING) ? PMIC_CHARGING : PMIC_CHARGED;
}

int pmic_light_to_charger(bool to_charger)
{
	int rc;

	if (!ready) {
		return -ENODEV;
	}
	rc = mfd_npm13xx_reg_write(mfd, LED_BASE, LED_MODE + PMIC_LED_RED,
				   to_charger ? LED_MODE_ERROR : LED_MODE_HOST);
	rc |= mfd_npm13xx_reg_write(mfd, LED_BASE, LED_MODE + PMIC_LED_GREEN,
				    to_charger ? LED_MODE_CHARGING : LED_MODE_HOST);
	return rc;
}

int pmic_led(unsigned led, bool on)
{
	if (!ready || led > PMIC_LED_GREEN) {
		return -ENODEV;
	}
	return mfd_npm13xx_reg_write(mfd, LED_BASE, (on ? LED_SET : LED_CLR) + 2u * led, 1u);
}

int pmic_ship(void)
{
	LOG_INF("into ship mode");
	log_panic();
	if (!ready) {
		return -ENODEV;
	}
	/* taken, the cell is parted from the board and nothing runs on after this */
	return regulator_parent_ship_mode(regulators);
}

int pmic_buck2_pwm(bool pwm)
{
	if (!ready) {
		return -ENODEV;
	}
	return mfd_npm13xx_reg_write(mfd, BUCK_BASE, pwm ? BUCK2_PWMSET : BUCK2_PWMCLR, 1u);
}
