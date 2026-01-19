#ifndef FIRMUPS_DEVICE_SDK_OPERATION_INTERNAL_H
#define FIRMUPS_DEVICE_SDK_OPERATION_INTERNAL_H

#define RETURN_IF_CBOR_ERROR(expr, ret)                                                            \
	do {                                                                                       \
		CborError err = (expr);                                                            \
		if (err != CborNoError) {                                                          \
			FIRMUPS_LOG_ERROR("CBOR error: %d\n", (err));                              \
			return (ret);                                                              \
		}                                                                                  \
	} while (0)

enum operation_id {
	OPERATION_ID_INVALID = 0,
	OPERATION_ID_ERROR = 1,
	OPERATION_ID_GET_PARAMETER_REQUEST = 2,
	OPERATION_ID_GET_PARAMETER_RESPONSE = 3,
	OPERATION_ID_SET_PARAMETER_REQUEST = 4,
	OPERATION_ID_SET_PARAMETER_RESPONSE = 5,
	OPERATION_ID_GET_DEVICE_INFO_REQUEST = 6,
	OPERATION_ID_GET_DEVICE_INFO_RESPONSE = 7,
	OPERATION_ID_SET_DEVICE_INFO_REQUEST = 8,
	OPERATION_ID_SET_DEVICE_INFO_RESPONSE = 9,
	OPERATION_ID_GET_FIRMWARE_REQUEST = 10,
	OPERATION_ID_GET_FIRMWARE_RESPONSE = 11,
};

#endif /* FIRMUPS_DEVICE_SDK_OPERATION_INTERNAL_H */
