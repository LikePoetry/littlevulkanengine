#include "../Include/RendererConfig.h"


#ifdef VULKAN
#define RENDERER_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "../Include/IRenderer.h"

#include "../../OS/Interfaces/ILog.h"



void vk_waitQueueIdle(Queue* pQueue) { vkQueueWaitIdle(pQueue->mVulkan.pVkQueue); }

#include "../ThirdParty/OpenSource/volk/volk.c"
#endif