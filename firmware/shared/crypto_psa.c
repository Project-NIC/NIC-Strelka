/* NIC-Strelka — common/crypto.h on the chip's CRACEN through PSA, the SDK's nRF Security
 * (COMMANDS.md, The lock; The taking; The frame over ESB). Built against nRF Connect SDK v3.4.0,
 * CRACEN's PSA drivers in; not run on hardware: the kit decides it. */
#include <string.h>
#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include "crypto.h"

#define CCM_ALG PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, TAG_BYTES)
#define INLINE_MAX 64u   /* the most this encrypts or decrypts in one go: the join's 16 B */

static int ready(void)
{
	static bool done;

	if (!done) {
		if (psa_crypto_init() != PSA_SUCCESS) {
			return -1;
		}
		done = true;
	}
	return 0;
}

static psa_status_t aes_key(const uint8_t key[KEY_BYTES], psa_key_usage_t usage,
			    psa_key_id_t *id)
{
	psa_key_attributes_t a = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&a, usage);
	psa_set_key_lifetime(&a, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&a, CCM_ALG);
	psa_set_key_type(&a, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&a, 8 * KEY_BYTES);
	return psa_import_key(&a, key, KEY_BYTES, id);
}

int strelka_ccm_encrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			uint8_t tag[TAG_BYTES])
{
	uint8_t buf[INLINE_MAX + TAG_BYTES];
	size_t out_len = 0;
	psa_key_id_t id;
	psa_status_t st;

	if (ready() || len > INLINE_MAX) {
		return -1;
	}
	if (aes_key(key, PSA_KEY_USAGE_ENCRYPT, &id) != PSA_SUCCESS) {
		return -1;
	}
	st = psa_aead_encrypt(id, CCM_ALG, nonce, 13, aad, aad_len, in, len, buf, sizeof(buf),
			      &out_len);
	psa_destroy_key(id);
	if (st != PSA_SUCCESS || out_len != len + TAG_BYTES) {
		return -1;
	}
	if (len) {
		memcpy(out, buf, len);
	}
	memcpy(tag, buf + len, TAG_BYTES);
	return 0;
}

int strelka_ccm_decrypt(const uint8_t key[KEY_BYTES], const uint8_t nonce[13], const uint8_t *aad,
			size_t aad_len, const uint8_t *in, size_t len, uint8_t *out,
			const uint8_t tag[TAG_BYTES])
{
	uint8_t buf[INLINE_MAX + TAG_BYTES], plain[INLINE_MAX];
	size_t out_len = 0;
	psa_key_id_t id;
	psa_status_t st;

	if (ready() || len > INLINE_MAX) {
		return -1;
	}
	if (len) {
		memcpy(buf, in, len);
	}
	memcpy(buf + len, tag, TAG_BYTES);
	if (aes_key(key, PSA_KEY_USAGE_DECRYPT, &id) != PSA_SUCCESS) {
		return -1;
	}
	st = psa_aead_decrypt(id, CCM_ALG, nonce, 13, aad, aad_len, buf, len + TAG_BYTES, plain,
			      sizeof(plain), &out_len);
	psa_destroy_key(id);
	if (st != PSA_SUCCESS || out_len != len) {
		return -1;
	}
	if (len) {
		memcpy(out, plain, len);
	}
	return 0;
}

int strelka_sha256(const uint8_t *in, size_t len, uint8_t out[32])
{
	size_t n = 0;

	if (ready()) {
		return -1;
	}
	return psa_hash_compute(PSA_ALG_SHA_256, in, len, out, 32, &n) == PSA_SUCCESS && n == 32 ?
	       0 : -1;
}

int strelka_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *in, size_t len,
			uint8_t out[32])
{
	psa_key_attributes_t a = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t id;
	psa_status_t st;
	size_t n = 0;

	if (ready()) {
		return -1;
	}
	psa_set_key_usage_flags(&a, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_lifetime(&a, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&a, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&a, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&a, 8 * key_len);
	if (psa_import_key(&a, key, key_len, &id) != PSA_SUCCESS) {
		return -1;
	}
	st = psa_mac_compute(id, PSA_ALG_HMAC(PSA_ALG_SHA_256), in, len, out, 32, &n);
	psa_destroy_key(id);
	return st == PSA_SUCCESS && n == 32 ? 0 : -1;
}

/* the taking's pair lives from begin to agree: one taking at a time */
static psa_key_id_t x25519_id;
static bool x25519_live;

int strelka_x25519_begin(uint8_t pub[32])
{
	psa_key_attributes_t a = PSA_KEY_ATTRIBUTES_INIT;
	size_t n = 0;

	if (ready()) {
		return -1;
	}
	if (x25519_live) {
		psa_destroy_key(x25519_id);
		x25519_live = false;
	}
	psa_set_key_usage_flags(&a, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&a, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&a, PSA_ALG_ECDH);
	psa_set_key_type(&a, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&a, 255);
	if (psa_generate_key(&a, &x25519_id) != PSA_SUCCESS) {
		return -1;
	}
	x25519_live = true;
	return psa_export_public_key(x25519_id, pub, 32, &n) == PSA_SUCCESS && n == 32 ? 0 : -1;
}

int strelka_x25519_agree(const uint8_t other_pub[32], uint8_t out[32])
{
	uint8_t secret[32], hash[32];
	size_t n = 0;
	psa_status_t st;

	if (!x25519_live) {
		return -1;
	}
	st = psa_raw_key_agreement(PSA_ALG_ECDH, x25519_id, other_pub, 32, secret, sizeof(secret),
				   &n);
	psa_destroy_key(x25519_id);
	x25519_live = false;
	if (st != PSA_SUCCESS || n != 32 || strelka_sha256(secret, 32, hash)) {
		return -1;
	}
	memcpy(out, hash, 32);          /* the SHA-256 of the secret; the taking takes 16 B */
	return 0;
}

int strelka_random(uint8_t *out, size_t len)
{
	if (ready()) {
		return -1;
	}
	return psa_generate_random(out, len) == PSA_SUCCESS ? 0 : -1;
}
