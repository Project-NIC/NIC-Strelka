#include <string.h>
#include "crypto.h"
#include "frame.h"
#include "link_rules.h"
#include "strelka_mgmt.h"

/* ---- the request's map, its keys in any order ---------------------------------------------- */

struct field {
	const char *key;
	bool (*dec)(zcbor_state_t *d, void *out);
	void *out;
	bool found;
};

struct bstr_fixed {         /* a byte string of one length */
	uint8_t *out;
	size_t len;
};
struct i16_list {
	int16_t *out;
	unsigned n;
};
struct u16_list {
	uint16_t *out;
	unsigned n;
};

static bool dec_u8(zcbor_state_t *d, void *out)
{
	uint32_t v;

	if (!zcbor_uint32_decode(d, &v) || v > UINT8_MAX) {
		return false;
	}
	*(uint8_t *)out = (uint8_t)v;
	return true;
}

static bool dec_u16(zcbor_state_t *d, void *out)
{
	uint32_t v;

	if (!zcbor_uint32_decode(d, &v) || v > UINT16_MAX) {
		return false;
	}
	*(uint16_t *)out = (uint16_t)v;
	return true;
}

static bool dec_u32(zcbor_state_t *d, void *out)
{
	return zcbor_uint32_decode(d, (uint32_t *)out);
}

static bool dec_u64(zcbor_state_t *d, void *out)
{
	return zcbor_uint64_decode(d, (uint64_t *)out);
}

static bool dec_i16(zcbor_state_t *d, void *out)
{
	int32_t v;

	if (!zcbor_int32_decode(d, &v) || v < INT16_MIN || v > INT16_MAX) {
		return false;
	}
	*(int16_t *)out = (int16_t)v;
	return true;
}

static bool dec_bool(zcbor_state_t *d, void *out)
{
	return zcbor_bool_decode(d, (bool *)out);
}

static bool dec_tstr(zcbor_state_t *d, void *out)
{
	return zcbor_tstr_decode(d, (struct zcbor_string *)out);
}

static bool dec_bstr_fixed(zcbor_state_t *d, void *out)
{
	struct bstr_fixed *f = out;
	struct zcbor_string s;

	if (!zcbor_bstr_decode(d, &s) || s.len != f->len) {
		return false;
	}
	memcpy(f->out, s.value, s.len);
	return true;
}

static bool dec_i16_list(zcbor_state_t *d, void *out)
{
	struct i16_list *l = out;

	if (!zcbor_list_start_decode(d)) {
		return false;
	}
	for (unsigned i = 0; i < l->n; i++) {
		if (!dec_i16(d, &l->out[i])) {
			return false;
		}
	}
	return zcbor_list_end_decode(d);
}

static bool dec_u16_list(zcbor_state_t *d, void *out)
{
	struct u16_list *l = out;

	if (!zcbor_list_start_decode(d)) {
		return false;
	}
	for (unsigned i = 0; i < l->n; i++) {
		if (!dec_u16(d, &l->out[i])) {
			return false;
		}
	}
	return zcbor_list_end_decode(d);
}

/* the map read, a field unknown skipped; an empty body is an empty map; false when malformed */
static bool decode_map(zcbor_state_t *d, struct field *fields, size_t n)
{
	if (d->payload >= d->payload_end) {
		return true;
	}
	if (!zcbor_map_start_decode(d)) {
		return false;
	}
	while (!zcbor_array_at_end(d)) {
		struct zcbor_string key;
		bool known = false;

		if (!zcbor_tstr_decode(d, &key)) {
			return false;
		}
		for (size_t i = 0; i < n; i++) {
			if (key.len == strlen(fields[i].key) &&
			    memcmp(key.value, fields[i].key, key.len) == 0) {
				if (!fields[i].dec(d, fields[i].out)) {
					return false;
				}
				fields[i].found = true;
				known = true;
				break;
			}
		}
		if (!known && !zcbor_any_skip(d, NULL)) {
			return false;
		}
	}
	return zcbor_map_end_decode(d);
}

