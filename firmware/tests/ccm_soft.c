/* NIC-Strelka — the host's stand-in for the chip's cryptography: AES-128 in software (FIPS 197)
 * and AES-CCM over it (NIST SP 800-38C; RFC 3610) with a nonce of 13 B and a tag of 4 B, as
 * common/crypto.h asks. For the tests alone; a unit uses CRACEN through PSA. The tags it makes
 * are held against Python's cryptography library by tools/test_firmware.py. The lock's hashes, the
 * random numbers and X25519 are tests/sha256_soft.c's and tests/x25519_soft.c's. */
#include <string.h>
#include "crypto.h"

static const uint8_t sbox[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static uint8_t xtime(uint8_t x)
{
	return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0));
}

static void expand(const uint8_t key[16], uint8_t rk[176])
{
	uint8_t rcon = 1;

	memcpy(rk, key, 16);
	for (int i = 16; i < 176; i += 4) {
		uint8_t t[4];

		memcpy(t, rk + i - 4, 4);
		if (i % 16 == 0) {
			uint8_t u = t[0];

			t[0] = sbox[t[1]] ^ rcon;
			t[1] = sbox[t[2]];
			t[2] = sbox[t[3]];
			t[3] = sbox[u];
			rcon = xtime(rcon);
		}
		for (int j = 0; j < 4; j++) {
			rk[i + j] = rk[i - 16 + j] ^ t[j];
		}
	}
}

static void encrypt_block(const uint8_t rk[176], const uint8_t in[16], uint8_t out[16])
{
	uint8_t s[16];

	for (int i = 0; i < 16; i++) {
		s[i] = in[i] ^ rk[i];
	}
	for (int round = 1; round <= 10; round++) {
		uint8_t t[16];

		for (int i = 0; i < 16; i++) {
			s[i] = sbox[s[i]];
		}
		/* ShiftRows: byte i of row r (i = 4c + r) moves left by r columns */
		for (int c = 0; c < 4; c++) {
			for (int r = 0; r < 4; r++) {
				t[4 * c + r] = s[4 * ((c + r) % 4) + r];
			}
		}
		if (round < 10) {
			for (int c = 0; c < 4; c++) {
				uint8_t *col = t + 4 * c;
				uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];

				col[0] = xtime(a0) ^ (xtime(a1) ^ a1) ^ a2 ^ a3;
				col[1] = a0 ^ xtime(a1) ^ (xtime(a2) ^ a2) ^ a3;
				col[2] = a0 ^ a1 ^ xtime(a2) ^ (xtime(a3) ^ a3);
				col[3] = (xtime(a0) ^ a0) ^ a1 ^ a2 ^ xtime(a3);
			}
		}
		for (int i = 0; i < 16; i++) {
			s[i] = t[i] ^ rk[16 * round + i];
		}
	}
	memcpy(out, s, 16);
}

/* CCM: L = 2 (a nonce of 13), M = 4 */
static void ccm(const uint8_t key[16], const uint8_t nonce[13], const uint8_t *aad,
		size_t aad_len, const uint8_t *in, size_t len, uint8_t *out, uint8_t tag[4],
		int decrypt)
{
	uint8_t rk[176], b[16], x[16], a[16], s[16];
	size_t at;

	expand(key, rk);
	/* the MAC's first block: the flags, the nonce, the payload's length */
	b[0] = (uint8_t)((aad_len ? 0x40 : 0) | (((4 - 2) / 2) << 3) | (2 - 1));
	memcpy(b + 1, nonce, 13);
	b[14] = (uint8_t)(len >> 8);
	b[15] = (uint8_t)len;
	encrypt_block(rk, b, x);
	if (aad_len) {
		memset(b, 0, 16);
		b[0] = (uint8_t)(aad_len >> 8);
		b[1] = (uint8_t)aad_len;
		at = aad_len < 14 ? aad_len : 14;
		memcpy(b + 2, aad, at);
		for (int i = 0; i < 16; i++) {
			b[i] ^= x[i];
		}
		encrypt_block(rk, b, x);
		while (at < aad_len) {
			size_t n = aad_len - at < 16 ? aad_len - at : 16;

			memset(b, 0, 16);
			memcpy(b, aad + at, n);
			for (int i = 0; i < 16; i++) {
				b[i] ^= x[i];
			}
			encrypt_block(rk, b, x);
			at += n;
		}
	}
	/* the counter blocks: the payload under A_1 …, the tag under A_0 */
	memset(a, 0, 16);
	a[0] = 2 - 1;
	memcpy(a + 1, nonce, 13);
	for (at = 0; at < len; at += 16) {
		size_t n = len - at < 16 ? len - at : 16;
		uint16_t ctr = (uint16_t)(at / 16 + 1);

		a[14] = (uint8_t)(ctr >> 8);
		a[15] = (uint8_t)ctr;
		encrypt_block(rk, a, s);
		memset(b, 0, 16);
		if (decrypt) {
			for (size_t i = 0; i < n; i++) {
				out[at + i] = in[at + i] ^ s[i];
			}
			memcpy(b, out + at, n);
		} else {
			memcpy(b, in + at, n);
			for (size_t i = 0; i < n; i++) {
				out[at + i] = in[at + i] ^ s[i];
			}
		}
		for (int i = 0; i < 16; i++) {
			b[i] ^= x[i];
		}
		encrypt_block(rk, b, x);
	}
	a[14] = a[15] = 0;
	encrypt_block(rk, a, s);
	for (int i = 0; i < 4; i++) {
		tag[i] = x[i] ^ s[i];
	}
}

int strelka_ccm_encrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			uint8_t tag[TAG_BYTES])
{
	ccm(key, nonce, aad, aad_len, in, len, out, tag, 0);
	return 0;
}

int strelka_ccm_decrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			const uint8_t tag[TAG_BYTES])
{
	uint8_t mine[TAG_BYTES];
	uint8_t diff = 0;

	ccm(key, nonce, aad, aad_len, in, len, out, mine, 1);
	for (unsigned i = 0; i < TAG_BYTES; i++) {
		diff |= mine[i] ^ tag[i];
	}
	return diff ? -1 : 0;
}

/* FIPS 197, appendix C.1: one block, for the test's own sanity */
int ccm_soft_self_test(void)
{
	static const uint8_t key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
					 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	static const uint8_t in[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
					0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
	static const uint8_t want[16] = { 0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 0xd8,
					  0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a };
	uint8_t rk[176], out[16];

	expand(key, rk);
	encrypt_block(rk, in, out);
	return memcmp(out, want, 16) == 0 ? 0 : -1;
}
