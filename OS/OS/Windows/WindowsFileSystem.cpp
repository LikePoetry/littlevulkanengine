#include <functional>

#include <functional>

#include "shlobj.h"
#include "commdlg.h"
#include <WinBase.h>

#include "../Interfaces/ILog.h"
#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IThread.h"
#include "../Interfaces/IMemory.h"

template <typename T>
static inline T withUTF16Path(const char* path, T(*function)(const wchar_t*))
{
	size_t len = strlen(path);
	wchar_t* buffer = (wchar_t*)alloca((len + 1) * sizeof(wchar_t));

	size_t resultLength = MultiByteToWideChar(CP_UTF8, 0, path, (int)len, buffer, (int)len);
	buffer[resultLength] = 0;

	return function(buffer);
}

static bool gInitialized = false;
static const char* gResourceMounts[RM_COUNT];

const char* getResourceMount(ResourceMount mount) {
	return gResourceMounts[mount];
}

static char gApplicationPath[FS_MAX_PATH] = {};
static char gDocumentsPath[FS_MAX_PATH] = {};

bool initFileSystem(FileSystemInitDesc* pDesc)
{
	if (gInitialized)
	{
		LOGF(LogLevel::eWARNING, "FileSystem already initialized.");
		return true;
	}
	ASSERT(pDesc);
	pSystemFileIO->GetResourceMount = getResourceMount;

	for (uint32_t i = 0; i < RM_COUNT; i++)
		gResourceMounts[i] = "";

	// Get application directory
	wchar_t utf16Path[FS_MAX_PATH];
	GetModuleFileNameW(0, utf16Path, FS_MAX_PATH);
	char applicationFilePath[FS_MAX_PATH] = {};
	WideCharToMultiByte(CP_UTF8, 0, utf16Path, -1, applicationFilePath, MAX_PATH, NULL, NULL);
	fsGetParentPath(applicationFilePath, gApplicationPath);

	gResourceMounts[RM_CONTENT] = gApplicationPath;
	gResourceMounts[RM_DEBUG] = gApplicationPath;

	// Get user directory
	PWSTR userDocuments = NULL;
	SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &userDocuments);
	WideCharToMultiByte(CP_UTF8, 0, userDocuments, -1, gDocumentsPath, MAX_PATH, NULL, NULL);
	CoTaskMemFree(userDocuments);
	gResourceMounts[RM_DOCUMENTS] = gDocumentsPath;
	gResourceMounts[RM_SAVE_0] = gApplicationPath;

	// Override Resource mounts
	for (uint32_t i = 0; i < RM_COUNT; i++)
	{
		if (pDesc->pResourceMounts[i])
			gResourceMounts[i] = pDesc->pResourceMounts[i];
	}

	gInitialized = true;
	return true;
}

static bool fsDirectoryExists(const char* path)
{
	return withUTF16Path<bool>(path, [](const wchar_t* pathStr)
		{
			DWORD attributes = GetFileAttributesW(pathStr);
			return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
		});
}

static bool fsCreateDirectory(const char* path)
{
	if (fsDirectoryExists(path))
	{
		return true;
	}

	char parentPath[FS_MAX_PATH] = { 0 };
	fsGetParentPath(path, parentPath);
	if (parentPath[0] != 0)
	{
		if (!fsDirectoryExists(parentPath))
		{
			fsCreateDirectory(parentPath);
		}
	}
	return withUTF16Path<bool>(path, [](const wchar_t* pathStr)
		{
			return ::CreateDirectoryW(pathStr, NULL) ? true : false;
		});
}

bool fsCreateDirectory(ResourceDirectory resourceDir)
{
	return fsCreateDirectory(fsGetResourceDirectory(resourceDir));
}

bool PlatformOpenFile(ResourceDirectory resourceDir, const char* fileName, FileMode mode, FileStream* pOut)
{
	const char* resourcePath = fsGetResourceDirectory(resourceDir);
	char filePath[FS_MAX_PATH] = {};
	fsAppendPathComponent(resourcePath, fileName, filePath);

	// Path utf-16 conversion
	size_t filePathLen = strlen(filePath);
	wchar_t* pathStr = (wchar_t*)alloca((filePathLen + 1) * sizeof(wchar_t));
	size_t pathStrLength =
		MultiByteToWideChar(CP_UTF8, 0, filePath, (int)filePathLen, pathStr, (int)filePathLen);
	pathStr[pathStrLength] = 0;

	// Mode string utf-16 conversion
	const char* modeStr = fsFileModeToString(mode);

	wchar_t modeWStr[4] = {};
	mbstowcs(modeWStr, modeStr, 4);

	FILE* fp = NULL;
	if (mode & FM_ALLOW_READ)
	{
		fp = _wfsopen(pathStr, modeWStr, _SH_DENYWR);
	}
	else
	{
		_wfopen_s(&fp, pathStr, modeWStr);
	}

	// We need to change mode for read | write mode to 'w+' or 'wb+'
	// if file doesn't exist so that it can be created
	if (!fp)
	{
		//Try changing mode to 'w+' or 'wb+'
		if ((mode & FM_READ_WRITE) == FM_READ_WRITE)
		{
			modeStr = fsOverwriteFileModeToString(mode);
			mbstowcs(modeWStr, modeStr, 4);
			if (mode & FM_ALLOW_READ)
			{
				fp = _wfsopen(pathStr, modeWStr, _SH_DENYWR);
			}
			else
			{
				_wfopen_s(&fp, pathStr, modeWStr);
			}
		}
	}

	if (fp)
	{
		*pOut = {};
		pOut->pFile = fp;
		pOut->mMode = mode;
		pOut->pIO = pSystemFileIO;

		pOut->mSize = -1;
		if (fseek(pOut->pFile, 0, SEEK_END) == 0)
		{
			pOut->mSize = ftell(pOut->pFile);
			rewind(pOut->pFile);
		}
		return true;
	}
	else
	{
		LOGF(LogLevel::eERROR, "Error opening file: %s -- %s (error: %s)", filePath, modeStr, strerror(errno));
	}

	return false;
}

