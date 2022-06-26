#pragma once
#include "../Renderer/Include/RendererConfig.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IThread.h"

#include "../Math/MathTypes.h"




typedef uint64_t ProfileToken;
#define PROFILE_INVALID_TOKEN (uint64_t) - 1

struct Cmd;
struct Renderer;
struct Queue;
struct FontDrawDesc;
struct UserInterface;


typedef struct ProfilerDesc
{

	Renderer* pRenderer = NULL;
	Queue** ppQueues = NULL;

	const char** ppProfilerNames = NULL;
	ProfileToken* pProfileTokens = NULL;

	uint32_t      mGpuProfilerCount = 0;
	uint32_t      mWidthUI = 0;
	uint32_t      mHeightUI = 0;

} ProfilerDesc;

// Must be called before adding any profiling
void initProfiler(ProfilerDesc* pDesc);

// Call only after initProfiler(), for manually adding Gpu Profilers
ProfileToken addGpuProfiler(Renderer* pRenderer, Queue* pQueue, const char* pProfilerName);