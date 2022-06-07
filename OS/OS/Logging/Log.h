#pragma once
#include "../Core/Config.h"
#include "../../OS/Interfaces/IFileSystem.h"

#include "stdbool.h"

#define FILENAME_NAME_LENGTH_LOG 23
#define INDENTATION_SIZE_LOG 4
#define LEVELS_LOG 6

// If you add more levels don't forget to change LOG_LEVELS macro to the actual number of levels
typedef enum LogLevel
{
	eNONE = 0,
	eRAW = 1,
	eDEBUG = 2,
	eINFO = 4,
	eWARNING = 8,
	eERROR = 16,
	eALL = ~0
} LogLevel;

typedef void (*LogCallbackFn)(void* user_data, const char* message);
typedef void (*LogCloseFn)(void* user_data);
typedef void (*LogFlushFn)(void* user_data);

#ifdef __cplusplus
extern "C"
{
#endif

	// Initialization/Exit functions are thread unsafe
	void initLog(const char* appName, LogLevel level);

	void addLogFile(const char* filename, FileMode file_mode, LogLevel log_level);
	void addLogCallback(const char* id, uint32_t log_level, void* user_data, LogCallbackFn callback, LogCloseFn close, LogFlushFn flush);

	void writeLog(uint32_t level, const char* filename, int line_number, const char* message, ...);

#ifdef __cplusplus
}    // extern "C"
#endif
