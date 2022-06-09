#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

class IApp
{
public:

	virtual const char* GetName() = 0;

	struct Settings
	{
		/// Window width
		int32_t  mWidth = -1;
		/// Window height
		int32_t  mHeight = -1;
		/// monitor index
		int32_t  mMonitorIndex = -1;
		/// x position for window
		int32_t  mWindowX = 0;
		///y position for window
		int32_t  mWindowY = 0;
		/// Set to true if fullscreen mode has been requested
		bool     mFullScreen = false;
		/// Set to true if app wants to use an external window
		bool     mExternalWindow = false;
		/// Drag to resize enabled
		bool     mDragToResize = true;
		/// Border less window
		bool     mBorderlessWindow = false;
		/// Set to true if oversize windows requested 
		bool     mAllowedOverSizeWindows = false;
		/// if settings is already initiazlied we don't fill when opening window
		bool     mInitialized = false;
		/// if requested to qui the application 
		bool     mQuit = false;
		/// if default automated testing enabled
		bool	 mDefaultAutomatedTesting = true;
		/// if benchmarking mode is enabled
		bool	 mBenchmarking = false;
		/// if the window is positioned in the center of the screen
		bool     mCentered = true;
		/// if the window is focused or in foreground
		bool     mFocused = false;
		/// Force lowDPI settings for this window
		bool     mForceLowDPI = false;
		/// if the platform user interface is visible
		bool     mShowPlatformUI = true;

		bool	mVSyncEnabled = false;

	} mSettings;

	WindowDesc* pWindow;
	const char* pCommandLine;

	static int			argc;
	static const char** argv;
};

#define DEFINE_APPLICATION_MAIN(appClass)							\
	int IApp::argc;													\
	const char** IApp::argv;										\
	extern int WindowsMain(int argc,char** argv,IApp* app);			\
																	\
	int main(int argc,char** argv)									\
	{																\
		IApp::argc = argc;											\
		IApp::argv = (const char**)argv;							\
		appClass app;												\
		return WindowsMain(argc, argv, &app);						\
	}																\