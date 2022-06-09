#include "../Core/Config.h"
// Don't compile stbtt unless we need it for UI or fonts
#if defined ENABLE_FORGE_UI || defined ENABLE_FORGE_FONTS

#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

#ifndef STB_RECT_PACK_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT(x)     ASSERT(x)
#include "../../ThirdParty/OpenSource/Nothings/stb_rect_pack.h"
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x)     ASSERT(x)

#include "../../ThirdParty/OpenSource/Nothings/stb_truetype.h"

#endif // UI OR FONTS

#endif // UI OR FONTS