#ifndef FIRMUPS_DEVICE_SDK_CODEC_CRYPTO_H
#define FIRMUPS_DEVICE_SDK_CODEC_CRYPTO_H

#include <stdint.h>

#include <firmups-device-sdk/sdk.h>

#define CRYPTO_NONCE_SIZE 16
#define CRYPTO_TAG_SIZE   16
#define CRYPTO_KEY_SIZE   16

struct crypto_context {
	uint8_t const *ad;
	uint8_t adlen;
	uint8_t nonce[CRYPTO_NONCE_SIZE];
#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
	encrypt_callback const encrypt_data;
	void *encrypt_data_userdata;
	decrypt_callback const decrypt_data;
	void *decrypt_data_userdata;
#else
	key_callback const get_key;
	void *get_key_userdata;
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
};

int encrypt_data(struct crypto_context const *ctx, uint8_t const *plaintext,
		 uint16_t plaintext_size, uint8_t *ciphertext, uint16_t ciphertext_size,
		 uint16_t *output_size);

int decrypt_data(struct crypto_context const *ctx, uint8_t const *ciphertext,
		 uint16_t ciphertext_size, uint8_t *plaintext, uint16_t plaintext_size,
		 uint16_t *output_size);

#endif /* FIRMUPS_DEVICE_SDK_CODEC_CRYPTO_H */
