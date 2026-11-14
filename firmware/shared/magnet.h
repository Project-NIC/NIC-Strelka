/* NIC-Strelka — the two magnets read while the unit runs (FIRMWARE.md, The magnets and the
 * light): a hold acts when it is let go, by how long it was held; while both are held neither's
 * own hold counts. Magnet B alone does nothing; both put on and magnet A taken away after 6 to
 * 8 s, B still there, are the factory settings — A let go sooner or later, or B first, nothing;
 * both held at the unit's start count nothing until both are let go, so that the PMIC's own
 * restart at 10 s, the magnets still there, asks nothing. Magnet A is the PMIC's SHPHLD, read
 * through its events on PMIC_INT; its Hall switch also takes the PMIC out of ship mode. Magnet B
 * is a pin of the processor's. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_MAGNET_H
#define STRELKA_MAGNET_H

#include <stdbool.h>
#include <stdint.h>

enum magnet_event {
	MAGNET_A_SHORT,          /* A let go under 6 s: the cell's state shown */
	MAGNET_A_OFF,            /* A held 6 s and let go: off */
	MAGNET_FACTORY,          /* both held, A let go after 6 to 8 s, B still there: the light
				  * orange, fast, 5 s, then the factory settings */
};

#define MAGNET_OFF_MS          6000
#define MAGNET_FACTORY_FROM_MS 6000
#define MAGNET_FACTORY_TO_MS   8000

int magnet_init(void (*cb)(enum magnet_event));
bool magnet_a_held(void);
bool magnet_b_held(void);

#endif
