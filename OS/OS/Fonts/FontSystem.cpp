#include "../Core/Config.h"

#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/IFont.h"

#define FONTSTASH_IMPLEMENTATION
#include "../../ThirdParty/OpenSource/Fontstash/src/fontstash.h"

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
	static void
		fonsImplementationRenderText(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
	static void fonsImplementationRemoveTexture(void* userPtr);

	FONScontext* pContext;

	const uint8_t* pPixels;
	bool           mUpdateTexture;

	uint32_t mWidth;
	uint32_t mHeight;
	float2   mScaleBias;

	float2         mDpiScale;
	float          mDpiScaleMin;
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
	return true;
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
	
}

void _Impl_FontStash::fonsImplementationRemoveTexture(void* userPtr) { UNREF_PARAM(userPtr); }