/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __MAME32UTIL_H__
#define __MAME32UTIL_H__

#define RECT_WIDTH(r)   (r.right  - r.left)
#define RECT_HEIGHT(r)  (r.bottom - r.top)

extern BOOL     bErrorMsgBox;

extern void __cdecl ErrorMsg(const char* fmt, ...);

extern void FontDecodeString(char *string, LOGFONT *f);
extern void FontEncodeString(LOGFONT *f, char *string);

extern UINT GetDepth(HWND hWnd);

extern BOOL OnNT(void);

/* Open a text file */
void    DisplayTextFile(HWND hWnd, char *cName);

/* Check for old version of comctl32.dll */
BOOL    GetDllVersion(void);


#endif
