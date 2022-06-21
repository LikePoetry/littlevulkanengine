#pragma once
#define VULKAN

#define VK_USE_PLATFORM_WIN32_KHR

#include "../../ThirdParty/OpenSource/volk/volk.h"

#define TARGET_VULKAN_API_VERSION VK_API_VERSION_1_1

#define ENABLE_DEBUG_UTILS_EXTENSION

#define VK_RAYTRACING_AVAILABLE

#define CHECK_VKRESULT(exp)                                                      \
	{                                                                            \
		VkResult vkres = (exp);                                                  \
		if (VK_SUCCESS != vkres)                                                 \
		{                                                                        \
			LOGF(eERROR, "%s: FAILED with VkResult: %d", #exp, vkres); \
			ASSERT(false);                                                       \
		}                                                                        \
	}