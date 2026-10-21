/* NIC-Strelka — the two magnets read while the unit runs (FIRMWARE.md, The magnets and the
 * light): a hold acts when it is let go, by how long it was held; while both are held neither's
 * own hold counts, and both held 10 s are the factory settings. Magnet A's Hall switch also
 * starts the board in hardware; whether its first hold reached 1 s is main's to judge from the
 * time since the start. */
#ifndef STRELKA_MAGNET_H
#define STRELKA_MAGNET_H

#include <stdbool.h>
#include <stdint.h>

enum magnet_event {
	MAGNET_A_SHORT,          /* A let go under 6 s: the cell's state shown */
	MAGNET_A_OFF,            /* A held 6 s and let go: off */
	MAGNET_B_START,          /* B held 3–6 s: the set starts measuring (Povodok) */
	MAGNET_B_STOP,           /* B held 6–10 s: the set stops (Povodok) */
	MAGNET_BOTH_10S,         /* both held 10 s: the light orange, the factory settings when
				  * let go */
	MAGNET_FACTORY,          /* both let go after that */
};

int magnet_init(void (*cb)(enum magnet_event));
bool magnet_a_held(void);
bool magnet_b_held(void);

#endif
