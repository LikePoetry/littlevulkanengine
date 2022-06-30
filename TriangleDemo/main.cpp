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


class App :public IApp
{
public:
	bool Init()
	{
		// FILE PATHS
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders");
		fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SHADER_BINARIES, "CompiledShaders");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
		fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");
		fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");



		return true;
	}

	void Exit()
	{
	}

	bool Load()
	{
		return true;
	}

	void Unload()
	{
		
	}

	void Update(float deltaTime)
	{
		
	}

	void Draw()
	{
	}

	const char* GetName() { return "Test Demo"; }

};

DEFINE_APPLICATION_MAIN(App)