//============================================================
//
//	opcntrl.h - Code for handling option resolutions in
//	Win32 controls
//
//============================================================

#ifndef OPCNTRL_H
#define OPCNTRL_H

#include <windows.h>
#include "opresolv.h"

BOOL win_prepare_option_control(HWND control, const struct OptionGuide *guide,
	const char *optspec);

#endif // OPCNTRL_H
