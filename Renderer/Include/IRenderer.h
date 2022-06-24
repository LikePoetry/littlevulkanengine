#pragma once
#include "../Renderer/Include/RendererConfig.h"

#include "../OS/Interfaces/ILog.h"
#include "../OS/Interfaces/IThread.h"

#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"

#include "../../ThirdParty/OpenSource/VulkanMemoryAllocator/VulkanMemoryAllocator.h"

#define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE)                                                                        \
	static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) | (TYPE)(b)); } \
	static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) & (TYPE)(b)); } \
	static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a | b); }                      \
	static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a & b); }

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

typedef enum RendererApi
{
	RENDERER_API_VULKAN,
	RENDERER_API_COUNT
} RendererApi;

typedef enum QueueType
{
	QUEUE_TYPE_GRAPHICS = 0,
	QUEUE_TYPE_TRANSFER,
	QUEUE_TYPE_COMPUTE,
	MAX_QUEUE_TYPE
} QueueType;

typedef enum QueueFlag
{
	QUEUE_FLAG_NONE = 0x0,
	QUEUE_FLAG_DISABLE_GPU_TIMEOUT = 0x1,
	QUEUE_FLAG_INIT_MICROPROFILE = 0x2,
	MAX_QUEUE_FLAG = 0xFFFFFFFF
} QueueFlag;
MAKE_ENUM_FLAG(uint32_t, QueueFlag)

typedef enum QueuePriority
{
	QUEUE_PRIORITY_NORMAL,
	QUEUE_PRIORITY_HIGH,
	QUEUE_PRIORITY_GLOBAL_REALTIME,
	MAX_QUEUE_PRIORITY
} QueuePriority;

typedef void (*LogFn)(LogLevel, const char*, const char*);

typedef enum ResourceState
{
	RESOURCE_STATE_UNDEFINED = 0,
	RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
	RESOURCE_STATE_INDEX_BUFFER = 0x2,
	RESOURCE_STATE_RENDER_TARGET = 0x4,
	RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
	RESOURCE_STATE_DEPTH_WRITE = 0x10,
	RESOURCE_STATE_DEPTH_READ = 0x20,
	RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
	RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
	RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
	RESOURCE_STATE_STREAM_OUT = 0x100,
	RESOURCE_STATE_INDIRECT_ARGUMENT = 0x200,
	RESOURCE_STATE_COPY_DEST = 0x400,
	RESOURCE_STATE_COPY_SOURCE = 0x800,
	RESOURCE_STATE_GENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
	RESOURCE_STATE_PRESENT = 0x1000,
	RESOURCE_STATE_COMMON = 0x2000,
	RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
	RESOURCE_STATE_SHADING_RATE_SOURCE = 0x8000,
} ResourceState;

/// Choosing Memory Type
typedef enum ResourceMemoryUsage
{
	/// No intended memory usage specified.
	RESOURCE_MEMORY_USAGE_UNKNOWN = 0,
	/// Memory will be used on device only, no need to be mapped on host.
	RESOURCE_MEMORY_USAGE_GPU_ONLY = 1,
	/// Memory will be mapped on host. Could be used for transfer to device.
	RESOURCE_MEMORY_USAGE_CPU_ONLY = 2,
	/// Memory will be used for frequent (dynamic) updates from host and reads on device.
	RESOURCE_MEMORY_USAGE_CPU_TO_GPU = 3,
	/// Memory will be used for writing on device and readback on host.
	RESOURCE_MEMORY_USAGE_GPU_TO_CPU = 4,
	RESOURCE_MEMORY_USAGE_COUNT,
	RESOURCE_MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF
} ResourceMemoryUsage;

typedef enum PipelineType
{
	PIPELINE_TYPE_UNDEFINED = 0,
	PIPELINE_TYPE_COMPUTE,
	PIPELINE_TYPE_GRAPHICS,
	PIPELINE_TYPE_RAYTRACING,
	PIPELINE_TYPE_COUNT,
} PipelineType;

typedef enum FilterType
{
	FILTER_NEAREST = 0,
	FILTER_LINEAR,
} FilterType;

typedef enum AddressMode
{
	ADDRESS_MODE_MIRROR,
	ADDRESS_MODE_REPEAT,
	ADDRESS_MODE_CLAMP_TO_EDGE,
	ADDRESS_MODE_CLAMP_TO_BORDER
} AddressMode;

typedef enum MipMapMode
{
	MIPMAP_MODE_NEAREST = 0,
	MIPMAP_MODE_LINEAR
} MipMapMode;

// Forward declarations
typedef struct RendererContext    RendererContext;
typedef struct Renderer           Renderer;
typedef struct Queue              Queue;
typedef struct Pipeline           Pipeline;
typedef struct Buffer             Buffer;
typedef struct Texture            Texture;
typedef struct RenderTarget       RenderTarget;
typedef struct Shader             Shader;
typedef struct DescriptorSet      DescriptorSet;
typedef struct DescriptorIndexMap DescriptorIndexMap;

typedef struct AccelerationStructure AccelerationStructure;


typedef struct IndirectDrawIndexArguments
{
	uint32_t mIndexCount;
	uint32_t mInstanceCount;
	uint32_t mStartIndex;
	uint32_t mVertexOffset;
	uint32_t mStartInstance;
} IndirectDrawIndexArguments;

typedef enum IndirectArgumentType
{
	INDIRECT_DRAW,
	INDIRECT_DRAW_INDEX,
	INDIRECT_DISPATCH,
	INDIRECT_VERTEX_BUFFER,
	INDIRECT_INDEX_BUFFER,
	INDIRECT_CONSTANT,
	INDIRECT_DESCRIPTOR_TABLE,         // only for vulkan
	INDIRECT_PIPELINE,                 // only for vulkan now, probably will add to dx when it comes to xbox
	INDIRECT_CONSTANT_BUFFER_VIEW,     // only for dx
	INDIRECT_SHADER_RESOURCE_VIEW,     // only for dx
	INDIRECT_UNORDERED_ACCESS_VIEW,    // only for dx
} IndirectArgumentType;
/************************************************/

