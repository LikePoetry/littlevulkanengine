//#include "GLWindowsWindow.h"
//#include "Interfaces/ILog.h"
//
//static bool s_GLFWInitialized = false;
//
//GLWindow* GLWindow::Create(const GLWindowProps& props) {
//	return new GLWindowsWindow(props);
//}
//
//GLWindowsWindow::GLWindowsWindow(const GLWindowProps& props)
//{
//	Init(props);
//}
//
//
//void GLWindowsWindow::Init(const GLWindowProps& props)
//{
//	if (!s_GLFWInitialized)
//	{
//		int success = glfwInit();
//		ASSERT(success);
//		s_GLFWInitialized = true;
//	}
//
//
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//
//
//	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.Title, nullptr, nullptr);
//}
//
//
//void GLWindowsWindow::OnUpdate()
//{
//	glfwPollEvents();
//}
#include "../Core/Config.h"

#include "../Interfaces/IApp.h"

#include "../Interfaces/IOperatingSystem.h"

#include "../Math/MathTypes.h"

#include "../../ThirdParty/OpenSource/EASTL/vector.h"

#define elementsOf(a) (sizeof(a) / sizeof((a)[0]))


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

IApp* glWindowAppRef = NULL;

WindowDesc* glWindow = nullptr;

bool      glWindowClassInitialized = false;

MonitorDesc* glMonitors = nullptr;
uint32_t     glMonitorCount = 0;

struct MonitorInfo
{
	unsigned index;
	WCHAR    adapterName[32];
};

static BOOL CALLBACK monitorCallback(HMONITOR pMonitor, HDC pDeviceContext, LPRECT pRect, LPARAM pParam)
{
	MONITORINFOEXW info;
	info.cbSize = sizeof(info);
	GetMonitorInfoW(pMonitor, &info);
	MonitorInfo* data = (MonitorInfo*)pParam;
	unsigned     index = data->index;

	if (wcscmp(info.szDevice, data->adapterName) == 0)
	{
		glMonitors[index].monitorRect = { (int)info.rcMonitor.left, (int)info.rcMonitor.top, (int)info.rcMonitor.right,
										 (int)info.rcMonitor.bottom };
		glMonitors[index].workRect = { (int)info.rcWork.left, (int)info.rcWork.top, (int)info.rcWork.right, (int)info.rcWork.bottom };
	}
	return TRUE;
}

