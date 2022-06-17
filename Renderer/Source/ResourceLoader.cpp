#include "../Include/RendererConfig.h"

#include "../ThirdParty/OpenSource/EASTL/string.h"
#include "../ThirdParty/OpenSource/EASTL/vector.h"

#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_bits.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_apis.h"

#if defined(ENABLE_MESHOPTIMIZER)
#include "../ThirdParty/OpenSource/meshoptimizer/src/meshoptimizer.h"
#endif

#define TINYKTX_IMPLEMENTATION
#include "../ThirdParty/OpenSource/tinyktx/tinyktx.h"

#define CGLTF_IMPLEMENTATION
#include "../ThirdParty/OpenSource/cgltf/cgltf.h"

#include "../Include/IRenderer.h"
#include "../Include/IResourceLoader.h"


#include "../OS/Interfaces/ILog.h"
#include "../OS/Interfaces/IThread.h"

#include "../OS/Core/TextureContainers.h"

#define MIP_REDUCE(s, mip) (max(1u, (uint32_t)((s) >> (mip))))

enum
{
	MAPPED_RANGE_FLAG_UNMAP_BUFFER = (1 << 0),
	MAPPED_RANGE_FLAG_TEMP_BUFFER = (1 << 1),
};


extern RendererApi gSelectedRendererApi;

static FORGE_CONSTEXPR const bool gUma = false;

#define MAX_FRAMES 3U

/************************************************************************/
// Surface Utils
/************************************************************************/

static inline constexpr ShaderSemantic util_cgltf_attrib_type_to_semantic(cgltf_attribute_type type, uint32_t index)
{
	switch (type)
	{
	case cgltf_attribute_type_position: return SEMANTIC_POSITION;
	case cgltf_attribute_type_normal: return SEMANTIC_NORMAL;
	case cgltf_attribute_type_tangent: return SEMANTIC_TANGENT;
	case cgltf_attribute_type_color: return SEMANTIC_COLOR;
	case cgltf_attribute_type_joints: return SEMANTIC_JOINTS;
	case cgltf_attribute_type_weights: return SEMANTIC_WEIGHTS;
	case cgltf_attribute_type_texcoord: return (ShaderSemantic)(SEMANTIC_TEXCOORD0 + index);
	default: return SEMANTIC_TEXCOORD0;
	}
}

static inline constexpr TinyImageFormat util_cgltf_type_to_image_format(cgltf_type type, cgltf_component_type compType)
{
	switch (type)
	{
	case cgltf_type_scalar:
		if (cgltf_component_type_r_8 == compType)
			return TinyImageFormat_R8_SINT;
		else if (cgltf_component_type_r_16 == compType)
			return TinyImageFormat_R16_SINT;
		else if (cgltf_component_type_r_16u == compType)
			return TinyImageFormat_R16_UINT;
		else if (cgltf_component_type_r_32f == compType)
			return TinyImageFormat_R32_SFLOAT;
		else if (cgltf_component_type_r_32u == compType)
			return TinyImageFormat_R32_UINT;
	case cgltf_type_vec2:
		if (cgltf_component_type_r_8 == compType)
			return TinyImageFormat_R8G8_SINT;
		else if (cgltf_component_type_r_16 == compType)
			return TinyImageFormat_R16G16_SINT;
		else if (cgltf_component_type_r_16u == compType)
			return TinyImageFormat_R16G16_UINT;
		else if (cgltf_component_type_r_32f == compType)
			return TinyImageFormat_R32G32_SFLOAT;
		else if (cgltf_component_type_r_32u == compType)
			return TinyImageFormat_R32G32_UINT;
	case cgltf_type_vec3:
		if (cgltf_component_type_r_8 == compType)
			return TinyImageFormat_R8G8B8_SINT;
		else if (cgltf_component_type_r_16 == compType)
			return TinyImageFormat_R16G16B16_SINT;
		else if (cgltf_component_type_r_16u == compType)
			return TinyImageFormat_R16G16B16_UINT;
		else if (cgltf_component_type_r_32f == compType)
			return TinyImageFormat_R32G32B32_SFLOAT;
		else if (cgltf_component_type_r_32u == compType)
			return TinyImageFormat_R32G32B32_UINT;
	case cgltf_type_vec4:
		if (cgltf_component_type_r_8 == compType)
			return TinyImageFormat_R8G8B8A8_SINT;
		else if (cgltf_component_type_r_16 == compType)
			return TinyImageFormat_R16G16B16A16_SINT;
		else if (cgltf_component_type_r_16u == compType)
			return TinyImageFormat_R16G16B16A16_UINT;
		else if (cgltf_component_type_r_32f == compType)
			return TinyImageFormat_R32G32B32A32_SFLOAT;
		else if (cgltf_component_type_r_32u == compType)
			return TinyImageFormat_R32G32B32A32_UINT;
		// #NOTE: Not applicable to vertex formats
	case cgltf_type_mat2:
	case cgltf_type_mat3:
	case cgltf_type_mat4:
	default: return TinyImageFormat_UNDEFINED;
	}
}

#define F16_EXPONENT_BITS 0x1F
#define F16_EXPONENT_SHIFT 10
#define F16_EXPONENT_BIAS 15
#define F16_MANTISSA_BITS 0x3ff
#define F16_MANTISSA_SHIFT (23 - F16_EXPONENT_SHIFT)
#define F16_MAX_EXPONENT (F16_EXPONENT_BITS << F16_EXPONENT_SHIFT)

static inline uint16_t util_float_to_half(float val)
{
	uint32_t f32 = (*(uint32_t*)&val);
	uint16_t f16 = 0;
	/* Decode IEEE 754 little-endian 32-bit floating-point value */
	int sign = (f32 >> 16) & 0x8000;
	/* Map exponent to the range [-127,128] */
	int exponent = ((f32 >> 23) & 0xff) - 127;
	int mantissa = f32 & 0x007fffff;
	if (exponent == 128)
	{ /* Infinity or NaN */
		f16 = (uint16_t)(sign | F16_MAX_EXPONENT);
		if (mantissa)
			f16 |= (mantissa & F16_MANTISSA_BITS);
	}
	else if (exponent > 15)
	{ /* Overflow - flush to Infinity */
		f16 = (unsigned short)(sign | F16_MAX_EXPONENT);
	}
	else if (exponent > -15)
	{ /* Representable value */
		exponent += F16_EXPONENT_BIAS;
		mantissa >>= F16_MANTISSA_SHIFT;
		f16 = (unsigned short)(sign | exponent << F16_EXPONENT_SHIFT | mantissa);
	}
	else
	{
		f16 = (unsigned short)sign;
	}
	return f16;
}

static inline void util_pack_float2_to_half2(uint32_t count, uint32_t stride, uint32_t offset, const uint8_t* src, uint8_t* dst)
{
	struct f2
	{
		float x;
		float y;
	};
	f2* f = (f2*)src;
	for (uint32_t e = 0; e < count; ++e)
	{
		*(uint32_t*)(dst + e * sizeof(uint32_t) + offset) =
			((util_float_to_half(f[e].x) & 0x0000FFFF) | ((util_float_to_half(f[e].y) << 16) & 0xFFFF0000));
	}
}

static inline uint32_t util_float2_to_unorm2x16(const float* v)
{
	uint32_t x = (uint32_t)round(clamp(v[0], 0, 1) * 65535.0f);
	uint32_t y = (uint32_t)round(clamp(v[1], 0, 1) * 65535.0f);
	return ((uint32_t)0x0000FFFF & x) | ((y << 16) & (uint32_t)0xFFFF0000);
}

#define OCT_WRAP(v, w) ((1.0f - abs((w))) * ((v) >= 0.0f ? 1.0f : -1.0f))

static inline void util_pack_float3_direction_to_half2(uint32_t count, uint32_t stride, uint32_t offset, const uint8_t* src, uint8_t* dst)
{
	struct f3
	{
		float x;
		float y;
		float z;
	};
	for (uint32_t e = 0; e < count; ++e)
	{
		f3    f = *(f3*)(src + e * stride);
		float absLength = (abs(f.x) + abs(f.y) + abs(f.z));
		f3    enc = {};
		if (absLength)
		{
			enc.x = f.x / absLength;
			enc.y = f.y / absLength;
			enc.z = f.z / absLength;
			if (enc.z < 0)
			{
				float oldX = enc.x;
				enc.x = OCT_WRAP(enc.x, enc.y);
				enc.y = OCT_WRAP(enc.y, oldX);
			}
			enc.x = enc.x * 0.5f + 0.5f;
			enc.y = enc.y * 0.5f + 0.5f;
			*(uint32_t*)(dst + e * sizeof(uint32_t) + offset) = util_float2_to_unorm2x16(&enc.x);
		}
		else
		{
			*(uint32_t*)(dst + e * sizeof(uint32_t) + offset) = 0;
		}
	}
}

/************************************************************************/
// Internal Structures
/************************************************************************/
typedef void (*PreMipStepFn)(FileStream* pStream, uint32_t mip);

typedef struct TextureUpdateDescInternal
{
	Texture* pTexture;
	FileStream        mStream;
	MappedMemoryRange mRange;
	uint32_t          mBaseMipLevel;
	uint32_t          mMipLevels;
	uint32_t          mBaseArrayLayer;
	uint32_t          mLayerCount;
	PreMipStepFn      pPreMipFunc;
	bool              mMipsAfterSlice;
} TextureUpdateDescInternal;

typedef struct CopyResourceSet
{
	Fence* pFence;
	Cmd* pCmd;
	CmdPool* pCmdPool;
	Buffer* mBuffer;
	uint64_t mAllocatedSpace;

	/// Buffers created in case we ran out of space in the original staging buffer
/// Will be cleaned up after the fence for this set is complete
	eastl::vector<Buffer*> mTempBuffers;

	Semaphore* pCopyCompletedSemaphore;
} CopyResourceSet;

