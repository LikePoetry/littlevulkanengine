#include "../Include/RendererConfig.h"

#include "../Include/IRay.h"

#include "../Include/IRenderer.h"



//extern VkAllocationCallbacks gVkAllocationCallbacks;

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