void collectGLMonitorInfo()
{
	if (glMonitors != nullptr)
	{
		return;
	}

	DISPLAY_DEVICEW adapter;
	adapter.cb = sizeof(adapter);

	int      found = 0;
	int      size = 0;
	uint32_t monitorCount = 0;

	for (int adapterIndex = 0;; ++adapterIndex)
	{
		if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
			break;

		if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
			continue;

		for (int displayIndex = 0;; displayIndex++)
		{
			DISPLAY_DEVICEW display;
			display.cb = sizeof(display);

			if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0))
				break;

			++monitorCount;
		}
	}

	if (monitorCount)
	{
		glMonitorCount = monitorCount;
		glMonitors = (MonitorDesc*)tf_calloc(monitorCount, sizeof(MonitorDesc));
		for (int adapterIndex = 0;; ++adapterIndex)
		{
			if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
				break;

			if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
				continue;

			for (int displayIndex = 0;; displayIndex++)
			{
				DISPLAY_DEVICEW display;
				HDC             dc;

				display.cb = sizeof(display);

				if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0))
					break;

				dc = CreateDCW(L"DISPLAY", adapter.DeviceName, NULL, NULL);

				MonitorDesc desc;
				desc.modesPruned = (adapter.StateFlags & DISPLAY_DEVICE_MODESPRUNED) != 0;

				wcsncpy_s(desc.adapterName, adapter.DeviceName, elementsOf(adapter.DeviceName));
				wcsncpy_s(desc.publicAdapterName, adapter.DeviceString, elementsOf(adapter.DeviceString));
				wcsncpy_s(desc.displayName, display.DeviceName, elementsOf(display.DeviceName));
				wcsncpy_s(desc.publicDisplayName, display.DeviceString, elementsOf(display.DeviceString));

				desc.physicalSize[0] = GetDeviceCaps(dc, HORZSIZE);
				desc.physicalSize[1] = GetDeviceCaps(dc, VERTSIZE);

				const float dpi = 96.0f;
				desc.dpi[0] = static_cast<UINT>(::GetDeviceCaps(dc, LOGPIXELSX) / dpi);
				desc.dpi[1] = static_cast<UINT>(::GetDeviceCaps(dc, LOGPIXELSY) / dpi);

				glMonitors[found] = (desc);
				MonitorInfo data = {};
				data.index = found;
				wcsncpy_s(data.adapterName, adapter.DeviceName, elementsOf(adapter.DeviceName));

				EnumDisplayMonitors(NULL, NULL, monitorCallback, (LPARAM)(&data));

				DeleteDC(dc);

				if ((adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) && displayIndex == 0)
				{
					MonitorDesc desc = glMonitors[0];
					glMonitors[0] = glMonitors[found];
					glMonitors[found] = desc;
				}

				found++;
			}
		}
	}
	else
	{
		LOGF(LogLevel::eDEBUG, "FallBack Option");
		//Fallback options incase enumeration fails
		//then default to the primary device
		monitorCount = 0;
		HMONITOR currentMonitor = NULL;
		currentMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
		if (currentMonitor)
		{
			monitorCount = 1;
			glMonitors = (MonitorDesc*)tf_calloc(monitorCount, sizeof(MonitorDesc));

			MONITORINFOEXW info;
			info.cbSize = sizeof(MONITORINFOEXW);
			bool        infoRead = GetMonitorInfoW(currentMonitor, &info);
			MonitorDesc desc = {};

			wcsncpy_s(desc.adapterName, info.szDevice, elementsOf(info.szDevice));
			wcsncpy_s(desc.publicAdapterName, info.szDevice, elementsOf(info.szDevice));
			wcsncpy_s(desc.displayName, info.szDevice, elementsOf(info.szDevice));
			wcsncpy_s(desc.publicDisplayName, info.szDevice, elementsOf(info.szDevice));
			desc.monitorRect = { (int)info.rcMonitor.left, (int)info.rcMonitor.top, (int)info.rcMonitor.right, (int)info.rcMonitor.bottom };
			desc.workRect = { (int)info.rcWork.left, (int)info.rcWork.top, (int)info.rcWork.right, (int)info.rcWork.bottom };
			glMonitors[0] = (desc);
			glMonitorCount = monitorCount;
		}
	}

	for (uint32_t monitor = 0; monitor < monitorCount; ++monitor)
	{
		MonitorDesc* pMonitor = &glMonitors[monitor];
		DEVMODEW     devMode = {};
		devMode.dmSize = sizeof(DEVMODEW);
		devMode.dmFields = DM_PELSHEIGHT | DM_PELSWIDTH;

		EnumDisplaySettingsW(pMonitor->adapterName, ENUM_CURRENT_SETTINGS, &devMode);
		pMonitor->defaultResolution.mHeight = devMode.dmPelsHeight;
		pMonitor->defaultResolution.mWidth = devMode.dmPelsWidth;

		pMonitor->dpi[0] = (uint32_t)((pMonitor->defaultResolution.mWidth * 25.4) / pMonitor->physicalSize[0]);
		pMonitor->dpi[1] = (uint32_t)((pMonitor->defaultResolution.mHeight * 25.4) / pMonitor->physicalSize[1]);

		eastl::vector<Resolution> displays;
		DWORD                     current = 0;
		while (EnumDisplaySettingsW(pMonitor->adapterName, current++, &devMode))
		{
			bool duplicate = false;
			for (uint32_t i = 0; i < (uint32_t)displays.size(); ++i)
			{
				if (displays[i].mWidth == (uint32_t)devMode.dmPelsWidth && displays[i].mHeight == (uint32_t)devMode.dmPelsHeight)
				{
					duplicate = true;
					break;
				}
			}

			if (duplicate)
				continue;

			Resolution videoMode = {};
			videoMode.mHeight = devMode.dmPelsHeight;
			videoMode.mWidth = devMode.dmPelsWidth;
			displays.emplace_back(videoMode);
		}
		qsort(displays.data(), displays.size(), sizeof(Resolution), [](const void* lhs, const void* rhs) {
			Resolution* pLhs = (Resolution*)lhs;
			Resolution* pRhs = (Resolution*)rhs;
			if (pLhs->mHeight == pRhs->mHeight)
				return (int)(pLhs->mWidth - pRhs->mWidth);

			return (int)(pLhs->mHeight - pRhs->mHeight);
			});

		pMonitor->resolutionCount = (uint32_t)displays.size();
		pMonitor->resolutions = (Resolution*)tf_calloc(pMonitor->resolutionCount, sizeof(Resolution));
		memcpy(pMonitor->resolutions, displays.data(), pMonitor->resolutionCount * sizeof(Resolution));
	}
}

void getGLRecommendedResolution(RectDesc* rect)
{
	*rect = { 0, 0, min(1920, (int)(GetSystemMetrics(SM_CXSCREEN) * 0.75)), min(1080, (int)(GetSystemMetrics(SM_CYSCREEN) * 0.75)) };
}

MonitorDesc* getGLMonitor(uint32_t index)
{
	ASSERT(glMonitorCount > index);
	return &glMonitors[index];
}

void openGLWindow(const char* app_name, WindowDesc* winDesc)
{
	uint32_t windowWidth = getRectWidth(&winDesc->clientRect);
	uint32_t windowHeight = getRectHeight(&winDesc->clientRect);

	winDesc->handle.window = glfwCreateWindow((int)windowWidth, (int)windowHeight, app_name, nullptr, nullptr);
}

void updateGLWindow()
{
	glfwPollEvents();
}

void initGLWindowClass()
{
	if (!glWindowClassInitialized)
	{
		int success = glfwInit();
		ASSERT(success);
		glWindowClassInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	collectGLMonitorInfo();
}
