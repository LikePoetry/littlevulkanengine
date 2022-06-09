#pragma once
#include "../Renderer/Include/RendererConfig.h"

#include "../OS/Interfaces/IThread.h"

// Forward declarations
typedef struct Renderer           Renderer;
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
	Renderer* pRenderer;
	Queue* pQueue;
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

typedef struct GPUSettings
{
	uint32_t				mUniformBufferAlignment;
	uint32_t				mUploadBufferTextureAlignment;
	uint32_t				mUploadBufferTextureRowAlignment;
	uint32_t				mMaxVertexInputBindings;
	uint32_t				mMaxRootSignatureDWORDS;
} GPUSettings;

typedef struct DEFINE_ALIGNED(Renderer, 64)
{
	struct
	{
		VkInstance						pVkInstance;
		VkPhysicalDevice				pVkActiveGPU;
		VkPhysicalDeviceProperties2* pVkActiveGPUProperties;
		VkDevice					pVkDevice;
		VkDebugUtilsMessengerEXT	pVkDebugUtilsMessenger;
		uint32_t**				pAvailableQueueCount;
		uint32_t**				pUsedQueueCount;
		VkDescriptorPool		pEmptyDescriptorPool;
		VkDescriptorSetLayout	pEmptyDescriptorSetLayout;
		VkDescriptorSet			pEmptyDescriptorSet;
		struct VmaAllocator_T* pVmaAllocator;
		uint32_t               mRaytracingSupported : 1;
		uint32_t               mYCbCrExtension : 1;
		uint32_t               mKHRSpirv14Extension : 1;
		uint32_t               mKHRAccelerationStructureExtension : 1;
		uint32_t               mKHRRayTracingPipelineExtension : 1;
		uint32_t               mKHRRayQueryExtension : 1;
		uint32_t               mAMDGCNShaderExtension : 1;
		uint32_t               mAMDDrawIndirectCountExtension : 1;
		uint32_t               mDescriptorIndexingExtension : 1;
		uint32_t               mShaderFloatControlsExtension : 1;
		uint32_t               mBufferDeviceAddressExtension : 1;
		uint32_t               mDeferredHostOperationsExtension : 1;
		uint32_t               mDrawIndirectCountExtension : 1;
		uint32_t               mDedicatedAllocationExtension : 1;
		uint32_t               mExternalMemoryExtension : 1;
		uint32_t               mDebugMarkerSupport : 1;
		uint32_t               mOwnInstance : 1;

		union
		{
			struct
			{
				uint8_t mGraphicsQueueFamilyIndex;
				uint8_t mTransferQueueFamilyIndex;
				uint8_t mComputeQueueFamilyIndex;
			};
			uint8_t mQueueFamilyIndices[3];
		} ;
	}mVulkan;

	struct NullDescriptors* pNullDescriptors;
	char* pName;
	GPUSettings* pActiveGpuSettings;
	/*ShaderMacro* pBuiltinShaderDefines;
	GPUCapBits* pCapBits;*/
	uint32_t                mLinkedNodeCount : 4;
	uint32_t                mUnlinkedRendererIndex : 4;
	uint32_t                mGpuMode : 3;
	uint32_t                mShaderTarget : 4;
	uint32_t                mEnableGpuBasedValidation : 1;
	char* pApiName;
	uint32_t                mBuiltinShaderDefinesCount;

} Renderer;