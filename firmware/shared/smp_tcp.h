/* NIC-Strelka — SMP over TCP on Povodok (COMMANDS.md, A Belka through Povodok: a port each):
 * port 1337 Povodok's own, 1337 + n the Belka numbered n while it is in the set; a frame from a
 * Belka's port goes to that Belka over ESB through the link and its answer comes back the same
 * way; every connection begins with the lock, 64/16, and until it is passed every other frame
 * answers 11, access denied. Povodok's own frames go to MCUmgr as a transport of its own, the
 * answers back on the connection they came by. Built against nRF Connect SDK v3.4.0, not run. */
#ifndef STRELKA_SMP_TCP_H
#define STRELKA_SMP_TCP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

/* the way to a Belka: the frame handed to the link for the Belka numbered n, with a token that
 * names the connection it came by and comes back with the answer; 0 taken, else the port
 * answers group 64's own code 1, the bridged target out of reach. Set by the link when it is
 * written; none set, every Belka is out of reach. An answer not back within SMP_TCP_BRIDGE_MS
 * — a Belka asleep is woken in its state's period first (FIRMWARE.md, The power) — is answered
 * with 4, a timeout, and the connection reads on; the answer coming later is dropped. */
#define SMP_TCP_BRIDGE_MS 3000
typedef int (*smp_tcp_bridge_fn)(uint8_t belka, uint32_t token, const uint8_t *frame,
				 size_t len);

int smp_tcp_init(void);                            /* Povodok's port and the thread */
/* out of the service mode: a Belka's setting commands — 64/3's write, 64/5, 64/7, 64/8, 64/9 —
 * answered 6 here, never bridged (FIRMWARE.md, The service mode) */
void smp_tcp_service(bool on);
/* a connection has passed the lock since the start: a phone joined, on the network or the lead */
bool smp_tcp_joined(void);
void smp_tcp_set_bridge(smp_tcp_bridge_fn fn);
/* the Belkas' ports: open for the numbers taken, closed for the rest */
void smp_tcp_belka_ports(const bool taken[STRELKA_BELKAS_MAX + 1]);
/* a Belka's answer, come back over the link: to the connection the token names, if it still
 * stands; from the link's thread, never waiting on the socket */
void smp_tcp_from_belka(uint8_t belka, uint32_t token, const uint8_t *frame, size_t len);
/* the phone: the address the last connection on Povodok's own port that passed the lock came
 * from, while it stands; false when none does (PHONE.md, The live view) */
struct net_sockaddr_in;
bool smp_tcp_phone_address(struct net_sockaddr_in *addr);

#endif
