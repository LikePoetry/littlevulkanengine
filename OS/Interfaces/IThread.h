#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IOperatingSystem.h"

typedef unsigned long ThreadID;
#define INVALID_THREAD_ID 0

#define MAX_THREAD_NAME_LENGTH 31
#define TIMEOUT_INFINITE UINT32_MAX



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

	void waitConditionVariable(ConditionVariable* cv, const Mutex* pMutex, uint32_t timeout);
	void wakeOneConditionVariable(ConditionVariable* cv);
	void wakeAllConditionVariable(ConditionVariable* cv);

	void setMainThread(void);
	void setCurrentThreadName(const char* name);
	ThreadID getCurrentThreadID(void);

	void getCurrentThreadName(char* buffer, int buffer_size);

	typedef void* ThreadHandle;
#ifdef __cplusplus
}    // extern "C"

struct MutexLock
{
	MutexLock(Mutex& rhs) : mMutex(rhs) { acquireMutex(&rhs); }
	~MutexLock() { releaseMutex(&mMutex); }

	/// Prevent copy construction.
	MutexLock(const MutexLock& rhs) = delete;
	/// Prevent assignment.
	MutexLock& operator=(const MutexLock& rhs) = delete;

	Mutex& mMutex;
};
#endif