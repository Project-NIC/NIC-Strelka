/* A Belka's side of the download on the host: MCUmgr's file group as it answers 8/0, read and
 * written here for tests/test_fetch.c and tests/test_world.c — on the unit, Zephyr's fs_mgmt. */
#ifndef STRELKA_FETCH_BELKA_H
#define STRELKA_FETCH_BELKA_H

#include <string.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "smp.h"

/* a request of 8/0: its name and offset, its sequence */
static inline bool fetch_belka_request(const uint8_t *frame, size_t len, char *name,
				       size_t name_cap, uint32_t *off, uint8_t *seq)
{
	struct smp_head h;
	struct zcbor_string key, s;
	bool has_name = false, has_off = false;

	if (len < SMP_HDR_BYTES) {
		return false;
	}
	smp_head_read(frame, &h);
	if (h.op != SMP_OP_READ || h.group != SMP_GROUP_FS || h.id != 0 ||
	    SMP_HDR_BYTES + (size_t)h.len > len) {
		return false;
	}
	{
		ZCBOR_STATE_D(d, 2, frame + SMP_HDR_BYTES, h.len, 1, 0);

		if (!zcbor_map_start_decode(d)) {
			return false;
		}
		while (!zcbor_array_at_end(d)) {
			if (!zcbor_tstr_decode(d, &key)) {
				return false;
			}
			if (key.len == 4 && memcmp(key.value, "name", 4) == 0) {
				if (!zcbor_tstr_decode(d, &s) || s.len >= name_cap) {
					return false;
				}
				memcpy(name, s.value, s.len);
				name[s.len] = '\0';
				has_name = true;
			} else if (key.len == 3 && memcmp(key.value, "off", 3) == 0) {
				if (!zcbor_uint32_decode(d, off)) {
					return false;
				}
				has_off = true;
			} else if (!zcbor_any_skip(d, NULL)) {
				return false;
			}
		}
	}
	*seq = h.seq;
	return has_name && has_off;
}

/* its answer: the data from off, the file's length with the first; or rc alone, an error */
static inline size_t fetch_belka_answer(uint8_t *out, size_t cap, uint8_t seq, int rc,
					uint32_t off, const uint8_t *data, size_t n,
					uint32_t file_len)
{
	struct smp_head h = { .op = SMP_OP_READ_RSP, .version = SMP_VERSION,
			      .group = SMP_GROUP_FS, .seq = seq };
	bool ok;

	{
		ZCBOR_STATE_E(e, 1, out + SMP_HDR_BYTES, cap - SMP_HDR_BYTES, 1);

		if (rc) {
			ok = zcbor_map_start_encode(e, 1) && zcbor_tstr_put_lit(e, "rc") &&
			     zcbor_int32_put(e, rc) && zcbor_map_end_encode(e, 1);
		} else {
			ok = zcbor_map_start_encode(e, 3) && zcbor_tstr_put_lit(e, "off") &&
			     zcbor_uint32_put(e, off) && zcbor_tstr_put_lit(e, "data") &&
			     zcbor_bstr_encode_ptr(e, (const char *)data, n) &&
			     (off != 0 || (zcbor_tstr_put_lit(e, "len") &&
					   zcbor_uint32_put(e, file_len))) &&
			     zcbor_map_end_encode(e, 3);
		}
		if (!ok) {
			return 0;
		}
		h.len = (uint16_t)(e->payload - (out + SMP_HDR_BYTES));
	}
	smp_head_write(out, &h);
	return SMP_HDR_BYTES + h.len;
}

/* how much of a file one answer carries in a frame of cap: its head and CBOR's keys left out */
#define FETCH_BELKA_DATA(cap) ((cap) - SMP_HDR_BYTES - 32u)

#endif
