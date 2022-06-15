#include "../Include/RendererConfig.h"

#include "../ThirdParty/OpenSource/EASTL/string.h"
#include "../ThirdParty/OpenSource/EASTL/vector.h"

#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_bits.h"
#include "../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_apis.h"

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

#define MAX_FRAMES 3U

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
	CmdPool pCmdPool;
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

void beginUpdateResource(TextureUpdateDesc* pTextureUpdate)
{
	const Texture*			texture = pTextureUpdate->pTexture;
	const TinyImageFormat	fmt = (TinyImageFormat)texture->mFormat;
	Renderer*				pRenderer = pResourceLoader->ppRenderers[texture->mNodeIndex];
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

}