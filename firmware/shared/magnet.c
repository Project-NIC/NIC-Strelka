#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "magnet.h"
#include "pmic.h"

/* magnet B: the overlay's zephyr,user node, magnet-b-gpios, U9's output, low when the magnet is
 * there, the processor's pull-up on it (BOARD.md); magnet A: the PMIC's SHPHLD, its events */
static const struct gpio_dt_spec mag_b = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), magnet_b_gpios);
static struct gpio_callback a_cb;
static volatile bool a_down;
static void (*event_cb)(enum magnet_event);
static struct k_work_delayable poll;
static bool a_was, b_was, a_alone, both_armed;
static int64_t a_since, both_since;

#define POLL_MS 20

bool magnet_a_held(void)
{
	return a_down;
}

static void on_pmic(const struct device *dev, struct gpio_callback *cb, uint32_t events)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_PRESS)) {
		a_down = true;
	}
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE)) {
		a_down = false;
	}
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
	if (a && b) {
		a_alone = false;                 /* held together: neither's own hold counts */
		if (!(a_was && b_was)) {
			both_since = now;
		}
	}
	if (a_was && !a) {
		if (a_alone) {
			tell(now - a_since >= MAGNET_OFF_MS ? MAGNET_A_OFF : MAGNET_A_SHORT);
		} else if (b && b_was && both_armed) {
			int64_t held = now - both_since;   /* A let go, B still there */

			if (held >= MAGNET_FACTORY_FROM_MS && held <= MAGNET_FACTORY_TO_MS) {
				tell(MAGNET_FACTORY);
			}
		}
	}
	if (!a && !b) {
		both_armed = true;
	}
	a_was = a;
	b_was = b;
	k_work_reschedule(&poll, K_MSEC(POLL_MS));
}

int magnet_init(void (*cb)(enum magnet_event))
{
	if (!gpio_is_ready_dt(&mag_b)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&mag_b, GPIO_INPUT);
	/* listened to first, then read, so that a release between the two is not lost */
	pmic_listen(&a_cb, on_pmic, BIT(NPM13XX_EVENT_SHIPHOLD_PRESS) |
		    BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE));
	a_down = pmic_magnet_a_held();
	event_cb = cb;
	a_was = magnet_a_held();
	b_was = magnet_b_held();
	a_since = both_since = k_uptime_get();
	/* a magnet held already is the switching-on hold, no gesture: its release tells nothing;
	 * both held already — after the PMIC's own restart, or after the factory settings — count
	 * nothing until both are let go */
	a_alone = !a_was;
	both_armed = !(a_was && b_was);
	k_work_init_delayable(&poll, look);
	k_work_reschedule(&poll, K_MSEC(POLL_MS));
	return 0;
}
