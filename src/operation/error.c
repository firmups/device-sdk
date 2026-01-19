#include "internal.h"
#include <cbor.h>
#include "../operation.h"

enum operation_error_code {
	InvalidOperation = 0,
	DecodingError = 1,
	EncodingError = 2,
	UnknownParameter = 3,
	DeviceNotFound = 4,
	FirmwareNotFound = 5,
	InternalError = 6,
};

enum firmups_sdk_error_code operation_parse_error_response(uint16_t opcode,
							   uint8_t *operation_buffer,
							   uint16_t operation_buffer_size)
{
	if (opcode != OPERATION_ID_ERROR) {
		return FIRMUPS_SDK_ERROR_NONE;
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
	// Error code
	if (!cbor_value_is_unsigned_integer(&array_it)) {
		FIRMUPS_LOG_WARNING("Expected unsigned integer for error code\n");
		return FIRMUPS_SDK_ERROR_OPERATION_PARSING;
	}
	uint64_t error_code;
	RETURN_IF_CBOR_ERROR(cbor_value_get_uint64(&array_it, &error_code),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	RETURN_IF_CBOR_ERROR(cbor_value_advance(&array_it), FIRMUPS_SDK_ERROR_OPERATION_PARSING);
	RETURN_IF_CBOR_ERROR(cbor_value_leave_container(&it, &array_it),
			     FIRMUPS_SDK_ERROR_OPERATION_PARSING);

	switch (error_code) {
	case InvalidOperation:
		FIRMUPS_LOG_ERROR("Server does not support the requested operation\n");
		return FIRMUPS_SDK_ERROR_UNSUPPORTED_OPERATION;
	case DecodingError:
		FIRMUPS_LOG_ERROR("Server could not decode the operation\n");
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	case EncodingError:
		FIRMUPS_LOG_ERROR("Server could not encode the operation result\n");
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	case UnknownParameter:
		FIRMUPS_LOG_ERROR("The operation contains unknown parameter(s)\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	case DeviceNotFound:
		FIRMUPS_LOG_ERROR("The device with the provided identifier was not found\n");
		return FIRMUPS_SDK_ERROR_NOT_FOUND;
	case FirmwareNotFound:
		FIRMUPS_LOG_ERROR("The firmware with the provided identifier was not found\n");
		return FIRMUPS_SDK_ERROR_NOT_FOUND;
	case InternalError:
	default:
		FIRMUPS_LOG_ERROR("Internal server error occurred\n");
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	}
}
