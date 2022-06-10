#pragma once
#include "../Renderer/Include/RendererConfig.h"

#include "../OS/Interfaces/ILog.h"
#include "../OS/Interfaces/IThread.h"

#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"

// default capability levels of the renderer
enum
{
	MAX_INSTANCE_EXTENSIONS = 64,
	MAX_DEVICE_EXTENSIONS = 64,
	/// Max number of GPUs in SLI or Cross-Fire
	MAX_LINKED_GPUS = 4,
	/// Max number of GPUs in unlinked mode
	MAX_UNLINKED_GPUS = 4,
	/// Max number of GPus for either linked or unlinked mode.
	MAX_MULTIPLE_GPUS = 4,
	MAX_RENDER_TARGET_ATTACHMENTS = 8,
	MAX_VERTEX_BINDINGS = 15,
	MAX_VERTEX_ATTRIBS = 15,
	MAX_RESOURCE_NAME_LENGTH = 256,
	MAX_SEMANTIC_NAME_LENGTH = 128,
	MAX_DEBUG_NAME_LENGTH = 128,
	MAX_MIP_LEVELS = 0xFFFFFFFF,
	MAX_SWAPCHAIN_IMAGES = 3,
	MAX_GPU_VENDOR_STRING_LENGTH = 256,    //max size for GPUVendorPreset strings
	MAX_PLANE_COUNT = 3,
	MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1,
};

typedef void (*LogFn)(LogLevel, const char*, const char*);

// Forward declarations
typedef struct RendererContext    RendererContext;
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

typedef struct ShaderMacro
{
	const char* definition;
	const char* value;
} ShaderMacro;

typedef enum ShadingRate
{
	SHADING_RATE_NOT_SUPPORTED = 0x0,
	SHADING_RATE_FULL = 0x1,
	SHADING_RATE_HALF = SHADING_RATE_FULL << 1,
	SHADING_RATE_QUARTER = SHADING_RATE_HALF << 1,
	SHADING_RATE_EIGHTH = SHADING_RATE_QUARTER << 1,
	SHADING_RATE_1X2 = SHADING_RATE_EIGHTH << 1,
	SHADING_RATE_2X1 = SHADING_RATE_1X2 << 1,
	SHADING_RATE_2X4 = SHADING_RATE_2X1 << 1,
	SHADING_RATE_4X2 = SHADING_RATE_2X4 << 1,
} ShadingRate;

typedef enum ShadingRateCaps
{
	SHADING_RATE_CAPS_NOT_SUPPORTED = 0x0,
	SHADING_RATE_CAPS_PER_DRAW = 0x1,
	SHADING_RATE_CAPS_PER_TILE = SHADING_RATE_CAPS_PER_DRAW << 1,
} ShadingRateCaps;

typedef enum GPUPresetLevel
{
	GPU_PRESET_NONE = 0,
	GPU_PRESET_OFFICE,    //This means unsupported
	GPU_PRESET_LOW,
	GPU_PRESET_MEDIUM,
	GPU_PRESET_HIGH,
	GPU_PRESET_ULTRA,
	GPU_PRESET_COUNT
} GPUPresetLevel;

typedef struct GPUVendorPreset
{
	char           mVendorId[MAX_GPU_VENDOR_STRING_LENGTH];
	char           mModelId[MAX_GPU_VENDOR_STRING_LENGTH];
	char           mRevisionId[MAX_GPU_VENDOR_STRING_LENGTH];    // Optional as not all gpu's have that. Default is : 0x00
	GPUPresetLevel mPresetLevel;
	char           mGpuName[MAX_GPU_VENDOR_STRING_LENGTH];    //If GPU Name is missing then value will be empty string
	char           mGpuDriverVersion[MAX_GPU_VENDOR_STRING_LENGTH];
	char           mGpuDriverDate[MAX_GPU_VENDOR_STRING_LENGTH];
} GPUVendorPreset;

typedef enum ShaderTarget
{
	// 5.1 is supported on all DX12 hardware
	shader_target_5_1,
	shader_target_6_0,
	shader_target_6_1,
	shader_target_6_2,
	shader_target_6_3,    //required for Raytracing
	shader_target_6_4,    //required for VRS
} ShaderTarget;

typedef enum GpuMode
{
	GPU_MODE_SINGLE = 0,
	GPU_MODE_LINKED,
	GPU_MODE_UNLINKED,
} GpuMode;

typedef struct RendererDesc
{
	struct
	{
		const char** ppInstanceLayers;
		const char** ppInstanceExtensions;
		const char** ppDeviceExtensions;
		uint32_t	mInstanceLayerCount;
		uint32_t	mInstanceExtensionCount;
		uint32_t	mDeviceExtensionCount;
		/// Flag to specify whether to request all queues from the gpu or just one of each type
		/// This will affect memory usage - Around 200 MB more used if all queues are requested
		bool mRequestAllAvailableQueues;
	} mVulkan;

	LogFn			pLogFn;
	ShaderTarget	mShaderTarget;
	GpuMode			mGpuMode;

	/// Required when creating unlinked multiple renderers. Optional otherwise, can be used for explicit GPU selection.
	RendererContext* pContext;
	uint32_t         mGpuIndex;

	/// This results in new validation not possible during API calls on the CPU, by creating patched shaders that have validation added directly to the shader.
	/// However, it can slow things down a lot, especially for applications with numerous PSOs. Time to see the first render frame may take several minutes
	bool mEnableGPUBasedValidation;

	bool mD3D11Supported;
	bool mGLESSupported;

} RendererDesc;