static bool all_found(const struct field *fields, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (!fields[i].found) {
			return false;
		}
	}
	return true;
}

#define FIELDS(f) f, sizeof(f) / sizeof((f)[0])

/* ---- the answer's pairs --------------------------------------------------------------------- */

static bool put_u(zcbor_state_t *e, const char *key, uint32_t v)
{
	return zcbor_tstr_encode_ptr(e, key, strlen(key)) && zcbor_uint32_put(e, v);
}

static bool put_i(zcbor_state_t *e, const char *key, int32_t v)
{
	return zcbor_tstr_encode_ptr(e, key, strlen(key)) && zcbor_int32_put(e, v);
}

static bool put_bool(zcbor_state_t *e, const char *key, bool v)
{
	return zcbor_tstr_encode_ptr(e, key, strlen(key)) && zcbor_bool_put(e, v);
}

static bool put_bstr(zcbor_state_t *e, const char *key, const uint8_t *v, size_t n)
{
	return zcbor_tstr_encode_ptr(e, key, strlen(key)) &&
	       zcbor_bstr_encode_ptr(e, (const char *)v, n);
}

static bool put_tstr(zcbor_state_t *e, const char *key, const char *v, size_t n)
{
	return zcbor_tstr_encode_ptr(e, key, strlen(key)) && zcbor_tstr_encode_ptr(e, v, n);
}

/* the chip's ID as b(8), little-endian as the HERE and the record carry it */
static bool put_chip(zcbor_state_t *e, const char *key, uint64_t chip)
{
	uint8_t b[8];

	for (int i = 0; i < 8; i++) {
		b[i] = (uint8_t)(chip >> (8 * i));
	}
	return put_bstr(e, key, b, 8);
}

static uint64_t chip_of(const uint8_t b[8])
{
	uint64_t chip = 0;

	for (int i = 7; i >= 0; i--) {
		chip = (chip << 8) | b[i];
	}
	return chip;
}

/* a code from the ops: a group's own written as err, Zephyr's returned */
static int answer_rc(zcbor_state_t *e, int rc)
{
	if (STRELKA_RC_IS_OWN(rc)) {
		bool ok = zcbor_tstr_put_lit(e, "err") && zcbor_map_start_encode(e, 2) &&
			  put_u(e, "group", SMP_GROUP_STRELKA) &&
			  put_u(e, "rc", (uint32_t)(rc - 256)) && zcbor_map_end_encode(e, 2);

		return ok ? SMP_ERR_OK : SMP_ERR_TOO_LARGE;
	}
	return rc;
}

#define ENCODED(ok) ((ok) ? SMP_ERR_OK : SMP_ERR_TOO_LARGE)

/* ---- the commands --------------------------------------------------------------------------- */

static int h_info(zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	struct strelka_info i;
	int rc;
	bool ok;

	memset(&i, 0, sizeof(i));
	rc = ops->info(&i, ctx);
	if (rc) {
		return answer_rc(e, rc);
	}
	ok = put_chip(e, "chip", i.chip) && put_u(e, "n", i.n) &&
	     put_u(e, "who", WHO_KIND(i.who) >> 6) &&     /* 0 a Povodok, 2 a Belka */
	     zcbor_tstr_put_lit(e, "fw") && zcbor_list_start_encode(e, 3) &&
	     zcbor_uint32_put(e, i.fw[0]) && zcbor_uint32_put(e, i.fw[1]) &&
	     zcbor_uint32_put(e, i.fw[2]) && zcbor_list_end_encode(e, 3) &&
	     put_bstr(e, "build", i.build, 4) && put_u(e, "sensor", i.sensor) &&
	     zcbor_tstr_put_lit(e, "card") && zcbor_map_start_encode(e, 4) &&
	     put_bool(e, "in", i.card_in) && put_u(e, "mb", i.card_mb) &&
	     put_u(e, "free", i.card_free_mb) && put_u(e, "fs", i.card_fs) &&
	     zcbor_map_end_encode(e, 4) &&
	     zcbor_tstr_put_lit(e, "cell") && zcbor_map_start_encode(e, 2) &&
	     put_u(e, "pct", i.cell_pct) && put_u(e, "mv", i.cell_mv) &&
	     zcbor_map_end_encode(e, 2) &&
	     put_i(e, "temp", i.temp_centi) && put_u(e, "state", i.state) &&
	     put_u(e, "fault", i.fault) && put_u(e, "cal", i.cal_time);
	return ENCODED(ok);
}

