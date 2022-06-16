//#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IThread.h"
#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

#include <process.h>		//	_beginthreadex

bool initMutex(Mutex* mutex)
{
	return InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION*)&mutex->mHandle, (DWORD)MUTEX_DEFAULT_SPIN_COUNT);
}

void destroyMutex(Mutex* mutex)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)&mutex->mHandle;
	DeleteCriticalSection(cs);
	memset(&mutex->mHandle, 0, sizeof(mutex->mHandle));
}

void acquireMutex(Mutex* mutex) { EnterCriticalSection((CRITICAL_SECTION*)&mutex->mHandle); }

void releaseMutex(Mutex* mutex) { LeaveCriticalSection((CRITICAL_SECTION*)&mutex->mHandle); }

void waitConditionVariable(ConditionVariable* cv, const Mutex* pMutex, uint32_t ms)
{
	SleepConditionVariableCS((PCONDITION_VARIABLE)cv->pHandle, (PCRITICAL_SECTION)&pMutex->mHandle, ms);
}

void wakeOneConditionVariable(ConditionVariable* cv) { WakeConditionVariable((PCONDITION_VARIABLE)cv->pHandle); }

void wakeAllConditionVariable(ConditionVariable* cv) { WakeAllConditionVariable((PCONDITION_VARIABLE)cv->pHandle); }
static ThreadID mainThreadID = 0;

void setMainThread() { mainThreadID = getCurrentThreadID(); }

ThreadID getCurrentThreadID() { return GetCurrentThreadId(); /* Windows built-in function*/ }

char* thread_name()
{
	__declspec(thread) static char name[MAX_THREAD_NAME_LENGTH + 1];
	return name;
}

void getCurrentThreadName(char* buffer, int size)
{
	const char* name = thread_name();
	if (name[0])
		snprintf(buffer, (size_t)size, "%s", name);
	else
		buffer[0] = 0;
}

void setCurrentThreadName(const char* name) { strcpy_s(thread_name(), MAX_THREAD_NAME_LENGTH + 1, name); }