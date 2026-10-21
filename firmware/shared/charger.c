#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "charger.h"

static const struct gpio_dt_spec stat = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), chg_stat_gpios);
static const struct gpio_dt_spec pg = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), chg_pg_gpios);
static uint16_t stat_history;      /* the last sixteen samples of STAT, active = charging */
static unsigned samples;

int charger_init(void)
{
	int rc = gpio_pin_configure_dt(&stat, GPIO_INPUT);

	rc |= gpio_pin_configure_dt(&pg, GPIO_INPUT);
	return rc;
}

void charger_service(void)
{
	stat_history = (uint16_t)((stat_history << 1) | (gpio_pin_get_dt(&stat) > 0 ? 1u : 0u));
	if (samples < 16) {
		samples++;
	}
}

bool charger_powered(void)
{
	return gpio_pin_get_dt(&pg) > 0;
}

enum charger_state charger_state(void)
{
	unsigned changes = 0;

	if (!charger_powered()) {
		return CHARGER_OFF_CABLE;
	}
	if (samples < 16) {
		return (stat_history & 1u) ? CHARGER_CHARGING : CHARGER_DONE;
	}
	/* a blink at 1 Hz changes the level twice in the last 1,6 s; a steady level never */
	for (unsigned i = 1; i < 16; i++) {
		changes += ((stat_history >> i) ^ (stat_history >> (i - 1))) & 1u;
	}
	if (changes >= 2) {
		return CHARGER_FAULT;
	}
	return (stat_history & 1u) ? CHARGER_CHARGING : CHARGER_DONE;
}
