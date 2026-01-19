#include "internal.h"
#include <cbor.h>
#include "../operation.h"

enum firmups_sdk_error_code operation_create_get_device_info_request(uint32_t device_id,
								     uint8_t *operation_buffer,
								     uint16_t operation_buffer_size,
								     uint16_t *opcode,
								     uint16_t *output_size)
{
	CborError err = CborNoError;
	CborEncoder operation_encoder, array_encoder;
	cbor_encoder_init(&operation_encoder, operation_buffer, operation_buffer_size, 0);
	err = cbor_encoder_create_array(&operation_encoder, &array_encoder, 1);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, device_id);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encoder_close_container(&operation_encoder, &array_encoder);
	if (err != CborNoError) {
		goto cleanup;
	}
	*output_size = cbor_encoder_get_buffer_size(&operation_encoder, operation_buffer);
	*opcode = OPERATION_ID_GET_DEVICE_INFO_REQUEST;
cleanup:
	if (err != CborNoError) {
		FIRMUPS_LOG_ERROR("CBOR encoding error: %d\n", err);
		return FIRMUPS_SDK_ERROR_OPERATION_ENCODING;
	}
	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code
operation_parse_get_device_info_response(uint16_t opcode, const uint8_t *operation_buffer,
					 uint16_t operation_buffer_size, uint32_t *firmware,
					 uint32_t *desired_firmware, uint8_t *status)
{
	if (opcode != OPERATION_ID_GET_DEVICE_INFO_RESPONSE) {
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
	if (cbor_value_is_null(&array_it)) {
		*firmware = 0;
	} else if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer or NULL for firmware version\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	} else {
		uint64_t firmware_version;
		RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &firmware_version),
				     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		*firmware = (uint32_t)firmware_version;
	}
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	// Desired firmware version
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for desired firmware version\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t desired_firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &desired_firmware_version),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*desired_firmware = (uint32_t)desired_firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	// Status
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for status\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t status_value;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &status_value),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*status = (uint8_t)status_value;

	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code
operation_create_set_device_info_request(uint32_t firmware, uint8_t status,
					 uint8_t *operation_buffer, uint16_t operation_buffer_size,
					 uint16_t *opcode, uint16_t *output_size)
{
	CborError err = CborNoError;
	CborEncoder operation_encoder, array_encoder;
	cbor_encoder_init(&operation_encoder, operation_buffer, operation_buffer_size, 0);
	err = cbor_encoder_create_array(&operation_encoder, &array_encoder, 2);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, firmware);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_uint(&array_encoder, status);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encoder_close_container(&operation_encoder, &array_encoder);
	if (err != CborNoError) {
		goto cleanup;
	}
	*output_size = cbor_encoder_get_buffer_size(&operation_encoder, operation_buffer);
	*opcode = OPERATION_ID_SET_DEVICE_INFO_REQUEST;
cleanup:
	if (err != CborNoError) {
		FIRMUPS_LOG_ERROR("CBOR encoding error: %d\n", err);
		return FIRMUPS_SDK_ERROR_OPERATION_ENCODING;
	}
	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code
operation_parse_set_device_info_response(uint16_t opcode, const uint8_t *operation_buffer,
					 uint16_t operation_buffer_size, uint32_t *firmware,
					 uint32_t *desired_firmware, uint8_t *status)
{
	if (opcode != OPERATION_ID_SET_DEVICE_INFO_RESPONSE) {
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
	uint64_t desired_firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &desired_firmware_version),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*desired_firmware = (uint32_t)desired_firmware_version;
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	// Status
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for status\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t status_value;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &status_value),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	*status = (uint8_t)status_value;

	return FIRMUPS_SDK_ERROR_NONE;
}