//Synchronization?
typedef struct CopyEngine
{
	Queue* pQueue;
	CopyResourceSet* resourceSets;
	uint64_t         bufferSize;
	uint32_t         bufferCount;
	/// Node index in linked GPU mode, Renderer index in unlinked mode
	uint32_t         nodeIndex;
	bool             isRecording;
	Semaphore* pLastCompletedSemaphore;

	/// For reading back GPU generated textures, we need to ensure writes have completed before performing the copy.
	eastl::vector<Semaphore*> mWaitSemaphores;
}CopyEngine;

typedef enum UpdateRequestType
{
	UPDATE_REQUEST_UPDATE_BUFFER,
	UPDATE_REQUEST_UPDATE_TEXTURE,
	UPDATE_REQUEST_BUFFER_BARRIER,
	UPDATE_REQUEST_TEXTURE_BARRIER,
	UPDATE_REQUEST_LOAD_TEXTURE,
	UPDATE_REQUEST_LOAD_GEOMETRY,
	UPDATE_REQUEST_COPY_TEXTURE,
	UPDATE_REQUEST_INVALID,
} UpdateRequestType;

typedef enum UploadFunctionResult
{
	UPLOAD_FUNCTION_RESULT_COMPLETED,
	UPLOAD_FUNCTION_RESULT_STAGING_BUFFER_FULL,
	UPLOAD_FUNCTION_RESULT_INVALID_REQUEST
} UploadFunctionResult;

struct UpdateRequest
{
	UpdateRequest(const BufferUpdateDesc& buffer) : mType(UPDATE_REQUEST_UPDATE_BUFFER), bufUpdateDesc(buffer) {}
	UpdateRequest(const TextureLoadDesc& texture) : mType(UPDATE_REQUEST_LOAD_TEXTURE), texLoadDesc(texture) {}
	UpdateRequest(const TextureUpdateDescInternal& texture) : mType(UPDATE_REQUEST_UPDATE_TEXTURE), texUpdateDesc(texture) {}
	UpdateRequest(const GeometryLoadDesc& geom) : mType(UPDATE_REQUEST_LOAD_GEOMETRY), geomLoadDesc(geom) {}
	UpdateRequest(const BufferBarrier& barrier) : mType(UPDATE_REQUEST_BUFFER_BARRIER), bufferBarrier(barrier) {}
	UpdateRequest(const TextureBarrier& barrier) : mType(UPDATE_REQUEST_TEXTURE_BARRIER), textureBarrier(barrier) {}
	UpdateRequest(const TextureCopyDesc& texture) : mType(UPDATE_REQUEST_COPY_TEXTURE), texCopyDesc(texture) {}

	UpdateRequestType mType = UPDATE_REQUEST_INVALID;
	uint64_t          mWaitIndex = 0;
	Buffer* pUploadBuffer = NULL;
	union
	{
		BufferUpdateDesc          bufUpdateDesc;
		TextureUpdateDescInternal texUpdateDesc;
		TextureLoadDesc           texLoadDesc;
		GeometryLoadDesc          geomLoadDesc;
		BufferBarrier             bufferBarrier;
		TextureBarrier            textureBarrier;
		TextureCopyDesc           texCopyDesc;
	};
};

struct ResourceLoader
{
	Renderer* ppRenderers[MAX_MULTIPLE_GPUS];
	uint32_t mGpuCount;

	ResourceLoaderDesc mDesc;

	volatile int mRun;
	ThreadHandle mThread;

	Mutex                        mQueueMutex;
	ConditionVariable            mQueueCond;
	Mutex                        mTokenMutex;
	ConditionVariable            mTokenCond;
	eastl::vector<UpdateRequest> mRequestQueue[MAX_MULTIPLE_GPUS];

	tfrg_atomic64_t mTokenCompleted;
	tfrg_atomic64_t mTokenSubmitted;
	tfrg_atomic64_t mTokenCounter;

	Mutex mSemaphoreMutex;

	SyncToken mCurrentTokenState[MAX_FRAMES];

	CopyEngine pCopyEngines[MAX_MULTIPLE_GPUS];
	uint32_t   mNextSet;
	uint32_t   mSubmittedSets;
};

static ResourceLoader* pResourceLoader = NULL;

/************************************************************************/
// Internal Functions
/************************************************************************/
/// Return a new staging buffer
static MappedMemoryRange allocateUploadMemory(Renderer* pRenderer, uint64_t memoryRequirement, uint32_t alignment)
{
	Buffer* buffer;
	{
		//LOGF(LogLevel::eINFO, "Allocating temporary staging buffer. Required allocation size of %llu is larger than the staging buffer capacity of %llu", memoryRequirement, size);
		buffer = {};
		BufferDesc bufferDesc = {};
		bufferDesc.mSize = memoryRequirement;
		bufferDesc.mAlignment = alignment;
		bufferDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_ONLY;
		bufferDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
		bufferDesc.mNodeIndex = pRenderer->mUnlinkedRendererIndex;
		vk_addBuffer(pRenderer, &bufferDesc, &buffer);
	}
	return { (uint8_t*)buffer->pCpuMappedAddress, buffer, 0, memoryRequirement };
}

static void cleanupCopyEngine(Renderer* pRenderer, CopyEngine* pCopyEngine)
{
	for (uint32_t i = 0; i < pCopyEngine->bufferCount; ++i)
	{
		CopyResourceSet& resourceSet = pCopyEngine->resourceSets[i];
		vk_removeBuffer(pRenderer, resourceSet.mBuffer);

		vk_removeSemaphore(pRenderer, resourceSet.pCopyCompletedSemaphore);

		vk_removeCmd(pRenderer, resourceSet.pCmd);
		vk_removeCmdPool(pRenderer, resourceSet.pCmdPool);
		vk_removeFence(pRenderer, resourceSet.pFence);

		if (!resourceSet.mTempBuffers.empty())
			LOGF(eINFO, "Was not cleaned up %d", i);
		for (Buffer*& buffer : resourceSet.mTempBuffers)
		{
			vk_removeBuffer(pRenderer, buffer);
		}
		pCopyEngine->resourceSets[i].mTempBuffers.set_capacity(0);
	}

	tf_free(pCopyEngine->resourceSets);

	vk_removeQueue(pRenderer, pCopyEngine->pQueue);
}

static bool waitCopyEngineSet(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, bool wait)
{
	ASSERT(!pCopyEngine->isRecording);
	CopyResourceSet& resourceSet = pCopyEngine->resourceSets[activeSet];
	bool completed = true;

	FenceStatus status;
	vk_getFenceStatus(pRenderer, resourceSet.pFence, &status);
	completed = status != FENCE_STATUS_INCOMPLETE;
	if (wait && !completed)
	{
		vk_waitForFences(pRenderer, 1, &resourceSet.pFence);
	}
	return completed;
}

static uint32_t util_get_texture_row_alignment(Renderer* pRenderer)
{
	return max(1u, pRenderer->pActiveGpuSettings->mUploadBufferTextureRowAlignment);
}

static uint32_t util_get_texture_subresource_alignment(Renderer* pRenderer, TinyImageFormat fmt = TinyImageFormat_UNDEFINED)
{
	uint32_t blockSize = max(1u, TinyImageFormat_BitSizeOfBlock(fmt) >> 3);
	uint32_t alignment = round_up(pRenderer->pActiveGpuSettings->mUploadBufferTextureAlignment, blockSize);
	return round_up(alignment, util_get_texture_row_alignment(pRenderer));
}

static void resetCopyEngineSet(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet)
{
	ASSERT(!pCopyEngine->isRecording);
	pCopyEngine->resourceSets[activeSet].mAllocatedSpace = 0;
	pCopyEngine->isRecording = false;

	for (Buffer*& buffer : pCopyEngine->resourceSets[activeSet].mTempBuffers)
	{
		vk_removeBuffer(pRenderer, buffer);
	}
	pCopyEngine->resourceSets[activeSet].mTempBuffers.clear();
}

static Cmd* acquireCmd(CopyEngine* pCopyEngine, size_t activeSet)
{
	CopyResourceSet& resourceSet = pCopyEngine->resourceSets[activeSet];
	if (!pCopyEngine->isRecording)
	{
		vk_resetCmdPool(pResourceLoader->ppRenderers[pCopyEngine->nodeIndex], resourceSet.pCmdPool);
		vk_beginCmd(resourceSet.pCmd);
		pCopyEngine->isRecording = true;
	}
	return resourceSet.pCmd;
}

static void streamerFlush(CopyEngine* pCopyEngine, size_t activeSet)
{
	if (pCopyEngine->isRecording)
	{
		CopyResourceSet& resourceSet = pCopyEngine->resourceSets[activeSet];
		vk_endCmd(resourceSet.pCmd);
		QueueSubmitDesc submitDesc = {};
		submitDesc.mCmdCount = 1;
		submitDesc.ppCmds = &resourceSet.pCmd;
		submitDesc.mSignalSemaphoreCount = 1;
		submitDesc.ppSignalSemaphores = &resourceSet.pCopyCompletedSemaphore;
		submitDesc.pSignalFence = resourceSet.pFence;
		if (!pCopyEngine->mWaitSemaphores.empty())
		{
			submitDesc.mWaitSemaphoreCount = (uint32_t)pCopyEngine->mWaitSemaphores.size();
			submitDesc.ppWaitSemaphores = &pCopyEngine->mWaitSemaphores[0];
			pCopyEngine->mWaitSemaphores.clear();
		}
		{
			vk_queueSubmit(pCopyEngine->pQueue, &submitDesc);
		}
		pCopyEngine->isRecording = false;
	}
}

