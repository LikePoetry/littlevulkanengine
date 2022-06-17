#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

#define FS_MAX_PATH 512

#ifdef __cplusplus
extern "C"
{
#endif

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
		FM_ALLOW_READ = 1 << 4, // Read Access to Other Processes, Usefull for Log System
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

	/************************************************************************/
	// MARK: - File IO
	/************************************************************************/
	/// Opens the file at `filePath` using the mode `mode`, returning a new FileStream that can be used
	/// to read from or modify the file. May return NULL if the file could not be opened.
	bool fsOpenStreamFromPath(const ResourceDirectory resourceDir, const char* fileName,
		FileMode mode, const char* password, FileStream* pOut);

	/// Opens a memory buffer as a FileStream, returning a stream that must be closed with `fsCloseStream`.
	bool fsOpenStreamFromMemory(const void* buffer, size_t bufferSize, FileMode mode, bool owner, FileStream* pOut);

	/// Closes and invalidates the file stream.
	bool fsCloseStream(FileStream* stream);

	/// Gets the current seek position in the file.
	ssize_t fsGetStreamSeekPosition(const FileStream* stream);

	/// Seeks to the specified position in the file, using `baseOffset` as the reference offset.
	bool fsSeekStream(FileStream* pStream, SeekBaseOffset baseOffset, ssize_t seekOffset);

	/// Returns the number of bytes read.
	size_t fsReadFromStream(FileStream* stream, void* outputBuffer, size_t bufferSizeInBytes);

	/// Reads at most `bufferSizeInBytes` bytes from sourceBuffer and writes them into the file.
	/// Returns the number of bytes written.
	size_t fsWriteToStream(FileStream* stream, const void* sourceBuffer, size_t byteCount);

	/// Gets the current size of the file. Returns -1 if the size is unknown or unavailable.
	ssize_t fsGetStreamFileSize(const FileStream* stream);

	/// Flushes all writes to the file stream to the underlying subsystem.
	bool fsFlushStream(FileStream* stream);

	/// Default file system using C File IO or Bundled File IO (Android) based on the ResourceDirectory
	extern IFileSystem* pSystemFileIO;
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

	/// Appends `newExtension` to `basePath`.
	/// If `basePath` already has an extension, `newExtension` will be appended to the end.
	void fsAppendPathExtension(const char* basePath, const char* newExtension, char* output);

	/// Get `path`'s parent path, excluding the end seperator. 
	void fsGetParentPath(const char* path, char* output);

	/// Returns `path`'s extension, excluding the '.'.
	void fsGetPathExtension(const char* path, char* output);
	/************************************************************************/
	// MARK: - Directory queries
	/************************************************************************/
	/// Returns location set for resource directory in fsSetPathForResourceDir.
	const char* fsGetResourceDirectory(ResourceDirectory resourceDir);
	/// Returns Resource Mount point for resource directory
	ResourceMount fsGetResourceDirectoryMount(ResourceDirectory resourceDir);

	/// Sets the relative path for `resourceDir` from `mount` to `bundledFolder`.
	/// The `resourceDir` will making use of the given IFileSystem `pIO` file functions.
	/// When `mount` is set to `RM_CONTENT` for a `resourceDir`, this directory is marked as a bundled resource folder.
	/// Bundled resource folders should only be used for Read operations.
	/// NOTE: A `resourceDir` can only be set once.
	void fsSetPathForResourceDir(IFileSystem* pIO, ResourceMount mount, ResourceDirectory resourceDir, const char* bundledFolder);

	/// Converts `mode` to a string which is compatible with the C standard library conventions for `fopen`
	/// parameter strings.
	static inline FORGE_CONSTEXPR const char* fsFileModeToString(FileMode mode)
	{
		mode = (FileMode)(mode & ~FM_ALLOW_READ);
		switch (mode)
		{
		case FM_READ: return "r";
		case FM_WRITE: return "w";
		case FM_APPEND: return "a";
		case FM_READ_BINARY: return "rb";
		case FM_WRITE_BINARY: return "wb";
		case FM_APPEND_BINARY: return "ab";
		case FM_READ_WRITE: return "r+";
		case FM_READ_APPEND: return "a+";
		case FM_READ_WRITE_BINARY: return "rb+";
		case FM_READ_APPEND_BINARY: return "ab+";
		default: return "r";
		}
	}

	static inline FORGE_CONSTEXPR const char* fsOverwriteFileModeToString(FileMode mode)
	{

		switch (mode)
		{
		case FM_READ_WRITE: return "w+";
		case FM_READ_WRITE_BINARY: return "wb+";
		default: return fsFileModeToString(mode);
		}
	}

#ifdef __cplusplus
} // extern "C"
#endif