/* NIC-Strelka — the two-coloured light, red and green, both for orange, each sunk by a processor
 * pin (FIRMWARE.md, The magnets and the light). The pages name the patterns; the blinks' lengths
 * are the firmware's: slow 100 ms in 2 s, fast 100 ms in 200 ms, a blink 500 ms in 1 s. */
#ifndef STRELKA_LIGHT_H
#define STRELKA_LIGHT_H

#include <stdint.h>

enum light {
	LIGHT_OFF,
	LIGHT_GREEN_SLOW,       /* in a set, waiting */
	LIGHT_GREEN_FAST,       /* measuring */
	LIGHT_GREEN_RED,        /* Povodok */
	LIGHT_RED_BLINK,        /* the cell low, under 20 % */
	LIGHT_GREEN_BLINK,      /* charging, the cell at 20 % or more */
	LIGHT_GREEN_STEADY,     /* charged */
	LIGHT_ORANGE_STEADY,    /* the charger's fault */
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

#endif