typedef enum DescriptorType
{
	DESCRIPTOR_TYPE_UNDEFINED = 0,
	DESCRIPTOR_TYPE_SAMPLER = 0x01,
	// SRV Read only texture
	DESCRIPTOR_TYPE_TEXTURE = (DESCRIPTOR_TYPE_SAMPLER << 1),
	/// UAV Texture
	DESCRIPTOR_TYPE_RW_TEXTURE = (DESCRIPTOR_TYPE_TEXTURE << 1),
	// SRV Read only buffer
	DESCRIPTOR_TYPE_BUFFER = (DESCRIPTOR_TYPE_RW_TEXTURE << 1),
	DESCRIPTOR_TYPE_BUFFER_RAW = (DESCRIPTOR_TYPE_BUFFER | (DESCRIPTOR_TYPE_BUFFER << 1)),
	/// UAV Buffer
	DESCRIPTOR_TYPE_RW_BUFFER = (DESCRIPTOR_TYPE_BUFFER << 2),
	DESCRIPTOR_TYPE_RW_BUFFER_RAW = (DESCRIPTOR_TYPE_RW_BUFFER | (DESCRIPTOR_TYPE_RW_BUFFER << 1)),
	/// Uniform buffer
	DESCRIPTOR_TYPE_UNIFORM_BUFFER = (DESCRIPTOR_TYPE_RW_BUFFER << 2),
	/// Push constant / Root constant
	DESCRIPTOR_TYPE_ROOT_CONSTANT = (DESCRIPTOR_TYPE_UNIFORM_BUFFER << 1),
	/// IA
	DESCRIPTOR_TYPE_VERTEX_BUFFER = (DESCRIPTOR_TYPE_ROOT_CONSTANT << 1),
	DESCRIPTOR_TYPE_INDEX_BUFFER = (DESCRIPTOR_TYPE_VERTEX_BUFFER << 1),
	DESCRIPTOR_TYPE_INDIRECT_BUFFER = (DESCRIPTOR_TYPE_INDEX_BUFFER << 1),
	/// Cubemap SRV
	DESCRIPTOR_TYPE_TEXTURE_CUBE = (DESCRIPTOR_TYPE_TEXTURE | (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 1)),
	/// RTV / DSV per mip slice
	DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES = (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 2),
	/// RTV / DSV per array slice
	DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES = (DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES << 1),
	/// RTV / DSV per depth slice
	DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES = (DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES << 1),
	DESCRIPTOR_TYPE_RAY_TRACING = (DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES << 1),
	/// Subpass input (descriptor type only available in Vulkan)
	DESCRIPTOR_TYPE_INPUT_ATTACHMENT = (DESCRIPTOR_TYPE_RAY_TRACING << 1),
	DESCRIPTOR_TYPE_TEXEL_BUFFER = (DESCRIPTOR_TYPE_INPUT_ATTACHMENT << 1),
	DESCRIPTOR_TYPE_RW_TEXEL_BUFFER = (DESCRIPTOR_TYPE_TEXEL_BUFFER << 1),
	DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = (DESCRIPTOR_TYPE_RW_TEXEL_BUFFER << 1),

	/// Khronos extension ray tracing
	DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE = (DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER << 1),
	DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE << 1),
	DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT << 1),
	DESCRIPTOR_TYPE_SHADER_BINDING_TABLE = (DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS << 1),
} DescriptorType;
MAKE_ENUM_FLAG(uint32_t, DescriptorType)

typedef enum ShaderStage
{
	SHADER_STAGE_NONE = 0,
	SHADER_STAGE_VERT = 0X00000001,
	SHADER_STAGE_TESC = 0X00000002,
	SHADER_STAGE_TESE = 0X00000004,
	SHADER_STAGE_GEOM = 0X00000008,
	SHADER_STAGE_FRAG = 0X00000010,
	SHADER_STAGE_COMP = 0X00000020,
	SHADER_STAGE_RAYTRACING = 0X00000040,
	SHADER_STAGE_ALL_GRAPHICS =
	((uint32_t)SHADER_STAGE_VERT | (uint32_t)SHADER_STAGE_TESC | (uint32_t)SHADER_STAGE_TESE | (uint32_t)SHADER_STAGE_GEOM |
		(uint32_t)SHADER_STAGE_FRAG),
	SHADER_STAGE_HULL = SHADER_STAGE_TESC,
	SHADER_STAGE_DOMN = SHADER_STAGE_TESE,
	SHADER_STAGE_COUNT = 7,
} ShaderStage;
MAKE_ENUM_FLAG(uint32_t, ShaderStage)

// This include is placed here because it uses data types defined previously in this file
// and forward enums are not allowed for some compilers (Xcode).
#include "IShaderReflection.h"

typedef enum SampleCount
{
	SAMPLE_COUNT_1 = 1,
	SAMPLE_COUNT_2 = 2,
	SAMPLE_COUNT_4 = 4,
	SAMPLE_COUNT_8 = 8,
	SAMPLE_COUNT_16 = 16,
} SampleCount;

typedef enum IndexType
{
	INDEX_TYPE_UINT32 = 0,
	INDEX_TYPE_UINT16,
} IndexType;

typedef enum ShaderSemantic
{
	SEMANTIC_UNDEFINED = 0,
	SEMANTIC_POSITION,
	SEMANTIC_NORMAL,
	SEMANTIC_COLOR,
	SEMANTIC_TANGENT,
	SEMANTIC_BITANGENT,
	SEMANTIC_JOINTS,
	SEMANTIC_WEIGHTS,
	SEMANTIC_SHADING_RATE,
	SEMANTIC_TEXCOORD0,
	SEMANTIC_TEXCOORD1,
	SEMANTIC_TEXCOORD2,
	SEMANTIC_TEXCOORD3,
	SEMANTIC_TEXCOORD4,
	SEMANTIC_TEXCOORD5,
	SEMANTIC_TEXCOORD6,
	SEMANTIC_TEXCOORD7,
	SEMANTIC_TEXCOORD8,
	SEMANTIC_TEXCOORD9,
} ShaderSemantic;

typedef enum BlendConstant
{
	BC_ZERO = 0,
	BC_ONE,
	BC_SRC_COLOR,
	BC_ONE_MINUS_SRC_COLOR,
	BC_DST_COLOR,
	BC_ONE_MINUS_DST_COLOR,
	BC_SRC_ALPHA,
	BC_ONE_MINUS_SRC_ALPHA,
	BC_DST_ALPHA,
	BC_ONE_MINUS_DST_ALPHA,
	BC_SRC_ALPHA_SATURATE,
	BC_BLEND_FACTOR,
	BC_ONE_MINUS_BLEND_FACTOR,
	MAX_BLEND_CONSTANTS
} BlendConstant;

typedef enum BlendMode
{
	BM_ADD,
	BM_SUBTRACT,
	BM_REVERSE_SUBTRACT,
	BM_MIN,
	BM_MAX,
	MAX_BLEND_MODES,
} BlendMode;

