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

VkBufferUsageFlags util_to_vk_buffer_usage(DescriptorType usage,bool typed)
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

#include "../ThirdParty/OpenSource/volk/volk.c"
#endif