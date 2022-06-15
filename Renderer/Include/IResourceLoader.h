#pragma once
#include "../Include/RendererConfig.h"

#include "../OS/Math/MathTypes.h"
#include "../OS/Core/Atomics.h"
#include "../Include/IRenderer.h"

typedef struct MappedMemoryRange
{
	uint8_t* pData;
	Buffer* pBuffer;
	uint64_t mOffset;
	uint64_t mSize;
	uint32_t mFlags;
} MappedMemoryRange;

typedef enum TextureContainerType
{
	/// Use whatever container is designed for that platform
	/// Windows, macOS, Linux - TEXTURE_CONTAINER_DDS
	/// iOS, Android          - TEXTURE_CONTAINER_KTX
	TEXTURE_CONTAINER_DEFAULT = 0,
	/// Explicit container types
	/// .dds
	TEXTURE_CONTAINER_DDS,
	/// .ktx
	TEXTURE_CONTAINER_KTX,
	/// .gnf
	TEXTURE_CONTAINER_GNF,
	/// .basis
	TEXTURE_CONTAINER_BASIS,
	/// .svt
	TEXTURE_CONTAINER_SVT,
} TextureContainerType;

// MARK: - Resource Loading

typedef struct BufferLoadDesc
{
	Buffer** ppBuffer;
	const void* pData;
	BufferDesc  mDesc;
	/// Force Reset buffer to NULL
	bool mForceReset;
} BufferLoadDesc;

typedef struct TextureLoadDesc
{
	Texture** ppTexture;
	/// Load empty texture
	TextureDesc* pDesc;
	/// Filename without extension. Extension will be determined based on mContainer
	const char* pFileName;
	/// Password for loading texture from encrypted files
	const char* pFilePassword;
	/// The index of the GPU in SLI/Cross-Fire that owns this texture, or the Renderer index in unlinked mode.
	uint32_t mNodeIndex;
	/// Following is ignored if pDesc != NULL.  pDesc->mFlags will be considered instead.
	TextureCreationFlags mCreationFlag;
	/// The texture file format (dds/ktx/...)
	TextureContainerType mContainer;
} TextureLoadDesc;

typedef struct Geometry
{
	struct Hair
	{
		uint32_t mVertexCountPerStrand;
		uint32_t mGuideCountPerStrand;
	};

	struct ShadowData
	{
		void* pIndices;
		void* pAttributes[MAX_VERTEX_ATTRIBS];
	};

	/// Index buffer to bind when drawing this geometry
	Buffer* pIndexBuffer;
	/// The array of vertex buffers to bind when drawing this geometry
	Buffer* pVertexBuffers[MAX_VERTEX_BINDINGS];
	/// The array of traditional draw arguments to draw each subset in this geometry
	IndirectDrawIndexArguments* pDrawArgs;
	/// Shadow copy of the geometry vertex and index data if requested through the load flags
	ShadowData* pShadow;

	/// The array of joint inverse bind-pose matrices ( object-space )
	mat4* pInverseBindPoses;
	/// The array of data to remap skin batch local joint ids to global joint ids
	uint32_t* pJointRemaps;
	/// The array of vertex buffer strides to bind when drawing this geometry
	uint32_t mVertexStrides[MAX_VERTEX_BINDINGS];
	/// Hair data
	Hair mHair;

	/// Number of vertex buffers in this geometry
	uint32_t mVertexBufferCount : 8;
	/// Index type (32 or 16 bit)
	uint32_t mIndexType : 2;
	/// Number of joints in the skinned geometry
	uint32_t mJointCount : 16;
	/// Number of draw args in the geometry
	uint32_t mDrawArgCount;
	/// Number of indices in the geometry
	uint32_t mIndexCount;
	/// Number of vertices in the geometry
	uint32_t mVertexCount;

	uint32_t mPad[3];
} Geometry;
static_assert(sizeof(Geometry) % 16 == 0, "GLTFContainer size must be a multiple of 16");

typedef enum GeometryLoadFlags
{
	/// Keep shadow copy of indices and vertices for CPU
	GEOMETRY_LOAD_FLAG_SHADOWED = 0x1,
	/// Use structured buffers instead of raw buffers
	GEOMETRY_LOAD_FLAG_STRUCTURED_BUFFERS = 0x2,
} GeometryLoadFlags;
MAKE_ENUM_FLAG(uint32_t, GeometryLoadFlags)

