/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __WIN32UI_H__
#define __WIN32UI_H__

#include "driver.h"
#include "options.h"
#include "ScreenShot.h"

extern const struct GameDriver driver_0;

#define DRIVER_ROOT &driver_0

extern struct GameDriver driver_neogeo;

#define DRIVER_NEOGEO &driver_neogeo

enum
{
    TAB_PICKER = 0,
    TAB_DISPLAY,
    TAB_MISC,
    NUM_TABS
};

typedef struct
{
    BOOL in_list;
    BOOL neogeo_clone;
    BOOL neogeo;
} game_data_type;

/* global variables */
extern char *column_names[COLUMN_MAX];

options_type * GetPlayingGameOptions(void);
game_data_type * GetGameData(void);

HWND  GetMainWindow(void);
int   GetNumGames(void);
void  GetRealColumnOrder(int order[]);
HICON LoadIconFromFile(char *iconname);
BOOL  GameUsesTrackball(int game);
void  UpdateScreenShot(void);
void  ResizePickerControls(HWND hWnd);
int   UpdateLoadProgress(const char* name, int current,int total);

// Move The in "The Title (notes)" to "Title, The (notes)"
char *ModifyThe(const char *str);

/* globalized for painting tree control */
extern HBITMAP      hBitmap;
extern HPALETTE     hPALbg;
extern MYBITMAPINFO bmDesc;

#endif
