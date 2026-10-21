/* NIC-Strelka — the network's name and password over Bluetooth LE, in Povodok's first minutes
 * after it is switched on (FIRMWARE.md, The platform; PHONE.md, The link): one service, the name
 * and the password written, a word to join, the outcome read; the writes under encryption, the
 * link paired first. Then Bluetooth LE goes off, before ESB and the frame take the radio. Built
 * against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_BLE_PROV_H
#define STRELKA_BLE_PROV_H

#include <stdint.h>

#define BLE_PROV_MINUTES 3                /* advertising this long after the start */

int ble_prov_init(void);                  /* advertising as "Strelka Povodok", off after the minutes */
void ble_prov_stop(void);                 /* off now: the radio is the set's */

#endif
