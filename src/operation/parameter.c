#include "internal.h"
#include <cbor.h>
#include "../operation.h"

enum firmups_sdk_error_code
operation_create_get_parameter_request(uint16_t param_id, enum firmups_sdk_parameter_type type,
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
	err = cbor_encode_int(&array_encoder, param_id);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encode_int(&array_encoder, type);
	if (err != CborNoError) {
		goto cleanup;
	}
	err = cbor_encoder_close_container(&operation_encoder, &array_encoder);
	if (err != CborNoError) {
		goto cleanup;
	}
	*output_size = cbor_encoder_get_buffer_size(&operation_encoder, operation_buffer);
	*opcode = OPERATION_ID_GET_PARAMETER_REQUEST;
cleanup:
	if (err != CborNoError) {
		FIRMUPS_LOG_ERROR("CBOR encoding error: %d\n", err);
		return FIRMUPS_SDK_ERROR_OPERATION_ENCODING;
	}
	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code
operation_parse_get_parameter_response(uint16_t opcode, enum firmups_sdk_parameter_type type,
				       const uint8_t *operation_buffer,
				       uint16_t operation_buffer_size, void *param_buffer,
				       uint16_t param_buffer_size, uint16_t *param_value_size)
{
	if (opcode != OPERATION_ID_GET_PARAMETER_RESPONSE) {
		FIRMUPS_LOG_WARNING("Unexpected opcode: %u\n", opcode);
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	}

	CborParser parser;
	CborValue it;
	cbor_parser_init(operation_buffer, operation_buffer_size, 0, &parser, &it);

	switch (type) {
	case FIRMUPS_SDK_PARAMETER_TYPE_INTEGER:
		if (!cbor_value_is_integer(&it)) {
			FIRMUPS_LOG_WARNING("Expected integer CBOR value\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		int64_t int_value;
		RETURN_IF_CBOR_ERROR(cbor_value_get_int64_checked(&it, &int_value),
				     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size < sizeof(int64_t)) {
			FIRMUPS_LOG_WARNING("Buffer too small for integer value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		*((int64_t *)param_buffer) = int_value;
		break;

	case FIRMUPS_SDK_PARAMETER_TYPE_BOOLEAN:
		if (!cbor_value_is_integer(&it)) {
			FIRMUPS_LOG_WARNING("Expected integer CBOR value representing bool\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		int64_t bool_value;
		RETURN_IF_CBOR_ERROR(cbor_value_get_int64_checked(&it, &bool_value),
				     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size < sizeof(bool)) {
			FIRMUPS_LOG_WARNING("Buffer too small for boolean value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		*((bool *)param_buffer) = (bool_value != 0);
		break;
	case FIRMUPS_SDK_PARAMETER_TYPE_FLOAT:
		if (!cbor_value_is_float(&it)) {
			FIRMUPS_LOG_WARNING("Expected float CBOR value\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		float float_value;
		RETURN_IF_CBOR_ERROR(cbor_value_get_float(&it, &float_value),
				     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size < sizeof(float)) {
			FIRMUPS_LOG_WARNING("Buffer too small for float value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		*((float *)param_buffer) = float_value;
		break;
	case FIRMUPS_SDK_PARAMETER_TYPE_DOUBLE:
		if (!cbor_value_is_double(&it)) {
			FIRMUPS_LOG_WARNING("Expected double CBOR value\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		double double_value;
		RETURN_IF_CBOR_ERROR(cbor_value_get_double(&it, &double_value),
				     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size < sizeof(double)) {
			FIRMUPS_LOG_WARNING("Buffer too small for double value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		*((double *)param_buffer) = double_value;
		break;
	case FIRMUPS_SDK_PARAMETER_TYPE_STRING:
		if (!cbor_value_is_byte_string(&it)) {
			FIRMUPS_LOG_WARNING("Expected byte string CBOR value for string\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		size_t str_size = param_buffer_size;
		RETURN_IF_CBOR_ERROR(
			cbor_value_copy_byte_string(&it, (uint8_t *)param_buffer, &str_size, NULL),
			FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size <= str_size) {
			FIRMUPS_LOG_WARNING("Buffer too small for string value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		};
		((char *)param_buffer)[str_size] = '\0'; // Null-terminate if string
		*param_value_size = (uint16_t)str_size + 1;
		break;
	case FIRMUPS_SDK_PARAMETER_TYPE_BINARY:
		if (!cbor_value_is_byte_string(&it)) {
			FIRMUPS_LOG_WARNING("Expected byte string CBOR value for binary\n");
			return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
		}
		size_t byte_size = param_buffer_size;
		RETURN_IF_CBOR_ERROR(
			cbor_value_copy_byte_string(&it, (uint8_t *)param_buffer, &byte_size, NULL),
			FIRMUPS_SDK_ERROR_OPERATION_PARSING);
		if (param_buffer_size <= byte_size) {
			FIRMUPS_LOG_WARNING("Buffer too small for bytes value\n");
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		};
		*param_value_size = (uint16_t)byte_size;
		break;
	default:
		FIRMUPS_LOG_WARNING("Unknown parameter type: %d\n", type);
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	return FIRMUPS_SDK_ERROR_NONE;
}