typedef enum CompareMode
{
	CMP_NEVER,
	CMP_LESS,
	CMP_EQUAL,
	CMP_LEQUAL,
	CMP_GREATER,
	CMP_NOTEQUAL,
	CMP_GEQUAL,
	CMP_ALWAYS,
	MAX_COMPARE_MODES,
} CompareMode;

typedef enum StencilOp
{
	STENCIL_OP_KEEP,
	STENCIL_OP_SET_ZERO,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCR,
	STENCIL_OP_DECR,
	STENCIL_OP_INCR_SAT,
	STENCIL_OP_DECR_SAT,
	MAX_STENCIL_OPS,
} StencilOp;

static const int RED = 0x1;
static const int GREEN = 0x2;
static const int BLUE = 0x4;
static const int ALPHA = 0x8;
static const int ALL = (RED | GREEN | BLUE | ALPHA);
static const int NONE = 0;

static const int BS_NONE = -1;
static const int DS_NONE = -1;
static const int RS_NONE = -1;

// Blend states are always attached to one of the eight or more render targets that
// are in a MRT
// Mask constants
typedef enum BlendStateTargets
{
	BLEND_STATE_TARGET_0 = 0x1,
	BLEND_STATE_TARGET_1 = 0x2,
	BLEND_STATE_TARGET_2 = 0x4,
	BLEND_STATE_TARGET_3 = 0x8,
	BLEND_STATE_TARGET_4 = 0x10,
	BLEND_STATE_TARGET_5 = 0x20,
	BLEND_STATE_TARGET_6 = 0x40,
	BLEND_STATE_TARGET_7 = 0x80,
	BLEND_STATE_TARGET_ALL = 0xFF,
} BlendStateTargets;
MAKE_ENUM_FLAG(uint32_t, BlendStateTargets)

typedef enum CullMode
{
	CULL_MODE_NONE = 0,
	CULL_MODE_BACK,
	CULL_MODE_FRONT,
	CULL_MODE_BOTH,
	MAX_CULL_MODES
} CullMode;

typedef enum FrontFace
{
	FRONT_FACE_CCW = 0,
	FRONT_FACE_CW
} FrontFace;

typedef enum FillMode
{
	FILL_MODE_SOLID,
	FILL_MODE_WIREFRAME,
	MAX_FILL_MODES
} FillMode;

typedef union ClearValue
{
	struct
	{
		float r;
		float g;
		float b;
		float a;
	};
	struct
	{
		float    depth;
		uint32_t stencil;
	};
} ClearValue;

typedef enum BufferCreationFlags
{
	/// Default flag (Buffer will use aliased memory, buffer will not be cpu accessible until mapBuffer is called)
	BUFFER_CREATION_FLAG_NONE = 0x01,
	/// Buffer will allocate its own memory (COMMITTED resource)
	BUFFER_CREATION_FLAG_OWN_MEMORY_BIT = 0x02,
	/// Buffer will be persistently mapped
	BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT = 0x04,
	/// Use ESRAM to store this buffer
	BUFFER_CREATION_FLAG_ESRAM = 0x08,
	/// Flag to specify not to allocate descriptors for the resource
	BUFFER_CREATION_FLAG_NO_DESCRIPTOR_VIEW_CREATION = 0x10,

	BUFFER_CREATION_FLAG_HOST_VISIBLE = 0x100,
	BUFFER_CREATION_FLAG_HOST_COHERENT = 0x200,
} BufferCreationFlags;
MAKE_ENUM_FLAG(uint32_t, BufferCreationFlags)

typedef enum TextureCreationFlags
{
	/// Default flag (Texture will use default allocation strategy decided by the api specific allocator)
	TEXTURE_CREATION_FLAG_NONE = 0,
	/// Texture will allocate its own memory (COMMITTED resource)
	TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT = 0x01,
	/// Texture will be allocated in memory which can be shared among multiple processes
	TEXTURE_CREATION_FLAG_EXPORT_BIT = 0x02,
	/// Texture will be allocated in memory which can be shared among multiple gpus
	TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT = 0x04,
	/// Texture will be imported from a handle created in another process
	TEXTURE_CREATION_FLAG_IMPORT_BIT = 0x08,
	/// Use ESRAM to store this texture
	TEXTURE_CREATION_FLAG_ESRAM = 0x10,
	/// Use on-tile memory to store this texture
	TEXTURE_CREATION_FLAG_ON_TILE = 0x20,
	/// Prevent compression meta data from generating (XBox)
	TEXTURE_CREATION_FLAG_NO_COMPRESSION = 0x40,
	/// Force 2D instead of automatically determining dimension based on width, height, depth
	TEXTURE_CREATION_FLAG_FORCE_2D = 0x80,
	/// Force 3D instead of automatically determining dimension based on width, height, depth
	TEXTURE_CREATION_FLAG_FORCE_3D = 0x100,
	/// Display target
	TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET = 0x200,
	/// Create an sRGB texture.
	TEXTURE_CREATION_FLAG_SRGB = 0x400,
	/// Create a normal map texture
	TEXTURE_CREATION_FLAG_NORMAL_MAP = 0x800,
	/// Fast clear
	TEXTURE_CREATION_FLAG_FAST_CLEAR = 0x1000,
	/// Fragment mask
	TEXTURE_CREATION_FLAG_FRAG_MASK = 0x2000,
	/// Doubles the amount of array layers of the texture when rendering VR. Also forces the texture to be a 2D Array texture.
	TEXTURE_CREATION_FLAG_VR_MULTIVIEW = 0x4000,
	/// Binds the FFR fragment density if this texture is used as a render target.
	TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING = 0x8000,
} TextureCreationFlags;
MAKE_ENUM_FLAG(uint32_t, TextureCreationFlags)

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

typedef enum SamplerRange
{
	SAMPLER_RANGE_FULL = 0,
	SAMPLER_RANGE_NARROW = 1,
} SamplerRange;

typedef enum SamplerModelConversion
{
	SAMPLER_MODEL_CONVERSION_RGB_IDENTITY = 0,
	SAMPLER_MODEL_CONVERSION_YCBCR_IDENTITY = 1,
	SAMPLER_MODEL_CONVERSION_YCBCR_709 = 2,
	SAMPLER_MODEL_CONVERSION_YCBCR_601 = 3,
	SAMPLER_MODEL_CONVERSION_YCBCR_2020 = 4,
} SamplerModelConversion;

