#pragma once
#ifndef FORGE_RENDERER_CONFIG_H
#error "VulkanConfig should be included from RendererConfig only"
#endif

#define VULKAN

#if defined(_WINDOWS) || defined(XBOX)
//#define VK_USE_PLATFORM_WIN32_KHR

#define VK_USE_GLFW_KHR
#elif defined(__ANDROID__)
#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#elif defined(__linux__) && !defined(VK_USE_PLATFORM_GGP)
// TODO: Separate vulkan ext from choosing xlib vs xcb
#define VK_USE_PLATFORM_XLIB_KHR    //Use Xlib or Xcb as display server, defaults to Xlib
#endif

#if defined(NX64)
#define VK_USE_PLATFORM_VI_NN
#include <vulkan/vulkan.h>
#include "../../../Switch/Common_3/Renderer/Vulkan/NX/NXVulkanExt.h"
#else
#include "../../ThirdParty/OpenSource/volk/volk.h"
#endif


// #define USE_EXTERNAL_MEMORY_EXTENSIONS
#ifdef USE_EXTERNAL_MEMORY_EXTENSIONS
#define VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME "Memory ext name"
#define VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME "Semaphore ext name"
#define VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME "Fence ext name"
#endif

#define TARGET_VULKAN_API_VERSION VK_API_VERSION_1_1

/************************************************************************/
// Debugging Macros
/************************************************************************/
// Uncomment this to enable render doc capture support
//#define ENABLE_RENDER_DOC

// Debug Utils Extension not present on many Android devices
#if !defined(__ANDROID__)
#define ENABLE_DEBUG_UTILS_EXTENSION
#endif


//////////////////////////////////////////////
//// Availability macros
//////////////////////////////////////////////

#if defined(VK_KHR_ray_tracing_pipeline) && defined(VK_KHR_acceleration_structure)
#define VK_RAYTRACING_AVAILABLE
#endif

#if defined(_WINDOWS) || defined(__linux__)
#define NSIGHT_AFTERMATH_AVAILABLE
#endif

#define CHECK_VKRESULT(exp)                                                      \
	{                                                                            \
		VkResult vkres = (exp);                                                  \
		if (VK_SUCCESS != vkres)                                                 \
		{                                                                        \
			LOGF(eERROR, "%s: FAILED with VkResult: %d", #exp, vkres); \
			ASSERT(false);                                                       \
		}                                                                        \
	}
