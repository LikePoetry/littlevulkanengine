#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

typedef enum ResourceMount
{
	///	Installed game directory / bundle resource directory
	RM_CONTENT = 0,
	///	For storing debug data such as log files. To be used only during development
	RM_DEBUG,
	///	Documents directory
	RM_DOCUMENTS,
	///	Save game data mount 0
	RM_SAVE_0,
	///
	RM_COUNT,
} ResourceMount;

typedef struct FileSystemInitDesc
{
	const char* pAppName;
	void* pPlatformData;
	const char* pResourceMounts[RM_COUNT];

} FileSystemInitDesc;

/***********************************************/
// Mark: - Initialization
/***********************************************/
///	Initializes the FileSystem API
bool initFileSystem(FileSystemInitDesc* pDesc);