static int h_session(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	struct strelka_session s;
	struct field f[] = {
		{ "s", dec_u16, &s.s, false }, { "rate", dec_u16, &s.rate_hz, false }, { "n", dec_u8, &s.n, false },
		{ "acc", dec_u8, &s.acc_g, false }, { "gyro", dec_u16, &s.gyro_dps, false },
		{ "live", dec_u8, &s.live, false }, { "stop", dec_u16, &s.stop_s, false },
		{ "t", dec_u64, &s.t_us, false }, { "tz", dec_i16, &s.tz_min, false }, { "p", dec_u32, &s.p, false },
	};

	memset(&s, 0, sizeof(s));
	s.stop_s = LINK_STOP_S_DEFAULT;
	if (!decode_map(d, FIELDS(f)) || !all_found(f, 6)) {
		return SMP_ERR_INVAL;
	}
	s.p_given = f[9].found;
	if (s.rate_hz == 0 || s.n == 0 || (s.acc_g != 16 && s.acc_g != 32) ||
	    (s.gyro_dps != 2000 && s.gyro_dps != 4000) || !live_code_ok(s.live) || s.stop_s == 0) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->session(&s, ctx));
}

static int h_sessions(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint32_t from = 0;
	struct field f[] = { { "from", dec_u32, &from, false } };
	struct strelka_session_entry list[STRELKA_SESSIONS_A_PAGE];
	unsigned n = 0;
	bool more = false, ok;
	int rc;

	if (!decode_map(d, FIELDS(f))) {
		return SMP_ERR_INVAL;
	}
	rc = ops->sessions(from, list, STRELKA_SESSIONS_A_PAGE, &n, &more, ctx);
	if (rc) {
		return answer_rc(e, rc);
	}
	ok = zcbor_tstr_put_lit(e, "list") && zcbor_list_start_encode(e, STRELKA_SESSIONS_A_PAGE);
	for (unsigned i = 0; ok && i < n; i++) {
		ok = zcbor_map_start_encode(e, 7) && put_u(e, "s", list[i].s) &&
		     put_tstr(e, "file", list[i].file, strnlen(list[i].file, STRELKA_FILE_NAME_MAX)) &&
		     put_u(e, "steps", list[i].steps) && put_u(e, "blocks", list[i].blocks) &&
		     put_u(e, "p0", list[i].p0) && put_u(e, "end", list[i].end) &&
		     put_u(e, "bytes", list[i].bytes) && zcbor_map_end_encode(e, 7);
	}
	ok = ok && zcbor_list_end_encode(e, STRELKA_SESSIONS_A_PAGE) && put_bool(e, "more", more);
	return ENCODED(ok);
}

