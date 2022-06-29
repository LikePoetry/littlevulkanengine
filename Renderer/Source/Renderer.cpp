#include "../Include/RendererConfig.h"

#include "../Include/IRenderer.h"
#include "../Include/IRay.h"


/************************************************************************/
// Internal initialization settings
/************************************************************************/
RendererApi gSelectedRendererApi;
bool        gD3D11Unsupported = false;
bool        gGLESUnsupported = false;

extern void initVulkanRenderer(const char* appName, const RendererDesc* pSettings, Renderer** ppRenderer);
extern void exitVulkanRenderer(Renderer* pRenderer);

static bool apiIsUnsupported(const RendererApi api)
{
	return false;
}

static void initRendererAPI(const char* appName, const RendererDesc* pSettings, Renderer** ppRenderer, const RendererApi api)
{
	//initVulkanRaytracingFunctions();
	initVulkanRenderer(appName, pSettings, ppRenderer);
}

void initRenderer(const char* appName, const RendererDesc* pSettings, Renderer** ppRenderer)
{
	ASSERT(ppRenderer);
	ASSERT(*ppRenderer == NULL);

	ASSERT(pSettings);

	gD3D11Unsupported = !pSettings->mD3D11Supported;
	gGLESUnsupported = !pSettings->mGLESSupported;

	// Init requested renderer API
	if (!apiIsUnsupported(gSelectedRendererApi))
	{
		initRendererAPI(appName, pSettings, ppRenderer, gSelectedRendererApi);
	}
	else
	{
		LOGF(LogLevel::eWARNING, "Requested Graphics API has been marked as disabled and/or not supported in the Renderer's descriptor!");
		LOGF(LogLevel::eWARNING, "Falling back to the first available API...");
	}

#if defined(USE_MULTIPLE_RENDER_APIS)
	// Fallback on other available APIs
	for (uint32_t i = 0; i < RENDERER_API_COUNT && !*ppRenderer; ++i)
	{
		if (i == gSelectedRendererApi || apiIsUnsupported((RendererApi)i))
			continue;

		gSelectedRendererApi = (RendererApi)i;
		initRendererAPI(appName, pSettings, ppRenderer, gSelectedRendererApi);
	}
#endif
}

static void exitRendererAPI(Renderer* pRenderer, const RendererApi api) 
{
	switch (api)
	{
#if defined(DIRECT3D11)
	case RENDERER_API_D3D11:
		exitD3D11Renderer(pRenderer);
		break;
#endif
#if defined(DIRECT3D12)
	case RENDERER_API_D3D12:
		exitD3D12Renderer(pRenderer);
		break;
#endif
#if defined(VULKAN)
	case RENDERER_API_VULKAN:
		exitVulkanRenderer(pRenderer);
		break;
#endif
#if defined(METAL)
	case RENDERER_API_METAL:
		exitMetalRenderer(pRenderer);
		break;
#endif
#if defined(GLES)
	case RENDERER_API_GLES:
		exitGLESRenderer(pRenderer);
		break;
#endif
#if defined(ORBIS)
	case RENDERER_API_ORBIS:
		exitOrbisRenderer(pRenderer);
		break;
#endif
#if defined(PROSPERO)
	case RENDERER_API_PROSPERO:
		exitProsperoRenderer(pRenderer);
		break;
#endif
	default:
		LOGF(LogLevel::eERROR, "No Renderer API defined!");
		break;
	}
}

void exitRenderer(Renderer* pRenderer)
{
	ASSERT(pRenderer);

	exitRendererAPI(pRenderer, gSelectedRendererApi);

	gD3D11Unsupported = false;
	gGLESUnsupported = false;
}

uint32_t getDescriptorIndexFromName(const RootSignature* pRootSignature, const char* pName)
{
	for (uint32_t i = 0; i < pRootSignature->mDescriptorCount; ++i)
	{
		if (!strcmp(pName, pRootSignature->pDescriptors[i].pName))
		{
			return i;
		}
	}

	return UINT32_MAX;
}