/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse
  
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Debug.c

  MAME debugging code

 ***************************************************************************/

#if defined(MAME_DEBUG)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "MAME32.h"
#include "debug.h"
#include "resource.h"
#include "M32Util.h"

#define MAKECOL(r, g, b) ((((r & 0x000000F8) << 7)) | \
                          (((g & 0x000000F8) << 2)) | \
                          (((b & 0x000000F8) >> 3)))

/***************************************************************************
    function prototypes
 ***************************************************************************/

static LRESULT CALLBACK MAME32Debug_MessageProc(HWND, UINT, WPARAM, LPARAM);

static HWND MAME32Debug_CreateWindow(HWND hWndParent);
static void OnPaint(HWND hWnd);
static void OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange);
static BOOL OnQueryNewPalette(HWND hWnd);
static void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo);
static void OnClose(HWND hWnd);
static void OnSysCommand(HWND hWnd, UINT cmd, int x, int y);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tDisplay_private
{
    struct osd_bitmap*  m_pBitmap;
    BITMAPINFO*         m_pInfo;
    HWND                m_hWnd;

    int                 m_nClientWidth;
    int                 m_nClientHeight;
    RECT                m_ClientRect;
    int                 m_nWindowWidth;
    int                 m_nWindowHeight;

    int                 m_nDepth;
    BOOL                m_bUpdatePalette;
    HPALETTE            m_hPalette;
    PALETTEENTRY        m_PalEntries[DEBUGGER_TOTAL_COLORS];
    UINT32              m_nTotalColors;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDisplay_private This;
static const int               safety = 16;

/***************************************************************************
    External functions  
 ***************************************************************************/

int MAME32Debug_init(options_type* pOptions)
{
    memset(&This, 0, sizeof(struct tDisplay_private));

    return 0;
}

void MAME32Debug_exit(void)
{

}

int MAME32Debug_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    int     i;
    RECT    Rect;
    struct osd_bitmap* pBmp;

    if (!mame_debug)
        return 0;

    This.m_nClientWidth  = width;
    This.m_nClientHeight = height;

    This.m_nDepth = depth;
    /* Both 15 and 16 bit are 555 */
    if (This.m_nDepth == 15)
        This.m_nDepth = 16;

    pBmp = osd_alloc_bitmap(width, height, This.m_nDepth);

    /* Create BitmapInfo */
    This.m_pInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);

    This.m_pInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER); 
    This.m_pInfo->bmiHeader.biWidth         =  (pBmp->line[1] - pBmp->line[0]) / (This.m_nDepth / 8);
    This.m_pInfo->bmiHeader.biHeight        = -(int)(This.m_nClientHeight); /* Negative means "top down" */
    This.m_pInfo->bmiHeader.biPlanes        = 1;
    This.m_pInfo->bmiHeader.biBitCount      = This.m_nDepth;
    This.m_pInfo->bmiHeader.biCompression   = BI_RGB;
    This.m_pInfo->bmiHeader.biSizeImage     = 0;
    This.m_pInfo->bmiHeader.biXPelsPerMeter = 0;
    This.m_pInfo->bmiHeader.biYPelsPerMeter = 0;
    This.m_pInfo->bmiHeader.biClrUsed       = 0;
    This.m_pInfo->bmiHeader.biClrImportant  = 0;

    osd_free_bitmap(pBmp);

    /* Palette */
    if (This.m_nDepth == 8)
    {
        LOGPALETTE LogPalette;

        LogPalette.palVersion    = 0x0300;
        LogPalette.palNumEntries = 1;
        This.m_hPalette = CreatePalette(&LogPalette);
        ResizePalette(This.m_hPalette, 256);

        /* Map image values to palette index */
        for (i = 0; i < 256; i++)
            ((WORD*)(This.m_pInfo->bmiColors))[i] = i;
    }

    This.m_ClientRect.left   = 0;
    This.m_ClientRect.top    = 0;
    This.m_ClientRect.right  = This.m_nClientWidth;
    This.m_ClientRect.bottom = This.m_nClientHeight;

    /* Calculate size of window based on desired client area. */
    Rect.left   = This.m_ClientRect.left;
    Rect.top    = This.m_ClientRect.top;
    Rect.right  = This.m_ClientRect.right;
    Rect.bottom = This.m_ClientRect.bottom;

    This.m_hWnd = MAME32Debug_CreateWindow(MAME32App.m_hWnd);

    AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW | WS_BORDER, FALSE);

    This.m_nWindowWidth  = RECT_WIDTH(Rect);
    This.m_nWindowHeight = RECT_HEIGHT(Rect);

    GetWindowRect(MAME32App.m_hWnd, &Rect);
    SetWindowPos(This.m_hWnd,
                 HWND_NOTOPMOST,
                 Rect.right, Rect.top,
                 This.m_nWindowWidth,
                 This.m_nWindowHeight,
                 0);

    ShowWindow(This.m_hWnd, SW_SHOW);

    return 0;
}

