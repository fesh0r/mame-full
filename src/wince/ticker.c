//============================================================
//
//	ticker.c - WinCE timing code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "driver.h"

//============================================================
//	STATIC VARIABLES
//============================================================

static cycles_t suspend_adjustment;
static cycles_t suspend_time;



//============================================================
//	osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return suspend_time ? suspend_time : GetTickCount() - suspend_adjustment;
}



//============================================================
//	osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return 1000;
}



//============================================================
//	win_timer_enable
//============================================================

void win_timer_enable(int enabled)
{
	cycles_t actual_cycles;

	actual_cycles = GetTickCount();
	if (!enabled)
	{
		suspend_time = actual_cycles;
	}
	else if (suspend_time > 0)
	{
		suspend_adjustment += actual_cycles - suspend_time;
		suspend_time = 0;
	}
}
