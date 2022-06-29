#include "../Core/Config.h"
#include "../Core/CPUConfig.h"

#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/ITime.h"
#include "../Interfaces/IScripting.h"
#include "../Interfaces/IUI.h"
#include "../Interfaces/IMemory.h"



#include "../../Renderer/Include/IRenderer.h"
// App Data
static IApp* pApp = nullptr;
static WindowDesc* gWindowDesc = nullptr;
static uint8_t gResetScenario = RESET_SCENARIO_NONE;
static bool           gShowPlatformUI = true;

/// CPU
static CpuInfo gCpu;

// UI
// UI
static UIComponent* pAPISwitchingComponent = NULL;
static UIComponent* pToggleVSyncComponent = NULL;
static UIWidget* pSwitchComponentLabelWidget = NULL;
static UIWidget* pSelectApUIWidget = NULL;
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
// STATIC HELPER FUNCTIONS
//------------------------------------------------------------------------

static inline float CounterToSecondsElapsed(int64_t start, int64_t end)
{
	return (float)(end - start) / (float)1e6;
}

//------------------------------------------------------------------------
// OPERATING SYSTEM INTERFACE FUNCTIONS
//------------------------------------------------------------------------
void requestShutdown()
{
	PostQuitMessage(0);
}

void onRequestReload()
{
	gResetScenario |= RESET_SCENARIO_RELOAD;
}

void onDeviceLost()
{
	gResetScenario |= RESET_SCENARIO_DEVICE_LOST;
}

void onAPISwitch()
{
	gResetScenario |= RESET_SCENARIO_API_SWITCH;
}

void onGpuModeSwitch()
{
	gResetScenario |= RESET_SCENARIO_GPU_MODE_SWITCH;
}

void errorMessagePopup(const char* title, const char* msg, void* windowHandle)
{
#ifndef AUTOMATED_TESTING
	MessageBoxA((HWND)windowHandle, msg, title, MB_OK);
#endif
}


CustomMessageProcessor sCustomProc = nullptr;
void setCustomMessageProcessor(CustomMessageProcessor proc)
{
	sCustomProc = proc;
}

CpuInfo* getCpuInfo() {
	return &gCpu;
}

//------------------------------------------------------------------------
// PLATFORM LAYER CORE SUBSYSTEMS
//------------------------------------------------------------------------
bool initBaseSubsystems()
{
	// Not exposed in the interface files / app layer
	extern bool platformInitFontSystem();
	extern bool platformInitUserInterface();
	extern void platformInitLuaScriptingSystem();
	extern void platformInitWindowSystem(WindowDesc*);

	platformInitWindowSystem(gWindowDesc);
	pApp->pWindow = gWindowDesc;

#ifdef ENABLE_FORGE_FONTS
	extern bool platformInitFontSystem();
	if (!platformInitFontSystem())
		return false;
#endif
#ifdef ENABLE_FORGE_UI
	extern bool platformInitUserInterface();
	if (!platformInitUserInterface())
		return false;
#endif
#ifdef ENABLE_FORGE_SCRIPTING
	extern void platformInitLuaScriptingSystem();
	platformInitLuaScriptingSystem();
#endif
	return true;
}

void updateBaseSubsystems(float deltaTime)
{
	// Not exposed in the interface files / app layer
	extern void platformUpdateLuaScriptingSystem();
	extern void platformUpdateUserInterface(float deltaTime);
	extern void platformUpdateWindowSystem();

	platformUpdateWindowSystem();

#ifdef ENABLE_FORGE_SCRIPTING
	extern void platformUpdateLuaScriptingSystem();
	platformUpdateLuaScriptingSystem();
#endif

#ifdef ENABLE_FORGE_UI
	extern void platformUpdateUserInterface(float deltaTime);
	platformUpdateUserInterface(deltaTime);
#endif
}

void exitBaseSubsystems()
{
	// Not exposed in the interface files / app layer
	extern void platformExitFontSystem();
	extern void platformExitUserInterface();
	extern void platformExitLuaScriptingSystem();
	extern void platformExitWindowSystem();

	platformExitWindowSystem();

#ifdef ENABLE_FORGE_UI
	platformExitUserInterface();
#endif

#ifdef ENABLE_FORGE_FONTS
	platformExitFontSystem();
#endif

#ifdef ENABLE_FORGE_SCRIPTING
	platformExitLuaScriptingSystem();
#endif
}

//------------------------------------------------------------------------
// PLATFORM LAYER USER INTERFACE
//------------------------------------------------------------------------
void setupPlatformUI(int32_t width, int32_t height)
{
	gSelectedApiIndex = gSelectedRendererApi;

	// WINDOW AND RESOLUTION CONTROL
	extern void platformSetupWindowSystemUI(IApp*);
	platformSetupWindowSystemUI(pApp);

	// VSYNC CONTROL

	UIComponentDesc uiDesc = {};
	uiDesc.mStartPosition = vec2(width * 0.4f, height * 0.90f);
	uiCreateComponent("VSync Control", &uiDesc, &pToggleVSyncComponent);

	CheckboxWidget checkbox;
	checkbox.pData = &pApp->mSettings.mVSyncEnabled;
	UIWidget* pCheckbox = uiCreateComponentWidget(pToggleVSyncComponent, "Toggle VSync\t\t\t\t\t", &checkbox, WIDGET_TYPE_CHECKBOX);
	REGISTER_LUA_WIDGET(pCheckbox);

	// API SWITCHING

	uiDesc = {};
	uiDesc.mStartPosition = vec2(width * 0.4f, height * 0.01f);
	uiCreateComponent("API Switching", &uiDesc, &pAPISwitchingComponent);

	static const char* pApiNames[] =
	{
		"Vulkan",
	};

	// Select Api 
	DropdownWidget selectApUIWidget;
	selectApUIWidget.pData = &gSelectedApiIndex;

	uint32_t apiCount = RENDERER_API_COUNT;

	ASSERT(apiCount != 0 && "No supported Graphics API available!");
	for (uint32_t i = 0; i < apiCount; ++i)
	{
		selectApUIWidget.mNames.push_back((char*)pApiNames[i]);
		selectApUIWidget.mValues.push_back(i);
	}

	pSelectApUIWidget = uiCreateComponentWidget(pAPISwitchingComponent, "Select API", &selectApUIWidget, WIDGET_TYPE_DROPDOWN);
	//TODO
	//pSelectApUIWidget->pOnEdited = onAPISwitch;
	REGISTER_LUA_WIDGET(pSelectApUIWidget);

	LuaScriptDesc apiScriptDesc = {};
	apiScriptDesc.pScriptFileName = "Test_API_Switching.lua";
	luaDefineScripts(&apiScriptDesc, 1);
}

