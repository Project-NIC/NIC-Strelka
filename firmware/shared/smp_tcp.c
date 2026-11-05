/* NIC-Strelka — SMP over TCP on Povodok: a transport of MCUmgr's for Povodok's own port, the
 * Belkas' ports bridged to the link, the lock on every connection (smp_tcp.h). One thread polls
 * the listening sockets and the connections; a whole frame cut from a stream goes to MCUmgr in a
 * net_buf whose user data names the connection and its generation, and MCUmgr's answer comes back
 * through `output` to that connection — one request in flight a connection, the stream read no
 * further until the answer has gone, so that a frame pipelined behind the lock's own is judged
 * after it. The connections' table is under one mutex, since the TCP thread, MCUmgr's work queue
 * and the link's thread all reach it; no send waits under it — a socket that will not take the
 * bytes within its time is closed, so that a phone gone quiet holds nothing else up. Built
 * against nRF Connect SDK v3.4.0, not run on hardware. */
#include <errno.h>
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
#define SEND_MS    1000                   /* an answer's time to go: a peer slower is dropped */
#define IDLE_S     120                    /* a connection without a frame that long is closed */
#define KEEP_IDLE_S 60                    /* TCP's keep-alive: after 60 s idle, every 10 s, 3 */
#define KEEP_INTVL_S 10
#define KEEP_CNT   3
#define LIVE_QUIET_MS 3000                /* the live view stops after the phone is this long
					   * silent; it reads 64/14 once a second (COMMANDS.md) */
#define HELD_BYTES 48                     /* a wrong MAC's answer, held for its delay */

struct conn {
	int sock;                         /* -1 free */
	uint16_t gen;                     /* one up each time the slot is taken: a late answer to
					   * a connection gone finds another generation */
	uint8_t belka;                    /* 0 Povodok's own port, else the Belka's number */
	bool in_flight;                   /* a request handed on, its answer not yet gone */
	struct net_sockaddr_in peer;      /* where it came from */
	uint32_t opened;                  /* the count when it passed the lock; 0 not yet */
	int64_t last_frame;               /* when its last whole frame came */
	int64_t flight_until;             /* a bridged request's deadline */
	struct smp_head flight_head;      /* the bridged request's header, for its timeout */
	uint8_t held[HELD_BYTES];         /* a wrong MAC's answer, sent at held_until: this */
	uint8_t held_len;                 /* connection waits alone, nothing read on it meanwhile */
	int64_t held_until;
	struct smp_stream stream;
	struct smp_lock lock;
};

struct tag {                              /* a net_buf's user data: 2 of the 4 B */
	uint8_t idx;
	uint16_t gen;
};
BUILD_ASSERT(sizeof(struct tag) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "the connection's tag fits the net_buf's user data");

static struct conn conns[CONNS];
static int listeners[LISTENERS];          /* [0] Povodok's, [n] the Belka's; -1 closed */
static bool wanted[LISTENERS];
static struct k_mutex lock;               /* conns[], the listeners' wants */
static uint32_t opened_count;             /* connections that passed the lock, so far */
static uint8_t wrong_run;                 /* wrong MACs in a row, every connection together:
					   * the next is answered after 1 s, 2, 4 … 32 */
static struct k_spinlock phone_lock;      /* the phone's address, read by the link's thread */
static struct net_sockaddr_in phone;
static bool phone_known;
static int phone_idx = -1;                /* its connection, and when it last sent a frame */
static int64_t phone_seen;
static struct smp_transport smpt;
static smp_tcp_bridge_fn bridge;
static atomic_t service_mode = ATOMIC_INIT(1);  /* the service mode: the Belkas' settings pass */

void smp_tcp_service(bool on)
{
	atomic_set(&service_mode, on);
}

bool smp_tcp_joined(void)
{
	return opened_count > 0;                /* one word, read alone */
}

/* a Belka's command that sets or erases: the service mode's alone */
static bool a_setting(const struct smp_head *h)
{
	return h->group == SMP_GROUP_STRELKA &&
	       ((h->id == CMD_CAL && h->op == SMP_OP_WRITE) || h->id == CMD_CARD ||
		h->id == CMD_FORGET || h->id == CMD_KEY || h->id == CMD_JOIN);
}
static K_THREAD_STACK_DEFINE(stack, 4096);
static struct k_thread thread;

/* the whole of it sent without waiting, tried again every 10 ms within `ms`; false when the
 * socket would not take it in that time or failed: the connection is then to be closed. Never
 * under the mutex. */
static bool send_all(int sock, const uint8_t *data, size_t len, int ms)
{
	int64_t until = k_uptime_get() + ms;

	while (len) {
		ssize_t n = zsock_send(sock, data, len, ZSOCK_MSG_DONTWAIT);

		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			if (ms == 0 || k_uptime_get() >= until) {
				return false;
			}
			k_sleep(K_MSEC(10));
			continue;
		}
		if (n <= 0) {
			return false;
		}
		data += n;
		len -= (size_t)n;
	}
	return true;
}

