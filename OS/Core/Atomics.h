#pragma once
#include "Config.h"

typedef volatile ALIGNAS(4) uint32_t tfrg_atomic32_t;
typedef volatile ALIGNAS(8) uint64_t tfrg_atomic64_t;
typedef volatile ALIGNAS(PTR_SIZE) uintptr_t tfrg_atomicptr_t;