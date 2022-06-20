#pragma once
#include "../Core/Config.h"

#define REGISTER_LUA_WIDGET(x) luaRegisterWidget((x))



/// Register a Forge UI Widget for modification via a Lua script
void luaRegisterWidget(const void* pWidgetHandle);