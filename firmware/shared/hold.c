#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "hold.h"

static const struct gpio_dt_spec hold = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), hold_gpios);

int hold_init(void)
{
	if (!gpio_is_ready_dt(&hold)) {
		return -ENODEV;
	}
	return gpio_pin_configure_dt(&hold, GPIO_OUTPUT_ACTIVE);
}

void hold_release(void)
{
	gpio_pin_set_dt(&hold, 0);
}
