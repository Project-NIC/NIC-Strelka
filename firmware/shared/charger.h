/* NIC-Strelka — the charger, TI's BQ25170, read on two pins (SLUSDJ8A, the pin table; FIRMWARE.md,
 * The charger and the supply): STAT low while it charges, high when the charge is done or not
 * running, blinking at 1 Hz on a fault; /PG low while the cable gives it power. Built against nRF
 * Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_CHARGER_H
#define STRELKA_CHARGER_H

#include <stdbool.h>
#include <stdint.h>

enum charger_state {
	CHARGER_OFF_CABLE,      /* no power on the cable */
	CHARGER_CHARGING,
	CHARGER_DONE,           /* on the cable, the charge done or not running */
	CHARGER_FAULT,          /* STAT blinking: the cell too cold or too hot, a timeout */
};

int charger_init(void);
/* sampled ten times a second, to tell the blink from a steady level */
void charger_service(void);
enum charger_state charger_state(void);
bool charger_powered(void);        /* /PG: the cable gives power */

#endif
