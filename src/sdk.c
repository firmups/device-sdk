#include <cbor.h>
#include <string.h>
#include <stdalign.h>
#include <stdatomic.h>

#include <firmups-device-sdk/sdk.h>
#include "codec/cose.h"
#include "operation.h"
#include "codec/cbor_helper.h"

#define OPERATION_BUFFER_SIZE 128
#define RESPONSE_BUFFER_SIZE  128

#define LOCK_CONTEXT(context, ret)                                                                 \
	do {                                                                                       \
		if (atomic_flag_test_and_set_explicit(&context->lock, memory_order_acquire)) {     \
			FIRMUPS_LOG_ERROR("SDK context is already in use\n");                      \
			return ret;                                                                \
		}                                                                                  \
	} while (0)

#define UNLOCK_CONTEXT(context)                                                                    \
	do {                                                                                       \
		atomic_flag_clear_explicit(&context->lock, memory_order_release);                  \
	} while (0)

struct firmups_sdk_firmware_download_context {
	uint16_t firmware_id;
	uint32_t offset;
	uint8_t *buffer1;
	uint16_t buffer1_size;
	uint8_t *buffer2;
	uint16_t buffer2_size;
	uint16_t chunk_size;
	bool valid;
	atomic_flag lock;
};

struct firmups_sdk_context {
	struct firmups_sdk_api api;
	struct cose_context cose_ctx;
	struct firmups_sdk_firmware_download_context firmware_download_ctx;
	uint32_t device_id;
	uint8_t operation_buffer[OPERATION_BUFFER_SIZE];
	uint8_t response_buffer[RESPONSE_BUFFER_SIZE];
	atomic_flag lock;
};

/* Predeclarations */
static enum firmups_sdk_error_code sdk_get_parameter(struct firmups_sdk_context *context,
						     uint16_t param_id,
						     enum firmups_sdk_parameter_type type,
						     void *buffer, uint16_t buffer_size,
						     uint16_t *output_size);
static enum firmups_sdk_error_code sdk_get_device_info(struct firmups_sdk_context *context,
						       struct firmups_sdk_device_info *info);
static enum firmups_sdk_error_code
sdk_set_device_info(struct firmups_sdk_context *context,
		    struct firmups_sdk_device_info_update const *info);
static enum firmups_sdk_error_code
sdk_firmware_download_initialize(struct firmups_sdk_context *context, uint16_t firmware_id,
				 uint8_t *buffer, uint16_t buffer_size);
static enum firmups_sdk_error_code
sdk_firmware_download_get_chunk(struct firmups_sdk_context *context, uint8_t const **output,
				uint16_t *output_size, bool *is_complete);
static enum firmups_sdk_error_code
sdk_firmware_download_finish(struct firmups_sdk_context *context);

/* Public Functions */
struct firmups_sdk_context *firmups_sdk_initialize(uint8_t *work_buffer, uint16_t work_buffer_size,
						   const struct firmups_sdk_api *api,
						   uint32_t device_id)
{
	if (work_buffer == NULL || api == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_sdk_initialize\n");
		return NULL;
	}
	if (work_buffer_size < sizeof(struct firmups_sdk_context)) {
		FIRMUPS_LOG_ERROR("Work buffer too small for SDK context\n");
		return NULL;
	}
	if (((uintptr_t)work_buffer % alignof(struct firmups_sdk_context)) != 0) {
		FIRMUPS_LOG_ERROR("Work buffer not properly aligned for SDK context\n");
		return NULL; // Misaligned
	}

	struct firmups_sdk_context *context = (struct firmups_sdk_context *)work_buffer;
	context->device_id = device_id;
	memcpy(&context->api, api, sizeof(*api));
	cose_init(&context->cose_ctx, &context->api);
	context->firmware_download_ctx.lock = (atomic_flag)ATOMIC_FLAG_INIT;
	context->firmware_download_ctx.valid = false;
	context->lock = (atomic_flag)ATOMIC_FLAG_INIT;

	// if (work_buffer_size - sizeof(*context) > 0) {
	// 	FIRMUPS_LOG_WARNING(
	// 		"Excess work buffer size provided to SDK initialization (%u bytes)\n",
	// 		work_buffer_size - sizeof(*context));
	// }
	return context;
}