/* under the mutex: the phone's address for the live view, the latest connection on Povodok's
 * own port that passed the lock */
static void phone_update(void)
{
	struct conn *latest = NULL;
	k_spinlock_key_t key;

	for (unsigned k = 0; k < CONNS; k++) {
		if (conns[k].sock >= 0 && conns[k].belka == 0 && conns[k].opened &&
		    (latest == NULL || conns[k].opened > latest->opened)) {
			latest = &conns[k];
		}
	}
	key = k_spin_lock(&phone_lock);
	phone_known = latest != NULL;
	phone_idx = latest != NULL ? (int)(latest - conns) : -1;
	if (latest != NULL) {
		phone = latest->peer;
		phone_seen = latest->last_frame;
	}
	k_spin_unlock(&phone_lock, key);
}

/* under the mutex: a frame came on the connection; the phone's, it keeps the live view going */
static void frame_seen(struct conn *c)
{
	k_spinlock_key_t key;

	c->last_frame = k_uptime_get();
	key = k_spin_lock(&phone_lock);
	if (phone_idx == (int)(c - conns)) {
		phone_seen = c->last_frame;
	}
	k_spin_unlock(&phone_lock, key);
}

/* under the mutex: the lock passed on a connection, noted when its answer goes out */
static void note_opened(struct conn *c)
{
	if (c->lock.open && c->opened == 0) {
		c->opened = ++opened_count;
		phone_update();
	}
}

/* under the mutex */
static void close_conn(struct conn *c)
{
	if (c->sock >= 0) {
		zsock_close(c->sock);
		c->sock = -1;
		c->in_flight = false;
		c->held_len = 0;
		strelka_x25519_end(&c->lock.pair);   /* a lock begun and never answered */
		phone_update();
	}
}

/* MCUmgr's answer: to the connection the request came by, if it is still that connection; the
 * send outside the mutex, the connection closed when it would not go */
