#include <string.h>
#include "smp_esb.h"

/* ---- one frame out, fragment by fragment ---------------------------------------------------- */

static bool out_begin(struct smp_esb_out *o, const uint8_t *frame, size_t len)
{
	if (len > SMP_FRAME_MAX) {
		return false;
	}
	memcpy(o->frame, frame, len);
	o->len = len;
	o->pkt_len = 0;
	o->busy = frag_tx_begin(&o->tx, o->frame, o->len);
	return o->busy;
}

/* the fragment on the air, made when the last was acknowledged */
static size_t out_next(struct smp_esb_out *o, const struct frag_keys *k, uint32_t *count,
		       uint8_t out[ESB_PAYLOAD_MAX])
{
	if (!o->busy) {
		return 0;
	}
	if (o->pkt_len == 0) {
		o->pkt_len = frag_tx_next(&o->tx, k, count, o->pkt);
		if (o->pkt_len == 0) {
			o->busy = false;
			return 0;
		}
	}
	memcpy(out, o->pkt, o->pkt_len);
	return o->pkt_len;
}

/* acknowledged: the next one, or the frame out whole; true when it is */
static bool out_acked(struct smp_esb_out *o)
{
	o->pkt_len = 0;
	if (o->tx.at >= o->tx.len) {
		o->busy = false;
		return true;
	}
	return false;
}

/* ---- Povodok -------------------------------------------------------------------------------- */

void smp_esb_povodok_init(struct smp_esb_povodok *p, const uint8_t key[KEY_BYTES],
			  uint32_t address, uint8_t number)
{
	memset(p, 0, sizeof(*p));
	memcpy(p->key, key, KEY_BYTES);
	p->address = address;
	p->number = number;
}

static struct frag_keys keys_to(const struct smp_esb_povodok *p, uint8_t belka)
{
	struct frag_keys k = { .address = p->address, .sender = WHO(WHO_POVODOK, p->number),
			       .receiver = WHO(WHO_BELKA, belka) };

	memcpy(k.key, p->key, KEY_BYTES);
	return k;
}

static struct frag_keys keys_from(const struct smp_esb_povodok *p, uint8_t belka)
{
	struct frag_keys k = keys_to(p, belka);

	k.sender = WHO(WHO_BELKA, belka);
	k.receiver = WHO(WHO_POVODOK, p->number);
	return k;
}

/* the head done or let go: the queue moves up, the Belka's taking count kept */
static void pop(struct smp_esb_povodok *p)
{
	uint8_t to = p->q[0].to;

	if (p->rx.any) {
		p->rx_any[to] = true;
		p->rx_last[to] = p->rx.last_count;
	}
	memmove(&p->q[0], &p->q[1], (p->qn - 1) * sizeof(p->q[0]));
	p->qn--;
	p->started = p->answering = p->reached = false;
	p->out.busy = false;
	p->out.pkt_len = 0;
}

int smp_esb_povodok_queue(struct smp_esb_povodok *p, uint8_t to, uint32_t token,
			  const uint8_t *frame, size_t len, uint32_t period)
{
	struct smp_esb_request *r;

	if (p->qn >= SMP_ESB_QUEUE || len > SMP_FRAME_MAX || to == 0 || to > STRELKA_BELKAS_MAX) {
		return -1;
	}
	r = &p->q[p->qn++];
	r->to = to;
	r->token = token;
	r->since = period;
	r->len = len;
	memcpy(r->frame, frame, len);
	return 0;
}

uint8_t smp_esb_povodok_peer(struct smp_esb_povodok *p, uint32_t period)
{
	/* a request older than the bridge's patience: the port has answered 4, so let it go */
	while (p->qn && period - p->q[0].since >= SMP_ESB_HOLD_PERIODS) {
		p->dropped++;
		pop(p);
	}
	if (p->qn == 0) {
		return 0;
	}
	if (!p->started) {
		struct smp_esb_request *r = &p->q[0];

		p->started = out_begin(&p->out, r->frame, r->len);
		frag_rx_begin(&p->rx, 0);
		p->rx.any = p->rx_any[r->to];
		p->rx.last_count = p->rx_last[r->to];
		if (!p->started) {
			pop(p);
			return 0;
		}
	}
	return p->q[0].to;
}