static MappedMemoryRange allocateStagingMemory(uint64_t memoryRequirement, uint32_t alignment, uint32_t nodeIndex)
{
	CopyEngine* pCopyEngine = &pResourceLoader->pCopyEngines[nodeIndex];

	uint64_t offset = pCopyEngine->resourceSets[pResourceLoader->mNextSet].mAllocatedSpace;
	if (alignment != 0)
	{
		offset = round_up_64(offset, alignment);
	}

	CopyResourceSet* pResourceSet = &pCopyEngine->resourceSets[pResourceLoader->mNextSet];
	uint64_t         size = (uint64_t)pResourceSet->mBuffer->mSize;
	bool             memoryAvailable = (offset < size) && (memoryRequirement <= size - offset);
	if (memoryAvailable && pResourceSet->mBuffer->pCpuMappedAddress)
	{
		Buffer* buffer = pResourceSet->mBuffer;
		ASSERT(buffer->pCpuMappedAddress);
		uint8_t* pDstData = (uint8_t*)buffer->pCpuMappedAddress + offset;
		pCopyEngine->resourceSets[pResourceLoader->mNextSet].mAllocatedSpace = offset + memoryRequirement;
		return { pDstData, buffer, offset, memoryRequirement };
	}
	else
	{
		if (pCopyEngine->bufferSize < memoryRequirement)
		{
			MappedMemoryRange range = allocateUploadMemory(pResourceLoader->ppRenderers[nodeIndex], memoryRequirement, alignment);
			//LOGF(LogLevel::eINFO, "Allocating temporary staging buffer. Required allocation size of %llu is larger than the staging buffer capacity of %llu", memoryRequirement, size);
			pResourceSet->mTempBuffers.emplace_back(range.pBuffer);
			return range;
		}
	}

	MappedMemoryRange range = allocateUploadMemory(pResourceLoader->ppRenderers[nodeIndex], memoryRequirement, alignment);
	//LOGF(LogLevel::eINFO, "Allocating temporary staging buffer. Required allocation size of %llu is larger than the staging buffer capacity of %llu", memoryRequirement, size);
	pResourceSet->mTempBuffers.emplace_back(range.pBuffer);
	return range;
}

static void freeAllUploadMemory()
{
	for (size_t i = 0; i < MAX_MULTIPLE_GPUS; ++i)
	{
		for (UpdateRequest& request : pResourceLoader->mRequestQueue[i])
		{
			if (request.pUploadBuffer)
			{
				vk_removeBuffer(pResourceLoader->ppRenderers[i], request.pUploadBuffer);
			}
		}
	}
}

static UploadFunctionResult updateTexture(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, const TextureUpdateDescInternal& texUpdateDesc)
{
	// When this call comes from updateResource, staging buffer data is already filled
	// All that is left to do is record and execute the Copy commands
	bool dataAlreadyFilled = texUpdateDesc.mRange.pBuffer ? true : false;
	Texture* texture = texUpdateDesc.pTexture;
	const TinyImageFormat fmt = (TinyImageFormat)texture->mFormat;
	FileStream stream = texUpdateDesc.mStream;
	Cmd* cmd = acquireCmd(pCopyEngine, activeSet);

	ASSERT(pCopyEngine->pQueue->mNodeIndex == texUpdateDesc.pTexture->mNodeIndex);

	const uint32_t sliceAlignment = util_get_texture_subresource_alignment(pRenderer, fmt);
	const uint32_t rowAlignment = util_get_texture_row_alignment(pRenderer);
	const uint32_t requiredSize = util_get_surface_size(fmt, texture->mWidth, texture->mHeight, texture->mDepth, rowAlignment, sliceAlignment, texUpdateDesc.mBaseMipLevel,
		texUpdateDesc.mMipLevels, texUpdateDesc.mBaseArrayLayer, texUpdateDesc.mLayerCount);

	TextureBarrier barrier;
	if (gSelectedRendererApi == RENDERER_API_VULKAN)
	{
		barrier = { texture, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_COPY_DEST };
		vk_cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
	}

	MappedMemoryRange upload = dataAlreadyFilled
		? texUpdateDesc.mRange
		: allocateStagingMemory(requiredSize, sliceAlignment, texture->mNodeIndex);
	uint64_t          offset = 0;

	if (!upload.pData)
	{
		return UPLOAD_FUNCTION_RESULT_STAGING_BUFFER_FULL;
	}

	uint32_t firstStart = texUpdateDesc.mMipsAfterSlice ? texUpdateDesc.mBaseMipLevel : texUpdateDesc.mBaseArrayLayer;
	uint32_t firstEnd = texUpdateDesc.mMipsAfterSlice ? (texUpdateDesc.mBaseMipLevel + texUpdateDesc.mMipLevels)
		: (texUpdateDesc.mBaseArrayLayer + texUpdateDesc.mLayerCount);
	uint32_t secondStart = texUpdateDesc.mMipsAfterSlice ? texUpdateDesc.mBaseArrayLayer : texUpdateDesc.mBaseMipLevel;
	uint32_t secondEnd = texUpdateDesc.mMipsAfterSlice ? (texUpdateDesc.mBaseArrayLayer + texUpdateDesc.mLayerCount)
		: (texUpdateDesc.mBaseMipLevel + texUpdateDesc.mMipLevels);

	for (uint32_t p = 0; p < 1; ++p)
	{
		for (uint32_t j = firstStart; j < firstEnd; ++j)
		{
			if (texUpdateDesc.mMipsAfterSlice && texUpdateDesc.pPreMipFunc)
			{
				texUpdateDesc.pPreMipFunc(&stream, j);
			}

			for (uint32_t i = secondStart; i < secondEnd; ++i)
			{
				if (!texUpdateDesc.mMipsAfterSlice && texUpdateDesc.pPreMipFunc)
				{
					texUpdateDesc.pPreMipFunc(&stream, i);
				}

				uint32_t mip = texUpdateDesc.mMipsAfterSlice ? j : i;
				uint32_t layer = texUpdateDesc.mMipsAfterSlice ? i : j;

				uint32_t w = MIP_REDUCE(texture->mWidth, mip);
				uint32_t h = MIP_REDUCE(texture->mHeight, mip);
				uint32_t d = MIP_REDUCE(texture->mDepth, mip);

				uint32_t numBytes = 0;
				uint32_t rowBytes = 0;
				uint32_t numRows = 0;

				bool ret = util_get_surface_info(w, h, fmt, &numBytes, &rowBytes, &numRows);
				if (!ret)
				{
					return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
				}

				uint32_t subRowPitch = round_up(rowBytes, rowAlignment);
				uint32_t subSlicePitch = round_up(subRowPitch * numRows, sliceAlignment);
				uint32_t subNumRows = numRows;
				uint32_t subDepth = d;
				uint32_t subRowSize = rowBytes;
				uint8_t* data = upload.pData + offset;

				if (!dataAlreadyFilled)
				{
					for (uint32_t z = 0; z < subDepth; ++z)
					{
						uint8_t* dstData = data + subSlicePitch * z;
						for (uint32_t r = 0; r < subNumRows; ++r)
						{
							ssize_t bytesRead = fsReadFromStream(&stream, dstData + r * subRowPitch, subRowSize);
							if (bytesRead != subRowSize)
							{
								return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
							}
						}
					}
				}
				SubresourceDataDesc subresourceDesc = {};
				subresourceDesc.mArrayLayer = layer;
				subresourceDesc.mMipLevel = mip;
				subresourceDesc.mSrcOffset = upload.mOffset + offset;
				subresourceDesc.mRowPitch = subRowPitch;
				subresourceDesc.mSlicePitch = subSlicePitch;
				vk_cmdUpdateSubresource(cmd, texture, upload.pBuffer, &subresourceDesc);
				offset += subDepth * subSlicePitch;
			}
		}
	}

	if (gSelectedRendererApi == RENDERER_API_VULKAN)
	{
		barrier = { texture, RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_SHADER_RESOURCE };
		vk_cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
	}

	if (stream.pIO)
	{
		fsCloseStream(&stream);
	}

	return UPLOAD_FUNCTION_RESULT_COMPLETED;

}

