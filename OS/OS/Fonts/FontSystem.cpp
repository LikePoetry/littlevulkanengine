#include "../Core/Config.h"

bool platformInitFontSystem()
{
#ifdef ENABLE_FORGE_FONTS

#else
	return true;
#endif // ENABLE_FORGE_FONTS
}