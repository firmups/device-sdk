#include <stddef.h>
#ifndef FIRMUPS_USE_CRYPTO_CALLBACKS
#include <crypto_aead.h>
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
#include <stdio.h>

#include "crypto.h"

int encrypt_data(struct crypto_context const *ctx, uint8_t const *plaintext,
		 uint16_t plaintext_size, uint8_t *ciphertext, uint16_t ciphertext_size,
		 uint16_t *output_size)
{
	if (plaintext == NULL || ciphertext == NULL ||
	    ciphertext_size < plaintext_size + CRYPTO_TAG_SIZE || output_size == NULL) {
		return -1;
	}
	unsigned long long o_size = 0;

#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
	int ret = ctx->encrypt_data(ciphertext, &o_size, plaintext, plaintext_size, ctx->ad,
				    ctx->adlen, ctx->nonce, ctx->encrypt_data_userdata);
#else
	uint8_t key[CRYPTO_KEY_SIZE] = {0};
	ctx->get_key(key, sizeof(key), ctx->get_key_userdata);
	int ret = crypto_aead_encrypt(ciphertext, &o_size, plaintext, plaintext_size, ctx->ad,
				      ctx->adlen, NULL, ctx->nonce, key);
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
	if (o_size > UINT16_MAX) {
		return -1;
	}
	*output_size = (uint16_t)o_size;
	return ret;
}

int decrypt_data(struct crypto_context const *ctx, uint8_t const *ciphertext,
		 uint16_t ciphertext_size, uint8_t *plaintext, uint16_t plaintext_size,
		 uint16_t *output_size)
{
	if (plaintext == NULL || ciphertext == NULL ||
	    plaintext_size + CRYPTO_TAG_SIZE < ciphertext_size || output_size == NULL) {
		return -1;
	}
	unsigned long long o_size = 0;

#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
	int ret = ctx->decrypt_data(plaintext, &o_size, ciphertext, ciphertext_size, ctx->ad,
				    ctx->adlen, ctx->nonce, ctx->decrypt_data_userdata);
#else
	uint8_t key[CRYPTO_KEY_SIZE] = {0};
	ctx->get_key(key, sizeof(key), ctx->get_key_userdata);
	int ret = crypto_aead_decrypt(plaintext, &o_size, NULL, ciphertext, ciphertext_size,
				      ctx->ad, ctx->adlen, ctx->nonce, key);
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
	if (o_size > UINT16_MAX) {
		return -1;
	}
	*output_size = (uint16_t)o_size;
	return ret;
}
