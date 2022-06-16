#include "../Include/RendererConfig.h"


#ifdef VULKAN
#define RENDERER_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "../Include/IRenderer.h"


#include "../../OS/Interfaces/ILog.h"
#include "../../OS/Math/MathTypes.h"

#include "../../ThirdParty/OpenSource/VulkanMemoryAllocator/VulkanMemoryAllocator.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"

#include "../Vulkan/VulkanCapsBuilder.h"

#include "../../OS/Interfaces/IMemory.h"

#define DECLARE_ZERO(type, var) type var = {};

static void* VKAPI_PTR gVkAllocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return tf_memalign(alignment, size);
}

static void* VKAPI_PTR
gVkReallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return tf_realloc(pOriginal, size);
}

static void VKAPI_PTR gVkFree(void* pUserData, void* pMemory) { tf_free(pMemory); }

static void VKAPI_PTR
gVkInternalAllocation(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

static void VKAPI_PTR
gVkInternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

VkAllocationCallbacks gVkAllocationCallbacks =
{
	// pUserData
	NULL,
	// pfnAllocation
	gVkAllocation,
	// pfnReallocation
	gVkReallocation,
	// pfnFree
	gVkFree,
	// pfnInternalAllocation
	gVkInternalAllocation,
	// pfnInternalFree
	gVkInternalFree
};

#define SAFE_FREE(p_var)       \
	if (p_var)                 \
	{                          \
		tf_free((void*)p_var); \
	}

VkBufferUsageFlags util_to_vk_buffer_usage(DescriptorType usage, bool typed)
{
	VkBufferUsageFlags result = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (usage & DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	{
		result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_RW_BUFFER)
	{
		result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (typed)
			result |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_BUFFER)
	{
		result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (typed)
			result |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_INDEX_BUFFER)
	{
		result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_VERTEX_BUFFER)
	{
		result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_INDIRECT_BUFFER)
	{
		result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	//#ifdef VK_RAYTRACING_AVAILABLE
	if (usage & DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)
	{
		result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
	}
	if (usage & DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT)
	{
		result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}
	if (usage & DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS)
	{
		result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	if (usage & DESCRIPTOR_TYPE_SHADER_BINDING_TABLE)
	{
		result |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	}
	//#endif
	return result;
}

VkAccessFlags util_to_vk_access_flags(ResourceState state)
{
	VkAccessFlags ret = 0;
	if (state & RESOURCE_STATE_COPY_SOURCE)
	{
		ret |= VK_ACCESS_TRANSFER_READ_BIT;
	}
	if (state & RESOURCE_STATE_COPY_DEST)
	{
		ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	{
		ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	}
	if (state & RESOURCE_STATE_INDEX_BUFFER)
	{
		ret |= VK_ACCESS_INDEX_READ_BIT;
	}
	if (state & RESOURCE_STATE_UNORDERED_ACCESS)
	{
		ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	}
	if (state & RESOURCE_STATE_INDIRECT_ARGUMENT)
	{
		ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	}
	if (state & RESOURCE_STATE_RENDER_TARGET)
	{
		ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	if (state & RESOURCE_STATE_DEPTH_WRITE)
	{
		ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	if (state & RESOURCE_STATE_SHADER_RESOURCE)
	{
		ret |= VK_ACCESS_SHADER_READ_BIT;
	}
	if (state & RESOURCE_STATE_PRESENT)
	{
		ret |= VK_ACCESS_MEMORY_READ_BIT;
	}
	if (state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
	{
		ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	}
	return ret;
}

/************************************************************************/
// Multi GPU Helper Functions
/************************************************************************/
void util_calculate_device_indices(
	Renderer* pRenderer,
	uint32_t nodeIndex,
	uint32_t* pSharedNodeIndices,
	uint32_t sharedNodeIndexCount,
	uint32_t* pIndices)
{
	for (uint32_t i = 0; i < pRenderer->mLinkedNodeCount; ++i)
		pIndices[i] = i;

	pIndices[nodeIndex] = nodeIndex;
	/************************************************************************/
	// Set the node indices which need sharing access to the creation node
	// Example: Texture created on GPU0 but GPU1 will need to access it, GPU2 does not care
	//		  pIndices = { 0, 0, 2 }
	/************************************************************************/
	for (uint32_t i = 0; i < sharedNodeIndexCount; ++i)
		pIndices[pSharedNodeIndices[i]] = nodeIndex;
}

VkImageLayout util_to_vk_image_layout(ResourceState usage)
{
	if (usage & RESOURCE_STATE_COPY_SOURCE)
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	if (usage & RESOURCE_STATE_COPY_DEST)
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	if (usage & RESOURCE_STATE_RENDER_TARGET)
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (usage & RESOURCE_STATE_DEPTH_WRITE)
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if (usage & RESOURCE_STATE_UNORDERED_ACCESS)
		return VK_IMAGE_LAYOUT_GENERAL;

	if (usage & RESOURCE_STATE_SHADER_RESOURCE)
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (usage & RESOURCE_STATE_PRESENT)
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	if (usage == RESOURCE_STATE_COMMON)
		return VK_IMAGE_LAYOUT_GENERAL;

#if defined(QUEST_VR)
	if (usage == RESOURCE_STATE_SHADING_RATE_SOURCE)
		return VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
#endif

	return VK_IMAGE_LAYOUT_UNDEFINED;
}

void util_get_planar_vk_image_memory_requirement(
	VkDevice device, VkImage image, uint32_t planesCount, VkMemoryRequirements* outVkMemReq, uint64_t* outPlanesOffsets)
{
	outVkMemReq->size = 0;
	outVkMemReq->alignment = 0;
	outVkMemReq->memoryTypeBits = 0;

	VkImagePlaneMemoryRequirementsInfo imagePlaneMemReqInfo = { VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, NULL };

	VkImageMemoryRequirementsInfo2 imagePlaneMemReqInfo2 = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2 };
	imagePlaneMemReqInfo2.pNext = &imagePlaneMemReqInfo;
	imagePlaneMemReqInfo2.image = image;

	VkMemoryDedicatedRequirements memDedicatedReq = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, NULL };
	VkMemoryRequirements2         memReq2 = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	memReq2.pNext = &memDedicatedReq;

	for (uint32_t i = 0; i < planesCount; ++i)
	{
		imagePlaneMemReqInfo.planeAspect = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i);
		vkGetImageMemoryRequirements2(device, &imagePlaneMemReqInfo2, &memReq2);

		outPlanesOffsets[i] += outVkMemReq->size;
		outVkMemReq->alignment = max(memReq2.memoryRequirements.alignment, outVkMemReq->alignment);
		outVkMemReq->size += round_up_64(memReq2.memoryRequirements.size, memReq2.memoryRequirements.alignment);
		outVkMemReq->memoryTypeBits |= memReq2.memoryRequirements.memoryTypeBits;
	}
}

uint32_t util_get_memory_type(
	uint32_t typeBits, const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryPropertyFlags& properties,
	VkBool32* memTypeFound = nullptr)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		LOGF(LogLevel::eERROR, "Could not find a matching memory type");
		ASSERT(0);
		return 0;
	}
}

VkSampleCountFlagBits util_to_vk_sample_count(SampleCount sampleCount)
{
	VkSampleCountFlagBits result = VK_SAMPLE_COUNT_1_BIT;
	switch (sampleCount)
	{
	case SAMPLE_COUNT_1: result = VK_SAMPLE_COUNT_1_BIT; break;
	case SAMPLE_COUNT_2: result = VK_SAMPLE_COUNT_2_BIT; break;
	case SAMPLE_COUNT_4: result = VK_SAMPLE_COUNT_4_BIT; break;
	case SAMPLE_COUNT_8: result = VK_SAMPLE_COUNT_8_BIT; break;
	case SAMPLE_COUNT_16: result = VK_SAMPLE_COUNT_16_BIT; break;
	}
	return result;
}

VkImageUsageFlags util_to_vk_image_usage(DescriptorType usage)
{
	VkImageUsageFlags result = 0;
	if (DESCRIPTOR_TYPE_TEXTURE == (usage & DESCRIPTOR_TYPE_TEXTURE))
		result |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (DESCRIPTOR_TYPE_RW_TEXTURE == (usage & DESCRIPTOR_TYPE_RW_TEXTURE))
		result |= VK_IMAGE_USAGE_STORAGE_BIT;
	return result;
}

// Determines pipeline stages involved for given accesses
VkPipelineStageFlags util_determine_pipeline_stage_flags(Renderer* pRenderer, VkAccessFlags accessFlags, QueueType queueType)
{
	VkPipelineStageFlags flags = 0;

	switch (queueType)
	{
	case QUEUE_TYPE_GRAPHICS:
	{
		if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

		if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
		{
			flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			if (pRenderer->pActiveGpuSettings->mGeometryShaderSupported)
			{
				flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			}
			if (pRenderer->pActiveGpuSettings->mTessellationSupported)
			{
				flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
				flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			}
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			if (pRenderer->mVulkan.mRaytracingSupported)
			{
				flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			}
		}

		if ((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

#if defined(QUEST_VR)
		if ((accessFlags & VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT) != 0)
			flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
#endif
		break;
	}
	case QUEUE_TYPE_COMPUTE:
	{
		if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 ||
			(accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
			(accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
			(accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
			return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		break;
	}
	case QUEUE_TYPE_TRANSFER: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	default: break;
	}

	// Compatible with both compute and graphics queues
	if ((accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) != 0)
		flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

	if ((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
		flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

	if ((accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)) != 0)
		flags |= VK_PIPELINE_STAGE_HOST_BIT;

	if (flags == 0)
		flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	return flags;
}

VkFormatFeatureFlags util_vk_image_usage_to_format_features(VkImageUsageFlags usage)
{
	VkFormatFeatureFlags result = (VkFormatFeatureFlags)0;
	if (VK_IMAGE_USAGE_SAMPLED_BIT == (usage & VK_IMAGE_USAGE_SAMPLED_BIT))
	{
		result |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	}
	if (VK_IMAGE_USAGE_STORAGE_BIT == (usage & VK_IMAGE_USAGE_STORAGE_BIT))
	{
		result |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
	}
	if (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
	{
		result |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	}
	if (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
	{
		result |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	return result;
}

VkImageAspectFlags util_vk_determine_aspect_mask(VkFormat format, bool includeStencilBit)
{
	VkImageAspectFlags result = 0;
	switch (format)
	{
		// Depth
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
		result = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
		// Stencil
	case VK_FORMAT_S8_UINT:
		result = VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
		// Depth/stencil
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		result = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (includeStencilBit)
			result |= VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
		// Assume everything else is Color
	default: result = VK_IMAGE_ASPECT_COLOR_BIT; break;
	}
	return result;
}

/************************************************************************/
// Virtual Texture
/************************************************************************/
static void alignedDivision(const VkExtent3D& extent, const VkExtent3D& granularity, VkExtent3D* out)
{
	out->width = (extent.width / granularity.width + ((extent.width % granularity.width) ? 1u : 0u));
	out->height = (extent.height / granularity.height + ((extent.height % granularity.height) ? 1u : 0u));
	out->depth = (extent.depth / granularity.depth + ((extent.depth % granularity.depth) ? 1u : 0u));
}

struct VTReadbackBufOffsets
{
	uint* pAlivePageCount;
	uint* pRemovePageCount;
	uint* pAlivePages;
	uint* pRemovePages;
	uint mTotalSize;
};

// Allocate Vulkan memory for the virtual page
static bool allocateVirtualPage(Renderer* pRenderer, Texture* pTexture, VirtualTexturePage& virtualPage, Buffer** ppIntermediateBuffer)
{
	if (virtualPage.mVulkan.imageMemoryBind.memory != VK_NULL_HANDLE)
	{
		//already filled
		return false;
	};

	BufferDesc desc = {};
	desc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER;
	desc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_ONLY;

	desc.mFirstElement = 0;
	desc.mElementCount = pTexture->pSvt->mSparseVirtualTexturePageWidth * pTexture->pSvt->mSparseVirtualTexturePageHeight;
	desc.mStructStride = sizeof(uint32_t);
	desc.mSize = desc.mElementCount * desc.mStructStride;
#if defined(ENABLE_GRAPHICS_DEBUG)
	char debugNameBuffer[MAX_DEBUG_NAME_LENGTH]{};
	snprintf(debugNameBuffer, MAX_DEBUG_NAME_LENGTH, "(tex %p) VT page #%u intermediate buffer", pTexture, virtualPage.index);
	desc.pName = debugNameBuffer;
#endif
	vk_addBuffer(pRenderer, &desc, ppIntermediateBuffer);

	VkMemoryRequirements memReqs = {};
	memReqs.size = virtualPage.mVulkan.size;
	memReqs.memoryTypeBits = pTexture->pSvt->mVulkan.mSparseMemoryTypeBits;
	memReqs.alignment = memReqs.size;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.pool = (VmaPool)pTexture->pSvt->mVulkan.pPool;

	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	CHECK_VKRESULT(vmaAllocateMemory(pRenderer->mVulkan.pVmaAllocator, &memReqs, &vmaAllocInfo, &allocation, &allocationInfo));
	ASSERT(allocation->GetAlignment() == memReqs.size || allocation->GetAlignment() == 0);

	virtualPage.mVulkan.pAllocation = allocation;
	virtualPage.mVulkan.imageMemoryBind.memory = allocation->GetMemory();
	virtualPage.mVulkan.imageMemoryBind.memoryOffset = allocation->GetOffset();

	// Sparse image memory binding
	virtualPage.mVulkan.imageMemoryBind.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	virtualPage.mVulkan.imageMemoryBind.subresource.mipLevel = virtualPage.mipLevel;
	virtualPage.mVulkan.imageMemoryBind.subresource.arrayLayer = virtualPage.layer;

	++pTexture->pSvt->mVirtualPageAliveCount;

	return true;
}

VirtualTexturePage* addPage(Renderer* pRenderer, Texture* pTexture, const VkOffset3D& offset, const VkExtent3D& extent, const VkDeviceSize size, const uint32_t mipLevel, uint32_t layer, uint32_t pageIndex)
{
	VirtualTexturePage& newPage = pTexture->pSvt->pPages[pageIndex];

	newPage.mVulkan.imageMemoryBind.offset = offset;
	newPage.mVulkan.imageMemoryBind.extent = extent;
	newPage.mVulkan.size = size;
	newPage.mipLevel = mipLevel;
	newPage.layer = layer;
	newPage.index = pageIndex;

	return &newPage;
}


static VTReadbackBufOffsets vtGetReadbackBufOffsets(uint32_t* buffer, uint32_t readbackBufSize, uint32_t pageCount, uint32_t currentImage)
{
	ASSERT(!!buffer == !!readbackBufSize);  // If you already know the readback buf size, why is buffer null?

	VTReadbackBufOffsets offsets;
	offsets.pAlivePageCount = buffer + ((readbackBufSize / sizeof(uint32_t)) * currentImage);
	offsets.pRemovePageCount = offsets.pAlivePageCount + 1;
	offsets.pAlivePages = offsets.pRemovePageCount + 1;
	offsets.pRemovePages = offsets.pAlivePages + pageCount;

	offsets.mTotalSize = (uint)((offsets.pRemovePages - offsets.pAlivePageCount) + pageCount) * sizeof(uint);
	return offsets;
}

static VkVTPendingPageDeletion vtGetPendingPageDeletion(VirtualTexture* pSvt, uint32_t currentImage)
{
	if (pSvt->mPendingDeletionCount <= currentImage)
	{
		// Grow arrays
		const uint32_t oldDeletionCount = pSvt->mPendingDeletionCount;
		pSvt->mPendingDeletionCount = currentImage + 1;
		pSvt->mVulkan.pPendingDeletedAllocations = (void**)tf_realloc(pSvt->mVulkan.pPendingDeletedAllocations,
			pSvt->mPendingDeletionCount * pSvt->mVirtualPageTotalCount * sizeof(pSvt->mVulkan.pPendingDeletedAllocations[0]));

		pSvt->pPendingDeletedAllocationsCount = (uint32_t*)tf_realloc(pSvt->pPendingDeletedAllocationsCount,
			pSvt->mPendingDeletionCount * sizeof(pSvt->pPendingDeletedAllocationsCount[0]));

		pSvt->pPendingDeletedBuffers = (Buffer**)tf_realloc(pSvt->pPendingDeletedBuffers,
			pSvt->mPendingDeletionCount * pSvt->mVirtualPageTotalCount * sizeof(pSvt->pPendingDeletedBuffers[0]));

		pSvt->pPendingDeletedBuffersCount = (uint32_t*)tf_realloc(pSvt->pPendingDeletedBuffersCount,
			pSvt->mPendingDeletionCount * sizeof(pSvt->pPendingDeletedBuffersCount[0]));

		// Zero the new counts
		for (uint32_t i = oldDeletionCount; i < pSvt->mPendingDeletionCount; i++)
		{
			pSvt->pPendingDeletedAllocationsCount[i] = 0;
			pSvt->pPendingDeletedBuffersCount[i] = 0;
		}
	}

	VkVTPendingPageDeletion pendingDeletion;
	pendingDeletion.pAllocations = (VmaAllocation*)&pSvt->mVulkan.pPendingDeletedAllocations[currentImage * pSvt->mVirtualPageTotalCount];
	pendingDeletion.pAllocationsCount = &pSvt->pPendingDeletedAllocationsCount[currentImage];
	pendingDeletion.pIntermediateBuffers = &pSvt->pPendingDeletedBuffers[currentImage * pSvt->mVirtualPageTotalCount];
	pendingDeletion.pIntermediateBuffersCount = &pSvt->pPendingDeletedBuffersCount[currentImage];
	return pendingDeletion;
}

static uint32_t vtGetReadbackBufSize(uint32_t pageCount, uint32_t imageCount)
{
	VTReadbackBufOffsets offsets = vtGetReadbackBufOffsets(NULL, 0, pageCount, 0);
	return offsets.mTotalSize;
}

void vk_waitQueueIdle(Queue* pQueue)
{
	vkQueueWaitIdle(pQueue->mVulkan.pVkQueue);
}

void vk_addBuffer(Renderer* pRenderer, const BufferDesc* pDesc, Buffer** ppBuffer)
{
	ASSERT(pRenderer);
	ASSERT(pDesc);
	ASSERT(pDesc->mSize > 0);
	ASSERT(VK_NULL_HANDLE != pRenderer->mVulkan.pVkDevice);
	ASSERT(pRenderer->mGpuMode != GPU_MODE_UNLINKED || pDesc->mNodeIndex == pRenderer->mUnlinkedRendererIndex);

	Buffer* pBuffer = (Buffer*)tf_calloc_memalign(1, alignof(Buffer), sizeof(Buffer));
	ASSERT(ppBuffer);

	uint64_t allocationSize = pDesc->mSize;
	// Align the buffer size to multiples of the dynamic uniform buffer minimum size
	if (pDesc->mDescriptors & DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	{
		uint64_t minAlignment = pRenderer->pActiveGpuSettings->mUniformBufferAlignment;
		allocationSize = round_up_64(allocationSize, minAlignment);
	}

	DECLARE_ZERO(VkBufferCreateInfo, add_info);
	add_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	add_info.pNext = NULL;
	add_info.flags = 0;
	add_info.size = allocationSize;
	add_info.usage = util_to_vk_buffer_usage(pDesc->mDescriptors, pDesc->mFormat != TinyImageFormat_UNDEFINED);
	add_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	add_info.queueFamilyIndexCount = 0;
	add_info.pQueueFamilyIndices = NULL;

	// Buffer can be used as dest in a transfer command (Uploading data to a storage buffer, Readback query data)
	if (pDesc->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY || pDesc->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_TO_CPU)
		add_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	const bool linkedMultiGpu = (pRenderer->mGpuMode == GPU_MODE_LINKED && (pDesc->pSharedNodeIndices || pDesc->mNodeIndex));

	VmaAllocationCreateInfo vma_mem_reqs = {};
	vma_mem_reqs.usage = (VmaMemoryUsage)pDesc->mMemoryUsage;
	if (pDesc->mFlags & BUFFER_CREATION_FLAG_OWN_MEMORY_BIT)
		vma_mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	if (pDesc->mFlags & BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT)
		vma_mem_reqs.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	if (linkedMultiGpu)
		vma_mem_reqs.flags |= VMA_ALLOCATION_CREATE_DONT_BIND_BIT;
	if (pDesc->mFlags & BUFFER_CREATION_FLAG_HOST_VISIBLE)
		vma_mem_reqs.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	if (pDesc->mFlags & BUFFER_CREATION_FLAG_HOST_COHERENT)
		vma_mem_reqs.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VmaAllocationInfo alloc_info = {};
	CHECK_VKRESULT(vmaCreateBuffer(
		pRenderer->mVulkan.pVmaAllocator, &add_info, &vma_mem_reqs, &pBuffer->mVulkan.pVkBuffer, &pBuffer->mVulkan.pVkAllocation,
		&alloc_info));

	pBuffer->pCpuMappedAddress = alloc_info.pMappedData;
	/************************************************************************/
	// Buffer to be used on multiple GPUs
	/************************************************************************/
	if (linkedMultiGpu)
	{
		VmaAllocationInfo allocInfo = {};
		vmaGetAllocationInfo(pRenderer->mVulkan.pVmaAllocator, pBuffer->mVulkan.pVkAllocation, &allocInfo);
		/************************************************************************/
		// Set all the device indices to the index of the device where we will create the buffer
		/************************************************************************/
		uint32_t* pIndices = (uint32_t*)alloca(pRenderer->mLinkedNodeCount * sizeof(uint32_t));
		util_calculate_device_indices(pRenderer, pDesc->mNodeIndex, pDesc->pSharedNodeIndices, pDesc->mSharedNodeIndexCount, pIndices);
		/************************************************************************/
		// #TODO : Move this to the Vulkan memory allocator
		/************************************************************************/
		VkBindBufferMemoryInfoKHR            bindInfo = { VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR };
		VkBindBufferMemoryDeviceGroupInfoKHR bindDeviceGroup = { VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO_KHR };
		bindDeviceGroup.deviceIndexCount = pRenderer->mLinkedNodeCount;
		bindDeviceGroup.pDeviceIndices = pIndices;
		bindInfo.buffer = pBuffer->mVulkan.pVkBuffer;
		bindInfo.memory = allocInfo.deviceMemory;
		bindInfo.memoryOffset = allocInfo.offset;
		bindInfo.pNext = &bindDeviceGroup;
		CHECK_VKRESULT(vkBindBufferMemory2KHR(pRenderer->mVulkan.pVkDevice, 1, &bindInfo));
		/************************************************************************/
		/************************************************************************/
	}
	/************************************************************************/
	// Set descriptor data
	/************************************************************************/
	if ((pDesc->mDescriptors & DESCRIPTOR_TYPE_UNIFORM_BUFFER) || (pDesc->mDescriptors & DESCRIPTOR_TYPE_BUFFER) ||
		(pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_BUFFER))
	{
		if ((pDesc->mDescriptors & DESCRIPTOR_TYPE_BUFFER) || (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_BUFFER))
		{
			pBuffer->mVulkan.mOffset = pDesc->mStructStride * pDesc->mFirstElement;
		}
	}

	if (add_info.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
	{
		VkBufferViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, NULL };
		viewInfo.buffer = pBuffer->mVulkan.pVkBuffer;
		viewInfo.flags = 0;
		viewInfo.format = (VkFormat)TinyImageFormat_ToVkFormat(pDesc->mFormat);
		viewInfo.offset = pDesc->mFirstElement * pDesc->mStructStride;
		viewInfo.range = pDesc->mElementCount * pDesc->mStructStride;
		VkFormatProperties formatProps = {};
		vkGetPhysicalDeviceFormatProperties(pRenderer->mVulkan.pVkActiveGPU, viewInfo.format, &formatProps);
		if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
		{
			LOGF(LogLevel::eWARNING, "Failed to create uniform texel buffer view for format %u", (uint32_t)pDesc->mFormat);
		}
		else
		{
			CHECK_VKRESULT(vkCreateBufferView(
				pRenderer->mVulkan.pVkDevice, &viewInfo, &gVkAllocationCallbacks, &pBuffer->mVulkan.pVkUniformTexelView));
		}
	}
	if (add_info.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
	{
		VkBufferViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, NULL };
		viewInfo.buffer = pBuffer->mVulkan.pVkBuffer;
		viewInfo.flags = 0;
		viewInfo.format = (VkFormat)TinyImageFormat_ToVkFormat(pDesc->mFormat);
		viewInfo.offset = pDesc->mFirstElement * pDesc->mStructStride;
		viewInfo.range = pDesc->mElementCount * pDesc->mStructStride;
		VkFormatProperties formatProps = {};
		vkGetPhysicalDeviceFormatProperties(pRenderer->mVulkan.pVkActiveGPU, viewInfo.format, &formatProps);
		if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
		{
			LOGF(LogLevel::eWARNING, "Failed to create storage texel buffer view for format %u", (uint32_t)pDesc->mFormat);
		}
		else
		{
			CHECK_VKRESULT(vkCreateBufferView(
				pRenderer->mVulkan.pVkDevice, &viewInfo, &gVkAllocationCallbacks, &pBuffer->mVulkan.pVkStorageTexelView));
		}
	}

#if defined(ENABLE_GRAPHICS_DEBUG)
	if (pDesc->pName)
	{
		setBufferName(pRenderer, pBuffer, pDesc->pName);
	}
#endif

	/************************************************************************/
	/************************************************************************/
	pBuffer->mSize = (uint32_t)pDesc->mSize;
	pBuffer->mMemoryUsage = pDesc->mMemoryUsage;
	pBuffer->mNodeIndex = pDesc->mNodeIndex;
	pBuffer->mDescriptors = pDesc->mDescriptors;

	*ppBuffer = pBuffer;
}

void vk_getFenceStatus(Renderer* pRenderer, Fence* pFence, FenceStatus* pFenceStatus)
{
	*pFenceStatus = FENCE_STATUS_COMPLETE;
	if (pFence->mVulkan.mSubmitted)
	{
		VkResult vkRes = vkGetFenceStatus(pRenderer->mVulkan.pVkDevice, pFence->mVulkan.pVkFence);
		if (vkRes == VK_SUCCESS)
		{
			vkResetFences(pRenderer->mVulkan.pVkDevice, 1, &pFence->mVulkan.pVkFence);
			pFence->mVulkan.mSubmitted = false;
		}

		*pFenceStatus = vkRes == VK_SUCCESS ? FENCE_STATUS_COMPLETE : FENCE_STATUS_INCOMPLETE;
	}
	else
	{
		*pFenceStatus = FENCE_STATUS_NOTSUBMITTED;
	}
}

void vk_waitForFences(Renderer* pRenderer, uint32_t fenceCount, Fence** ppFences)
{
	ASSERT(pRenderer);
	ASSERT(fenceCount);
	ASSERT(ppFences);

	VkFence* fences = (VkFence*)alloca(fenceCount * sizeof(VkFence));
	uint32_t numValidFences = 0;
	for (uint32_t i = 0; i < fenceCount; ++i)
	{
		if (ppFences[i]->mVulkan.mSubmitted)
			fences[numValidFences++] = ppFences[i]->mVulkan.pVkFence;
	}

	if (numValidFences)
	{
		CHECK_VKRESULT(vkWaitForFences(pRenderer->mVulkan.pVkDevice, numValidFences, fences, VK_TRUE, UINT64_MAX));
		CHECK_VKRESULT(vkResetFences(pRenderer->mVulkan.pVkDevice, numValidFences, fences));
	}

	for (uint32_t i = 0; i < fenceCount; ++i)
		ppFences[i]->mVulkan.mSubmitted = false;
}

void vk_removeBuffer(Renderer* pRenderer, Buffer* pBuffer)
{
	ASSERT(pRenderer);
	ASSERT(pBuffer);
	ASSERT(VK_NULL_HANDLE != pRenderer->mVulkan.pVkDevice);
	ASSERT(VK_NULL_HANDLE != pBuffer->mVulkan.pVkBuffer);
	if (pBuffer->mVulkan.pVkUniformTexelView)
	{
		vkDestroyBufferView(pRenderer->mVulkan.pVkDevice, pBuffer->mVulkan.pVkUniformTexelView, &gVkAllocationCallbacks);
		pBuffer->mVulkan.pVkUniformTexelView = VK_NULL_HANDLE;
	}
	if (pBuffer->mVulkan.pVkStorageTexelView)
	{
		vkDestroyBufferView(pRenderer->mVulkan.pVkDevice, pBuffer->mVulkan.pVkStorageTexelView, &gVkAllocationCallbacks);
		pBuffer->mVulkan.pVkStorageTexelView = VK_NULL_HANDLE;
	}

	vmaDestroyBuffer(pRenderer->mVulkan.pVmaAllocator, pBuffer->mVulkan.pVkBuffer, pBuffer->mVulkan.pVkAllocation);

	SAFE_FREE(pBuffer);
}

void vk_addTexture(Renderer* pRenderer, const TextureDesc* pDesc, Texture** ppTexture)
{
	ASSERT(pRenderer);
	ASSERT(pDesc && pDesc->mWidth && pDesc->mHeight && (pDesc->mDepth || pDesc->mArraySize));
	ASSERT(pRenderer->mGpuMode != GPU_MODE_UNLINKED || pDesc->mNodeIndex == pRenderer->mUnlinkedRendererIndex);
	if (pDesc->mSampleCount > SAMPLE_COUNT_1 && pDesc->mMipLevels > 1)
	{
		LOGF(LogLevel::eERROR, "Multi-Sampled textures cannot have mip maps");
		ASSERT(false);
		return;
	}

	size_t totalSize = sizeof(Texture);
	totalSize += (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE ? (pDesc->mMipLevels * sizeof(VkImageView)) : 0);
	Texture* pTexture = (Texture*)tf_calloc_memalign(1, alignof(Texture), totalSize);
	ASSERT(pTexture);

	if (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
		pTexture->mVulkan.pVkUAVDescriptors = (VkImageView*)(pTexture + 1);

	if (pDesc->pNativeHandle && !(pDesc->mFlags & TEXTURE_CREATION_FLAG_IMPORT_BIT))
	{
		pTexture->mOwnsImage = false;
		pTexture->mVulkan.pVkImage = (VkImage)pDesc->pNativeHandle;
	}
	else
	{
		pTexture->mOwnsImage = true;
	}

	VkImageUsageFlags additionalFlags = 0;
	if (pDesc->mStartState & RESOURCE_STATE_RENDER_TARGET)
		additionalFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	else if (pDesc->mStartState & RESOURCE_STATE_DEPTH_WRITE)
		additionalFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	uint arraySize = pDesc->mArraySize;
#if defined(QUEST_VR)
	if (additionalFlags == 0 && // If not a render target
		!!(pDesc->mFlags & TEXTURE_CREATION_FLAG_VR_MULTIVIEW))
	{
		// Double the array size
		arraySize *= 2;
	}
#endif

	VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;
	if (pDesc->mFlags & TEXTURE_CREATION_FLAG_FORCE_2D)
	{
		ASSERT(pDesc->mDepth == 1);
		image_type = VK_IMAGE_TYPE_2D;
	}
	else if (pDesc->mFlags & TEXTURE_CREATION_FLAG_FORCE_3D)
	{
		image_type = VK_IMAGE_TYPE_3D;
	}
	else
	{
		if (pDesc->mDepth > 1)
			image_type = VK_IMAGE_TYPE_3D;
		else if (pDesc->mHeight > 1)
			image_type = VK_IMAGE_TYPE_2D;
		else
			image_type = VK_IMAGE_TYPE_1D;
	}

	DescriptorType descriptors = pDesc->mDescriptors;
	bool           cubemapRequired = (DESCRIPTOR_TYPE_TEXTURE_CUBE == (descriptors & DESCRIPTOR_TYPE_TEXTURE_CUBE));
	bool           arrayRequired = false;

	const bool     isPlanarFormat = TinyImageFormat_IsPlanar(pDesc->mFormat);
	const uint32_t numOfPlanes = TinyImageFormat_NumOfPlanes(pDesc->mFormat);
	const bool     isSinglePlane = TinyImageFormat_IsSinglePlane(pDesc->mFormat);
	ASSERT(
		((isSinglePlane && numOfPlanes == 1) || (!isSinglePlane && numOfPlanes > 1 && numOfPlanes <= MAX_PLANE_COUNT)) &&
		"Number of planes for multi-planar formats must be 2 or 3 and for single-planar formats it must be 1.");

	if (image_type == VK_IMAGE_TYPE_3D)
		arrayRequired = true;

	if (VK_NULL_HANDLE == pTexture->mVulkan.pVkImage)
	{
		DECLARE_ZERO(VkImageCreateInfo, add_info);
		add_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		add_info.pNext = NULL;
		add_info.flags = 0;
		add_info.imageType = image_type;
		add_info.format = (VkFormat)TinyImageFormat_ToVkFormat(pDesc->mFormat);
		add_info.extent.width = pDesc->mWidth;
		add_info.extent.height = pDesc->mHeight;
		add_info.extent.depth = pDesc->mDepth;
		add_info.mipLevels = pDesc->mMipLevels;
		add_info.arrayLayers = arraySize;
		add_info.samples = util_to_vk_sample_count(pDesc->mSampleCount);
		add_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		add_info.usage = util_to_vk_image_usage(descriptors);
		add_info.usage |= additionalFlags;
		add_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		add_info.queueFamilyIndexCount = 0;
		add_info.pQueueFamilyIndices = NULL;
		add_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (cubemapRequired)
			add_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		if (arrayRequired)
			add_info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

		DECLARE_ZERO(VkFormatProperties, format_props);
		vkGetPhysicalDeviceFormatProperties(pRenderer->mVulkan.pVkActiveGPU, add_info.format, &format_props);
		if (isPlanarFormat)    // multi-planar formats must have each plane separately bound to memory, rather than having a single memory binding for the whole image
		{
			ASSERT(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DISJOINT_BIT);
			add_info.flags |= VK_IMAGE_CREATE_DISJOINT_BIT;
		}

		if ((VK_IMAGE_USAGE_SAMPLED_BIT & add_info.usage) || (VK_IMAGE_USAGE_STORAGE_BIT & add_info.usage))
		{
			// Make it easy to copy to and from textures
			add_info.usage |= (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		}

		ASSERT(pRenderer->pCapBits->canShaderReadFrom[pDesc->mFormat] && "GPU shader can't' read from this format");

		// Verify that GPU supports this format
		VkFormatFeatureFlags format_features = util_vk_image_usage_to_format_features(add_info.usage);

		VkFormatFeatureFlags flags = format_props.optimalTilingFeatures & format_features;
		ASSERT((0 != flags) && "Format is not supported for GPU local images (i.e. not host visible images)");

		const bool linkedMultiGpu = (pRenderer->mGpuMode == GPU_MODE_LINKED) && (pDesc->pSharedNodeIndices || pDesc->mNodeIndex);

		VmaAllocationCreateInfo mem_reqs = { 0 };
		if (pDesc->mFlags & TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT)
			mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		if (linkedMultiGpu)
			mem_reqs.flags |= VMA_ALLOCATION_CREATE_DONT_BIND_BIT;
		mem_reqs.usage = (VmaMemoryUsage)VMA_MEMORY_USAGE_GPU_ONLY;

		VkExternalMemoryImageCreateInfoKHR externalInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR, NULL };

#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VkImportMemoryWin32HandleInfoKHR importInfo = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, NULL };
#endif
		VkExportMemoryAllocateInfoKHR exportMemoryInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR, NULL };

		if (pRenderer->mVulkan.mExternalMemoryExtension && pDesc->mFlags & TEXTURE_CREATION_FLAG_IMPORT_BIT)
		{
			add_info.pNext = &externalInfo;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
			struct ImportHandleInfo
			{
				void* pHandle;
				VkExternalMemoryHandleTypeFlagBitsKHR mHandleType;
			};

			ImportHandleInfo* pHandleInfo = (ImportHandleInfo*)pDesc->pNativeHandle;
			importInfo.handle = pHandleInfo->pHandle;
			importInfo.handleType = pHandleInfo->mHandleType;

			externalInfo.handleTypes = pHandleInfo->mHandleType;

			mem_reqs.pUserData = &importInfo;
			// Allocate external (importable / exportable) memory as dedicated memory to avoid adding unnecessary complexity to the Vulkan Memory Allocator
			mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
#endif
		}
		else if (pRenderer->mVulkan.mExternalMemoryExtension && pDesc->mFlags & TEXTURE_CREATION_FLAG_EXPORT_BIT)
		{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
			exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#endif

			mem_reqs.pUserData = &exportMemoryInfo;
			// Allocate external (importable / exportable) memory as dedicated memory to avoid adding unnecessary complexity to the Vulkan Memory Allocator
			mem_reqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}


		// If lazy allocation is requested, check that the hardware supports it
		bool lazyAllocation = pDesc->mFlags & TEXTURE_CREATION_FLAG_ON_TILE;
		if (lazyAllocation)
		{
			uint32_t memoryTypeIndex = 0;
			VmaAllocationCreateInfo lazyMemReqs = mem_reqs;
			lazyMemReqs.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
			VkResult result = vmaFindMemoryTypeIndex(pRenderer->mVulkan.pVmaAllocator, UINT32_MAX, &lazyMemReqs, &memoryTypeIndex);
			if (VK_SUCCESS == result)
			{
				mem_reqs = lazyMemReqs;
				add_info.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
				// The Vulkan spec states: If usage includes VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				// then bits other than VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				// and VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT must not be set
				add_info.usage &= (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
				pTexture->mLazilyAllocated = true;
			}
		}

		VmaAllocationInfo alloc_info = {};
		if (isSinglePlane)
		{
			CHECK_VKRESULT(vmaCreateImage(
				pRenderer->mVulkan.pVmaAllocator, &add_info, &mem_reqs, &pTexture->mVulkan.pVkImage, &pTexture->mVulkan.pVkAllocation,
				&alloc_info));
		}
		else    // Multi-planar formats
		{
			// Create info requires the mutable format flag set for multi planar images
			// Also pass the format list for mutable formats as per recommendation from the spec
			// Might help to keep DCC enabled if we ever use this as a output format
			// DCC gets disabled when we pass mutable format bit to the create info. Passing the format list helps the driver to enable it
			VkFormat                       planarFormat = (VkFormat)TinyImageFormat_ToVkFormat(pDesc->mFormat);
			VkImageFormatListCreateInfoKHR formatList = {};
			formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
			formatList.pNext = NULL;
			formatList.pViewFormats = &planarFormat;
			formatList.viewFormatCount = 1;

			add_info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
			add_info.pNext = &formatList;    //-V506

			// Create Image
			CHECK_VKRESULT(vkCreateImage(pRenderer->mVulkan.pVkDevice, &add_info, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkImage));

			VkMemoryRequirements vkMemReq = {};
			uint64_t             planesOffsets[MAX_PLANE_COUNT] = { 0 };
			util_get_planar_vk_image_memory_requirement(
				pRenderer->mVulkan.pVkDevice, pTexture->mVulkan.pVkImage, numOfPlanes, &vkMemReq, planesOffsets);

			// Allocate image memory
			VkMemoryAllocateInfo mem_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			mem_alloc_info.allocationSize = vkMemReq.size;
			VkPhysicalDeviceMemoryProperties memProps = {};
			vkGetPhysicalDeviceMemoryProperties(pRenderer->mVulkan.pVkActiveGPU, &memProps);
			mem_alloc_info.memoryTypeIndex = util_get_memory_type(vkMemReq.memoryTypeBits, memProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			CHECK_VKRESULT(vkAllocateMemory(
				pRenderer->mVulkan.pVkDevice, &mem_alloc_info, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkDeviceMemory));

			// Bind planes to their memories
			VkBindImageMemoryInfo      bindImagesMemoryInfo[MAX_PLANE_COUNT];
			VkBindImagePlaneMemoryInfo bindImagePlanesMemoryInfo[MAX_PLANE_COUNT];
			for (uint32_t i = 0; i < numOfPlanes; ++i)
			{
				VkBindImagePlaneMemoryInfo& bindImagePlaneMemoryInfo = bindImagePlanesMemoryInfo[i];
				bindImagePlaneMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
				bindImagePlaneMemoryInfo.pNext = NULL;
				bindImagePlaneMemoryInfo.planeAspect = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i);

				VkBindImageMemoryInfo& bindImageMemoryInfo = bindImagesMemoryInfo[i];
				bindImageMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
				bindImageMemoryInfo.pNext = &bindImagePlaneMemoryInfo;
				bindImageMemoryInfo.image = pTexture->mVulkan.pVkImage;
				bindImageMemoryInfo.memory = pTexture->mVulkan.pVkDeviceMemory;
				bindImageMemoryInfo.memoryOffset = planesOffsets[i];
			}

			CHECK_VKRESULT(vkBindImageMemory2(pRenderer->mVulkan.pVkDevice, numOfPlanes, bindImagesMemoryInfo));
		}

		/************************************************************************/
		// Texture to be used on multiple GPUs
		/************************************************************************/
		if (linkedMultiGpu)
		{
			VmaAllocationInfo allocInfo = {};
			vmaGetAllocationInfo(pRenderer->mVulkan.pVmaAllocator, pTexture->mVulkan.pVkAllocation, &allocInfo);
			/************************************************************************/
			// Set all the device indices to the index of the device where we will create the texture
			/************************************************************************/
			uint32_t* pIndices = (uint32_t*)alloca(pRenderer->mLinkedNodeCount * sizeof(uint32_t));
			util_calculate_device_indices(pRenderer, pDesc->mNodeIndex, pDesc->pSharedNodeIndices, pDesc->mSharedNodeIndexCount, pIndices);
			/************************************************************************/
			// #TODO : Move this to the Vulkan memory allocator
			/************************************************************************/
			VkBindImageMemoryInfoKHR            bindInfo = { VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR };
			VkBindImageMemoryDeviceGroupInfoKHR bindDeviceGroup = { VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO_KHR };
			bindDeviceGroup.deviceIndexCount = pRenderer->mLinkedNodeCount;
			bindDeviceGroup.pDeviceIndices = pIndices;
			bindInfo.image = pTexture->mVulkan.pVkImage;
			bindInfo.memory = allocInfo.deviceMemory;
			bindInfo.memoryOffset = allocInfo.offset;
			bindInfo.pNext = &bindDeviceGroup;
			CHECK_VKRESULT(vkBindImageMemory2KHR(pRenderer->mVulkan.pVkDevice, 1, &bindInfo));
			/************************************************************************/
			/************************************************************************/
		}
	}
	/************************************************************************/
	// Create image view
	/************************************************************************/
	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	switch (image_type)
	{
	case VK_IMAGE_TYPE_1D: view_type = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D; break;
	case VK_IMAGE_TYPE_2D:
		if (cubemapRequired)
			view_type = (arraySize > 6) ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
		else
			view_type = arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		break;
	case VK_IMAGE_TYPE_3D:
		if (arraySize > 1)
		{
			LOGF(LogLevel::eERROR, "Cannot support 3D Texture Array in Vulkan");
			ASSERT(false);
		}
		view_type = VK_IMAGE_VIEW_TYPE_3D;
		break;
	default: ASSERT(false && "Image Format not supported!"); break;
	}

	ASSERT(view_type != VK_IMAGE_VIEW_TYPE_MAX_ENUM && "Invalid Image View");

	VkImageViewCreateInfo srvDesc = {};
	// SRV
	srvDesc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	srvDesc.pNext = NULL;
	srvDesc.flags = 0;
	srvDesc.image = pTexture->mVulkan.pVkImage;
	srvDesc.viewType = view_type;
	srvDesc.format = (VkFormat)TinyImageFormat_ToVkFormat(pDesc->mFormat);
	srvDesc.components.r = VK_COMPONENT_SWIZZLE_R;
	srvDesc.components.g = VK_COMPONENT_SWIZZLE_G;
	srvDesc.components.b = VK_COMPONENT_SWIZZLE_B;
	srvDesc.components.a = VK_COMPONENT_SWIZZLE_A;
	srvDesc.subresourceRange.aspectMask = util_vk_determine_aspect_mask(srvDesc.format, false);
	srvDesc.subresourceRange.baseMipLevel = 0;
	srvDesc.subresourceRange.levelCount = pDesc->mMipLevels;
	srvDesc.subresourceRange.baseArrayLayer = 0;
	srvDesc.subresourceRange.layerCount = arraySize;
	pTexture->mAspectMask = util_vk_determine_aspect_mask(srvDesc.format, true);

	if (pDesc->pVkSamplerYcbcrConversionInfo)
	{
		srvDesc.pNext = pDesc->pVkSamplerYcbcrConversionInfo;
	}

	if (descriptors & DESCRIPTOR_TYPE_TEXTURE)
	{
		CHECK_VKRESULT(
			vkCreateImageView(pRenderer->mVulkan.pVkDevice, &srvDesc, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkSRVDescriptor));
	}

	// SRV stencil
	if ((TinyImageFormat_HasStencil(pDesc->mFormat)) && (descriptors & DESCRIPTOR_TYPE_TEXTURE))
	{
		srvDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		CHECK_VKRESULT(
			vkCreateImageView(pRenderer->mVulkan.pVkDevice, &srvDesc, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkSRVStencilDescriptor));
	}

	// UAV
	if (descriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
	{
		VkImageViewCreateInfo uavDesc = srvDesc;
		// #NOTE : We dont support imageCube, imageCubeArray for consistency with other APIs
		// All cubemaps will be used as image2DArray for Image Load / Store ops
		if (uavDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY || uavDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
			uavDesc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		uavDesc.subresourceRange.levelCount = 1;
		for (uint32_t i = 0; i < pDesc->mMipLevels; ++i)
		{
			uavDesc.subresourceRange.baseMipLevel = i;
			CHECK_VKRESULT(vkCreateImageView(
				pRenderer->mVulkan.pVkDevice, &uavDesc, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkUAVDescriptors[i]));
		}
	}
	/************************************************************************/
	/************************************************************************/
	pTexture->mNodeIndex = pDesc->mNodeIndex;
	pTexture->mWidth = pDesc->mWidth;
	pTexture->mHeight = pDesc->mHeight;
	pTexture->mDepth = pDesc->mDepth;
	pTexture->mMipLevels = pDesc->mMipLevels;
	pTexture->mUav = pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE;
	pTexture->mArraySizeMinusOne = arraySize - 1;
	pTexture->mFormat = pDesc->mFormat;

#if defined(ENABLE_GRAPHICS_DEBUG)
	if (pDesc->pName)
	{
		setTextureName(pRenderer, pTexture, pDesc->pName);
	}
#endif

	* ppTexture = pTexture;
}

void vk_addVirtualTexture(Cmd* pCmd, const TextureDesc* pDesc, Texture** ppTexture, void* pImageData)
{
	ASSERT(pCmd);
	Texture* pTexture = (Texture*)tf_calloc_memalign(1, alignof(Texture), sizeof(*pTexture) + sizeof(VirtualTexture));
	ASSERT(pTexture);

	Renderer* pRenderer = pCmd->pRenderer;

	pTexture->pSvt = (VirtualTexture*)(pTexture + 1);

	uint32_t imageSize = 0;
	uint32_t mipSize = pDesc->mWidth * pDesc->mHeight * pDesc->mDepth;
	while (mipSize > 0)
	{
		imageSize += mipSize;
		mipSize /= 4;
	}

	pTexture->pSvt->pVirtualImageData = pImageData;
	pTexture->mFormat = pDesc->mFormat;
	ASSERT(pTexture->mFormat == pDesc->mFormat);

	VkFormat format = (VkFormat)TinyImageFormat_ToVkFormat((TinyImageFormat)pTexture->mFormat);
	ASSERT(VK_FORMAT_UNDEFINED != format);
	pTexture->mOwnsImage = true;

	VkImageCreateInfo add_info = {};
	add_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	add_info.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
	add_info.imageType = VK_IMAGE_TYPE_2D;
	add_info.format = format;
	add_info.extent.width = pDesc->mWidth;
	add_info.extent.height = pDesc->mHeight;
	add_info.extent.depth = pDesc->mDepth;
	add_info.mipLevels = pDesc->mMipLevels;
	add_info.arrayLayers = 1;
	add_info.samples = VK_SAMPLE_COUNT_1_BIT;
	add_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	add_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	add_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	add_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	CHECK_VKRESULT(vkCreateImage(pRenderer->mVulkan.pVkDevice, &add_info, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkImage));

	// Get memory requirements
	VkMemoryRequirements sparseImageMemoryReqs;
	// Sparse image memory requirement counts
	vkGetImageMemoryRequirements(pRenderer->mVulkan.pVkDevice, pTexture->mVulkan.pVkImage, &sparseImageMemoryReqs);

	// Check requested image size against hardware sparse limit
	if (sparseImageMemoryReqs.size > pRenderer->mVulkan.pVkActiveGPUProperties->properties.limits.sparseAddressSpaceSize)
	{
		LOGF(LogLevel::eERROR, "Requested sparse image size exceeds supported sparse address space size!");
		return;
	}

	// Get sparse memory requirements
	// Count
	uint32_t sparseMemoryReqsCount;
	vkGetImageSparseMemoryRequirements(pRenderer->mVulkan.pVkDevice, pTexture->mVulkan.pVkImage, &sparseMemoryReqsCount, NULL);  // Get count
	VkSparseImageMemoryRequirements* sparseMemoryReqs = NULL;

	if (sparseMemoryReqsCount == 0)
	{
		LOGF(LogLevel::eERROR, "No memory requirements for the sparse image!");
		return;
	}
	else
	{
		sparseMemoryReqs = (VkSparseImageMemoryRequirements*)tf_calloc(sparseMemoryReqsCount, sizeof(VkSparseImageMemoryRequirements));
		vkGetImageSparseMemoryRequirements(pRenderer->mVulkan.pVkDevice, pTexture->mVulkan.pVkImage, &sparseMemoryReqsCount, sparseMemoryReqs);  // Get reqs
	}

	ASSERT(sparseMemoryReqsCount == 1 && "Multiple sparse image memory requirements not currently implemented");

	pTexture->pSvt->mSparseVirtualTexturePageWidth = sparseMemoryReqs[0].formatProperties.imageGranularity.width;
	pTexture->pSvt->mSparseVirtualTexturePageHeight = sparseMemoryReqs[0].formatProperties.imageGranularity.height;
	pTexture->pSvt->mVirtualPageTotalCount = imageSize / (uint32_t)(pTexture->pSvt->mSparseVirtualTexturePageWidth * pTexture->pSvt->mSparseVirtualTexturePageHeight);
	pTexture->pSvt->mReadbackBufferSize = vtGetReadbackBufSize(pTexture->pSvt->mVirtualPageTotalCount, 1);
	pTexture->pSvt->mPageVisibilityBufferSize = pTexture->pSvt->mVirtualPageTotalCount * 2 * sizeof(uint);

	uint32_t TiledMiplevel = pDesc->mMipLevels - (uint32_t)log2(min((uint32_t)pTexture->pSvt->mSparseVirtualTexturePageWidth, (uint32_t)pTexture->pSvt->mSparseVirtualTexturePageHeight));
	pTexture->pSvt->mTiledMipLevelCount = (uint8_t)TiledMiplevel;

	LOGF(LogLevel::eINFO, "Sparse image memory requirements: %d", sparseMemoryReqsCount);

	// Get sparse image requirements for the color aspect
	VkSparseImageMemoryRequirements sparseMemoryReq = {};
	bool colorAspectFound = false;
	for (int i = 0; i < (int)sparseMemoryReqsCount; ++i)
	{
		VkSparseImageMemoryRequirements reqs = sparseMemoryReqs[i];

		if (reqs.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			sparseMemoryReq = reqs;
			colorAspectFound = true;
			break;
		}
	}

	SAFE_FREE(sparseMemoryReqs);
	sparseMemoryReqs = NULL;

	if (!colorAspectFound)
	{
		LOGF(LogLevel::eERROR, "Could not find sparse image memory requirements for color aspect bit!");
		return;
	}

	VkPhysicalDeviceMemoryProperties memProps = {};
	vkGetPhysicalDeviceMemoryProperties(pRenderer->mVulkan.pVkActiveGPU, &memProps);

	// todo:
	// Calculate number of required sparse memory bindings by alignment
	assert((sparseImageMemoryReqs.size % sparseImageMemoryReqs.alignment) == 0);
	pTexture->pSvt->mVulkan.mSparseMemoryTypeBits = sparseImageMemoryReqs.memoryTypeBits;

	// Check if the format has a single mip tail for all layers or one mip tail for each layer
	// The mip tail contains all mip levels > sparseMemoryReq.imageMipTailFirstLod
	bool singleMipTail = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

	pTexture->pSvt->pPages = (VirtualTexturePage*)tf_calloc(pTexture->pSvt->mVirtualPageTotalCount, sizeof(VirtualTexturePage));
	pTexture->pSvt->mVulkan.pSparseImageMemoryBinds = (VkSparseImageMemoryBind*)tf_calloc(pTexture->pSvt->mVirtualPageTotalCount, sizeof(VkSparseImageMemoryBind));

	pTexture->pSvt->mVulkan.pOpaqueMemoryBindAllocations = (void**)tf_calloc(pTexture->pSvt->mVirtualPageTotalCount, sizeof(VmaAllocation));
	pTexture->pSvt->mVulkan.pOpaqueMemoryBinds = (VkSparseMemoryBind*)tf_calloc(pTexture->pSvt->mVirtualPageTotalCount, sizeof(VkSparseMemoryBind));

	VmaPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.memoryTypeIndex = util_get_memory_type(sparseImageMemoryReqs.memoryTypeBits, memProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CHECK_VKRESULT(vmaCreatePool(pRenderer->mVulkan.pVmaAllocator, &poolCreateInfo, (VmaPool*)&pTexture->pSvt->mVulkan.pPool));

	uint32_t currentPageIndex = 0;

	// Sparse bindings for each mip level of all layers outside of the mip tail
	for (uint32_t layer = 0; layer < 1; layer++)
	{
		// sparseMemoryReq.imageMipTailFirstLod is the first mip level that's stored inside the mip tail
		for (uint32_t mipLevel = 0; mipLevel < TiledMiplevel; mipLevel++)
		{
			VkExtent3D extent;
			extent.width = max(add_info.extent.width >> mipLevel, 1u);
			extent.height = max(add_info.extent.height >> mipLevel, 1u);
			extent.depth = max(add_info.extent.depth >> mipLevel, 1u);

			VkImageSubresource subResource{};
			subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subResource.mipLevel = mipLevel;
			subResource.arrayLayer = layer;

			// Aligned sizes by image granularity
			VkExtent3D imageGranularity = sparseMemoryReq.formatProperties.imageGranularity;
			VkExtent3D sparseBindCounts = {};
			VkExtent3D lastBlockExtent = {};
			alignedDivision(extent, imageGranularity, &sparseBindCounts);
			lastBlockExtent.width =
				((extent.width % imageGranularity.width) ? extent.width % imageGranularity.width : imageGranularity.width);
			lastBlockExtent.height =
				((extent.height % imageGranularity.height) ? extent.height % imageGranularity.height : imageGranularity.height);
			lastBlockExtent.depth =
				((extent.depth % imageGranularity.depth) ? extent.depth % imageGranularity.depth : imageGranularity.depth);

			// Allocate memory for some blocks
			uint32_t index = 0;
			for (uint32_t z = 0; z < sparseBindCounts.depth; z++)
			{
				for (uint32_t y = 0; y < sparseBindCounts.height; y++)
				{
					for (uint32_t x = 0; x < sparseBindCounts.width; x++)
					{
						// Offset
						VkOffset3D offset;
						offset.x = x * imageGranularity.width;
						offset.y = y * imageGranularity.height;
						offset.z = z * imageGranularity.depth;
						// Size of the page
						VkExtent3D extent;
						extent.width = (x == sparseBindCounts.width - 1) ? lastBlockExtent.width : imageGranularity.width;
						extent.height = (y == sparseBindCounts.height - 1) ? lastBlockExtent.height : imageGranularity.height;
						extent.depth = (z == sparseBindCounts.depth - 1) ? lastBlockExtent.depth : imageGranularity.depth;

						// Add new virtual page
						VirtualTexturePage* newPage = addPage(pRenderer, pTexture, offset, extent, pTexture->pSvt->mSparseVirtualTexturePageWidth * pTexture->pSvt->mSparseVirtualTexturePageHeight * sizeof(uint), mipLevel, layer, currentPageIndex);
						currentPageIndex++;
						newPage->mVulkan.imageMemoryBind.subresource = subResource;

						index++;
					}
				}
			}
		}

		// Check if format has one mip tail per layer
		if ((!singleMipTail) && (sparseMemoryReq.imageMipTailFirstLod < pDesc->mMipLevels))
		{
			// Allocate memory for the mip tail
			VkMemoryRequirements memReqs = {};
			memReqs.size = sparseMemoryReq.imageMipTailSize;
			memReqs.memoryTypeBits = pTexture->pSvt->mVulkan.mSparseMemoryTypeBits;
			memReqs.alignment = memReqs.size;

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.memoryTypeBits = memReqs.memoryTypeBits;
			allocCreateInfo.pool = (VmaPool)pTexture->pSvt->mVulkan.pPool;

			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;
			CHECK_VKRESULT(vmaAllocateMemory(pRenderer->mVulkan.pVmaAllocator, &memReqs, &allocCreateInfo, &allocation, &allocationInfo));

			// (Opaque) sparse memory binding
			VkSparseMemoryBind sparseMemoryBind{};
			sparseMemoryBind.resourceOffset = sparseMemoryReq.imageMipTailOffset + layer * sparseMemoryReq.imageMipTailStride;
			sparseMemoryBind.size = sparseMemoryReq.imageMipTailSize;
			sparseMemoryBind.memory = allocation->GetMemory();
			sparseMemoryBind.memoryOffset = allocation->GetOffset();

			pTexture->pSvt->mVulkan.pOpaqueMemoryBindAllocations[pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount] = allocation;
			pTexture->pSvt->mVulkan.pOpaqueMemoryBinds[pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount] = sparseMemoryBind;
			pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount++;
		}
	}    // end layers and mips

	LOGF(LogLevel::eINFO, "Virtual Texture info: Dim %d x %d Pages %d", pDesc->mWidth, pDesc->mHeight, pTexture->pSvt->mVirtualPageTotalCount);

	// Check if format has one mip tail for all layers
	if ((sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) &&
		(sparseMemoryReq.imageMipTailFirstLod < pDesc->mMipLevels))
	{
		// Allocate memory for the mip tail
		VkMemoryRequirements memReqs = {};
		memReqs.size = sparseMemoryReq.imageMipTailSize;
		memReqs.memoryTypeBits = pTexture->pSvt->mVulkan.mSparseMemoryTypeBits;
		memReqs.alignment = memReqs.size;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.memoryTypeBits = memReqs.memoryTypeBits;
		allocCreateInfo.pool = (VmaPool)pTexture->pSvt->mVulkan.pPool;

		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
		CHECK_VKRESULT(vmaAllocateMemory(pRenderer->mVulkan.pVmaAllocator, &memReqs, &allocCreateInfo, &allocation, &allocationInfo));

		// (Opaque) sparse memory binding
		VkSparseMemoryBind sparseMemoryBind{};
		sparseMemoryBind.resourceOffset = sparseMemoryReq.imageMipTailOffset;
		sparseMemoryBind.size = sparseMemoryReq.imageMipTailSize;
		sparseMemoryBind.memory = allocation->GetMemory();
		sparseMemoryBind.memoryOffset = allocation->GetOffset();

		pTexture->pSvt->mVulkan.pOpaqueMemoryBindAllocations[pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount] = allocation;
		pTexture->pSvt->mVulkan.pOpaqueMemoryBinds[pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount] = sparseMemoryBind;
		pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount++;
	}

	/************************************************************************/
	// Create image view
	/************************************************************************/
	VkImageViewCreateInfo view = {};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = format;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = 1;
	view.subresourceRange.levelCount = pDesc->mMipLevels;
	view.image = pTexture->mVulkan.pVkImage;
	pTexture->mAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	CHECK_VKRESULT(vkCreateImageView(pRenderer->mVulkan.pVkDevice, &view, &gVkAllocationCallbacks, &pTexture->mVulkan.pVkSRVDescriptor));

	TextureBarrier textureBarriers[] = { { pTexture, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_COPY_DEST } };
	vk_cmdResourceBarrier(pCmd, 0, NULL, 1, textureBarriers, 0, NULL);

	// Fill smallest (non-tail) mip map level
	vk_fillVirtualTextureLevel(pCmd, pTexture, TiledMiplevel - 1, 0);

	pTexture->mOwnsImage = true;
	pTexture->mNodeIndex = pDesc->mNodeIndex;
	pTexture->mMipLevels = pDesc->mMipLevels;
	pTexture->mWidth = pDesc->mWidth;
	pTexture->mHeight = pDesc->mHeight;
	pTexture->mDepth = pDesc->mDepth;

#if defined(ENABLE_GRAPHICS_DEBUG)
	if (pDesc->pName)
	{
		setTextureName(pRenderer, pTexture, pDesc->pName);
	}
#endif

	* ppTexture = pTexture;
}

void vk_fillVirtualTextureLevel(Cmd* pCmd, Texture* pTexture, uint32_t mipLevel, uint32_t currentImage)
{
	//Bind data
	uint32_t imageMemoryCount = 0;

	for (uint32_t i = 0; i < pTexture->pSvt->mVirtualPageTotalCount; i++)
	{
		VirtualTexturePage* pPage = &pTexture->pSvt->pPages[i];
		ASSERT(pPage->index == i);

		if (pPage->mipLevel == mipLevel)
		{
			vk_uploadVirtualTexturePage(pCmd, pTexture, pPage, &imageMemoryCount, currentImage);
		}
	}

	vk_updateVirtualTextureMemory(pCmd, pTexture, imageMemoryCount);
}

void vk_updateVirtualTextureMemory(Cmd* pCmd, Texture* pTexture, uint32_t imageMemoryCount)
{
	// Update sparse bind info
	if (imageMemoryCount > 0)
	{
		VkBindSparseInfo bindSparseInfo = {};
		bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;

		// Image memory binds
		VkSparseImageMemoryBindInfo imageMemoryBindInfo = {};
		imageMemoryBindInfo.image = pTexture->mVulkan.pVkImage;
		imageMemoryBindInfo.bindCount = imageMemoryCount;
		imageMemoryBindInfo.pBinds = pTexture->pSvt->mVulkan.pSparseImageMemoryBinds;
		bindSparseInfo.imageBindCount = 1;
		bindSparseInfo.pImageBinds = &imageMemoryBindInfo;

		// Opaque image memory binds (mip tail)
		VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo = {};
		opaqueMemoryBindInfo.image = pTexture->mVulkan.pVkImage;
		opaqueMemoryBindInfo.bindCount = pTexture->pSvt->mVulkan.mOpaqueMemoryBindsCount;
		opaqueMemoryBindInfo.pBinds = pTexture->pSvt->mVulkan.pOpaqueMemoryBinds;
		bindSparseInfo.imageOpaqueBindCount = (opaqueMemoryBindInfo.bindCount > 0) ? 1 : 0;
		bindSparseInfo.pImageOpaqueBinds = &opaqueMemoryBindInfo;

		CHECK_VKRESULT(vkQueueBindSparse(pCmd->pQueue->mVulkan.pVkQueue, (uint32_t)1, &bindSparseInfo, VK_NULL_HANDLE));
	}
}

void vk_uploadVirtualTexturePage(Cmd* pCmd, Texture* pTexture, VirtualTexturePage* pPage, uint32_t* imageMemoryCount, uint32_t currentImage)
{
	Buffer* pIntermediateBuffer = NULL;
	if (allocateVirtualPage(pCmd->pRenderer, pTexture, *pPage, &pIntermediateBuffer))
	{
		void* pData = (void*)((unsigned char*)pTexture->pSvt->pVirtualImageData + (pPage->index * pPage->mVulkan.size));

		const bool intermediateMap = !pIntermediateBuffer->pCpuMappedAddress;
		if (intermediateMap)
		{
			vk_mapBuffer(pCmd->pRenderer, pIntermediateBuffer, NULL);
		}

		//CPU to GPU
		memcpy(pIntermediateBuffer->pCpuMappedAddress, pData, pPage->mVulkan.size);

		if (intermediateMap)
		{
			vk_unmapBuffer(pCmd->pRenderer, pIntermediateBuffer);
		}

		//Copy image to VkImage
		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.mipLevel = pPage->mipLevel;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = pPage->mVulkan.imageMemoryBind.offset;
		region.imageOffset.z = 0;
		region.imageExtent = { (uint32_t)pTexture->pSvt->mSparseVirtualTexturePageWidth, (uint32_t)pTexture->pSvt->mSparseVirtualTexturePageHeight, 1 };

		vkCmdCopyBufferToImage(
			pCmd->mVulkan.pVkCmdBuf,
			pIntermediateBuffer->mVulkan.pVkBuffer,
			pTexture->mVulkan.pVkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);

		// Update list of memory-backed sparse image memory binds
		pTexture->pSvt->mVulkan.pSparseImageMemoryBinds[(*imageMemoryCount)++] = pPage->mVulkan.imageMemoryBind;

		// Schedule deletion of this intermediate buffer
		const VkVTPendingPageDeletion pendingDeletion = vtGetPendingPageDeletion(pTexture->pSvt, currentImage);
		pendingDeletion.pIntermediateBuffers[(*pendingDeletion.pIntermediateBuffersCount)++] = pIntermediateBuffer;
	}
}


void vk_resetCmdPool(Renderer* pRenderer, CmdPool* pCmdPool)
{
	ASSERT(pRenderer);
	ASSERT(pCmdPool);

	CHECK_VKRESULT(vkResetCommandPool(pRenderer->mVulkan.pVkDevice, pCmdPool->pVkCmdPool, 0));
}

void vk_beginCmd(Cmd* pCmd)
{
	ASSERT(pCmd);
	ASSERT(VK_NULL_HANDLE != pCmd->mVulkan.pVkCmdBuf);

	DECLARE_ZERO(VkCommandBufferBeginInfo, begin_info);
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	VkDeviceGroupCommandBufferBeginInfoKHR deviceGroupBeginInfo = { VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO_KHR };
	deviceGroupBeginInfo.pNext = NULL;
	if (pCmd->pRenderer->mGpuMode == GPU_MODE_LINKED)
	{
		deviceGroupBeginInfo.deviceMask = (1 << pCmd->mVulkan.mNodeIndex);
		begin_info.pNext = &deviceGroupBeginInfo;
	}

	CHECK_VKRESULT(vkBeginCommandBuffer(pCmd->mVulkan.pVkCmdBuf, &begin_info));

	// Reset CPU side data
	pCmd->mVulkan.pBoundPipelineLayout = NULL;
}

void vk_cmdUpdateBuffer(Cmd* pCmd, Buffer* pBuffer, uint64_t dstOffset, Buffer* pSrcBuffer, uint64_t srcOffset, uint64_t size)
{
	ASSERT(pCmd);
	ASSERT(pSrcBuffer);
	ASSERT(pSrcBuffer->mVulkan.pVkBuffer);
	ASSERT(pBuffer);
	ASSERT(pBuffer->mVulkan.pVkBuffer);
	ASSERT(srcOffset + size <= pSrcBuffer->mSize);
	ASSERT(dstOffset + size <= pBuffer->mSize);

	DECLARE_ZERO(VkBufferCopy, region);
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;
	region.size = (VkDeviceSize)size;
	vkCmdCopyBuffer(pCmd->mVulkan.pVkCmdBuf, pSrcBuffer->mVulkan.pVkBuffer, pBuffer->mVulkan.pVkBuffer, 1, &region);
}

void vk_cmdResourceBarrier(
	Cmd* pCmd, uint32_t numBufferBarriers, BufferBarrier* pBufferBarriers, uint32_t numTextureBarriers, TextureBarrier* pTextureBarriers,
	uint32_t numRtBarriers, RenderTargetBarrier* pRtBarriers)
{
	VkImageMemoryBarrier* imageBarriers =
		(numTextureBarriers + numRtBarriers)
		? (VkImageMemoryBarrier*)alloca((numTextureBarriers + numRtBarriers) * sizeof(VkImageMemoryBarrier))
		: NULL;
	uint32_t imageBarrierCount = 0;
	VkBufferMemoryBarrier* bufferBarriers =
		numBufferBarriers
		? (VkBufferMemoryBarrier*)alloca(numBufferBarriers * sizeof(VkBufferMemoryBarrier))
		: NULL;
	uint32_t bufferBarrierCount = 0;
	VkAccessFlags srcAccessFlags = 0;
	VkAccessFlags dstAccessFlags = 0;

	for (uint32_t i = 0; i < numBufferBarriers; ++i)
	{
		BufferBarrier* pTrans = &pBufferBarriers[i];
		Buffer* pBuffer = pTrans->pBuffer;
		VkBufferMemoryBarrier* pBufferBarrier = NULL;

		if (RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mCurrentState && RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mNewState)
		{
			pBufferBarrier = &bufferBarriers[bufferBarrierCount++];             //-V522
			pBufferBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;    //-V522
			pBufferBarrier->pNext = NULL;

			pBufferBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			pBufferBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		}
		else
		{
			pBufferBarrier = &bufferBarriers[bufferBarrierCount++];
			pBufferBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			pBufferBarrier->pNext = NULL;

			pBufferBarrier->srcAccessMask = util_to_vk_access_flags(pTrans->mCurrentState);
			pBufferBarrier->dstAccessMask = util_to_vk_access_flags(pTrans->mNewState);
		}

		if (pBufferBarrier)
		{
			pBufferBarrier->buffer = pBuffer->mVulkan.pVkBuffer;
			pBufferBarrier->size = VK_WHOLE_SIZE;
			pBufferBarrier->offset = 0;

			if (pTrans->mAcquire)
			{
				pBufferBarrier->srcQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
				pBufferBarrier->dstQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
			}
			else if (pTrans->mRelease)
			{
				pBufferBarrier->srcQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
				pBufferBarrier->dstQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
			}
			else
			{
				pBufferBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pBufferBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			srcAccessFlags |= pBufferBarrier->srcAccessMask;
			dstAccessFlags |= pBufferBarrier->dstAccessMask;
		}
	}

	for (uint32_t i = 0; i < numTextureBarriers; ++i)
	{
		TextureBarrier* pTrans = &pTextureBarriers[i];
		Texture* pTexture = pTrans->pTexture;
		VkImageMemoryBarrier* pImageBarrier = NULL;

		if (RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mCurrentState && RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mNewState)
		{
			pImageBarrier = &imageBarriers[imageBarrierCount++];              //-V522
			pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;    //-V522
			pImageBarrier->pNext = NULL;

			pImageBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			pImageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			pImageBarrier->oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			pImageBarrier->newLayout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			pImageBarrier = &imageBarriers[imageBarrierCount++];
			pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			pImageBarrier->pNext = NULL;

			pImageBarrier->srcAccessMask = util_to_vk_access_flags(pTrans->mCurrentState);
			pImageBarrier->dstAccessMask = util_to_vk_access_flags(pTrans->mNewState);
			pImageBarrier->oldLayout = util_to_vk_image_layout(pTrans->mCurrentState);
			pImageBarrier->newLayout = util_to_vk_image_layout(pTrans->mNewState);
		}

		if (pImageBarrier)
		{
			pImageBarrier->image = pTexture->mVulkan.pVkImage;
			pImageBarrier->subresourceRange.aspectMask = (VkImageAspectFlags)pTexture->mAspectMask;
			pImageBarrier->subresourceRange.baseMipLevel = pTrans->mSubresourceBarrier ? pTrans->mMipLevel : 0;
			pImageBarrier->subresourceRange.levelCount = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
			pImageBarrier->subresourceRange.baseArrayLayer = pTrans->mSubresourceBarrier ? pTrans->mArrayLayer : 0;
			pImageBarrier->subresourceRange.layerCount = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

			if (pTrans->mAcquire && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
			{
				pImageBarrier->srcQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
				pImageBarrier->dstQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
			}
			else if (pTrans->mRelease && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
			{
				pImageBarrier->srcQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
				pImageBarrier->dstQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
			}
			else
			{
				pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			srcAccessFlags |= pImageBarrier->srcAccessMask;
			dstAccessFlags |= pImageBarrier->dstAccessMask;
		}
	}

	for (uint32_t i = 0; i < numRtBarriers; ++i)
	{
		RenderTargetBarrier* pTrans = &pRtBarriers[i];
		Texture* pTexture = pTrans->pRenderTarget->pTexture;
		VkImageMemoryBarrier* pImageBarrier = NULL;

		if (RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mCurrentState && RESOURCE_STATE_UNORDERED_ACCESS == pTrans->mNewState)
		{
			pImageBarrier = &imageBarriers[imageBarrierCount++];
			pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			pImageBarrier->pNext = NULL;

			pImageBarrier->srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			pImageBarrier->dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			pImageBarrier->oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			pImageBarrier->newLayout = VK_IMAGE_LAYOUT_GENERAL;
		}
		else
		{
			pImageBarrier = &imageBarriers[imageBarrierCount++];
			pImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			pImageBarrier->pNext = NULL;

			pImageBarrier->srcAccessMask = util_to_vk_access_flags(pTrans->mCurrentState);
			pImageBarrier->dstAccessMask = util_to_vk_access_flags(pTrans->mNewState);
			pImageBarrier->oldLayout = util_to_vk_image_layout(pTrans->mCurrentState);
			pImageBarrier->newLayout = util_to_vk_image_layout(pTrans->mNewState);
		}

		if (pImageBarrier)
		{
			pImageBarrier->image = pTexture->mVulkan.pVkImage;
			pImageBarrier->subresourceRange.aspectMask = (VkImageAspectFlags)pTexture->mAspectMask;
			pImageBarrier->subresourceRange.baseMipLevel = pTrans->mSubresourceBarrier ? pTrans->mMipLevel : 0;
			pImageBarrier->subresourceRange.levelCount = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
			pImageBarrier->subresourceRange.baseArrayLayer = pTrans->mSubresourceBarrier ? pTrans->mArrayLayer : 0;
			pImageBarrier->subresourceRange.layerCount = pTrans->mSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;

			if (pTrans->mAcquire && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
			{
				pImageBarrier->srcQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
				pImageBarrier->dstQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
			}
			else if (pTrans->mRelease && pTrans->mCurrentState != RESOURCE_STATE_UNDEFINED)
			{
				pImageBarrier->srcQueueFamilyIndex = pCmd->pQueue->mVulkan.mVkQueueFamilyIndex;
				pImageBarrier->dstQueueFamilyIndex = pCmd->pRenderer->mVulkan.mQueueFamilyIndices[pTrans->mQueueType];
			}
			else
			{
				pImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			srcAccessFlags |= pImageBarrier->srcAccessMask;
			dstAccessFlags |= pImageBarrier->dstAccessMask;
		}
	}

	VkPipelineStageFlags srcStageMask =
		util_determine_pipeline_stage_flags(pCmd->pRenderer, srcAccessFlags, (QueueType)pCmd->mVulkan.mType);
	VkPipelineStageFlags dstStageMask =
		util_determine_pipeline_stage_flags(pCmd->pRenderer, dstAccessFlags, (QueueType)pCmd->mVulkan.mType);

	if (bufferBarrierCount || imageBarrierCount)
	{
		vkCmdPipelineBarrier(
			pCmd->mVulkan.pVkCmdBuf, srcStageMask, dstStageMask, 0, 0, NULL, bufferBarrierCount, bufferBarriers, imageBarrierCount,
			imageBarriers);
	}
}

void vk_cmdUpdateSubresource(Cmd* pCmd, Texture* pTexture, Buffer* pSrcBuffer, const SubresourceDataDesc* pSubresourceDesc)
{
	const TinyImageFormat fmt = (TinyImageFormat)pTexture->mFormat;
	const bool            isSinglePlane = TinyImageFormat_IsSinglePlane(fmt);

	if (isSinglePlane)
	{
		const uint32_t width = max<uint32_t>(1, pTexture->mWidth >> pSubresourceDesc->mMipLevel);
		const uint32_t height = max<uint32_t>(1, pTexture->mHeight >> pSubresourceDesc->mMipLevel);
		const uint32_t depth = max<uint32_t>(1, pTexture->mDepth >> pSubresourceDesc->mMipLevel);
		const uint32_t numBlocksWide = pSubresourceDesc->mRowPitch / (TinyImageFormat_BitSizeOfBlock(fmt) >> 3);
		const uint32_t numBlocksHigh = (pSubresourceDesc->mSlicePitch / pSubresourceDesc->mRowPitch);

		VkBufferImageCopy copy = {};
		copy.bufferOffset = pSubresourceDesc->mSrcOffset;
		copy.bufferRowLength = numBlocksWide * TinyImageFormat_WidthOfBlock(fmt);
		copy.bufferImageHeight = numBlocksHigh * TinyImageFormat_HeightOfBlock(fmt);
		copy.imageSubresource.aspectMask = (VkImageAspectFlags)pTexture->mAspectMask;
		copy.imageSubresource.mipLevel = pSubresourceDesc->mMipLevel;
		copy.imageSubresource.baseArrayLayer = pSubresourceDesc->mArrayLayer;
		copy.imageSubresource.layerCount = 1;
		copy.imageOffset.x = 0;
		copy.imageOffset.y = 0;
		copy.imageOffset.z = 0;
		copy.imageExtent.width = width;
		copy.imageExtent.height = height;
		copy.imageExtent.depth = depth;

		vkCmdCopyBufferToImage(
			pCmd->mVulkan.pVkCmdBuf, pSrcBuffer->mVulkan.pVkBuffer, pTexture->mVulkan.pVkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copy);
	}
	else
	{
		const uint32_t width = pTexture->mWidth;
		const uint32_t height = pTexture->mHeight;
		const uint32_t depth = pTexture->mDepth;
		const uint32_t numOfPlanes = TinyImageFormat_NumOfPlanes(fmt);

		uint64_t          offset = pSubresourceDesc->mSrcOffset;
		VkBufferImageCopy bufferImagesCopy[MAX_PLANE_COUNT];

		for (uint32_t i = 0; i < numOfPlanes; ++i)
		{
			VkBufferImageCopy& copy = bufferImagesCopy[i];
			copy.bufferOffset = offset;
			copy.bufferRowLength = 0;
			copy.bufferImageHeight = 0;
			copy.imageSubresource.aspectMask = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i);
			copy.imageSubresource.mipLevel = pSubresourceDesc->mMipLevel;
			copy.imageSubresource.baseArrayLayer = pSubresourceDesc->mArrayLayer;
			copy.imageSubresource.layerCount = 1;
			copy.imageOffset.x = 0;
			copy.imageOffset.y = 0;
			copy.imageOffset.z = 0;
			copy.imageExtent.width = TinyImageFormat_PlaneWidth(fmt, i, width);
			copy.imageExtent.height = TinyImageFormat_PlaneHeight(fmt, i, height);
			copy.imageExtent.depth = depth;
			offset += copy.imageExtent.width * copy.imageExtent.height * TinyImageFormat_PlaneSizeOfBlock(fmt, i);
		}

		vkCmdCopyBufferToImage(
			pCmd->mVulkan.pVkCmdBuf, pSrcBuffer->mVulkan.pVkBuffer, pTexture->mVulkan.pVkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			numOfPlanes, bufferImagesCopy);
	}
}

/************************************************************************/
// Buffer Functions
/************************************************************************/
void vk_mapBuffer(Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange)
{
	ASSERT(pBuffer->mMemoryUsage != RESOURCE_MEMORY_USAGE_GPU_ONLY && "Trying to map non-cpu accessible resource");

	CHECK_VKRESULT(vmaMapMemory(pRenderer->mVulkan.pVmaAllocator, pBuffer->mVulkan.pVkAllocation, &pBuffer->pCpuMappedAddress));

	if (pRange)
	{
		pBuffer->pCpuMappedAddress = ((uint8_t*)pBuffer->pCpuMappedAddress + pRange->mOffset);
	}
}

void vk_unmapBuffer(Renderer* pRenderer, Buffer* pBuffer)
{
	ASSERT(pBuffer->mMemoryUsage != RESOURCE_MEMORY_USAGE_GPU_ONLY && "Trying to unmap non-cpu accessible resource");

	vmaUnmapMemory(pRenderer->mVulkan.pVmaAllocator, pBuffer->mVulkan.pVkAllocation);
	pBuffer->pCpuMappedAddress = NULL;
}

#include "../ThirdParty/OpenSource/volk/volk.c"
#endif