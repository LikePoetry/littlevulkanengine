#include "../Core/Config.h"

#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/IFont.h"

#define FONTSTASH_IMPLEMENTATION
#include "../../ThirdParty/OpenSource/Fontstash/src/fontstash.h"

#include "../Core/RingBuffer.h"

#include "../../Renderer/Include/IRenderer.h"
#include "../../Renderer/Include/IResourceLoader.h"

#include "../../ThirdParty/OpenSource/EASTL/vector.h"
#include "../../ThirdParty/OpenSource/EASTL/string.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"

#include "../Interfaces/IMemory.h"

class _Impl_FontStash
{
public:
	bool init(int width_, int height_)
	{
		// create FONS context
		FONSparams params;
		memset(&params, 0, sizeof(params));
		params.width = width_;
		params.height = height_;
		params.flags = (unsigned char)FONS_ZERO_TOPLEFT;
		params.renderCreate = fonsImplementationGenerateTexture;
		params.renderUpdate = fonsImplementationModifyTexture;
		params.renderDelete = fonsImplementationRemoveTexture;
		params.renderDraw = fonsImplementationRenderText;
		params.userPtr = this;

		pContext = fonsCreateInternal(&params);

		return true;
	}

	bool initRender(Renderer* renderer, int width_, int height_, uint32_t ringSizeBytes) 
	{
		pRenderer = renderer;

		// create image
		TextureDesc desc = {};
		desc.mArraySize = 1;
		desc.mDepth = 1;
		desc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
		desc.mFormat = TinyImageFormat_R8_UNORM;
		desc.mHeight = height_;
		desc.mMipLevels = 1;
		desc.mSampleCount = SAMPLE_COUNT_1;
		desc.mStartState = RESOURCE_STATE_COMMON;
		desc.mWidth = width_;
		desc.pName = "Fontstash Texture";
		TextureLoadDesc loadDesc = {};
		loadDesc.ppTexture = &pCurrentTexture;
		loadDesc.pDesc = &desc;
		addResource(&loadDesc, NULL);

		/************************************************************************/
		// Rendering resources
		/************************************************************************/
		SamplerDesc samplerDesc = { FILTER_LINEAR,
									FILTER_LINEAR,
									MIPMAP_MODE_NEAREST,
									ADDRESS_MODE_CLAMP_TO_EDGE,
									ADDRESS_MODE_CLAMP_TO_EDGE,
									ADDRESS_MODE_CLAMP_TO_EDGE };
		vk_addSampler(pRenderer, &samplerDesc, &pDefaultSampler);

#ifdef ENABLE_TEXT_PRECOMPILED_SHADERS
		BinaryShaderDesc binaryShaderDesc = {};
		binaryShaderDesc.mStages = SHADER_STAGE_VERT | SHADER_STAGE_FRAG;
		binaryShaderDesc.mVert.mByteCodeSize = sizeof(gShaderFontstash2DVert);
		binaryShaderDesc.mVert.pByteCode = (char*)gShaderFontstash2DVert;
		binaryShaderDesc.mVert.pEntryPoint = "main";
		binaryShaderDesc.mFrag.mByteCodeSize = sizeof(gShaderFontstashFrag);
		binaryShaderDesc.mFrag.pByteCode = (char*)gShaderFontstashFrag;
		binaryShaderDesc.mFrag.pEntryPoint = "main";
		addShaderBinary(pRenderer, &binaryShaderDesc, &pShaders[0]);
		binaryShaderDesc.mVert.mByteCodeSize = sizeof(gShaderFontstash3DVert);
		binaryShaderDesc.mVert.pByteCode = (char*)gShaderFontstash3DVert;
		binaryShaderDesc.mVert.pEntryPoint = "main";
		addShaderBinary(pRenderer, &binaryShaderDesc, &pShaders[1]);
#else
		ShaderLoadDesc text2DShaderDesc = {};
		text2DShaderDesc.mStages[0] = { "fontstash2D.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
		text2DShaderDesc.mStages[1] = { "fontstash.frag", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
		ShaderLoadDesc text3DShaderDesc = {};
		text3DShaderDesc.mStages[0] = { "fontstash3D.vert", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
		text3DShaderDesc.mStages[1] = { "fontstash.frag", NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };

		addShader(pRenderer, &text2DShaderDesc, &pShaders[0]);
		addShader(pRenderer, &text3DShaderDesc, &pShaders[1]);
#endif

		RootSignatureDesc textureRootDesc = { pShaders, 2 };
		const char* pStaticSamplers[] = { "uSampler0" };
		textureRootDesc.mStaticSamplerCount = 1;
		textureRootDesc.ppStaticSamplerNames = pStaticSamplers;
		textureRootDesc.ppStaticSamplers = &pDefaultSampler;
		vk_addRootSignature(pRenderer, &textureRootDesc, &pRootSignature);
		mRootConstantIndex = getDescriptorIndexFromName(pRootSignature, "uRootConstants");

		addUniformGPURingBuffer(pRenderer, 65536, &pUniformRingBuffer, true);

		DescriptorSetDesc setDesc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
		vk_addDescriptorSet(pRenderer, &setDesc, &pDescriptorSets);
		DescriptorData setParams[1] = {};
		setParams[0].pName = "uTex0";
		setParams[0].ppTextures = &pCurrentTexture;
		vk_updateDescriptorSet(pRenderer, 0, pDescriptorSets, 1, setParams);

		BufferDesc vbDesc = {};
		vbDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
		vbDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
		vbDesc.mSize = ringSizeBytes;
		vbDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
		addGPURingBuffer(pRenderer, &vbDesc, &pMeshRingBuffer);
		/************************************************************************/
		/************************************************************************/

		return true;
	}

	static int  fonsImplementationGenerateTexture(void* userPtr, int width, int height);
	static void fonsImplementationModifyTexture(void* userPtr, int* rect, const unsigned char* data);
	static void fonsImplementationRenderText(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
	static void fonsImplementationRemoveTexture(void* userPtr);

	Renderer* pRenderer;
	FONScontext* pContext;

	const uint8_t* pPixels;
	Texture* pCurrentTexture;
	bool     mUpdateTexture;

	uint32_t mWidth;
	uint32_t mHeight;
	float2   mScaleBias;

	eastl::vector<void*>         mFontBuffers;
	eastl::vector<uint32_t>      mFontBufferSizes;
	eastl::vector<eastl::string> mFontNames;

	CameraMatrix mProjView;
	mat4 mWorldMat;
	Cmd* pCmd;

	Shader* pShaders[2];
	RootSignature* pRootSignature;
	DescriptorSet* pDescriptorSets;
	Pipeline* pPipelines[2];
	/// Default states
	Sampler* pDefaultSampler;

	GPURingBuffer* pUniformRingBuffer;
	GPURingBuffer* pMeshRingBuffer;
	float2         mDpiScale;
	float          mDpiScaleMin;
	uint32_t       mRootConstantIndex;
	bool           mText3D;
};


// FONTSTASH
float                  m_fFontMaxSize;
int32_t                mWidth;
int32_t                mHeight;
_Impl_FontStash* impl;

bool renderInitialized = false;

const int TextureAtlasDimension = 2048;

bool platformInitFontSystem()
{
#ifdef ENABLE_FORGE_FONTS
	impl = tf_placement_new<_Impl_FontStash>(tf_calloc(1, sizeof(_Impl_FontStash)));
	{
		float dpiScale[2];
		getDpiScale(dpiScale);
		impl->mDpiScale.x = dpiScale[0];
		impl->mDpiScale.y = dpiScale[1];
	}
	impl->mDpiScaleMin = min(impl->mDpiScale.x, impl->mDpiScale.y);

	mWidth = TextureAtlasDimension * (int)ceilf(impl->mDpiScale.x);
	mHeight = TextureAtlasDimension * (int)ceilf(impl->mDpiScale.y);

	bool success = impl->init(mWidth, mHeight);

	m_fFontMaxSize = min(mWidth, mHeight) / 10.0f;    // see fontstash.h, line 1271, for fontSize calculation

	return success;
#else
	return true;
#endif // ENABLE_FORGE_FONTS
}

void* fntGetRawFontData(uint32_t fontID)
{
	if (fontID < impl->mFontBuffers.size())
		return impl->mFontBuffers[fontID];
	else
		return NULL;
}

uint32_t fntGetRawFontDataSize(uint32_t fontID)
{
	if (fontID < impl->mFontBufferSizes.size())
		return impl->mFontBufferSizes[fontID];
	else
		return UINT_MAX;
	
}

bool initFontSystem(FontSystemDesc* pDesc) 
{
#ifdef ENABLE_FORGE_FONTS
	ASSERT(!renderInitialized);

	bool success = impl->initRender((Renderer*)pDesc->pRenderer, mWidth, mHeight, pDesc->mFontstashRingSizeBytes);
	if (success)
		renderInitialized = true;

	return success;
#else
	return true;
#endif
}

void fntDefineFonts(const FontDesc* pDescs, uint32_t count, uint32_t* pOutIDs)
{
#ifdef ENABLE_FORGE_FONTS
	ASSERT(pDescs);
	ASSERT(pOutIDs);
	ASSERT(count > 0);

	for (uint32_t i = 0; i < count; ++i)
	{
		//uint32_t id = (uint32_t)pFontStash->defineFont(pDescs[i].pFontName, pDescs[i].pFontPath);
		uint32_t id;
		FONScontext* fs = impl->pContext;

		FileStream fh = {};
		if (fsOpenStreamFromPath(RD_FONTS, pDescs[i].pFontPath, FM_READ_BINARY, pDescs[i].pFontPassword, &fh))
		{
			ssize_t bytes = fsGetStreamFileSize(&fh);
			void* buffer = tf_malloc(bytes);
			fsReadFromStream(&fh, buffer, bytes);

			// add buffer to font buffers for cleanup
			impl->mFontBuffers.emplace_back(buffer);
			impl->mFontBufferSizes.emplace_back((uint32_t)bytes);
			impl->mFontNames.emplace_back(pDescs[i].pFontPath);

			fsCloseStream(&fh);

			id = fonsAddFontMem(fs, pDescs[i].pFontName, (unsigned char*)buffer, (int)bytes, 0);
		}
		else
		{
			id = INT32_MAX;
		}

		ASSERT(id != INT32_MAX);

		pOutIDs[i] = id;
	}


#endif
}


// --  FONS renderer implementation --
int _Impl_FontStash::fonsImplementationGenerateTexture(void* userPtr, int width, int height)
{
	_Impl_FontStash* ctx = (_Impl_FontStash*)userPtr;
	ctx->mWidth = width;
	ctx->mHeight = height;

	ctx->mUpdateTexture = true;

	return 1;
}

void _Impl_FontStash::fonsImplementationModifyTexture(void* userPtr, int* rect, const unsigned char* data)
{
	UNREF_PARAM(rect);

	_Impl_FontStash* ctx = (_Impl_FontStash*)userPtr;

	ctx->pPixels = data;
	ctx->mUpdateTexture = true;
}

void _Impl_FontStash::fonsImplementationRenderText(
	void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
	_Impl_FontStash* ctx = (_Impl_FontStash*)userPtr;
	if (!ctx->pCurrentTexture)
		return;

	Cmd* pCmd = ctx->pCmd;
	if (ctx->mUpdateTexture)
	{
		vk_waitQueueIdle(pCmd->pQueue);

		SyncToken token = {};
		TextureUpdateDesc updateDesc = {};
		updateDesc.pTexture = ctx->pCurrentTexture;
		beginUpdateResource(&updateDesc);
		for (uint32_t r = 0; r < updateDesc.mRowCount; ++r)
		{
			memcmp(updateDesc.pMappedData + r * updateDesc.mDstRowStride,
				ctx->pPixels + r * updateDesc.mSrcRowStride,
				updateDesc.mSrcRowStride);
		}
		endUpdateResource(&updateDesc, &token);
		waitForToken(&token);

		ctx->mUpdateTexture = false;
	}

	GPURingBufferOffset buffer = getGPURingBufferOffset(ctx->pMeshRingBuffer, nverts * sizeof(float4));
	BufferUpdateDesc update = { buffer.pBuffer,buffer.mOffset };
	beginUpdateResource(&update);
	float4* vtx = (float4*)update.pMappedData;
	// build vertices
	for (int impl = 0; impl < nverts; impl++)
	{
		vtx[impl].setX(verts[impl * 2 + 0]);
		vtx[impl].setY(verts[impl * 2 + 1]);
		vtx[impl].setZ(tcoords[impl * 2 + 0]);
		vtx[impl].setW(tcoords[impl * 2 + 1]);
	}
	endUpdateResource(&update, NULL);

	// extract color
	float4 color = unpackA8B8G8R8_SRGB(*colors);

	uint32_t pipelineIndex = ctx->mText3D ? 1 : 0;
	Pipeline* pPipeline = ctx->pPipelines[pipelineIndex];
	ASSERT(pPipeline);

	vk_cmdBindPipeline(pCmd, pPipeline);

	struct UniformData
	{
		float4 color;
		float2 scaleBias;
	} data;

	data.color = color;
	data.scaleBias = ctx->mScaleBias;

	if(ctx->mText3D)
	{
		mat4 mvp = (ctx->mProjView * ctx->mWorldMat).getPrimaryMatrix();
		data.color = color;
		data.scaleBias.x = -data.scaleBias.x;

		GPURingBufferOffset uniformBlock = getGPURingBufferOffset(ctx->pUniformRingBuffer, sizeof(mvp));
		BufferUpdateDesc    updateDesc = { uniformBlock.pBuffer, uniformBlock.mOffset };
		beginUpdateResource(&updateDesc);
		*((mat4*)updateDesc.pMappedData) = mvp;
		endUpdateResource(&updateDesc, NULL);

		const uint32_t size = sizeof(mvp);
		const uint32_t stride = sizeof(float4);

		DescriptorDataRange range = { (uint32_t)uniformBlock.mOffset, size };
		DescriptorData params[1] = {};
		params[0].pName = "uniformBlock_rootcbv";
		params[0].ppBuffers = &uniformBlock.pBuffer;
		params[0].pRanges = &range;
		vk_cmdBindDescriptorSetWithRootCbvs(pCmd, 0, ctx->pDescriptorSets, 1, params);
		vk_cmdBindPushConstants(pCmd, ctx->pRootSignature, ctx->mRootConstantIndex, &data);
		vk_cmdBindVertexBuffer(pCmd, 1, &buffer.pBuffer, &stride, &buffer.mOffset);
		vk_cmdDraw(pCmd, nverts, 0);
	}
	else
	{
		const uint32_t stride = sizeof(float4);
		vk_cmdBindDescriptorSet(pCmd, 0, ctx->pDescriptorSets);
		vk_cmdBindPushConstants(pCmd, ctx->pRootSignature, ctx->mRootConstantIndex, &data);
		vk_cmdBindVertexBuffer(pCmd, 1, &buffer.pBuffer, &stride, &buffer.mOffset);
		vk_cmdDraw(pCmd, nverts, 0);
	}
}

void _Impl_FontStash::fonsImplementationRemoveTexture(void* userPtr) { UNREF_PARAM(userPtr); }