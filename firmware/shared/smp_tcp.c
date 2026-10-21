/* NIC-Strelka — SMP over TCP on Povodok: a transport of MCUmgr's for Povodok's own port, the
 * Belkas' ports bridged to the link, the lock on every connection (smp_tcp.h). One thread polls
 * the listening sockets and the connections; a whole frame cut from a stream goes to MCUmgr in a
 * net_buf whose user data names the connection and its generation, and MCUmgr's answer comes back
 * through `output` to that connection — one request in flight a connection, the stream read no
 * further until the answer has gone, so that a frame pipelined behind the lock's own is judged
 * after it. The connections' table and every send are under one mutex, since the TCP thread,
 * MCUmgr's work queue and the link's thread all reach them. Built against nRF Connect SDK v3.4.0,
 * not run on hardware. */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <mgmt/mcumgr/transport/smp_internal.h>   /* smp_rx_req, as the UDP transport takes it */
#include <zephyr/logging/log.h>
#include "smp.h"
#include "smp_tcp.h"
#include "strelka_mgmt.h"
#include "strelka_mgmt_zephyr.h"

LOG_MODULE_REGISTER(smp_tcp, LOG_LEVEL_INF);

#define CONNS      8                      /* connections at once, every port together */
#define LISTENERS  (1 + STRELKA_BELKAS_MAX)
#define BACKLOG    2

struct conn {
	int sock;                         /* -1 free */
	uint8_t gen;                      /* one up each time the slot is taken: a late answer to
					   * a connection gone finds another generation */
	uint8_t belka;                    /* 0 Povodok's own port, else the Belka's number */
	bool in_flight;                   /* a request handed on, its answer not yet gone */
	struct net_sockaddr_in peer;      /* where it came from */
	uint32_t opened;                  /* the count when it passed the lock; 0 not yet */
	struct smp_stream stream;
	struct smp_lock lock;
};

struct tag {                              /* a net_buf's user data: 2 of the 4 B */
	uint8_t idx, gen;
};
BUILD_ASSERT(sizeof(struct tag) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "the connection's tag fits the net_buf's user data");

static struct conn conns[CONNS];
static int listeners[LISTENERS];          /* [0] Povodok's, [n] the Belka's; -1 closed */
static bool wanted[LISTENERS];
static struct k_mutex lock;               /* conns[], the listeners' wants, every send */
static uint32_t opened_count;             /* connections that passed the lock, so far */
static struct smp_transport smpt;
static smp_tcp_bridge_fn bridge;
static K_THREAD_STACK_DEFINE(stack, 4096);
static struct k_thread thread;

/* under the mutex */
static void send_all(int sock, const uint8_t *data, size_t len)
{
	while (len) {
		ssize_t n = zsock_send(sock, data, len, 0);

		if (n <= 0) {
			return;
		}
		data += n;
		len -= (size_t)n;
	}
}

/* under the mutex: the lock passed on a connection, noted when its answer goes out */
static void note_opened(struct conn *c)
{
	if (c->lock.open && c->opened == 0) {
		c->opened = ++opened_count;
	}
}

/* under the mutex */
static void close_conn(struct conn *c)
{
	if (c->sock >= 0) {
		zsock_close(c->sock);
		c->sock = -1;
		c->in_flight = false;
	}
}

/* MCUmgr's answer: to the connection the request came by, if it is still that connection */
static int output(struct net_buf *nb)
{
	struct tag t;

	memcpy(&t, net_buf_user_data(nb), sizeof(t));
	k_mutex_lock(&lock, K_FOREVER);
	if (t.idx < CONNS && conns[t.idx].sock >= 0 && conns[t.idx].gen == t.gen) {
		send_all(conns[t.idx].sock, nb->data, nb->len);
		conns[t.idx].in_flight = false;
		note_opened(&conns[t.idx]);
	}
	k_mutex_unlock(&lock);
	smp_packet_free(nb);
	return 0;
}

static uint16_t get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);
	return CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE;
}

static int ud_copy(struct net_buf *dst, const struct net_buf *src)
{
	memcpy(net_buf_user_data(dst), net_buf_user_data(src), sizeof(struct tag));
	return 0;
}

