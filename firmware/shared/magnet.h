/* NIC-Strelka — the two magnets read while the unit runs (FIRMWARE.md, The magnets and the
 * light): a hold acts when it is let go, by how long it was held; while both are held neither's
 * own hold counts, and both held 8 s are the factory settings, made at once, before the PMIC's
 * own restart at 10 s. Magnet A is the PMIC's SHPHLD, read through its events on PMIC_INT; its
 * Hall switch also takes the PMIC out of ship mode. Magnet B is a pin of the processor's. Built
 * against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_MAGNET_H
#define STRELKA_MAGNET_H

#include <stdbool.h>
#include <stdint.h>

enum magnet_event {
	MAGNET_A_SHORT,          /* A let go under 6 s: the cell's state shown */
	MAGNET_A_OFF,            /* A held 6 s and let go: off */
	MAGNET_B_START,          /* B held 3–6 s: the set starts measuring (Povodok); a Belka's
				  * disk opened for writing on its cable */
	MAGNET_B_STOP,           /* B held 6–10 s: the set stops (Povodok) */
	MAGNET_BOTH_8S,          /* both held 8 s: the light orange */
	MAGNET_FACTORY,          /* right after it: the factory settings */
};

int magnet_init(void (*cb)(enum magnet_event));
bool magnet_a_held(void);
bool magnet_b_held(void);

#endif
