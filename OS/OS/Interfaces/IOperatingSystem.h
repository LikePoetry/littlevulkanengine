#pragma once
#include <sys/stat.h>
#include <stdlib.h>
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

#include "../Interfaces/IMemory.h"

#define MAX_MONITOR_COUNT 32

typedef struct RectDesc
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} RectDesc;

typedef struct WindowHandle
{
	void* window;    //hWnd
} WindowHandle;

typedef struct WindowDesc
{

	WindowHandle handle;
	RectDesc     windowedRect;
	RectDesc     fullscreenRect;
	RectDesc     clientRect;
	uint32_t     windowsFlags;
	bool         fullScreen;
	bool         cursorCaptured;
	bool         iconified;
	bool         maximized;
	bool         minimized;
	bool         hide;
	bool         noresizeFrame;
	bool         borderlessWindow;
	bool         overrideDefaultPosition;
	bool         centered;
	bool         forceLowDPI;

	int32_t mWindowMode;

	int32_t pCurRes[MAX_MONITOR_COUNT];
	int32_t pLastRes[MAX_MONITOR_COUNT];

	int32_t mWndX;
	int32_t mWndY;
	int32_t mWndW;
	int32_t mWndH;

	bool    mCursorHidden;
	int32_t mCursorInsideWindow;
	bool    mCursorClipped;
	bool    mMinimizeRequested;

} WindowDesc;

typedef struct Resolution
{
	uint32_t mWidth;
	uint32_t mHeight;
} Resolution;

// Monitor data
//
typedef struct
{
	RectDesc     monitorRect;
	RectDesc     workRect;
	unsigned int dpi[2];
	unsigned int physicalSize[2];
	WCHAR adapterName[32];
	WCHAR displayName[32];
	WCHAR publicAdapterName[128];
	WCHAR publicDisplayName[128];
	Resolution* resolutions;
	Resolution  defaultResolution;
	uint32_t    resolutionCount;
	bool        modesPruned;
	bool        modeChanged;
} MonitorDesc;

inline int getRectWidth(const RectDesc* rect) { return rect->right - rect->left; }
inline int getRectHeight(const RectDesc* rect) { return rect->bottom - rect->top; }

// Window handling
void openWindow(const char* app_name, WindowDesc* winDesc);
void setWindowRect(WindowDesc* winDesc, const RectDesc* rect);
void setWindowSize(WindowDesc* winDesc, unsigned width, unsigned height);
void toggleBorderless(WindowDesc* winDesc, unsigned width, unsigned height);
void toggleFullscreen(WindowDesc* winDesc);
void centerWindow(WindowDesc* winDesc);

// Mouse and cursor handling
void  setMousePositionRelative(const WindowDesc* winDesc, int32_t x, int32_t y);

void getRecommendedResolution(RectDesc* rect);
MonitorDesc* getMonitor(uint32_t index);


//------------------------------------------------------------------------
// PLATFORM LAYER
//------------------------------------------------------------------------
typedef enum ResetScenario
{

	RESET_SCENARIO_NONE = 0x0,
	RESET_SCENARIO_RELOAD = 0x1,
	RESET_SCENARIO_DEVICE_LOST = 0x2,
	RESET_SCENARIO_API_SWITCH = 0x4,
	RESET_SCENARIO_GPU_MODE_SWITCH = 0x8,

} ResetScenario;

void onRequestReload();


typedef int32_t(*CustomMessageProcessor)(WindowDesc* pWindow, void* msg);
void setCustomMessageProcessor(CustomMessageProcessor proc);