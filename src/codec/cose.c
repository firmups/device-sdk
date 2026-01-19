#include <cbor.h>
#include <stdio.h>
#include <inttypes.h>

#include "cose.h"
#include "cbor_helper.h"

#define RETURN_IF_CBOR_ERROR(expr, ret)                                                            \
	do {                                                                                       \
		CborError err = (expr);                                                            \
		if (err != CborNoError) {                                                          \
			FIRMUPS_LOG_ERROR("CBOR error: %d\n", (err));                              \
			return (ret);                                                              \
		}                                                                                  \
	} while (0)

#define COSE_HEADER_MAP_SIZE              5
#define PROTECTED_HEADER_BSTR_PREFIX_SIZE 2
#define AAD_PREFIX_SIZE                   10
#define AAD_SUFFIX_SIZE                   1

enum encryption_algo {
	ENCRYPTION_ALGO_AES_GCM_128 = 1,
	ENCRYPTION_ALGO_ASCON_AEAD_128 = 35
};

enum cose_header_key {
	COSE_HEADER_ENCRYPTION_ALGO = 1,
	COSE_HEADER_CRITICAL_HEADER_LIST = 2,
	COSE_HEADER_ENCRYPTION_NONCE = 5,
	COSE_HEADER_DEVICE_ID = 8608,
	COSE_HEADER_OPCODE = 8633
};

struct protected_header_parse_context {
	bool device_id_found;
	bool opcode_found;
	bool algo_found;
	bool nonce_found;
	uint32_t device_id;
	uint16_t opcode;
	enum encryption_algo algo;
};

static enum firmups_sdk_error_code
get_definite_length_byte_string_view(CborValue *it, uint8_t const **protected_header,
				     size_t *protected_header_size);
static enum firmups_sdk_error_code
add_byte_string_to_buffer(CborEncoder const *it, struct crypto_context const *crypto_context,
			  uint8_t const *operation, uint16_t operation_size,
			  uint8_t *message_buffer, uint16_t message_buffer_size,
			  uint16_t *message_size);
static enum firmups_sdk_error_code
decode_protected_header(struct protected_header_parse_context *ctx,
			struct crypto_context *crypto_context, uint8_t const *protected_header,
			uint8_t protected_header_size);
static enum firmups_sdk_error_code
parse_header_map_entry(struct protected_header_parse_context *ctx,
		       struct crypto_context *crypto_context, enum cose_header_key key,
		       CborValue *value);
static bool is_known_cose_header(enum cose_header_key key);
static enum firmups_sdk_error_code create_aad_prefix(uint8_t *aad_buffer, size_t aad_buffer_size);
static enum firmups_sdk_error_code create_aad_suffix(uint8_t *suffix_buffer,
						     size_t suffix_buffer_size);
static enum firmups_sdk_error_code create_aad(uint8_t *aad_buffer, size_t aad_buffer_size,
					      uint8_t const *protected_header,
					      size_t protected_header_size, uint8_t *aad_size);
static enum firmups_sdk_error_code
encode_protected_header(struct protected_header_parse_context *ctx,
			struct crypto_context const *crypto_context,
			uint8_t *protected_header_buffer, uint8_t protected_header_buffer_size,
			size_t *protected_header_size);

