#pragma once
#include "../Core/Config.h"

#define REGISTER_LUA_WIDGET(x) luaRegisterWidget((x))

/****************************************************************************/
// MARK: - Lua Scripting Data Structs
/****************************************************************************/

typedef struct LuaScriptDesc
{

	const char* pScriptFileName = NULL;
	const char* pScriptFilePassword = NULL;

} LuaScriptDesc;


/// Adds an array of scripts to the Lua interface by filenames
void luaDefineScripts(LuaScriptDesc* pDescs, uint32_t count);

/// Register a Forge UI Widget for modification via a Lua script
void luaRegisterWidget(const void* pWidgetHandle);