#include <string.h>
#include "crc32.h"
#include "frame.h"

unsigned live_steps(uint8_t code)
{
	switch (code) {
	case S0_QUAT_50:
	case S0_RAW_50:
		return 1;
	case S0_QUAT_100:
	case S0_RAW_100:
		return 2;
	default:
		return 0;
	}
}

unsigned live_step_bytes(uint8_t code)
{
	switch (code) {
	case S0_QUAT_50:
	case S0_QUAT_100:
		return QUAT_BYTES;
	case S0_RAW_50:
	case S0_RAW_100:
		return STEP_BYTES;
	default:
		return 0;
	}
}

unsigned live_body_bytes(uint8_t code)
{
	return live_steps(code) * live_step_bytes(code);
}

bool live_code_ok(uint8_t code)
{
	return code == S0_LIVE_OFF || live_steps(code) > 0;
}

size_t frame_ram_len(const struct strelka_packet *p)
{
	return 3u + p->length;
}

static size_t head(struct strelka_packet *p, uint8_t who, uint8_t s0, uint32_t period,
		   uint16_t state, size_t body_len)
{
	p->who = who;
	p->length = (uint8_t)(6u + body_len);
	p->s0 = s0;
	p->period = period;
	p->state = state;
	return frame_ram_len(p);
}

size_t frame_sync(struct strelka_packet *p, uint8_t povodok, uint8_t live, uint8_t item,
		  uint32_t period, uint16_t state, const uint8_t sig[SYNC_BODY_BYTES])
{
	memcpy(p->body, sig, SYNC_BODY_BYTES);
	return head(p, WHO(WHO_POVODOK, povodok), S0(live, item), period, state, SYNC_BODY_BYTES);
}

size_t frame_call(struct strelka_packet *p, uint8_t povodok, uint8_t attempt, uint32_t period,
		  uint16_t state, const uint8_t sig[SYNC_BODY_BYTES])
{
	memcpy(p->body, sig, SYNC_BODY_BYTES);
	return head(p, WHO(WHO_POVODOK, povodok), S0(S0_CALL, attempt), period, state,
		    SYNC_BODY_BYTES);
}

size_t frame_live(struct strelka_packet *p, uint8_t belka, uint8_t live, uint8_t item,
		  uint32_t period, uint16_t state, const void *steps)
{
	size_t n = live_body_bytes(live);

	if (n) {
		memcpy(p->body, steps, n);
	}
	return head(p, WHO(WHO_BELKA, belka), S0(live, item), period, state, n);
}

size_t frame_miss(struct strelka_packet *p, uint8_t povodok, uint32_t period, uint16_t state,
		  const uint8_t missed[MISS_BODY_BYTES])
{
	memcpy(p->body, missed, MISS_BODY_BYTES);
	return head(p, WHO(WHO_POVODOK, povodok), S0(S0_MISS, 0), period, state, MISS_BODY_BYTES);
}

size_t frame_pass_begin(struct strelka_packet *p, uint8_t povodok, uint32_t period,
			uint16_t state)
{
	return head(p, WHO(WHO_POVODOK, povodok), S0(S0_PASS, 0), period, state, 0);
}

bool frame_pass_add(struct strelka_packet *p, uint8_t belka, uint8_t s0, uint16_t state,
		    const void *steps)
{
	size_t at = p->length - 6u;
	size_t n = live_body_bytes(S0_HIGH(s0));
	struct strelka_pass_entry e = { belka, s0, state };

	if (at + PASS_EACH_BYTES + n > STRELKA_BODY_MAX) {
		return false;
	}
	memcpy(p->body + at, &e, PASS_EACH_BYTES);
	if (n) {
		memcpy(p->body + at + PASS_EACH_BYTES, steps, n);
	}
	p->length = (uint8_t)(p->length + PASS_EACH_BYTES + n);
	return true;
}

size_t frame_here(struct strelka_packet *p, uint64_t chip, uint32_t period)
{
	for (int i = 0; i < 8; i++) {
		p->body[i] = (uint8_t)(chip >> (8 * i));
	}
	return head(p, WHO(WHO_BELKA, 0), S0(S0_HERE, 0), period, 0, HERE_BODY_BYTES);
}

static void sync_nonce(uint8_t nonce[NONCE_BYTES], const struct strelka_packet *p,
		       uint32_t address)
{
	frag_nonce(nonce, address, p->who, 0, p->period, p->s0);
}

