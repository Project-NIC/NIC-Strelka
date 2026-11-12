/* NIC-Strelka — the unit's power: how it started and how it goes off (FIRMWARE.md, The PMIC).
 * Ship mode is the off; magnet A held 96 ms or the cable takes the PMIC out of it and the board
 * starts as from power-on. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_POWER_H
#define STRELKA_POWER_H

#include <stdbool.h>
#include <stdint.h>

/* At the start, after pmic_init, gauge_init and light_init, before anything else runs. Returns
 * once the unit is switched on: magnet A held; the cable, on Povodok (`cable_on`); or a restart
 * the unit did not ask to be off by — a soft reset, a watchdog's, a lockup. A Belka or Kryska
 * woken by the cable alone, or any unit switched off on it, idles here on the cable's power, the
 * light the PMIC's, the gauge read every second, until magnet A switches it on; the cable gone,
 * or no reason to run, it goes into ship mode and does not return. With no PMIC answering, the
 * development kit's case, it returns at once. */
void power_start(bool cable_on);

/* the unit restarted on purpose, staying on; after the factory settings, the magnets still held
 * count nothing until they are let go */
FUNC_NORETURN void power_restart(bool factory);
bool power_after_factory(void);

/* when magnet A was put on, as near as the unit knows: the moment it was seen in the idle, or
 * the start less the boot's time before the application ran — some 0,3 s, 3 s more on a cable,
 * where MCUboot waits for a command */
int64_t power_a_since(void);

/* main has its watchdog installed: the feeding of one left running from before a reset stops */
void power_watchdog_taken(void);

/* the gauge's state kept now; the unit's loop calls it once an hour */
void power_keep(void);

/* Off, the record closed by the caller first: the gauge's state kept; off the cable into ship
 * mode, on it a restart into power_start's idle, so that the charge goes on and the gauge with it.
 * Does not return. */
FUNC_NORETURN void power_off(void);

#endif