static UploadFunctionResult loadTexture(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, const UpdateRequest& pTextureUpdate)
{
	const TextureLoadDesc* pTextureDesc = &pTextureUpdate.texLoadDesc;

	ASSERT((((pTextureDesc->mCreationFlag & TEXTURE_CREATION_FLAG_SRGB) == 0) ||
		(pTextureDesc->pFileName != NULL)) &&
		"Only textures loaded from file can have TEXTURE_CREATION_FLAG_SRGB. "
		"Please change format of the provided texture if you need srgb format."
	);

	if (pTextureDesc->pFileName)
	{
		FileStream stream = {};
		char fileName[FS_MAX_PATH] = {};
		bool success = false;
		TextureUpdateDescInternal updateDesc = {};
		TextureContainerType      container = pTextureDesc->mContainer;
		static const char* extensions[] = { NULL, "dds", "ktx", "gnf", "basis", "svt" };

		if (TEXTURE_CONTAINER_DEFAULT == container)
		{
			container = TEXTURE_CONTAINER_DDS;
		}
		TextureDesc textureDesc = {};
		textureDesc.pName = pTextureDesc->pFileName;
		textureDesc.mFlags |= pTextureDesc->mCreationFlag;

		// Validate that we have found the file format now
		ASSERT(container != TEXTURE_CONTAINER_DEFAULT);    //-V547
		if (TEXTURE_CONTAINER_DEFAULT == container)        //-V547
		{
			return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
		}

		fsAppendPathExtension(pTextureDesc->pFileName, extensions[container], fileName);

		switch (container)
		{
		case TEXTURE_CONTAINER_DDS:
		{
			success = fsOpenStreamFromPath(RD_TEXTURES, fileName, FM_READ_BINARY, pTextureDesc->pFilePassword, &stream);
			if (success)
			{
				success = loadDDSTextureDesc(&stream, &textureDesc);
			}
			break;
		}case TEXTURE_CONTAINER_KTX:
		{
			success = fsOpenStreamFromPath(RD_TEXTURES, fileName, FM_READ_BINARY, pTextureDesc->pFilePassword, &stream);
			if (success)
			{
				success = loadKTXTextureDesc(&stream, &textureDesc);
				updateDesc.mMipsAfterSlice = true;
				// KTX stores mip size before the mip data
				// This function gets called to skip the mip size so we read the mip data
				updateDesc.pPreMipFunc = [](FileStream* pStream, uint32_t) {
					uint32_t mipSize = 0;
					fsReadFromStream(pStream, &mipSize, sizeof(mipSize));
				};
			}
			break;
		}
		case TEXTURE_CONTAINER_BASIS:
		{
			void* data = NULL;
			uint32_t dataSize = 0;
			success = fsOpenStreamFromPath(RD_TEXTURES, fileName, FM_READ_BINARY, pTextureDesc->pFilePassword, &stream);
			if (success)
			{
				success = loadBASISTextureDesc(&stream, &textureDesc, &data, &dataSize);
				if (success)
				{
					fsCloseStream(&stream);
					fsOpenStreamFromMemory(data, dataSize, FM_READ_BINARY, true, &stream);
				}
			}
			break;
		}
		case TEXTURE_CONTAINER_GNF:
		{

		}
		default: break;
		}

		if (success)
		{
			textureDesc.mStartState = RESOURCE_STATE_COMMON;
			textureDesc.mNodeIndex = pTextureDesc->mNodeIndex;

			if (pTextureDesc->mCreationFlag & TEXTURE_CREATION_FLAG_SRGB)
			{
				TinyImageFormat srgbFormat = TinyImageFormat_ToSRGB(textureDesc.mFormat);
				if (srgbFormat != TinyImageFormat_UNDEFINED)
					textureDesc.mFormat = srgbFormat;
				else
				{
					LOGF(eWARNING,
						"Trying to load '%s' image using SRGB profile. "
						"But image has '%s' format, which doesn't have SRGB counterpart.",
						pTextureDesc->pFileName, TinyImageFormat_Name(textureDesc.mFormat));
				}
			}

			if (NULL != pTextureDesc->pDesc)
				textureDesc.pVkSamplerYcbcrConversionInfo = pTextureDesc->pDesc->pVkSamplerYcbcrConversionInfo;
			vk_addTexture(pRenderer, &textureDesc, pTextureDesc->ppTexture);

			updateDesc.mStream = stream;
			updateDesc.pTexture = *pTextureDesc->ppTexture;
			updateDesc.mBaseMipLevel = 0;
			updateDesc.mMipLevels = textureDesc.mMipLevels;
			updateDesc.mBaseArrayLayer = 0;
			updateDesc.mLayerCount = textureDesc.mArraySize;

			return updateTexture(pRenderer, pCopyEngine, activeSet, updateDesc);
		}
		/************************************************************************/
		// Sparse Textures
		/************************************************************************/
		if (TEXTURE_CONTAINER_SVT == container)
		{
			if (fsOpenStreamFromPath(RD_TEXTURES, fileName, FM_READ_BINARY, pTextureDesc->pFilePassword, &stream))
			{
				success = loadSVTTextureDesc(&stream, &textureDesc);
				if (success)
				{
					ssize_t dataSize = fsGetStreamFileSize(&stream) - fsGetStreamSeekPosition(&stream);
					void* data = tf_malloc(dataSize);
					fsReadFromStream(&stream, data, dataSize);

					textureDesc.mStartState = RESOURCE_STATE_COPY_DEST;
					textureDesc.mFlags |= pTextureDesc->mCreationFlag;
					textureDesc.mNodeIndex = pTextureDesc->mNodeIndex;
					if (pTextureDesc->mCreationFlag & TEXTURE_CREATION_FLAG_SRGB)
					{
						TinyImageFormat srgbFormat = TinyImageFormat_ToSRGB(textureDesc.mFormat);
						if (srgbFormat != TinyImageFormat_UNDEFINED)
							textureDesc.mFormat = srgbFormat;
						else
						{
							LOGF(eWARNING,
								"Trying to load '%s' image using SRGB profile. "
								"But image has '%s' format, which doesn't have SRGB counterpart.",
								pTextureDesc->pFileName, TinyImageFormat_Name(textureDesc.mFormat));
						}
					}

					vk_addVirtualTexture(acquireCmd(pCopyEngine, activeSet), &textureDesc, pTextureDesc->ppTexture, data);

					fsCloseStream(&stream);

					if ((*pTextureDesc->ppTexture)->pSvt->mVirtualPageTotalCount == 0)
						return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;

					return UPLOAD_FUNCTION_RESULT_COMPLETED;
				}
			}

			return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
		}
	}

	return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
}

static UploadFunctionResult updateBuffer(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, const BufferUpdateDesc& bufUpdateDesc)
{
	ASSERT(pCopyEngine->pQueue->mNodeIndex == bufUpdateDesc.pBuffer->mNodeIndex);
	Buffer* pBuffer = bufUpdateDesc.pBuffer;
	ASSERT(pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY || pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_TO_CPU);

	Cmd* pCmd = acquireCmd(pCopyEngine, activeSet);

	MappedMemoryRange range = bufUpdateDesc.mInternal.mMappedRange;
	vk_cmdUpdateBuffer(pCmd, pBuffer, bufUpdateDesc.mDstOffset, range.pBuffer, range.mOffset, range.mSize);

	return UPLOAD_FUNCTION_RESULT_COMPLETED;
}

