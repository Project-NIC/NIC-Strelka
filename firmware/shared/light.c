#include <zephyr/kernel.h>
#include "light.h"
#include "pmic.h"

struct pattern {
	enum light_colour a, b;  /* two phases, the second COLOUR_DARK for a blink */
	uint16_t a_ms, b_ms;     /* 0 b_ms: steady */
	uint8_t blinks;          /* the two phases this many times, then dark for gap_ms; 0 as 1 */
	uint16_t gap_ms;
};

static const struct pattern patterns[] = {
	[LIGHT_OFF] = { COLOUR_DARK, COLOUR_DARK, 1000, 0 },
	[LIGHT_GREEN_SLOW] = { COLOUR_GREEN, COLOUR_DARK, 50, 9950 },
	[LIGHT_GREEN_FAST] = { COLOUR_GREEN, COLOUR_DARK, 50, 200, 2, 4700 },
	[LIGHT_GREEN_RED] = { COLOUR_GREEN, COLOUR_RED, 500, 500 },
	[LIGHT_RED_BLINK] = { COLOUR_RED, COLOUR_DARK, 500, 500 },
	[LIGHT_CHARGER] = { COLOUR_DARK, COLOUR_DARK, 1000, 0 },     /* the PMIC's, not driven */
	[LIGHT_RED_STEADY] = { COLOUR_RED, COLOUR_RED, 1000, 0 },
	[LIGHT_ORANGE_FAST] = { COLOUR_ORANGE, COLOUR_DARK, 100, 100 },
};

static enum light standing;
static bool phase_b;
static uint8_t blink_no;         /* the blinks of the pattern's cycle shown */
static bool charger_has_it;      /* the drivers in the charger's modes, LIGHT_CHARGER */
static K_MUTEX_DEFINE(lock);     /* the drivers' state: the work queue's steps, light_off_now */
static int64_t flash_until;      /* a flash or the cell's blinks run until then */
static struct k_work_delayable work;

/* the drivers to the host, each colour set; written only when it changes, a TWI write each */
static int lit = -1;

static void drive(enum light_colour c)
{
	k_mutex_lock(&lock, K_FOREVER);
	if (charger_has_it) {
		pmic_light_to_charger(false);
		charger_has_it = false;
		lit = -1;
	}
	if ((int)c != lit) {
		pmic_led(PMIC_LED_GREEN, (c & COLOUR_GREEN) != 0);
		pmic_led(PMIC_LED_RED, (c & COLOUR_RED) != 0);
		lit = (int)c;
	}
	k_mutex_unlock(&lock);
}

static void step(struct k_work *w)
{
	const struct pattern *p = &patterns[standing];

	ARG_UNUSED(w);
	if (k_uptime_get() < flash_until) {
		k_work_reschedule(&work, K_TIMEOUT_ABS_MS(flash_until));
		return;
	}
	if (standing == LIGHT_CHARGER) {
		k_mutex_lock(&lock, K_FOREVER);
		if (!charger_has_it) {
			pmic_light_to_charger(true);
			charger_has_it = true;
		}
		k_mutex_unlock(&lock);
		return;              /* the PMIC lights it; nothing to step */
	}
	phase_b = !phase_b && p->b_ms;
	drive(phase_b ? p->b : p->a);
	if (!phase_b) {
		k_work_reschedule(&work, K_MSEC(p->a_ms));
	} else if (++blink_no < p->blinks) {
		k_work_reschedule(&work, K_MSEC(p->b_ms));
	} else {
		blink_no = 0;            /* the cycle's last: the gap, or the second phase alone */
		k_work_reschedule(&work, K_MSEC(p->gap_ms ? p->gap_ms : p->b_ms));
	}
}

int light_init(void)
{
	/* the charger has the light until a pattern takes it: its modes written, since a soft reset
	 * leaves the PMIC's as the last run set them; with no PMIC answering — the development kit
	 * alone — the patterns run on, writing nothing */
	charger_has_it = pmic_present() && pmic_light_to_charger(true) == 0;
	k_work_init_delayable(&work, step);
	light_set(LIGHT_CHARGER);
	return pmic_present() ? 0 : -ENODEV;
}

void light_set(enum light pattern)
{
	if (pattern == standing && pattern == LIGHT_CHARGER && charger_has_it) {
		return;
	}
	standing = pattern;
	phase_b = true;              /* step() turns it to the first phase */
	blink_no = 0;
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

static bool cell_init;

void light_show_cell(uint8_t pct)
{
	if (!cell_init) {
		k_work_init_delayable(&cell_work, cell_step);
		cell_init = true;
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

void light_off_now(void)
{
	k_work_cancel_delayable(&work);
	if (cell_init) {
		k_work_cancel_delayable(&cell_work);
	}
	standing = LIGHT_OFF;
	flash_until = 0;
	k_mutex_lock(&lock, K_FOREVER);
	lit = -1;                /* written whatever the cache says */
	k_mutex_unlock(&lock);
	drive(COLOUR_DARK);
}
