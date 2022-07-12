#pragma once
#include "Core/GLWindow.h"

#include <GLFW/glfw3.h>


class GLWindowsWindow :public GLWindow
{
public:
	GLWindowsWindow(const GLWindowProps& props);
	virtual ~GLWindowsWindow() {};

	void OnUpdate() override;

private:
	virtual void Init(const GLWindowProps& props);

private:
	GLFWwindow* m_Window;

};