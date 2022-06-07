#include "../Core/Config.h"

#include "../Interfaces/IApp.h"
#include "../Interfaces/IOperatingSystem.h"

#define FORGE_WINDOW_CLASS L"The Forge"

IApp* pWindowAppRef = NULL;

bool      gWindowClassInitialized = false;
WNDCLASSW gWindowClass;

//------------------------------------------------------------------------
// MONITOR AND RESOLUTION HANDLING INTERFACE FUNCTIONS
//------------------------------------------------------------------------

// Window event handler - Use as less as possible
LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

void initWindowClass()
{
	if (!gWindowClassInitialized)
	{
		HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
		memset(&gWindowClass, 0, sizeof(gWindowClass));
		gWindowClass.style = 0;
		gWindowClass.lpfnWndProc = WinProc;
		gWindowClass.hInstance = instance;
		gWindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		gWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		gWindowClass.lpszClassName = FORGE_WINDOW_CLASS;
	}
}