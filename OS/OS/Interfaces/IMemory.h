#pragma once

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

#define tf_malloc(size) tf_malloc_internal(size, __FILE__, __LINE__, __FUNCTION__)
#define tf_calloc(count, size) tf_calloc_internal(count, size, __FILE__, __LINE__, __FUNCTION__)
#define tf_realloc(ptr, size) tf_realloc_internal(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#define tf_free(ptr) tf_free_internal(ptr, __FILE__, __LINE__, __FUNCTION__)