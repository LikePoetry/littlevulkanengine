#include "../OS/Interfaces/ICameraController.h"
#include "../OS/Interfaces/IApp.h"
#include "../OS/Interfaces/ILog.h"
#include "../OS/Interfaces/IInput.h"
#include "../OS/Interfaces/IFileSystem.h"
#include "../OS/Interfaces/ITime.h"
#include "../OS/Interfaces/IProfiler.h"
#include "../OS/Interfaces/IScreenshot.h"
#include "../OS/Interfaces/IScripting.h"

#include "../OS/Interfaces/IFont.h"
#include "../OS/Interfaces/IUI.h"

#include "../Renderer/Include/IRenderer.h"
#include "../Renderer/Include/IResourceLoader.h"

#include "../OS/Math/MathTypes.h"

#include "../OS/Interfaces/IMemory.h"

#include "../OS/Core/EntryPoint.h"
#include "../OS/Core/Application.h"


class TriangleDemo :public Application
{
public:
	TriangleDemo()
	{

	};

	~TriangleDemo() 
	{

	};

private:
};

Application* CreateApplication() {
	return tf_placement_new<TriangleDemo>(tf_calloc(1,sizeof(TriangleDemo)));
}