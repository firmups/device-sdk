#include "cbor_helper.h"

inline size_t cbor_helper_bstr_header_size(size_t len)
{
	if (len <= 23) {
		return 1;
	} else if (len <= 0xFF) {
		return 2;
	} else if (len <= 0xFFFF) {
		return 3;
	} else if (len <= 0xFFFFFFFFu) {
		return 5;
	} else {
		return 9;
	}
}

enum firmups_sdk_error_code cbor_helper_write_bstr_header(uint8_t *buffer, uint8_t buffer_size,
							  size_t len)
{
	if (!buffer_size) {
		return FIRMUPS_SDK_ERROR_MESSAGE_ENCODING;
	}

	if (len <= 23) {
		if (buffer_size < 1) {
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		buffer[0] = (uint8_t)(0x40U | (uint8_t)len);
		return FIRMUPS_SDK_ERROR_NONE;
	} else if (len <= 0xFFU) {
		if (buffer_size < 2) {
			return FIRMUPS_SDK_ERROR_MESSAGE_ENCODING;
		}
		buffer[0] = 0x58;
		buffer[1] = (uint8_t)len;
		return FIRMUPS_SDK_ERROR_NONE;
	} else if (len <= 0xFFFFU) {
		if (buffer_size < 3) {
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		buffer[0] = 0x59;
		buffer[1] = (uint8_t)((len >> 8) & 0xFF);
		buffer[2] = (uint8_t)(len & 0xFF);
		return FIRMUPS_SDK_ERROR_NONE;
	} else if (len <= 0xFFFFFFFFULL) {
		if (buffer_size < 5) {
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		buffer[0] = 0x5A;
		buffer[1] = (uint8_t)((len >> 24) & 0xFF);
		buffer[2] = (uint8_t)((len >> 16) & 0xFF);
		buffer[3] = (uint8_t)((len >> 8) & 0xFF);
		buffer[4] = (uint8_t)(len & 0xFF);
		return FIRMUPS_SDK_ERROR_NONE;
	} else {
		if (buffer_size < 9) {
			return FIRMUPS_SDK_ERROR_BUFFER_TOO_SMALL;
		}
		buffer[0] = 0x5B;
		buffer[1] = (uint8_t)((len >> 56) & 0xFF);
		buffer[2] = (uint8_t)((len >> 48) & 0xFF);
		buffer[3] = (uint8_t)((len >> 40) & 0xFF);
		buffer[4] = (uint8_t)((len >> 32) & 0xFF);
		buffer[5] = (uint8_t)((len >> 24) & 0xFF);
		buffer[6] = (uint8_t)((len >> 16) & 0xFF);
		buffer[7] = (uint8_t)((len >> 8) & 0xFF);
		buffer[8] = (uint8_t)(len & 0xFF);
		return FIRMUPS_SDK_ERROR_NONE;
	}
}
