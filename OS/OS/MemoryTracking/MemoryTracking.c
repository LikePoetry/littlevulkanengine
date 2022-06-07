#pragma once
#include "../Core/Config.h"

#include "wchar.h"

#include "../../ThirdParty/OpenSource/EASTL/EABase/eabase.h"
#include "../../ThirdParty/OpenSource/ModifiedSonyMath/vectormath_settings.hpp"

#include <stdlib.h>
#include <memory.h>

#define MEM_MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_TO(size, alignment) (((size) + (alignment)-1) & ~((alignment)-1))
#define MIN_ALLOC_ALIGNMENT MEM_MAX(VECTORMATH_MIN_ALIGN, EA_PLATFORM_MIN_MALLOC_ALIGNMENT)

#define MTUNER_ALIGNED_ALLOC(_handle, _ptr, _size, _overhead, _align)
#define MTUNER_REALLOC(_handle, _ptr, _size, _overhead, _prevPtr)
#define MTUNER_FREE(_handle, _ptr)

// Just include the cpp here so we don't have to add it to the all projects
#include "../../ThirdParty/OpenSource/FluidStudios/MemoryManager/mmgr.c"

#define _CRT_SECURE_NO_WARNINGS 1

void* tf_malloc_internal(size_t size, const char* f, int l, const char* sf)
{
	void* pMemAlign = mmgrAllocator(f, l, sf, m_alloc_malloc, MIN_ALLOC_ALIGNMENT, size);

	// If using MTuner, report allocation to rmem.
	MTUNER_ALIGNED_ALLOC(0, pMemAlign, size, 0, align);

	// Return handle to allocated memory.
	return pMemAlign;

	/*return tf_memalign_internal(MIN_ALLOC_ALIGNMENT, size, f, l, sf);*/
}

void* tf_memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf)
{
	void* pMemAlign = mmgrAllocator(f, l, sf, m_alloc_malloc, align, size);

	// If using MTuner, report allocation to rmem.
	MTUNER_ALIGNED_ALLOC(0, pMemAlign, size, 0, align);

	// Return handle to allocated memory.
	return pMemAlign;
}

void* tf_realloc_internal(void* ptr, size_t size, const char* f, int l, const char* sf)
{
	void* pRealloc = mmgrReallocator(f, l, sf, m_alloc_realloc, size, ptr);

	// If using MTuner, report reallocation to rmem.
	MTUNER_REALLOC(0, pRealloc, size, 0, ptr);

	// Return handle to reallocated memory.
	return pRealloc;
}

void tf_free_internal(void* ptr, const char* f, int l, const char* sf)
{
	// If using MTuner, report free to rmem.
	MTUNER_FREE(0, ptr);

	mmgrDeallocator(f, l, sf, m_alloc_free, ptr);
}