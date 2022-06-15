#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

typedef unsigned long ThreadID;
#define INVALID_THREAD_ID 0

#define MAX_THREAD_NAME_LENGTH 31




#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct Mutex
	{
		CRITICAL_SECTION mHandle;
	} Mutex;


#define THREAD_LOCAL __declspec( thread )

#define MUTEX_DEFAULT_SPIN_COUNT 1500

	bool initMutex(Mutex* pMutex);
	void destroyMutex(Mutex* pMutex);

	void acquireMutex(Mutex* pMutex);
	void releaseMutex(Mutex* pMutex);

	typedef struct ConditionVariable
	{
		void* pHandle;
	} ConditionVariable;

	void setMainThread(void);
	void setCurrentThreadName(const char* name);
	ThreadID getCurrentThreadID(void);

	void getCurrentThreadName(char* buffer, int buffer_size);

	typedef void* ThreadHandle;
#ifdef __cplusplus
}    // extern "C"
#endif