#include <stdio.h>
#include <string.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "fetch.h"
#include "smp.h"

void fetch_begin(struct fetch *f, uint16_t s, const uint8_t *belkas, const uint32_t *from,
		 unsigned n)
{
	memset(f, 0, sizeof(*f));
	if (n > STRELKA_BELKAS_MAX) {
		n = STRELKA_BELKAS_MAX;
	}
	f->on = true;
	f->s = s;
	f->n = n;
	f->last_failed = -1;
	for (unsigned i = 0; i < n; i++) {
		f->belka[i] = belkas[i];
		f->off[i] = from ? from[i] : 0;
	}
}

static int find(const struct fetch *f, uint8_t belka)
{
	for (unsigned i = 0; i < f->n; i++) {
		if (f->belka[i] == belka) {
			return (int)i;
		}
	}
	return -1;
}

size_t fetch_next(struct fetch *f, uint8_t *belka, uint8_t *frame, size_t cap)
{
	char name[FETCH_NAME_MAX];
	struct smp_head h = { .op = SMP_OP_READ, .version = SMP_VERSION, .group = SMP_GROUP_FS };
	bool ok;

	if (!f->on || cap < SMP_HDR_BYTES + 48) {
		return 0;
	}
	int pick = -1, others = 0;

	for (unsigned k = 0; k < f->n; k++) {
		others += f->end[k] == FETCH_RUNNING && (int)k != f->last_failed;
	}
	for (unsigned k = 0; k < f->n; k++) {
		unsigned i = (f->at + k) % f->n;

		if (f->end[i] != FETCH_RUNNING || ((int)i == f->last_failed && others)) {
			continue;
		}
		if (pick < 0 || f->off[i] < f->off[pick]) {
			pick = (int)i;               /* the furthest behind, the first in turn of them */
		}
	}
	if (pick >= 0) {
		unsigned i = (unsigned)pick;

		snprintf(name, sizeof(name), FETCH_NAME, f->s, f->belka[i]);
		{
			ZCBOR_STATE_E(e, 1, frame + SMP_HDR_BYTES, cap - SMP_HDR_BYTES, 1);

			ok = zcbor_map_start_encode(e, 2) && zcbor_tstr_put_lit(e, "name") &&
			     zcbor_tstr_encode_ptr(e, name, strlen(name)) &&
			     zcbor_tstr_put_lit(e, "off") && zcbor_uint32_put(e, f->off[i]) &&
			     zcbor_map_end_encode(e, 2);
			if (!ok) {
				return 0;
			}
			h.len = (uint16_t)(e->payload - (frame + SMP_HDR_BYTES));
		}
		h.seq = f->seq++;
		smp_head_write(frame, &h);
		*belka = f->belka[i];
		f->at = i + 1;                   /* the next in turn */
		if (pick != f->last_failed) {
			f->last_failed = -1;
		}
		return SMP_HDR_BYTES + h.len;
	}
	return 0;
}

int fetch_answer(struct fetch *f, uint8_t belka, const uint8_t *frame, size_t len, uint32_t *off,
		 const uint8_t **data_at, size_t *data_len)
{
	struct smp_head h;
	struct zcbor_string key, data = { 0 };
	uint32_t got_off = 0, got_len = 0;
	int32_t rc = 0;
	bool has_off = false, has_len = false, has_data = false, has_rc = false;
	int i = find(f, belka);

	if (i < 0 || f->end[i] != FETCH_RUNNING || len < SMP_HDR_BYTES) {
		return 0;
	}
	smp_head_read(frame, &h);
	if (h.op != SMP_OP_READ_RSP || h.group != SMP_GROUP_FS || h.id != 0 ||
	    SMP_HDR_BYTES + (size_t)h.len > len) {
		return 0;
	}
	{
		ZCBOR_STATE_D(d, 2, frame + SMP_HDR_BYTES, h.len, 1, 0);

		if (!zcbor_map_start_decode(d)) {
			return 0;
		}
		while (!zcbor_array_at_end(d)) {
			bool ok;

			if (!zcbor_tstr_decode(d, &key)) {
				return 0;
			}
			if (key.len == 3 && memcmp(key.value, "off", 3) == 0) {
				ok = has_off = zcbor_uint32_decode(d, &got_off);
			} else if (key.len == 3 && memcmp(key.value, "len", 3) == 0) {
				ok = has_len = zcbor_uint32_decode(d, &got_len);
			} else if (key.len == 4 && memcmp(key.value, "data", 4) == 0) {
				ok = has_data = zcbor_bstr_decode(d, &data);
			} else if (key.len == 2 && memcmp(key.value, "rc", 2) == 0) {
				ok = has_rc = zcbor_int32_decode(d, &rc);
			} else {
				ok = zcbor_any_skip(d, NULL);
			}
			if (!ok) {
				return 0;
			}
		}
	}
	if (has_rc && rc != 0) {
		f->end[i] = FETCH_REFUSED;       /* her file not there, or another error */
		f->here[i] = true;
		return -1;
	}
	if (!has_off || !has_data || got_off != f->off[i]) {
		return 0;                        /* not the answer to the request awaited */
	}
	if (has_len) {
		f->len[i] = got_len;
		f->len_known[i] = true;
	}
	f->off[i] += (uint32_t)data.len;
	f->tries[i] = 0;
	f->here[i] = true;
	if (data.len == 0 || (f->len_known[i] && f->off[i] >= f->len[i])) {
		f->end[i] = FETCH_WHOLE;
	}
	*off = got_off;
	*data_at = data.value;
	*data_len = data.len;
	return 1;
}

void fetch_let_go(struct fetch *f, uint8_t belka, uint32_t now)
{
	int i = find(f, belka);

	if (i < 0 || f->end[i] != FETCH_RUNNING) {
		return;
	}
	f->last_failed = i;
	if (f->tries[i] == 0) {
		f->first_miss[i] = now;
	}
	if (f->tries[i] < UINT8_MAX) {
		f->tries[i]++;
	}
	if (!f->here[i] && f->tries[i] >= FETCH_TRIES &&
	    now - f->first_miss[i] >= FETCH_ELSEWHERE_PERIODS) {
		f->end[i] = FETCH_ELSEWHERE;
	} else if (f->here[i] && f->tries[i] >= FETCH_GONE) {
		f->end[i] = FETCH_GONE_END;
	}
}

bool fetch_done(const struct fetch *f)
{
	for (unsigned i = 0; i < f->n; i++) {
		if (f->end[i] == FETCH_RUNNING) {
			return false;
		}
	}
	return true;
}

bool fetch_here(const struct fetch *f, uint8_t belka)
{
	int i = find(f, belka);

	return i >= 0 && f->here[i];
}

uint8_t fetch_end_of(const struct fetch *f, uint8_t belka)
{
	int i = find(f, belka);

	return i < 0 ? FETCH_RUNNING : f->end[i];
}
