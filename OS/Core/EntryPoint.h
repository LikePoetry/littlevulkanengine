#pragma once
#include <Core/Application.h>


extern Application* CreateApplication();


const char* GetAppName() { return "App"; };
/***********************/
// Ӧ�����
/************************/
int main(int argc, char** argv)
{
	//��ʼ���ڴ����
	if (!initMemAlloc(GetAppName()))
		return EXIT_FAILURE;

	//��ʼ���ļ�ϵͳ
	FileSystemInitDesc fsDesc = {};
	fsDesc.pAppName = GetAppName();
	if (!initFileSystem(&fsDesc))
		return EXIT_FAILURE;

	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

	//��ʼ����־ϵͳ
	initLog(GetAppName(), DEFAULT_LOG_LEVEL);
	
	//����Ӧ��
	auto app = CreateApplication();

	//��ѭ����
	app->Run();

	//�ͷ���Դ
	tf_free(app);
}
