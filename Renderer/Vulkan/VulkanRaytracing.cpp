#include "../Include/RendererConfig.h"



#include "../../ThirdParty/OpenSource/EASTL/vector.h"
#include "../../ThirdParty/OpenSource/EASTL/sort.h"
#include "../../ThirdParty/OpenSource/EASTL/string.h"

#include "../Include/IRenderer.h"

// Renderer
#include "../Include/IRenderer.h"
#include "../Include/IRay.h"
#include "../Include/IResourceLoader.h"

#include "../../OS/Interfaces/IMemory.h"

extern VkAllocationCallbacks gVkAllocationCallbacks;

struct AccelerationStructureBottom
{
	Buffer* pVertexBuffer;
	Buffer* pIndexBuffer;
	Buffer* pASBuffer;
	VkAccelerationStructureKHR           mAccelerationStructure;
	uint64_t                             mStructureDeviceAddress;
	VkAccelerationStructureGeometryKHR* pGeometryDescs;
	VkBuildAccelerationStructureFlagsKHR mFlags;
	VkBuffer                             mScratchBufferBottom;
	VkDeviceMemory                       mScratchBufferBottomMemory;
	uint64_t                             mScratchBufferBottomDeviceAddress;
	uint32_t                             mDescCount;
	uint32_t                             mPrimitiveCount;
};

struct AccelerationStructure
{
	AccelerationStructureBottom          mBottomAS;
	Buffer* pInstanceDescBuffer;
	Buffer* pASBuffer;
	VkBuffer                             mScratchBufferTop;
	VkDeviceMemory                       mScratchBufferTopMemory;
	uint64_t                             mScratchBufferTopDeviceAddress;
	uint32_t                             mInstanceDescCount;
	uint64_t                             mScratchBufferSize;
	VkAccelerationStructureGeometryKHR   mInstanceBufferDesc;
	VkBuildAccelerationStructureFlagsKHR mFlags;
	VkAccelerationStructureKHR           mAccelerationStructure;
	uint64_t                             mStructureDeviceAddress;
	uint32_t                             mPrimitiveCount;
};

