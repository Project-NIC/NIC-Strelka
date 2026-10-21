#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "light.h"

/* the pins: the overlay's zephyr,user node, led-green-gpios and led-red-gpios, active low, sinking
 * the cathodes */
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), led_green_gpios);
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), led_red_gpios);

struct pattern {
	enum light_colour a, b;  /* two phases, the second COLOUR_DARK for a blink */
	uint16_t a_ms, b_ms;     /* 0 b_ms: steady */
};

static const struct pattern patterns[] = {
	[LIGHT_OFF] = { COLOUR_DARK, COLOUR_DARK, 1000, 0 },
	[LIGHT_GREEN_SLOW] = { COLOUR_GREEN, COLOUR_DARK, 100, 1900 },
	[LIGHT_GREEN_FAST] = { COLOUR_GREEN, COLOUR_DARK, 100, 100 },
	[LIGHT_GREEN_RED] = { COLOUR_GREEN, COLOUR_RED, 500, 500 },
	[LIGHT_RED_BLINK] = { COLOUR_RED, COLOUR_DARK, 500, 500 },
	[LIGHT_GREEN_BLINK] = { COLOUR_GREEN, COLOUR_DARK, 500, 500 },
	[LIGHT_GREEN_STEADY] = { COLOUR_GREEN, COLOUR_GREEN, 1000, 0 },
	[LIGHT_ORANGE_STEADY] = { COLOUR_ORANGE, COLOUR_ORANGE, 1000, 0 },
	[LIGHT_RED_STEADY] = { COLOUR_RED, COLOUR_RED, 1000, 0 },
	[LIGHT_ORANGE_FAST] = { COLOUR_ORANGE, COLOUR_DARK, 100, 100 },
};

static enum light standing;
static bool phase_b;
static int64_t flash_until;      /* a flash or the cell's blinks run until then */
static struct k_work_delayable work;

static void drive(enum light_colour c)
{
	gpio_pin_set_dt(&green, (c & COLOUR_GREEN) != 0);
	gpio_pin_set_dt(&red, (c & COLOUR_RED) != 0);
}

static void step(struct k_work *w)
{
	const struct pattern *p = &patterns[standing];

	ARG_UNUSED(w);
	if (k_uptime_get() < flash_until) {
		k_work_reschedule(&work, K_TIMEOUT_ABS_MS(flash_until));
		return;
	}
	phase_b = !phase_b && p->b_ms;
	drive(phase_b ? p->b : p->a);
	k_work_reschedule(&work, K_MSEC(phase_b ? p->b_ms : p->a_ms));
}

int light_init(void)
{
	if (!gpio_is_ready_dt(&green) || !gpio_is_ready_dt(&red)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE);
	k_work_init_delayable(&work, step);
	light_set(LIGHT_OFF);
	return 0;
}

void light_set(enum light pattern)
{
	standing = pattern;
	phase_b = true;              /* step() turns it to the first phase */
	k_work_reschedule(&work, K_NO_WAIT);
}

void light_flash(enum light_colour c, uint32_t ms)
{
	flash_until = k_uptime_get() + ms;
	drive(c);
	k_work_reschedule(&work, K_MSEC(ms));
}

/* the blinks by the cell: 76–100 % green four, 51–75 orange three, 26–50 orange two, 11–25
 * red one, 10 and less red fast for 2 s (FIRMWARE.md, A short hold of A shows) */
static struct k_work_delayable cell_work;
static enum light_colour cell_colour;
static int cell_blinks;
static uint16_t cell_on_ms, cell_off_ms;
static bool cell_lit;

static void cell_step(struct k_work *w)
{
	ARG_UNUSED(w);
	if (cell_lit) {
		drive(COLOUR_DARK);
		cell_lit = false;
		cell_blinks--;
		if (cell_blinks > 0) {
			k_work_reschedule(&cell_work, K_MSEC(cell_off_ms));
		}
	} else if (cell_blinks > 0) {
		drive(cell_colour);
		cell_lit = true;
		k_work_reschedule(&cell_work, K_MSEC(cell_on_ms));
	}
}

void light_show_cell(uint8_t pct)
{
	static bool init;

	if (!init) {
		k_work_init_delayable(&cell_work, cell_step);
		init = true;
	}
	cell_on_ms = 150;
	cell_off_ms = 250;
	if (pct <= 10) {
		cell_colour = COLOUR_RED;
		cell_blinks = 10;            /* fast: ten in 2 s */
		cell_on_ms = cell_off_ms = 100;
	} else if (pct <= 25) {
		cell_colour = COLOUR_RED;
		cell_blinks = 1;
	} else if (pct <= 50) {
		cell_colour = COLOUR_ORANGE;
		cell_blinks = 2;
	} else if (pct <= 75) {
		cell_colour = COLOUR_ORANGE;
		cell_blinks = 3;
	} else {
		cell_colour = COLOUR_GREEN;
		cell_blinks = 4;
	}
	flash_until = k_uptime_get() + cell_blinks * (cell_on_ms + cell_off_ms) + 200;
	drive(COLOUR_DARK);
	k_work_reschedule(&work, K_TIMEOUT_ABS_MS(flash_until));
	k_work_reschedule(&cell_work, K_MSEC(200));
}
