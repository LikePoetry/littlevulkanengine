#include "Application.h"

#include "Interfaces/ILog.h"


Application* Application::s_Instance = nullptr;
Application::Application()
{
	ASSERT(!s_Instance);

	s_Instance = this;

	m_Window = GLWindow::Create();

}


Application::~Application() {

}

void Application::Close()
{

}

void Application::Run()
{
	while (m_Running)
	{
		m_Window->OnUpdate();
	}
}