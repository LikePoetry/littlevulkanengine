#pragma once
#define MAX_THREAD_NAME_LENGTH 31

#define INVALID_THREAD_ID 0

void acquireMutex(Mutex* pMutex);
void releaseMutex(Mutex* pMutex);

#define THREAD_LOCAL __declspec( thread )
void			getCurrentThreadName(char* buffer, int buffer_size);

typedef struct Mutex 
{
	CRITICAL_SECTION mHandle;
} Mutex;