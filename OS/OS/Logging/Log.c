#include "../Logging/Log.h"

#include "../Interfaces/ILog.h"


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
}