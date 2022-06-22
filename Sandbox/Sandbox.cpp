#include "../OS/Interfaces/IApp.h"
#include "../OS/Interfaces/ILog.h"
#include "../Renderer/Include/IRenderer.h"

#include "../OS/Math/MathTypes.h"


const int      gSphereResolution = 30;    // Increase for higher resolution spheres
const float    gSphereDiameter = 0.5f;

Renderer* pRenderer = NULL;

Queue* pGraphicsQueue = NULL;

int              gNumberOfSpherePoints;

float* pSpherePoints = 0;



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

		// Generate sphere vertex buffer
		generateSpherePoints(&pSpherePoints, &gNumberOfSpherePoints, gSphereResolution, gSphereDiameter);

		// window and renderer setup
		RendererDesc settings;
		memset(&settings, 0, sizeof(settings));
		settings.mD3D11Supported = false;
		settings.mGLESSupported = false;

		initRenderer(GetName(), &settings, &pRenderer);
		//check for init success
		if (!pRenderer)
			return false;

		QueueDesc queueDesc = {};
		queueDesc.mType = QUEUE_TYPE_GRAPHICS;
		queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
		vk_addQueue(pRenderer, &queueDesc, &pGraphicsQueue);



		LOGF(LogLevel::eERROR, "error ocure");
		return false;
	}
	const char* GetName() { return "Test Demo"; }
};


DEFINE_APPLICATION_MAIN(App)