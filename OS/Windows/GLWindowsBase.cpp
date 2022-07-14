#include "../Core/Config.h"
#include "../Core/CPUConfig.h"

#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/ITime.h"
#include "../Interfaces/IScripting.h"
//#include "../Interfaces/IUI.h"
#include "../Interfaces/IMemory.h"



#include "../../Renderer/Include/IRenderer.h"

// App Data
static IApp* pApp = nullptr;
static WindowDesc* gWindowDesc = nullptr;


/// CPU
static CpuInfo gCpu;

// GLWindowsWindow.cpp
extern IApp* glWindowAppRef;
extern WindowDesc* glWindow;

extern MonitorDesc* glMonitors;
extern uint32_t     glMonitorCount;

extern "C" void* gLogWindowHandle;

static inline float CounterToSecondsElapsed(int64_t start, int64_t end)
{
	return (float)(end - start) / (float)1e6;
}


// GLWindowsWindow.cpp
extern void initGLWindowClass();

bool initGLBaseSubsystems()
{
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

// UI
//static UIComponent* pAPISwitchingComponent = NULL;
//static UIComponent* pToggleVSyncComponent = NULL;
//static UIWidget* pSwitchComponentLabelWidget = NULL;
//static UIWidget* pSelectApUIWidget = NULL;
static uint32_t     gSelectedApiIndex = 0;

// Renderer.cpp
extern RendererApi gSelectedRendererApi;

void setupGLPlatformUI(int32_t width, int32_t height)
{
	gSelectedApiIndex = gSelectedRendererApi;

	//// WINDOW AND RESOLUTION CONTROL
	//extern void platformSetupWindowSystemUI(IApp*);
	//platformSetupWindowSystemUI(pApp);

	//// VSYNC CONTROL

	//UIComponentDesc uiDesc = {};
	//uiDesc.mStartPosition = vec2(width * 0.4f, height * 0.90f);
	//uiCreateComponent("VSync Control", &uiDesc, &pToggleVSyncComponent);

	//CheckboxWidget checkbox;
	//checkbox.pData = &pApp->mSettings.mVSyncEnabled;
	//UIWidget* pCheckbox = uiCreateComponentWidget(pToggleVSyncComponent, "Toggle VSync\t\t\t\t\t", &checkbox, WIDGET_TYPE_CHECKBOX);
	//REGISTER_LUA_WIDGET(pCheckbox);

	//// API SWITCHING

	//uiDesc = {};
	//uiDesc.mStartPosition = vec2(width * 0.4f, height * 0.01f);
	//uiCreateComponent("API Switching", &uiDesc, &pAPISwitchingComponent);

	//static const char* pApiNames[] =
	//{
	//	"Vulkan",
	//};

	//// Select Api 
	//DropdownWidget selectApUIWidget;
	//selectApUIWidget.pData = &gSelectedApiIndex;

	//uint32_t apiCount = RENDERER_API_COUNT;

	//ASSERT(apiCount != 0 && "No supported Graphics API available!");
	//for (uint32_t i = 0; i < apiCount; ++i)
	//{
	//	selectApUIWidget.mNames.push_back((char*)pApiNames[i]);
	//	selectApUIWidget.mValues.push_back(i);
	//}

	//pSelectApUIWidget = uiCreateComponentWidget(pAPISwitchingComponent, "Select API", &selectApUIWidget, WIDGET_TYPE_DROPDOWN);
	////TODO
	////pSelectApUIWidget->pOnEdited = onAPISwitch;
	//REGISTER_LUA_WIDGET(pSelectApUIWidget);

	//LuaScriptDesc apiScriptDesc = {};
	//apiScriptDesc.pScriptFileName = "Test_API_Switching.lua";
	//luaDefineScripts(&apiScriptDesc, 1);
}

void updateGLBaseSubsystems(float deltaTime)
{
	extern void platformUpdateWindowSystem();
	platformUpdateWindowSystem();

#ifdef ENABLE_FORGE_SCRIPTING
	extern void platformUpdateLuaScriptingSystem();
	platformUpdateLuaScriptingSystem();
#endif

//#ifdef ENABLE_FORGE_UI
//	extern void platformUpdateUserInterface(float deltaTime);
//	platformUpdateUserInterface(deltaTime);
//#endif
}

int GLWindowsMain(int argc, char** argv, IApp* app)
{
	//初始化内存管理
	if (!initMemAlloc(app->GetName()))
		return EXIT_FAILURE;

	//初始化文件系统
	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = app->GetName();
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");


	//初始化日志系统
	initLog(app->GetName(), DEFAULT_LOG_LEVEL);

	pApp = app;
	glWindowAppRef = pApp;

	//初始化窗体
	initGLWindowClass();

	//初始化CPU信息
	initCpuInfo(&gCpu);

	IApp::Settings* pSettings = &pApp->mSettings;
	WindowDesc window = {};
	glWindow = &window;// WindowsWindow.cpp
	gWindowDesc = &window; // WindowsBase.cpp
	gLogWindowHandle = (void*)&window.handle.window; // WindowsLog.c, save the address to this handle to avoid having to adding includes to WindowsLog.c to use WindowDesc*.

	//获取显示设备  并初始化窗体
	{
		if (pSettings->mMonitorIndex < 0 || pSettings->mMonitorIndex >= (int)glMonitorCount)
		{
			pSettings->mMonitorIndex = 0;
		}

		if (pSettings->mWidth <= 0 || pSettings->mHeight <= 0)
		{
			RectDesc rect = {};

			getGLRecommendedResolution(&rect);
			pSettings->mWidth = getRectWidth(&rect);
			pSettings->mHeight = getRectHeight(&rect);
		}

		MonitorDesc* monitor = getGLMonitor(pSettings->mMonitorIndex);
		ASSERT(monitor != nullptr);

		glWindow->clientRect = { (int)pSettings->mWindowX + monitor->monitorRect.left, (int)pSettings->mWindowY + monitor->monitorRect.top,
								(int)pSettings->mWidth, (int)pSettings->mHeight };

		glWindow->windowedRect = glWindow->clientRect;
		glWindow->fullScreen = pSettings->mFullScreen;
		glWindow->maximized = false;
		glWindow->noresizeFrame = !pSettings->mDragToResize;
		glWindow->borderlessWindow = pSettings->mBorderlessWindow;
		glWindow->centered = pSettings->mCentered;
		glWindow->forceLowDPI = pSettings->mForceLowDPI;
		glWindow->overrideDefaultPosition = true;
		glWindow->cursorCaptured = false;

		if (!pSettings->mExternalWindow)
			openGLWindow(pApp->GetName(), glWindow);

		pSettings->mWidth = glWindow->fullScreen ? getRectWidth(&glWindow->fullscreenRect) : getRectWidth(&glWindow->clientRect);
		pSettings->mHeight =
			glWindow->fullScreen ? getRectHeight(&glWindow->fullscreenRect) : getRectHeight(&glWindow->clientRect);

		pApp->pCommandLine = GetCommandLineA();
	}

	{
		//初始化子系统
		if (!initGLBaseSubsystems())
			return EXIT_FAILURE;

		//初始化时间系统
		Timer t;
		initTimer(&t);

		//初始化 APP
		if (!pApp->Init())
			return EXIT_FAILURE;

		// 设置
		setupGLPlatformUI(pSettings->mWidth, pSettings->mHeight);
		pSettings->mInitialized = true;

		//载入 APP资源
		if (!pApp->Load())
			return EXIT_FAILURE;
		LOGF(LogLevel::eINFO, "Application Init+Load %f", getTimerMSec(&t, false) / 1000.0f);

		bool quit = false;
		int64_t lastCounter = getUSec(false);

		while (!quit)
		{
			int64_t counter = getUSec(false);
			float deltaTime = CounterToSecondsElapsed(lastCounter, counter);

			lastCounter = counter;

			// if framerate appears to drop below about 6, assume we're at a breakpoint and simulate 20fps.
			if (deltaTime > 0.15f)
				deltaTime = 0.05f;

			bool lastMinimized = glWindow->minimized;

			updateGLWindow();

			// UPDATE BASE INTERFACES
			updateGLBaseSubsystems(deltaTime);

			// UPDATE APP
			pApp->Update(deltaTime);
			pApp->Draw();
		}
	}


	return 0;
}