#pragma once
#include <stdint.h>

# define PTR_SIZE 8

#define FORGE_CONSTEXPR

#define UNREF_PARAM(x) (x)
#define ALIGNAS(x) __declspec( align( x ) ) 
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#define FORGE_CALLCONV __cdecl

#include <crtdbg.h>
#define COMPILE_ASSERT(exp) _STATIC_ASSERT(exp) 

#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

#if defined(_M_X64)
#define ARCH_X64
#define ARCH_X86_FAMILY
#elif defined(_M_IX86)
#define ARCH_X86
#define ARCH_X86_FAMILY
#else
#error "Unsupported architecture for msvc compiler"
#endif

//////////////////////////////////////////////
//// General options
//////////////////////////////////////////////
#define ENABLE_FORGE_SCRIPTING
#define ENABLE_FORGE_UI
#define ENABLE_FORGE_FONTS
#define ENABLE_FORGE_INPUT
#define ENABLE_ZIP_FILESYSTEM
#define ENABLE_SCREENSHOT
#define ENABLE_PROFILER
#define ENABLE_MESHOPTIMIZER


#define API_INTERFACE

//////////////////////////////////////////////
//// Build related options
//////////////////////////////////////////////
#define DEFAULT_LOG_LEVEL eALL