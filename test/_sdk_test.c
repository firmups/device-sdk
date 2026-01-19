#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <firmups-device-sdk/sdk.h>

int main() {
    struct firmups_sdk_api api;
    uint8_t work_buffer[512];
    struct firmups_sdk_context* context = firmups_sdk_initialize(work_buffer, sizeof(work_buffer), &api);

    uint8_t output_buffer[128];
    uint16_t output_size;
    int ret = firmups_sdk_encode_hello(context, output_buffer, sizeof(output_buffer), &output_size);
    
    printf("Hex array: ");
    for (size_t i = 0; i < output_size; i++) {
        printf("%02X ", output_buffer[i]);  // prints each byte as two-digit hex
    }
    printf("\n");

    assert(ret == 0);
    char message[64];
    ret = firmups_sdk_decode_hello(context, output_buffer, output_size, message, sizeof(message));
    assert(ret == 0);
    assert(strcmp(message, "Hello, Firmups!") == 0);

    return 0;
}