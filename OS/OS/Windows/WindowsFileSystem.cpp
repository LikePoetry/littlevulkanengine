#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/ILog.h"


static bool gInitialized = false;


bool initFileSystem(FileSystemInitDesc* pDesc)
{
	if (gInitialized)
	{
		//LOGF();
	}
	return true;
}  

bool PlatformOpenFile(ResourceDirectory resourceDir, const char* fileName, FileMode mode, FileStream* pOut)
{
	const char* resourcePath = fsGetResourceDirectory(resourceDir);
	char filePath[FS_MAX_PATH] = {};
	fsAppendPathComponent(resourcePath, fileName, filePath);
}

