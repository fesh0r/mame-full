/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

enum
{
    FORMAT_PNG = 0,
    FORMAT_BMP,
    FORMAT_UNKNOWN,
    FORMAT_MAX = FORMAT_UNKNOWN
};

/* Located in ScreenShot.c */
extern char pic_format[FORMAT_MAX][4];

typedef struct _mybitmapinfo {
    int bmWidth;
    int bmHeight;
    int bmColors;
} MYBITMAPINFO, *LPMYBITMAPINFO;

BOOL LoadScreenShot(int nGame, int nType);
BOOL DrawScreenShot(HWND hWnd);
void FreeScreenShot(void);
BOOL GetScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict);
BOOL ScreenShotLoaded(void);

BOOL    LoadDIB(LPCTSTR filename, HGLOBAL *phDIB, HPALETTE *pPal, BOOL flyer);
BOOL    DrawDIB(HWND hWnd, HDC hDC, HGLOBAL hDIB, HPALETTE hPal);
HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc);

#endif