/* the lock of the connection a request came by, for group 64's handlers on the work queue */
struct smp_lock *strelka_mgmt_lock(struct net_buf *nb)
{
	struct tag t;

	memcpy(&t, net_buf_user_data(nb), sizeof(t));
	return t.idx < CONNS && conns[t.idx].gen == t.gen ? &conns[t.idx].lock : NULL;
}

/* a whole frame from a connection, the TCP thread's, under the mutex: the version, the lock
 * first; Povodok's own to MCUmgr, a Belka's to the link — the lock's own command answered here
 * for both. False: one in flight now, the stream read no further until it is answered */
static bool on_frame(const uint8_t *frame, size_t len, void *ctx)
{
	struct conn *c = ctx;
	struct smp_head h;
	struct net_buf *nb;
	struct tag t = { .idx = (uint8_t)(c - conns), .gen = c->gen };
	uint8_t out[32];
	size_t n;

	smp_head_read(frame, &h);
	if (h.op != SMP_OP_READ && h.op != SMP_OP_WRITE) {
		return true;
	}
	if (h.version != SMP_VERSION) {
		n = smp_error_frame(out, sizeof(out), &h,
				    h.version < SMP_VERSION ? SMP_ERR_TOO_OLD : SMP_ERR_TOO_NEW);
		send_all(c->sock, out, n);
		return true;
	}
	if (!smp_lock_passes(&c->lock, &h)) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_ACCESS);
		send_all(c->sock, out, n);
		return true;
	}
	if (c->belka && !(h.group == SMP_GROUP_STRELKA && h.id == CMD_AUTH)) {
		if (bridge == NULL || bridge(c->belka, frame, len) != 0) {
			n = smp_group_error_frame(out, sizeof(out), &h, SMP_GROUP_STRELKA,
						  STRELKA_ERR_UNREACHABLE);
			send_all(c->sock, out, n);
			return true;
		}
		c->in_flight = true;             /* the answer comes by smp_tcp_from_belka */
		return false;
	}
	nb = smp_packet_alloc();
	if (nb == NULL) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_NOMEM);
		send_all(c->sock, out, n);
		return true;
	}
	net_buf_add_mem(nb, frame, len);
	memcpy(net_buf_user_data(nb), &t, sizeof(t));
	c->in_flight = true;
	smp_rx_req(&smpt, nb);
	return false;
}

static int open_listener(uint16_t port)
{
	struct net_sockaddr_in addr;
	int sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

	if (sock < 0) {
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = NET_AF_INET;
	addr.sin_port = net_htons(port);
	if (zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)) < 0 ||
	    zsock_listen(sock, BACKLOG) < 0) {
		zsock_close(sock);
		return -1;
	}
	return sock;
}

/* the listeners as the set stands: Povodok's always, a Belka's while it is taken */
static void sync_listeners(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	for (unsigned i = 0; i < LISTENERS; i++) {
		if (wanted[i] && listeners[i] < 0) {
			listeners[i] = open_listener((uint16_t)(SMP_PORT + i));
			if (listeners[i] >= 0) {
				LOG_INF("port %u open", SMP_PORT + i);
			}
		} else if (!wanted[i] && listeners[i] >= 0) {
			zsock_close(listeners[i]);
			listeners[i] = -1;
			for (unsigned k = 0; k < CONNS; k++) {
				if (conns[k].sock >= 0 && conns[k].belka == i) {
					close_conn(&conns[k]);
				}
			}
		}
	}
	k_mutex_unlock(&lock);
}

static void accept_on(unsigned i)
{
	struct net_sockaddr_in peer;
	net_socklen_t peer_len = sizeof(peer);
	int sock = zsock_accept(listeners[i], (struct net_sockaddr *)&peer, &peer_len);

	if (sock < 0) {
		return;
	}
	k_mutex_lock(&lock, K_FOREVER);
	for (unsigned k = 0; k < CONNS; k++) {
		if (conns[k].sock < 0) {
			conns[k].sock = sock;
			conns[k].gen++;
			conns[k].belka = (uint8_t)i;
			conns[k].in_flight = false;
			conns[k].peer = peer;
			conns[k].opened = 0;
			smp_stream_init(&conns[k].stream);
			smp_lock_init(&conns[k].lock);
			k_mutex_unlock(&lock);
			return;
		}
	}
	k_mutex_unlock(&lock);
	zsock_close(sock);                       /* no room: refused */
}

