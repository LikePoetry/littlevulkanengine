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

static bool FileStreamClose(FileStream* pFile)
{
	if (fclose(pFile->pFile) == EOF)
	{
		LOGF(LogLevel::eERROR, "Error closing system FileStream", errno);
		return false;
	}

	return true;
}

static size_t FileStreamRead(FileStream* pFile, void* outputBuffer, size_t bufferSizeInBytes)
{
	size_t bytesRead = fread(outputBuffer, 1, bufferSizeInBytes, pFile->pFile);
	if (bytesRead != bufferSizeInBytes)
	{
		if (ferror(pFile->pFile) != 0)
		{
			LOGF(LogLevel::eWARNING, "Error reading from system FileStream: %s", strerror(errno));
		}
	}
	return bytesRead;
}

static size_t FileStreamWrite(FileStream* pFile, const void* sourceBuffer, size_t byteCount)
{
	if ((pFile->mMode & (FM_WRITE | FM_APPEND)) == 0)
	{
		LOGF(LogLevel::eWARNING, "Writing to FileStream with mode %u", pFile->mMode);
		return 0;
	}

	size_t bytesWritten = fwrite(sourceBuffer, 1, byteCount, pFile->pFile);

	if (bytesWritten != byteCount)
	{
		if (ferror(pFile->pFile) != 0)
		{
			LOGF(LogLevel::eWARNING, "Error writing to system FileStream: %s", strerror(errno));
		}
	}

	return bytesWritten;
}

static bool FileStreamSeek(FileStream* pFile, SeekBaseOffset baseOffset, ssize_t seekOffset)
{
	if ((pFile->mMode & FM_BINARY) == 0 && baseOffset != SBO_START_OF_FILE)
	{
		LOGF(LogLevel::eWARNING, "Text-mode FileStreams only support SBO_START_OF_FILE");
		return false;
	}

	int origin = SEEK_SET;
	switch (baseOffset)
	{
	case SBO_START_OF_FILE: origin = SEEK_SET; break;
	case SBO_CURRENT_POSITION: origin = SEEK_CUR; break;
	case SBO_END_OF_FILE: origin = SEEK_END; break;
	}

	return fseek(pFile->pFile, (long)seekOffset, origin) == 0;
}

static ssize_t FileStreamGetSeekPosition(const FileStream* pFile)
{
	long int result = ftell(pFile->pFile);
	if (result == -1L)
	{
		LOGF(LogLevel::eWARNING, "Error getting seek position in FileStream: %i", errno);
	}
	return result;
}

static ssize_t FileStreamGetSize(const FileStream* pFile)
{
	return pFile->mSize;
}

bool FileStreamFlush(FileStream* pFile)
{
	if (fflush(pFile->pFile) == EOF)
	{
		LOGF(LogLevel::eWARNING, "Error flushing system FileStream: %s", strerror(errno));
		return false;
	}

	return true;
}

