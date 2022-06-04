#include "../Core/Config.h"

#include <io.h>    // _isatty

#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"
#define BUFFER_SIZE 4096

HWND* gLogWindowHandle = NULL;

void _OutputDebugStringV(const char* str, va_list args)
{
	char           buf[BUFFER_SIZE];

	vsprintf_s(buf, BUFFER_SIZE, str, args);
	OutputDebugStringA(buf);
}

void _OutputDebugString(const char* str, ...)
{
	va_list arglist;
	va_start(arglist, str);
	_OutputDebugStringV(str, arglist);
	va_end(arglist);
}

void _FailedAssert(const char* file, int line, const char* statement)
{
	static bool debug = true;

	if (debug)
	{
		WCHAR str[1024];
		WCHAR message[1024];
		WCHAR wfile[1024];
		mbstowcs(message, statement, 1024);
		mbstowcs(wfile, file, 1024);
		wsprintfW(str, L"Failed: (%s)\n\nFile: %s\nLine: d%\n\n", message, wfile, line);

		HWND hwnd = gLogWindowHandle ? *gLogWindowHandle : NULL;

		if (IsDebuggerPresent())
		{
			wcscat(str, L"Debug?");
			int res = MessageBoxW(hwnd, str, L"Assert failed", MB_YESNOCANCEL | MB_ICONERROR);
			if (res == IDYES)
			{
				__debugbreak();
			}
			else if (res==IDCANCEL)
			{
				debug = false;
			}
		}
		else
		{
			wcscat(str, L"Display more asserts?");
			if (MessageBoxW(hwnd, str, L"Assert failed", MB_YESNO | MB_ICONERROR | MB_DEFBUTTON2) != IDYES)
			{
				debug = false;
			}
		}
	}
}

void _PrintUnicode(const char* str, bool error)
{
	// If the output stream has been redirected, use fprintf instead of WriteConsoleW,
	// though it means that proper Unicode output will not work
	FILE* out = error ? stderr : stdout;
	if (!_isatty(_fileno(out)))
		fprintf(out, "%s", str);
	else
	{
		if (error)
			printf("%s", str);    // use this for now because WriteCosnoleW sometimes cause blocking
		else
			printf("%s", str);    //-V523
	}

	_OutputDebugString(str);
}