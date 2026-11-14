/* NIC-Strelka — the network's name and password over Bluetooth LE, in Povodok's service mode,
 * from the start until a phone has joined Povodok, on the network or the lead, or the
 * application's 64/20 (FIRMWARE.md, The service mode; PHONE.md, The link): one service, the name
 * and the password written, a word to join, the outcome read; the writes under encryption, the
 * link paired first. Then Bluetooth LE goes off until the unit is switched off and on, and the
 * radio is the set's, MPSL uninitialised, for ESB. Built against nRF Connect SDK v3.4.0, not run
 * on hardware. */
#ifndef STRELKA_BLE_PROV_H
#define STRELKA_BLE_PROV_H

#include <stdint.h>


int ble_prov_init(void);                  /* advertising as "Strelka Povodok", the service mode */
void ble_prov_stop(void);                 /* off now: the radio is the set's */
void ble_prov_stop_soon(void);            /* 64/20's: the stop asked, from any thread */
void ble_prov_poll(void);                 /* from main's loop: the stop asked done there */

#endif