void togglePlatformUI()
{
	gShowPlatformUI = pApp->mSettings.mShowPlatformUI;

#ifdef ENABLE_FORGE_UI
	extern void platformToggleWindowSystemUI(bool);
	platformToggleWindowSystemUI(gShowPlatformUI);

	uiSetComponentActive(pToggleVSyncComponent, gShowPlatformUI);
	uiSetComponentActive(pAPISwitchingComponent, gShowPlatformUI);
#endif
}

//------------------------------------------------------------------------
// APP ENTRY POINT
//------------------------------------------------------------------------

// WindowsWindow.cpp
extern void initWindowClass();
extern void exitWindowClass();

int WindowsMain(int argc, char** argv, IApp* app)
{
	if (!initMemAlloc(app->GetName()))
		return EXIT_FAILURE;

	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = app->GetName();
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

#ifdef ENABLE_MTUNER
	rmemInit(0);
#endif

	initLog(app->GetName(), DEFAULT_LOG_LEVEL);

#ifdef ENABLE_FORGE_STACKTRACE_DUMP
	if (!WindowsStackTrace::Init())
		return EXIT_FAILURE;
#endif

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
		pSettings->mInitialized = true;

		if (!pApp->Load())
			return EXIT_FAILURE;
		LOGF(LogLevel::eINFO, "Application Init+Load %f", getTimerMSec(&t, false) / 1000.0f);

	}
	bool quit = false;
	int64_t lastCounter = getUSec(false);

	while (!quit)
	{
		if (gResetScenario != RESET_SCENARIO_NONE)
		{
			if (gResetScenario & RESET_SCENARIO_RELOAD)
			{
				pApp->Unload();

				if (!pApp->Load())
					return EXIT_FAILURE;

				gResetScenario &= ~RESET_SCENARIO_RELOAD;
				continue;
			}

			if (gResetScenario & RESET_SCENARIO_DEVICE_LOST)
			{
				errorMessagePopup(
					"Graphics Device Lost",
					"Connection to the graphics device has been lost.\nPlease verify the integrity of your graphics drivers.\nCheck the "
					"logs for further details.",
					&pApp->pWindow->handle.window);
			}

			pApp->Unload();

			pApp->Exit();

			gSelectedRendererApi = (RendererApi)gSelectedApiIndex;
			pSettings->mInitialized = false;

			closeWindow(app->pWindow);
			openWindow(app->GetName(), app->pWindow);

			exitBaseSubsystems();

			{
				if (!initBaseSubsystems())
					return EXIT_FAILURE;

				Timer t;
				initTimer(&t);
				if (!pApp->Init())
					return EXIT_FAILURE;

				setupPlatformUI(pSettings->mWidth, pSettings->mHeight);
				pSettings->mInitialized = true;

				if (!pApp->Load())
					return EXIT_FAILURE;

				LOGF(LogLevel::eINFO, "Application Reset %f", getTimerMSec(&t, false) / 1000.0f);

			}
			gResetScenario = RESET_SCENARIO_NONE;
			continue;
		}

		int64_t counter = getUSec(false);
		float deltaTime = CounterToSecondsElapsed(lastCounter, counter);

		lastCounter = counter;

		// if framerate appears to drop below about 6, assume we're at a breakpoint and simulate 20fps.
		if (deltaTime > 0.15f)
			deltaTime = 0.05f;

		bool lastMinimized = gWindow->minimized;

		extern bool handleMessages();
		quit = handleMessages() || pSettings->mQuit;

		// If window is minimized let other processes take over
		if (gWindow->minimized)
		{
			// Call update once after minimize so app can react.
			if (lastMinimized != gWindow->minimized)
			{
				pApp->Update(deltaTime);
			}
			threadSleep(1);
			continue;
		}

		// UPDATE BASE INTERFACES
		updateBaseSubsystems(deltaTime);

		// UPDATE APP
		pApp->Update(deltaTime);
		pApp->Draw();

		if (gShowPlatformUI != pApp->mSettings.mShowPlatformUI)
		{
			togglePlatformUI();
		}


	}

	pApp->mSettings.mQuit = true;
	pApp->Unload();
	pApp->Exit();

	exitWindowClass();

	exitLog();

	exitBaseSubsystems();

	exitFileSystem();

#ifdef ENABLE_FORGE_STACKTRACE_DUMP
	WindowsStackTrace::Exit();
#endif

#ifdef ENABLE_MTUNER
	rmemUnload();
	rmemShutDown();
#endif

	exitMemAlloc();

	gWindow = NULL;
	gWindowDesc = NULL;
	gLogWindowHandle = NULL;
	return 0;
}