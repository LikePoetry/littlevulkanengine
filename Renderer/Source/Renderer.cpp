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