//============================================================
//
//	ticker.c - Win32 timing code
//
//============================================================

// MAME headers
#include "driver.h"
#include "ticker.h"



//============================================================
//	PROTOTYPES
//============================================================

static TICKER init_ticker(void);
static TICKER rdtsc_ticker(void);
static TICKER time_ticker(void);



//============================================================
//	GLOBAL VARIABLES
//============================================================

// global ticker function and divider
TICKER			(*ticker)(void) = init_ticker;
TICKER			ticks_per_sec;

//============================================================
//	init_ticker
//============================================================

static TICKER init_ticker(void)
{
}


static TICKER time_ticker(void)
{
}
