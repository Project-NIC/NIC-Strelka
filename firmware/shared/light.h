/* NIC-Strelka — the two-coloured light, red and green, both for orange, each sunk by one of the
 * PMIC's LED drivers through 1,5 kΩ, some 1 mA: red LED0, green LED1 (FIRMWARE.md, The magnets and
 * the light; The PMIC; BOARD.md, D3). In a set a blink of 50 ms every 10 s, measuring two of 50 ms
 * every 5 s, sparing the cell; fast 100 ms in 200 ms, a blink 500 ms in 1 s. The charge's own
 * light is the PMIC's: LIGHT_CHARGER gives both drivers back to the charger, green while it
 * charges, red for its fault. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_LIGHT_H
#define STRELKA_LIGHT_H

#include <stdint.h>

enum light {
	LIGHT_OFF,
	LIGHT_GREEN_SLOW,       /* in a set, waiting: a blink every 10 s */
	LIGHT_GREEN_FAST,       /* measuring: two blinks every 5 s */
	LIGHT_GREEN_RED,        /* Povodok */
	LIGHT_RED_BLINK,        /* the cell low, under 20 % */
	LIGHT_CHARGER,          /* on the cable, the PMIC's own: green charging, red its fault */
	LIGHT_RED_STEADY,       /* a fault */
	LIGHT_ORANGE_FAST,      /* sought by the application, blink */
};

enum light_colour {
	COLOUR_DARK = 0,
	COLOUR_GREEN = 1,
	COLOUR_RED = 2,
	COLOUR_ORANGE = 3,
};

int light_init(void);
void light_set(enum light pattern);              /* the standing pattern */
void light_flash(enum light_colour c, uint32_t ms); /* a colour for a while, the pattern after */
void light_show_cell(uint8_t pct);               /* a short hold of A: the blinks by the cell */
void light_off_now(void);                        /* dark at once, on the caller's own thread */

#endif