typedef struct GPUCapBits
{
	bool canShaderReadFrom[TinyImageFormat_Count];
	bool canShaderWriteTo[TinyImageFormat_Count];
	bool canRenderTargetWriteTo[TinyImageFormat_Count];
} GPUCapBits;

typedef enum WaveOpsSupportFlags
{
	WAVE_OPS_SUPPORT_FLAG_NONE = 0x0,
	WAVE_OPS_SUPPORT_FLAG_BASIC_BIT = 0x00000001,
	WAVE_OPS_SUPPORT_FLAG_VOTE_BIT = 0x00000002,
	WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT = 0x00000004,
	WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT = 0x00000008,
	WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT = 0x00000010,
	WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT = 0x00000020,
	WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT = 0x00000040,
	WAVE_OPS_SUPPORT_FLAG_QUAD_BIT = 0x00000080,
	WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV = 0x00000100,
	WAVE_OPS_SUPPORT_FLAG_ALL = 0x7FFFFFFF
} WaveOpsSupportFlags;

typedef struct GPUSettings
{
	uint32_t				mUniformBufferAlignment;
	uint32_t				mUploadBufferTextureAlignment;
	uint32_t				mUploadBufferTextureRowAlignment;
	uint32_t				mMaxVertexInputBindings;
	uint32_t				mMaxRootSignatureDWORDS;
	uint32_t				mWaveLaneCount;
	WaveOpsSupportFlags		mWaveOpsSupportFlags;
	GPUVendorPreset			mGpuVendorPreset;
	// Variable Rate Shading
	ShadingRate     mShadingRates;
	ShadingRateCaps mShadingRateCaps;
	uint32_t        mShadingRateTexelWidth;
	uint32_t        mShadingRateTexelHeight;

	uint32_t mMultiDrawIndirect : 1;
	uint32_t mROVsSupported : 1;
	uint32_t mTessellationSupported : 1;
	uint32_t mGeometryShaderSupported : 1;
	uint32_t mGpuBreadcrumbs : 1;
	uint32_t mHDRSupported : 1;

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
		uint32_t** pAvailableQueueCount;
		uint32_t** pUsedQueueCount;
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
		};
	}mVulkan;

	struct NullDescriptors* pNullDescriptors;
	char* pName;
	GPUSettings* pActiveGpuSettings;
	ShaderMacro* pBuiltinShaderDefines;
	GPUCapBits* pCapBits;
	uint32_t                mLinkedNodeCount : 4;
	uint32_t                mUnlinkedRendererIndex : 4;
	uint32_t                mGpuMode : 3;
	uint32_t                mShaderTarget : 4;
	uint32_t                mEnableGpuBasedValidation : 1;
	char* pApiName;
	uint32_t                mBuiltinShaderDefinesCount;

} Renderer;

typedef struct GpuInfo
{
	struct
	{
		VkPhysicalDevice             pGPU;
		VkPhysicalDeviceProperties2  mGPUProperties;
	} mVulkan;
	GPUSettings mSettings;
} GpuInfo;

typedef struct DEFINE_ALIGNED(RendererContext, 64)
{
	struct
	{
		VkInstance	pVkInstance;
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
		VkDebugUtilsMessengerEXT pVkDebugUtilsMessenger;
#else
		VkDebugReportCallbackEXT pVkDebugReport;
#endif
	}mVulkan;

	GpuInfo* pGpus;
	uint32_t mGpuCount;
}RendererContext;

//#ifdef __INTELLISENSE__
//// IntelliSense is the code completion engine in Visual Studio. When it parses the source files, __INTELLISENSE__ macro is defined.
//// Here we trick IntelliSense into thinking that the renderer functions are not function pointers, but just regular functions.
//// What this achieves is filtering out duplicated function names from code completion results and improving the code completion for function parameters.
//// This dramatically improves the quality of life for Visual Studio users.
//#define DECLARE_RENDERER_FUNCTION(ret, name, ...)                     \
//	ret name(__VA_ARGS__);
//#else
//#define DECLARE_RENDERER_FUNCTION(ret, name, ...)                     \
//	typedef API_INTERFACE ret(FORGE_CALLCONV* name##Fn)(__VA_ARGS__); \
//	extern name##Fn       name;
//#endif

// Multiple renderer API (optional)


// queue/fence/swapchain functions
//DECLARE_RENDERER_FUNCTION(void, waitQueueIdle, Queue* p_queue)
void vk_waitQueueIdle(Queue* p_queue);