static void thread_fn(void *a, void *b, void *c)
{
	static uint8_t buf[SMP_STREAM_REST];
	struct zsock_pollfd fds[LISTENERS + CONNS];
	int who[LISTENERS + CONNS];              /* a listener's index, or -1 - a connection's */

	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		unsigned n = 0;

		sync_listeners();
		k_mutex_lock(&lock, K_FOREVER);
		for (unsigned i = 0; i < LISTENERS; i++) {
			if (listeners[i] >= 0) {
				fds[n].fd = listeners[i];
				fds[n].events = ZSOCK_POLLIN;
				who[n++] = (int)i;
			}
		}
		for (unsigned k = 0; k < CONNS; k++) {
			struct conn *cn = &conns[k];

			if (cn->sock < 0) {
				continue;
			}
			/* an answer gone, bytes kept behind the last frame: they go first */
			if (!cn->in_flight && smp_stream_has_rest(&cn->stream) &&
			    !smp_stream_resume(&cn->stream, on_frame, cn)) {
				close_conn(cn);
				continue;
			}
			if (!cn->in_flight) {
				fds[n].fd = cn->sock;
				fds[n].events = ZSOCK_POLLIN;
				who[n++] = -1 - (int)k;
			}
		}
		k_mutex_unlock(&lock);
		if (zsock_poll(fds, (int)n, 100) <= 0) {
			continue;
		}
		for (unsigned i = 0; i < n; i++) {
			if (!(fds[i].revents & (ZSOCK_POLLIN | ZSOCK_POLLHUP | ZSOCK_POLLERR))) {
				continue;
			}
			if (who[i] >= 0) {
				accept_on((unsigned)who[i]);
				continue;
			}
			struct conn *cn = &conns[-1 - who[i]];
			ssize_t got = zsock_recv(cn->sock, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);

			k_mutex_lock(&lock, K_FOREVER);
			if (got <= 0 || !smp_stream_feed(&cn->stream, buf, (size_t)got, on_frame, cn)) {
				close_conn(cn);
			}
			k_mutex_unlock(&lock);
		}
	}
}

int smp_tcp_init(void)
{
	int rc;

	k_mutex_init(&lock);
	for (unsigned k = 0; k < CONNS; k++) {
		conns[k].sock = -1;
	}
	for (unsigned i = 0; i < LISTENERS; i++) {
		listeners[i] = -1;
	}
	wanted[0] = true;
	smpt.functions.output = output;
	smpt.functions.get_mtu = get_mtu;
	smpt.functions.ud_copy = ud_copy;
	rc = smp_transport_init(&smpt);
	if (rc) {
		return rc;
	}
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), thread_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_thread_name_set(&thread, "smp_tcp");
	return 0;
}

void smp_tcp_set_bridge(smp_tcp_bridge_fn fn)
{
	bridge = fn;
}

void smp_tcp_belka_ports(const bool taken[STRELKA_BELKAS_MAX + 1])
{
	k_mutex_lock(&lock, K_FOREVER);
	for (unsigned n = 1; n <= STRELKA_BELKAS_MAX; n++) {
		wanted[n] = taken[n];
	}
	k_mutex_unlock(&lock);
}

void smp_tcp_from_belka(uint8_t belka, const uint8_t *frame, size_t len)
{
	k_mutex_lock(&lock, K_FOREVER);
	for (unsigned k = 0; k < CONNS; k++) {
		if (conns[k].sock >= 0 && conns[k].belka == belka) {
			send_all(conns[k].sock, frame, len);
			conns[k].in_flight = false;
		}
	}
	k_mutex_unlock(&lock);
}

bool smp_tcp_phone_address(struct net_sockaddr_in *addr)
{
	struct conn *latest = NULL;

	k_mutex_lock(&lock, K_FOREVER);
	for (unsigned k = 0; k < CONNS; k++) {
		if (conns[k].sock >= 0 && conns[k].belka == 0 && conns[k].opened &&
		    (latest == NULL || conns[k].opened > latest->opened)) {
			latest = &conns[k];
		}
	}
	if (latest != NULL) {
		*addr = latest->peer;
	}
	k_mutex_unlock(&lock);
	return latest != NULL;
}
