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
			addTexture(pRenderer, &textureDesc, pTextureDesc->ppTexture);

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

					addVirtualTexture(acquireCmd(pCopyEngine, activeSet), &textureDesc, pTextureDesc->ppTexture, data);

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