void frame_sync_sign(struct strelka_packet *p, const uint8_t key[KEY_BYTES], uint32_t address)
{
	uint8_t nonce[NONCE_BYTES];

	sync_nonce(nonce, p, address);
	strelka_ccm_encrypt(key, nonce, (const uint8_t *)p, STRELKA_RAM_HEAD_BYTES, NULL, 0, NULL,
			    p->body);
}

bool frame_sync_check(const struct strelka_packet *p, const uint8_t key[KEY_BYTES],
		      uint32_t address)
{
	uint8_t nonce[NONCE_BYTES];

	sync_nonce(nonce, p, address);
	return strelka_ccm_decrypt(key, nonce, (const uint8_t *)p, STRELKA_RAM_HEAD_BYTES, NULL, 0,
				   NULL, p->body) == 0;
}

bool frame_read(const struct strelka_packet *p, size_t ram_len, struct frame *f)
{
	uint8_t high = S0_HIGH(p->s0);
	size_t want;

	if (ram_len < STRELKA_RAM_HEAD_BYTES || ram_len != frame_ram_len(p) || p->length < 6u) {
		return false;
	}
	f->sender = WHO_KIND(p->who);
	f->number = WHO_NUMBER(p->who);
	f->s0 = p->s0;
	f->period = p->period;
	f->state = p->state;
	f->body = p->body;
	f->body_len = p->length - 6u;
	if (f->sender == WHO_POVODOK) {
		if (f->number >= STRELKA_POVODOKS_MAX) {
			return false;
		}
		switch (high) {
		case S0_CALL:
			f->kind = FRAME_CALL;
			want = SYNC_BODY_BYTES;
			break;
		case S0_MISS:
			f->kind = FRAME_MISS;
			want = MISS_BODY_BYTES;
			break;
		case S0_PASS:
			f->kind = FRAME_PASS;
			return true;             /* its items are read one by one */
		default:
			if (!live_code_ok(high)) {
				return false;
			}
			f->kind = FRAME_SYNC;
			want = SYNC_BODY_BYTES;
		}
	} else if (f->sender == WHO_BELKA) {
		if (high == S0_HERE) {
			if (f->number != 0) {
				return false;
			}
			f->kind = FRAME_HERE;
			want = HERE_BODY_BYTES;
		} else {
			if (f->number == 0 || !live_code_ok(high)) {
				return false;
			}
			f->kind = FRAME_LIVE;
			want = live_body_bytes(high);
		}
	} else {
		return false;                    /* the senders 1 and 3 are free */
	}
	return f->body_len == want;
}

bool frame_pass_next(const struct frame *f, size_t *at, struct pass_item *item)
{
	struct strelka_pass_entry e;
	size_t n;

	if (f->kind != FRAME_PASS || *at + PASS_EACH_BYTES > f->body_len) {
		return false;
	}
	memcpy(&e, f->body + *at, PASS_EACH_BYTES);
	if (e.belka == 0 || e.belka > STRELKA_BELKAS_MAX || !live_code_ok(S0_HIGH(e.s0))) {
		return false;
	}
	n = live_body_bytes(S0_HIGH(e.s0));
	if (*at + PASS_EACH_BYTES + n > f->body_len) {
		return false;
	}
	item->belka = e.belka;
	item->s0 = e.s0;
	item->state = e.state;
	item->steps = f->body + *at + PASS_EACH_BYTES;
	item->steps_len = n;
	*at += PASS_EACH_BYTES + n;
	return true;
}

void miss_set(uint8_t missed[MISS_BODY_BYTES], unsigned belka)
{
	if (belka >= 1 && belka <= STRELKA_BELKAS_MAX) {
		missed[belka / 8] |= (uint8_t)(1u << (belka % 8));
	}
}

bool miss_has(const uint8_t missed[MISS_BODY_BYTES], unsigned belka)
{
	return belka >= 1 && belka <= STRELKA_BELKAS_MAX &&
	       (missed[belka / 8] >> (belka % 8)) & 1u;
}

/* SplitMix64 seeded with the chip's ID, its number attempt + 1, the low 16 bits: the seed plus the
 * golden-ratio gamma once a number, stirred by the mixing function's two multiplications and three
 * shifts (Steele, Lea and Flood, OOPSLA 2014). A CRC would not do: it is linear, so two IDs that
 * drew one number at one attempt would draw one number at every attempt. */
uint16_t search_draw(uint64_t chip, uint8_t attempt)
{
	uint64_t z = chip + ((uint64_t)attempt + 1u) * 0x9E3779B97F4A7C15ull;

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return (uint16_t)(z ^ (z >> 31));
}
