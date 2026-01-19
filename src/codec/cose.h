#ifndef FIRMUPS_DEVICE_SDK_CODEC_COSE_H
#define FIRMUPS_DEVICE_SDK_CODEC_COSE_H

#include <stdint.h>
#include <firmups-device-sdk/error.h>
#include <firmups-device-sdk/sdk.h>

#include "crypto.h"

#define COSE_HEADER_SIZE                                                                           \
	COSE_PROTECTED_HEADER_MAX_SIZE +                                                           \
		4 // 2 byte bstr header + 1 byte unprotected header + 1 byte array overhead
#define COSE_PROTECTED_HEADER_MAX_SIZE 51
#define COSE_AAD_MAX_SIZE              COSE_PROTECTED_HEADER_MAX_SIZE + 13

struct cose_context {
	struct firmups_sdk_api const *api;
	struct crypto_context crypto_ctx;
	uint8_t aad_buffer[COSE_AAD_MAX_SIZE];
};

enum firmups_sdk_error_code cose_init(struct cose_context *ctx, struct firmups_sdk_api const *api);

enum firmups_sdk_error_code cose_encrypt_msg(struct cose_context *ctx, uint32_t device_id,
					     uint16_t opcode, uint8_t const *operation,
					     uint16_t operation_size, uint8_t *message_buffer,
					     uint16_t message_buffer_size, uint16_t *message_size);

enum firmups_sdk_error_code cose_decrypt_msg(struct cose_context *ctx, uint8_t const *message,
					     uint16_t message_size, uint32_t *device_id,
					     uint16_t *opcode, uint8_t *operation_buffer,
					     uint16_t operation_buffer_size,
					     uint16_t *operation_size);
#endif /* FIRMUPS_DEVICE_SDK_CODEC_COSE_H */
