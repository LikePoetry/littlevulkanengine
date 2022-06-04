#pragma once
#include "../Core/Config.h"
#include "../Interfaces/IThread.h"
#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

#include <process.h>		//	_beginthreadex


void acquireMutex(Mutex* mutex) { EnterCriticalSection((CRITICAL_SECTION*)&mutex->mHandle); }

void releaseMutex(Mutex* mutex) { LeaveCriticalSection((CRITICAL_SECTION*)&mutex->mHandle); }

char* thread_name()
{
	__declspec(thread) static char name[MAX_THREAD_NAME_LENGTH + 1];
	return name;
}

void getCurrentThreadName(char* buffer,int size)
{
	const char* name = thread_name();
	if (name[0])
		snprintf(buffer, (size_t)size, "%s", name);
	else
		buffer[0] = 0;
}