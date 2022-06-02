#pragma once
#include "../Core/Config.h"

class IApp
{
public:
	virtual const char* GetName() = 0;
public:
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