typedef enum SampleLocation
{
	SAMPLE_LOCATION_COSITED = 0,
	SAMPLE_LOCATION_MIDPOINT = 1,
} SampleLocation;

typedef struct BufferBarrier
{
	Buffer* pBuffer;
	ResourceState mCurrentState;
	ResourceState mNewState;
	uint8_t       mBeginOnly : 1;
	uint8_t       mEndOnly : 1;
	uint8_t       mAcquire : 1;
	uint8_t       mRelease : 1;
	uint8_t       mQueueType : 5;
} BufferBarrier;

typedef struct TextureBarrier
{
	Texture* pTexture;
	ResourceState mCurrentState;
	ResourceState mNewState;
	uint8_t       mBeginOnly : 1;
	uint8_t       mEndOnly : 1;
	uint8_t       mAcquire : 1;
	uint8_t       mRelease : 1;
	uint8_t       mQueueType : 5;
	/// Specifiy whether following barrier targets particular subresource
	uint8_t mSubresourceBarrier : 1;
	/// Following values are ignored if mSubresourceBarrier is false
	uint8_t  mMipLevel : 7;
	uint16_t mArrayLayer;
} TextureBarrier;

typedef struct RenderTargetBarrier
{
	RenderTarget* pRenderTarget;
	ResourceState mCurrentState;
	ResourceState mNewState;
	uint8_t       mBeginOnly : 1;
	uint8_t       mEndOnly : 1;
	uint8_t       mAcquire : 1;
	uint8_t       mRelease : 1;
	uint8_t       mQueueType : 5;
	/// Specifiy whether following barrier targets particular subresource
	uint8_t mSubresourceBarrier : 1;
	/// Following values are ignored if mSubresourceBarrier is false
	uint8_t  mMipLevel : 7;
	uint16_t mArrayLayer;
} RenderTargetBarrier;

typedef struct ReadRange
{
	uint64_t mOffset;
	uint64_t mSize;
} ReadRange;

/// Data structure holding necessary info to create a Texture
typedef struct TextureDesc
{
	/// Optimized clear value (recommended to use this same value when clearing the rendertarget)
	ClearValue mClearValue;
	/// Pointer to native texture handle if the texture does not own underlying resource
	const void* pNativeHandle;
	/// Debug name used in gpu profile
	const char* pName;
	/// GPU indices to share this texture
	uint32_t* pSharedNodeIndices;

	VkSamplerYcbcrConversionInfo* pVkSamplerYcbcrConversionInfo;

	/// Texture creation flags (decides memory allocation strategy, sharing access,...)
	TextureCreationFlags mFlags;
	/// Width
	uint32_t mWidth;
	/// Height
	uint32_t mHeight;
	/// Depth (Should be 1 if not a mType is not TEXTURE_TYPE_3D)
	uint32_t mDepth;
	/// Texture array size (Should be 1 if texture is not a texture array or cubemap)
	uint32_t mArraySize;
	/// Number of mip levels
	uint32_t mMipLevels;
	/// Number of multisamples per pixel (currently Textures created with mUsage TEXTURE_USAGE_SAMPLED_IMAGE only support SAMPLE_COUNT_1)
	SampleCount mSampleCount;
	/// The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for mSampleCount
	uint32_t mSampleQuality;
	///  image format
	TinyImageFormat mFormat;
	/// What state will the texture get created in
	ResourceState mStartState;
	/// Descriptor creation
	DescriptorType mDescriptors;
	/// Number of GPUs to share this texture
	uint32_t mSharedNodeIndexCount;
	/// GPU which will own this texture
	uint32_t mNodeIndex;
} TextureDesc;

// Virtual texture page as a part of the partially resident texture
// Contains memory bindings, offsets and status information
struct VirtualTexturePage
{
	/// Miplevel for this page
	uint32_t mipLevel;
	/// Array layer for this page
	uint32_t layer;
	/// Index for this page
	uint32_t index;
	struct
	{
		/// Allocation and resource for this tile
		void* pAllocation;
		/// Sparse image memory bind for this page
		VkSparseImageMemoryBind imageMemoryBind;
		/// Byte size for this page
		VkDeviceSize size;
	} mVulkan;
};

typedef struct VirtualTexture
{
	struct
	{
		/// GPU memory pool for tiles
		void* pPool;
		/// Sparse image memory bindings of all memory-backed virtual tables
		VkSparseImageMemoryBind* pSparseImageMemoryBinds;
		/// Sparse opaque memory bindings for the mip tail (if present)
		VkSparseMemoryBind* pOpaqueMemoryBinds;
		/// GPU allocations for opaque memory binds (mip tail)
		void** pOpaqueMemoryBindAllocations;
		/// Pending allocation deletions
		void** pPendingDeletedAllocations;
		//Memory type bits for Sparse texture's memory
		uint32_t mSparseMemoryTypeBits;
		/// Number of opaque memory binds
		uint32_t mOpaqueMemoryBindsCount;
	} mVulkan;

	/// Virtual Texture members
	VirtualTexturePage* pPages;
	/// Pending intermediate buffer deletions
	Buffer** pPendingDeletedBuffers;
	/// Pending intermediate buffer deletions count
	uint32_t* pPendingDeletedBuffersCount;
	/// Pending allocation deletions count
	uint32_t* pPendingDeletedAllocationsCount;
	/// Readback buffer, must be filled by app. Size = mReadbackBufferSize * imagecount
	Buffer* pReadbackBuffer;
	/// Original Pixel image data
	void* pVirtualImageData;
	///  Total pages count
	uint32_t mVirtualPageTotalCount;
	///  Alive pages count
	uint32_t mVirtualPageAliveCount;
	/// Size of the readback buffer per image
	uint32_t mReadbackBufferSize;
	/// Size of the readback buffer per image
	uint32_t mPageVisibilityBufferSize;
	/// Sparse Virtual Texture Width
	uint16_t mSparseVirtualTexturePageWidth;
	/// Sparse Virtual Texture Height
	uint16_t mSparseVirtualTexturePageHeight;
	/// Number of mip levels that are tiled
	uint8_t mTiledMipLevelCount;
	/// Size of the pending deletion arrays in image count (highest currentImage + 1)
	uint8_t mPendingDeletionCount;
} VirtualTexture;

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

	VirtualTexture* pSvt;
	/// Current state of the buffer
	uint32_t mWidth : 16;
	uint32_t mHeight : 16;
	uint32_t mDepth : 16;
	uint32_t mMipLevels : 5;
	uint32_t mArraySizeMinusOne : 11;
	uint32_t mFormat : 8;
	/// Flags specifying which aspects (COLOR,DEPTH,STENCIL) are included in the pVkImageView
	uint32_t mAspectMask : 4;
	uint32_t mNodeIndex : 4;
	uint32_t mUav : 1;
	/// This value will be false if the underlying resource is not owned by the texture (swapchain textures,...)
	uint32_t mOwnsImage : 1;
	// Only applies to Vulkan but kept here as adding it inside mVulkan block increases the size of the struct and triggers assert below
	uint32_t mLazilyAllocated : 1;

} Texture;
// One cache line
COMPILE_ASSERT(sizeof(Texture) == 8 * sizeof(uint64_t));

