#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

#define FS_MAX_PATH 512

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

typedef enum ResourceDirectory
{
	/// The main application's shader binaries directory
	RD_SHADER_BINARIES = 0,
	/// The main application's shader source directory
	RD_SHADER_SOURCES,

	RD_PIPELINE_CACHE,
	/// The main application's texture source directory (TODO processed texture folder)
	RD_TEXTURES,
	RD_MESHES,
	RD_FONTS,
	RD_ANIMATIONS,
	RD_AUDIO,
	RD_GPU_CONFIG,
	RD_LOG,
	RD_SCRIPTS,
	RD_SCREENSHOTS,
	RD_OTHER_FILES,

	// Libraries can have their own directories.
	// Up to 100 libraries are suported.
	____rd_lib_counter_begin = RD_OTHER_FILES + 1,

	// Add libraries here
	RD_MIDDLEWARE_0 = ____rd_lib_counter_begin,
	RD_MIDDLEWARE_1,
	RD_MIDDLEWARE_2,
	RD_MIDDLEWARE_3,
	RD_MIDDLEWARE_4,
	RD_MIDDLEWARE_5,
	RD_MIDDLEWARE_6,
	RD_MIDDLEWARE_7,
	RD_MIDDLEWARE_8,
	RD_MIDDLEWARE_9,
	RD_MIDDLEWARE_10,
	RD_MIDDLEWARE_11,
	RD_MIDDLEWARE_12,
	RD_MIDDLEWARE_13,
	RD_MIDDLEWARE_14,
	RD_MIDDLEWARE_15,

	____rd_lib_counter_end = ____rd_lib_counter_begin + 99 * 2,
	RD_COUNT
} ResourceDirectory;

typedef enum SeekBaseOffset
{
	SBO_START_OF_FILE = 0,
	SBO_CURRENT_POSITION,
	SBO_END_OF_FILE,
} SeekBaseOffset;

typedef enum FileMode
{
	FM_READ = 1 << 0,
	FM_WRITE = 1 << 1,
	FM_APPEND = 1 << 2,
	FM_BINARY = 1 << 3,
	FM_ALLOW_READ = 1 << 4,// Read Access to Other Processes, Usefull for Log System
	FM_READ_WRITE = FM_READ | FM_WRITE,
	FM_READ_APPEND = FM_READ | FM_APPEND,
	FM_WRITE_BINARY = FM_WRITE | FM_BINARY,
	FM_READ_BINARY = FM_READ | FM_BINARY,
	FM_APPEND_BINARY = FM_APPEND | FM_BINARY,
	FM_READ_WRITE_BINARY = FM_READ | FM_WRITE | FM_BINARY,
	FM_READ_APPEND_BINARY = FM_READ | FM_APPEND | FM_BINARY,
	FM_WRITE_ALLOW_READ = FM_WRITE | FM_ALLOW_READ,
	FM_APPEND_ALLOW_READ = FM_READ | FM_ALLOW_READ,
	FM_READ_WRITE_ALLOW_READ = FM_READ | FM_WRITE | FM_ALLOW_READ,
	FM_READ_APPEND_ALLOW_READ = FM_READ | FM_APPEND | FM_ALLOW_READ,
	FM_WRITE_BINARY_ALLOW_READ = FM_WRITE | FM_BINARY | FM_ALLOW_READ,
	FM_APPEND_BINARY_ALLOW_READ = FM_APPEND | FM_BINARY | FM_ALLOW_READ,
	FM_READ_WRITE_BINARY_ALLOW_READ = FM_READ | FM_WRITE | FM_BINARY | FM_ALLOW_READ,
	FM_READ_APPEND_BINARY_ALLOW_READ = FM_READ | FM_APPEND | FM_BINARY | FM_ALLOW_READ
} FileMode;

typedef struct IFileSystem IFileSystem;

typedef struct MemoryStream
{
	uint8_t* pBuffer;
	size_t   mCursor;
	size_t   mCapacity;
	bool     mOwner;
} MemoryStream;

typedef struct FileStream
{
	IFileSystem* pIO;
	struct FileStream* pBase; // for chaining streams
	union
	{
		FILE* pFile;
		MemoryStream  mMemory;
		void* pUser;
	};
	ssize_t           mSize;
	FileMode          mMode;
	ResourceMount     mMount;
} FileStream;

typedef struct FileSystemInitDesc
{
	const char* pAppName;
	void* pPlatformData;
	const char* pResourceMounts[RM_COUNT];

} FileSystemInitDesc;

struct IFileSystem
{
	bool		(*Open)(IFileSystem* pIO,
		const ResourceDirectory resourceDir,
		const char* fileName,
		FileMode mode,
		const char* password,
		FileStream* pOut);
	bool		(*Close)(FileStream* pFile);
	size_t(*Read)(FileStream* pFile, void* outputBuffer, size_t bufferSizeInBytes);
	size_t(*Write)(FileStream* pFile, const void* sourceBuffer, size_t byteCount);
	bool		(*Seek)(FileStream* pFile, SeekBaseOffset baseOffset, ssize_t seekOffset);
	ssize_t(*GetSeekPosition)(const FileStream* pFile);
	ssize_t(*GetFileSize)(const FileStream* pFile);
	bool		(*Flush)(FileStream* pFile);
	bool		(*IsAtEnd)(const FileStream* pFile);
	const char* (*GetResourceMount)(ResourceMount mount);

	bool		(*GetPropInt64)(FileStream* pFile, int32_t prop, int64_t* pValue);
	bool		(*SetPropInt64)(FileStream* pFile, int32_t prop, int64_t* pValue);

	void* pUser;
};

/***********************************************************************/
// Mark: - Initialization
/***********************************************************************/
///	Initializes the FileSystem API
bool initFileSystem(FileSystemInitDesc* pDesc);

/************************************************************************/
// MARK: - Minor filename manipulation
/************************************************************************/
/// Appends `pathComponent` to `basePath`, where `basePath` is assumed to be a directory.
void fsAppendPathComponent(const char* basePath, const char* pathComponent, char* output);

/************************************************************************/
// MARK: - Directory queries
/************************************************************************/
/// Returns location set for resource directory in fsSetPathForResourceDir.
const char* fsGetResourceDirectory(ResourceDirectory resourceDir);