static bool put_cal(zcbor_state_t *e, const struct strelka_cal *c)
{
	bool ok = zcbor_tstr_put_lit(e, "acc") && zcbor_list_start_encode(e, 6);

	for (unsigned i = 0; ok && i < 6; i++) {
		ok = zcbor_int32_put(e, c->acc[i]);
	}
	ok = ok && zcbor_list_end_encode(e, 6) && zcbor_tstr_put_lit(e, "gscale") &&
	     zcbor_list_start_encode(e, 3);
	for (unsigned i = 0; ok && i < 3; i++) {
		ok = zcbor_uint32_put(e, c->gscale[i]);
	}
	ok = ok && zcbor_list_end_encode(e, 3) && zcbor_tstr_put_lit(e, "gzero") &&
	     zcbor_list_start_encode(e, 3);
	for (unsigned i = 0; ok && i < 3; i++) {
		ok = zcbor_int32_put(e, c->gzero[i]);
	}
	return ok && zcbor_list_end_encode(e, 3) && put_i(e, "temp", c->temp) &&
	       put_u(e, "t", c->t);
}

static int h_cal(uint8_t op, zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops,
		 void *ctx)
{
	struct strelka_cal c;
	int rc;

	memset(&c, 0, sizeof(c));
	if (op == SMP_OP_READ) {
		rc = ops->cal_read(&c, ctx);
		return rc ? answer_rc(e, rc) : ENCODED(put_cal(e, &c));
	}
	{
		struct i16_list acc = { c.acc, 6 }, gzero = { c.gzero, 3 };
		struct u16_list gscale = { c.gscale, 3 };
		struct field f[] = {
			{ "acc", dec_i16_list, &acc, false }, { "gscale", dec_u16_list, &gscale, false },
			{ "gzero", dec_i16_list, &gzero, false }, { "temp", dec_i16, &c.temp, false },
			{ "t", dec_u32, &c.t, false },
		};

		if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f))) {
			return SMP_ERR_INVAL;
		}
	}
	return answer_rc(e, ops->cal_write(&c, ctx));
}

static int h_blink(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint32_t s = 0;
	struct field f[] = { { "s", dec_u32, &s, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f))) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->blink(s, ctx));
}

static int h_card(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint8_t what = 0;
	struct field f[] = { { "do", dec_u8, &what, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || what < 1 || what > 2) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->card(what, ctx));
}

static int h_key(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint8_t pub[32], out[32];
	struct bstr_fixed pub_f = { pub, 32 };
	struct field f[] = { { "pub", dec_bstr_fixed, &pub_f, false } };
	int rc;

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f))) {
		return SMP_ERR_INVAL;
	}
	rc = ops->key(pub, out, ctx);
	return rc ? answer_rc(e, rc) : ENCODED(put_bstr(e, "pub", out, 32));
}

static int h_join(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	struct strelka_join j;
	struct bstr_fixed addr = { j.addr, 4 }, key = { j.key, KEY_BYTES }, nonce = { j.nonce, 13 },
			  mic = { j.mic, 4 };
	struct field f[] = {
		{ "n", dec_u8, &j.n, false }, { "addr", dec_bstr_fixed, &addr, false }, { "ch", dec_u8, &j.ch, false },
		{ "key", dec_bstr_fixed, &key, false }, { "nonce", dec_bstr_fixed, &nonce, false },
		{ "mic", dec_bstr_fixed, &mic, false },
	};

	memset(&j, 0, sizeof(j));
	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || j.n < 1 ||
	    j.n > STRELKA_BELKAS_MAX || j.ch >= STRELKA_SET_CHANNELS) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->join(&j, ctx));
}

static int h_bool(const char *key, zcbor_state_t *d, zcbor_state_t *e,
		  int (*op)(bool, void *), void *ctx)
{
	bool v = false;
	struct field f[] = { { key, dec_bool, &v, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f))) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, op(v, ctx));
}

static int h_live(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint8_t code = 0;
	struct field f[] = { { "live", dec_u8, &code, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || !live_code_ok(code)) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->live(code, ctx));
}