void MAME32Debug_close_display(void)
{ 
    if (!mame_debug)
        return;

    if (This.m_nDepth == 8)
    {
        if (This.m_hPalette != NULL)
        {
            DeletePalette(This.m_hPalette);
            This.m_hPalette = NULL;
        }
    }

    if (This.m_pInfo != NULL)
    {
        free(This.m_pInfo);
        This.m_pInfo = NULL;
    }
}

void MAME32Debug_update_display(struct osd_bitmap *debug_bitmap)
{
    if (!mame_debug)
        return;

    This.m_pBitmap = debug_bitmap;

    InvalidateRect(This.m_hWnd, &This.m_ClientRect, FALSE);

    UpdateWindow(This.m_hWnd);
}

int MAME32Debug_allocate_colors(int          modifiable,
                                const UINT8* debug_palette,
                                UINT32*      debug_pens)
{
    int i;

    if (!mame_debug)
        return 0;

    if (!debug_pens)
        return 1;

    if (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
    {
        for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
        {
            if (This.m_nDepth == 16) /* 555 */
            {
                *debug_pens++ = debug_palette[3 * i + 0] * 0x7c00 / 0xff 
                              + debug_palette[3 * i + 1] * 0x03e0 / 0xff
                              + debug_palette[3 * i + 2] * 0x001f / 0xff;
            }
            else
            if (This.m_nDepth == 32)
            {
                *debug_pens++ = debug_palette[3 * i + 0] * 0x7c00 / 0xff 
                              + debug_palette[3 * i + 1] * 0x03e0 / 0xff
                              + debug_palette[3 * i + 2] * 0x001f / 0xff;
            }
        }
    }
    else
    {
        if (This.m_nDepth == 8)
        {
            for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
            {
                *debug_pens++ = i;
                This.m_PalEntries[i].peRed    = debug_palette[3 * i + 0];
                This.m_PalEntries[i].peGreen  = debug_palette[3 * i + 1];
                This.m_PalEntries[i].peBlue   = debug_palette[3 * i + 2];
                This.m_PalEntries[i].peFlags  = PC_NOCOLLAPSE;
            }

            SetPaletteEntries(This.m_hPalette, 0, DEBUGGER_TOTAL_COLORS, This.m_PalEntries);
        }
        else
        if (This.m_nDepth == 16)
        {
            for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
            {
                *debug_pens++ = MAKECOL(debug_palette[3 * i + 0],
                                        debug_palette[3 * i + 1],
                                        debug_palette[3 * i + 2]);
            }
        }
    }

    return 0;
}

void MAME32Debug_set_debugger_focus(int debugger_has_focus)
{
    if (!mame_debug)
        return;

    if (debugger_has_focus)
    {
        SetForegroundWindow(This.m_hWnd);
    }
}

/***************************************************************************
    Internal functions  
 ***************************************************************************/

static HWND MAME32Debug_CreateWindow(HWND hWndParent)
{
    static BOOL     bRegistered = FALSE;
    HINSTANCE       hInstance = GetModuleHandle(NULL);

    if (bRegistered == FALSE)
    {
        WNDCLASS WndClass;

        WndClass.style          = CS_SAVEBITS | CS_BYTEALIGNCLIENT | CS_OWNDC;
        WndClass.lpfnWndProc    = MAME32Debug_MessageProc;
        WndClass.cbClsExtra     = 0;
        WndClass.cbWndExtra     = 0;
        WndClass.hInstance      = hInstance;
        WndClass.hIcon          = LoadIcon(hInstance, MAKEINTATOM(IDI_MAME32_ICON));
        WndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        WndClass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
        WndClass.lpszMenuName   = NULL;
        WndClass.lpszClassName  = (LPCSTR)"classDebugger";
        
        if (RegisterClass(&WndClass) == 0)
            return NULL;
        bRegistered = TRUE;
    }

    This.m_hWnd = CreateWindowEx(0,
                                 "classDebugger",
                                 MAME32NAME " Debugger",
                                 WS_OVERLAPPEDWINDOW | WS_BORDER,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 0, 0,
                                 hWndParent,
                                 NULL,
                                 hInstance,
                                 NULL);

    return This.m_hWnd;
}

static LRESULT CALLBACK MAME32Debug_MessageProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        HANDLE_MSG(hWnd, WM_PAINT,              OnPaint);
        HANDLE_MSG(hWnd, WM_PALETTECHANGED,     OnPaletteChanged);
        HANDLE_MSG(hWnd, WM_QUERYNEWPALETTE,    OnQueryNewPalette);
        HANDLE_MSG(hWnd, WM_GETMINMAXINFO,      OnGetMinMaxInfo);
        HANDLE_MSG(hWnd, WM_CLOSE,              OnClose);
        HANDLE_MSG(hWnd, WM_SYSCOMMAND,         OnSysCommand);
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT     ps;

    BeginPaint(hWnd, &ps);

    if (This.m_pBitmap == NULL)
    {
        EndPaint(hWnd, &ps); 
        return;
    }

    if (This.m_nDepth == 8)
    {
        /* 8 bit uses windows palette. */
        HPALETTE hOldPalette;

        hOldPalette = SelectPalette(ps.hdc, This.m_hPalette, FALSE);
        RealizePalette(ps.hdc);

        StretchDIBits(ps.hdc,
                      0, 0,
                      This.m_nClientWidth,
                      This.m_nClientHeight,
                      0,
                      0,
                      This.m_nClientWidth,
                      This.m_nClientHeight,
                      This.m_pBitmap->line[0],
                      This.m_pInfo,
                      DIB_PAL_COLORS,
                      SRCCOPY);

        if (hOldPalette != NULL)
            SelectPalette(ps.hdc, hOldPalette, FALSE);
    }
    else
    if (This.m_nDepth == 16)
    {
        StretchDIBits(ps.hdc,
                      0, 0,
                      This.m_nClientWidth,
                      This.m_nClientHeight,
                      0,
                      0,
                      This.m_nClientWidth,
                      This.m_nClientHeight,
                      This.m_pBitmap->line[0],
                      This.m_pInfo,
                      DIB_RGB_COLORS,
                      SRCCOPY);
    }

    EndPaint(hWnd, &ps); 
}

static void OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange)
{
    if (hWnd == hWndPaletteChange) 
        return; 
    
    OnQueryNewPalette(hWnd);
}

static BOOL OnQueryNewPalette(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, TRUE); 
    UpdateWindow(hWnd); 
    return TRUE;
}

static void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo)
{
    pMinMaxInfo->ptMaxSize.x      = This.m_nWindowWidth;
    pMinMaxInfo->ptMaxSize.y      = This.m_nWindowHeight;
    pMinMaxInfo->ptMaxTrackSize.x = This.m_nWindowWidth;
    pMinMaxInfo->ptMaxTrackSize.y = This.m_nWindowHeight;
    pMinMaxInfo->ptMinTrackSize.x = This.m_nWindowWidth;
    pMinMaxInfo->ptMinTrackSize.y = This.m_nWindowHeight;
}

static void OnClose(HWND hWnd)
{
    SendMessage(MAME32App.m_hWnd, WM_CLOSE, 0, 0);
}

static void OnSysCommand(HWND hWnd, UINT cmd, int x, int y)
{
    if (cmd != SC_KEYMENU)
        FORWARD_WM_SYSCOMMAND(hWnd, cmd, x, y, DefWindowProc);
}

#endif