typedef struct DEFINE_ALIGNED(RenderTarget, 64)
{
	Texture* pTexture;
	union
	{
		struct
		{
			VkImageView  pVkDescriptor;
			VkImageView* pVkSliceDescriptors;
			uint32_t     mId;
		} mVulkan;
	};
	ClearValue      mClearValue;
	uint32_t        mArraySize : 16;
	uint32_t        mDepth : 16;
	uint32_t        mWidth : 16;
	uint32_t        mHeight : 16;
	uint32_t        mDescriptors : 20;
	uint32_t        mMipLevels : 10;
	uint32_t        mSampleQuality : 5;
	TinyImageFormat mFormat;
	SampleCount     mSampleCount;
	bool            mVRMultiview;
	bool            mVRFoveatedRendering;
} RenderTarget;
COMPILE_ASSERT(sizeof(RenderTarget) <= 32 * sizeof(uint64_t));

typedef struct SamplerDesc
{
	FilterType  mMinFilter;
	FilterType  mMagFilter;
	MipMapMode  mMipMapMode;
	AddressMode mAddressU;
	AddressMode mAddressV;
	AddressMode mAddressW;
	float       mMipLodBias;
	bool		mSetLodRange;
	float       mMinLod;
	float       mMaxLod;
	float       mMaxAnisotropy;
	CompareMode mCompareFunc;

	struct
	{
		TinyImageFormat        mFormat;
		SamplerModelConversion mModel;
		SamplerRange           mRange;
		SampleLocation         mChromaOffsetX;
		SampleLocation         mChromaOffsetY;
		FilterType             mChromaFilter;
		bool                   mForceExplicitReconstruction;
	} mSamplerConversionDesc;
} SamplerDesc;

typedef struct DEFINE_ALIGNED(Sampler, 16)
{
	union
	{
		struct
		{
			/// Native handle of the underlying resource
			VkSampler                    pVkSampler;
			VkSamplerYcbcrConversion     pVkSamplerYcbcrConversion;
			VkSamplerYcbcrConversionInfo mVkSamplerYcbcrConversionInfo;
		} mVulkan;
	};
} Sampler;

typedef enum DescriptorUpdateFrequency
{
	DESCRIPTOR_UPDATE_FREQ_NONE = 0,
	DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
	DESCRIPTOR_UPDATE_FREQ_PER_BATCH,
	DESCRIPTOR_UPDATE_FREQ_PER_DRAW,
	DESCRIPTOR_UPDATE_FREQ_COUNT,
} DescriptorUpdateFrequency;

typedef struct DEFINE_ALIGNED(DescriptorInfo, 16)
{
	const char* pName;
	uint32_t    mType : 21;
	uint32_t    mDim : 4;
	uint32_t    mRootDescriptor : 1;
	uint32_t    mStaticSampler : 1;
	uint32_t    mUpdateFrequency : 3;
	uint32_t    mSize;
	uint32_t    mHandleIndex;
	union
	{
		uint32_t mVkType;
		uint32_t mReg : 20;
		uint32_t mVkStages : 8;
	} mVulkan;
} DescriptorInfo;
COMPILE_ASSERT(sizeof(DescriptorInfo) == 4 * sizeof(uint64_t));

typedef enum RootSignatureFlags
{
	/// Default flag
	ROOT_SIGNATURE_FLAG_NONE = 0,
	/// Local root signature used mainly in raytracing shaders
	ROOT_SIGNATURE_FLAG_LOCAL_BIT = 0x1,
} RootSignatureFlags;
MAKE_ENUM_FLAG(uint32_t, RootSignatureFlags)

typedef struct RootSignatureDesc
{
	Shader** ppShaders;
	uint32_t           mShaderCount;
	uint32_t           mMaxBindlessTextures;
	const char** ppStaticSamplerNames;
	Sampler** ppStaticSamplers;
	uint32_t           mStaticSamplerCount;
	RootSignatureFlags mFlags;
} RootSignatureDesc;

typedef struct DEFINE_ALIGNED(RootSignature, 64)
{
	/// Number of descriptors declared in the root signature layout
	uint32_t mDescriptorCount;
	/// Graphics or Compute
	PipelineType mPipelineType;
	/// Array of all descriptors declared in the root signature layout
	DescriptorInfo* pDescriptors;
	/// Translates hash of descriptor name to descriptor index in pDescriptors array
	DescriptorIndexMap* pDescriptorNameToIndexMap;
	union
	{
		struct
		{
			VkPipelineLayout            pPipelineLayout;
			VkDescriptorSetLayout       mVkDescriptorSetLayouts[DESCRIPTOR_UPDATE_FREQ_COUNT];
			uint32_t                    mVkCumulativeDescriptorCounts[DESCRIPTOR_UPDATE_FREQ_COUNT];
			uint32_t                    mCumulativeDescriptorSizes[DESCRIPTOR_UPDATE_FREQ_COUNT];
			uint8_t                     mVkDynamicDescriptorCounts[DESCRIPTOR_UPDATE_FREQ_COUNT];
			VkDescriptorPoolSize        mPoolSizes[DESCRIPTOR_UPDATE_FREQ_COUNT][MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT];
			uint8_t                     mPoolSizeCount[DESCRIPTOR_UPDATE_FREQ_COUNT];
			VkDescriptorPool            pEmptyDescriptorPool[DESCRIPTOR_UPDATE_FREQ_COUNT];
			VkDescriptorSet             pEmptyDescriptorSet[DESCRIPTOR_UPDATE_FREQ_COUNT];
		} mVulkan;
	};
} RootSignature;
COMPILE_ASSERT(sizeof(RootSignature) <= 72 * sizeof(uint64_t));

typedef struct DescriptorDataRange
{
	uint32_t mOffset;
	uint32_t mSize;
} DescriptorDataRange;

