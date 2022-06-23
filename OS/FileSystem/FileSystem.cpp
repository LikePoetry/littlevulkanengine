#include "../Interfaces/IFileSystem.h"

#include "../Math/MathTypes.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

#define MEMORY_STREAM_GROW_SIZE 4096
#define STREAM_COPY_BUFFER_SIZE 4096
#define STREAM_FIND_BUFFER_SIZE 1024

bool PlatformOpenFile(ResourceDirectory resourceDir, const char* fileName, FileMode mode, FileStream* pOut);

typedef struct ResourceDirectoryInfo
{
	IFileSystem* pIO = NULL;
	ResourceMount mMount = RM_CONTENT;
	char          mPath[FS_MAX_PATH] = {};
	bool          mBundled = false;
} ResourceDirectoryInfo;

static ResourceDirectoryInfo gResourceDirectories[RD_COUNT] = {};

/************************************************************************/
// Memory Stream Functions
/************************************************************************/
static inline FORGE_CONSTEXPR size_t MemoryStreamAvailableSize(FileStream* pStream, size_t requestedSize)
{
	return min((ssize_t)requestedSize, max((ssize_t)pStream->mSize - (ssize_t)pStream->mMemory.mCursor, (ssize_t)0));
}

static bool MemoryStreamClose(FileStream* pStream)
{
	MemoryStream* mem = &pStream->mMemory;
	if (mem->mOwner)
	{
		tf_free(mem->pBuffer);
	}

	return true;
}

static size_t MemoryStreamRead(FileStream* pStream, void* outputBuffer, size_t bufferSizeInBytes)
{
	if (!(pStream->mMode & FM_READ))
	{
		LOGF(LogLevel::eWARNING, "Attempting to read from stream that doesn't have FM_READ flag.");
		return 0;
	}

	if ((ssize_t)pStream->mMemory.mCursor >= pStream->mSize)
	{
		return 0;
	}

	size_t bytesToRead = MemoryStreamAvailableSize(pStream, bufferSizeInBytes);
	memcpy(outputBuffer, pStream->mMemory.pBuffer + pStream->mMemory.mCursor, bytesToRead);
	pStream->mMemory.mCursor += bytesToRead;
	return bytesToRead;
}

static size_t MemoryStreamWrite(FileStream* pStream, const void* sourceBuffer, size_t byteCount)
{
	if (!(pStream->mMode & (FM_WRITE | FM_APPEND)))
	{
		LOGF(LogLevel::eWARNING, "Attempting to write to stream that doesn't have FM_WRITE or FM_APPEND flags.");
		return 0;
	}

	if (pStream->mMemory.mCursor > (size_t)pStream->mSize)
	{
		LOGF(eWARNING, "Creating discontinuity in initialized memory in memory stream.");
	}


	size_t availableCapacity = 0;
	if (pStream->mMemory.mCapacity >= pStream->mMemory.mCursor)
		availableCapacity = pStream->mMemory.mCapacity - pStream->mMemory.mCursor;

	if (byteCount > availableCapacity)
	{
		size_t newCapacity = pStream->mMemory.mCursor + byteCount;
		newCapacity = MEMORY_STREAM_GROW_SIZE *
			(newCapacity / MEMORY_STREAM_GROW_SIZE +
				(newCapacity % MEMORY_STREAM_GROW_SIZE == 0 ? 0 : 1));
		void* newBuffer = tf_realloc(pStream->mMemory.pBuffer, newCapacity);
		if (!newBuffer)
		{
			LOGF(eERROR, "Failed to reallocate memory stream buffer with new capacity %lu.", (unsigned long)newCapacity);
			return 0;
		}
		pStream->mMemory.pBuffer = (uint8_t*)newBuffer;
		pStream->mMemory.mCapacity = newCapacity;
	}
	memcpy(pStream->mMemory.pBuffer + pStream->mMemory.mCursor, sourceBuffer, byteCount);
	pStream->mMemory.mCursor += byteCount;
	pStream->mSize = max(pStream->mSize, (ssize_t)pStream->mMemory.mCursor);
	return byteCount;
}


static bool MemoryStreamSeek(FileStream* pStream, SeekBaseOffset baseOffset, ssize_t seekOffset)
{
	switch (baseOffset)
	{
	case SBO_START_OF_FILE:
	{
		if (seekOffset < 0 || seekOffset > pStream->mSize)
		{
			return false;
		}
		pStream->mMemory.mCursor = seekOffset;
	}
	break;
	case SBO_CURRENT_POSITION:
	{
		ssize_t newPosition = (ssize_t)pStream->mMemory.mCursor + seekOffset;
		if (newPosition < 0 || newPosition > pStream->mSize)
		{
			return false;
		}
		pStream->mMemory.mCursor = (size_t)newPosition;
	}
	break;

	case SBO_END_OF_FILE:
	{
		ssize_t newPosition = (ssize_t)pStream->mSize + seekOffset;
		if (newPosition < 0 || newPosition > pStream->mSize)
		{
			return false;
		}
		pStream->mMemory.mCursor = (size_t)newPosition;
	}
	break;
	}
	return true;
}

