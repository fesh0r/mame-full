//============================================================
//
//	debugwin.h - Win32 debug window handling
//
//============================================================

#ifndef __WIN_DEBUGWIN__
#define __WIN_DEBUGWIN__


//============================================================
//	GLOBAL VARIABLES
//============================================================

// windows



//============================================================
//	PROTOTYPES
//============================================================

int debugwin_init_windows(void);
void debugwin_show(int type);
int debugwin_is_debugger_visible(void);

#endif
