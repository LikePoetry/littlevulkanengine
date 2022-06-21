#include "../Interfaces/IScripting.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IUI.h"

// RENDERER
#include "../../Renderer/Include/IRenderer.h"

#include "../../ThirdParty/OpenSource/EASTL/list.h"

// PREPROCESSOR DEFINES
#define MAX_LUA_STR_LEN 256
#define MAX_NUM_SCRIPTS  16

typedef struct ScriptInfo
{
	char* pFileName;
	char* pFilePassword;
}ScriptInfo;



static ScriptInfo* pTestScripts = NULL;
static uint32_t    mTestScriptCount = 0;




static void strErase(char* str, size_t& strSize, size_t pos)
{
	ASSERT(str);
	ASSERT(strSize);
	ASSERT(pos < strSize);

	if (pos == strSize - 1)
		str[pos] = 0;
	else
		memmove(str + pos, str + pos + 1, strSize - pos);

	--strSize;
}

static void TrimString(char* str)
{
	size_t size = strlen(str);

	if (isdigit(str[0]))
		strErase(str, size, 0);
	for (uint32_t i = 0; i < size; ++i)
	{
		if (isspace(str[i]) || (!isalnum(str[i]) && str[i] != '_'))
			strErase(str, size, i--);
	}
}

void luaDefineScripts(LuaScriptDesc* pDescs, uint32_t count)
{
#ifdef ENABLE_FORGE_SCRIPTING
	ASSERT(mTestScriptCount + count < MAX_NUM_SCRIPTS);

	for (uint32_t i = 0; i < count; ++i)
	{
		if (pDescs[i].pScriptFileName)
			strcpy(pTestScripts[mTestScriptCount].pFileName, pDescs[i].pScriptFileName);

		if (pDescs[i].pScriptFilePassword)
			strcpy(pTestScripts[mTestScriptCount].pFilePassword, pDescs[i].pScriptFilePassword);

		++mTestScriptCount;
	}
#else
	LOGF(LogLevel::eWARNING, "Attempting to use Forge Lua Scripting without define!");
	LOGF(LogLevel::eWARNING, "Make sure to define 'ENABLE_FORGE_SCRIPTING' for Scripting to work!");
#endif
}


void luaRegisterWidget(const void* pWidgetHandle)
{
	ASSERT(pWidgetHandle);
	const UIWidget* pWidget = (const UIWidget*)pWidgetHandle;

	typedef eastl::pair<char*, WidgetCallback> NamePtrPair;
	eastl::vector<NamePtrPair> functionsList;
	char functionName[MAX_LABEL_STR_LENGTH];

	TrimString(functionName);

}