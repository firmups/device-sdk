#ifndef FIRMUPS_DEVICE_SDK_CODEC_CBOR_HELPER_H
#define FIRMUPS_DEVICE_SDK_CODEC_CBOR_HELPER_H

#include <stddef.h>
#include <stdint.h>
#include <firmups-device-sdk/error.h>

size_t cbor_helper_bstr_header_size(size_t len);
enum firmups_sdk_error_code cbor_helper_write_bstr_header(uint8_t *buffer, uint8_t buffer_size,
							  size_t len);

#endif /* FIRMUPS_DEVICE_SDK_CODEC_CBOR_HELPER_H */
