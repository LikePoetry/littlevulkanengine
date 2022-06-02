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