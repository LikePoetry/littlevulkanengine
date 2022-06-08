#include "../Core/Config.h"

#include "../Interfaces/IMemory.h"

class _Impl_FontStash
{

};


// FONTSTASH
float                  m_fFontMaxSize;
int32_t                mWidth;
int32_t                mHeight;
_Impl_FontStash* impl;

bool platformInitFontSystem()
{
#ifdef ENABLE_FORGE_FONTS
	impl = tf_placement_new<_Impl_FontStash>(tf_calloc(1, sizeof(_Impl_FontStash)));
	{
		float dpiScale[2];
		getDpiScale(dpiScale);
	}
#else
	return true;
#endif // ENABLE_FORGE_FONTS
}