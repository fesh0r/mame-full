/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-99 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  debug.c

  MAME debugging code, using a console

 ***************************************************************************/

#if defined(MAME_DEBUG)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "MAME32.h"
#include "M32Util.h"
#include "osd_dbg.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int  MAMEDebug_Keypressed(void);
static void MAMEDebug_SetForeground(void);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

/***************************************************************************
    External functions  
 ***************************************************************************/

void osd_screen_update(void)
{

}

void osd_put_screen_char(int ch, int attr, int x, int y)
{
    int   written;
    COORD coord;
    
    coord.X = x;
    coord.Y = y;
    
    WriteConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE),
                                &(char)ch, 1, coord, &written);
    FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                (WORD)attr, 1, coord, &written);
}

void osd_set_screen_curpos(int x, int y)
{
    COORD coord;
    
    coord.X = x;
    coord.Y = y;
    
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

int osd_debug_readkey(void)
{
    INPUT_RECORD ir;
    int read;

    while (1)
    {
        MAME32App.ProcessMessages();
        
        if (MAMEDebug_Keypressed())
        {
            if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read))
                break;
            
            if (ir.EventType != KEY_EVENT)
                continue;
            
            if (!ir.Event.KeyEvent.bKeyDown)
                continue;
            
            return (ir.Event.KeyEvent.wVirtualScanCode);
        }
        
    }
    return 0;
}

void osd_set_screen_size(unsigned width, unsigned height)
{
    BOOL    bSuccess;
    COORD   coordScreen;

    coordScreen.X = width;
    coordScreen.Y = height;
    bSuccess = SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coordScreen);
   
    if (!bSuccess)
    {
        ErrorMsg("osd_set_screen_size: Unable to set screen size %dx%d\n", width, height);
        return;
    }

    /*
       osd_set_screen_size is usually called when breaking into
       debug mode. So bring the debug window to the foreground.
    */
    MAMEDebug_SetForeground();
}

void osd_get_screen_size(unsigned *width, unsigned *height)
{
    BOOL    bSuccess;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    bSuccess = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    if (bSuccess)
    {
        *width  = csbi.dwSize.X;
        *height = csbi.dwSize.Y;
    }
    else
    {
        ErrorMsg("osd_get_screen_size: Unable to get screen size\n");
        *width  = 80;
        *height = 25;
    }
}


/***************************************************************************
    Internal functions  
 ***************************************************************************/

static int MAMEDebug_Keypressed(void)
{
    int count;
    
    MAME32App.ProcessMessages();
    
    count = 0;
    if (!GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &count))
        return 0;
    
    return count >= 1;
}

static void MAMEDebug_SetForeground(void)
{
    static HWND m_hWndDebug = NULL;

    if (m_hWndDebug == NULL)
    {
        /* Need a better way to get the console HWND */
        SetConsoleTitle(MAME32NAME " Debugger");
        m_hWndDebug = FindWindow("ConsoleWindowClass", MAME32NAME " Debugger");
    }

    SetForegroundWindow(m_hWndDebug);
}

#endif
