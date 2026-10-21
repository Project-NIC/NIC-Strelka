/* NIC-Strelka — the fuel gauge, ADI's MAX17055 on TWIM30, set the way of ADI's Software
 * Implementation Guide (UG6365, rev. 2, the EZ configuration) with the registers FIRMWARE.md lists
 * for the LiFePO4 cell, and only after it has lost its settings — the cell joined — which its POR
 * bit says; what it learns of the cell kept across power-ons and written back after a POR
 * (FIRMWARE.md, The charger and the supply). Built against nRF Connect SDK v3.4.0, not run on
 * hardware: the kit decides it, and the cell's model is characterised on the bench. */
#ifndef STRELKA_GAUGE_H
#define STRELKA_GAUGE_H

#include <stdbool.h>
#include <stdint.h>

#define GAUGE_LEARNED_BYTES 10        /* RCOMP0, TempCo, FullCapRep, Cycles, FullCapNom */

struct gauge_learned {
	uint16_t rcomp0, tempco, fullcaprep, cycles, fullcapnom;
};

/* the learned values kept by the unit, or NULL for none: after a POR they are written back */
int gauge_init(const struct gauge_learned *kept);
bool gauge_present(void);          /* it answered on the bus */
/* once a second: the readings refreshed; true when the learned values changed and are to be kept
 * (bit 6 of Cycles turned), `out` filled */
bool gauge_service(struct gauge_learned *out);
uint8_t gauge_pct(void);           /* RepSOC, 0–100 */
uint16_t gauge_mv(void);           /* VCell */
int16_t gauge_temp_centi(void);    /* its own temperature, 0,01 °C */
bool gauge_empty(void);            /* RepSOC 0: the record to close, the board to go off */

#endif
