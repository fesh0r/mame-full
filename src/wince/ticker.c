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
//	PROTOTYPES
//============================================================

static cycles_t init_cycle_counter(void);
#ifdef _X86_
static cycles_t rdtsc_cycle_counter(void);
#endif // _X86_
static cycles_t tickcount_cycle_counter(void);
static cycles_t nop_cycle_counter(void);

//============================================================
//	STATIC VARIABLES
//============================================================

// global cycle_counter function and divider
static cycles_t	(*cycle_counter)(void) = init_cycle_counter;
static cycles_t	(*ticks_counter)(void) = init_cycle_counter;
static cycles_t	cycles_per_sec;
static cycles_t suspend_adjustment;
static cycles_t suspend_time;



//============================================================
//	init_cycle_counter
//============================================================

#ifdef _X86_
static int has_rdtsc(void)
{
	int nFeatures = 0;

	__asm {

		mov eax, 1
		cpuid
		mov nFeatures, edx
	}

	return ((nFeatures & 0x10) == 0x10) ? TRUE : FALSE;
}
#endif

static cycles_t init_cycle_counter(void)
{
	cycles_t start, end;
	DWORD a, b;
	int priority;

	suspend_adjustment = 0;
	suspend_time = 0;

#ifdef _X86_
	if (has_rdtsc())
	{
		// if the RDTSC instruction is available use it because
		// it is more precise and has less overhead than timeGetTime()
		cycle_counter = rdtsc_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
		logerror("using RDTSC for timing ... ");
	}
	else
#endif // _X86_
	{
		cycle_counter = tickcount_cycle_counter;
		ticks_counter = tickcount_cycle_counter;
		logerror("using GetTickCount for timing ... ");
	}

	// temporarily set our priority higher
	priority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// wait for an edge on the timeGetTime call
	a = GetTickCount();
	do
	{
		b = GetTickCount();
	} while (a == b);

	// get the starting cycle count
	start = (*cycle_counter)();

	// now wait for 1/4 second total
	do
	{
		a = GetTickCount();
	} while (a - b < 250);

	// get the ending cycle count
	end = (*cycle_counter)();

	// compute ticks_per_sec
	cycles_per_sec = (end - start) * 4;

	// reduce our priority
	SetThreadPriority(GetCurrentThread(), priority);

	// log the results
	logerror("cycles/second = %d\n", (int)cycles_per_sec);

	// return the current cycle count
	return (*cycle_counter)();
}



//============================================================
//	rdtsc_cycle_counter
//============================================================

#ifdef _X86_
static cycles_t rdtsc_cycle_counter(void)
{
	INT64 result;
	INT64 *presult = &result;

	__asm {

		rdtsc
		mov ebx, presult
		mov [ebx],eax
		mov [ebx+4],edx
	}

	return result;
}
#endif


//============================================================
//	tickcount_cycle_counter
//============================================================

static cycles_t tickcount_cycle_counter(void)
{
	// use GetTickCount
	return (cycles_t)GetTickCount();
}



//============================================================
//	nop_cycle_counter
//============================================================

static cycles_t nop_cycle_counter(void)
{
	return 0;
}



//============================================================
//	osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return suspend_time ? suspend_time : (*cycle_counter)() - suspend_adjustment;
}



//============================================================
//	osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return cycles_per_sec;
}



//============================================================
//	osd_profiling_ticks
//============================================================

cycles_t osd_profiling_ticks(void)
{
	return (*ticks_counter)();
}



//============================================================
//	win_timer_enable
//============================================================

void win_timer_enable(int enabled)
{
	cycles_t actual_cycles;

	actual_cycles = (*cycle_counter)();
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
