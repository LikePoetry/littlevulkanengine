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


/// CPU
static CpuInfo gCpu;

// GLWindowsWindow.cpp
extern IApp* glWindowAppRef;
extern WindowDesc* glWindow;

extern MonitorDesc* glMonitors;
extern uint32_t     glMonitorCount;

extern "C" void * gLogWindowHandle;

static inline float CounterToSecondsElapsed(int64_t start, int64_t end)
{
	return (float)(end - start) / (float)1e6;
}


// GLWindowsWindow.cpp
extern void initGLWindowClass();

bool initGLBaseSubsystems()
{
	pApp->pWindow = gWindowDesc;
	return true;
}

int GLWindowsMain(int argc, char** argv, IApp* app)
{
	//��ʼ���ڴ����
	if (!initMemAlloc(app->GetName()))
		return EXIT_FAILURE;

	//��ʼ���ļ�ϵͳ
	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = app->GetName();
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");


	//��ʼ����־ϵͳ
	initLog(app->GetName(), DEFAULT_LOG_LEVEL);

	pApp = app;
	glWindowAppRef = pApp;

	//��ʼ������
	initGLWindowClass();

	//��ʼ��CPU��Ϣ
	initCpuInfo(&gCpu);

	IApp::Settings* pSettings = &pApp->mSettings;
	WindowDesc window = {};
	glWindow = &window;// WindowsWindow.cpp
	gWindowDesc = &window; // WindowsBase.cpp
	gLogWindowHandle = (void*)&window.handle.window; // WindowsLog.c, save the address to this handle to avoid having to adding includes to WindowsLog.c to use WindowDesc*.

	//��ȡ��ʾ�豸  ����ʼ������
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
		//��ʼ����ϵͳ
		if (!initGLBaseSubsystems())
			return EXIT_FAILURE;

		//��ʼ��ʱ��ϵͳ
		Timer t;
		initTimer(&t);

		if (!pApp->Init())
			return EXIT_FAILURE;

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
			// UPDATE APP
			pApp->Update(deltaTime);
			pApp->Draw();
		}
	}


	return 0;
}