static UploadFunctionResult loadGeometry(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, UpdateRequest& pGeometryLoad)
{
	Geometry* geom = NULL;
	GeometryLoadDesc* pDesc = &pGeometryLoad.geomLoadDesc;

	BufferUpdateDesc indexUpdateDesc = {};
	BufferUpdateDesc vertexUpdateDesc[MAX_VERTEX_BINDINGS] = {};

	//data for overdraw optimization
	uint32_t positionBinding = 0;
	void* positionPointer = NULL;

	char iext[FS_MAX_PATH] = { 0 };
	fsGetPathExtension(pDesc->pFileName, iext);

	// Geometry in gltf container
	if (iext[0] != 0 && (stricmp(iext, "gltf") == 0 || stricmp(iext, "glb") == 0))
	{
		FileStream file = {};
		if (!fsOpenStreamFromPath(RD_MESHES, pDesc->pFileName, FM_READ_BINARY, pDesc->pFilePassword, &file))
		{
			LOGF(eERROR, "Failed to open gltf file %s", pDesc->pFileName);
			ASSERT(false);
			return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
		}

		ssize_t fileSize = fsGetStreamFileSize(&file);
		void* fileData = tf_malloc(fileSize);

		fsReadFromStream(&file, fileData, fileSize);

		cgltf_options options = {};
		cgltf_data* data = NULL;
		options.memory_alloc = [](void* user, cgltf_size size) { return tf_malloc(size); };
		options.memory_free = [](void* user, void* ptr) { tf_free(ptr); };
		cgltf_result result = cgltf_parse(&options, fileData, fileSize, &data);
		fsCloseStream(&file);

		if (cgltf_result_success != result)
		{
			LOGF(eERROR, "Failed to parse gltf file %s with error %u", pDesc->pFileName, (uint32_t)result);
			ASSERT(false);
			tf_free(fileData);
			return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
		}

#if defined(FORGE_DEBUG)
		result = cgltf_validate(data);
		if (cgltf_result_success != result)
		{
			LOGF(eWARNING, "GLTF validation finished with error %u for file %s", (uint32_t)result, pDesc->pFileName);
		}
#endif

		// Load buffers located in separate files (.bin) using our file system
		for (uint32_t i = 0; i < data->buffers_count; ++i)
		{
			const char* uri = data->buffers[i].uri;

			if (!uri || data->buffers[i].data)
			{
				continue;
			}

			if (strncmp(uri, "data:", 5) != 0 && !strstr(uri, "://"))
			{
				char parent[FS_MAX_PATH] = { 0 };
				fsGetParentPath(pDesc->pFileName, parent);
				char path[FS_MAX_PATH] = { 0 };
				fsAppendPathComponent(parent, uri, path);
				FileStream fs = {};
				if (fsOpenStreamFromPath(RD_MESHES, path, FM_READ_BINARY, pDesc->pFilePassword, &fs))
				{
					ASSERT(fsGetStreamFileSize(&fs) >= (ssize_t)data->buffers[i].size);
					data->buffers[i].data = tf_malloc(data->buffers[i].size);
					fsReadFromStream(&fs, data->buffers[i].data, data->buffers[i].size);
				}
				fsCloseStream(&fs);
			}
		}

		result = cgltf_load_buffers(&options, data, pDesc->pFileName);
		if (cgltf_result_success != result)
		{
			LOGF(eERROR, "Failed to load buffers from gltf file %s with error %u", pDesc->pFileName, (uint32_t)result);
			ASSERT(false);
			tf_free(fileData);
			return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
		}

		typedef void (*PackingFunction)(uint32_t count, uint32_t stride, uint32_t offset, const uint8_t* src, uint8_t* dst);

		uint32_t         vertexStrides[SEMANTIC_TEXCOORD9 + 1] = {};
		uint32_t         vertexAttribCount[SEMANTIC_TEXCOORD9 + 1] = {};
		uint32_t         vertexOffsets[SEMANTIC_TEXCOORD9 + 1] = {};
		uint32_t         vertexBindings[SEMANTIC_TEXCOORD9 + 1] = {};
		cgltf_attribute* vertexAttribs[SEMANTIC_TEXCOORD9 + 1] = {};
		PackingFunction  vertexPacking[SEMANTIC_TEXCOORD9 + 1] = {};
		for (uint32_t i = 0; i < SEMANTIC_TEXCOORD9 + 1; ++i)
			vertexOffsets[i] = UINT_MAX;

		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;
		uint32_t drawCount = 0;
		uint32_t jointCount = 0;
		uint32_t vertexBufferCount = 0;

		// Find number of traditional draw calls required to draw this piece of geometry
		// Find total index count, total vertex count
		for (uint32_t i = 0; i < data->meshes_count; ++i)
		{
			for (uint32_t p = 0; p < data->meshes[i].primitives_count; ++p)
			{
				const cgltf_primitive* prim = &data->meshes[i].primitives[p];
				indexCount += (uint32_t)prim->indices->count;
				vertexCount += (uint32_t)prim->attributes->data->count;
				++drawCount;

				for (uint32_t i = 0; i < prim->attributes_count; ++i)
					vertexAttribs[util_cgltf_attrib_type_to_semantic(prim->attributes[i].type, prim->attributes[i].index)] =
					&prim->attributes[i];
			}
		}

		uint32_t defaultTexcoordSemantic = SEMANTIC_UNDEFINED;
		uint32_t defaultTexcoordStride = 0;

		// Determine vertex stride for each binding
		for (uint32_t i = 0; i < pDesc->pVertexLayout->mAttribCount; ++i)
		{
			const VertexAttrib* attr = &pDesc->pVertexLayout->mAttribs[i];
			const cgltf_attribute* cgltfAttr = vertexAttribs[attr->mSemantic];

			bool defaultTexcoords = !cgltfAttr && (attr->mSemantic >= SEMANTIC_TEXCOORD0 && attr->mSemantic <= SEMANTIC_TEXCOORD9);
			ASSERT(cgltfAttr || defaultTexcoords);

			const uint32_t dstFormatSize = TinyImageFormat_BitSizeOfBlock(attr->mFormat) >> 3;

			if (defaultTexcoords)
			{
				// Make sure there are only 1 set of default texcoords
				ASSERT(defaultTexcoordSemantic == SEMANTIC_UNDEFINED);

				defaultTexcoordStride = dstFormatSize ? dstFormatSize : 2 * sizeof(float);    // (0.f, 0.f)
				defaultTexcoordSemantic = attr->mSemantic;

				vertexStrides[attr->mBinding] += defaultTexcoordStride;
				vertexOffsets[defaultTexcoordSemantic] = attr->mOffset;
				vertexBindings[defaultTexcoordSemantic] = attr->mBinding;
				++vertexAttribCount[attr->mBinding];

				continue;
			}

			const uint32_t srcFormatSize = (uint32_t)cgltfAttr->data->stride;    //-V522

			vertexStrides[attr->mBinding] += dstFormatSize ? dstFormatSize : srcFormatSize;
			vertexOffsets[attr->mSemantic] = attr->mOffset;
			vertexBindings[attr->mSemantic] = attr->mBinding;
			++vertexAttribCount[attr->mBinding];

			// Compare vertex attrib format to the gltf attrib type
			// Select a packing function if dst format is packed version
			// Texcoords - Pack float2 to half2
			// Directions - Pack float3 to float2 to unorm2x16 (Normal, Tangent)
			// Position - No packing yet
			const TinyImageFormat srcFormat = util_cgltf_type_to_image_format(cgltfAttr->data->type, cgltfAttr->data->component_type);
			const TinyImageFormat dstFormat = attr->mFormat == TinyImageFormat_UNDEFINED ? srcFormat : attr->mFormat;

			if (dstFormat != srcFormat)
			{
				// Select appropriate packing function which will be used when filling the vertex buffer
				switch (cgltfAttr->type)
				{
				case cgltf_attribute_type_texcoord:
				{
					if (sizeof(uint32_t) == dstFormatSize && sizeof(float[2]) == srcFormatSize)
						vertexPacking[attr->mSemantic] = util_pack_float2_to_half2;
					// #TODO: Add more variations if needed
					break;
				}
				case cgltf_attribute_type_normal:
				case cgltf_attribute_type_tangent:
				{
					if (sizeof(uint32_t) == dstFormatSize && (sizeof(float[3]) == srcFormatSize || sizeof(float[4]) == srcFormatSize))
						vertexPacking[attr->mSemantic] = util_pack_float3_direction_to_half2;
					// #TODO: Add more variations if needed
					break;
				}
				default: break;
				}
			}
		}

		// Determine number of vertex buffers needed based on number of unique bindings found
		// For each unique binding the vertex stride will be non zero
		for (uint32_t i = 0; i < MAX_VERTEX_BINDINGS; ++i)
			if (vertexStrides[i])
				++vertexBufferCount;

		for (uint32_t i = 0; i < data->skins_count; ++i)
			jointCount += (uint32_t)data->skins[i].joints_count;

		// Determine index stride
		// This depends on vertex count rather than the stride specified in gltf
		// since gltf assumes we have index buffer per primitive which is non optimal
		const uint32_t indexStride = vertexCount > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);

		uint32_t totalSize = 0;
		totalSize += round_up(sizeof(Geometry), 16);
		totalSize += round_up(drawCount * sizeof(IndirectDrawIndexArguments), 16);
		totalSize += round_up(jointCount * sizeof(mat4), 16);
		totalSize += round_up(jointCount * sizeof(uint32_t), 16);

		geom = (Geometry*)tf_calloc(1, totalSize);
		ASSERT(geom);

		geom->pDrawArgs = (IndirectDrawIndexArguments*)(geom + 1);    //-V1027
		geom->pInverseBindPoses = (mat4*)((uint8_t*)geom->pDrawArgs + round_up(drawCount * sizeof(*geom->pDrawArgs), 16));
		geom->pJointRemaps = (uint32_t*)((uint8_t*)geom->pInverseBindPoses + round_up(jointCount * sizeof(*geom->pInverseBindPoses), 16));

		uint32_t shadowSize = 0;
		if (pDesc->mFlags & GEOMETRY_LOAD_FLAG_SHADOWED)
		{
			shadowSize += (uint32_t)vertexAttribs[SEMANTIC_POSITION]->data->stride * vertexCount;
			shadowSize += (uint32_t)vertexAttribs[SEMANTIC_NORMAL]->data->stride * vertexCount;
			shadowSize += indexCount * indexStride;

			geom->pShadow = (Geometry::ShadowData*)tf_calloc(1, sizeof(Geometry::ShadowData) + shadowSize);
			geom->pShadow->pIndices = geom->pShadow + 1;
			geom->pShadow->pAttributes[SEMANTIC_POSITION] = (uint8_t*)geom->pShadow->pIndices + (indexCount * indexStride);
			geom->pShadow->pAttributes[SEMANTIC_NORMAL] = (uint8_t*)geom->pShadow->pAttributes[SEMANTIC_POSITION] +
				((uint32_t)vertexAttribs[SEMANTIC_POSITION]->data->stride * vertexCount);
			// #TODO: Add more if needed
		}

		geom->mVertexBufferCount = vertexBufferCount;
		geom->mDrawArgCount = drawCount;
		geom->mIndexCount = indexCount;
		geom->mVertexCount = vertexCount;
		geom->mIndexType = (sizeof(uint16_t) == indexStride) ? INDEX_TYPE_UINT16 : INDEX_TYPE_UINT32;
		geom->mJointCount = jointCount;

		// Allocate buffer memory
		const bool structuredBuffers = (pDesc->mFlags & GEOMETRY_LOAD_FLAG_STRUCTURED_BUFFERS);

		// Index buffer
		BufferDesc indexBufferDesc = {};
		indexBufferDesc.mDescriptors =
			DESCRIPTOR_TYPE_INDEX_BUFFER | (structuredBuffers ? (DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER)
				: (DESCRIPTOR_TYPE_BUFFER_RAW | DESCRIPTOR_TYPE_RW_BUFFER_RAW));
		indexBufferDesc.mSize = indexStride * indexCount;
		indexBufferDesc.mElementCount = indexBufferDesc.mSize / (structuredBuffers ? indexStride : sizeof(uint32_t));
		indexBufferDesc.mStructStride = indexStride;
		indexBufferDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		indexBufferDesc.mStartState = gUma ? RESOURCE_STATE_INDEX_BUFFER : indexBufferDesc.mStartState;
		vk_addBuffer(pRenderer, &indexBufferDesc, &geom->pIndexBuffer);

		indexUpdateDesc.mSize = indexCount * indexStride;
		indexUpdateDesc.pBuffer = geom->pIndexBuffer;
		if (gUma)
		{
			indexUpdateDesc.mInternal.mMappedRange = { (uint8_t*)geom->pIndexBuffer->pCpuMappedAddress };
		}
		else
		{
			indexUpdateDesc.mInternal.mMappedRange = allocateStagingMemory(indexUpdateDesc.mSize, RESOURCE_BUFFER_ALIGNMENT, pDesc->mNodeIndex);
		}
		indexUpdateDesc.pMappedData = indexUpdateDesc.mInternal.mMappedRange.pData;

		uint32_t bufferCounter = 0;
		for (uint32_t i = 0; i < MAX_VERTEX_BINDINGS; ++i)
		{
			if (!vertexStrides[i])
				continue;

			BufferDesc vertexBufferDesc = {};
			vertexBufferDesc.mDescriptors =
				DESCRIPTOR_TYPE_VERTEX_BUFFER | (structuredBuffers ? (DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER)
					: (DESCRIPTOR_TYPE_BUFFER_RAW | DESCRIPTOR_TYPE_RW_BUFFER_RAW));
			vertexBufferDesc.mSize = vertexStrides[i] * vertexCount;
			vertexBufferDesc.mElementCount = vertexBufferDesc.mSize / (structuredBuffers ? vertexStrides[i] : sizeof(uint32_t));
			vertexBufferDesc.mStructStride = vertexStrides[i];
			vertexBufferDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
			vertexBufferDesc.mStartState = gUma ? RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : vertexBufferDesc.mStartState;
			vk_addBuffer(pRenderer, &vertexBufferDesc, &geom->pVertexBuffers[bufferCounter]);

			geom->mVertexStrides[bufferCounter] = vertexStrides[i];

			vertexUpdateDesc[i].pBuffer = geom->pVertexBuffers[bufferCounter];
			vertexUpdateDesc[i].mSize = vertexBufferDesc.mSize;
			if (gUma)
			{
				vertexUpdateDesc[i].mInternal.mMappedRange = { (uint8_t*)geom->pVertexBuffers[bufferCounter]->pCpuMappedAddress, 0 };
			}
			else
			{
				vertexUpdateDesc[i].mInternal.mMappedRange = allocateStagingMemory(vertexUpdateDesc[i].mSize, RESOURCE_BUFFER_ALIGNMENT, pDesc->mNodeIndex);
			}
			vertexUpdateDesc[i].pMappedData = vertexUpdateDesc[i].mInternal.mMappedRange.pData;
			++bufferCounter;
		}

		indexCount = 0;
		vertexCount = 0;
		drawCount = 0;

		for (uint32_t i = 0; i < data->meshes_count; ++i)
		{
			for (uint32_t p = 0; p < data->meshes[i].primitives_count; ++p)
			{
				const cgltf_primitive* prim = &data->meshes[i].primitives[p];
				/************************************************************************/
				// Fill index buffer for this primitive
				/************************************************************************/
				if (sizeof(uint16_t) == indexStride)
				{
					uint16_t* dst = (uint16_t*)indexUpdateDesc.pMappedData;
					for (uint32_t idx = 0; idx < prim->indices->count; ++idx)
						dst[indexCount + idx] = vertexCount + (uint16_t)cgltf_accessor_read_index(prim->indices, idx);
				}
				else
				{
					uint32_t* dst = (uint32_t*)indexUpdateDesc.pMappedData;
					for (uint32_t idx = 0; idx < prim->indices->count; ++idx)
						dst[indexCount + idx] = vertexCount + (uint32_t)cgltf_accessor_read_index(prim->indices, idx);
				}
				/************************************************************************/
				// Fill vertex buffers for this primitive
				/************************************************************************/
				for (uint32_t a = 0; a < prim->attributes_count; ++a)
				{
					cgltf_attribute* attr = &prim->attributes[a];
					uint32_t         index = util_cgltf_attrib_type_to_semantic(attr->type, attr->index);

					if (vertexOffsets[index] != UINT_MAX)
					{
						const uint32_t binding = vertexBindings[index];
						const uint32_t offset = vertexOffsets[index];
						const uint32_t stride = vertexStrides[binding];
						const uint8_t* src =
							(uint8_t*)attr->data->buffer_view->buffer->data + attr->data->offset + attr->data->buffer_view->offset;

						// If this vertex attribute is not interleaved with any other attribute use fast path instead of copying one by one
						// In this case a simple memcpy will be enough to transfer the data to the buffer
						if (1 == vertexAttribCount[binding])
						{
							uint8_t* dst = (uint8_t*)vertexUpdateDesc[binding].pMappedData + vertexCount * stride;
							if (vertexPacking[index])
								vertexPacking[index]((uint32_t)attr->data->count, (uint32_t)attr->data->stride, 0, src, dst);
							else
								memcpy(dst, src, attr->data->count * attr->data->stride);
						}
						else
						{
							uint8_t* dst = (uint8_t*)vertexUpdateDesc[binding].pMappedData + vertexCount * stride;
							// Loop through all vertices copying into the correct place in the vertex buffer
							// Example:
							// [ POSITION | NORMAL | TEXCOORD ] => [ 0 | 12 | 24 ], [ 32 | 44 | 52 ], ... (vertex stride of 32 => 12 + 12 + 8)
							if (vertexPacking[index])
								vertexPacking[index]((uint32_t)attr->data->count, (uint32_t)attr->data->stride, offset, src, dst);
							else
								for (uint32_t e = 0; e < attr->data->count; ++e)
									memcpy(dst + e * stride + offset, src + e * attr->data->stride, attr->data->stride);
						}
					}
				}

				// If used, set default texcoords in buffer to (0.f, 0.f) - assume copy engine does not give us zero'd memory
				if (defaultTexcoordSemantic != SEMANTIC_UNDEFINED)
				{
					const uint32_t binding = vertexBindings[defaultTexcoordSemantic];
					const uint32_t offset = vertexOffsets[defaultTexcoordSemantic];
					const uint32_t stride = vertexStrides[binding];

					uint8_t* dst = (uint8_t*)vertexUpdateDesc[binding].pMappedData + vertexCount * stride;
					const uint32_t count = (uint32_t)prim->attributes[0].data->count;

					for (uint32_t i = 0; i < count; ++i)
						memset(dst + i * stride + offset, 0, defaultTexcoordStride);
				}

				/************************************************************************/
				// Fill draw arguments for this primitive
				/************************************************************************/
				geom->pDrawArgs[drawCount].mIndexCount = (uint32_t)prim->indices->count;
				geom->pDrawArgs[drawCount].mInstanceCount = 1;
				geom->pDrawArgs[drawCount].mStartIndex = indexCount;
				geom->pDrawArgs[drawCount].mStartInstance = 0;
				// Since we already offset indices when creating the index buffer, vertex offset will be zero
				// With this approach, we can draw everything in one draw call or use the traditional draw per subset without the
				// need for changing shader code
				geom->pDrawArgs[drawCount].mVertexOffset = 0;

				indexCount += (uint32_t)prim->indices->count;
				vertexCount += (uint32_t)prim->attributes->data->count;
				++drawCount;
			}
		}

		// Load the remap joint indices generated in the offline process
		uint32_t remapCount = 0;
		for (uint32_t i = 0; i < data->skins_count; ++i)
		{
			const cgltf_skin* skin = &data->skins[i];
			uint32_t          extrasSize = (uint32_t)(skin->extras.end_offset - skin->extras.start_offset);
			if (extrasSize)
			{
				const char* jointRemaps = (const char*)data->json + skin->extras.start_offset;
				jsmn_parser parser = {};
				jsmntok_t* tokens = (jsmntok_t*)tf_malloc((skin->joints_count + 1) * sizeof(jsmntok_t));
				jsmn_parse(&parser, (const char*)jointRemaps, extrasSize, tokens, skin->joints_count + 1);
				ASSERT(tokens[0].size == (int)skin->joints_count + 1);
				cgltf_accessor_unpack_floats(
					skin->inverse_bind_matrices, (cgltf_float*)geom->pInverseBindPoses,
					skin->joints_count * sizeof(float[16]) / sizeof(float));
				for (uint32_t r = 0; r < skin->joints_count; ++r)
					geom->pJointRemaps[remapCount + r] = atoi(jointRemaps + tokens[1 + r].start);
				tf_free(tokens);
			}

			remapCount += (uint32_t)skin->joints_count;
		}

		// Load the tressfx specific data generated in the offline process
		if (stricmp(data->asset.generator, "tressfx") == 0)
		{
			// { "mVertexCountPerStrand" : "16", "mGuideCountPerStrand" : "3456" }
			uint32_t    extrasSize = (uint32_t)(data->asset.extras.end_offset - data->asset.extras.start_offset);
			const char* json = data->json + data->asset.extras.start_offset;
			jsmn_parser parser = {};
			jsmntok_t   tokens[5] = {};
			jsmn_parse(&parser, (const char*)json, extrasSize, tokens, 5);
			geom->mHair.mVertexCountPerStrand = atoi(json + tokens[2].start);
			geom->mHair.mGuideCountPerStrand = atoi(json + tokens[4].start);
		}

		if (pDesc->mFlags & GEOMETRY_LOAD_FLAG_SHADOWED)
		{
			indexCount = 0;
			vertexCount = 0;

			for (uint32_t i = 0; i < data->meshes_count; ++i)
			{
				for (uint32_t p = 0; p < data->meshes[i].primitives_count; ++p)
				{
					const cgltf_primitive* prim = &data->meshes[i].primitives[p];
					/************************************************************************/
					// Fill index buffer for this primitive
					/************************************************************************/
					if (sizeof(uint16_t) == indexStride)
					{
						uint16_t* dst = (uint16_t*)geom->pShadow->pIndices;
						for (uint32_t idx = 0; idx < prim->indices->count; ++idx)
							dst[indexCount + idx] = vertexCount + (uint16_t)cgltf_accessor_read_index(prim->indices, idx);
					}
					else
					{
						uint32_t* dst = (uint32_t*)geom->pShadow->pIndices;
						for (uint32_t idx = 0; idx < prim->indices->count; ++idx)
							dst[indexCount + idx] = vertexCount + (uint32_t)cgltf_accessor_read_index(prim->indices, idx);
					}

					for (uint32_t a = 0; a < prim->attributes_count; ++a)
					{
						cgltf_attribute* attr = &prim->attributes[a];
						if (cgltf_attribute_type_position == attr->type)
						{
							const uint8_t* src =
								(uint8_t*)attr->data->buffer_view->buffer->data + attr->data->offset + attr->data->buffer_view->offset;
							uint8_t* dst = (uint8_t*)geom->pShadow->pAttributes[SEMANTIC_POSITION] + vertexCount * attr->data->stride;
							memcpy(dst, src, attr->data->count * attr->data->stride);
						}
						else if (cgltf_attribute_type_normal == attr->type)
						{
							const uint8_t* src =
								(uint8_t*)attr->data->buffer_view->buffer->data + attr->data->offset + attr->data->buffer_view->offset;
							uint8_t* dst = (uint8_t*)geom->pShadow->pAttributes[SEMANTIC_NORMAL] + vertexCount * attr->data->stride;
							memcpy(dst, src, attr->data->count * attr->data->stride);
						}
					}

					indexCount += (uint32_t)prim->indices->count;
					vertexCount += (uint32_t)prim->attributes->data->count;
				}
			}
		}

		//fill the vertexPointer only if the final layout is a float
		for (size_t i = 0; i < pDesc->pVertexLayout->mAttribCount; i++)
		{
			TinyImageFormat v = pDesc->pVertexLayout->mAttribs[i].mFormat;

			if (pDesc->pVertexLayout->mAttribs[i].mSemantic == SEMANTIC_POSITION &&
				(v == TinyImageFormat_R32G32B32_SFLOAT || v == TinyImageFormat_R32G32B32A32_SFLOAT))
			{
				positionBinding = vertexBindings[SEMANTIC_POSITION];
				positionPointer = (char*)vertexUpdateDesc[positionBinding].pMappedData + vertexOffsets[SEMANTIC_POSITION];
			}
		}

		data->file_data = fileData;
		cgltf_free(data);

		tf_free(pDesc->pVertexLayout);

		*pDesc->ppGeometry = geom;
	}

	// Optmize mesh
