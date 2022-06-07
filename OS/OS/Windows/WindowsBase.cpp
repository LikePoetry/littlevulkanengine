#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"
#include "../Interfaces/IFileSystem.h"

// App Data
static IApp* pApp = nullptr;

static uint8_t gResetScenario = RESET_SCENARIO_NONE;

// WindowsWindow.cpp
extern IApp* pWindowAppRef;

CustomMessageProcessor sCustomProc = nullptr;
void setCustomMessageProcessor(CustomMessageProcessor proc)
{
	sCustomProc = proc;
}

//------------------------------------------------------------------------
// OPERATING SYSTEM INTERFACE FUNCTIONS
//------------------------------------------------------------------------
void onRequestReload()
{
	gResetScenario |= RESET_SCENARIO_RELOAD;
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

	return 0;
}