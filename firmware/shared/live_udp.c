#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include "live_udp.h"
#include "protocol.h"
#include "smp_tcp.h"

static int sock = -1;

int live_udp_init(void)
{
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	return sock < 0 ? -1 : 0;
}

void live_udp_send(const uint8_t *msg, size_t len)
{
	struct net_sockaddr_in to;

	if (sock < 0 || !smp_tcp_phone_address(&to)) {
		return;
	}
	to.sin_port = net_htons(SMP_PORT);
	/* from the link's thread: never waiting for a buffer; a period's message that finds none
	 * is lost, as PHONE.md has it */
	(void)zsock_sendto(sock, msg, len, ZSOCK_MSG_DONTWAIT, (struct net_sockaddr *)&to,
			   sizeof(to));
}