static int h_found(zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	struct strelka_found list[STRELKA_FOUND_MAX];
	unsigned n = 0;
	int rc = ops->found(list, STRELKA_FOUND_MAX, &n, ctx);
	bool ok;

	if (rc) {
		return answer_rc(e, rc);
	}
	ok = zcbor_tstr_put_lit(e, "found") && zcbor_list_start_encode(e, STRELKA_FOUND_MAX);
	for (unsigned i = 0; ok && i < n; i++) {
		ok = zcbor_map_start_encode(e, 3) && put_chip(e, "chip", list[i].chip) &&
		     put_i(e, "rssi", list[i].rssi) && put_u(e, "ago", list[i].ago_s) &&
		     zcbor_map_end_encode(e, 3);
	}
	return ENCODED(ok && zcbor_list_end_encode(e, STRELKA_FOUND_MAX));
}

static int h_take(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint8_t chip[8], n = 0;
	struct bstr_fixed chip_f = { chip, 8 };
	struct field f[] = { { "chip", dec_bstr_fixed, &chip_f, false }, { "n", dec_u8, &n, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || n < 1 || n > STRELKA_BELKAS_MAX) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->take(chip_of(chip), n, ctx));
}

static int h_set(zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	static struct strelka_set set;    /* 63 Belkas' lines: too much for a stack */
	int rc;
	bool ok;

	memset(&set, 0, sizeof(set));
	rc = ops->set(&set, ctx);
	if (rc) {
		return answer_rc(e, rc);
	}
	ok = zcbor_tstr_put_lit(e, "belkas") && zcbor_list_start_encode(e, STRELKA_BELKAS_MAX);
	for (unsigned i = 0; ok && i < set.n && i < STRELKA_BELKAS_MAX; i++) {
		const struct strelka_set_belka *b = &set.belka[i];

		ok = zcbor_map_start_encode(e, 6) && put_u(e, "n", b->n) &&
		     put_chip(e, "chip", b->chip) && put_u(e, "ago", b->ago) && put_u(e, "by", b->by) &&
		     put_u(e, "state", b->state) && put_u(e, "fault", b->fault) &&
		     zcbor_map_end_encode(e, 6);
	}
	ok = ok && zcbor_list_end_encode(e, STRELKA_BELKAS_MAX) &&
	     put_u(e, "povodoks", set.povodoks) && put_u(e, "ch", set.ch) &&
	     put_u(e, "spare", set.spare);
	return ENCODED(ok);
}

static int h_drop(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	uint8_t n = 0;
	struct field f[] = { { "n", dec_u8, &n, false } };

	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || n < 1 || n > STRELKA_BELKAS_MAX) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->drop(n, ctx));
}

/* the MAC owed for the nonce and the secret, as 64/16 and 64/17 take it: the application's public
 * key agreed with the pair begun at the nonce's reading */
static int mac_checked(zcbor_state_t *d, const struct strelka_ops *ops, struct smp_lock *lock,
		       void *ctx, uint8_t new_hash[32], bool *new_given)
{
	uint8_t mac[SMP_MAC_BYTES], hash[32], pub[32], secret[32];
	struct bstr_fixed mac_f = { mac, SMP_MAC_BYTES }, new_f = { new_hash, 32 },
			  pub_f = { pub, 32 };
	struct field f[] = { { "mac", dec_bstr_fixed, &mac_f, false },
			     { "new", dec_bstr_fixed, &new_f, false },
			     { "pub", dec_bstr_fixed, &pub_f, false } };
	int rc;

	if (!decode_map(d, FIELDS(f)) || !f[0].found || !f[2].found) {
		return SMP_ERR_INVAL;
	}
	if (new_given) {
		*new_given = f[1].found;
	}
	rc = ops->password_hash(hash, ctx);
	if (rc) {
		return rc;
	}
	if (!lock->nonce_set || strelka_x25519_agree(pub, secret)) {
		lock->nonce_set = false;
		return SMP_ERR_ACCESS;
	}
	return smp_lock_check(lock, hash, secret, mac) ? SMP_ERR_OK : SMP_ERR_ACCESS;
}

static int h_auth(uint8_t op, zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops,
		  struct smp_lock *lock, void *ctx)
{
	int rc;

