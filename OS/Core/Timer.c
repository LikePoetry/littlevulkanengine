#include "../Interfaces/ITime.h"

void initTimer(Timer* pTimer) { resetTimer(pTimer); }

unsigned getTimerMSec(Timer* pTimer, bool reset)
{
	unsigned currentTime = getSystemTime();
	unsigned elapsedTime = currentTime - pTimer->mStartTime;
	if (reset)
		pTimer->mStartTime = currentTime;

	return elapsedTime;
}

void resetTimer(Timer* pTimer) { pTimer->mStartTime = getSystemTime(); }