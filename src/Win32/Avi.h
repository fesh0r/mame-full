/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __AVI_H__
#define __AVI_H__

BOOL    AviStartCapture(char* filename, int width, int height, int depth);
void    AviEndCapture( void );

void    AviAddBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries);

void    SetAviFPS(int fps);
BOOL    GetAviCapture( void );

#endif