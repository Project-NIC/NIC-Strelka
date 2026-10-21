/* NIC-Strelka — the phone's way to Povodok (PHONE.md, The link): on the lead, Povodok is the
 * USB network adapter with its own address, 192.168.7.1, and gives the phone its by DHCP; on
 * Wi-Fi it is a client of the access point whose name and password Bluetooth LE brought
 * (ble_prov.c), kept in the settings, at 5 GHz alone, its address from the network's DHCP. Built
 * against nRF Connect SDK v3.4.0, not run on hardware: the phones decide it (bench.md). */
#ifndef STRELKA_PHONE_NET_H
#define STRELKA_PHONE_NET_H

#include <stdbool.h>
#include <stddef.h>

int phone_net_init(void);                 /* the lead's network, and the Wi-Fi's stored network */
/* the network's name and password from the application: kept, and joined */
int phone_net_wifi_set(const char *ssid, size_t ssid_len, const char *password, size_t pw_len);
bool phone_net_wifi_connected(void);
void phone_net_forget(void);              /* the network forgotten, with the factory settings */

#endif
