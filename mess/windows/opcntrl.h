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
optreserr_t win_add_resolution_parameter(HWND control, option_resolution *resolution);

#endif // OPCNTRL_H
