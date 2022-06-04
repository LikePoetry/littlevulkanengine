#include "../Interfaces/IFileSystem.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

bool PlatformOpenFile(ResourceDirectory resourceDir, const char* fileName, FileMode mode, FileStream* pOut);

typedef struct ResourceDirectoryInfo
{
	IFileSystem* pIO = NULL;
	ResourceMount mMount = RM_CONTENT;
	char          mPath[FS_MAX_PATH] = {};
	bool          mBundled = false;
} ResourceDirectoryInfo;

static ResourceDirectoryInfo gResourceDirectories[RD_COUNT] = {};

/***********************************/
// File stream Functions
/***********************************/
static bool FileStreamOpen(IFileSystem*, const ResourceDirectory resourceDir, const char* fileName, FileMode mode, const char* password, FileStream* pOut)
{
	if (password)
	{
		LOGF(eWARNING, "System file streams do not support encrypted files");
	}
	if (PlatformOpenFile(resourceDir, fileName, mode, pOut))
	{
		pOut->mMount = fsGetResourceDirectoryMount(resourceDir);
		return true;
	}
	return false;
}

static IFileSystem gSystemFileIO =
{
	FileStreamOpen,
	FileStreamClose,
	FileStreamRead,
	FileStreamWrite,
	FileStreamSeek,
	FileStreamGetSeekPosition,
	FileStreamGetSize,
	FileStreamFlush,
	FileStreamIsAtEnd
};

IFileSystem* pSystemFileIO = &gSystemFileIO;

void fsAppendPathComponent(const char* basePath, const char* pathComponent, char* output)
{
	const size_t componentLength = strlen(pathComponent);
	const size_t baseLength = strlen(basePath);

	strncpy(output, basePath, baseLength);
	size_t newPathLength = baseLength;
	output[baseLength] = '\0';

	if (componentLength == 0)
	{
		return;
	}


}


const char* fsGetResourceDirectory(ResourceDirectory resourceDir)
{
	const ResourceDirectoryInfo* dir = &gResourceDirectories[resourceDir];

	if (!dir->pIO)
	{
		LOGF_IF(
			LogLevel::eERROR, !dir->mPath[0],
			"Trying to get an unset resource directory '%d', make sure the resourceDirectory is set on start of the application",
			resourceDir);
		ASSERT(dir->mPath[0] != 0);
	}
	return dir->mPath;
}