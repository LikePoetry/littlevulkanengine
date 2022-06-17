#include "../Core/Config.h"

#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/IFont.h"

#define FONTSTASH_IMPLEMENTATION
#include "../../ThirdParty/OpenSource/Fontstash/src/fontstash.h"

#include "../Core/RingBuffer.h"

#include "../../Renderer/Include/IRenderer.h"
#include "../../Renderer/Include/IResourceLoader.h"

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

	static int  fonsImplementationGenerateTexture(void* userPtr, int width, int height);
	static void fonsImplementationModifyTexture(void* userPtr, int* rect, const unsigned char* data);
	static void fonsImplementationRenderText(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
	static void fonsImplementationRemoveTexture(void* userPtr);

	FONScontext* pContext;

	const uint8_t* pPixels;
	Texture* pCurrentTexture;
	bool     mUpdateTexture;

	uint32_t mWidth;
	uint32_t mHeight;
	float2   mScaleBias;

	CameraMatrix mProjView;
	mat4 mWorldMat;
	Cmd* pCmd;

	RootSignature* pRootSignature;
	DescriptorSet* pDescriptorSets;
	Pipeline* pPipelines[2];


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