//============================================================
//
//	wimgtool.h - Win32 GUI Imgtool
//
//============================================================

#ifndef WIMGTOOL_H
#define WIMGTOOL_H

#include <windows.h>
#include "../imgtool.h"

extern const TCHAR wimgtool_class[];
extern imgtool_library *library;

BOOL wimgtool_registerclass(void);


#endif // WIMGTOOL_H
