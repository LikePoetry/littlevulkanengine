#pragma once
#include "Include/RendererConfig.h"

static const uint32_t MAX_SHADER_STAGE_COUNT = 5;

typedef enum TextureDimension
{
	TEXTURE_DIM_1D,
	TEXTURE_DIM_2D,
	TEXTURE_DIM_2DMS,
	TEXTURE_DIM_3D,
	TEXTURE_DIM_CUBE,
	TEXTURE_DIM_1D_ARRAY,
	TEXTURE_DIM_2D_ARRAY,
	TEXTURE_DIM_2DMS_ARRAY,
	TEXTURE_DIM_CUBE_ARRAY,
	TEXTURE_DIM_COUNT,
	TEXTURE_DIM_UNDEFINED,
} TextureDimension;

#include <ctype.h>

struct VertexInput
{
	// resource name
	const char* name;

	// The size of the attribute
	uint32_t size;

	// name size
	uint32_t name_size;
};

struct ShaderResource
{
	// resource Type
	DescriptorType type;

	// The resource set for binding frequency
	uint32_t set;

	// The resource binding location
	uint32_t reg;

	// The size of the resource. This will be the DescriptorInfo array size for textures
	uint32_t size;

	// what stages use this resource
	ShaderStage used_stages;

	// resource name
	const char* name;

	// name size
	uint32_t name_size;

	// 1D / 2D / Array / MSAA / ...
	TextureDimension dim;
};

struct ShaderVariable
{
	// Variable name
	const char* name;

	// parents resource index
	uint32_t parent_index;

	// The offset of the Variable.
	uint32_t offset;

	// The size of the Variable.
	uint32_t size;

	// name size
	uint32_t name_size;
};

struct ShaderReflection
{
	// single large allication for names to reduce number of allocations
	char* pNamePool;
	VertexInput* pVertexInputs;
	ShaderResource* pShaderResources;
	ShaderVariable* pVariables;
	char* pEntryPoint;
	ShaderStage mShaderStage;

	uint32_t mNamePoolSize;
	uint32_t mVertexInputsCount;
	uint32_t mShaderResourceCount;
	uint32_t mVariableCount;

	// Thread group size for compute shader
	uint32_t mNumThreadsPerGroup[3];

	//number of tessellation control point
	uint32_t mNumControlPoint;
};


struct PipelineReflection
{
	ShaderStage mShaderStages;
	// the individual stages reflection data.
	ShaderReflection mStageReflections[MAX_SHADER_STAGE_COUNT];
	uint32_t         mStageReflectionCount;

	uint32_t mVertexStageIndex;
	uint32_t mHullStageIndex;
	uint32_t mDomainStageIndex;
	uint32_t mGeometryStageIndex;
	uint32_t mPixelStageIndex;

	ShaderResource* pShaderResources;
	uint32_t        mShaderResourceCount;

	ShaderVariable* pVariables;
	uint32_t        mVariableCount;
};

void destroyShaderReflection(ShaderReflection* pReflection);
void createPipelineReflection(ShaderReflection* pReflection, uint32_t stageCount, PipelineReflection* pOutReflection);
void destroyPipelineReflection(PipelineReflection* pReflection);

inline bool isDescriptorRootCbv(const char* resourceName)
{
	char lower[MAX_RESOURCE_NAME_LENGTH] = {};
	uint32_t length = (uint32_t)strlen(resourceName);
	for (uint32_t i = 0; i < length; ++i)
	{
		lower[i] = tolower(resourceName[i]);
	}
	return strstr(lower, "rootcbv");
}