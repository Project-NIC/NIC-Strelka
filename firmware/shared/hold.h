/* NIC-Strelka — HOLD, P1.23: high, it keeps the board on through Q1; let go, the board switches
 * off once magnet A is away too (FIRMWARE.md, The charger and the supply, The hold). */
#ifndef STRELKA_HOLD_H
#define STRELKA_HOLD_H

int hold_init(void);         /* the pin high, as early as the application runs; MCUboot's first
			      * instructions set it before, the kit's work */
void hold_release(void);     /* the board off: the record closed by the caller first */

#endif
