#pragma once

#include "Interfaces/IOperatingSystem.h"

struct GLWindowProps
{
	const char* Title;
	uint32_t Width;
	uint32_t Height;

	GLWindowProps(const char* title = "Shen Eye",
		uint32_t width = 900,
		uint32_t height = 600)
		:Title(title), Width(width), Height(height)
	{

	}
};


class GLWindow
{
public:
	static GLWindow* Create(const GLWindowProps& props = GLWindowProps());

	virtual void OnUpdate() = 0;

	virtual void* GetNativeWindow() const = 0;
};