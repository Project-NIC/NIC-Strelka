#include <string.h>
#include "frag.h"

void frag_nonce(uint8_t nonce[NONCE_BYTES], uint32_t address, uint8_t sender, uint8_t receiver,
		uint32_t count, uint8_t index)
{
	memset(nonce, 0, NONCE_BYTES);
	for (int i = 0; i < 4; i++) {
		nonce[i] = (uint8_t)(address >> (8 * i));
		nonce[5 + i] = (uint8_t)(count >> (8 * i));
	}
	nonce[4] = sender;
	nonce[9] = index;
	nonce[10] = receiver;
}

void frag_tx_begin(struct frag_tx *t, const uint8_t *frame, size_t len)
{
	t->frame = frame;
	t->len = len > SMP_FRAME_MAX ? SMP_FRAME_MAX : len;
	t->at = 0;
	t->index = 0;
}

size_t frag_tx_next(struct frag_tx *t, const struct frag_keys *k, uint32_t *count,
		    uint8_t out[ESB_PAYLOAD_MAX])
{
	size_t left = t->len - t->at;
	size_t n = left > FRAG_DATA_MAX ? FRAG_DATA_MAX : left;
	uint8_t kind, nonce[NONCE_BYTES];

	if (t->at == t->len && (t->len > 0 || t->index > 0)) {
		return 0;                        /* all gone; an empty frame goes as one whole */
	}
	if (t->at == 0 && n == left) {
		kind = FRAG_WHOLE;
	} else if (t->at == 0) {
		kind = FRAG_FIRST;
	} else if (n == left) {
		kind = FRAG_LAST;
	} else {
		kind = FRAG_MIDDLE;
	}
	out[0] = FRAG_BYTE0(kind, t->index);
	for (int i = 0; i < 4; i++) {
		out[1 + i] = (uint8_t)(*count >> (8 * i));
	}
	memcpy(out + FRAG_HEAD_BYTES, t->frame + t->at, n);
	frag_nonce(nonce, k->address, k->sender, k->receiver, *count, t->index);
	strelka_ccm_encrypt(k->key, nonce, out, FRAG_HEAD_BYTES + n, NULL, 0, NULL,
			    out + FRAG_HEAD_BYTES + n);
	t->at += n;
	t->index++;
	(*count)++;
	return FRAG_HEAD_BYTES + n + FRAG_MIC_BYTES;
}

void frag_rx_begin(struct frag_rx *r, uint32_t floor)
{
	memset(r, 0, sizeof(*r));
	r->floor = floor;
}

enum frag_result frag_rx_take(struct frag_rx *r, const struct frag_keys *k, const uint8_t *payload,
			      size_t len)
{
	uint8_t kind, index, nonce[NONCE_BYTES];
	uint32_t count = 0;
	size_t n;

	if (len < FRAG_HEAD_BYTES + FRAG_MIC_BYTES || len > ESB_PAYLOAD_MAX) {
		return FRAG_DROPPED;
	}
	kind = FRAG_KIND(payload[0]);
	index = FRAG_INDEX(payload[0]);
	for (int i = 0; i < 4; i++) {
		count |= (uint32_t)payload[1 + i] << (8 * i);
	}
	if (count < r->floor || (r->any && count <= r->last_count)) {
		return FRAG_DROPPED;             /* a replay, or an old fragment */
	}
	n = len - FRAG_HEAD_BYTES - FRAG_MIC_BYTES;
	frag_nonce(nonce, k->address, k->sender, k->receiver, count, index);
	if (strelka_ccm_decrypt(k->key, nonce, payload, FRAG_HEAD_BYTES + n, NULL, 0, NULL,
				payload + FRAG_HEAD_BYTES + n) != 0) {
		return FRAG_DROPPED;             /* not the set's */
	}
	r->any = true;
	r->last_count = count;
	if (kind == FRAG_WHOLE || kind == FRAG_FIRST) {
		if (index != 0) {
			r->open = false;
			return FRAG_DROPPED;
		}
		r->len = 0;
		r->open = true;
		r->expect = 0;
	} else if (!r->open || index != r->expect) {
		r->open = false;                 /* a piece of a frame not begun, or out of order */
		return FRAG_DROPPED;
	}
	if (r->len + n > SMP_FRAME_MAX) {
		r->open = false;
		return FRAG_DROPPED;
	}
	memcpy(r->frame + r->len, payload + FRAG_HEAD_BYTES, n);
	r->len += n;
	r->expect = (uint8_t)(index + 1);
	if (kind == FRAG_WHOLE || kind == FRAG_LAST) {
		r->open = false;
		return FRAG_FRAME;
	}
	return FRAG_NONE;
}
