#ifndef FIRMUPS_DEVICE_SDK_SDK_H
#define FIRMUPS_DEVICE_SDK_SDK_H

#include <stdint.h>
#include <stdbool.h>

#include <firmups-device-sdk/log.h>
#include <firmups-device-sdk/error.h>

typedef enum firmups_sdk_error_code (*random_bytes_callback)(uint8_t *buffer, uint16_t size,
							     void *userdata);
#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
typedef enum firmups_sdk_error_code (*encrypt_callback)(unsigned char *c, unsigned long long *clen,
							const unsigned char *m,
							unsigned long long mlen,
							const unsigned char *ad,
							unsigned long long adlen,
							const unsigned char *npub, void *userdata);
typedef enum firmups_sdk_error_code (*decrypt_callback)(unsigned char *m, unsigned long long *mlen,
							const unsigned char *c,
							unsigned long long clen,
							const unsigned char *ad,
							unsigned long long adlen,
							const unsigned char *npub, void *userdata);
#else
typedef enum firmups_sdk_error_code (*key_callback)(uint8_t *key_buffer, uint16_t key_buffer_size,
						    void *userdata);
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
typedef enum firmups_sdk_error_code (*send_callback)(uint8_t *send_buffer,
						     uint16_t send_buffer_size,
						     uint8_t *response_buffer,
						     uint16_t response_buffer_size,
						     uint16_t *response_size, void *userdata);
typedef enum firmups_sdk_error_code (*receive_callback)(uint8_t *buffer, uint8_t size,
							void *userdata);

struct firmups_sdk_context;
struct firmups_sdk_firmware_download_context;

struct firmups_sdk_api {
	random_bytes_callback get_random_bytes;
	void *random_bytes_userdata;
#ifdef FIRMUPS_USE_CRYPTO_CALLBACKS
	encrypt_callback encrypt_data;
	void *encrypt_data_userdata;
	decrypt_callback decrypt_data;
	void *decrypt_data_userdata;
#else
	key_callback get_key;
	void *get_key_userdata;
#endif // FIRMUPS_USE_CRYPTO_CALLBACKS
	send_callback send_data;
	void *send_data_userdata;
	receive_callback receive_data;
	void *receive_data_userdata;
	// Add SDK API function pointers and members here
};

struct firmups_sdk_device_info {
	uint32_t firmware;
	uint32_t desired_firmware;
	uint8_t status;
};

struct firmups_sdk_device_info_update {
	uint32_t firmware;
	uint8_t status;
};

struct firmups_sdk_context *firmups_sdk_initialize(uint8_t *work_buffer, uint16_t work_buffer_size,
						   const struct firmups_sdk_api *api,
						   uint32_t device_id);

enum firmups_sdk_error_code firmups_sdk_get_parameter_int(struct firmups_sdk_context *context,
							  uint16_t param_id, int64_t *param_value);
enum firmups_sdk_error_code firmups_sdk_get_parameter_bool(struct firmups_sdk_context *context,
							   uint16_t param_id, bool *param_value);
enum firmups_sdk_error_code firmups_sdk_get_parameter_float(struct firmups_sdk_context *context,
							    uint16_t param_id, float *param_value);
enum firmups_sdk_error_code firmups_sdk_get_parameter_double(struct firmups_sdk_context *context,
							     uint16_t param_id,
							     double *param_value);
enum firmups_sdk_error_code firmups_sdk_get_parameter_string(struct firmups_sdk_context *context,
							     uint16_t param_id, char *param_value,
							     uint16_t param_buffer_size,
							     uint16_t *actual_size);
enum firmups_sdk_error_code firmups_sdk_get_parameter_binary(struct firmups_sdk_context *context,
							     uint16_t param_id,
							     uint8_t *param_value,
							     uint16_t param_buffer_size,
							     uint16_t *actual_size);
enum firmups_sdk_error_code firmups_sdk_get_device_info(struct firmups_sdk_context *context,
							struct firmups_sdk_device_info *info);
enum firmups_sdk_error_code
firmups_sdk_set_device_info(struct firmups_sdk_context *context,
			    struct firmups_sdk_device_info_update const *info);
enum firmups_sdk_error_code
firmups_sdk_firmware_download_initialize(struct firmups_sdk_context *context, uint16_t firmware_id,
					 uint8_t *buffer, uint16_t buffer_size);
enum firmups_sdk_error_code
firmups_sdk_firmware_download_get_chunk(struct firmups_sdk_context *context, uint8_t const **output,
					uint16_t *output_size, bool *is_complete);
enum firmups_sdk_error_code
firmups_sdk_firmware_download_finish(struct firmups_sdk_context *context);

#endif // FIRMUPS_DEVICE_SDK_SDK_H