bool FileStreamIsAtEnd(const FileStream* pFile)
{
	return feof(pFile->pFile) != 0;
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

/*****************************************************************/
// Platform independent filename,extension functions
/****************************************************************/
static inline FORGE_CONSTEXPR const char fsGetDirectorySeparator()
{
	return '\\';
}

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

	const char directorySeparator = fsGetDirectorySeparator();
	const char directorySeparatorStr[2] = { directorySeparator,0 };
	const char forwardSlash = '/';	// Forward slash is accepted on all platforms as a path component.

	if (newPathLength != 0 && output[newPathLength - 1] != directorySeparator)
	{
		// Append a trailing slash to the directory
		strncat(output, directorySeparatorStr, 1);
		newPathLength += 1;
		output[newPathLength] = '\0';
	}

	// ./ or .\ means current directory
	// ../ or ..\ means parent directory.

	for (size_t i = 0; i < componentLength; i++)
	{
		LOGF_IF(
			LogLevel::eERROR, newPathLength >= FS_MAX_PATH,
			"Appended path length '%d' greater than FS_MAX_PATH, base: \"%s\", component: \"%s\""
			, newPathLength, basePath, pathComponent);
		ASSERT(newPathLength < FS_MAX_PATH);

		if ((pathComponent[i] == directorySeparator || pathComponent[i] == forwardSlash) &&
			newPathLength != 0 && output[newPathLength - 1] != directorySeparator)
		{
			// We've encountered a new directory
			strncat(output, directorySeparatorStr, 1);
			newPathLength += 1;
			output[newPathLength] = '\0';
			continue;
		}
		else if (pathComponent[i] == '.')
		{
			size_t j = i + 1;
			if (j < componentLength)
			{
				if (pathComponent[j] == directorySeparator || pathComponent[j] == forwardSlash)
				{
					// ./, so it's referencing the current directory.
					// We can just skip it.
					i = j;
					continue;
				}
				else if (
					pathComponent[j] == '.' && ++j < componentLength &&
					(pathComponent[j] == directorySeparator || pathComponent[j] == forwardSlash))
				{
					// ../, so referencing the parent directory.

					if (newPathLength > 1 && output[newPathLength - 1] == directorySeparator)
					{
						// Delete any trailing directory separator.
						newPathLength -= 1;
					}

					// Backtrack until we come to the next directory separator
					for (; newPathLength > 0; newPathLength -= 1)
					{
						if (output[newPathLength - 1] == directorySeparator ||
							output[newPathLength - 1] == forwardSlash)
						{
							break;
						}
					}

					i = j;    // Skip past the ../
					continue;
				}
			}
		}
		output[newPathLength] = pathComponent[i];
		newPathLength += 1;
		output[newPathLength] = '\0';
	}
	if (output[newPathLength - 1] == directorySeparator)
	{
		// Delete any trailing directory separator.
		newPathLength -= 1;
	}
	output[newPathLength] = '\0';
}

void fsGetParentPath(const char* path, char* output)
{
	size_t pathLength = strlen(path);
	ASSERT(pathLength != 0);

	const char directorySeparator = fsGetDirectorySeparator();
	const char forwardSlash = '/';

	//Find last seperator
	const char* dirSeperatorLoc = strrchr(path, directorySeparator);
	if (dirSeperatorLoc == NULL)
	{
		dirSeperatorLoc = strrchr(path, forwardSlash);
		if (dirSeperatorLoc == NULL)
		{
			return;
		}
	}

	const size_t outputLength = pathLength - strlen(dirSeperatorLoc);
	strncpy(output, path, outputLength);
	output[outputLength] = '\0';
	return;
}

/************************************************************************/
// Platform independent directory queries
/************************************************************************/
bool fsCreateDirectory(ResourceDirectory resourceDir);


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

ResourceMount fsGetResourceDirectoryMount(ResourceDirectory resourceDir)
{
	return gResourceDirectories[resourceDir].mMount;
}

void fsSetPathForResourceDir(IFileSystem* pIO, ResourceMount mount, ResourceDirectory resourceDir, const char* bundledFolder) 
{
	ASSERT(pIO);
	ResourceDirectoryInfo* dir = &gResourceDirectories[resourceDir];

	if (dir->mPath[0]!=0)
	{
		LOGF(LogLevel::eWARNING, "Resource directory {%d} already set on:'%s'", resourceDir, dir->mPath);
		return;
	}

	dir->mMount = mount;

	if (RM_COUNT==mount)
	{
		dir->mBundled = true;
	}

	char resourcePath[FS_MAX_PATH] = { 0 };
	fsAppendPathComponent(pIO->GetResourceMount ? pIO->GetResourceMount(mount) : "", bundledFolder, resourcePath);
	strncpy(dir->mPath, resourcePath, FS_MAX_PATH);
	dir->pIO = pIO;

	if (!dir->mBundled&&dir->mPath[0]!=0)
	{
		if (!fsCreateDirectory(resourceDir))
		{
			LOGF(LogLevel::eERROR, "Could not create direcotry '%s' in filesystem", resourcePath);
		}
	}

}