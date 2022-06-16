#pragma once
#include "Config.h"

typedef volatile ALIGNAS(4) uint32_t tfrg_atomic32_t;
typedef volatile ALIGNAS(8) uint64_t tfrg_atomic64_t;
typedef volatile ALIGNAS(PTR_SIZE) uintptr_t tfrg_atomicptr_t;

#include <windows.h>
#include <intrin.h>

#define tfrg_memorybarrier_release() _ReadWriteBarrier()

#define tfrg_atomic64_load_relaxed(pVar) (*(pVar))
#define tfrg_atomic64_store_relaxed(dst, val) InterlockedExchange64( (volatile LONG64*)(dst), val )
#define tfrg_atomic64_add_relaxed(dst, val) InterlockedExchangeAdd64( (volatile LONG64*)(dst), (val) )

static inline uint64_t tfrg_atomic64_store_release(tfrg_atomic64_t* pVar, uint64_t val)
{
	tfrg_memorybarrier_release();
	return tfrg_atomic64_store_relaxed(pVar, val);
}