enum firmups_sdk_error_code cose_init(struct cose_context *ctx, struct firmups_sdk_api const *api)
{
	if (ctx == NULL || api == NULL) {
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}
	struct crypto_context crypto_context = {
		.ad = NULL,
		.adlen = 0,
		.nonce = {0},
#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
		.encrypt_data = api->encrypt_data,
		.encrypt_data_userdata = api->random_bytes_userdata,
		.decrypt_data = api->decrypt_data,
		.decrypt_data_userdata = api->random_bytes_userdata,
#else
		.get_key = api->get_key,
		.get_key_userdata = api->get_key_userdata,
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
	};
	ctx->api = api;
	memcpy(&ctx->crypto_ctx, &crypto_context, sizeof(crypto_context));

	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code cose_encrypt_msg(struct cose_context *ctx, uint32_t device_id,
					     uint16_t opcode, uint8_t const *operation,
					     uint16_t operation_size, uint8_t *message_buffer,
					     uint16_t message_buffer_size, uint16_t *message_size)
{
	enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;
	uint8_t *protected_header_buffer;
	size_t protected_header_size = 0;

	if (ctx == NULL || ctx->api == NULL || operation == NULL || message_buffer == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to cose_encrypt_msg\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	ctx->crypto_ctx.ad = NULL;
	ctx->crypto_ctx.adlen = 0;
	memset(&ctx->crypto_ctx.nonce, 0, sizeof(ctx->crypto_ctx.nonce));

	ret_code = ctx->api->get_random_bytes(ctx->crypto_ctx.nonce, sizeof(ctx->crypto_ctx.nonce),
					      ctx->api->random_bytes_userdata);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		FIRMUPS_LOG_ERROR("Failed to get random bytes for nonce\n");
		return ret_code;
	}

	struct protected_header_parse_context header_ctx = {
		.device_id_found = true,
		.opcode_found = true,
		.algo_found = false,
		.nonce_found = true,
		.device_id = device_id,
		.opcode = opcode,
	};

	// Construct protected header into AAD buffer
	ret_code = create_aad_prefix(ctx->aad_buffer, sizeof(ctx->aad_buffer));
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	protected_header_buffer =
		ctx->aad_buffer + AAD_PREFIX_SIZE + PROTECTED_HEADER_BSTR_PREFIX_SIZE;
	ret_code = encode_protected_header(&header_ctx, &ctx->crypto_ctx, protected_header_buffer,
					   COSE_PROTECTED_HEADER_MAX_SIZE, &protected_header_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	};
	// Protected header CBOR byte string prefix will always be 2 bytes [23-255bytes payload]
	ret_code = cbor_helper_write_bstr_header(ctx->aad_buffer + AAD_PREFIX_SIZE,
						 PROTECTED_HEADER_BSTR_PREFIX_SIZE,
						 protected_header_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	uint8_t *protected_header_end = protected_header_buffer + protected_header_size;
	ret_code =
		create_aad_suffix(protected_header_end, sizeof(ctx->aad_buffer) - AAD_PREFIX_SIZE -
								protected_header_size);
	uint8_t *aad_end = protected_header_end + AAD_SUFFIX_SIZE;
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ctx->crypto_ctx.ad = ctx->aad_buffer;
	ctx->crypto_ctx.adlen = (size_t)(aad_end - ctx->aad_buffer);

	// Create COSE_Encrypt0 message
	CborEncoder encoder, msg_array_encoder, unprotected_header_map_encoder;
	cbor_encoder_init(&encoder, message_buffer, message_buffer_size, 0);
	RETURN_IF_CBOR_ERROR(cbor_encoder_create_array(&encoder, &msg_array_encoder, 3),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// Protected header (see above)
	RETURN_IF_CBOR_ERROR(cbor_encode_byte_string(&msg_array_encoder, protected_header_buffer,
						     protected_header_size),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// Unprotected header (empty)
	RETURN_IF_CBOR_ERROR(
		cbor_encoder_create_map(&msg_array_encoder, &unprotected_header_map_encoder, 0),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(
		cbor_encoder_close_container(&msg_array_encoder, &unprotected_header_map_encoder),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);

	ret_code = add_byte_string_to_buffer(&msg_array_encoder, &ctx->crypto_ctx, operation,
					     operation_size, message_buffer, message_buffer_size,
					     message_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	// Before in buffer hack was applied:
	// RETURN_IF_CBOR_ERROR(encrypt_data(&crypto_context, operation, operation_size,
	// crypto_buffer, 				  sizeof(crypto_buffer), &o_size),
	// FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// // Ciphertext
	// RETURN_IF_CBOR_ERROR(cbor_encode_byte_string(&msg_array_encoder, crypto_buffer, o_size),
	// 		     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// RETURN_IF_CBOR_ERROR(cbor_encoder_close_container(&encoder, &msg_array_encoder),
	// 		     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// if (message_size != NULL) {
	// 	*message_size = cbor_encoder_get_buffer_size(&encoder, message_buffer);
	// }

	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code cose_decrypt_msg(struct cose_context *ctx, uint8_t const *message,
					     uint16_t message_size, uint32_t *device_id,
					     uint16_t *opcode, uint8_t *operation_buffer,
					     uint16_t operation_buffer_size,
					     uint16_t *operation_size)
{
	enum firmups_sdk_error_code ret = FIRMUPS_SDK_ERROR_NONE;
	uint8_t const *protected_header_buffer;
	size_t protected_header_size;
	uint8_t const *encrypted_operation_buffer;
	size_t encrypted_operation_size;

	if (ctx == NULL || ctx->api == NULL || message == NULL || device_id == NULL ||
	    opcode == NULL || operation_buffer == NULL) {
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}
	FIRMUPS_LOG_DEBUG("COSE decrypting message of size %u\n", message_size);
	for (size_t i = 0; i < message_size; i++) {
		FIRMUPS_LOG_DEBUG("%02X ", message[i]); // prints each byte as two-digit hex
	}
	FIRMUPS_LOG_DEBUG("\n");

	ctx->crypto_ctx.ad = NULL;
	ctx->crypto_ctx.adlen = 0;
	memset(&ctx->crypto_ctx.nonce, 0, sizeof(ctx->crypto_ctx.nonce));

	CborParser message_parser;
	CborValue cbor_value;
	cbor_parser_init(message, message_size, 0, &message_parser, &cbor_value);

	size_t array_size = 0;
	if (!cbor_value_is_array(&cbor_value)) {
		FIRMUPS_LOG_WARNING("COSE message is not an array\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}
	cbor_value_get_array_length(&cbor_value, &array_size);
	if (array_size != 3) {
		FIRMUPS_LOG_WARNING("Unexpected COSE message array size: %zu\n", array_size);
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	CborValue array_it;
	cbor_value_enter_container(&cbor_value, &array_it);
	ret = get_definite_length_byte_string_view(&array_it, &protected_header_buffer,
						   &protected_header_size);
	if (ret != FIRMUPS_SDK_ERROR_NONE) {
		return ret;
	}

	ret = create_aad(ctx->aad_buffer, sizeof(ctx->aad_buffer), protected_header_buffer,
			 protected_header_size, &ctx->crypto_ctx.adlen);
	if (ret != FIRMUPS_SDK_ERROR_NONE) {
		return ret;
	}
	ctx->crypto_ctx.ad = ctx->aad_buffer;

	if (!cbor_value_is_map(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected map for unprotected header\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}
	cbor_value_advance(&array_it);

	if (!cbor_value_is_byte_string(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected byte string for encrypted operation\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}
	ret = get_definite_length_byte_string_view(&array_it, &encrypted_operation_buffer,
						   &encrypted_operation_size);
	if (ret != FIRMUPS_SDK_ERROR_NONE) {
		return ret;
	}
	if (!cbor_value_at_end(&array_it)) {
		FIRMUPS_LOG_WARNING("Extra data at end of COSE message array\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	// Parse protected header
	struct protected_header_parse_context parse_ctx = {0};
	ret = decode_protected_header(&parse_ctx, &ctx->crypto_ctx, protected_header_buffer,
				      protected_header_size);
	if (ret != FIRMUPS_SDK_ERROR_NONE) {
		return ret;
	}
	*device_id = parse_ctx.device_id;
	*opcode = parse_ctx.opcode;
	return decrypt_data(&ctx->crypto_ctx, encrypted_operation_buffer, encrypted_operation_size,
			    operation_buffer, operation_buffer_size, operation_size);
}

// Private functions
static enum firmups_sdk_error_code
add_byte_string_to_buffer(CborEncoder const *it, struct crypto_context const *crypto_context,
			  uint8_t const *operation, uint16_t operation_size,
			  uint8_t *message_buffer, uint16_t message_buffer_size,
			  uint16_t *message_size)
{
	// ToDO: Improve this function to use officital CBOR APIs
	enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;
	const uint32_t cipher_len = (uint32_t)operation_size + CRYPTO_TAG_SIZE;
	const size_t prefix_size = cbor_encoder_get_buffer_size(it, message_buffer);
	uint16_t crypto_size = 0;
	size_t cipher_hdr_size = cbor_helper_bstr_header_size(cipher_len);
	const size_t required_total = prefix_size + cipher_hdr_size + cipher_len;

	if (required_total > message_buffer_size) {
		FIRMUPS_LOG_ERROR("message_buffer too small: need %zu bytes\n", required_total);
		return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
	}
	ret_code = cbor_helper_write_bstr_header(message_buffer + prefix_size, cipher_hdr_size,
						 cipher_len);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	uint8_t *cipher_dst = message_buffer + prefix_size + cipher_hdr_size;
	RETURN_IF_CBOR_ERROR(
		encrypt_data(crypto_context, operation, operation_size, cipher_dst,
			     (uint16_t)(message_buffer_size - (cipher_dst - message_buffer)),
			     &crypto_size),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	if (crypto_size != cipher_len) {
		FIRMUPS_LOG_ERROR("Ciphertext size mismatch (got %u, expected %u)\n",
				  (unsigned)crypto_size, (unsigned)cipher_len);
		return FIRMUPS_SDK_ERROR_MESSAGE_ENCODING;
	}
	*message_size = required_total;
	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code
encode_protected_header(struct protected_header_parse_context *ctx,
			struct crypto_context const *crypto_context,
			uint8_t *protected_header_buffer, uint8_t protected_header_buffer_size,
			size_t *protected_header_size)
{
	CborEncoder header_encoder, protected_header_map_encoder, crit_array_encoder;
	cbor_encoder_init(&header_encoder, protected_header_buffer, protected_header_buffer_size,
			  0);
	RETURN_IF_CBOR_ERROR(cbor_encoder_create_map(&header_encoder, &protected_header_map_encoder,
						     COSE_HEADER_MAP_SIZE),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(
		cbor_encode_int(&protected_header_map_encoder, COSE_HEADER_ENCRYPTION_ALGO),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	ctx->algo_found = true;
#ifdef FIRMUPS_USE_AES
	RETURN_IF_CBOR_ERROR(
		cbor_encode_int(&protected_header_map_encoder, ENCRYPTION_ALGO_AES_GCM_128),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	ctx->algo = ENCRYPTION_ALGO_AES_GCM_128;
#else
	RETURN_IF_CBOR_ERROR(
		cbor_encode_int(&protected_header_map_encoder, ENCRYPTION_ALGO_ASCON_AEAD_128),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	ctx->algo = ENCRYPTION_ALGO_ASCON_AEAD_128;
#endif
	RETURN_IF_CBOR_ERROR(
		cbor_encode_int(&protected_header_map_encoder, COSE_HEADER_ENCRYPTION_NONCE),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_byte_string(&protected_header_map_encoder,
						     crypto_context->nonce,
						     sizeof(crypto_context->nonce)),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);

	// Encode crit section
	RETURN_IF_CBOR_ERROR(
		cbor_encode_int(&protected_header_map_encoder, COSE_HEADER_CRITICAL_HEADER_LIST),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	cbor_encoder_create_array(&protected_header_map_encoder, &crit_array_encoder, 2);
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&crit_array_encoder, COSE_HEADER_DEVICE_ID),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&crit_array_encoder, COSE_HEADER_OPCODE),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(
		cbor_encoder_close_container(&protected_header_map_encoder, &crit_array_encoder),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// Encode device ID
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&protected_header_map_encoder, COSE_HEADER_DEVICE_ID),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&protected_header_map_encoder, ctx->device_id),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	// Encode opcode
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&protected_header_map_encoder, COSE_HEADER_OPCODE),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_int(&protected_header_map_encoder, ctx->opcode),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(
		cbor_encoder_close_container(&header_encoder, &protected_header_map_encoder),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	*protected_header_size =
		cbor_encoder_get_buffer_size(&header_encoder, protected_header_buffer);
	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code
get_definite_length_byte_string_view(CborValue *it, uint8_t const **protected_header,
				     size_t *protected_header_size)
{
	size_t total_len = 0;
	CborError err;

	if (!it || !protected_header) {
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}
	if (!cbor_value_is_byte_string(it)) {
		FIRMUPS_LOG_WARNING("Expected byte string\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	err = cbor_value_calculate_string_length(it, &total_len);
	if (err != CborNoError) {
		FIRMUPS_LOG_WARNING("Byte string is not definite-length\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	err = cbor_value_begin_string_iteration(it);
	if (err != CborNoError) {
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	err = cbor_value_get_byte_string_chunk(it, protected_header, protected_header_size, it);
	if (err != CborNoError) {
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	bool at_end = cbor_value_string_iteration_at_end(it);
	if (!at_end || *protected_header_size != total_len) {
		FIRMUPS_LOG_WARNING("String iteration not at end or chunk length mismatch\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	err = cbor_value_finish_string_iteration(it);
	if (err != CborNoError) {
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code
decode_protected_header(struct protected_header_parse_context *ctx,
			struct crypto_context *crypto_context, uint8_t const *protected_header,
			uint8_t protected_header_size)
{
	CborValue cbor_value;
	CborParser protected_header_parser;
	size_t array_size = 0;
	enum firmups_sdk_error_code ret;
	ctx->device_id_found = false;
	ctx->opcode_found = false;
	ctx->algo_found = false;
	ctx->nonce_found = false;

	cbor_parser_init(protected_header, protected_header_size, 0, &protected_header_parser,
			 &cbor_value);
	if (!cbor_value_is_map(&cbor_value)) {
		FIRMUPS_LOG_WARNING("Expected map for protected header\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}
	cbor_value_get_map_length(&cbor_value, &array_size);
	if (array_size != 5) {
		FIRMUPS_LOG_WARNING("Unexpected protected header map size: %zu\n", array_size);
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}
	CborValue map_it;
	uint64_t map_key;
	RETURN_IF_CBOR_ERROR(cbor_value_enter_container(&cbor_value, &map_it),
			     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
	while (!cbor_value_at_end(&map_it)) {
		if (!cbor_value_is_unsigned_integer(&map_it)) {
			FIRMUPS_LOG_WARNING("Expected unsigned integer for header map key\n");
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&map_it, &map_key),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		FIRMUPS_LOG_DEBUG("Parsing header map entry... key: %lu\n", map_key);
		RETURN_IF_CBOR_ERROR(cbor_value_advance(&map_it),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		ret = parse_header_map_entry(ctx, crypto_context, (int)map_key, &map_it);
		if (ret != 0) {
			return ret;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_advance(&map_it),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
	}

	// ToDO: Better sentinel values
	if (ctx->device_id_found == false || ctx->opcode_found == false ||
	    ctx->algo_found == false || ctx->nonce_found == false) {
		FIRMUPS_LOG_WARNING("Missing required protected header fields\n");
		return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
	}

	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code
parse_header_map_entry(struct protected_header_parse_context *ctx,
		       struct crypto_context *crypto_context, enum cose_header_key key,
		       CborValue *value)
{
	uint64_t value_buffer;
	CborValue array_it;
	switch (key) {
	case COSE_HEADER_ENCRYPTION_ALGO:
		if (!cbor_value_is_unsigned_integer(value)) {
			FIRMUPS_LOG_WARNING("Expected unsigned integer for encryption algorithm\n");
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(value, &value_buffer),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
#ifdef FIRMUPS_USE_AES
		if (value_buffer != ENCRYPTION_ALGO_AES_GCM_128) {
			FIRMUPS_LOG_WARNING("Unsupported encryption algorithm %lu expected "
					    "AES GCM 128\n",
					    value_buffer);
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
#else
		if (value_buffer != ENCRYPTION_ALGO_ASCON_AEAD_128) {
			FIRMUPS_LOG_WARNING("Unsupported encryption algorithm %lu expected "
					    "ASCON AEAD 128\n",
					    value_buffer);
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
#endif // FIRMUPS_USE_ASCON
		ctx->algo = (enum encryption_algo)value_buffer;
		ctx->algo_found = true;
		break;
	case COSE_HEADER_CRITICAL_HEADER_LIST:
		if (!cbor_value_is_array(value)) {
			FIRMUPS_LOG_WARNING("Expected array for critical header list\n");
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_enter_container(value, &array_it),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		while (!cbor_value_at_end(&array_it)) {
			if (!cbor_value_is_unsigned_integer(&array_it)) {
				FIRMUPS_LOG_WARNING("Expected unsigned integer for critical header "
						    "list entry\n");
				return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
			}
			RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &value_buffer),
					     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
			FIRMUPS_LOG_DEBUG("Critical header entry: %lu\n", value_buffer);
			if (!is_known_cose_header((enum cose_header_key)value_buffer)) {
				FIRMUPS_LOG_WARNING("Unknown critical header: %lu\n", value_buffer);
				return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
			}
			RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it),
					     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		}
		break;
	case COSE_HEADER_ENCRYPTION_NONCE:
		if (!cbor_value_is_byte_string(value)) {
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		value_buffer = sizeof(crypto_context->nonce);
		FIRMUPS_LOG_DEBUG("Copying nonce of size %zu\n", value_buffer);
		RETURN_IF_CBOR_ERROR(cbor_value_copy_byte_string(value, crypto_context->nonce,
								 (size_t *)&value_buffer, NULL),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		if (value_buffer > sizeof(crypto_context->nonce)) {
			FIRMUPS_LOG_WARNING("Nonce size %lu exceeds expected size %zu\n",
					    value_buffer, sizeof(crypto_context->nonce));
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		ctx->nonce_found = true;
		break;
	case COSE_HEADER_DEVICE_ID:
		if (!cbor_value_is_unsigned_integer(value)) {
			FIRMUPS_LOG_WARNING("Expected unsigned integer for device ID\n");
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(value, &value_buffer),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		if (value_buffer > UINT32_MAX) {
			FIRMUPS_LOG_WARNING("Device ID %lu exceeds maximum value %u\n",
					    value_buffer, UINT32_MAX);
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		ctx->device_id = (uint32_t)value_buffer;
		ctx->device_id_found = true;
		break;
	case COSE_HEADER_OPCODE:
		if (!cbor_value_is_unsigned_integer(value)) {
			FIRMUPS_LOG_WARNING("Expected unsigned integer for opcode\n");
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(value, &value_buffer),
				     FIRMUPS_SDK_ERROR_MESSAGE_PARSING);
		if (value_buffer > UINT16_MAX) {
			FIRMUPS_LOG_WARNING("Opcode %" PRIu64 " exceeds maximum value %u\n",
					    value_buffer, UINT16_MAX);
			return FIRMUPS_SDK_ERROR_MESSAGE_PARSING;
		}
		ctx->opcode = (uint16_t)value_buffer;
		ctx->opcode_found = true;
		break;
	default:
		FIRMUPS_LOG_WARNING("Unknown protected header key: %u\n", key);
	}
	return 0;
}

static bool is_known_cose_header(enum cose_header_key key)
{
	switch (key) {
	case COSE_HEADER_ENCRYPTION_ALGO:
	case COSE_HEADER_CRITICAL_HEADER_LIST:
	case COSE_HEADER_ENCRYPTION_NONCE:
	case COSE_HEADER_DEVICE_ID:
	case COSE_HEADER_OPCODE:
		return true;
	default:
		return false;
	}
}

static enum firmups_sdk_error_code create_aad_prefix(uint8_t *aad_buffer, size_t aad_buffer_size)
{
	CborEncoder enc_e, enc_arr;
	cbor_encoder_init(&enc_e, aad_buffer, aad_buffer_size, 0);
	RETURN_IF_CBOR_ERROR(cbor_encoder_create_array(&enc_e, &enc_arr, 3),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_text_stringz(&enc_arr, "Encrypt0"),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code create_aad_suffix(uint8_t *suffix_buffer,
						     size_t suffix_buffer_size)
{
	CborEncoder enc_arr;
	cbor_encoder_init(&enc_arr, suffix_buffer, suffix_buffer_size, 0);
	const uint8_t empty[] = {};
	RETURN_IF_CBOR_ERROR(cbor_encode_byte_string(&enc_arr, empty, sizeof(empty)),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code create_aad(uint8_t *aad_buffer, size_t aad_buffer_size,
					      uint8_t const *protected_header,
					      size_t protected_header_size, uint8_t *aad_size)
{
	CborEncoder enc_e, enc_arr;
	cbor_encoder_init(&enc_e, aad_buffer, aad_buffer_size, 0);
	RETURN_IF_CBOR_ERROR(cbor_encoder_create_array(&enc_e, &enc_arr, 3),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encode_text_stringz(&enc_arr, "Encrypt0"),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(
		cbor_encode_byte_string(&enc_arr, protected_header, protected_header_size),
		FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	const uint8_t empty[] = {};
	RETURN_IF_CBOR_ERROR(cbor_encode_byte_string(&enc_arr, empty, sizeof(empty)),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);
	RETURN_IF_CBOR_ERROR(cbor_encoder_close_container(&enc_e, &enc_arr),
			     FIRMUPS_SDK_ERROR_MESSAGE_ENCODING);

	*aad_size = cbor_encoder_get_buffer_size(&enc_e, aad_buffer);
	return FIRMUPS_SDK_ERROR_NONE;
}
