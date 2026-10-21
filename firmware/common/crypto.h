/* NIC-Strelka — what the firmware asks of the chip's cryptography, CRACEN through PSA on a
 * unit (shared/crypto_psa.c), a software AES for the host tests (tests/ccm_soft.c): the
 * fragments' and the SYNC's tags by AES-CCM with the set's key, the lock's HMAC, the taking's
 * X25519 (COMMANDS.md, The lock; The taking; The frame over ESB). */
#ifndef STRELKA_CRYPTO_H
#define STRELKA_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#define KEY_BYTES    16u
#define TAG_BYTES    4u

/* AES-CCM with a tag of 4 B: `in` of `len` bytes is encrypted into `out` (the two may be one
 * buffer), `aad` of `aad_len` is covered by the tag alone; len 0 gives the tag over the aad,
 * the fragments' and the SYNC's case. 0 on success; the decrypt fails on a wrong tag. */
int strelka_ccm_encrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			uint8_t tag[TAG_BYTES]);
int strelka_ccm_decrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			const uint8_t tag[TAG_BYTES]);

/* the lock: SHA-256 and HMAC-SHA256 (COMMANDS.md, 64/16 and 64/17) */
int strelka_sha256(const uint8_t *in, size_t len, uint8_t out[32]);
int strelka_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *in, size_t len,
			uint8_t out[32]);

/* a fresh X25519 pair, its public key out; then the SHA-256 of the shared secret with the other's
 * public key, 32 B: the lock's secret whole (COMMANDS.md, 64/16), the taking's key its first 16 B
 * (64/8 and 64/9). One pair lives at a time, the latest begun. */
int strelka_x25519_begin(uint8_t pub[32]);
int strelka_x25519_agree(const uint8_t other_pub[32], uint8_t secret[32]);

int strelka_random(uint8_t *out, size_t len);

#endif
