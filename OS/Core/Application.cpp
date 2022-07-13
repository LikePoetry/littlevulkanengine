//#include "Application.h"
//
//
//#define MAX_PLANETS 20 
//
//#include "../OS/Interfaces/ICameraController.h"
//#include "../OS/Interfaces/IApp.h"
//#include "../OS/Interfaces/ILog.h"
//#include "../OS/Interfaces/IInput.h"
//#include "../OS/Interfaces/IFileSystem.h"
//#include "../OS/Interfaces/ITime.h"
//#include "../OS/Interfaces/IProfiler.h"
//#include "../OS/Interfaces/IScreenshot.h"
//#include "../OS/Interfaces/IScripting.h"
//
//#include "../OS/Interfaces/IFont.h"
//#include "../OS/Interfaces/IUI.h"
//
//#include "../Renderer/Include/IRenderer.h"
//#include "../Renderer/Include/IResourceLoader.h"
//
//#include "../OS/Math/MathTypes.h"
//
//#include "../OS/Interfaces/IMemory.h"
//
///// Demo structures
//struct PlanetInfoStruct
//{
//	mat4  mTranslationMat;
//	mat4  mScaleMat;
//	mat4  mSharedMat;    // Matrix to pass down to children
//	vec4  mColor;
//	uint  mParentIndex;
//	float mYOrbitSpeed;    // Rotation speed around parent
//	float mZOrbitSpeed;
//	float mRotationSpeed;    // Rotation speed around self
//};
//
//struct UniformBlock
//{
//	CameraMatrix mProjectView;
//	mat4 mToWorldMat[MAX_PLANETS];
//	vec4 mColor[MAX_PLANETS];
//
//	// Point Light Information
//	vec3 mLightPosition;
//	vec3 mLightColor;
//};
//
//const uint32_t gImageCount = 3;
//const int      gSphereResolution = 1;    // Increase for higher resolution spheres
//const float    gSphereDiameter = 0.5f;
//const uint     gNumPlanets = 11;
//const uint     gTimeOffset = 600000;
//const float    gRotSelfScale = 0.0004f;
//const float    gRotOrbitYScale = 0.001f;
//const float    gRotOrbitZScale = 0.00001f;
//
//Renderer* pRenderer = NULL;
//
//Queue* pGraphicsQueue = NULL;
//CmdPool* pCmdPools[gImageCount] = { NULL };
//Cmd* pCmds[gImageCount] = { NULL };
//
//SwapChain* pSwapChain = NULL;
//RenderTarget* pDepthBuffer = NULL;
//Fence* pRenderCompleteFences[gImageCount] = { NULL };
//Semaphore* pImageAcquiredSemaphore = NULL;
//Semaphore* pRenderCompleteSemaphores[gImageCount] = { NULL };
//
//Shader* pSphereShader = NULL;
//Buffer* pSphereVertexBuffer = NULL;
//Pipeline* pSpherePipeline = NULL;
//
//Shader* pSkyBoxDrawShader = NULL;
//Buffer* pSkyBoxVertexBuffer = NULL;
//Pipeline* pSkyBoxDrawPipeline = NULL;
//RootSignature* pRootSignature = NULL;
//Sampler* pSamplerSkyBox = NULL;
//
//Texture* pSkyBoxTextures[6];
//DescriptorSet* pDescriptorSetTexture = { NULL };
//DescriptorSet* pDescriptorSetUniforms = { NULL };
//
//Buffer* pProjViewUniformBuffer[gImageCount] = { NULL };
//Buffer* pSkyboxUniformBuffer[gImageCount] = { NULL };
//
//uint32_t gFrameIndex = 0;
//ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;
//
//Shader* pCrashShader = NULL;
//
//int              gNumberOfSpherePoints;
//UniformBlock     gUniformData;
//UniformBlock     gUniformDataSky;
//PlanetInfoStruct gPlanetInfoData[gNumPlanets];
//
//ICameraController* pCameraController = NULL;
//
//UIComponent* pGuiWindow = NULL;
//
//uint32_t gFontID = 0;
//
//bool bHasCrashed = false;
//bool bSimulateCrash = false;
//
//uint32_t gCrashedFrame = 0;
//const uint32_t gMarkerCount = 2;
//const uint32_t gValidMarkerValue = 1U;
//Buffer* pMarkerBuffer[gImageCount] = { NULL };
//
//Pipeline* pCrashPipeline = NULL;
//
//const char* pSkyBoxImageFileNames[] = { "Skybox_right1",  "Skybox_left2",  "Skybox_top3",
//										"Skybox_bottom4", "Skybox_front5", "Skybox_back6" };
//FontDrawDesc gFrameTimeDraw;
//
//
//float* pSpherePoints = 0;
//
////Generate sky box vertex buffer
//const float gSkyBoxPoints[] = {
//	10.0f,  -10.0f, -10.0f, 6.0f,    // -z
//	-10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   -10.0f, 10.0f,
//	-10.0f, 6.0f,   10.0f,  10.0f,  -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,
//
//	-10.0f, -10.0f, 10.0f,  2.0f,    //-x
//	-10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
//	-10.0f, 2.0f,   -10.0f, 10.0f,  10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,
//
//	10.0f,  -10.0f, -10.0f, 1.0f,    //+x
//	10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
//	10.0f,  1.0f,   10.0f,  10.0f,  -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,
//
//	-10.0f, -10.0f, 10.0f,  5.0f,    // +z
//	-10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  10.0f,
//	10.0f,  5.0f,   10.0f,  -10.0f, 10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,
//
//	-10.0f, 10.0f,  -10.0f, 3.0f,    //+y
//	10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   10.0f,  10.0f,
//	10.0f,  3.0f,   -10.0f, 10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,
//
//	10.0f,  -10.0f, 10.0f,  4.0f,    //-y
//	10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
//	-10.0f, 4.0f,   -10.0f, -10.0f, 10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
//};
//
//bool gTakeScreenshot = false;
//
//void updateDescriptorSets()
//{
//	// Prepare descriptor sets
//	DescriptorData params[6] = {};
//	params[0].pName = "RightText";
//	params[0].ppTextures = &pSkyBoxTextures[0];
//	params[1].pName = "LeftText";
//	params[1].ppTextures = &pSkyBoxTextures[1];
//	params[2].pName = "TopText";
//	params[2].ppTextures = &pSkyBoxTextures[2];
//	params[3].pName = "BotText";
//	params[3].ppTextures = &pSkyBoxTextures[3];
//	params[4].pName = "FrontText";
//	params[4].ppTextures = &pSkyBoxTextures[4];
//	params[5].pName = "BackText";
//	params[5].ppTextures = &pSkyBoxTextures[5];
//	vk_updateDescriptorSet(pRenderer, 0, pDescriptorSetTexture, 6, params);
//
//	for (uint32_t i = 0; i < gImageCount; ++i)
//	{
//		DescriptorData params[1] = {};
//		params[0].pName = "uniformBlock";
//		params[0].ppBuffers = &pSkyboxUniformBuffer[i];
//		vk_updateDescriptorSet(pRenderer, i * 2 + 0, pDescriptorSetUniforms, 1, params);
//
//		params[0].pName = "uniformBlock";
//		params[0].ppBuffers = &pProjViewUniformBuffer[i];
//		vk_updateDescriptorSet(pRenderer, i * 2 + 1, pDescriptorSetUniforms, 1, params);
//	}
//}
//
//void initMarkers()
//{
//	BufferLoadDesc breadcrumbBuffer = {};
//	breadcrumbBuffer.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNDEFINED;
//	breadcrumbBuffer.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
//	breadcrumbBuffer.mDesc.mSize = (gMarkerCount + 3) / 4 * 4 * sizeof(uint32_t);
//	breadcrumbBuffer.mDesc.mFlags = BUFFER_CREATION_FLAG_NONE;
//	breadcrumbBuffer.mDesc.mStartState = RESOURCE_STATE_COPY_DEST;
//	breadcrumbBuffer.pData = NULL;
//
//	for (uint32_t i = 0; i < gImageCount; ++i)
//	{
//		breadcrumbBuffer.ppBuffer = &pMarkerBuffer[i];
//		addResource(&breadcrumbBuffer, NULL);
//	}
//}
//
//const char* GetAppName() { return "App"; };
//
//Application* Application::s_Instance = nullptr;
//Application::Application()
//{
//	ASSERT(!s_Instance);
//
//	s_Instance = this;
//
//	m_Window = GLWindow::Create();
//
//	//Timer t;
//	//initTimer(&t);
//
//	Init();
//	Load();
//
//}
//
//
//Application::~Application() {
//
//}
//
//void Application::Init()
//{
//	// FILE PATHS
//	fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders");
//	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SHADER_BINARIES, "CompiledShaders");
//	fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
//	fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
//	fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
//	fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");
//	fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");
//
//	// Generate sphere vertex buffer
//	generateCuboidPoints(&pSpherePoints, &gNumberOfSpherePoints);
//
//	// window and renderer setup
//	RendererDesc settings;
//	memset(&settings, 0, sizeof(settings));
//	settings.mD3D11Supported = false;
//	settings.mGLESSupported = false;
//
//	initRenderer(GetAppName(), &settings, &pRenderer);
//	//check for init success
//	if (!pRenderer)
//		LOGF(LogLevel::eERROR, "init Renderer failed!");
//
//	QueueDesc queueDesc = {};
//	queueDesc.mType = QUEUE_TYPE_GRAPHICS;
//	queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
//	vk_addQueue(pRenderer, &queueDesc, &pGraphicsQueue);
//
//	for (uint32_t i = 0; i < gImageCount; ++i)
//	{
//		CmdPoolDesc cmdPoolDesc = {};
//		cmdPoolDesc.pQueue = pGraphicsQueue;
//		vk_addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPools[i]);
//		CmdDesc cmdDesc = {};
//		cmdDesc.pPool = pCmdPools[i];
//		vk_addCmd(pRenderer, &cmdDesc, &pCmds[i]);
//
//		vk_addFence(pRenderer, &pRenderCompleteFences[i]);
//		vk_addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
//	}
//	vk_addSemaphore(pRenderer, &pImageAcquiredSemaphore);
//
//	initScreenshotInterface(pRenderer, pGraphicsQueue);
//
//	initResourceLoaderInterface(pRenderer);
//
//	// Loads Skybox Textures
//	for (size_t i = 0; i < 6; i++)
//	{
//		TextureLoadDesc textureDesc = {};
//		textureDesc.pFileName = pSkyBoxImageFileNames[i];
//		textureDesc.ppTexture = &pSkyBoxTextures[i];
//		// Textures representing color should be stored in SRGB or HDR format
//		textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
//		addResource(&textureDesc, NULL);
//	}
//
//	ShaderLoadDesc skyShader = {};
//	skyShader.mStages[0] = { "skybox.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
//	skyShader.mStages[1] = { "skybox.frag", NULL, 0 };
//	ShaderLoadDesc basicShader = {};
//	basicShader.mStages[0] = { "basic.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
//	basicShader.mStages[1] = { "basic.frag", NULL, 0 };
//	ShaderLoadDesc crashShader = {};
//	crashShader.mStages[0] = { "crash.vert", NULL, 0 };
//	crashShader.mStages[1] = { "basic.frag", NULL, 0 };
//
//	addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);
//	addShader(pRenderer, &basicShader, &pSphereShader);
//
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//		addShader(pRenderer, &crashShader, &pCrashShader);
//
//	SamplerDesc samplerDesc = { FILTER_LINEAR,
//								FILTER_LINEAR,
//								MIPMAP_MODE_NEAREST,
//								ADDRESS_MODE_CLAMP_TO_EDGE,
//								ADDRESS_MODE_CLAMP_TO_EDGE,
//								ADDRESS_MODE_CLAMP_TO_EDGE };
//
//	vk_addSampler(pRenderer, &samplerDesc, &pSamplerSkyBox);
//
//	Shader* shaders[3];
//	uint32_t shadersCount = 2;
//	shaders[0] = pSphereShader;
//	shaders[1] = pSkyBoxDrawShader;
//
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	{
//		shaders[2] = pCrashShader;
//		shadersCount = 3;
//	}
//
//	const char* pStaticSamplers[] = { "uSampler0" };
//	RootSignatureDesc rootDesc = {};
//	rootDesc.mStaticSamplerCount = 1;
//	rootDesc.ppStaticSamplerNames = pStaticSamplers;
//	rootDesc.ppStaticSamplers = &pSamplerSkyBox;
//	rootDesc.mShaderCount = shadersCount;
//	rootDesc.ppShaders = shaders;
//	vk_addRootSignature(pRenderer, &rootDesc, &pRootSignature);
//
//
//	DescriptorSetDesc desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
//	vk_addDescriptorSet(pRenderer, &desc, &pDescriptorSetTexture);
//	desc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount * 2 };
//	vk_addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
//
//	uint64_t       sphereDataSize = gNumberOfSpherePoints * sizeof(float);
//	BufferLoadDesc sphereVbDesc = {};
//	sphereVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
//	sphereVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
//	sphereVbDesc.mDesc.mSize = sphereDataSize;
//	sphereVbDesc.pData = pSpherePoints;
//	sphereVbDesc.ppBuffer = &pSphereVertexBuffer;
//	addResource(&sphereVbDesc, NULL);
//
//	// Need to free memory associated w/ sphere points.
//	tf_free(pSpherePoints);
//
//	uint64_t       skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
//	BufferLoadDesc skyboxVbDesc = {};
//	skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
//	skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
//	skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
//	skyboxVbDesc.pData = gSkyBoxPoints;
//	skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
//	addResource(&skyboxVbDesc, NULL);
//
//	BufferLoadDesc ubDesc = {};
//	ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//	ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
//	ubDesc.mDesc.mSize = sizeof(UniformBlock);
//	ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
//	ubDesc.pData = NULL;
//	for (uint32_t i = 0; i < gImageCount; ++i)
//	{
//		ubDesc.ppBuffer = &pProjViewUniformBuffer[i];
//		addResource(&ubDesc, NULL);
//		ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
//		addResource(&ubDesc, NULL);
//	}
//
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	{
//		// Initialize breadcrumb buffer to write markers in it.
//		initMarkers();
//	}
//
//	waitForAllResourceLoads();
//
//	{
//		// Setup planets (Rotation speeds are relative to Earth's, some values randomly given)
//			// Sun
//		gPlanetInfoData[0].mParentIndex = 0;
//		gPlanetInfoData[0].mYOrbitSpeed = 0;    // Earth years for one orbit
//		gPlanetInfoData[0].mZOrbitSpeed = 0;
//		gPlanetInfoData[0].mRotationSpeed = 24.0f;    // Earth days for one rotation
//		gPlanetInfoData[0].mTranslationMat = mat4::identity();
//		gPlanetInfoData[0].mScaleMat = mat4::scale(vec3(10.0f));
//		gPlanetInfoData[0].mColor = vec4(0.97f, 0.38f, 0.09f, 0.0f);
//
//		// Mercury
//		gPlanetInfoData[1].mParentIndex = 0;
//		gPlanetInfoData[1].mYOrbitSpeed = 0.5f;
//		gPlanetInfoData[1].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[1].mRotationSpeed = 58.7f;
//		gPlanetInfoData[1].mTranslationMat = mat4::translation(vec3(10.0f, 0, 0));
//		gPlanetInfoData[1].mScaleMat = mat4::scale(vec3(1.0f));
//		gPlanetInfoData[1].mColor = vec4(0.45f, 0.07f, 0.006f, 1.0f);
//
//		// Venus
//		gPlanetInfoData[2].mParentIndex = 0;
//		gPlanetInfoData[2].mYOrbitSpeed = 0.8f;
//		gPlanetInfoData[2].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[2].mRotationSpeed = 243.0f;
//		gPlanetInfoData[2].mTranslationMat = mat4::translation(vec3(20.0f, 0, 5));
//		gPlanetInfoData[2].mScaleMat = mat4::scale(vec3(2));
//		gPlanetInfoData[2].mColor = vec4(0.6f, 0.32f, 0.006f, 1.0f);
//
//		// Earth
//		gPlanetInfoData[3].mParentIndex = 0;
//		gPlanetInfoData[3].mYOrbitSpeed = 1.0f;
//		gPlanetInfoData[3].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[3].mRotationSpeed = 1.0f;
//		gPlanetInfoData[3].mTranslationMat = mat4::translation(vec3(30.0f, 0, 0));
//		gPlanetInfoData[3].mScaleMat = mat4::scale(vec3(4));
//		gPlanetInfoData[3].mColor = vec4(0.07f, 0.028f, 0.61f, 1.0f);
//
//		// Mars
//		gPlanetInfoData[4].mParentIndex = 0;
//		gPlanetInfoData[4].mYOrbitSpeed = 2.0f;
//		gPlanetInfoData[4].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[4].mRotationSpeed = 1.1f;
//		gPlanetInfoData[4].mTranslationMat = mat4::translation(vec3(40.0f, 0, 0));
//		gPlanetInfoData[4].mScaleMat = mat4::scale(vec3(3));
//		gPlanetInfoData[4].mColor = vec4(0.79f, 0.07f, 0.006f, 1.0f);
//
//		// Jupiter
//		gPlanetInfoData[5].mParentIndex = 0;
//		gPlanetInfoData[5].mYOrbitSpeed = 11.0f;
//		gPlanetInfoData[5].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[5].mRotationSpeed = 0.4f;
//		gPlanetInfoData[5].mTranslationMat = mat4::translation(vec3(50.0f, 0, 0));
//		gPlanetInfoData[5].mScaleMat = mat4::scale(vec3(8));
//		gPlanetInfoData[5].mColor = vec4(0.32f, 0.13f, 0.13f, 1);
//
//		// Saturn
//		gPlanetInfoData[6].mParentIndex = 0;
//		gPlanetInfoData[6].mYOrbitSpeed = 29.4f;
//		gPlanetInfoData[6].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[6].mRotationSpeed = 0.5f;
//		gPlanetInfoData[6].mTranslationMat = mat4::translation(vec3(60.0f, 0, 0));
//		gPlanetInfoData[6].mScaleMat = mat4::scale(vec3(6));
//		gPlanetInfoData[6].mColor = vec4(0.45f, 0.45f, 0.21f, 1.0f);
//
//		// Uranus
//		gPlanetInfoData[7].mParentIndex = 0;
//		gPlanetInfoData[7].mYOrbitSpeed = 84.07f;
//		gPlanetInfoData[7].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[7].mRotationSpeed = 0.8f;
//		gPlanetInfoData[7].mTranslationMat = mat4::translation(vec3(70.0f, 0, 0));
//		gPlanetInfoData[7].mScaleMat = mat4::scale(vec3(7));
//		gPlanetInfoData[7].mColor = vec4(0.13f, 0.13f, 0.32f, 1.0f);
//
//		// Neptune
//		gPlanetInfoData[8].mParentIndex = 0;
//		gPlanetInfoData[8].mYOrbitSpeed = 164.81f;
//		gPlanetInfoData[8].mZOrbitSpeed = 0.0f;
//		gPlanetInfoData[8].mRotationSpeed = 0.9f;
//		gPlanetInfoData[8].mTranslationMat = mat4::translation(vec3(80.0f, 0, 0));
//		gPlanetInfoData[8].mScaleMat = mat4::scale(vec3(8));
//		gPlanetInfoData[8].mColor = vec4(0.21f, 0.028f, 0.79f, 1.0f);
//
//		// Pluto - Not a planet XDD
//		gPlanetInfoData[9].mParentIndex = 0;
//		gPlanetInfoData[9].mYOrbitSpeed = 247.7f;
//		gPlanetInfoData[9].mZOrbitSpeed = 1.0f;
//		gPlanetInfoData[9].mRotationSpeed = 7.0f;
//		gPlanetInfoData[9].mTranslationMat = mat4::translation(vec3(90.0f, 0, 0));
//		gPlanetInfoData[9].mScaleMat = mat4::scale(vec3(1.0f));
//		gPlanetInfoData[9].mColor = vec4(0.45f, 0.21f, 0.21f, 1.0f);
//
//		// Moon
//		gPlanetInfoData[10].mParentIndex = 3;
//		gPlanetInfoData[10].mYOrbitSpeed = 1.0f;
//		gPlanetInfoData[10].mZOrbitSpeed = 200.0f;
//		gPlanetInfoData[10].mRotationSpeed = 27.0f;
//		gPlanetInfoData[10].mTranslationMat = mat4::translation(vec3(5.0f, 0, 0));
//		gPlanetInfoData[10].mScaleMat = mat4::scale(vec3(1));
//		gPlanetInfoData[10].mColor = vec4(0.07f, 0.07f, 0.13f, 1.0f);
//
//	}
//
//	CameraMotionParameters cmp{ 160.0f, 600.0f, 200.0f };
//	vec3                   camPos{ 48.0f, 48.0f, 20.0f };
//	vec3                   lookAt{ vec3(0) };
//
//	pCameraController = initFpsCameraController(camPos, lookAt);
//
//	pCameraController->setMotionParameters(cmp);
//
//	updateDescriptorSets();
//
//	gFrameIndex = 0;
//
//
//
//}
//
//bool Application::addSwapChain()
//{
//	SwapChainDesc swapChainDesc = {};
//	swapChainDesc.mWindow = ((GLFWwindow*)(m_Window->GetNativeWindow()));
//	swapChainDesc.mPresentQueueCount = 1;
//	swapChainDesc.ppPresentQueues = &pGraphicsQueue;
//	swapChainDesc.mWidth = 900;
//	swapChainDesc.mHeight = 600;
//	swapChainDesc.mImageCount = gImageCount;
//	swapChainDesc.mColorFormat = vk_getRecommendedSwapchainFormat(true, true);
//	swapChainDesc.mEnableVsync = false;
//	swapChainDesc.mFlags = SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
//	::vk_addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
//
//	return pSwapChain != NULL;
//}
//
//bool addDepthBuffer()
//{
//	// Add depth buffer
//	RenderTargetDesc depthRT = {};
//	depthRT.mArraySize = 1;
//	depthRT.mClearValue.depth = 0.0f;
//	depthRT.mClearValue.stencil = 0;
//	depthRT.mDepth = 1;
//	depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
//	depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
//	depthRT.mHeight = 600;
//	depthRT.mSampleCount = SAMPLE_COUNT_1;
//	depthRT.mSampleQuality = 0;
//	depthRT.mWidth = 900;
//	depthRT.mFlags = TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
//	vk_addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);
//
//	return pDepthBuffer != NULL;
//}
//
//void Application::Load()
//{
//	if (!addSwapChain())
//		LOGF(LogLevel::eERROR, "addSwapChain failed!");
//
//	if (!addDepthBuffer())
//		LOGF(LogLevel::eERROR, "addDepthBuffer failed!");
//
//	RenderTarget* ppPipelineRenderTargets[] =
//	{
//		pSwapChain->ppRenderTargets[0],
//		pDepthBuffer
//	};
//
//	//layout and pipeline for sphere draw
//	VertexLayout vertexLayout = {};
//	vertexLayout.mAttribCount = 2;
//	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
//	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
//	vertexLayout.mAttribs[0].mBinding = 0;
//	vertexLayout.mAttribs[0].mLocation = 0;
//	vertexLayout.mAttribs[0].mOffset = 0;
//	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
//	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
//	vertexLayout.mAttribs[1].mBinding = 0;
//	vertexLayout.mAttribs[1].mLocation = 1;
//	vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
//
//	RasterizerStateDesc rasterizerStateDesc = {};
//	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;
//
//	RasterizerStateDesc sphereRasterizerStateDesc = {};
//	sphereRasterizerStateDesc.mCullMode = CULL_MODE_FRONT;
//
//	DepthStateDesc depthStateDesc = {};
//	depthStateDesc.mDepthTest = true;
//	depthStateDesc.mDepthWrite = true;
//	depthStateDesc.mDepthFunc = CMP_GEQUAL;
//
//	PipelineDesc desc = {};
//	desc.mType = PIPELINE_TYPE_GRAPHICS;
//	GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
//	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
//	pipelineSettings.mRenderTargetCount = 1;
//	pipelineSettings.pDepthState = &depthStateDesc;
//	pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
//	pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
//	pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
//	pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
//	pipelineSettings.pRootSignature = pRootSignature;
//	pipelineSettings.pShaderProgram = pSphereShader;
//	pipelineSettings.pVertexLayout = &vertexLayout;
//	pipelineSettings.pRasterizerState = &sphereRasterizerStateDesc;
//	pipelineSettings.mVRFoveatedRendering = true;
//	vk_addPipeline(pRenderer, &desc, &pSpherePipeline);
//
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	{
//		pipelineSettings.pShaderProgram = pCrashShader;
//		vk_addPipeline(pRenderer, &desc, &pCrashPipeline);
//	}
//
//	//layout and pipeline for skybox draw
//	vertexLayout = {};
//	vertexLayout.mAttribCount = 1;
//	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
//	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
//	vertexLayout.mAttribs[0].mBinding = 0;
//	vertexLayout.mAttribs[0].mLocation = 0;
//	vertexLayout.mAttribs[0].mOffset = 0;
//
//	pipelineSettings.pDepthState = NULL;
//	pipelineSettings.pRasterizerState = &rasterizerStateDesc;
//	pipelineSettings.pShaderProgram = pSkyBoxDrawShader; //-V519
//	vk_addPipeline(pRenderer, &desc, &pSkyBoxDrawPipeline);
//}
//
//void Application::Close()
//{
//
//}
//
//void Update(float deltaTime)
//{
//	pCameraController->update(deltaTime);
//
//	static float currentTime = 0.0f;
//	currentTime += deltaTime * 1000.0f;
//
//	// update camera with time
//	mat4 viewMat = pCameraController->getViewMatrix();
//
//	const float aspectInverse = (float)600 / (float)900;
//	const float horizontal_fov = PI / 2.0f;
//	CameraMatrix projMat = CameraMatrix::perspective(horizontal_fov, aspectInverse, 1000.0f, 0.1f);
//	gUniformData.mProjectView = projMat * viewMat;
//
//	// point light parameters
//	gUniformData.mLightPosition = vec3(0, 0, 0);
//	gUniformData.mLightColor = vec3(0.9f, 0.9f, 0.7f);    // Pale Yellow
//
//	// update planet transformations
//	for (unsigned int i = 0; i < gNumPlanets; i++)
//	{
//		mat4 rotSelf, rotOrbitY, rotOrbitZ, trans, scale, parentMat;
//		rotSelf = rotOrbitY = rotOrbitZ = parentMat = mat4::identity();
//		if (gPlanetInfoData[i].mRotationSpeed > 0.0f)
//			rotSelf = mat4::rotationY(gRotSelfScale * (currentTime + gTimeOffset) / gPlanetInfoData[i].mRotationSpeed);
//		if (gPlanetInfoData[i].mYOrbitSpeed > 0.0f)
//			rotOrbitY = mat4::rotationY(gRotOrbitYScale * (currentTime + gTimeOffset) / gPlanetInfoData[i].mYOrbitSpeed);
//		if (gPlanetInfoData[i].mZOrbitSpeed > 0.0f)
//			rotOrbitZ = mat4::rotationZ(gRotOrbitZScale * (currentTime + gTimeOffset) / gPlanetInfoData[i].mZOrbitSpeed);
//		if (gPlanetInfoData[i].mParentIndex > 0)
//			parentMat = gPlanetInfoData[gPlanetInfoData[i].mParentIndex].mSharedMat;
//
//		trans = gPlanetInfoData[i].mTranslationMat;
//		scale = gPlanetInfoData[i].mScaleMat;
//
//		gPlanetInfoData[i].mSharedMat = parentMat * rotOrbitY * trans;
//		gUniformData.mToWorldMat[i] = parentMat * rotOrbitY * rotOrbitZ * trans * rotSelf * scale;
//		gUniformData.mColor[i] = gPlanetInfoData[i].mColor;
//	}
//
//	viewMat.setTranslation(vec3(0));
//	gUniformDataSky = gUniformData;
//	gUniformDataSky.mProjectView = projMat * viewMat;
//}
//
//void Draw()
//{
//	if (pSwapChain->mEnableVsync != false)
//	{
//		vk_waitQueueIdle(pGraphicsQueue);
//		::vk_toggleVSync(pRenderer, &pSwapChain);
//	}
//
//	uint32_t swapchainImageIndex;
//	vk_acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);
//
//	RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
//	Semaphore* pRenderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
//	Fence* pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];
//
//	// Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
//	FenceStatus fenceStatus;
//	vk_getFenceStatus(pRenderer, pRenderCompleteFence, &fenceStatus);
//	if (fenceStatus == FENCE_STATUS_INCOMPLETE)
//		vk_waitForFences(pRenderer, 1, &pRenderCompleteFence);
//
//	//if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	//{
//	//	// Check breadcrumb markers
//	//	checkMarkers();
//	//}
//
//	// Update uniform buffers
//	BufferUpdateDesc viewProjCbv = { pProjViewUniformBuffer[gFrameIndex] };
//	beginUpdateResource(&viewProjCbv);
//	*(UniformBlock*)viewProjCbv.pMappedData = gUniformData;
//	endUpdateResource(&viewProjCbv, NULL);
//
//	// Sky box data
//	BufferUpdateDesc skyboxViewProjCbv = { pSkyboxUniformBuffer[gFrameIndex] };
//	beginUpdateResource(&skyboxViewProjCbv);
//	*(UniformBlock*)skyboxViewProjCbv.pMappedData = gUniformDataSky;
//	endUpdateResource(&skyboxViewProjCbv, NULL);
//
//	// Reset cmd pool for this frame
//	vk_resetCmdPool(pRenderer, pCmdPools[gFrameIndex]);
//
//	Cmd* cmd = pCmds[gFrameIndex];
//	vk_beginCmd(cmd);
//
//	//if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	//{
//	//	// Reset markers values
//	//	resetMarkers(cmd);
//	//}
//
//	//cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);
//
//	RenderTargetBarrier barriers[] = {
//		{ pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
//	};
//	vk_cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
//
//	// simply record the screen cleaning command
//	LoadActionsDesc loadActions = {};
//	loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
//	loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
//	loadActions.mClearDepth.depth = 0.0f;
//	loadActions.mClearDepth.stencil = 0;
//	vk_cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthBuffer, &loadActions, NULL, NULL, -1, -1);
//	vk_cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
//	vk_cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);
//
//	const uint32_t sphereVbStride = sizeof(float) * 6;
//	const uint32_t skyboxVbStride = sizeof(float) * 4;
//
//	// draw skybox
//	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
//	vk_cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 1.0f, 1.0f);
//	vk_cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
//	vk_cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
//	vk_cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
//	vk_cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
//	vk_cmdDraw(cmd, 36, 0);
//	vk_cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
//	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
//
//	////// draw planets
//	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Planets");
//
//	Pipeline* pipeline = pSpherePipeline;
//
//	// Using the malfunctioned pipeline
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs && bSimulateCrash)
//	{
//		gCrashedFrame = gFrameIndex;
//		bSimulateCrash = false;
//		bHasCrashed = true;
//		pipeline = pCrashPipeline;
//		LOGF(LogLevel::eERROR, "[Breadcrumb] Simulating a GPU crash situation...");
//	}
//
//	vk_cmdBindPipeline(cmd, pipeline);
//	vk_cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 1, pDescriptorSetUniforms);
//	vk_cmdBindVertexBuffer(cmd, 1, &pSphereVertexBuffer, &sphereVbStride, NULL);
//
//	if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	{
//		// Marker on top of the pip, won't wait for the following draw commands.
//		vk_cmdWriteMarker(cmd, MARKER_TYPE_IN, gValidMarkerValue, pMarkerBuffer[gFrameIndex], 0, false);
//	}
//
//	vk_cmdDrawInstanced(cmd, gNumberOfSpherePoints / 6, 0, gNumPlanets, 0);
//
//	//if (pRenderer->pActiveGpuSettings->mGpuBreadcrumbs)
//	//{
//	//	// Marker on bottom of the pip, will wait for draw command to be executed.
//	//	vk_cmdWriteMarker(cmd, MARKER_TYPE_OUT, gValidMarkerValue, pMarkerBuffer[gFrameIndex], 1, false);
//	//}
//
//	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
//
//	loadActions = {};
//	loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
//	vk_cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
//	//cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");
//
//	//gFrameTimeDraw.mFontColor = 0xff00ffff;
//	//gFrameTimeDraw.mFontSize = 18.0f;
//	//gFrameTimeDraw.mFontID = gFontID;
//	/*float2 txtSizePx = cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
//	cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gGpuProfileToken, &gFrameTimeDraw);
//
//	cmdDrawUserInterface(cmd);*/
//
//	vk_cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
//	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
//
//	barriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
//	vk_cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
//
//	//cmdEndGpuFrameProfile(cmd, gGpuProfileToken);
//	vk_endCmd(cmd);
//
//	QueueSubmitDesc submitDesc = {};
//	submitDesc.mCmdCount = 1;
//	submitDesc.mSignalSemaphoreCount = 1;
//	submitDesc.mWaitSemaphoreCount = 1;
//	submitDesc.ppCmds = &cmd;
//	submitDesc.ppSignalSemaphores = &pRenderCompleteSemaphore;
//	submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
//	submitDesc.pSignalFence = pRenderCompleteFence;
//	vk_queueSubmit(pGraphicsQueue, &submitDesc);
//	QueuePresentDesc presentDesc = {};
//	presentDesc.mIndex = swapchainImageIndex;
//	presentDesc.mWaitSemaphoreCount = 1;
//	presentDesc.pSwapChain = pSwapChain;
//	presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
//	presentDesc.mSubmitDone = true;
//
//	// captureScreenshot() must be used before presentation.
//	//if (gTakeScreenshot)
//	//{
//	//	// Metal platforms need one renderpass to prepare the swapchain textures for copy.
//	//	if (prepareScreenshot(pSwapChain))
//	//	{
//	//		captureScreenshot(pSwapChain, swapchainImageIndex, RESOURCE_STATE_PRESENT, "01_Transformations_Screenshot.png");
//	//		gTakeScreenshot = false;
//	//	}
//	//}
//
//	vk_queuePresent(pGraphicsQueue, &presentDesc);
//	//flipProfiler();
//
//	gFrameIndex = (gFrameIndex + 1) % gImageCount;
//}
//
//void Application::Run()
//{
//	while (m_Running)
//	{
//		m_Window->OnUpdate();
//		float deltaTime = 0.05f;
//		Update(deltaTime);
//		Draw();
//	}
//}