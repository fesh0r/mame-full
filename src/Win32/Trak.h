/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __TRAK_H__
#define __TRAK_H__

#include "osdepend.h"

#include "joystick.h" /* cmk temp */

#define OSD_TRAK_LEFT   OSD_JOY_LEFT
#define OSD_TRAK_RIGHT  OSD_JOY_RIGHT
#define OSD_TRAK_UP     OSD_JOY_UP
#define OSD_TRAK_DOWN   OSD_JOY_DOWN
#define OSD_TRAK_FIRE1  OSD_JOY_FIRE1
#define OSD_TRAK_FIRE2  OSD_JOY_FIRE2
#define OSD_TRAK_FIRE3  OSD_JOY_FIRE3
#define OSD_TRAK_FIRE4  OSD_JOY_FIRE4
#define OSD_TRAK_FIRE   OSD_JOY_FIRE    /* any of the fire buttons */
#define OSD_MAX_TRAK    OSD_MAX_JOY

#define TRAK_MAXX_RES   120
#define TRAK_MAXY_RES   120

struct OSDTrak
{
    int         (*init)(options_type *options);
    void        (*exit)(void);
    void        (*trak_read)(int player, int *deltax, int *deltay);
    int         (*trak_pressed)(int trakcode);
    
    BOOL        (*OnMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

extern struct OSDTrak Trak;

#endif
