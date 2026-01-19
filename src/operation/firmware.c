#include "internal.h"
#include <cbor.h>
#include "../operation.h"

enum firmups_sdk_error_code
operation_create_get_firmware_request(uint32_t firmware, uint32_t offset, uint32_t length,
				      uint8_t *operation_buffer, uint16_t operation_buffer_size,
				      uint16_t *opcode, uint16_t *output_size)
{
	CborError err = CborNoError;
	CborEncoder operation_encoder, array_encoder;
	cbor_encoder_init(&operation_encoder, operation_buffer, operation_buffer_size, 0);
	err = cbor_encoder_create_array(&operation_encoder, &array_encoder, 3);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, firmware);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, offset);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, length);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encoder_close_container(&operation_encoder, &array_encoder);
	if (err != CborNoError) {
		goto cleanup;
	}
	*output_size = cbor_encoder_get_buffer_size(&operation_encoder, operation_buffer);
	*opcode = OPERATION_ID_GET_FIRMWARE_REQUEST;
cleanup:
	if (err != CborNoError) {
		FIRMUPS_LOG_ERROR("CBOR encoding error: %d\n", err);
		return FIRMUPS_SDK_ERROR_OPERATION_ENCODING;
	}
	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code operation_parse_get_firmware_response(
	uint16_t opcode, const uint8_t *operation_buffer, uint16_t operation_buffer_size,
	uint32_t *firmware, uint32_t *offset, uint8_t *firmware_data,
	uint16_t firmware_data_buffer_size, uint16_t *firmware_data_size)
{
	if (opcode != OPERATION_ID_GET_FIRMWARE_RESPONSE) {
		FIRMUPS_LOG_WARNING("Unexpected opcode: %u\n", opcode);
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	}

	CborParser parser;
	CborValue it, array_it;
	RETURN_IF_CBOR_ERROR(
		cbor_parser_init(operation_buffer, operation_buffer_size, 0, &parser, &it),
		FIRMUPS_SDK_ERROR_OPERATION_PARSING);

	if (!cbor_value_is_array(&it)) {
		FIRMUPS_LOG_WARNING("Expected array CBOR value\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}

	RETURN_IF_CBOR_ERROR(cbor_value_enter_container(&it, &array_it),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	// Firmware version
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for firmware version\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &firmware_version),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*firmware = (uint32_t)firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	// Desired firmware version
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for desired firmware version\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t offset_value;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &offset_value),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*offset = (uint32_t)offset_value;
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	uint64_t length;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &length),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	if (length > firmware_data_buffer_size) {
		FIRMUPS_LOG_WARNING("Firmware data buffer too small\n");
		return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
	}
	// Firmware data
	if (!cbor_value_is_byte_string(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected byte string for firmware data\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}

	size_t buf_len = firmware_data_buffer_size;
	RETURN_IF_CBOR_ERROR(cbor_value_copy_byte_string(&array_it, firmware_data, &buf_len, NULL),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	if (buf_len != length) {
		FIRMUPS_LOG_WARNING("Firmware data length mismatch\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	// memcpy(firmware_data, bufferptr, buf_len);
	*firmware_data_size = (uint16_t)buf_len;

	return FIRMUPS_SDK_ERROR_NONE;
}
