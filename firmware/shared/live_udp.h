/* NIC-Strelka — the live view over UDP on Povodok (PHONE.md, The live view): the first Povodok's
 * gathering of each period, in one message or two, to the phone — the address the application's
 * connection on port 1337 came from, UDP port 1337 there, while that connection stands. Built
 * against nRF Connect SDK v3.4.0, not run on hardware. */
#ifndef STRELKA_LIVE_UDP_H
#define STRELKA_LIVE_UDP_H

#include <stddef.h>
#include <stdint.h>

int live_udp_init(void);
/* a message to the phone; nothing sent when no connection of the application's stands */
void live_udp_send(const uint8_t *msg, size_t len);

#endif
