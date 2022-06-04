#pragma once
#include "../Core/Config.h"
#include "../../OS/Interfaces/IFileSystem.h"

#define FILENAME_NAME_LENGTH_LOG 23
#define INDENTATION_SIZE_LOG 4
#define LEVELS_LOG 6

// If you add more level don't forget to change LOG_LEVELS macro to the actual number of levels
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

// Initialization/Exit functions are thread unsafe
void WriteLog(uint32_t level, const char* filename, int line_number, const char* message, ...);
