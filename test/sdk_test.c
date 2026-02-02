#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unity.h>

#include <firmups-device-sdk/sdk.h>

uint8_t const *test_request_buffer = NULL;
uint16_t test_request_buffer_size = 0;
uint8_t const *test_response_buffer = NULL;
uint16_t test_response_buffer_size = 0;

void setUp(void)
{
}
void tearDown(void)
{
	test_request_buffer = NULL;
	test_request_buffer_size = 0;
	test_response_buffer = NULL;
	test_response_buffer_size = 0;
}

int firmups_sdk_log_debug(const char *file, int line, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("[DEBUG] (%s:%d) ", file, line);
	int ret = vprintf(format, args);
	printf("\n");

	va_end(args);
	return ret;
}

int firmups_sdk_log_info(const char *file, int line, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("[INFO] (%s:%d) ", file, line);
	int ret = vprintf(format, args);
	printf("\n");

	va_end(args);
	return ret;
}

int firmups_sdk_log_warning(const char *file, int line, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("[WARNING] (%s:%d) ", file, line);
	int ret = vprintf(format, args);
	printf("\n");

	va_end(args);
	return ret;
}
int firmups_sdk_log_error(const char *file, int line, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("[ERROR] (%s:%d) ", file, line);
	int ret = vprintf(format, args);
	printf("\n");

	va_end(args);
	return ret;
}

enum firmups_sdk_error_code get_random_bytes(uint8_t *buffer, uint16_t size, void *userdata)
{
	memset(buffer, 0, size);
	return FIRMUPS_SDK_ERROR_NONE;
}

enum firmups_sdk_error_code get_key(uint8_t *key_buffer, uint16_t key_buffer_size, void *userdata)
{
	memset(key_buffer, 0, key_buffer_size);
	return FIRMUPS_SDK_ERROR_NONE;
}

void print_hex(const uint8_t *data, size_t len)
{
	printf("Hex: ");
	for (size_t i = 0; i < len; i++) {
		printf("0x%02X,", data[i]);
	}
	printf("\n");
}

enum firmups_sdk_error_code send_data(uint8_t *send_buffer, uint16_t send_buffer_size,
				      uint8_t *response_buffer, uint16_t response_buffer_size,
				      uint16_t *response_size, void *userdata)
{
	if (test_request_buffer == NULL || test_request_buffer_size == 0) {
		UNITY_TEST_ASSERT(false, __LINE__, "No test request buffer set");
	}
	print_hex(send_buffer, send_buffer_size);
	UNITY_TEST_ASSERT_EQUAL_UINT16(test_request_buffer_size, send_buffer_size, __LINE__,
				       "Sent data size does not match expected request size");
	uint32_t min_size = send_buffer_size;
	UNITY_TEST_ASSERT_EQUAL_MEMORY(test_request_buffer, send_buffer, min_size, __LINE__,
				       "Sent data does not match expected request");
	if (test_response_buffer == NULL || test_response_buffer_size == 0) {
		UNITY_TEST_ASSERT(false, __LINE__, "No test response buffer set");
	}
	for (size_t i = 0; i < test_response_buffer_size; i++) {
		response_buffer[i] = test_response_buffer[i];
	}
	*response_size = test_response_buffer_size;
	return FIRMUPS_SDK_ERROR_NONE;
}

// void test_get_device_info(void)
// {
// 	struct firmups_sdk_api api = {
// 		.get_random_bytes = get_random_bytes, .get_key = get_key, .send_data = send_data};
// 	uint8_t work_buffer[512];
// 	struct firmups_sdk_context *context =
// 		firmups_sdk_initialize(work_buffer, sizeof(work_buffer), &api, 1);

// 	uint8_t const request_buffer[] = {
// 		0x83, 0x58, 0x26, 0xA5, 0x01, 0x18, 0x23, 0x05, 0x50, 0x00, 0x00, 0x00, 0x00,
// 		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
// 		0x82, 0x19, 0x21, 0xA0, 0x19, 0x21, 0xB9, 0x19, 0x21, 0xA0, 0x01, 0x19, 0x21,
// 		0xB9, 0x06, 0xA0, 0x52, 0xCC, 0x1B, 0x01, 0xF0, 0xC3, 0x2F, 0xBF, 0xE7, 0xBE,
// 		0x6E, 0x2A, 0x46, 0x86, 0x77, 0xC9, 0x2C, 0x7D, 0x64};
// 	test_request_buffer = request_buffer;
// 	test_request_buffer_size = sizeof(request_buffer);

// 	uint8_t const response_buffer[] = {
// 		0x83, 0x58, 0x27, 0xA5, 0x01, 0x18, 0x23, 0x05, 0x50, 0x00, 0x00, 0x00, 0x00,
// 		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
// 		0x82, 0x19, 0x21, 0xA0, 0x19, 0x21, 0xB9, 0x19, 0x21, 0xA0, 0x18, 0x19, 0x19,
// 		0x21, 0xB9, 0x02, 0xA0, 0x52, 0xDB, 0xC8, 0xB1, 0x1F, 0xD8, 0x2D, 0xDB, 0x7F,
// 		0xD6, 0xD7, 0x1F, 0xE6, 0x0D, 0xA7, 0xBC, 0x37, 0x5F, 0xFB};
// 	test_response_buffer = response_buffer;
// 	test_response_buffer_size = sizeof(response_buffer);

// 	struct firmups_sdk_device_info info = {0};
// 	enum firmups_sdk_error_code ret = firmups_sdk_get_device_info(context, &info);
// 	TEST_ASSERT_EQUAL_INT(FIRMUPS_SDK_ERROR_NONE, ret);
// }

void test_example(void)
{
	// Example test function
	TEST_ASSERT_EQUAL_INT(1, 1);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_example);
	return UNITY_END();
}
