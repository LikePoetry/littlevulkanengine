#include "../Core/Config.h"
#include "../Core/CPUConfig.h"

#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"
#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/ITime.h"

#include "../../Renderer/Include/IRenderer.h"
// App Data
static IApp* pApp = nullptr;
static WindowDesc* gWindowDesc = nullptr;
static uint8_t gResetScenario = RESET_SCENARIO_NONE;

/// CPU
static CpuInfo gCpu;

// UI
static uint32_t     gSelectedApiIndex = 0;

// Renderer.cpp
extern RendererApi gSelectedRendererApi;

// WindowsWindow.cpp
extern IApp* pWindowAppRef;
extern WindowDesc* gWindow;
extern bool gCursorVisible;
extern bool gCursorInsideRectangle;
extern MonitorDesc* gMonitors;
extern uint32_t     gMonitorCount;

// WindowsLog.c
extern "C" HWND * gLogWindowHandle;

//------------------------------------------------------------------------
// OPERATING SYSTEM INTERFACE FUNCTIONS
//------------------------------------------------------------------------
void onRequestReload()
{
	gResetScenario |= RESET_SCENARIO_RELOAD;
}

CustomMessageProcessor sCustomProc = nullptr;
void setCustomMessageProcessor(CustomMessageProcessor proc)
{
	sCustomProc = proc;
}

//------------------------------------------------------------------------
// PLATFORM LAYER CORE SUBSYSTEMS
//------------------------------------------------------------------------
bool initBaseSubsystems()
{
	// Not exposed in the interface files / app layer
	extern bool platformInitFontSystem();

#ifdef ENABLE_FORGE_FONTS
	extern bool platformInitFontSystem();
	if (!platformInitFontSystem())
		return false;
#endif
	return true;
}

//------------------------------------------------------------------------
// PLATFORM LAYER USER INTERFACE
//------------------------------------------------------------------------
void setupPlatformUI(int32_t width,int32_t height)
{
	gSelectedApiIndex = gSelectedRendererApi;

	// WINDOW AND RESOLUTION CONTROL
	extern void platformSetupWindowSystemUI(IApp*);
}

//------------------------------------------------------------------------
// APP ENTRY POINT
//------------------------------------------------------------------------

// WindowsWindow.cpp
extern void initWindowClass();

int WindowsMain(int argc, char** argv, IApp* app)
{
	if (!initMemAlloc(app->GetName()))
		return EXIT_FAILURE;

	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = app->GetName();
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

	initLog(app->GetName(), DEFAULT_LOG_LEVEL);

	pApp = app;
	pWindowAppRef = app;

	initWindowClass();

	initCpuInfo(&gCpu);

	IApp::Settings* pSettings = &pApp->mSettings;
	WindowDesc window = {};
	gWindow = &window;// WindowsWindow.cpp
	gWindowDesc = &window; // WindowsBase.cpp
	gLogWindowHandle = (HWND*)&window.handle.window; // WindowsLog.c, save the address to this handle to avoid having to adding includes to WindowsLog.c to use WindowDesc*.

	if (pSettings->mMonitorIndex < 0 || pSettings->mMonitorIndex >= (int)gMonitorCount)
	{
		pSettings->mMonitorIndex = 0;
	}

	if (pSettings->mWidth <= 0 || pSettings->mHeight <= 0)
	{
		RectDesc rect = {};

		getRecommendedResolution(&rect);
		pSettings->mWidth = getRectWidth(&rect);
		pSettings->mHeight = getRectHeight(&rect);
	}

	MonitorDesc* monitor = getMonitor(pSettings->mMonitorIndex);
	ASSERT(monitor != nullptr);

	gWindow->clientRect = { (int)pSettings->mWindowX + monitor->monitorRect.left, (int)pSettings->mWindowY + monitor->monitorRect.top,
							(int)pSettings->mWidth, (int)pSettings->mHeight };

	gWindow->windowedRect = gWindow->clientRect;
	gWindow->fullScreen = pSettings->mFullScreen;
	gWindow->maximized = false;
	gWindow->noresizeFrame = !pSettings->mDragToResize;
	gWindow->borderlessWindow = pSettings->mBorderlessWindow;
	gWindow->centered = pSettings->mCentered;
	gWindow->forceLowDPI = pSettings->mForceLowDPI;
	gWindow->overrideDefaultPosition = true;
	gWindow->cursorCaptured = false;

	if (!pSettings->mExternalWindow)
		openWindow(pApp->GetName(), gWindow);

	pSettings->mWidth = gWindow->fullScreen ? getRectWidth(&gWindow->fullscreenRect) : getRectWidth(&gWindow->clientRect);
	pSettings->mHeight =
		gWindow->fullScreen ? getRectHeight(&gWindow->fullscreenRect) : getRectHeight(&gWindow->clientRect);

	pApp->pCommandLine = GetCommandLineA();

	{
		if (!initBaseSubsystems())
			return EXIT_FAILURE;

		Timer t;
		initTimer(&t);

		if (!pApp->Init())
			return EXIT_FAILURE;

		setupPlatformUI(pSettings->mWidth, pSettings->mHeight);
	}

	return 0;
}