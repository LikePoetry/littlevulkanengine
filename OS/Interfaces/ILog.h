#pragma once
#include "../Core/Config.h"
#include "../../OS/Logging/Log.h"

#ifdef __cplusplus
extern "C"
{
#endif
	void _FailedAssert(const char* file, int line, const char* statement);
	void _OutputDebugString(const char* str, ...);
	void _OutputDebugStringV(const char* str, va_list args);

	void _PrintUnicode(const char* str, bool error);

#ifdef __cplusplus
}    // extern "C"
#endif

#define ASSERT(b) \
	if (!(b))     \
	_FailedAssert(__FILE__, __LINE__, #b)

// Usage: LOGF(LogLevel::eINFO | LogLevel::eDEBUG, "Whatever string %s, this is an int %d", "This is a string", 1)
#define LOGF(log_level, ...) writeLog((log_level), __FILE__, __LINE__, __VA_ARGS__)
// Usage: LOGF_IF(LogLevel::eINFO | LogLevel::eDEBUG, boolean_value && integer_value == 5, "Whatever string %s, this is an int %d", "This is a string", 1)
#define LOGF_IF(log_level, condition, ...) ((condition) ? writeLog((log_level), __FILE__, __LINE__, __VA_ARGS__) : (void)0)