	if (lock == NULL || ops->password_hash == NULL) {
		return SMP_ERR_NOTSUP;
	}
	if (op == SMP_OP_READ) {
		uint8_t pub[32];

		/* the nonce, and a fresh pair for this connection — one lives at a time, so a
		 * second reading before the answer spends the first */
		if (strelka_random(lock->nonce, SMP_NONCE_BYTES) != 0 || strelka_x25519_begin(pub)) {
			return SMP_ERR_UNKNOWN;
		}
		lock->nonce_set = true;
		return ENCODED(put_bstr(e, "nonce", lock->nonce, SMP_NONCE_BYTES) &&
			       put_bstr(e, "pub", pub, 32));
	}
	rc = mac_checked(d, ops, lock, ctx, NULL, NULL);
	if (rc == SMP_ERR_OK) {
		lock->open = true;
	}
	return rc;
}

static int h_password(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops,
		      struct smp_lock *lock, void *ctx)
{
	uint8_t new_hash[32];
	bool new_given = false;
	int rc;

	if (lock == NULL || ops->password_hash == NULL || ops->password_set == NULL) {
		return SMP_ERR_NOTSUP;
	}
	rc = mac_checked(d, ops, lock, ctx, new_hash, &new_given);
	if (rc) {
		return rc;
	}
	if (!new_given) {
		return SMP_ERR_INVAL;
	}
	return answer_rc(e, ops->password_set(new_hash, ctx));
}

static int h_table(zcbor_state_t *d, zcbor_state_t *e, const struct strelka_ops *ops, void *ctx)
{
	struct strelka_table_piece p;
	struct zcbor_string csv = { 0 };
	struct field f[] = {
		{ "off", dec_u32, &p.off, false }, { "len", dec_u32, &p.len, false }, { "csv", dec_tstr, &csv, false },
	};

	memset(&p, 0, sizeof(p));
	if (!decode_map(d, FIELDS(f)) || !all_found(FIELDS(f)) || csv.len > STRELKA_TABLE_PIECE_MAX ||
	    p.off > p.len || csv.len > p.len - p.off) {    /* no wrap in 32 bits */
		return SMP_ERR_INVAL;
	}
	p.csv = csv.value;
	p.csv_len = csv.len;
	return answer_rc(e, ops->table(&p, ctx));
}

/* ---- the dispatch --------------------------------------------------------------------------- */

#define NEEDS(fn) do { if ((fn) == NULL) { return SMP_ERR_NOTSUP; } } while (0)

