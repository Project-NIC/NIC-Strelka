/* NIC-Strelka — the fuel gauge: Nordic's nRF Fuel Gauge, nrfxlib's library for a secondary cell,
 * fed once a second with the cell's voltage, current and temperature from the PMIC's charger and
 * told the charger's state and the cable's, the way of the SDK's sample npm13xx_fuel_gauge; its
 * state kept by the unit on the way into ship mode and once an hour, and given back at the start
 * (FIRMWARE.md, The PMIC; What a unit keeps). The cell's model is made from the cell itself by
 * Nordic's nPM PowerUP; until then the build carries the SDK sample's, a Li-ion cell's, which
 * reads a LiFePO4 cell wrongly. Built against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_GAUGE_H
#define STRELKA_GAUGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GAUGE_STATE_MAX 1024u    /* room for the library's state, nrf_fuel_gauge_state_size */

/* the state kept by the unit, or NULL for none */
int gauge_init(const uint8_t *state, size_t len);
bool gauge_present(void);          /* the PMIC answered and the library runs */
void gauge_service(void);          /* once a second */
uint8_t gauge_pct(void);           /* the state of charge, 0–100 */
uint16_t gauge_mv(void);           /* the cell's voltage */
int16_t gauge_temp_centi(void);    /* the cell's temperature by its NTC, 0,01 °C */
/* the cell empty: under the unit's end voltage, CONFIG_STRELKA_CELL_EMPTY_MV, for ten seconds
 * running — by its voltage, the gauge's 0 % not trusted until the cell's own model is made */
bool gauge_empty(void);
/* the library's state for the store; its length, 0 when there is none */
size_t gauge_state(uint8_t *buf, size_t size);

#endif
