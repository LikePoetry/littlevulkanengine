#include "../Core/Config.h"

#include "../Interfaces/IUI.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/ITime.h"
//#include "../Interfaces/IInput.h"
//#include "../Interfaces/IScripting.h"
#include "../Interfaces/IOperatingSystem.h"

static WindowDesc* pWindowRef = NULL;
static UIComponent* pWindowControlsComponent = NULL;

void platformSetupWindowSystemUI(IApp* pApp)
{
	float dpiSacle;
	{
		float dpiScaleArray[2];
		getDpiScale(dpiScaleArray);
		dpiSacle = dpiScaleArray[0];
	}

	vec2 UIPosition= { pApp->mSettings.mWidth * 0.775f, pApp->mSettings.mHeight * 0.01f };
	vec2 UIPanelSize = vec2(400.f, 750.f) / dpiSacle;

	UIComponentDesc uiDesc = {};
	uiDesc.mStartPosition = UIPosition;
	uiDesc.mStartSize = UIPanelSize;
	uiDesc.mFontID = 0;
	uiDesc.mFontSize = 16.0f;

	uiCreateComponent("Window and Resolution Controls", &uiDesc, &pWindowControlsComponent);
	//uiSetComponentFlags(pWindowControlsComponent, GUI_COMPONENT_FLAGS_START_COLLAPSED);
	//strcpy(gPlatformName, "Windows");
}