typedef struct DescriptorData
{
	/// User can either set name of descriptor or index (index in pRootSignature->pDescriptors array)
	/// Name of descriptor
	const char* pName;
	/// Number of array entries to update (array size of ppTextures/ppBuffers/...)
	uint32_t    mCount;
	/// Dst offset into the array descriptor (useful for updating few entries in a large array)
	// Example: to update 6th entry in a bindless texture descriptor, mArrayOffset will be 6 and mCount will be 1)
	uint32_t    mArrayOffset : 20;
	// Index in pRootSignature->pDescriptors array - Cache index using getDescriptorIndexFromName to avoid using string checks at runtime
	uint32_t    mIndex : 10;
	uint32_t    mBindByIndex : 1;
	uint32_t    mExtractBuffer : 1;

	union
	{
		// Range to bind (buffer offset, size)
		DescriptorDataRange* pRanges;
		// Descriptor set buffer extraction options
		uint32_t                mDescriptorSetBufferIndex;
		struct
		{
			// When binding UAV, control the mip slice to to bind for UAV (example - generating mipmaps in a compute shader)
			uint16_t            mUAVMipSlice;
			// Binds entire mip chain as array of UAV
			bool                mBindMipChain;
		};
		// Binds stencil only descriptor instead of color/depth
		bool                    mBindStencilResource;
	};
	/// Array of resources containing descriptor handles or constant to be used in ring buffer memory - DescriptorRange can hold only one resource type array
	union
	{
		/// Array of texture descriptors (srv and uav textures)
		Texture** ppTextures;
		/// Array of sampler descriptors
		Sampler** ppSamplers;
		/// Array of buffer descriptors (srv, uav and cbv buffers)
		Buffer** ppBuffers;
		/// Array of pipeline descriptors
		Pipeline** ppPipelines;
		/// DescriptorSet buffer extraction
		DescriptorSet** ppDescriptorSet;
		/// Custom binding (raytracing acceleration structure ...)
		AccelerationStructure** ppAccelerationStructures;
	};
} DescriptorData;

typedef struct DEFINE_ALIGNED(DescriptorSet, 64)
{
	union
	{
		struct
		{
			VkDescriptorSet* pHandles;
			const RootSignature* pRootSignature;
			uint8_t* pDescriptorData;
			struct DynamicUniformData* pDynamicUniformData;
			VkDescriptorPool             pDescriptorPool;
			uint32_t                     mMaxSets;
			uint8_t                      mDynamicOffsetCount;
			uint8_t                      mUpdateFrequency;
			uint8_t                      mNodeIndex;
			uint8_t                      mPadA;
		} mVulkan;
	};
} DescriptorSet;

typedef struct CmdPoolDesc
{
	Queue* pQueue;
	bool   mTransient;
} CmdPoolDesc;

typedef struct CmdPool
{
	VkCommandPool pVkCmdPool;

	Queue* pQueue;
} CmdPool;

typedef struct CmdDesc
{
	CmdPool* pPool;
#if defined(ORBIS) || defined(PROSPERO)
	uint32_t mMaxSize;
#endif
	bool mSecondary;
} CmdDesc;

typedef struct DEFINE_ALIGNED(Cmd, 64)
{
	union
	{
		struct
		{
			VkCommandBuffer pVkCmdBuf;
			VkRenderPass pVkActiveRenderPass;
			VkPipelineLayout pBoundPipelineLayout;
			uint32_t mNodeIndex : 4;
			uint32_t mType : 3;
			uint32_t mPadA;
			CmdPool* pCmdPool;
			uint64_t mPadB[9];
		}mVulkan;
	};
	Renderer* pRenderer;
	Queue* pQueue;
} Cmd;

typedef enum FenceStatus
{
	FENCE_STATUS_COMPLETE = 0,
	FENCE_STATUS_INCOMPLETE,
	FENCE_STATUS_NOTSUBMITTED,
} FenceStatus;

typedef struct Fence
{
	union
	{
		struct
		{
			VkFence pVkFence;
			uint32_t mSubmitted : 1;
			uint32_t mPadA;
			uint32_t mPadB;
			uint32_t mPadC;

		} mVulkan;
	};
} Fence;

typedef struct Semaphore
{
	union
	{
		struct
		{
			VkSemaphore pVkSemaphore;
			uint32_t    mCurrentNodeIndex : 5;
			uint32_t    mSignaled : 1;
			uint32_t    mPadA;
			uint64_t    mPadB;
			uint64_t    mPadC;
		} mVulkan;
	};
} Semaphore;

typedef struct QueueDesc
{
	QueueType     mType;
	QueueFlag     mFlag;
	QueuePriority mPriority;
	uint32_t      mNodeIndex;
} QueueDesc;

/// Data structure holding necessary info to create a Buffer
typedef struct BufferDesc
{
	/// Size of the buffer (in bytes)
	uint64_t mSize;
	/// Set this to specify a counter buffer for this buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
	struct Buffer* pCounterBuffer;
	/// Index of the first element accessible by the SRV/UAV (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
	uint64_t mFirstElement;
	/// Number of elements in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
	uint64_t mElementCount;
	/// Size of each element (in bytes) in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
	uint64_t mStructStride;
	/// Debug name used in gpu profile
	const char* pName;
	uint32_t* pSharedNodeIndices;
	/// Alignment
	uint32_t mAlignment;
	/// Decides which memory heap buffer will use (default, upload, readback)
	ResourceMemoryUsage mMemoryUsage;
	/// Creation flags of the buffer
	BufferCreationFlags mFlags;
	/// What type of queue the buffer is owned by
	QueueType mQueueType;
	/// What state will the buffer get created in
	ResourceState mStartState;
	/// ICB draw type
	IndirectArgumentType mICBDrawType;
	/// ICB max vertex buffers slots count
	uint32_t mICBMaxVertexBufferBind;
	/// ICB max vertex buffers slots count
	uint32_t mICBMaxFragmentBufferBind;
	/// Format of the buffer (applicable to typed storage buffers (Buffer<T>)
	TinyImageFormat mFormat;
	/// Flags specifying the suitable usage of this buffer (Uniform buffer, Vertex Buffer, Index Buffer,...)
	DescriptorType mDescriptors;
	/// The index of the GPU in SLI/Cross-Fire that owns this buffer, or the Renderer index in unlinked mode.
	uint32_t       mNodeIndex;
	uint32_t       mSharedNodeIndexCount;
} BufferDesc;