size_t smp_esb_povodok_next(struct smp_esb_povodok *p, uint8_t out[ESB_PAYLOAD_MAX])
{
	struct frag_keys k;

	if (p->qn == 0 || !p->started || p->answering) {
		return 0;
	}
	k = keys_to(p, p->q[0].to);
	return out_next(&p->out, &k, &p->tx_count[p->q[0].to], out);
}

void smp_esb_povodok_acked(struct smp_esb_povodok *p)
{
	if (p->qn && p->started && !p->answering) {
		p->reached = true;
		if (out_acked(&p->out)) {
			p->answering = true;
		}
	}
}

bool smp_esb_povodok_reached(const struct smp_esb_povodok *p)
{
	return p->qn && p->started && (p->reached || p->answering);
}

void smp_esb_povodok_failed(struct smp_esb_povodok *p)
{
	if (p->qn && p->started) {
		p->dropped++;
		pop(p);
	}
}

int smp_esb_povodok_take(struct smp_esb_povodok *p, uint8_t from, const uint8_t *payload,
			 size_t len, const uint8_t **frame, size_t *frame_len, uint32_t *token)
{
	struct frag_keys k;
	enum frag_result r;

	if (p->qn == 0 || !p->answering || from != p->q[0].to) {
		return -1;                       /* no answer awaited from her */
	}
	k = keys_from(p, from);
	r = frag_rx_take(&p->rx, &k, payload, len);
	if (r == FRAG_FRAME) {
		*frame = p->rx.frame;
		*frame_len = p->rx.len;
		*token = p->q[0].token;
		pop(p);                          /* the frame stays in rx until the next exchange */
		return 1;
	}
	return r == FRAG_NONE ? 0 : -1;
}

/* ---- a Belka -------------------------------------------------------------------------------- */

void smp_esb_belka_init(struct smp_esb_belka *b, const uint8_t key[KEY_BYTES], uint32_t address,
			uint8_t number)
{
	memset(b, 0, sizeof(*b));
	memcpy(b->key, key, KEY_BYTES);
	b->address = address;
	b->number = number;
	frag_rx_begin(&b->rx, 0);
}

static struct frag_keys belka_keys(const struct smp_esb_belka *b, uint8_t povodok, bool sending)
{
	struct frag_keys k = { .address = b->address };

	memcpy(k.key, b->key, KEY_BYTES);
	k.sender = sending ? WHO(WHO_BELKA, b->number) : WHO(WHO_POVODOK, povodok);
	k.receiver = sending ? WHO(WHO_POVODOK, povodok) : WHO(WHO_BELKA, b->number);
	return k;
}

int smp_esb_belka_take(struct smp_esb_belka *b, uint8_t povodok, const uint8_t *payload,
		       size_t len)
{
	struct frag_keys k = belka_keys(b, povodok, false);
	enum frag_result r;

	if (povodok >= STRELKA_POVODOKS_MAX) {
		return -1;
	}
	if (povodok != b->rx_from) {
		/* another Povodok: its own count, a frame begun by the last one dropped */
		b->rx_any[b->rx_from] = b->rx.any;
		b->rx_last[b->rx_from] = b->rx.last_count;
		frag_rx_begin(&b->rx, 0);
		b->rx.any = b->rx_any[povodok];
		b->rx.last_count = b->rx_last[povodok];
		b->rx_from = povodok;
	}
	r = frag_rx_take(&b->rx, &k, payload, len);

	return r == FRAG_FRAME ? 1 : r == FRAG_NONE ? 0 : -1;
}

int smp_esb_belka_answer(struct smp_esb_belka *b, const uint8_t *frame, size_t len)
{
	return out_begin(&b->out, frame, len) ? 0 : -1;
}

size_t smp_esb_belka_next(struct smp_esb_belka *b, uint8_t povodok,
			  uint8_t out[ESB_PAYLOAD_MAX])
{
	struct frag_keys k = belka_keys(b, povodok, true);

	return out_next(&b->out, &k, &b->tx_count, out);
}

void smp_esb_belka_acked(struct smp_esb_belka *b)
{
	(void)out_acked(&b->out);
}