void vk_addRaytracingPipeline(const PipelineDesc* pMainDesc, Pipeline** ppPipeline)
{
	const RaytracingPipelineDesc* pDesc = &pMainDesc->mRaytracingDesc;
	VkPipelineCache               psoCache = pMainDesc->pCache ? pMainDesc->pCache->mVulkan.pCache : VK_NULL_HANDLE;

	Pipeline* pResult = (Pipeline*)tf_calloc_memalign(1, alignof(Pipeline), sizeof(Pipeline));
	ASSERT(pResult);

	pResult->mVulkan.mType = PIPELINE_TYPE_RAYTRACING;
	eastl::vector<VkPipelineShaderStageCreateInfo>     stages;
	eastl::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
	/************************************************************************/
	// Generate Stage Names
	/************************************************************************/
	stages.reserve(1 + pDesc->mMissShaderCount + pDesc->mHitGroupCount);
	groups.reserve(1 + pDesc->mMissShaderCount + pDesc->mHitGroupCount);
	pResult->mVulkan.mShaderStageCount = 0;
	pResult->mVulkan.ppShaderStageNames = (const char**)tf_calloc(1 + pDesc->mMissShaderCount + pDesc->mHitGroupCount * 3, sizeof(char*));

	//////////////////////////////////////////////////////////////////////////
	//1. Ray-gen shader
	{
		VkPipelineShaderStageCreateInfo stageCreateInfo = {};
		stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCreateInfo.pNext = nullptr;
		stageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		stageCreateInfo.module = pDesc->pRayGenShader->mVulkan.pShaderModules[0];
		stageCreateInfo.pName = "main";
		stageCreateInfo.flags = 0;
		stageCreateInfo.pSpecializationInfo = pDesc->pRayGenShader->mVulkan.pSpecializationInfo;
		stages.push_back(stageCreateInfo);

		pResult->mVulkan.ppShaderStageNames[pResult->mVulkan.mShaderStageCount] = pDesc->pRayGenShader->mVulkan.pEntryNames[0];
		pResult->mVulkan.mShaderStageCount += 1;

		VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
		groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupInfo.pNext = nullptr;
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupInfo.generalShader = (uint32_t)stages.size() - 1;
		groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		groups.push_back(groupInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	//2. Miss shaders
	{
		VkPipelineShaderStageCreateInfo stageCreateInfo = {};
		stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCreateInfo.pNext = nullptr;
		stageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		stageCreateInfo.module = VK_NULL_HANDLE;
		stageCreateInfo.pName = "main";
		stageCreateInfo.flags = 0;

		VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
		groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupInfo.pNext = nullptr;
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
		for (uint32_t i = 0; i < pDesc->mMissShaderCount; ++i)
		{
			stageCreateInfo.module = pDesc->ppMissShaders[i]->mVulkan.pShaderModules[0];
			stageCreateInfo.pSpecializationInfo = pDesc->ppMissShaders[i]->mVulkan.pSpecializationInfo;
			stages.push_back(stageCreateInfo);

			pResult->mVulkan.ppShaderStageNames[pResult->mVulkan.mShaderStageCount] = pDesc->ppMissShaders[i]->mVulkan.pEntryNames[0];
			pResult->mVulkan.mShaderStageCount += 1;

			groupInfo.generalShader = (uint32_t)stages.size() - 1;
			groups.push_back(groupInfo);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//3. Hit group
	{
		VkPipelineShaderStageCreateInfo stageCreateInfo = {};
		stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCreateInfo.pNext = nullptr;
		stageCreateInfo.stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		stageCreateInfo.module = VK_NULL_HANDLE;
		stageCreateInfo.pName = "main";
		stageCreateInfo.flags = 0;

		VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
		groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupInfo.pNext = nullptr;
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

		for (uint32_t i = 0; i < pDesc->mHitGroupCount; ++i)
		{
			groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
			groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
			groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
			groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

			if (pDesc->pHitGroups[i].pIntersectionShader)
			{
				stageCreateInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				stageCreateInfo.module = pDesc->pHitGroups[i].pIntersectionShader->mVulkan.pShaderModules[0];
				stageCreateInfo.pSpecializationInfo = pDesc->pHitGroups[i].pIntersectionShader->mVulkan.pSpecializationInfo;
				stages.push_back(stageCreateInfo);
				pResult->mVulkan.ppShaderStageNames[pResult->mVulkan.mShaderStageCount] = pDesc->pHitGroups[i].pHitGroupName;
				pResult->mVulkan.mShaderStageCount += 1;

				groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
				groupInfo.intersectionShader = (uint32_t)stages.size() - 1;
			}
			if (pDesc->pHitGroups[i].pAnyHitShader)
			{
				stageCreateInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				stageCreateInfo.module = pDesc->pHitGroups[i].pAnyHitShader->mVulkan.pShaderModules[0];
				stageCreateInfo.pSpecializationInfo = pDesc->pHitGroups[i].pAnyHitShader->mVulkan.pSpecializationInfo;
				stages.push_back(stageCreateInfo);
				pResult->mVulkan.ppShaderStageNames[pResult->mVulkan.mShaderStageCount] = pDesc->pHitGroups[i].pHitGroupName;
				pResult->mVulkan.mShaderStageCount += 1;

				groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				groupInfo.anyHitShader = (uint32_t)stages.size() - 1;
			}
			if (pDesc->pHitGroups[i].pClosestHitShader)
			{
				stageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				stageCreateInfo.module = pDesc->pHitGroups[i].pClosestHitShader->mVulkan.pShaderModules[0];
				stageCreateInfo.pSpecializationInfo = pDesc->pHitGroups[i].pClosestHitShader->mVulkan.pSpecializationInfo;
				stages.push_back(stageCreateInfo);
				pResult->mVulkan.ppShaderStageNames[pResult->mVulkan.mShaderStageCount] = pDesc->pHitGroups[i].pHitGroupName;
				pResult->mVulkan.mShaderStageCount += 1;

				groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				groupInfo.closestHitShader = (uint32_t)stages.size() - 1;
			}
			groups.push_back(groupInfo);
		}
	}
	/************************************************************************/
	// Create Pipeline
	/************************************************************************/

	uint32_t maxRecursionDepth = min(pDesc->mMaxTraceRecursionDepth, pDesc->pRaytracing->mRayTracingPipelineProperties.maxRayRecursionDepth);

	VkRayTracingPipelineCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	createInfo.flags = 0;    //VkPipelineCreateFlagBits
	createInfo.stageCount = (uint32_t)stages.size();
	createInfo.pStages = stages.data();
	createInfo.groupCount = (uint32_t)groups.size();    //ray-gen groups
	createInfo.pGroups = groups.data();
	createInfo.maxPipelineRayRecursionDepth = maxRecursionDepth;
	createInfo.layout = pDesc->pGlobalRootSignature->mVulkan.pPipelineLayout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = 0;

	CHECK_VKRESULT(vkCreateRayTracingPipelinesKHR(
		pDesc->pRaytracing->pRenderer->mVulkan.pVkDevice, VK_NULL_HANDLE, psoCache, 1, &createInfo, &gVkAllocationCallbacks,
		&pResult->mVulkan.pVkPipeline));

	*ppPipeline = pResult;
}

void vk_FillRaytracingDescriptorData(uint32_t count, AccelerationStructure** const ppAccelerationStructures, VkAccelerationStructureKHR* pOutHandles)
{
	for (uint32_t i = 0; i < count; ++i)
	{
		pOutHandles[i] = ppAccelerationStructures[i]->mAccelerationStructure;
	}
}