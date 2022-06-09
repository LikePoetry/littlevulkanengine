#pragma once
#include "../Renderer/Include/RendererConfig.h"

#include "../OS/Interfaces/IThread.h"

// Forward declarations
typedef struct Queue              Queue;
typedef struct Texture            Texture;

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
				/// Contains resource allocation info such as parent heap, offset in heap
				struct VmaAllocation_T* pVkAllocation;
				VkDeviceMemory pVkDeviceMemory;
			};
		} mVulkan;
	};

} Texture;

typedef struct CmdPool
{
	VkCommandPool pVkCmdPool;

	Queue* pQueue;
} CmdPool;

typedef struct DEFINE_ALIGNED(Cmd, 64)
{
	union
	{
		struct
		{
			VkCommandBuffer pVkCmdBuffer;
			VkRenderPass pVkActiveRenderPass;
			VkPipelineLayout pBoundPipelineLayout;
			uint32_t mNodeIndex : 4;
			uint32_t mType : 3;
			uint32_t mPadA;
			CmdPool* pCmdPool;
			uint64_t mPadB[9];
		};
	};
} Cmd;

typedef struct Queue
{
	union
	{
		struct
		{
			VkQueue pVkQueue;
			Mutex* pSubmitMutex;
			uint32_t mFlags;
			float    mTimestampPeriod;
			uint32_t mVkQueueFamilyIndex : 5;
			uint32_t mVkQueueIndex : 5;
			uint32_t mGpuMode : 3;
		} mVulkan;

	};
	uint32_t mType : 3;
	uint32_t mNodeIndex : 4;
} Queue;


