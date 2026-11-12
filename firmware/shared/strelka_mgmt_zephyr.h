/* NIC-Strelka — Strelka's group 64 registered with MCUmgr on a unit: each command's handlers
 * call common/strelka_mgmt.c with the unit's ops. The lock a request's connection carries comes
 * from the transport (smp_tcp.c on Povodok); where there is none, a Belka reached through the
 * set's signed fragments alone, the lock is NULL and 64/16 answers unsupported. */
#ifndef STRELKA_MGMT_ZEPHYR_H
#define STRELKA_MGMT_ZEPHYR_H

#include <zephyr/net_buf.h>
#include "strelka_mgmt.h"

void strelka_mgmt_zephyr_init(const struct strelka_ops *ops, void *ctx);
/* the transport's: the lock of the connection the request's buffer came by; weak, NULL by default */
struct smp_lock *strelka_mgmt_lock(struct net_buf *nb);

#endif