#if defined(ENABLE_MESHOPTIMIZER)
	if (pDesc->mOptimizationFlags && geom)
	{
		size_t optimizerScratchSize = 128 * 1024 * 1024;
		size_t remapSize = (geom->mVertexCount * sizeof(uint32_t));
		size_t totalScratchSize = remapSize + optimizerScratchSize;

		//ramap + optimizer scratch
		uint32_t* scratchMemory = (uint32_t*)tf_malloc(totalScratchSize);

		uint32_t* remap = scratchMemory;

		void* optimizerScratch = &scratchMemory[geom->mVertexCount];
		meshopt_SetScratchMemory(optimizerScratchSize, optimizerScratch);

		meshopt_Stream streams[MAX_VERTEX_BINDINGS];
		for (size_t i = 0; i < MAX_VERTEX_BINDINGS; i++)
		{
			if (!geom->mVertexStrides[i])
				continue;

			streams[i].data = vertexUpdateDesc[i].pMappedData;
			streams[i].size = geom->mVertexStrides[i];
			streams[i].stride = geom->mVertexStrides[i];
		}

		//generating remap & new vertex/index sets
		if (geom->mIndexType == INDEX_TYPE_UINT16)
		{
			meshopt_generateVertexRemapMulti(remap, (uint16_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount, streams, geom->mVertexBufferCount);
			meshopt_remapIndexBuffer((uint16_t*)indexUpdateDesc.pMappedData, (uint16_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, remap);
		}
		else
		{
			meshopt_generateVertexRemapMulti(remap, (uint32_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount, streams, geom->mVertexBufferCount);
			meshopt_remapIndexBuffer((uint32_t*)indexUpdateDesc.pMappedData, (uint32_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, remap);
		}

		for (size_t i = 0; i < MAX_VERTEX_BINDINGS; i++)
		{
			if (!geom->mVertexStrides[i])
				continue;

			meshopt_remapVertexBuffer(vertexUpdateDesc[i].pMappedData, vertexUpdateDesc[i].pMappedData, geom->mVertexCount, geom->mVertexStrides[i], remap);
		}

		//optimize
		//do we need to optmize per primitive ? 
		///optmizations like overdraw clearly can alter across primitives. but can vertex cache & vertex fetch do it ?
		if (geom->mIndexType == INDEX_TYPE_UINT16)
		{
			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_VERTEXCACHE)
			{
				meshopt_optimizeVertexCache((uint16_t*)indexUpdateDesc.pMappedData, (uint16_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount);
			}

			//we can only run this if position data is not packed
			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_OVERDRAW && positionPointer)
			{
				for (size_t i = 0; i < geom->mDrawArgCount; i++)
				{
					uint16_t* src = &((uint16_t*)indexUpdateDesc.pMappedData)[geom->pDrawArgs[i].mStartIndex];

					const float kThreshold = 1.01f;
					meshopt_optimizeOverdraw(src, src, geom->pDrawArgs[i].mIndexCount, (float*)positionPointer, geom->mVertexCount, geom->mVertexStrides[positionBinding], kThreshold);
				}
			}

			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_VERTEXFETCH)
			{
				meshopt_optimizeVertexFetchRemap(remap, (uint16_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount);
				meshopt_remapIndexBuffer((uint16_t*)indexUpdateDesc.pMappedData, (uint16_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, remap);

				for (size_t i = 0; i < MAX_VERTEX_BINDINGS; i++)
				{
					if (!geom->mVertexStrides[i])
						continue;

					meshopt_remapVertexBuffer(vertexUpdateDesc[i].pMappedData, vertexUpdateDesc[i].pMappedData, geom->mVertexCount, geom->mVertexStrides[i], remap);
				}
			}
		}
		else
		{
			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_VERTEXCACHE)
			{
				meshopt_optimizeVertexCache((uint32_t*)indexUpdateDesc.pMappedData, (uint32_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount);
			}

			//we can only run this if position data is not packed
			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_OVERDRAW && positionPointer)
			{
				for (size_t i = 0; i < geom->mDrawArgCount; i++)
				{
					uint32_t* src = &((uint32_t*)indexUpdateDesc.pMappedData)[geom->pDrawArgs[i].mStartIndex];

					const float kThreshold = 1.01f;
					meshopt_optimizeOverdraw(src, src, geom->pDrawArgs[i].mIndexCount, (float*)positionPointer, geom->mVertexCount, geom->mVertexStrides[positionBinding], kThreshold);
				}
			}

			if (pDesc->mOptimizationFlags & MESH_OPTIMIZATION_FLAG_VERTEXFETCH)
			{
				meshopt_optimizeVertexFetchRemap(remap, (uint32_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, geom->mVertexCount);
				meshopt_remapIndexBuffer((uint32_t*)indexUpdateDesc.pMappedData, (uint32_t*)indexUpdateDesc.pMappedData, geom->mIndexCount, remap);

				for (size_t i = 0; i < MAX_VERTEX_BINDINGS; i++)
				{
					if (!geom->mVertexStrides[i])
						continue;

					meshopt_remapVertexBuffer(vertexUpdateDesc[i].pMappedData, vertexUpdateDesc[i].pMappedData, geom->mVertexCount, geom->mVertexStrides[i], remap);
				}
			}
		}

		tf_free(scratchMemory);
	}
#endif

	// Upload mesh
	UploadFunctionResult uploadResult = UPLOAD_FUNCTION_RESULT_COMPLETED;
	if (!gUma)
	{
		uploadResult = updateBuffer(pRenderer, pCopyEngine, activeSet, indexUpdateDesc);

		for (uint32_t i = 0; i < MAX_VERTEX_BINDINGS; ++i)
		{
			if (vertexUpdateDesc[i].pMappedData)
			{
				uploadResult = updateBuffer(pRenderer, pCopyEngine, activeSet, vertexUpdateDesc[i]);
			}
		}
	}

	return uploadResult;
}

static UploadFunctionResult copyTexture(Renderer* pRenderer, CopyEngine* pCopyEngine, size_t activeSet, TextureCopyDesc& pTextureCopy)
{
	Texture* texture = pTextureCopy.pTexture;
	const TinyImageFormat fmt = (TinyImageFormat)texture->mFormat;

	Cmd* cmd = acquireCmd(pCopyEngine, activeSet);

	if (pTextureCopy.pWaitSemaphore)
		pCopyEngine->mWaitSemaphores.push_back(pTextureCopy.pWaitSemaphore);
	if (gSelectedRendererApi == RENDERER_API_VULKAN)
	{
		TextureBarrier barrier = { texture, pTextureCopy.mTextureState, RESOURCE_STATE_COPY_SOURCE };
		barrier.mAcquire = 1;
		barrier.mQueueType = pTextureCopy.mQueueType;
		vk_cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
	}
	uint32_t numBytes = 0;
	uint32_t rowBytes = 0;
	uint32_t numRows = 0;

	bool ret = util_get_surface_info(texture->mWidth, texture->mHeight, fmt, &numBytes, &rowBytes, &numRows);
	if (!ret)
	{
		return UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;
	}

	SubresourceDataDesc subresourceDesc = {};
	subresourceDesc.mArrayLayer = pTextureCopy.mTextureArrayLayer;
	subresourceDesc.mMipLevel = pTextureCopy.mTextureMipLevel;
	subresourceDesc.mSrcOffset = pTextureCopy.mBufferOffset;
	const uint32_t sliceAlignment = util_get_texture_subresource_alignment(pRenderer, fmt);
	const uint32_t rowAlignment = util_get_texture_row_alignment(pRenderer);
	uint32_t subRowPitch = round_up(rowBytes, rowAlignment);
	uint32_t subSlicePitch = round_up(subRowPitch * numRows, sliceAlignment);
	subresourceDesc.mRowPitch = subRowPitch;
	subresourceDesc.mSlicePitch = subSlicePitch;
	vk_cmdCopySubresource(cmd, pTextureCopy.pBuffer, pTextureCopy.pTexture, &subresourceDesc);


	return UPLOAD_FUNCTION_RESULT_COMPLETED;

}

static void queueTextureUpdate(ResourceLoader* pLoader, TextureUpdateDescInternal* pTextureUpdate, SyncToken* token)
{
	ASSERT(pTextureUpdate->mRange.pBuffer);

	uint32_t nodeIndex = pTextureUpdate->pTexture->mNodeIndex;
	acquireMutex(&pLoader->mQueueMutex);

	SyncToken t = tfrg_atomic64_add_relaxed(&pLoader->mTokenCounter, 1) + 1;

	pLoader->mRequestQueue[nodeIndex].emplace_back(UpdateRequest(*pTextureUpdate));
	pLoader->mRequestQueue[nodeIndex].back().mWaitIndex = t;
	pLoader->mRequestQueue[nodeIndex].back().pUploadBuffer =
		(pTextureUpdate->mRange.mFlags & MAPPED_RANGE_FLAG_TEMP_BUFFER) ? pTextureUpdate->mRange.pBuffer : NULL;
	releaseMutex(&pLoader->mQueueMutex);
	wakeOneConditionVariable(&pLoader->mQueueCond);
	if (token)
		*token = max(t, *token);
}

/************************************************************************/
// Internal Resource Loader Implementation
/************************************************************************/
static bool areTasksAvailable(ResourceLoader* pLoader)
{
	for (size_t i = 0; i < MAX_MULTIPLE_GPUS; i++)
	{
		if (!pLoader->mRequestQueue[i].empty())
		{
			return true;
		}
	}
	return false;
}

static void streamerThreadFunc(void* pThreadData)
{
	ResourceLoader* pLoader = (ResourceLoader*)pThreadData;
	ASSERT(pLoader);

	SyncToken maxToken = {};

	while (pLoader->mRun)
	{
		acquireMutex(&pLoader->mQueueMutex);

		// Check for pending tokens
		// Safe to use mTokenCounter as we are inside critical section
		bool allTokensSingnaled = (pLoader->mTokenCompleted == tfrg_atomic64_load_relaxed(&pLoader->mTokenCounter));

		while (!areTasksAvailable(pLoader) && allTokensSingnaled && pLoader->mRun)
		{
			// No waiting if not running dedicated resource loader thread.
			if (pLoader->mDesc.mSingleThreaded)
			{
				releaseMutex(&pLoader->mQueueMutex);
				return;
			}
			// Sleep until someone adds an update request to the queue
			waitConditionVariable(&pLoader->mQueueCond, &pLoader->mQueueMutex, TIMEOUT_INFINITE);
		}

		releaseMutex(&pLoader->mQueueMutex);

		pLoader->mNextSet = (pLoader->mNextSet + 1) % pLoader->mDesc.mBufferCount;
		for (uint32_t nodeIndex = 0; nodeIndex < pLoader->mGpuCount; ++nodeIndex)
		{
			waitCopyEngineSet(pLoader->ppRenderers[nodeIndex], &pLoader->pCopyEngines[nodeIndex], pLoader->mNextSet, true);
			resetCopyEngineSet(pLoader->ppRenderers[nodeIndex], &pLoader->pCopyEngines[nodeIndex], pLoader->mNextSet);
		}

		// Signal pending tokens from previous frames
		acquireMutex(&pLoader->mTokenMutex);
		tfrg_atomic64_store_release(&pLoader->mTokenCompleted, pLoader->mCurrentTokenState[pLoader->mNextSet]);
		releaseMutex(&pLoader->mTokenMutex);
		wakeAllConditionVariable(&pLoader->mTokenCond);

		uint64_t completionMask = 0;

		for (uint32_t nodeIndex = 0; nodeIndex < pLoader->mGpuCount; ++nodeIndex)
		{
			acquireMutex(&pLoader->mQueueMutex);

			eastl::vector<UpdateRequest>& requestQueue = pLoader->mRequestQueue[nodeIndex];
			CopyEngine& copyEngine = pLoader->pCopyEngines[nodeIndex];

			if (!requestQueue.size())
			{
				releaseMutex(&pLoader->mQueueMutex);
				continue;
			}

			eastl::vector<UpdateRequest> activeQueue;
			eastl::swap(requestQueue, activeQueue);
			releaseMutex(&pLoader->mQueueMutex);

			Renderer* pRenderer = pLoader->ppRenderers[nodeIndex];
			size_t requestCount = activeQueue.size();
			SyncToken maxNodeToken = {};

			for (size_t j = 0; j < requestCount; ++j)
			{
				UpdateRequest updateState = activeQueue[j];

				UploadFunctionResult result = UPLOAD_FUNCTION_RESULT_COMPLETED;
				switch (updateState.mType)
				{
				case UPDATE_REQUEST_UPDATE_BUFFER:
					result = updateBuffer(pRenderer, &copyEngine, pLoader->mNextSet, updateState.bufUpdateDesc);
					break;
				case UPDATE_REQUEST_UPDATE_TEXTURE:
					result = updateTexture(pRenderer, &copyEngine, pLoader->mNextSet, updateState.texUpdateDesc);
					break;
				case UPDATE_REQUEST_BUFFER_BARRIER:
					vk_cmdResourceBarrier(acquireCmd(&copyEngine, pLoader->mNextSet), 1, &updateState.bufferBarrier, 0, NULL, 0, NULL);
					result = UPLOAD_FUNCTION_RESULT_COMPLETED;
					break;
				case UPDATE_REQUEST_TEXTURE_BARRIER:
					vk_cmdResourceBarrier(acquireCmd(&copyEngine, pLoader->mNextSet), 0, NULL, 1, &updateState.textureBarrier, 0, NULL);
					result = UPLOAD_FUNCTION_RESULT_COMPLETED;
					break;
				case UPDATE_REQUEST_LOAD_TEXTURE:
					result = loadTexture(pRenderer, &copyEngine, pLoader->mNextSet, updateState);
					break;
				case UPDATE_REQUEST_LOAD_GEOMETRY:
					result = loadGeometry(pRenderer, &copyEngine, pLoader->mNextSet, updateState);
					break;
				case UPDATE_REQUEST_COPY_TEXTURE:
					result = copyTexture(pRenderer, &copyEngine, pLoader->mNextSet, updateState.texCopyDesc);
					break;
				case UPDATE_REQUEST_INVALID: break;
				}

				if (updateState.pUploadBuffer)
				{
					CopyResourceSet& resourceSet = copyEngine.resourceSets[pLoader->mNextSet];
					resourceSet.mTempBuffers.push_back(updateState.pUploadBuffer);
				}

				bool completed = result == UPLOAD_FUNCTION_RESULT_COMPLETED || result == UPLOAD_FUNCTION_RESULT_INVALID_REQUEST;

				completionMask |= (uint64_t)completed << nodeIndex;

				if (updateState.mWaitIndex && completed)
				{
					ASSERT(maxNodeToken < updateState.mWaitIndex);
					maxNodeToken = updateState.mWaitIndex;
				}

				ASSERT(result != UPLOAD_FUNCTION_RESULT_STAGING_BUFFER_FULL);
			}
			maxToken = max(maxToken, maxNodeToken);
		}

		if (completionMask != 0)
		{
			for (uint32_t nodeIndex = 0; nodeIndex < pLoader->mGpuCount; ++nodeIndex)
			{
				if (completionMask & ((uint64_t)1 << nodeIndex))
				{
					streamerFlush(&pLoader->pCopyEngines[nodeIndex], pLoader->mNextSet);
					acquireMutex(&pLoader->mSemaphoreMutex);
					pLoader->pCopyEngines[nodeIndex].pLastCompletedSemaphore =
						pLoader->pCopyEngines[nodeIndex].resourceSets[pLoader->mNextSet].pCopyCompletedSemaphore;
					releaseMutex(&pLoader->mSemaphoreMutex);
				}
			}

		}

		SyncToken nextToken = max(maxToken, getLastTokenCompleted());
		pLoader->mCurrentTokenState[pLoader->mNextSet] = nextToken;

		// Signal submitted tokens
		acquireMutex(&pLoader->mTokenMutex);
		tfrg_atomic64_store_release(&pLoader->mTokenSubmitted, pLoader->mCurrentTokenState[pLoader->mNextSet]);
		releaseMutex(&pLoader->mTokenMutex);
		wakeAllConditionVariable(&pLoader->mTokenCond);

		if (pResourceLoader->mDesc.mSingleThreaded)
		{
			return;
		}
	}

	for (uint32_t nodeIndex = 0; nodeIndex < pLoader->mGpuCount; ++nodeIndex)
	{
		streamerFlush(&pLoader->pCopyEngines[nodeIndex], pLoader->mNextSet);
		{
			vk_waitQueueIdle(pLoader->pCopyEngines[nodeIndex].pQueue);
		}
		cleanupCopyEngine(pLoader->ppRenderers[nodeIndex], &pLoader->pCopyEngines[nodeIndex]);
	}
	freeAllUploadMemory();
}

void beginUpdateResource(TextureUpdateDesc* pTextureUpdate)
{
	const Texture* texture = pTextureUpdate->pTexture;
	const TinyImageFormat	fmt = (TinyImageFormat)texture->mFormat;
	Renderer* pRenderer = pResourceLoader->ppRenderers[texture->mNodeIndex];
	const uint32_t			alignment = util_get_texture_subresource_alignment(pRenderer, fmt);

	bool success = util_get_surface_info(MIP_REDUCE(texture->mWidth,
		pTextureUpdate->mMipLevel),
		MIP_REDUCE(texture->mHeight, pTextureUpdate->mMipLevel),
		fmt,
		&pTextureUpdate->mSrcSliceStride,
		&pTextureUpdate->mSrcRowStride,
		&pTextureUpdate->mRowCount);
	ASSERT(success);
	UNREF_PARAM(success);

	pTextureUpdate->mDstRowStride = round_up(pTextureUpdate->mSrcRowStride, util_get_texture_row_alignment(pRenderer));
	pTextureUpdate->mDstSliceStride = round_up(pTextureUpdate->mDstRowStride * pTextureUpdate->mRowCount, alignment);

	const ssize_t requiredSize = round_up(
		MIP_REDUCE(texture->mDepth, pTextureUpdate->mMipLevel) * pTextureUpdate->mDstSliceStride, alignment);

	// We need to use a staging buffer.
	pTextureUpdate->mInternal.mMappedRange = allocateUploadMemory(pRenderer, requiredSize, alignment);
	pTextureUpdate->mInternal.mMappedRange.mFlags = MAPPED_RANGE_FLAG_TEMP_BUFFER;
	pTextureUpdate->pMappedData = pTextureUpdate->mInternal.mMappedRange.pData;
}

void endUpdateResource(TextureUpdateDesc* pTextureUpdate, SyncToken* token)
{
	TextureUpdateDescInternal desc = {};
	desc.pTexture = pTextureUpdate->pTexture;
	desc.mRange = pTextureUpdate->mInternal.mMappedRange;
	desc.mBaseMipLevel = pTextureUpdate->mMipLevel;
	desc.mMipLevels = 1;
	desc.mBaseArrayLayer = pTextureUpdate->mArrayLayer;
	desc.mLayerCount = 1;
	queueTextureUpdate(pResourceLoader, &desc, token);

	// Restore the state to before the beginUpdateResource call.
	pTextureUpdate->pMappedData = NULL;
	pTextureUpdate->mInternal = {};
	if (pResourceLoader->mDesc.mSingleThreaded)
	{
		streamerThreadFunc(pResourceLoader);
	}
}

SyncToken getLastTokenCompleted()
{
	return tfrg_atomic64_load_acquire(&pResourceLoader->mTokenCompleted);
}