typedef struct DEFINE_ALIGNED(Buffer, 64)
{
	/// CPU address of the mapped buffer (applicable to buffers created in CPU accessible heaps (CPU, CPU_TO_GPU, GPU_TO_CPU)
	void* pCpuMappedAddress;
	union
	{
		struct
		{
			/// Native handle of the underlying resource
			VkBuffer pVkBuffer;
			/// Buffer view
			VkBufferView pVkStorageTexelView;
			VkBufferView pVkUniformTexelView;
			/// Contains resource allocation info such as parent heap, offset in heap
			struct VmaAllocation_T* pVkAllocation;
			uint64_t mOffset;
		} mVulkan;
		uint64_t mSize : 32;
		uint64_t mDescriptors : 20;
		uint64_t mMemoryUsage : 3;
		uint64_t mNodeIndex : 4;
	};
} Buffer;

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

/// ShaderConstant only supported by Vulkan and Metal APIs
typedef struct ShaderConstant
{
	const void* pValue;
	uint32_t       mIndex;
	uint32_t       mSize;
} ShaderConstant;

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

typedef enum VertexAttribRate
{
	VERTEX_ATTRIB_RATE_VERTEX = 0,
	VERTEX_ATTRIB_RATE_INSTANCE = 1,
	VERTEX_ATTRIB_RATE_COUNT,
} VertexAttribRate;

typedef struct VertexAttrib
{
	ShaderSemantic   mSemantic;
	uint32_t         mSemanticNameLength;
	char             mSemanticName[MAX_SEMANTIC_NAME_LENGTH];
	TinyImageFormat  mFormat;
	uint32_t         mBinding;
	uint32_t         mLocation;
	uint32_t         mOffset;
	VertexAttribRate mRate;
} VertexAttrib;

typedef struct VertexLayout
{
	uint32_t     mAttribCount;
	VertexAttrib mAttribs[MAX_VERTEX_ATTRIBS];
	uint32_t     mStrides[MAX_VERTEX_BINDINGS];
} VertexLayout;

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

typedef enum DefaultResourceAlignment
{
	RESOURCE_BUFFER_ALIGNMENT = 4U,
} DefaultResourceAlignment;

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
MAKE_ENUM_FLAG(uint32_t, WaveOpsSupportFlags);

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

typedef struct DescriptorSetDesc
{
	RootSignature* pRootSignature;
	DescriptorUpdateFrequency mUpdateFrequency;
	uint32_t                  mMaxSets;
	uint32_t                  mNodeIndex;
} DescriptorSetDesc;

typedef struct BinaryShaderStageDesc
{
#if defined(PROSPERO)
	ProsperoBinaryShaderStageDesc mStruct;
#else
	/// Byte code array
	void* pByteCode;
	uint32_t    mByteCodeSize;
	const char* pEntryPoint;
#if defined(METAL)
	// Shader source is needed for reflection
	char* pSource;
	uint32_t mSourceSize;
#endif
#if defined(GLES)
	GLuint   mShader;
#endif
#endif
} BinaryShaderStageDesc;

typedef struct BinaryShaderDesc
{
	ShaderStage           mStages;
	/// Specify whether shader will own byte code memory
	uint32_t              mOwnByteCode : 1;
	BinaryShaderStageDesc mVert;
	BinaryShaderStageDesc mFrag;
	BinaryShaderStageDesc mGeom;
	BinaryShaderStageDesc mHull;
	BinaryShaderStageDesc mDomain;
	BinaryShaderStageDesc mComp;
	const ShaderConstant* pConstants;
	uint32_t              mConstantCount;
} BinaryShaderDesc;

typedef struct Shader
{
	ShaderStage mStages : 31;
	bool mIsMultiviewVR : 1;
	uint32_t mNumThreadsPerGroup[3];
	union
	{
		struct
		{
			VkShaderModule* pShaderModules;
			char** pEntryNames;
			VkSpecializationInfo* pSpecializationInfo;
		} mVulkan;
		PipelineReflection* pReflection;
	};

} Shader;