typedef enum MeshOptimizerFlags
{
	MESH_OPTIMIZATION_FLAG_OFF = 0x0,
	/// Vertex cache optimization
	MESH_OPTIMIZATION_FLAG_VERTEXCACHE = 0x1,
	/// Overdraw optimization
	MESH_OPTIMIZATION_FLAG_OVERDRAW = 0x2,
	/// Vertex fetch optimization
	MESH_OPTIMIZATION_FLAG_VERTEXFETCH = 0x4,
	/// All
	MESH_OPTIMIZATION_FLAG_ALL = 0x7,
} MeshOptimizerFlags;
MAKE_ENUM_FLAG(uint32_t, MeshOptimizerFlags)

typedef struct GeometryLoadDesc
{
	/// Output geometry
	Geometry** ppGeometry;
	/// Filename of geometry container
	const char* pFileName;
	/// Password for file
	const char* pFilePassword;
	/// Loading flags
	GeometryLoadFlags mFlags;
	/// Optimization flags
	MeshOptimizerFlags mOptimizationFlags;
	/// Linked gpu node / Unlinked Renderer index
	uint32_t mNodeIndex;
	/// Specifies how to arrange the vertex data loaded from the file into GPU memory
	VertexLayout* pVertexLayout;
} GeometryLoadDesc;

typedef uint64_t SyncToken;

typedef struct BufferUpdateDesc
{
	Buffer* pBuffer;
	uint64_t mDstOffset;
	uint64_t mSize;

	/// To be filled by the caller
/// Example:
/// BufferUpdateDesc update = { pBuffer, bufferDstOffset };
/// beginUpdateResource(&update);
/// ParticleVertex* vertices = (ParticleVertex*)update.pMappedData;
///   for (uint32_t i = 0; i < particleCount; ++i)
///	    vertices[i] = { rand() };
/// endUpdateResource(&update, &token);
	void* pMappedData;

	/// Internal
	struct
	{
		MappedMemoryRange mMappedRange;
	} mInternal;
} BufferUpdateDesc;

/// #NOTE: Only use for procedural textures which are created on CPU (noise textures, font texture, ...)
typedef struct TextureUpdateDesc
{
	Texture* pTexture;
	uint32_t mMipLevel;
	uint32_t mArrayLayer;
	/// To be filled by the caller
	/// Example:
	/// BufferUpdateDesc update = { pTexture, 2, 1 };
	/// beginUpdateResource(&update);
	/// Row by row copy is required if mDstRowStride > mSrcRowStride. Single memcpy will work if mDstRowStride == mSrcRowStride
	/// 2D
	/// for (uint32_t r = 0; r < update.mRowCount; ++r)
	///     memcpy(update.pMappedData + r * update.mDstRowStride, srcPixels + r * update.mSrcRowStride, update.mSrcRowStride);
	/// 3D
	/// for (uint32_t z = 0; z < depth; ++z)
	/// {
	///     uint8_t* dstData = update.pMappedData + update.mDstSliceStride * z;
	///     uint8_t* srcData = srcPixels + update.mSrcSliceStride * z;
	///     for (uint32_t r = 0; r < update.mRowCount; ++r)
	///         memcpy(dstData + r * update.mDstRowStride, srcData + r * update.mSrcRowStride, update.mSrcRowStride);
	/// }
	/// endUpdateResource(&update, &token);
	uint8_t* pMappedData;
	/// Size of each row in destination including padding - Needs to be respected otherwise texture data will be corrupted if dst row stride is not the same as src row stride
	uint32_t mDstRowStride;
	/// Number of rows in this slice of the texture
	uint32_t mRowCount;
	/// Src row stride for convenience (mRowCount * width * texture format size)
	uint32_t mSrcRowStride;
	/// Size of each slice in destination including padding - Use for offsetting dst data updating 3D textures
	uint32_t mDstSliceStride;
	/// Size of each slice in src - Use for offsetting src data when updating 3D textures
	uint32_t mSrcSliceStride;

	/// Internal
	struct
	{
		MappedMemoryRange mMappedRange;
	} mInternal;

} TextureUpdateDesc;

typedef struct TextureCopyDesc
{
	Texture* pTexture;
	Buffer* pBuffer;
	/// Semaphore to synchronize graphics/compute operations that write to the texture with the texture -> buffer copy.
	Semaphore* pWaitSemaphore;
	uint32_t mTextureMipLevel;
	uint32_t mTextureArrayLayer;
	/// Current texture state.
	ResourceState mTextureState;
	/// Queue the texture is copied from.
	QueueType mQueueType;
	uint64_t mBufferOffset;
} TextureCopyDesc;

typedef struct ResourceLoaderDesc
{
	uint64_t mBufferSize;
	uint32_t mBufferCount;
	bool     mSingleThreaded;
} ResourceLoaderDesc;

extern ResourceLoaderDesc gDefaultResourceLoaderDesc;

void beginUpdateResource(TextureUpdateDesc* pTextureDesc);