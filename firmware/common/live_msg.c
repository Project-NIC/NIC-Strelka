#include <string.h>
#include "frame.h"
#include "live_msg.h"

void live_msg_begin(struct live_msg *m, uint16_t session, uint32_t period, uint16_t state,
		    uint8_t s0, uint8_t povodoks, const uint16_t povodok_state[STRELKA_POVODOKS_MAX])
{
	struct live_msg_head h = {
		.kind = LIVE_MSG_KIND, .version = LIVE_MSG_VERSION, .session = session,
		.period = period, .state = state, .s0 = s0, .povodoks = povodoks, .belkas = 0,
	};

	memcpy(h.povodok_state, povodok_state, sizeof(h.povodok_state));
	memcpy(m->bytes, &h, sizeof(h));
	m->len = sizeof(h);
}

bool live_msg_add(struct live_msg *m, uint8_t belka, uint8_t s0, uint8_t by, uint16_t state,
		  const uint8_t *steps, size_t steps_len)
{
	struct live_msg_entry e = { .belka = belka, .s0 = s0, .by = by, .state = state };
	struct live_msg_head *h = (struct live_msg_head *)m->bytes;

	if (m->len + sizeof(e) + steps_len > sizeof(m->bytes) || h->belkas == UINT8_MAX) {
		return false;
	}
	memcpy(m->bytes + m->len, &e, sizeof(e));
	m->len += sizeof(e);
	if (steps_len) {
		memcpy(m->bytes + m->len, steps, steps_len);
		m->len += steps_len;
	}
	h->belkas++;
	return true;
}

unsigned live_msg_from_link(const struct povodok_link *l, uint16_t session,
			    struct live_msg out[LIVE_MSG_MESSAGES])
{
	unsigned n = 1;
	uint8_t povodoks = povodok_link_povodoks(l);

	if (!l->first) {
		return 0;
	}
	live_msg_begin(&out[0], session, l->period, l->sync_state, l->sync_s0, povodoks,
		       l->povodok_state);
	for (unsigned k = 1; k <= STRELKA_BELKAS_MAX; k++) {
		const struct povodok_belka *b = &l->belka[k];

		if (!b->taken || !(b->heard || b->passed)) {
			continue;
		}
		if (!live_msg_add(&out[n - 1], (uint8_t)k, b->s0, b->by, b->state, b->steps,
				  b->steps_len)) {
			if (n == LIVE_MSG_MESSAGES) {
				break;                   /* more than two messages' worth: left out */
			}
			live_msg_begin(&out[n], session, l->period, l->sync_state, l->sync_s0,
				       povodoks, l->povodok_state);
			n++;
			live_msg_add(&out[n - 1], (uint8_t)k, b->s0, b->by, b->state, b->steps,
				     b->steps_len);
		}
	}
	return n;
}

bool live_msg_read_head(const uint8_t *bytes, size_t len, struct live_msg_head *h)
{
	if (len < sizeof(*h)) {
		return false;
	}
	memcpy(h, bytes, sizeof(*h));
	return h->kind == LIVE_MSG_KIND && h->version == LIVE_MSG_VERSION;
}

bool live_msg_next(const uint8_t *bytes, size_t len, size_t *at, struct live_msg_entry *e,
		   const uint8_t **steps, size_t *steps_len)
{
	size_t n;

	if (*at + sizeof(*e) > len) {
		return false;
	}
	memcpy(e, bytes + *at, sizeof(*e));
	if (e->belka == 0 || e->belka > STRELKA_BELKAS_MAX || !live_code_ok(S0_HIGH(e->s0))) {
		return false;
	}
	n = live_body_bytes(S0_HIGH(e->s0));
	if (*at + sizeof(*e) + n > len) {
		return false;
	}
	*steps = bytes + *at + sizeof(*e);
	*steps_len = n;
	*at += sizeof(*e) + n;
	return true;
}
