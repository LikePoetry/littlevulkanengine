#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "../Core/Config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Time related functions
uint32_t getSystemTime(void);

/// Low res OS timer
typedef struct Timer
{
	uint32_t mStartTime;
}Timer;

void		initTimer(Timer* pTimer);
void		resetTimer(Timer* pTimer);
uint32_t	getTimerMSec(Timer* pTimer, bool reset);

#ifdef __cplusplus
}
#endif