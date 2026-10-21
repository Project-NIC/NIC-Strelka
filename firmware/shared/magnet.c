#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "magnet.h"

/* the overlay's zephyr,user node, magnet-a-gpios and magnet-b-gpios: the Hall switches' outputs,
 * low when a magnet is there, the processor's pull-up on them (BOARD.md) */
static const struct gpio_dt_spec mag_a = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), magnet_a_gpios);
static const struct gpio_dt_spec mag_b = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), magnet_b_gpios);
static void (*event_cb)(enum magnet_event);
static struct k_work_delayable poll;
static bool a_was, b_was, a_alone, b_alone, both_told;
static int64_t a_since, b_since, both_since;

#define POLL_MS 20

bool magnet_a_held(void)
{
	return gpio_pin_get_dt(&mag_a) > 0;
}

bool magnet_b_held(void)
{
	return gpio_pin_get_dt(&mag_b) > 0;
}

static void tell(enum magnet_event e)
{
	if (event_cb) {
		event_cb(e);
	}
}

static void look(struct k_work *w)
{
	bool a = magnet_a_held(), b = magnet_b_held();
	int64_t now = k_uptime_get();

	ARG_UNUSED(w);
	if (a && !a_was) {
		a_since = now;
		a_alone = true;
	}
	if (b && !b_was) {
		b_since = now;
		b_alone = true;
	}
	if (a && b) {
		a_alone = b_alone = false;       /* held together: neither's own hold counts */
		if (!(a_was && b_was)) {
			both_since = now;
		} else if (!both_told && now - both_since >= 10000) {
			both_told = true;
			tell(MAGNET_BOTH_10S);
		}
	}
	if (!a && !b && both_told) {
		both_told = false;
		tell(MAGNET_FACTORY);
	}
	if (a_was && !a && a_alone) {
		tell(now - a_since >= 6000 ? MAGNET_A_OFF : MAGNET_A_SHORT);
	}
	if (b_was && !b && b_alone) {
		int64_t held = now - b_since;

		if (held >= 3000 && held < 6000) {
			tell(MAGNET_B_START);
		} else if (held >= 6000 && held < 10000) {
			tell(MAGNET_B_STOP);
		}
	}
	a_was = a;
	b_was = b;
	k_work_reschedule(&poll, K_MSEC(POLL_MS));
}

int magnet_init(void (*cb)(enum magnet_event))
{
	if (!gpio_is_ready_dt(&mag_a) || !gpio_is_ready_dt(&mag_b)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&mag_a, GPIO_INPUT);
	gpio_pin_configure_dt(&mag_b, GPIO_INPUT);
	event_cb = cb;
	a_was = magnet_a_held();
	b_was = magnet_b_held();
	a_since = b_since = k_uptime_get();
	/* a magnet held already is the switching-on hold, no gesture: its release tells nothing */
	a_alone = !a_was;
	b_alone = !b_was;
	k_work_init_delayable(&poll, look);
	k_work_reschedule(&poll, K_MSEC(POLL_MS));
	return 0;
}
