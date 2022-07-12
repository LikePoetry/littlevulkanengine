#pragma once
#include <Core/Application.h>


extern Application* CreateApplication();



/***********************/
// 应用入口
/************************/
int main(int argc, char** argv)
{
	//初始化内存管理
	if (!initMemAlloc("App"))
		return EXIT_FAILURE;

	//初始化文件系统
	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = "App";
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

	//初始化日志系统
	initLog("App", DEFAULT_LOG_LEVEL);

	//创建应用
	auto app = CreateApplication();

	//主循环体
	app->Run();

	//释放资源
	tf_free(app);
}
