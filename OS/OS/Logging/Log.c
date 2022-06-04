#include "../Logging/Log.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IThread.h"

#define LOG_CALLBACK_MAX_ID FS_MAX_PATH
#define LOG_MAX_BUFFER 1024

typedef struct LogCallback
{
	char			mID[LOG_CALLBACK_MAX_ID];
	void*			mUserData;
	LogCallbackFn	mCallback;
	LogCloseFn		mClose;
	LogFlushFn		mFlush;
	uint32_t		mLevel;
} LogCallback;

typedef struct Log
{
	LogCallback* pCallbacks;
		size_t			mCallbacksSize;
		Mutex			mLogMutex;
		uint32_t		mLogLevel;
		uint32_t		mIndentation;
} Log;

static Log gLogger;

static THREAD_LOCAL char gLogBuffer[LOG_MAX_BUFFER + 2];
static bool gConsoleLogging = true;

#define LOG_PREAMBLE_SIZE (56 + MAX_THREAD_NAME_LENGTH + FILENAME_NAME_LENGTH_LOG)
#define LOG_LEVEL_SIZE 6

// Returns the part of the path after the last / or \(if any)
static const char* getFilename(const char* path)
{
	for (const char* ptr = path; &ptr != '\0'; ++ptr)
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			path = ptr + 1;
		}
	}
	return path;
}

static uint32_t writeLogPreamble(char* buffer, uint32_t buffer_size, const char* file, int line);

void writeLog(uint32_t level, const char* filename, int line_number, const char* message, ...)
{
	typedef struct Prefix {
		uint32_t first;
		const char* second;
	} Prefix;

	static Prefix logLevelPrefixes[] =
	{
		{eWARNING,"WARN| "},
		{eINFO,"INFO| "},
		{eDEBUG,"DBG| "},
		{eERROR,"ERR| "},
	};

	uint32_t log_levels[LEVELS_LOG];
	uint32_t log_level_count = 0;

	// Check flags
	for (uint32_t i = 0; i < sizeof(logLevelPrefixes) / sizeof(logLevelPrefixes[0]); i++)
	{
		Prefix* it = &logLevelPrefixes[i];
		if (it->first & level)
		{
			log_levels[log_level_count] = i;
			++log_level_count;
		}
	}

	uint32_t preable_end = writeLogPreamble(gLogBuffer, LOG_PREAMBLE_SIZE, filename, line_number);

	// Prepare indentation
	uint32_t indentation = gLogger.mIndentation * INDENTATION_SIZE_LOG;
	memset(gLogBuffer + preable_end, ' ', indentation);

	uint32_t offset = preable_end + LOG_LEVEL_SIZE + indentation;
	va_list args;
	va_start(args, message);
	offset += vsnprintf(gLogBuffer + offset, LOG_MAX_BUFFER - offset, message, args);
	va_end(args);

	offset = (offset > LOG_MAX_BUFFER) ? LOG_MAX_BUFFER : offset;
	gLogBuffer[offset] = '\n';
	gLogBuffer[offset + 1] = 0;

	// Log for each flag
	for (uint32_t i = 0; i < log_level_count; ++i)
	{
		strncpy(gLogBuffer + preable_end, logLevelPrefixes[log_levels[i]].second, LOG_LEVEL_SIZE);

		if (gConsoleLogging)
		{
			_PrintUnicode(gLogBuffer, level & eERROR);
		}

		acquireMutex(&gLogger.mLogMutex);
		{
			for (LogCallback* pCallback = gLogger.pCallbacks;
				pCallback != gLogger.pCallbacks + gLogger.mCallbacksSize;
				++pCallback)
			{
				if (pCallback->mLevel & log_levels[i])
					pCallback->mCallback(pCallback->mUserData, gLogBuffer);
			}
		}
		releaseMutex(&gLogger.mLogMutex);
	}
}

static uint32_t writeLogPreamble(char* buffer, uint32_t buffer_size, const char* file, int line)
{
	uint32_t pos = 0;
	// Data and time
	if (pos < buffer_size)
	{
		time_t t = time(NULL);
		struct tm	time_info;
		localtime_s(&time_info, &t);

		pos += snprintf(
			buffer + pos, buffer_size - pos, "%04d-%02d-%02d %02d:%02d:%02d ",
			1900 + time_info.tm_year, 1 + time_info.tm_mon,
			time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
	}

	if (pos < buffer_size)
	{
		char thread_name[MAX_THREAD_NAME_LENGTH + 1] = { 0 };
		getCurrentThreadName(thread_name, MAX_THREAD_NAME_LENGTH + 1);
		pos += snprintf(buffer + pos, buffer_size - pos, "[%-15s]",
			thread_name[0] == 0 ? "NoName" : thread_name);

	}

	if (pos < buffer_size)
	{
		file = getFilename(file);
		pos += snprintf(buffer + pos, buffer_size - pos, " %22.*s:%-5i ",
			FILENAME_NAME_LENGTH_LOG, file, line);
	}
	return pos;
}