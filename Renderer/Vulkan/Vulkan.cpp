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

#include "../ThirdParty/OpenSource/volk/volk.c"
#endif