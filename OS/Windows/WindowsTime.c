#pragma once


#include "../Core/Config.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/ITime.h"
#include "../Interfaces/IThread.h"

#include <time.h>
#include <stdint.h>
#include <windows.h>
#include <timeapi.h>
/************************************************************************/
// Time Related Functions
/************************************************************************/
uint32_t getSystemTime() { return (uint32_t)timeGetTime(); }