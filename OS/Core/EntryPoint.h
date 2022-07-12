#pragma once
#include <Core/Application.h>


extern Application* CreateApplication();



/***********************/
// Ӧ�����
/************************/
int main(int argc, char** argv)
{
	//��ʼ���ڴ����
	if (!initMemAlloc("App"))
		return EXIT_FAILURE;

	//��ʼ���ļ�ϵͳ
	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = "App";
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

	//��ʼ����־ϵͳ
	initLog("App", DEFAULT_LOG_LEVEL);

	//����Ӧ��
	auto app = CreateApplication();

	//��ѭ����
	app->Run();

	//�ͷ���Դ
	tf_free(app);
}