enum firmups_sdk_error_code firmups_sdk_get_parameter_int(struct firmups_sdk_context *context,
							  uint16_t param_id, int64_t *param_value)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_INTEGER,
				     param_value, sizeof(int64_t), NULL);
	UNLOCK_CONTEXT(context);
	return ret_code;
}
enum firmups_sdk_error_code firmups_sdk_get_parameter_bool(struct firmups_sdk_context *context,
							   uint16_t param_id, bool *param_value)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_BOOLEAN,
				     param_value, sizeof(bool), NULL);
	UNLOCK_CONTEXT(context);
	return ret_code;
}
enum firmups_sdk_error_code firmups_sdk_get_parameter_float(struct firmups_sdk_context *context,
							    uint16_t param_id, float *param_value)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_FLOAT,
				     param_value, sizeof(float), NULL);
	UNLOCK_CONTEXT(context);
	return ret_code;
}
enum firmups_sdk_error_code firmups_sdk_get_parameter_double(struct firmups_sdk_context *context,
							     uint16_t param_id, double *param_value)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_DOUBLE,
				     param_value, sizeof(double), NULL);
	UNLOCK_CONTEXT(context);
	return ret_code;
}
enum firmups_sdk_error_code firmups_sdk_get_parameter_string(struct firmups_sdk_context *context,
							     uint16_t param_id, char *param_value,
							     uint16_t param_buffer_size,
							     uint16_t *actual_size)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_STRING,
				     param_value, param_buffer_size, actual_size);
	UNLOCK_CONTEXT(context);
	return ret_code;
}
enum firmups_sdk_error_code firmups_sdk_get_parameter_binary(struct firmups_sdk_context *context,
							     uint16_t param_id,
							     uint8_t *param_value,
							     uint16_t param_buffer_size,
							     uint16_t *actual_size)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_parameter(context, param_id, FIRMUPS_SDK_PARAMETER_TYPE_BINARY,
				     param_value, param_buffer_size, actual_size);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

enum firmups_sdk_error_code firmups_sdk_get_device_info(struct firmups_sdk_context *context,
							struct firmups_sdk_device_info *info)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_get_device_info(context, info);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

enum firmups_sdk_error_code
firmups_sdk_set_device_info(struct firmups_sdk_context *context,
			    struct firmups_sdk_device_info_update const *info)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_set_device_info(context, info);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

enum firmups_sdk_error_code
firmups_sdk_firmware_download_initialize(struct firmups_sdk_context *context, uint16_t firmware_id,
					 uint8_t *buffer, uint16_t buffer_size)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_firmware_download_initialize(context, firmware_id, buffer, buffer_size);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

enum firmups_sdk_error_code
firmups_sdk_firmware_download_get_chunk(struct firmups_sdk_context *context, uint8_t const **output,
					uint16_t *output_size, bool *is_complete)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_firmware_download_get_chunk(context, output, output_size, is_complete);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