static int output(struct net_buf *nb)
{
	struct tag t;
	int sock = -1;
	bool ok;

	memcpy(&t, net_buf_user_data(nb), sizeof(t));
	k_mutex_lock(&lock, K_FOREVER);
	if (t.idx < CONNS && conns[t.idx].sock >= 0 && conns[t.idx].gen == t.gen) {
		struct conn *c = &conns[t.idx];
		struct smp_head h = { 0 };

		if (nb->len >= SMP_HDR_BYTES) {
			smp_head_read(nb->data, &h);
		}
		if (nb->len >= SMP_HDR_BYTES && h.op == SMP_OP_WRITE_RSP &&
		    h.group == SMP_GROUP_STRELKA && h.id == CMD_AUTH) {
			if (c->lock.failures > 0 && nb->len <= sizeof(c->held)) {
				/* a wrong MAC: held on this connection alone, the delay from the
				 * run of wrong ones over every connection; in_flight stays */
				if (wrong_run < UINT8_MAX) {
					wrong_run++;
				}
				memcpy(c->held, nb->data, nb->len);
				c->held_len = (uint8_t)nb->len;
				c->held_until = k_uptime_get() +
						(int64_t)MSEC_PER_SEC * (1 << MIN(wrong_run - 1, 5));
				k_mutex_unlock(&lock);
				smp_packet_free(nb);
				return 0;
			}
			if (c->lock.open) {
				wrong_run = 0;           /* a right one */
			}
		}
		sock = c->sock;
	}
	k_mutex_unlock(&lock);
	if (sock >= 0) {
		ok = send_all(sock, nb->data, nb->len, SEND_MS);
		k_mutex_lock(&lock, K_FOREVER);
		if (conns[t.idx].sock == sock && conns[t.idx].gen == t.gen) {
			conns[t.idx].in_flight = false;
			if (ok) {
				note_opened(&conns[t.idx]);
			} else {
				close_conn(&conns[t.idx]);
			}
		}
		k_mutex_unlock(&lock);
	}
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

/* an error frame to the connection from the TCP thread, under the mutex: a short wait, and the
 * connection closed when it would not take it; true when it went */
static bool answer_error(struct conn *c, const uint8_t *out, size_t n)
{
	if (!send_all(c->sock, out, n, 100)) {
		close_conn(c);
		return false;
	}
	return true;
}

/* a whole frame from a connection, the TCP thread's, under the mutex: the version, the lock
 * first; Povodok's own to MCUmgr, a Belka's to the link — the lock's own command answered here
 * for both. False: one in flight now, the stream read no further until it is answered, or the
 * connection closed */
static bool on_frame(const uint8_t *frame, size_t len, void *ctx)
{
	struct conn *c = ctx;
	struct smp_head h;
	struct net_buf *nb;
	struct tag t = { .idx = (uint8_t)(c - conns), .gen = c->gen };
	uint8_t out[32];
	size_t n;

	frame_seen(c);
	smp_head_read(frame, &h);
	if (h.op != SMP_OP_READ && h.op != SMP_OP_WRITE) {
		return true;
	}
	if (h.version > SMP_VERSION) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_TOO_NEW);   /* 0 and 1 taken */
		return answer_error(c, out, n);
	}
	if (!smp_lock_passes(&c->lock, &h)) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_ACCESS);
		return answer_error(c, out, n);
	}
	if (c->belka && !atomic_get(&service_mode) && a_setting(&h)) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_BAD_STATE);
		return answer_error(c, out, n);
	}
	if (c->belka && !(h.group == SMP_GROUP_STRELKA && h.id == CMD_AUTH)) {
		uint32_t token = ((uint32_t)t.idx << 16) | t.gen;

		if (bridge == NULL || bridge(c->belka, token, frame, len) != 0) {
			n = smp_group_error_frame(out, sizeof(out), &h, SMP_GROUP_STRELKA,
						  STRELKA_ERR_UNREACHABLE);
			return answer_error(c, out, n);
		}
		c->in_flight = true;             /* the answer comes by smp_tcp_from_belka */
		c->flight_until = k_uptime_get() + SMP_TCP_BRIDGE_MS;
		c->flight_head = h;
		return false;
	}
	nb = smp_packet_alloc();
	if (nb == NULL) {
		n = smp_error_frame(out, sizeof(out), &h, SMP_ERR_NOMEM);
		return answer_error(c, out, n);
	}
	net_buf_add_mem(nb, frame, len);
	memcpy(net_buf_user_data(nb), &t, sizeof(t));
	c->in_flight = true;
	c->flight_head.group = 0xFFFF;           /* MCUmgr's: no deadline here */
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
	/* a peer gone without a FIN — a phone's Wi-Fi dropped — is found by TCP's keep-alive,
	 * so that its slot comes free */
	{
		int on = 1, idle = KEEP_IDLE_S, intvl = KEEP_INTVL_S, cnt = KEEP_CNT;

		(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_KEEPALIVE, &on, sizeof(on));
		(void)zsock_setsockopt(sock, NET_IPPROTO_TCP, ZSOCK_TCP_KEEPIDLE, &idle, sizeof(idle));
		(void)zsock_setsockopt(sock, NET_IPPROTO_TCP, ZSOCK_TCP_KEEPINTVL, &intvl,
				       sizeof(intvl));
		(void)zsock_setsockopt(sock, NET_IPPROTO_TCP, ZSOCK_TCP_KEEPCNT, &cnt, sizeof(cnt));
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
			conns[k].last_frame = k_uptime_get();
			conns[k].held_len = 0;
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
			int64_t now = k_uptime_get();

			if (cn->sock < 0) {
				continue;
			}
			/* a wrong MAC's answer held: out when its delay is over, the connection
			 * reading on, or closed if it was the third */
			if (cn->held_len && now >= cn->held_until) {
				bool sent = answer_error(cn, cn->held, cn->held_len);

				cn->held_len = 0;
				if (!sent) {
					continue;
				}
				cn->in_flight = false;
			}
			/* a bridged request the Belka never answered: a timeout to the peer, and
			 * the connection reads on; a connection idle too long let go */
			if (cn->in_flight && cn->flight_head.group != 0xFFFF && now >= cn->flight_until) {
				uint8_t out[32];
				size_t n = smp_error_frame(out, sizeof(out), &cn->flight_head,
							   SMP_ERR_TIMEOUT);

				cn->in_flight = false;
				if (!answer_error(cn, out, n)) {
					continue;
				}
			}
			if (now - cn->last_frame > (int64_t)IDLE_S * 1000 ||
			    (cn->lock.failures >= 3 && !cn->held_len)) {
				close_conn(cn);          /* idle too long; or three wrong MACs */
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

void smp_tcp_from_belka(uint8_t belka, uint32_t token, const uint8_t *frame, size_t len)
{
	unsigned idx = token >> 16;
	uint16_t gen = (uint16_t)token;
	struct smp_head h;
	struct conn *c;

	if (len < SMP_HDR_BYTES) {
		return;
	}
	smp_head_read(frame, &h);
	k_mutex_lock(&lock, K_FOREVER);
	c = idx < CONNS ? &conns[idx] : NULL;
	/* the answer to the request awaited, by its group, command and sequence: one come after
	 * its request was answered with 4 is dropped, not taken for the next request's */
	if (c != NULL && c->sock >= 0 && c->gen == gen && c->belka == belka && c->in_flight &&
	    h.group == c->flight_head.group && h.id == c->flight_head.id &&
	    h.seq == c->flight_head.seq) {
		c->in_flight = false;
		/* the link's thread: no waiting on the socket; a peer that cannot take an answer
		 * at once has stopped reading, and is let go */
		if (!send_all(c->sock, frame, len, 0)) {
			close_conn(c);
		}
	}
	k_mutex_unlock(&lock);
}

bool smp_tcp_phone_address(struct net_sockaddr_in *addr)
{
	k_spinlock_key_t key = k_spin_lock(&phone_lock);
	bool known = phone_known && k_uptime_get() - phone_seen <= LIVE_QUIET_MS;

	if (known) {
		*addr = phone;
	}
	k_spin_unlock(&phone_lock, key);
	return known;
}
