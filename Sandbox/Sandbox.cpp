#include "../OS/Interfaces/IApp.h"
#include "../OS/Interfaces/ILog.h"
#include "../OS/Interfaces/IScreenshot.h"
#include "../Renderer/Include/IRenderer.h"

#include "../OS/Math/MathTypes.h"

#include "../Renderer/Include/IResourceLoader.h"

const uint32_t gImageCount = 3;
const int      gSphereResolution = 30;    // Increase for higher resolution spheres
const float    gSphereDiameter = 0.5f;

Renderer* pRenderer = NULL;

Queue* pGraphicsQueue = NULL;
CmdPool* pCmdPools[gImageCount] = { NULL };
Cmd* pCmds[gImageCount] = { NULL };

Fence* pRenderCompleteFences[gImageCount] = { NULL };
Semaphore* pImageAcquiredSemaphore = NULL;
Semaphore* pRenderCompleteSemaphores[gImageCount] = { NULL };

Shader* pSphereShader = NULL;

Shader* pSkyBoxDrawShader = NULL;


Texture* pSkyBoxTextures[6];

int              gNumberOfSpherePoints;

const char* pSkyBoxImageFileNames[] = { "Skybox_right1",  "Skybox_left2",  "Skybox_top3",
										"Skybox_bottom4", "Skybox_front5", "Skybox_back6" };

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

		for (uint32_t i = 0; i < gImageCount; ++i)
		{
			CmdPoolDesc cmdPoolDesc = {};
			cmdPoolDesc.pQueue = pGraphicsQueue;
			vk_addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPools[i]);
			CmdDesc cmdDesc = {};
			cmdDesc.pPool = pCmdPools[i];
			vk_addCmd(pRenderer, &cmdDesc, &pCmds[i]);

			vk_addFence(pRenderer, &pRenderCompleteFences[i]);
			vk_addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
		}
		vk_addSemaphore(pRenderer, &pImageAcquiredSemaphore);

		initScreenshotInterface(pRenderer, pGraphicsQueue);

		initResourceLoaderInterface(pRenderer);

		// Loads Skybox Textures
		for (size_t i = 0; i < 6; i++)
		{
			TextureLoadDesc textureDesc = {};
			textureDesc.pFileName = pSkyBoxImageFileNames[i];
			textureDesc.ppTexture = &pSkyBoxTextures[i];
			// Textures representing color should be stored in SRGB or HDR format
			textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
			addResource(&textureDesc, NULL);
		}

		ShaderLoadDesc skyShader = {};
		skyShader.mStages[0] = { "skybox.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
		skyShader.mStages[1] = { "skybox.frag", NULL, 0 };
		ShaderLoadDesc basicShader = {};
		basicShader.mStages[0] = { "basic.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
		basicShader.mStages[1] = { "basic.frag", NULL, 0 };
		ShaderLoadDesc crashShader = {};
		crashShader.mStages[0] = { "crash.vert", NULL, 0 };
		crashShader.mStages[1] = { "basic.frag", NULL, 0 };

		addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);
		addShader(pRenderer, &basicShader, &pSphereShader);


		LOGF(LogLevel::eERROR, "error ocure");
		return false;
	}
	const char* GetName() { return "Test Demo"; }
};


DEFINE_APPLICATION_MAIN(App)