#include "GLWindowsWindow.h"
#include "Interfaces/ILog.h"

static bool s_GLFWInitialized = false;

GLWindow* GLWindow::Create(const GLWindowProps& props) {
	return new GLWindowsWindow(props);
}

GLWindowsWindow::GLWindowsWindow(const GLWindowProps& props)
{
	Init(props);
}


void GLWindowsWindow::Init(const GLWindowProps& props)
{
	if (!s_GLFWInitialized)
	{
		int success = glfwInit();
		ASSERT(success);
		s_GLFWInitialized = true;
	}


	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);


	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.Title, nullptr, nullptr);
}


void GLWindowsWindow::OnUpdate()
{
	glfwPollEvents();
}
