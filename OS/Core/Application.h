#pragma once
class Application
{
public:
	Application();
	virtual ~Application();

	void Close();
	void Run();
private:
	static Application* s_Instance;
};

// To be defined in CLIENT
Application* CreateApplication();
