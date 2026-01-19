#ifndef FIRMUPS_DEVICE_SDK_LOG_H
#define FIRMUPS_DEVICE_SDK_LOG_H

#include <stdarg.h>

#ifndef FIRMUPS_LOG_LEVEL
#ifdef FIRMUPS_LOG_LEVEL_DEBUG
#define FIRMUPS_LOG_LEVEL 4
#elif FIRMUPS_LOG_LEVEL_INFO
#define FIRMUPS_LOG_LEVEL 3
#elif FIRMUPS_LOG_LEVEL_WARNING
#define FIRMUPS_LOG_LEVEL 2
#elif FIRMUPS_LOG_LEVEL_ERROR
#define FIRMUPS_LOG_LEVEL 1
#else
#define FIRMUPS_LOG_LEVEL 0
#endif
#endif // FIRMUPS_LOG_LEVEL

#define __FILENAME__                                                                               \
	(strrchr(__FILE__, '/')                                                                    \
		 ? strrchr(__FILE__, '/') + 1                                                      \
		 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

// DEBUG
#if FIRMUPS_LOG_LEVEL >= 4
int firmups_sdk_log_debug(const char *file, int line, const char *format, ...);
#define FIRMUPS_LOG_DEBUG(format, ...)                                                             \
	firmups_sdk_log_debug(__FILENAME__, __LINE__, format, ##__VA_ARGS__)
#else
#define FIRMUPS_LOG_DEBUG(format, ...)
#endif
// INFO
#if FIRMUPS_LOG_LEVEL >= 3
int firmups_sdk_log_info(const char *file, int line, const char *format, ...);
#define FIRMUPS_LOG_INFO(format, ...)                                                              \
	firmups_sdk_log_info(__FILENAME__, __LINE__, format, ##__VA_ARGS__)
#else
#define FIRMUPS_LOG_INFO(format, ...)
#endif
// WARNING
#if FIRMUPS_LOG_LEVEL >= 2
int firmups_sdk_log_warning(const char *file, int line, const char *format, ...);
#define FIRMUPS_LOG_WARNING(format, ...)                                                           \
	firmups_sdk_log_warning(__FILENAME__, __LINE__, format, ##__VA_ARGS__)
#else
#define FIRMUPS_LOG_WARNING(format, ...)
#endif
// ERROR
#if FIRMUPS_LOG_LEVEL >= 1
int firmups_sdk_log_error(const char *file, int line, const char *format, ...);
#define FIRMUPS_LOG_ERROR(format, ...)                                                             \
	firmups_sdk_log_error(__FILENAME__, __LINE__, format, ##__VA_ARGS__)
#else
#define FIRMUPS_LOG_ERROR(format, ...)
#endif

#endif // FIRMUPS_DEVICE_SDK_LOG_H