int strelka_mgmt_handle(uint8_t op, uint8_t id, zcbor_state_t *d, zcbor_state_t *e,
			const struct strelka_ops *ops, struct smp_lock *lock, void *ctx)
{
	bool read = op == SMP_OP_READ, write = op == SMP_OP_WRITE;

	if (!read && !write) {
		return SMP_ERR_INVAL;
	}
	switch (id) {
	case CMD_INFO:
		if (!read) { break; }
		NEEDS(ops->info);
		return h_info(e, ops, ctx);
	case CMD_SESSION:
		if (!write) { break; }
		NEEDS(ops->session);
		return h_session(d, e, ops, ctx);
	case CMD_SESSIONS:
		if (!read) { break; }
		NEEDS(ops->sessions);
		return h_sessions(d, e, ops, ctx);
	case CMD_CAL:
		if (read) {
			NEEDS(ops->cal_read);
		} else {
			NEEDS(ops->cal_write);
		}
		return h_cal(op, d, e, ops, ctx);
	case CMD_BLINK:
		if (!write) { break; }
		NEEDS(ops->blink);
		return h_blink(d, e, ops, ctx);
	case CMD_CARD:
		if (!write) { break; }
		NEEDS(ops->card);
		return h_card(d, e, ops, ctx);
	case CMD_OFF:
		if (!write) { break; }
		NEEDS(ops->off);
		return answer_rc(e, ops->off(ctx));
	case CMD_FORGET:
		if (!write) { break; }
		NEEDS(ops->forget);
		return answer_rc(e, ops->forget(ctx));
	case CMD_KEY:
		if (!write) { break; }
		NEEDS(ops->key);
		return h_key(d, e, ops, ctx);
	case CMD_JOIN:
		if (!write) { break; }
		NEEDS(ops->join);
		return h_join(d, e, ops, ctx);
	case CMD_RECORD:
		if (!write) { break; }
		NEEDS(ops->record);
		return h_bool("run", d, e, ops->record, ctx);
	case CMD_LIVE:
		if (!write) { break; }
		NEEDS(ops->live);
		return h_live(d, e, ops, ctx);
	case CMD_SEARCH:
		if (read) {
			NEEDS(ops->found);
			return h_found(e, ops, ctx);
		}
		NEEDS(ops->search);
		return h_bool("run", d, e, ops->search, ctx);
	case CMD_TAKE:
		if (!write) { break; }
		NEEDS(ops->take);
		return h_take(d, e, ops, ctx);
	case CMD_SET:
		if (!read) { break; }
		NEEDS(ops->set);
		return h_set(e, ops, ctx);
	case CMD_DROP:
		if (!write) { break; }
		NEEDS(ops->drop);
		return h_drop(d, e, ops, ctx);
	case CMD_AUTH:
		return h_auth(op, d, e, ops, lock, ctx);
	case CMD_PASSWORD:
		if (!write) { break; }
		return h_password(d, e, ops, lock, ctx);
	case CMD_TABLE:
		if (!write) { break; }
		NEEDS(ops->table);
		return h_table(d, e, ops, ctx);
	default:
		break;
	}
	return SMP_ERR_NOTSUP;
}

size_t strelka_mgmt_frame(const uint8_t *frame, size_t len, uint8_t *out, size_t cap,
			  const struct strelka_ops *ops, struct smp_lock *lock, void *ctx)
{
	struct smp_head h, rsp;
	int rc;

	if (len < SMP_HDR_BYTES || cap < SMP_HDR_BYTES + 4) {
		return 0;
	}
	smp_head_read(frame, &h);
	if (len != SMP_HDR_BYTES + h.len || (h.op != SMP_OP_READ && h.op != SMP_OP_WRITE)) {
		return 0;                        /* not a request: nothing answered */
	}
	if (h.version != SMP_VERSION) {
		return smp_error_frame(out, cap, &h, h.version < SMP_VERSION ? SMP_ERR_TOO_OLD :
								       SMP_ERR_TOO_NEW);
	}
	if (!smp_lock_passes(lock, &h)) {
		return smp_error_frame(out, cap, &h, SMP_ERR_ACCESS);
	}
	if (h.group != SMP_GROUP_STRELKA) {
		return smp_error_frame(out, cap, &h, SMP_ERR_NOTSUP);
	}
	{
		ZCBOR_STATE_D(d, 4, frame + SMP_HDR_BYTES, h.len, 1, 0);
		ZCBOR_STATE_E(e, 4, out + SMP_HDR_BYTES, cap - SMP_HDR_BYTES, 1);

		if (!zcbor_map_start_encode(e, 16)) {
			return 0;
		}
		rc = strelka_mgmt_handle(h.op, h.id, d, e, ops, lock, ctx);
		if (rc != SMP_ERR_OK) {
			return smp_error_frame(out, cap, &h, rc);
		}
		if (!zcbor_map_end_encode(e, 16)) {
			return smp_error_frame(out, cap, &h, SMP_ERR_TOO_LARGE);
		}
		smp_head_answer(&h, (uint16_t)(e->payload - (out + SMP_HDR_BYTES)), &rsp);
	}
	smp_head_write(out, &rsp);
	return SMP_HDR_BYTES + rsp.len;
}