static ssize_t MemoryStreamGetSeekPosition(const FileStream* pStream)
{
	return pStream->mMemory.mCursor;
}

static ssize_t MemoryStreamGetSize(const FileStream* pStream)
{
	return pStream->mSize;
}

static bool MemoryStreamFlush(FileStream*)
{
	// No-op.
	return true;
}

static bool MemoryStreamIsAtEnd(const FileStream* pStream)
{
	return (ssize_t)pStream->mMemory.mCursor == pStream->mSize;
}

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

void fsAppendPathExtension(const char* basePath, const char* extension, char* output)
{
	size_t       extensionLength = strlen(extension);
	const size_t baseLength = strlen(basePath);
	const size_t maxPathLength = baseLength + extensionLength + 1;    // + 1 due to a possible added directory slash.

	LOGF_IF(LogLevel::eERROR, maxPathLength >= FS_MAX_PATH, "Extension path length '%d' greater than FS_MAX_PATH", maxPathLength);
	ASSERT(maxPathLength < FS_MAX_PATH);

	strncpy(output, basePath, baseLength);

	if (extensionLength == 0)
	{
		return;
	}

	const char directorySeparator = fsGetDirectorySeparator();
	const char forwardSlash = '/';    // Forward slash is accepted on all platforms as a path component.

	// Extension validation
	for (size_t i = 0; i < extensionLength; i += 1)
	{
		LOGF_IF(
			LogLevel::eERROR, extension[i] == directorySeparator || extension[i] == forwardSlash,
			"Extension '%s' contains directory specifiers", extension);
		ASSERT(extension[i] != directorySeparator && extension[i] != forwardSlash);
	}
	LOGF_IF(LogLevel::eERROR, extension[extensionLength - 1] == '.', "Extension '%s' ends with a '.' character", extension);

	if (extension[0] == '.')
	{
		extension += 1;
		extensionLength -= 1;
	}

	strncat(output, ".", 1);
	strncat(output, extension, extensionLength);
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

/// Get `path`'s file name, without extension or parent path.
void fsGetPathFileName(const char* path, char* output) 
{
	const size_t pathLength = strlen(path);
	ASSERT(pathLength != 0);

	char parentPath[FS_MAX_PATH] = { 0 };
	fsGetParentPath(path, parentPath);
	size_t parentPathLength = strlen(parentPath);

	const char directorySeparator = fsGetDirectorySeparator();
	const char forwardSlash = '/';    // Forward slash is accepted on all platforms as a path component.
	if (parentPathLength < pathLength && (path[parentPathLength] == directorySeparator || path[parentPathLength] == forwardSlash))
	{
		parentPathLength += 1;
	}

	char extension[FS_MAX_PATH] = { 0 };
	fsGetPathExtension(path, extension);
	const size_t extensionLength = extension[0] != 0 ? strlen(extension) + 1 : 0;    // Include dot in the length
	const size_t outputLength = pathLength - parentPathLength - extensionLength;
	strncpy(output, path + parentPathLength, outputLength);
	output[outputLength] = '\0';
}

/// Returns `path`'s extension, excluding the '.'.
void fsGetPathExtension(const char* path, char* output)
{
	size_t pathLength = strlen(path);
	ASSERT(pathLength != 0);
	const char* dotLocation = strrchr(path, '.');
	if (dotLocation == NULL)
	{
		return;
	}
	dotLocation += 1;
	const size_t extensionLength = strlen(dotLocation);
	const char   directorySeparator = fsGetDirectorySeparator();
	const char   forwardSlash = '/';    // Forward slash is accepted on all platforms as a path component.
	if (extensionLength == 0 || dotLocation[0] == forwardSlash || dotLocation[0] == directorySeparator)    // Make sure it is not "../"
	{
		return;
	}
	strncpy(output, dotLocation, extensionLength);
	output[extensionLength] = '\0';
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


/************************************************************************/
// MARK: - File Queries
/************************************************************************/
/// Gets the time of last modification for the file at `fileName`, within 'resourceDir'.
time_t fsGetLastModifiedTime(ResourceDirectory resourceDir, const char* fileName) 
{
	const char* resourcePath = fsGetResourceDirectory(resourceDir);
	char filePath[FS_MAX_PATH] = { 0 };
	fsAppendPathComponent(resourcePath, fileName, filePath);

	// Fix paths for Windows 7 - needs to be generalized and propagated
	//eastl::string path = eastl::string(filePath);
	//auto directoryPos = path.find(":");
	//eastl::string cleanPath = path.substr(directoryPos - 1);

	struct stat fileInfo = { 0 };
	stat(filePath, &fileInfo);
	return fileInfo.st_mtime;
}

/************************************************************************/
// File IO
/************************************************************************/
static IFileSystem gMemoryFileIO =
{
	NULL,
	MemoryStreamClose,
	MemoryStreamRead,
	MemoryStreamWrite,
	MemoryStreamSeek,
	MemoryStreamGetSeekPosition,
	MemoryStreamGetSize,
	MemoryStreamFlush,
	MemoryStreamIsAtEnd
};

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

bool fsOpenStreamFromMemory(const void* buffer, size_t bufferSize, FileMode mode, bool owner, FileStream* pOut)
{
	FileStream stream = {};

	size_t size = buffer ? bufferSize : 0;
	size_t capacity = bufferSize;
	// Move cursor to the end for appending buffer
	size_t cursor = (mode & FM_APPEND) ? size : 0;

	// For write and append streams we have to own the memory as we might need to resize it
	if ((mode & (FM_WRITE | FM_APPEND)) && (!owner || !buffer))
	{
		// make capacity multiple of MEMORY_STREAM_GROW_SIZE
		capacity = MEMORY_STREAM_GROW_SIZE *
			(capacity / MEMORY_STREAM_GROW_SIZE +
				(capacity % MEMORY_STREAM_GROW_SIZE == 0 ? 0 : 1));
		void* newBuffer = tf_malloc(capacity);
		ASSERT(newBuffer);
		if (buffer)
			memcpy(newBuffer, buffer, size);

		buffer = newBuffer;
		owner = true;
	}

	stream.pIO = &gMemoryFileIO;
	stream.mMemory.pBuffer = (uint8_t*)buffer;
	stream.mMemory.mCursor = cursor;
	stream.mMemory.mCapacity = capacity;
	stream.mMemory.mOwner = owner;
	stream.mSize = size;
	stream.mMode = mode;
	*pOut = stream;
	return true;
}

/// Opens the file at `filePath` using the mode `mode`, returning a new FileStream that can be used
/// to read from or modify the file. May return NULL if the file could not be opened.
bool fsOpenStreamFromPath(const ResourceDirectory resourceDir, const char* fileName,
	FileMode mode, const char* password, FileStream* pOut)
{
	IFileSystem* io = gResourceDirectories[resourceDir].pIO;
	if (!io)
	{
		LOGF(LogLevel::eERROR, "Trying to get an unset resource directory '%d', make sure the resourceDirectory is set on start of the application", resourceDir);
		return false;
	}

	return io->Open(io, resourceDir, fileName, mode, password, pOut);
}



/// Closes and invalidates the file stream.
bool fsCloseStream(FileStream* pStream)
{
	return pStream->pIO->Close(pStream);
}

/// Gets the current seek position in the file.
ssize_t fsGetStreamSeekPosition(const FileStream* pStream)
{
	return pStream->pIO->GetSeekPosition(pStream);
}

/// Seeks to the specified position in the file, using `baseOffset` as the reference offset.
bool fsSeekStream(FileStream* pStream, SeekBaseOffset baseOffset, ssize_t seekOffset)
{
	return pStream->pIO->Seek(pStream, baseOffset, seekOffset);
}

/// Returns the number of bytes read.
size_t fsReadFromStream(FileStream* pStream, void* pOutputBuffer, size_t bufferSizeInBytes)
{
	return pStream->pIO->Read(pStream, pOutputBuffer, bufferSizeInBytes);
}

/// Reads at most `bufferSizeInBytes` bytes from sourceBuffer and writes them into the file.
/// Returns the number of bytes written.
size_t fsWriteToStream(FileStream* pStream, const void* pSourceBuffer, size_t byteCount)
{
	return pStream->pIO->Write(pStream, pSourceBuffer, byteCount);
}

/// Gets the current size of the file. Returns -1 if the size is unknown or unavailable.
ssize_t fsGetStreamFileSize(const FileStream* pStream)
{
	return pStream->pIO->GetFileSize(pStream);
}

/// Flushes all writes to the file stream to the underlying subsystem.
bool fsFlushStream(FileStream* pStream)
{
	return pStream->pIO->Flush(pStream);
}

/// Returns whether the current seek position is at the end of the file stream.
bool fsStreamAtEnd(const FileStream* pStream)
{
	return pStream->pIO->IsAtEnd(pStream);
}