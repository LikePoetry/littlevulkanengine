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

bool initConditionVariable(ConditionVariable* cv)
{
	cv->pHandle = (CONDITION_VARIABLE*)tf_calloc(1, sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable((PCONDITION_VARIABLE)cv->pHandle);
	return true;
}

void destroyConditionVariable(ConditionVariable* cv) { tf_free(cv->pHandle); }

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

unsigned WINAPI ThreadFunctionStatic(void* data) 
{
	ThreadDesc item = *((ThreadDesc*)(data));
	tf_free(data);

	if (item.mThreadName[0] != 0)
	{
		// Local TheForge thread name, used for logging
		setCurrentThreadName(item.mThreadName);

#ifdef _WINDOWS
		// OS Thread name, for debugging purposes
		WCHAR windowsThreadName[sizeof(item.mThreadName)] = { 0 };
		mbstowcs(windowsThreadName, item.mThreadName, strlen(item.mThreadName) + 1);
		HRESULT res = SetThreadDescription(GetCurrentThread(), windowsThreadName);
		ASSERT(!FAILED(res));
#endif
	}

	if (item.mHasAffinityMask)
	{
		const DWORD_PTR res = SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)item.mAffinityMask);
		ASSERT(res != 0);
	}

	item.pFunc(item.pData);
	return 0;
}

void initThread(ThreadDesc* pDesc, ThreadHandle* pHandle)
{
	ASSERT(pHandle != NULL);

	// Copy the contents of ThreadDesc because if the variable is in the stack we might access corrupted data.
	ThreadDesc* pDescCopy = (ThreadDesc*)tf_malloc(sizeof(ThreadDesc));
	*pDescCopy = *pDesc;

	ThreadHandle handle = (ThreadHandle)_beginthreadex(0, 0, ThreadFunctionStatic, pDescCopy, 0, 0);
	ASSERT(handle != NULL);
	*pHandle = handle;
}

void joinThread(ThreadHandle handle)
{
	ASSERT(handle != NULL);
	WaitForSingleObject((HANDLE)handle, INFINITE);
	CloseHandle((HANDLE)handle);
	handle = NULL;
}

void setCurrentThreadName(const char* name) { strcpy_s(thread_name(), MAX_THREAD_NAME_LENGTH + 1, name); }