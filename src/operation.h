#ifndef OPERATION_H
#define OPERATION_H

#include <stdint.h>
#include <firmups-device-sdk/error.h>
#include <firmups-device-sdk/sdk.h>

enum firmups_sdk_parameter_type {
	FIRMUPS_SDK_PARAMETER_TYPE_INTEGER = 1,
	FIRMUPS_SDK_PARAMETER_TYPE_BOOLEAN = 2,
	FIRMUPS_SDK_PARAMETER_TYPE_FLOAT = 3,
	FIRMUPS_SDK_PARAMETER_TYPE_DOUBLE = 4,
	FIRMUPS_SDK_PARAMETER_TYPE_STRING = 5,
	FIRMUPS_SDK_PARAMETER_TYPE_BINARY = 6
};

enum firmups_sdk_error_code
operation_create_get_parameter_request(uint16_t param_id, enum firmups_sdk_parameter_type type,
				       uint8_t *operation_buffer, uint16_t operation_buffer_size,
				       uint16_t *opcode, uint16_t *output_size);

enum firmups_sdk_error_code
operation_parse_get_parameter_response(uint16_t opcode, enum firmups_sdk_parameter_type type,
				       const uint8_t *operation_buffer,
				       uint16_t operation_buffer_size, void *param_buffer,
				       uint16_t param_buffer_size, uint16_t *param_value_size);

enum firmups_sdk_error_code operation_create_get_device_info_request(uint32_t device_id,
								     uint8_t *operation_buffer,
								     uint16_t operation_buffer_size,
								     uint16_t *opcode,
								     uint16_t *output_size);

enum firmups_sdk_error_code
operation_parse_get_device_info_response(uint16_t opcode, const uint8_t *operation_buffer,
					 uint16_t operation_buffer_size, uint32_t *firmware,
					 uint32_t *desired_firmware, uint8_t *status);

enum firmups_sdk_error_code
operation_create_set_device_info_request(uint32_t firmware, uint8_t status,
					 uint8_t *operation_buffer, uint16_t operation_buffer_size,
					 uint16_t *opcode, uint16_t *output_size);

enum firmups_sdk_error_code
operation_parse_set_device_info_response(uint16_t opcode, const uint8_t *operation_buffer,
					 uint16_t operation_buffer_size, uint32_t *firmware,
					 uint32_t *desired_firmware, uint8_t *status);

enum firmups_sdk_error_code
operation_create_get_firmware_request(uint32_t firmware, uint32_t offset, uint32_t length,
				      uint8_t *operation_buffer, uint16_t operation_buffer_size,
				      uint16_t *opcode, uint16_t *output_size);

enum firmups_sdk_error_code operation_parse_get_firmware_response(
	uint16_t opcode, const uint8_t *operation_buffer, uint16_t operation_buffer_size,
	uint32_t *firmware, uint32_t *offset, uint8_t *firmware_data,
	uint16_t firmware_data_buffer_size, uint16_t *firmware_data_size);

enum firmups_sdk_error_code operation_parse_error_response(uint16_t opcode,
							   uint8_t *operation_buffer,
							   uint16_t operation_buffer_size);

#endif /* OPERATION_H */