enum firmups_sdk_error_code
firmups_sdk_firmware_download_finish(struct firmups_sdk_context *context)
{
	enum firmups_sdk_error_code ret_code;
	LOCK_CONTEXT(context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	ret_code = sdk_firmware_download_finish(context);
	UNLOCK_CONTEXT(context);
	return ret_code;
}

/* Private Functions */
static enum firmups_sdk_error_code sdk_get_parameter(struct firmups_sdk_context *context,
						     uint16_t param_id,
						     enum firmups_sdk_parameter_type type,
						     void *buffer, uint16_t buffer_size,
						     uint16_t *output_size)
{
	if (context == NULL || buffer == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_sdk_get_parameter\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	uint16_t o_size = 0;
	uint16_t operation_output_size = 0;
	uint32_t out_device_id = 0;
	uint16_t out_opcode = 0;
	enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;

	ret_code = operation_create_get_parameter_request(param_id, type, context->response_buffer,
							  sizeof(context->response_buffer),
							  &out_opcode, &operation_output_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = cose_encrypt_msg(&context->cose_ctx, context->device_id, out_opcode,
				    context->response_buffer, operation_output_size,
				    context->operation_buffer, sizeof(context->operation_buffer),
				    &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code =
		context->api.send_data(context->operation_buffer, o_size, context->response_buffer,
				       sizeof(context->response_buffer), &operation_output_size,
				       context->api.send_data_userdata);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_decrypt_msg(
		&context->cose_ctx, context->response_buffer, operation_output_size, &out_device_id,
		&out_opcode, context->operation_buffer, sizeof(context->operation_buffer), &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_error_response(out_opcode, context->operation_buffer, o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_get_parameter_response(
		out_opcode, type, context->operation_buffer, o_size, buffer, buffer_size, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	if (output_size != NULL) {
		*output_size = o_size;
	}

	return ret_code;
}

static enum firmups_sdk_error_code sdk_get_device_info(struct firmups_sdk_context *context,
						       struct firmups_sdk_device_info *info)
{
	if (context == NULL || info == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_get_device_info\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	uint16_t o_size = 0;
	uint16_t operation_output_size = 0;
	uint32_t out_device_id = 0;
	uint16_t out_opcode = 0;
	enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;

	ret_code = operation_create_get_device_info_request(
		context->device_id, context->response_buffer, sizeof(context->response_buffer),
							    &out_opcode, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_encrypt_msg(&context->cose_ctx, context->device_id, out_opcode,
				    context->response_buffer, o_size, context->operation_buffer,
				    sizeof(context->operation_buffer), &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code =
		context->api.send_data(context->operation_buffer, o_size, context->response_buffer,
				       sizeof(context->response_buffer), &operation_output_size,
				       context->api.send_data_userdata);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_decrypt_msg(
		&context->cose_ctx, context->response_buffer, operation_output_size, &out_device_id,
		&out_opcode, context->operation_buffer, sizeof(context->operation_buffer), &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_error_response(out_opcode, context->operation_buffer, o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_get_device_info_response(out_opcode, context->operation_buffer,
							    o_size, &info->firmware,
							    &info->desired_firmware, &info->status);

	return ret_code;
}

static enum firmups_sdk_error_code
sdk_set_device_info(struct firmups_sdk_context *context,
		    struct firmups_sdk_device_info_update const *info)
{
	if (context == NULL || info == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_get_device_info\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	uint16_t o_size = 0;
	uint16_t operation_output_size = 0;
	uint32_t out_device_id = 0;
	uint16_t out_opcode = 0;
	enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;

	ret_code = operation_create_set_device_info_request(
		info->firmware, info->status, context->response_buffer,
		sizeof(context->response_buffer), &out_opcode, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_encrypt_msg(&context->cose_ctx, context->device_id, out_opcode,
				    context->response_buffer, o_size, context->operation_buffer,
				    sizeof(context->operation_buffer), &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code =
		context->api.send_data(context->operation_buffer, o_size, context->response_buffer,
				       sizeof(context->response_buffer), &operation_output_size,
				       context->api.send_data_userdata);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_decrypt_msg(
		&context->cose_ctx, context->response_buffer, operation_output_size, &out_device_id,
		&out_opcode, context->operation_buffer, sizeof(context->operation_buffer), &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_error_response(out_opcode, context->operation_buffer, o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	uint32_t firmware = 0;
	uint32_t desired_firmware = 0;
	uint8_t status = 0;
	ret_code = operation_parse_set_device_info_response(out_opcode, context->operation_buffer,
							    o_size, &firmware, &desired_firmware,
							    &status);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	if (firmware != info->firmware || status != info->status) {
		FIRMUPS_LOG_ERROR("Set device info response does not match the request\n");
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	}

	return ret_code;
}

static enum firmups_sdk_error_code
sdk_firmware_download_initialize(struct firmups_sdk_context *context, uint16_t firmware_id,
				 uint8_t *buffer, uint16_t buffer_size)
{
	if (context == NULL || buffer == NULL || buffer_size <= 2) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_sdk_firmware_download_initialize\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	uint16_t buffer1_size = buffer_size / 2;
	uint16_t buffer2_size = buffer_size - buffer1_size;
	uint8_t *buffer1 = buffer;
	uint8_t *buffer2 = buffer + buffer1_size;

	uint8_t chunk_size_loss =
		cbor_helper_bstr_header_size(buffer1_size) + COSE_HEADER_SIZE + CRYPTO_TAG_SIZE;

	if (buffer1_size < chunk_size_loss) {
		FIRMUPS_LOG_ERROR("Buffer size too small for firmware download initialization\n");
		return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
	}

	uint16_t chunk_size = buffer1_size - chunk_size_loss;
	if (chunk_size < 40) {
		FIRMUPS_LOG_WARNING("Download chunk size very small (< 40 bytes)\n");
	}

	struct firmups_sdk_firmware_download_context *download_context =
		&context->firmware_download_ctx;
	LOCK_CONTEXT(download_context, FIRMUPS_SDK_ERROR_UNSUPPORTED_CONCURRENCY);
	download_context->valid = true;
	download_context->offset = 0;
	download_context->firmware_id = firmware_id;
	download_context->buffer1 = buffer1;
	download_context->buffer1_size = buffer1_size;
	download_context->buffer2 = buffer2;
	download_context->buffer2_size = buffer2_size;
	download_context->chunk_size = chunk_size;

	return FIRMUPS_SDK_ERROR_NONE;
}

static enum firmups_sdk_error_code
sdk_firmware_download_get_chunk(struct firmups_sdk_context *context, uint8_t const **output,
				uint16_t *output_size, bool *is_complete)
{
	static enum firmups_sdk_error_code ret_code = FIRMUPS_SDK_ERROR_NONE;
	uint32_t out_device_id = 0;
	uint16_t o_size = 0;
	uint16_t out_opcode = 0;
	uint16_t response_size = 0;

	if (context == NULL || output == NULL || output_size == NULL || is_complete == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_sdk_firmware_download_get_chunk\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}
	struct firmups_sdk_firmware_download_context *download_context =
		&context->firmware_download_ctx;
	if (!download_context->valid) {
		FIRMUPS_LOG_ERROR("Firmware download context is not initialized\n");
		return FIRMUPS_SDK_ERROR_NOT_INITIALIZED;
	};

	ret_code = operation_create_get_firmware_request(
		download_context->firmware_id, download_context->offset,
		download_context->chunk_size, download_context->buffer1,
		download_context->buffer1_size, &out_opcode, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_encrypt_msg(&context->cose_ctx, context->device_id, out_opcode,
				    download_context->buffer1, o_size, download_context->buffer2,
				    download_context->buffer2_size, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = context->api.send_data(download_context->buffer2, o_size,
					  download_context->buffer1, download_context->buffer1_size,
					  &response_size, context->api.send_data_userdata);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}
	ret_code = cose_decrypt_msg(&context->cose_ctx, download_context->buffer1, response_size,
				    &out_device_id, &out_opcode, download_context->buffer2,
				    download_context->buffer2_size, &o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	ret_code = operation_parse_error_response(out_opcode, context->operation_buffer, o_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	uint32_t firmware = 0;
	uint32_t offset = 0;
	ret_code = operation_parse_get_firmware_response(
		out_opcode, download_context->buffer2, o_size, &firmware, &offset,
		download_context->buffer1, download_context->buffer1_size, output_size);
	if (ret_code != FIRMUPS_SDK_ERROR_NONE) {
		return ret_code;
	}

	if ((offset != download_context->offset) || (firmware != download_context->firmware_id)) {
		FIRMUPS_LOG_ERROR("Unexpected offset/firmware_id in firmware response");
		return FIRMUPS_SDK_ERROR_UNEXPECTED_RESPONSE;
	}

	download_context->offset += *output_size;
	*is_complete = (*output_size < download_context->chunk_size);
	*output = download_context->buffer1;

	return ret_code;
}

static enum firmups_sdk_error_code sdk_firmware_download_finish(struct firmups_sdk_context *context)
{
	if (context == NULL) {
		FIRMUPS_LOG_ERROR("Invalid argument to firmups_sdk_firmware_download_finish\n");
		return FIRMUPS_SDK_ERROR_INVALID_ARGUMENT;
	}

	struct firmups_sdk_firmware_download_context *download_context =
		&context->firmware_download_ctx;
	download_context->valid = false;
	UNLOCK_CONTEXT(download_context);
	return FIRMUPS_SDK_ERROR_NONE;
}
