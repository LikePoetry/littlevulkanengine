#pragma once

#include "../Core/Config.h"

#ifdef __cplusplus
#include <new>
#include "../../ThirdParty/OpenSource/EASTL/utility.h"
#else
#include <stdint.h>
#endif
#ifdef __cplusplus
extern "C"
{
#endif

	bool initMemAlloc(const char* appName);

	void* tf_malloc_internal(size_t size, const char* f, int l, const char* sf);
	void* tf_memalign_internal(size_t align, size_t size, const char* f, int l, const char* sf);
	void* tf_calloc_internal(size_t count, size_t size, const char* f, int l, const char* sf);
	void* tf_calloc_memalign_internal(size_t count, size_t align, size_t size, const char* f, int l, const char* sf);
	void* tf_realloc_internal(void* ptr, size_t size, const char* f, int l, const char* sf);
	void  tf_free_internal(void* ptr, const char* f, int l, const char* sf);

#ifdef __cplusplus
}    // extern "C"
#endif

#ifdef __cplusplus
template <typename T, typename... Args>
static T* tf_placement_new(void* ptr, Args&&... args)
{
	return new (ptr) T(eastl::forward<Args>(args)...);
}

template <typename T, typename... Args>
static T* tf_new_internal(const char* f, int l, const char* sf, Args&&... args)
{
	T* ptr = (T*)tf_memalign_internal(alignof(T), sizeof(T), f, l, sf);
	return tf_placement_new<T>(ptr, eastl::forward<Args>(args)...);
}

template <typename T>
static void tf_delete_internal(T* ptr, const char* f, int l, const char* sf)
{
	if (ptr)
	{
		ptr->~T();
		tf_free_internal(ptr, f, l, sf);
	}
}
#endif

#ifndef tf_malloc
#define tf_malloc(size) tf_malloc_internal(size, __FILE__, __LINE__, __FUNCTION__)
#endif
#ifndef tf_memalign
#define tf_memalign(align, size) tf_memalign_internal(align, size, __FILE__, __LINE__, __FUNCTION__)
#endif
#ifndef tf_calloc
#define tf_calloc(count, size) tf_calloc_internal(count, size, __FILE__, __LINE__, __FUNCTION__)
#endif
#ifndef tf_calloc_memalign
#define tf_calloc_memalign(count, align, size) tf_calloc_memalign_internal(count, align, size, __FILE__, __LINE__, __FUNCTION__)
#endif
#ifndef tf_realloc
#define tf_realloc(ptr, size) tf_realloc_internal(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#endif
#ifndef tf_free
#define tf_free(ptr) tf_free_internal(ptr, __FILE__, __LINE__, __FUNCTION__)
#endif

#ifdef __cplusplus
#ifndef tf_new
#define tf_new(ObjectType, ...) tf_new_internal<ObjectType>(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif
#ifndef tf_delete
#define tf_delete(ptr) tf_delete_internal(ptr, __FILE__, __LINE__, __FUNCTION__)
#endif
#endif