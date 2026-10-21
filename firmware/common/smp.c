#include <string.h>
#include <zcbor_encode.h>
#include "crypto.h"
#include "smp.h"

void smp_head_read(const uint8_t b[SMP_HDR_BYTES], struct smp_head *h)
{
	h->op = b[0] & 0x07u;
	h->version = (uint8_t)((b[0] >> 3) & 0x03u);
	h->flags = b[1];
	h->len = (uint16_t)((b[2] << 8) | b[3]);
	h->group = (uint16_t)((b[4] << 8) | b[5]);
	h->seq = b[6];
	h->id = b[7];
}

void smp_head_write(uint8_t b[SMP_HDR_BYTES], const struct smp_head *h)
{
	b[0] = (uint8_t)((h->op & 0x07u) | ((h->version & 0x03u) << 3));
	b[1] = h->flags;
	b[2] = (uint8_t)(h->len >> 8);
	b[3] = (uint8_t)h->len;
	b[4] = (uint8_t)(h->group >> 8);
	b[5] = (uint8_t)h->group;
	b[6] = h->seq;
	b[7] = h->id;
}

void smp_head_answer(const struct smp_head *req, uint16_t len, struct smp_head *rsp)
{
	*rsp = *req;
	rsp->op = req->op == SMP_OP_READ ? SMP_OP_READ_RSP : SMP_OP_WRITE_RSP;
	rsp->version = SMP_VERSION;
	rsp->flags = 0;
	rsp->len = len;
}

void smp_stream_init(struct smp_stream *s)
{
	s->have = 0;
	s->rest_len = 0;
}

bool smp_stream_feed(struct smp_stream *s, const uint8_t *data, size_t n, smp_frame_fn on_frame,
		     void *ctx)
{
	if (s->rest_len) {
		/* bytes kept from before go first; what comes now is kept behind them */
		if (s->rest_len + n > sizeof(s->rest)) {
			s->have = 0;
			s->rest_len = 0;
			return false;
		}
		memcpy(s->rest + s->rest_len, data, n);
		s->rest_len += n;
		return true;
	}
	while (n) {
		size_t want, take;
		struct smp_head h;

		if (s->have < SMP_HDR_BYTES) {
			want = SMP_HDR_BYTES;
		} else {
			smp_head_read(s->buf, &h);
			if (h.len > SMP_FRAME_MAX) {
				s->have = 0;
				return false;
			}
			want = SMP_HDR_BYTES + h.len;
		}
		take = want - s->have < n ? want - s->have : n;
		memcpy(s->buf + s->have, data, take);
		s->have += take;
		data += take;
		n -= take;
		if (s->have < SMP_HDR_BYTES) {
			return true;
		}
		smp_head_read(s->buf, &h);
		if (h.len > SMP_FRAME_MAX) {
			s->have = 0;
			return false;
		}
		if (s->have == SMP_HDR_BYTES + h.len) {
			bool go_on = on_frame(s->buf, s->have, ctx);

			s->have = 0;
			if (!go_on && n) {
				if (n > sizeof(s->rest)) {
					return false;
				}
				memcpy(s->rest, data, n);
				s->rest_len = n;
				return true;
			}
		}
	}
	return true;
}

bool smp_stream_resume(struct smp_stream *s, smp_frame_fn on_frame, void *ctx)
{
	uint8_t kept[SMP_STREAM_REST];
	size_t n = s->rest_len;

	if (n == 0) {
		return true;
	}
	memcpy(kept, s->rest, n);
	s->rest_len = 0;
	return smp_stream_feed(s, kept, n, on_frame, ctx);
}

bool smp_stream_has_rest(const struct smp_stream *s)
{
	return s->rest_len != 0;
}

void smp_lock_init(struct smp_lock *l)
{
	memset(l, 0, sizeof(*l));
}

bool smp_lock_passes(const struct smp_lock *l, const struct smp_head *h)
{
	return l == NULL || l->open || (h->group == SMP_GROUP_STRELKA && h->id == CMD_AUTH);
}

bool smp_lock_check(struct smp_lock *l, const uint8_t hash[32], const uint8_t secret[32],
		    const uint8_t mac[SMP_MAC_BYTES])
{
	uint8_t want[SMP_MAC_BYTES], over[SMP_NONCE_BYTES + 32];
	uint8_t diff = 0;

	memcpy(over, l->nonce, SMP_NONCE_BYTES);
	memcpy(over + SMP_NONCE_BYTES, secret, 32);
	if (!l->nonce_set || strelka_hmac_sha256(hash, 32, over, sizeof(over), want) != 0) {
		l->nonce_set = false;
		return false;
	}
	l->nonce_set = false;            /* spent, right or wrong */
	for (unsigned i = 0; i < SMP_MAC_BYTES; i++) {
		diff |= (uint8_t)(want[i] ^ mac[i]);
	}
	return diff == 0;
}

static size_t error_frame(uint8_t *out, size_t cap, const struct smp_head *req, bool group_own,
			  uint16_t group, int rc)
{
	ZCBOR_STATE_E(e, 3, out + SMP_HDR_BYTES, cap > SMP_HDR_BYTES ? cap - SMP_HDR_BYTES : 0, 1);
	struct smp_head rsp;
	bool ok;

	if (cap < SMP_HDR_BYTES) {
		return 0;
	}
	ok = zcbor_map_start_encode(e, 1);
	if (group_own) {
		ok = ok && zcbor_tstr_put_lit(e, "err") && zcbor_map_start_encode(e, 2) &&
		     zcbor_tstr_put_lit(e, "group") && zcbor_uint32_put(e, group) &&
		     zcbor_tstr_put_lit(e, "rc") && zcbor_uint32_put(e, (uint32_t)rc) &&
		     zcbor_map_end_encode(e, 2);
	} else {
		ok = ok && zcbor_tstr_put_lit(e, "rc") && zcbor_int32_put(e, rc);
	}
	ok = ok && zcbor_map_end_encode(e, 1);
	if (!ok) {
		return 0;
	}
	smp_head_answer(req, (uint16_t)(e->payload - (out + SMP_HDR_BYTES)), &rsp);
	smp_head_write(out, &rsp);
	return SMP_HDR_BYTES + rsp.len;
}

size_t smp_error_frame(uint8_t *out, size_t cap, const struct smp_head *req, int rc)
{
	return error_frame(out, cap, req, false, 0, rc);
}

size_t smp_group_error_frame(uint8_t *out, size_t cap, const struct smp_head *req, uint16_t group,
			     uint16_t rc)
{
	return error_frame(out, cap, req, true, group, rc);
}
