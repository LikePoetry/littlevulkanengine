#pragma once
#include "../Renderer/Include/RendererConfig.h"

typedef struct DEFINE_ALIGNED(Texture, 64)
{
	union
	{
		struct
		{
			/// Opaque handle used by shaders for doing read/write operations on the texture
			VkImageView pVkSRVDescriptor;
			/// Opaque handle used by shaders for doing read/write operations on the texture
			VkImageView* pVkUAVDescriptors;
			/// Opaque handle used by shaders for doing read/write operations on the texture
			VkImageView pVkSRVStencilDescriptor;
			/// Native handle of the underlying resource
			VkImage pVkImage;
			union 
			{
				VkDeviceMemory pVkDeviceMemory;
			};
		} mVulkan;
	};
};

typedef struct DEFINE_ALIGNED(Cmd, 64)
{

} Cmd;