typedef struct BlendStateDesc
{
	/// Source blend factor per render target.
	BlendConstant mSrcFactors[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Destination blend factor per render target.
	BlendConstant mDstFactors[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Source alpha blend factor per render target.
	BlendConstant mSrcAlphaFactors[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Destination alpha blend factor per render target.
	BlendConstant mDstAlphaFactors[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Blend mode per render target.
	BlendMode mBlendModes[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Alpha blend mode per render target.
	BlendMode mBlendAlphaModes[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Write mask per render target.
	int32_t mMasks[MAX_RENDER_TARGET_ATTACHMENTS];
	/// Mask that identifies the render targets affected by the blend state.
	BlendStateTargets mRenderTargetMask;
	/// Set whether alpha to coverage should be enabled.
	bool mAlphaToCoverage;
	/// Set whether each render target has an unique blend function. When false the blend function in slot 0 will be used for all render targets.
	bool mIndependentBlend;
} BlendStateDesc;



typedef struct DepthStateDesc
{
	bool        mDepthTest;
	bool        mDepthWrite;
	CompareMode mDepthFunc;
	bool        mStencilTest;
	uint8_t     mStencilReadMask;
	uint8_t     mStencilWriteMask;
	CompareMode mStencilFrontFunc;
	StencilOp   mStencilFrontFail;
	StencilOp   mDepthFrontFail;
	StencilOp   mStencilFrontPass;
	CompareMode mStencilBackFunc;
	StencilOp   mStencilBackFail;
	StencilOp   mDepthBackFail;
	StencilOp   mStencilBackPass;
} DepthStateDesc;

typedef struct RasterizerStateDesc
{
	CullMode  mCullMode;
	int32_t   mDepthBias;
	float     mSlopeScaledDepthBias;
	FillMode  mFillMode;
	FrontFace mFrontFace;
	bool      mMultiSample;
	bool      mScissor;
	bool      mDepthClampEnable;
} RasterizerStateDesc;

typedef struct DEFINE_ALIGNED(Pipeline, 64)
{
	union
	{
		struct
		{
			VkPipeline   pVkPipeline;
			PipelineType mType;
			uint32_t     mShaderStageCount;
			//In DX12 this information is stored in ID3D12StateObject.
			//But for Vulkan we need to store it manually
			const char** ppShaderStageNames;
			uint64_t     mPadB[4];
		} mVulkan;
	};
} Pipeline;

typedef enum PipelineCacheFlags
{
	PIPELINE_CACHE_FLAG_NONE = 0x0,
	PIPELINE_CACHE_FLAG_EXTERNALLY_SYNCHRONIZED = 0x1,
} PipelineCacheFlags;
MAKE_ENUM_FLAG(uint32_t, PipelineCacheFlags);

typedef struct PipelineCache
{
	union 
	{
		struct 
		{
			VkPipelineCache pCache;
		} mVulkan;
	};
} PipelineCache;

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

typedef struct QueueSubmitDesc
{
	Cmd** ppCmds;
	Fence* pSignalFence;
	Semaphore** ppWaitSemaphores;
	Semaphore** ppSignalSemaphores;
	uint32_t    mCmdCount;
	uint32_t    mWaitSemaphoreCount;
	uint32_t    mSignalSemaphoreCount;
	bool        mSubmitDone;
} QueueSubmitDesc;

typedef struct SubresourceDataDesc
{
	uint64_t mSrcOffset;
	uint32_t mMipLevel;
	uint32_t mArrayLayer;
	uint32_t mRowPitch;
	uint32_t mSlicePitch;
} SubresourceDataDesc;

struct VkVTPendingPageDeletion
{
	VmaAllocation* pAllocations;
	uint32_t* pAllocationsCount;

	Buffer** pIntermediateBuffers;
	uint32_t* pIntermediateBuffersCount;
};
API_INTERFACE void FORGE_CALLCONV initRenderer(const char* app_name, const RendererDesc* p_settings, Renderer** pRenderer);




void vk_waitQueueIdle(Queue* p_queue);
void vk_removeQueue(Renderer* pRenderer, Queue* pQueue);

void vk_addBuffer(Renderer* pRenderer, const BufferDesc* pDesc, Buffer** ppBuffer);
void vk_getFenceStatus(Renderer* pRenderer, Fence* pFence, FenceStatus* pFenceStatus);
void vk_waitForFences(Renderer* pRenderer, uint32_t fenceCount, Fence** ppFences);
void vk_removeBuffer(Renderer* pRenderer, Buffer* pBuffer);
void vk_addTexture(Renderer* pRenderer, const TextureDesc* pDesc, Texture** ppTexture);
void vk_addVirtualTexture(Cmd* pCmd, const TextureDesc* pDesc, Texture** ppTexture, void* pImageData);
void vk_fillVirtualTextureLevel(Cmd* pCmd, Texture* pTexture, uint32_t mipLevel, uint32_t currentImage);
void vk_updateVirtualTextureMemory(Cmd* pCmd, Texture* pTexture, uint32_t imageMemoryCount);


void vk_uploadVirtualTexturePage(Cmd* pCmd, Texture* pTexture, VirtualTexturePage* pPage, uint32_t* imageMemoryCount, uint32_t currentImage);
// command buffer functions
void vk_resetCmdPool(Renderer* pRenderer, CmdPool* pCmdPool);
void vk_beginCmd(Cmd* pCmd);
void vk_cmdUpdateBuffer(Cmd* pCmd, Buffer* pBuffer, uint64_t dstOffset, Buffer* pSrcBuffer, uint64_t srcOffset, uint64_t size);
void vk_cmdResourceBarrier(
	Cmd* pCmd, uint32_t numBufferBarriers, BufferBarrier* pBufferBarriers, uint32_t numTextureBarriers, TextureBarrier* pTextureBarriers,
	uint32_t numRtBarriers, RenderTargetBarrier* pRtBarriers);
void vk_cmdUpdateSubresource(Cmd* pCmd, Texture* pTexture, Buffer* pSrcBuffer, const SubresourceDataDesc* pSubresourceDesc);
void vk_cmdCopySubresource(Cmd* pCmd, Buffer* pDstBuffer, Texture* pTexture, const SubresourceDataDesc* pSubresourceDesc);
void vk_cmdBindPipeline(Cmd* pCmd, Pipeline* pPipeline);
void vk_cmdBindDescriptorSetWithRootCbvs(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t count, const DescriptorData* pParams);
void vk_cmdBindDescriptorSet(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet);
void vk_cmdBindPushConstants(Cmd* pCmd, RootSignature* pRootSignature, uint32_t paramIndex, const void* pConstants);
void vk_cmdBindVertexBuffer(Cmd* pCmd, uint32_t bufferCount, Buffer** ppBuffers, const uint32_t* pStrides, const uint64_t* pOffsets);

void vk_cmdDraw(Cmd* pCmd, uint32_t vertex_count, uint32_t first_vertex);

void vk_addCmdPool(Renderer* pRenderer, const CmdPoolDesc* pDesc, CmdPool** ppCmdPool);
void vk_endCmd(Cmd* pCmd);

void vk_removeCmd(Renderer* pRenderer, Cmd* pCmd);
void vk_addCmd(Renderer* pRenderer, const CmdDesc* pDesc, Cmd** ppCmd);
void vk_removeCmdPool(Renderer* pRenderer, CmdPool* pCmdPool);

void vk_mapBuffer(Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange);
void vk_unmapBuffer(Renderer* pRenderer, Buffer* pBuffer);

void vk_queueSubmit(Queue* pQueue, const QueueSubmitDesc* pDesc);

void vk_removeSemaphore(Renderer* pRenderer, Semaphore* pSemaphore);

void vk_addFence(Renderer* pRenderer, Fence** ppFence);
void vk_removeFence(Renderer* pRenderer, Fence* pFence);


void vk_updateDescriptorSet(Renderer* pRenderer, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t count, const DescriptorData* pParams);

void vk_initRenderer(const char* appName, const RendererDesc* pDesc, Renderer** ppRenderer);

void vk_addQueue(Renderer* pRenderer, QueueDesc* pDesc, Queue** ppQueue);

void vk_addSampler(Renderer* pRenderer, const SamplerDesc* pDesc, Sampler** ppSampler);

void vk_setTextureName(Renderer* pRenderer, Texture* pTexture, const char* pName);
void vk_setBufferName(Renderer* pRenderer, Buffer* pBuffer, const char* pName);

void vk_addSemaphore(Renderer* pRenderer, Semaphore** ppSemaphore);

void vk_addShaderBinary(Renderer* pRenderer, const BinaryShaderDesc* pDesc, Shader** ppShaderProgram);

void vk_addRootSignature(Renderer* pRenderer, const RootSignatureDesc* pRootSignatureDesc, RootSignature** ppRootSignature);

void vk_addDescriptorSet(Renderer* pRenderer, const DescriptorSetDesc* pDesc, DescriptorSet** ppDescriptorSet);



/************************************************************************/
/************************************************************************/
uint32_t getDescriptorIndexFromName(const RootSignature* pRootSignature, const char* pName);
// clang-format on