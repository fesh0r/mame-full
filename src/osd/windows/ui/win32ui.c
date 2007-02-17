/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  win32ui.c

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/
#define MULTISESSION 0

#ifdef _MSC_VER
#ifndef NONAMELESSUNION
#define NONAMELESSUNION 
#endif
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <dlgs.h>
#include <string.h>
#include <sys/stat.h>
#include <wingdi.h>
#include <time.h>
#include <tchar.h>

#include <driver.h>
#include <osdepend.h>
#include <unzip.h>

#include "resource.h"
#include "resource.hm"

#include "screenshot.h"
#include "mame32.h"
#include "M32Util.h"
#include "file.h"
#include "audit32.h"
#include "Directories.h"
#include "Properties.h"
#include "ColumnEdit.h"
#include "picker.h"
#include "tabview.h"
#include "bitmask.h"
#include "TreeView.h"
#include "Splitters.h"
#include "DirWatch.h"
#include "help.h"
#include "history.h"
#include "options.h"
#include "dialogs.h"
#include "state.h"
#include "windows/input.h"
#include "windows/config.h"
#include "windows/window.h"

#include "DirectDraw.h"
#include "DirectInput.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif // LVS_EX_LABELTIP

// fix warning: cast does not match function type
#if defined(__GNUC__) && defined(ListView_CreateDragImage)
#undef ListView_CreateDragImage
#endif

#ifndef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)(LRESULT)(int)SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))
#endif // ListView_CreateDragImage

#ifndef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
    SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))
#endif // TreeView_EditLabel

#ifndef HDF_SORTUP
#define HDF_SORTUP 0x400
#endif // HDF_SORTUP

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x200
#endif // HDF_SORTDOWN

#ifndef LVM_SETBKIMAGEA
#define LVM_SETBKIMAGEA         (LVM_FIRST + 68)
#endif // LVM_SETBKIMAGEA

#ifndef LVM_SETBKIMAGEW
#define LVM_SETBKIMAGEW         (LVM_FIRST + 138)
#endif // LVM_SETBKIMAGEW

#ifndef LVM_GETBKIMAGEA
#define LVM_GETBKIMAGEA         (LVM_FIRST + 69)
#endif // LVM_GETBKIMAGEA

#ifndef LVM_GETBKIMAGEW
#define LVM_GETBKIMAGEW         (LVM_FIRST + 139)
#endif // LVM_GETBKIMAGEW

#ifndef LVBKIMAGE

typedef struct tagLVBKIMAGEA
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEA, *LPLVBKIMAGEA;

typedef struct tagLVBKIMAGEW
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPWSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEW, *LPLVBKIMAGEW;

#ifdef UNICODE
#define LVBKIMAGE               LVBKIMAGEW
#define LPLVBKIMAGE             LPLVBKIMAGEW
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEW
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEW
#else
#define LVBKIMAGE               LVBKIMAGEA
#define LPLVBKIMAGE             LPLVBKIMAGEA
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEA
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEA
#endif
#endif

#ifndef LVBKIF_SOURCE_NONE
#define LVBKIF_SOURCE_NONE      0x00000000
#endif // LVBKIF_SOURCE_NONE

#ifndef LVBKIF_SOURCE_HBITMAP
#define LVBKIF_SOURCE_HBITMAP   0x00000001
#endif

#ifndef LVBKIF_SOURCE_URL
#define LVBKIF_SOURCE_URL       0x00000002
#endif // LVBKIF_SOURCE_URL

#ifndef LVBKIF_SOURCE_MASK
#define LVBKIF_SOURCE_MASK      0x00000003
#endif // LVBKIF_SOURCE_MASK

#ifndef LVBKIF_STYLE_NORMAL
#define LVBKIF_STYLE_NORMAL     0x00000000
#endif // LVBKIF_STYLE_NORMAL

#ifndef LVBKIF_STYLE_TILE
#define LVBKIF_STYLE_TILE       0x00000010
#endif // LVBKIF_STYLE_TILE

#ifndef LVBKIF_STYLE_MASK
#define LVBKIF_STYLE_MASK       0x00000010
#endif // LVBKIF_STYLE_MASK

#ifndef ListView_SetBkImage
#define ListView_SetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_SETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_SetBkImage

#ifndef ListView_GetBkImage
#define ListView_GetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_GETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_GetBkImage

#define MM_PLAY_GAME (WM_APP + 15000)

#define JOYGUI_MS 100

#define JOYGUI_TIMER 1
#define SCREENSHOT_TIMER 2
#define GAMEWND_TIMER 3

/* Max size of a sub-menu */
#define DBU_MIN_WIDTH  292
#define DBU_MIN_HEIGHT 190

int MIN_WIDTH  = DBU_MIN_WIDTH;
int MIN_HEIGHT = DBU_MIN_HEIGHT;

/* Max number of bkground picture files */
#define MAX_BGFILES 100

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif

#define NO_FOLDER -1
#define STATESAVE_VERSION 1

/***************************************************************************
 externally defined global variables
 ***************************************************************************/
extern const ICONDATA g_iconData[];
extern const char g_szPlayGameString[];
extern const char g_szGameCountString[];

/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL             Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);
static void             Win32UI_exit(void);

static BOOL             PumpMessage(void);
static BOOL             OnIdle(HWND hWnd);
static void             OnSize(HWND hwnd, UINT state, int width, int height);
static long WINAPI      MameWindowProc(HWND hwnd,UINT message,UINT wParam,LONG lParam);

static void             SetView(int menu_id);
static void             ResetListView(void);
static void             UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit);
static void             DestroyIcons(void);
static void             ReloadIcons(void);
static void             PollGUIJoystick(void);
static void             PressKey(HWND hwnd,UINT vk);
static BOOL             MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static void             KeyboardKeyDown(int syskey, int vk_code, int special);
static void             KeyboardKeyUp(int syskey, int vk_code, int special);
static void             KeyboardStateClear(void);

static void             UpdateStatusBar(void);
static BOOL             PickerHitTest(HWND hWnd);
static BOOL             TreeViewNotify(NMHDR *nm);

static void             ResetBackground(char *szFile);
static void				RandomSelectBackground(void);
static void             LoadBackgroundBitmap(void);
static void             PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y);

static int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2);
static int              GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem);

static void             DisableSelection(void);
static void             EnableSelection(int nGame);

static int              GetSelectedPick(void);
static HICON			GetSelectedPickItemIcon(void);
static void             SetRandomPickItem(void);
static void				PickColor(COLORREF *cDefault);

static LPTREEFOLDER     GetSelectedFolder(void);
static HICON			GetSelectedFolderIcon(void);

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

static BOOL             SelectLanguageFile(HWND hWnd, TCHAR* filename);
static void             MamePlayRecordGame(void);
static void             MamePlayBackGame(void);
static void             MamePlayRecordWave(void);
static void             MamePlayRecordMNG(void);
static void				MameLoadState(void);
static void             MamePlayGame(void);
static void             MamePlayGameWithOptions(int nGame);
static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static int UpdateLoadProgress(const char* name, const rom_load_data *romdata);
static BOOL GameCheck(void);
static BOOL FolderCheck(void);

static void             ToggleScreenShot(void);
static void             AdjustMetrics(void);
static void             EnablePlayOptions(int nIndex, options_type *o);

/* Icon routines */
static DWORD            GetShellLargeIconSize(void);
static DWORD            GetShellSmallIconSize(void);
static void             CreateIcons(void);
static int              GetIconForDriver(int nItem);
static void             AddDriverIcon(int nItem,int default_icon_index);

// Context Menu handlers
static void             UpdateMenu(HMENU hMenu);
static void InitTreeContextMenu(HMENU hTreeMenu);
static void InitBodyContextMenu(HMENU hBodyContextMenu);
static void ToggleShowFolder(int folder);
static BOOL             HandleTreeContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleScreenShotContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static void				GamePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static void				GamePicker_OnBodyContextMenu(POINT pt);

static void             InitListView(void);
/* Re/initialize the ListView header columns */
static void ResetColumnDisplay(BOOL first_time);

static void             CopyToolTipText (LPTOOLTIPTEXT lpttt);

static void             ProgressBarShow(void);
static void             ProgressBarHide(void);
static void             ResizeProgressBar(void);
static void             ProgressBarStep(void);
static void              ProgressBarStepParam(int iGameIndex, int nGameCount);

static HWND             InitProgressBar(HWND hParent);
static HWND             InitToolbar(HWND hParent);
static HWND             InitStatusBar(HWND hParent);

static LRESULT          Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);

static void             UpdateHistory(void);


void RemoveCurrentGameCustomFolder(void);
void RemoveGameCustomFolder(int driver_index);

void BeginListViewDrag(NM_LISTVIEW *pnmv);
void MouseMoveListViewDrag(POINTS pt);
void ButtonUpListViewDrag(POINTS p);

void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height);

BOOL MouseHasBeenMoved(void);
void SwitchFullScreenMode(void);

// Game Window Communication Functions
#if MULTISESSION
BOOL SendMessageToEmulationWindow(UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL SendIconToEmulationWindow(int nGameIndex);
HWND GetGameWindow(void);
#else
void SendMessageToProcess(LPPROCESS_INFORMATION lpProcessInformation,
						  UINT Msg, WPARAM wParam, LPARAM lParam);
void SendIconToProcess(LPPROCESS_INFORMATION lpProcessInformation, int nGameIndex);
HWND GetGameWindow(LPPROCESS_INFORMATION lpProcessInformation);
#endif

static BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam);
/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	BYTE    bWidth;               /* Width of the image */
	BYTE    bHeight;              /* Height of the image (times 2) */
	BYTE    bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	BYTE    bReserved;            /* Reserved */
	WORD    wPlanes;              /* Color Planes */
	WORD    wBitCount;            /* Bits per pixel */
	DWORD   dwBytesInRes;         /* how many bytes in this resource? */
	DWORD   dwImageOffset;        /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; /* Width, Height and bpp */
	LPBYTE          lpBits;                /* ptr to DIB bits */
	DWORD           dwNumBytes;            /* how many bytes? */
	LPBITMAPINFO    lpbi;                  /* ptr to header */
	LPBYTE          lpXOR;                 /* ptr to XOR image bits */
	LPBYTE          lpAND;                 /* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

/* Which edges of a control are anchored to the corresponding side of the parent window */
#define RA_LEFT     0x01
#define RA_RIGHT    0x02
#define RA_TOP      0x04
#define RA_BOTTOM   0x08
#define RA_ALL      0x0F

#define RA_END  0
#define RA_ID   1
#define RA_HWND 2

typedef struct
{
	int         type;       /* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
							   to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int     id;         /* Window control id */
		HWND    hwnd;       /* Window handle */
	} u;
	BOOL		setfont;	/* Do we set this item's font? */
	int         action;     /* What to do when control is resized */
	void        *subwindow; /* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT        rect;       /* Client rect of window; must be initialized before first resize */
	ResizeItem* items;      /* Array of subitems to be resized */
} Resize;

static void ResizeWindow(HWND hParent, Resize *r);
static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hFont, BOOL bRedraw);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

#define MAX_MENUS 3

#define SPLITTER_WIDTH	4
#define MIN_VIEW_WIDTH	10

// Struct needed for Game Window Communication

typedef struct
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
} FINDWINDOWHANDLE;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND   hMain  = NULL;
static HACCEL hAccel = NULL;

static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;
static HWND hTabCtrl  = NULL;

static BOOL g_bAbortLoading = FALSE; /* doesn't work right */
static BOOL g_bCloseLoading = FALSE;

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     /* Font for list view */

static int game_count = 0;
static int optionfolder_count = 0;

/* global data--know where to send messages */
static BOOL in_emulation;

/* idle work at startup */
static BOOL idle_work;

static int  game_index;
static int  progBarStep;

static BOOL bDoGameCheck = FALSE;

/* Tree control variables */
static BOOL bShowTree      = 1;
static BOOL bShowToolBar   = 1;
static BOOL bShowStatusBar = 1;
static BOOL bShowTabCtrl   = 1;
static BOOL bProgressShown = FALSE;
static BOOL bListReady     = FALSE;

#define	WM_MAME32_FILECHANGED	(WM_USER + 0)
#define	WM_MAME32_AUDITGAME		(WM_USER + 1)

static PDIRWATCHER s_pWatcher;

/* use a joystick subsystem in the gui? */
static struct OSDJoystick* g_pJoyGUI = NULL;

/* store current keyboard state (in internal codes) here */
static input_code keyboard_state[ __code_max ]; /* __code_max #defines the number of internal key_codes */

/* table copied from windows/inputs.c */
// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3

typedef struct
{
	char		name[40];	    // functionality name (optional)
	input_seq	is;				// the input sequence (the keys pressed)
	UINT		func_id;        // the identifier
	input_seq* (*getiniptr)(void);// pointer to function to get the value from .ini file
} GUISequence;

static GUISequence GUISequenceControl[]=
{
	{"gui_key_up",                SEQ_DEF_0,    ID_UI_UP,           Get_ui_key_up },
	{"gui_key_down",              SEQ_DEF_0,    ID_UI_DOWN,         Get_ui_key_down },
	{"gui_key_left",              SEQ_DEF_0,    ID_UI_LEFT,         Get_ui_key_left },
	{"gui_key_right",             SEQ_DEF_0,    ID_UI_RIGHT,        Get_ui_key_right },
	{"gui_key_start",             SEQ_DEF_0,    ID_UI_START,        Get_ui_key_start },
	{"gui_key_pgup",              SEQ_DEF_0,    ID_UI_PGUP,         Get_ui_key_pgup },
	{"gui_key_pgdwn",             SEQ_DEF_0,    ID_UI_PGDOWN,       Get_ui_key_pgdwn },
	{"gui_key_home",              SEQ_DEF_0,    ID_UI_HOME,         Get_ui_key_home },
	{"gui_key_end",               SEQ_DEF_0,    ID_UI_END,          Get_ui_key_end },
	{"gui_key_ss_change",         SEQ_DEF_0,    IDC_SSFRAME,        Get_ui_key_ss_change },
	{"gui_key_history_up",        SEQ_DEF_0,    ID_UI_HISTORY_UP,   Get_ui_key_history_up },
	{"gui_key_history_down",      SEQ_DEF_0,    ID_UI_HISTORY_DOWN, Get_ui_key_history_down },
	
	{"gui_key_context_filters",    SEQ_DEF_0,    ID_CONTEXT_FILTERS,       Get_ui_key_context_filters },
	{"gui_key_select_random",      SEQ_DEF_0,    ID_CONTEXT_SELECT_RANDOM, Get_ui_key_select_random },
	{"gui_key_game_audit",         SEQ_DEF_0,    ID_GAME_AUDIT,            Get_ui_key_game_audit },
	{"gui_key_game_properties",    SEQ_DEF_0,    ID_GAME_PROPERTIES,       Get_ui_key_game_properties },
	{"gui_key_help_contents",      SEQ_DEF_0,    ID_HELP_CONTENTS,         Get_ui_key_help_contents },
	{"gui_key_update_gamelist",    SEQ_DEF_0,    ID_UPDATE_GAMELIST,       Get_ui_key_update_gamelist },
	{"gui_key_view_folders",       SEQ_DEF_0,    ID_VIEW_FOLDERS,          Get_ui_key_view_folders },
	{"gui_key_view_fullscreen",    SEQ_DEF_0,    ID_VIEW_FULLSCREEN,       Get_ui_key_view_fullscreen },
	{"gui_key_view_pagetab",       SEQ_DEF_0,    ID_VIEW_PAGETAB,          Get_ui_key_view_pagetab },
	{"gui_key_view_picture_area",  SEQ_DEF_0,    ID_VIEW_PICTURE_AREA,     Get_ui_key_view_picture_area },
	{"gui_key_view_status",        SEQ_DEF_0,    ID_VIEW_STATUS,           Get_ui_key_view_status },
	{"gui_key_view_toolbars",      SEQ_DEF_0,    ID_VIEW_TOOLBARS,         Get_ui_key_view_toolbars },

	{"gui_key_view_tab_cabinet",     SEQ_DEF_0,  ID_VIEW_TAB_CABINET,       Get_ui_key_view_tab_cabinet },
	{"gui_key_view_tab_cpanel",      SEQ_DEF_0,  ID_VIEW_TAB_CONTROL_PANEL, Get_ui_key_view_tab_cpanel },
	{"gui_key_view_tab_flyer",       SEQ_DEF_0,  ID_VIEW_TAB_FLYER,         Get_ui_key_view_tab_flyer },
	{"gui_key_view_tab_history",     SEQ_DEF_0,  ID_VIEW_TAB_HISTORY,       Get_ui_key_view_tab_history },
	{"gui_key_view_tab_marquee",     SEQ_DEF_0,  ID_VIEW_TAB_MARQUEE,       Get_ui_key_view_tab_marquee },
	{"gui_key_view_tab_screenshot",  SEQ_DEF_0,  ID_VIEW_TAB_SCREENSHOT,    Get_ui_key_view_tab_screenshot },
	{"gui_key_view_tab_title",       SEQ_DEF_0,  ID_VIEW_TAB_TITLE,         Get_ui_key_view_tab_title },
	{"gui_key_quit",                 SEQ_DEF_0,  ID_FILE_EXIT,              Get_ui_key_quit },
};


#define NUM_GUI_SEQUENCES (sizeof(GUISequenceControl) / sizeof(GUISequenceControl[0]))


static UINT    lastColumnClick   = 0;
static WNDPROC g_lpHistoryWndProc = NULL;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

/* Tool and Status bar variables */
static HWND hStatusBar = 0;
static HWND hToolBar   = 0;

/* Column Order as Displayed */
static BOOL oldControl = FALSE;
static BOOL xpControl = FALSE;

/* Used to recalculate the main window layout */
static int  bottomMargin;
static int  topMargin;
static int  have_history = FALSE;

static BOOL have_selection = FALSE;

static HBITMAP hMissing_bitmap;

/* Icon variables */
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static HIMAGELIST   hHeaderImages = NULL;
static int          *icon_index = NULL; /* for custom per-game icons */

static TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,    TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_PICTURE_AREA,TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_LARGE_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_SMALL_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{4, ID_VIEW_LIST_MENU,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{5, ID_VIEW_DETAIL,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 5},
	{6, ID_VIEW_GROUPED, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 6},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{7, ID_HELP_ABOUT,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7},
	{8, ID_HELP_CONTENTS,   TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 8}
};

#define NUM_TOOLBUTTONS (sizeof(tbb) / sizeof(tbb[0]))

#define NUM_TOOLTIPS 8

static char szTbStrings[NUM_TOOLTIPS + 1][30] =
{
	"Toggle Folder List",
	"Toggle Screen Shot",
	"Large Icons",
	"Small Icons",
	"List",
	"Details",
	"Grouped",
	"About",
	"Help"
};

static int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_LARGE_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
	ID_VIEW_GROUPED,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

static const int s_nPickers[] =
{
	IDC_LIST
#ifdef MESS
	,IDC_SWLIST
#endif
};

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            FALSE, RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_HWND, { 0 },            FALSE, RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  FALSE, RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     TRUE,	RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     TRUE,	RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, FALSE,	RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  TRUE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    FALSE,	RA_RIGHT | RA_TOP,                 NULL },
#ifdef MESS
	{ RA_ID,   { IDC_SWLIST },    TRUE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER3 },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
#endif /* MESS */
	{ RA_END,  { 0 },            FALSE, 0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

/* our dialog/configured options */
static options_type playing_game_options;

/* last directory for common file dialogs */
static char last_directory[MAX_PATH];

/* system-wide window message sent out with an ATOM of the current game name
   each time it changes */
static UINT g_mame32_message = 0;
static BOOL g_bDoBroadcast   = FALSE;

static BOOL use_gui_romloading = FALSE;

static BOOL g_listview_dragging = FALSE;
HIMAGELIST himl_drag;
int game_dragged; /* which game started the drag */
HTREEITEM prev_drag_drop_target; /* which tree view item we're currently highlighting */

static BOOL g_in_treeview_edit = FALSE;

typedef struct
{
    const char *name;
    int index;
} driver_data_type;
static driver_data_type *sorted_drivers;

static char * g_pRecordName = NULL;
static char * g_pPlayBkName = NULL;
static char * g_pSaveStateName = NULL;
static char * g_pRecordWaveName = NULL;
static char * g_pRecordMNGName = NULL;
static char * override_playback_directory = NULL;
static char * override_savestate_directory = NULL;

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Background Image handles also accessed from TreeView.c */
static HPALETTE         hPALbg   = 0;
static HBITMAP          hBackground  = 0;
static MYBITMAPINFO     bmDesc;

/* List view Column text */
const char* column_names[COLUMN_MAX] =
{
#ifdef MESS
	"System",
	"Bios",
#else
	"Game",
	"ROMs",
#endif
	"Samples",
	"Directory",
	"Type",
	"Trackball",
	"Played",
	"Manufacturer",
	"Year",
	"Clone Of",
	"Source",
	"Play Time"
};

/* a tiny compile is without Neogeo games */
#if (defined(NEOFREE) || defined(TINY_COMPILE)) && !defined(NEOMAME)
game_driver driver_neogeo =
{
	__FILE__,
	0,
	"Neo-Geo Fake driver",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
#ifdef MESS
	0,
	0,
	0,
#endif
	NOT_A_DRIVER,
};
#else
extern game_driver driver_neogeo;
#endif

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) \
    SendMessage(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) \
    SendMessage(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

/***************************************************************************
    External functions
 ***************************************************************************/

static void CreateCommandLine(int nGameIndex, char* pCmdLine)
{
	char pModule[_MAX_PATH];
	options_type* pOpts;
	LPTREEFOLDER folder;
	int screens_counter = 0;

	// this command line can grow too long for win9x, so we try to not send
	// some default values

	GetModuleFileName(GetModuleHandle(NULL), pModule, _MAX_PATH);

	folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(nGameIndex) );
	if( folder )
	{
		pOpts = GetGameOptions(nGameIndex, folder->m_nFolderId);
	}
	else
	{
		pOpts = GetGameOptions(nGameIndex, NO_FOLDER);
	}

	sprintf(pCmdLine, "%s %s", pModule, drivers[nGameIndex]->name);

#ifdef MESS
	MessCreateCommandLine(pCmdLine, pOpts, drivers[nGameIndex]);
#endif

	sprintf(&pCmdLine[strlen(pCmdLine)], " -rp \"%s\"",            GetRomDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -sp \"%s\"",         GetSampleDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -inipath \"%s\"",			GetIniDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cfg_directory \"%s\"",      GetCfgDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -nvram_directory \"%s\"",    GetNvramDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -memcard_directory \"%s\"",  GetMemcardDir());

	if (override_playback_directory != NULL)
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -input_directory \"%s\"",override_playback_directory);
	}
	else
		sprintf(&pCmdLine[strlen(pCmdLine)], " -input_directory \"%s\"",GetInpDir());

	sprintf(&pCmdLine[strlen(pCmdLine)], " -state_directory \"%s\"",    GetStateDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -artpath \"%s\"",	GetArtDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -snapshot_directory \"%s\"", GetImgDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -diff_directory \"%s\"",     GetDiffDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -ctrlr_directory \"%s\"",    GetCtrlrDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -comment_directory \"%s\"",  GetCommentDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cheat_file \"%s\"",         GetCheatFileName());

	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sskip_gameinfo", pOpts->skip_gameinfo ? "" : "no");
	/* Core Misc Options*/
	if (DriverHasOptionalBIOS(nGameIndex))
		sprintf(&pCmdLine[strlen(pCmdLine)], " -bios %s",pOpts->bios);
	if (pOpts->cheat)
        sprintf(&pCmdLine[strlen(pCmdLine)], " -%sc",                   pOpts->cheat ? "" : "no" );
	/* save states and input recording */
	if (g_pSaveStateName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -state \"%s\"",          g_pSaveStateName);
	if (pOpts->autosave)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -autosave");
	if (pOpts->mt_render)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -mt");
	if (g_pPlayBkName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -pb \"%s\"",             g_pPlayBkName);
	if (g_pRecordName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -rec \"%s\"",            g_pRecordName);
	if (g_pRecordMNGName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -mngwrite \"%s\"",       g_pRecordMNGName);
	if (g_pRecordWaveName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -wavwrite \"%s\"",       g_pRecordWaveName);
	/* debugging options */
#ifdef _DEBUG
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sd",               pOpts->mame_debug   ? "" : "no");
#endif
	if (pOpts->errorlog)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%slog",                     pOpts->errorlog   ? "" : "no");
	//Unsupported options in UI
/*
	{ "oslog",                    "0",        OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",    OPTION_BOOLEAN,    "display additional diagnostic information" },
	{ "validate;valid",           NULL,   OPTION_COMMAND,    "perform driver validation on all game drivers" },
*/
	/* Performance Options */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%safs",                     pOpts->autoframeskip   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fs %d",						pOpts->frameskip );
	sprintf(&pCmdLine[strlen(pCmdLine)], " -ftr %d",					pOpts->frames_to_display);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sthrottle",				pOpts->throttle   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssleep",					pOpts->sleep   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%srdtsc",					pOpts->old_timing ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -priority %d",				pOpts->priority);
	/* video */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -video %s",                  pOpts->videomode );
	sprintf(&pCmdLine[strlen(pCmdLine)], " -numscreens %d",             pOpts->numscreens);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sw",                      pOpts->window_mode? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smax",                    pOpts->maximize? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ska",                     pOpts->keepaspect ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -prescale %d",               pOpts->prescale);
	if( pOpts->effect )
		sprintf(&pCmdLine[strlen(pCmdLine)], " -effect \"%s\"",         pOpts->effect);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -pause_brightness %f",       pOpts->f_pause_bright); 
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%swaitvsync",              pOpts->wait_vsync ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssyncrefresh",            pOpts->syncrefresh ? "" : "no");
	/* Video Rotation Options*/
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%srotate", pOpts->rotate ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sror",pOpts->ror ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%srol",pOpts->rol ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sautoror",pOpts->auto_ror ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sautorol",pOpts->auto_rol ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipx",pOpts->flipx ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipy",pOpts->flipy ? "" : "no");
	/*Direct Draw Video Options*/
	if( strcmp( pOpts->videomode, "ddraw" ) == 0 )
	{
		if (pOpts->ddraw_stretch)
			sprintf(&pCmdLine[strlen(pCmdLine)], " -%shws",  pOpts->ddraw_stretch ? "" : "no");
	}
	/*Direct 3D Video Options*/
	if( strcmp( pOpts->videomode, "d3d" ) == 0 )
	{
		if (pOpts->d3d_filter)
			sprintf(&pCmdLine[strlen(pCmdLine)], " -%sfilter",pOpts->d3d_filter ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -d3dversion %d",         pOpts->d3d_version);
	}
	/* Per window options */
	for( screens_counter = 0; screens_counter < min(pOpts->numscreens, MAX_SCREENS); screens_counter++ )
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -screen%d %s", screens_counter, pOpts->screen_params[screens_counter].screen);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -aspect%d %s", screens_counter, pOpts->screen_params[screens_counter].aspect);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -resolution%d %s", screens_counter, pOpts->screen_params[screens_counter].resolution);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -view%d %s", screens_counter ,pOpts->screen_params[screens_counter].view);
	}
	/* Full screen Options */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%stb",					    pOpts->use_triplebuf ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sswitchres",              pOpts->switchres ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fsb %f",					pOpts->gfx_brightness);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fsc %f",                    pOpts->gfx_contrast);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fsg %f",                    pOpts->gfx_gamma);
	/* Game screen options*/
	sprintf(&pCmdLine[strlen(pCmdLine)], " -brightness %f",pOpts->f_bright_correct);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -contrast %f",pOpts->f_contrast_correct);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -gamma %f",pOpts->f_gamma_correct);
	/* Vector rendering Options*/
	if (DriverIsVector(nGameIndex))
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%saa",                      pOpts->antialias ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -beam %f",                   pOpts->f_beam);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -flicker %f",                pOpts->f_flicker);
	}
	/* Artwork options*/
	if (pOpts->artwork_crop)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sartcrop",            pOpts->artwork_crop ? "" : "no");
	if (pOpts->backdrops)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sbackdrop",            pOpts->backdrops ? "" : "no");
	if (pOpts->overlays)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%soverlay",             pOpts->overlays ? "" : "no");
	if (pOpts->bezels)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sbezel",               pOpts->bezels ? "" : "no");
	/* Core Sound options*/
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssound",                   pOpts->enable_sound ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -sr %d",                     pOpts->samplerate);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssamples",                 pOpts->use_samples ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -volume %d",                    pOpts->attenuation);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -audio_latency %i",          pOpts->audio_latency);
	/* input options*/
	if (strlen(pOpts->ctrlr) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -ctrlr \"%s\"",              pOpts->ctrlr);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smouse",					pOpts->use_mouse ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sjoy",					pOpts->use_joystick ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sgun",					(!pOpts->window_mode && pOpts->lightgun) ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdual",				pOpts->dual_lightgun ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sreload",				pOpts->offscreen_reload ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssteady",					pOpts->steadykey ? "" : "no");
	if (pOpts->use_joystick)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -a2d %f",                pOpts->f_a2d);
	if (strlen(pOpts->digital) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -digital \"%s\"",            pOpts->digital);
	if (strlen(pOpts->paddle) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -paddle \"%s\"",             pOpts->paddle);
	if (strlen(pOpts->adstick) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -adstick \"%s\"",            pOpts->adstick);
	if (strlen(pOpts->pedal) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -pedal \"%s\"",              pOpts->pedal);
	if (strlen(pOpts->dial) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -dial \"%s\"",               pOpts->dial);
	if (strlen(pOpts->trackball) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -trackball \"%s\"",          pOpts->trackball);
	if (strlen(pOpts->lightgun_device) > 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -lightgun_device \"%s\"",    pOpts->lightgun_device);
	dprintf("Launching MAME32:");
	dprintf("%s\n",pCmdLine);
}

static BOOL WaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG   msg;

	while (1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE;
		}
	}
	return FALSE;
}

static DWORD RunMAME(int nGameIndex)
{
	time_t start, end;
	double elapsedtime;

#if MULTISESSION
	int argc = 0;
	const char *argv[100];
	char pModule[_MAX_PATH];
	extern int DECL_SPEC main_(int, char**);
	int exit_code;
	int UIPriority = GetThreadPriority(GetCurrentThread());

	GetModuleFileName(GetModuleHandle(NULL), pModule, _MAX_PATH);
	argv[argc++] = pModule;
	argv[argc++] = drivers[nGameIndex]->name;

	if (override_playback_directory != NULL)
	{
		argv[argc++] = "-input_directory";
		argv[argc++] = override_playback_directory;
	}
	if (g_pPlayBkName != NULL)
	{
		argv[argc++] = "-pb";
		argv[argc++] = g_pPlayBkName;
	}
	if (g_pRecordName != NULL)
	{
		argv[argc++] = "-rec";
		argv[argc++] = g_pRecordName;
	}
	if (g_pRecordWaveName != NULL)
	{
		argv[argc++] = "-wavwrite";
		argv[argc++] = g_pRecordWaveName;
	}
	if (g_pRecordMNGName != NULL)
	{
		argv[argc++] = "-mngwrite";
		argv[argc++] = g_pRecordMNGName;
	}
	if (g_pSaveStateName != NULL)
	{
		argv[argc++] = "-state";
		argv[argc++] = g_pSaveStateName;
	}

	ShowWindow(hMain, SW_HIDE);

	time(&start);
	SetTimer(hMain, GAMEWND_TIMER, 1000/*1s*/, NULL);
	exit_code = main_(argc, (char **)argv);
	time(&end);
	/*This is to make sure this timer is killed, if the Game Window was not found
	Should not happen, but you never know... */
	KillTimer(hMain,GAMEWND_TIMER);
	//Restore UI Thread Priority
	SetThreadPriority(GetCurrentThread(),UIPriority);
	elapsedtime = end - start;
	if (exit_code == 0)
	{
		// Check the exitcode before incrementing Playtime
		IncrementPlayTime(nGameIndex, elapsedtime);
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
	}

	if (GetHideMouseOnStartup())
	{
		ShowCursor(FALSE);
	}
	else
	{
		// recover windows cursor and our main window
		while (1)
		{
			if (ShowCursor(TRUE) >= 0)
				break;
		}
	}
	ShowWindow(hMain, SW_SHOW);

	return 0;

#else

	DWORD               dwExitCode = 0;
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
	char pCmdLine[2048];
	HWND hGameWnd = NULL;
	long lGameWndStyle = 0;
	int UIPriority = GetThreadPriority(GetCurrentThread());	//Save the Priority
	BOOL bPickerIdling[sizeof(s_nPickers) / sizeof(s_nPickers[0])];
	int i;

#ifdef MESS
	SaveGameOptions(nGameIndex);
#endif

	CreateCommandLine(nGameIndex, pCmdLine );

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	if (!CreateProcess(NULL,
						pCmdLine,
						NULL,		  /* Process handle not inheritable. */
						NULL,		  /* Thread handle not inheritable. */
						TRUE,		  /* Handle inheritance.  */
						0,			  /* Creation flags. */
						NULL,		  /* Use parent's environment block.  */
						NULL,		  /* Use parent's starting directory.  */
						&si,		  /* STARTUPINFO */
						&pi))		  /* PROCESS_INFORMATION */
	{
		dprintf("CreateProcess failed.");
		dwExitCode = GetLastError();
	}
	else
	{

		ShowWindow(hMain, SW_HIDE);
		SendIconToProcess(&pi, nGameIndex);
		if( ! GetGameCaption() )
		{
			hGameWnd = GetGameWindow(&pi);
			if( hGameWnd )
			{
				lGameWndStyle = GetWindowLong(hGameWnd, GWL_STYLE);
				lGameWndStyle = lGameWndStyle & (WS_BORDER ^ 0xffffffff);
				SetWindowLong(hGameWnd, GWL_STYLE, lGameWndStyle);
				SetWindowPos(hGameWnd,0,0,0,0,0,SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
			}
		}
		time(&start);

		// Save idling info
		for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		{
			HWND hwndPicker = GetDlgItem(hMain, s_nPickers[i]);
			bPickerIdling[i] = Picker_IsIdling(hwndPicker);
			if (bPickerIdling[i])
				Picker_ClearIdle(hwndPicker);
		}

		// Wait until child process exits.
		WaitWithMessageLoop(pi.hProcess);

		GetExitCodeProcess(pi.hProcess, &dwExitCode);

		time(&end);
		elapsedtime = end - start;
		if( dwExitCode == 0 )
		{
			// Check the exitcode before incrementing Playtime
			IncrementPlayTime(nGameIndex, elapsedtime);
			ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		}

		ShowWindow(hMain, SW_SHOW);
		//Restore UI Thread Priority
		SetThreadPriority(GetCurrentThread(),UIPriority);
		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// Restore idling, if necessary
		for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		{
			HWND hwndPicker = GetDlgItem(hMain, s_nPickers[i]);
			if (bPickerIdling[i])
				Picker_ResetIdle(hwndPicker);
		}

#ifdef MESS
		LoadGameOptions(nGameIndex);
		MessReadMountedSoftware(nGameIndex);
#endif
	}

	return dwExitCode;
#endif
}

int Mame32Main(HINSTANCE    hInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	dprintf("MAME32 starting\n");

	use_gui_romloading = TRUE;

	if (__argc != 1)
	{
		/* Rename main because gcc will use it instead of WinMain even with -mwindows */
		extern int DECL_SPEC mame_main(int, char**);
		exit(mame_main(__argc, __argv));
	}
	if (!Win32UI_init(hInstance, lpCmdLine, nCmdShow))
		return 1;

	// pump message, but quit on WM_QUIT
	while(PumpMessage())
		;

	Win32UI_exit();
	return 0;
}


HWND GetMainWindow(void)
{
	return hMain;
}

HWND GetTreeView(void)
{
	return hTreeView;
}

void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX];
	int nColumnMax;
	int i;

	nColumnMax = Picker_GetNumColumns(hwndList);

	/* Get the Column Order and save it */
	if (!oldControl)
	{
		ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++)
		{
			order[i] = Picker_GetRealColumnFromViewColumn(hwndList, tmpOrder[i]);
		}
	}
}

/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	ICONIMAGE           IconImage;
	LPICONDIRENTRY      lpIDE = NULL;
	UINT                nNumImages;
	UINT                nBufferIndex = 0;
	HICON               hIcon = NULL;

	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it 'reserved' ?	 (ie 0) */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Then that's the number of images in the ICO file */
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if ( nNumImages < 1 )
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
	{
		return NULL;
	}

	/* Assign icon directory entries from buffer */
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if ( lpIDE->dwImageOffset > nBufferSize )
	{
		return NULL;
	}

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
	{
		return NULL;
	}

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;

	hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000,
			(*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biWidth, (*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biHeight/2, 0 );

	/* It failed, odds are good we're on NT so try the non-Ex way */
	if (hIcon == NULL)
	{
		/* We would break on NT if we try with a 16bpp image */
		if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		{
			hIcon = CreateIconFromResource(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000);
		}
	}
	return hIcon;
}

HICON LoadIconFromFile(const char *iconname)
{
	HICON hIcon = 0;
	struct stat file_stat;
	char tmpStr[MAX_PATH];
	char tmpIcoName[MAX_PATH];
	const char* sDirName = GetImgDir();
	PBYTE bufferPtr;
	zip_error ziperr;
	zip_file *zip;
	const zip_file_header *entry;

	sprintf(tmpStr, "%s/%s.ico", GetIconsDir(), iconname);
	if (stat(tmpStr, &file_stat) != 0
	|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
	{
		sprintf(tmpStr, "%s/%s.ico", sDirName, iconname);
		if (stat(tmpStr, &file_stat) != 0
		|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
		{
			sprintf(tmpStr, "%s/icons.zip", GetIconsDir());
			sprintf(tmpIcoName, "%s.ico", iconname);

			ziperr = zip_file_open(tmpStr, &zip);
			if (ziperr == ZIPERR_NONE)
			{
				entry = zip_file_first_file(zip);
				while(!hIcon && entry)
				{
					if (!mame_stricmp(entry->filename, tmpIcoName))
					{
						bufferPtr = malloc(entry->uncompressed_length);
						if (bufferPtr)
						{
							ziperr = zip_file_decompress(zip, bufferPtr, entry->uncompressed_length);
							if (ziperr == ZIPERR_NONE)
							{
								hIcon = FormatICOInMemoryToHICON(bufferPtr, entry->uncompressed_length);
							}
							free(bufferPtr);
						}
					}
					entry = zip_file_next_file(zip);
				}
				zip_file_close(zip);
			}
		}
	}
	return hIcon;
}

/* Return the number of games currently displayed */
int GetNumGames()
{
	return game_count;
}

/* Return the number of folders with options */
int GetNumOptionFolders()
{
	return optionfolder_count;
}

/* Return the number of folders with options */
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}


/* Sets the treeview and listviews sizes in accordance with their visibility and the splitters */
static void ResizeTreeAndListViews(BOOL bResizeHidden)
{
	int i;
	int nLastWidth = 0;
	int nLastWidth2 = 0;
	int nLeftWindowWidth = 0;
	RECT rect;
	BOOL bVisible;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	/* Tree control */
	ShowWindow(GetDlgItem(hMain, IDC_TREE), bShowTree ? SW_SHOW : SW_HIDE);

	for (i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bVisible = GetWindowLong(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE ? TRUE : FALSE;
		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH/2 - nLastWidth;

			/* special case for the rightmost pane when the screenshot is gone */
			if (!GetShowScreenShot() && !g_splitterInfo[i+1].nSplitterWindow)
				nLeftWindowWidth = rect.right - nLastWidth;

			/* woah?  are we overlapping ourselves? */
			if (nLeftWindowWidth < MIN_VIEW_WIDTH)
			{
				nLastWidth = nLastWidth2;
				nLeftWindowWidth = nSplitterOffset[i] - MIN_VIEW_WIDTH - (SPLITTER_WIDTH*3/2) - nLastWidth;
				i--;
			}

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 2,
				nLeftWindowWidth, (rect.bottom - rect.top) - 4 , TRUE);

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 2,
				SPLITTER_WIDTH, (rect.bottom - rect.top) - 4, TRUE);
		}

		if (bVisible)
		{
			nLastWidth2 = nLastWidth;
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH; 
		}
	}
}

/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot(void)
{
	RECT rect;
	int  nWidth;
	RECT fRect;
	POINT p = {0, 0};

	/* first time through can't do this stuff */
	if (hwndList == NULL)
		return;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowScreenShot())
	{
		nWidth = nSplitterOffset[GetSplitterCount() - 1];
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_CHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_CHECKED);
	}
	else
	{
		nWidth = rect.right;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
	}

	ResizeTreeAndListViews(FALSE);

	FreeScreenShot();

	if (have_selection)
	{
#ifdef MESS
		if (!s_szSelectedItem[0] || !LoadScreenShotEx(Picker_GetSelectedItem(hwndList), s_szSelectedItem,
			TabView_GetCurrentTab(hTabCtrl)))
#endif
		{
			// load and set image, or empty it if we don't have one
			LoadScreenShot(Picker_GetSelectedItem(hwndList), TabView_GetCurrentTab(hTabCtrl));
		}
	}

	// figure out if we have a history or not, to place our other windows properly
	UpdateHistory();

	// setup the picture area

	if (GetShowScreenShot())
	{
		DWORD dwStyle;
		DWORD dwStyleEx;
		BOOL showing_history;

		ClientToScreen(hMain, &p);
		GetWindowRect(GetDlgItem(hMain, IDC_SSFRAME), &fRect);
		OffsetRect(&fRect, -p.x, -p.y);

		// show history on this tab IFF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		showing_history = (have_history && (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() || GetHistoryTab() == TAB_ALL ) &&
						   GetShowTab(TAB_HISTORY) == FALSE);
		CalculateBestScreenShotRect(GetDlgItem(hMain, IDC_SSFRAME), &rect,showing_history);
			
		dwStyle   = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_STYLE);
		dwStyleEx = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_EXSTYLE);
			
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);
		MoveWindow(GetDlgItem(hMain, IDC_SSPICTURE),
				   fRect.left  + rect.left,
				   fRect.top   + rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top,
				   TRUE);
		
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),
				   (TabView_GetCurrentTab(hTabCtrl) != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_SHOW);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),bShowTabCtrl ? SW_SHOW : SW_HIDE);

		InvalidateRect(GetDlgItem(hMain,IDC_SSPICTURE),NULL,FALSE);
	}
	else
	{
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),SW_HIDE);
	}

}

void ResizePickerControls(HWND hWnd)
{
	RECT frameRect;
	RECT rect, sRect;
	int  nListWidth, nScreenShotWidth;
	static BOOL firstTime = TRUE;
	int  doSSControls = TRUE;
	int i, nSplitterCount;

	nSplitterCount = GetSplitterCount();

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc the display sizes based on g_splitterInfo */
	if (firstTime)
	{
		RECT rWindow;

		for (i = 0; i < nSplitterCount; i++)
			nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;

		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		/*buttonMargin = (sRect.bottom + 4); */

		firstTime = FALSE;
	}
	else
	{
		doSSControls = GetShowScreenShot();
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, TRUE);

	ResizeTreeAndListViews(TRUE);

	nListWidth = nSplitterOffset[nSplitterCount-1];
	nScreenShotWidth = (rect.right - nListWidth) - 4;

	/* Screen shot Page tab control */
	if (bShowTabCtrl)
	{
		MoveWindow(GetDlgItem(hWnd, IDC_SSTAB), nListWidth + 4, rect.top + 2,
			nScreenShotWidth - 2, rect.top + 20, doSSControls);
		rect.top += 20;
	}

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2,
		nScreenShotWidth - 2, (rect.bottom - rect.top) - 4, doSSControls);

	/* The screen shot controls */
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);

	/* Text control - game history */
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + (nScreenShotWidth - 22);

	
	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 14;
		sRect.bottom = (rect.bottom - rect.top) - 30;   
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + 264;
		sRect.bottom = (rect.bottom - rect.top) - 278;
	}

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY),
		sRect.left, sRect.top,
		sRect.right - sRect.left, sRect.bottom, doSSControls);

	/* the other screen shot controls will be properly placed in UpdateScreenshot() */

}

char *ModifyThe(const char *str)
{
	static int  bufno = 0;
	static char buffer[4][255];

	if (strncmp(str, "The ", 4) == 0)
	{
		char *s, *p;
		char temp[255];

		strcpy(temp, &str[4]);

		bufno = (bufno + 1) % 4;

		s = buffer[bufno];

		/* Check for version notes in parens */
		p = strchr(temp, '(');
		if (p)
		{
			p[-1] = '\0';
		}

		strcpy(s, temp);
		strcat(s, ", The");

		if (p)
		{
			strcat(s, " ");
			strcat(s, p);
		}

		return s;
	}
	return (char *)str;
}

HBITMAP GetBackgroundBitmap(void)
{
	return hBackground;
}

HPALETTE GetBackgroundPalette(void)
{
	return hPALbg;
}

MYBITMAPINFO * GetBackgroundInfo(void)
{
	return &bmDesc;
}

BOOL GetUseOldControl(void)
{
	return oldControl;
}

BOOL GetUseXPControl(void)
{
	return xpControl;
}

int GetMinimumScreenShotWindowWidth(void)
{
	BITMAP bmp;
	GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

	return bmp.bmWidth + 6; // 6 is for a little breathing room
}


int GetDriverIndex(const game_driver *driver)
{
	return GetGameNameIndex(driver->name);
}

int GetParentIndex(const game_driver *driver)
{
	return GetGameNameIndex(driver->parent);
}

int GetParentRomSetIndex(const game_driver *driver)
{
	int nParentIndex = GetGameNameIndex(driver->parent);

	if( nParentIndex >= 0)
	{
		if ((drivers[nParentIndex]->flags & NOT_A_DRIVER) == 0)
			return nParentIndex;
	}

	return -1;
}

int GetGameNameIndex(const char *name)
{
    driver_data_type *driver_index_info;
	driver_data_type key;
	key.name = name;

	// uses our sorted array of driver names to get the index in log time
	driver_index_info = bsearch(&key,sorted_drivers,game_count,sizeof(driver_data_type),
								DriverDataCompareFunc);

	if (driver_index_info == NULL)
		return -1;

	return driver_index_info->index;

}

int GetIndexFromSortedIndex(int sorted_index)
{
	return sorted_drivers[sorted_index].index;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

// used for our sorted array of game names
int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2)
{
    return strcmp( ((driver_data_type *)arg1)->name, ((driver_data_type *)arg2)->name );
}

static void ResetBackground(char *szFile)
{
	char szDestFile[MAX_PATH];

	/* The MAME core load the .png file first, so we only need replace this file */
	sprintf(szDestFile, "%s\\bkground.png", GetBgDir());
	SetFileAttributes(szDestFile, FILE_ATTRIBUTE_NORMAL);
	CopyFileA(szFile, szDestFile, FALSE);
}

static void RandomSelectBackground(void)
{
	struct _finddata_t c_file;
	long hFile;
	char szFile[MAX_PATH];
	int count=0;
	const char *szDir=GetBgDir();
	char *buf=malloc(_MAX_FNAME * MAX_BGFILES);

	if (buf == NULL)
		return;

	sprintf(szFile, "%s\\*.bmp", szDir);
	hFile = _findfirst(szFile, &c_file);
	if (hFile != -1L)
	{
		int Done = 0;
		while (!Done && count < MAX_BGFILES)
		{
			memcpy(buf + count * _MAX_FNAME, c_file.name, _MAX_FNAME);
			count++;
			Done = _findnext(hFile, &c_file);
		}
		_findclose(hFile);
	}
	sprintf(szFile, "%s\\*.png", szDir);
	hFile = _findfirst(szFile, &c_file);
	if (hFile != -1L)
	{
		int Done = 0;
		while (!Done && count < MAX_BGFILES)
		{
			memcpy(buf + count * _MAX_FNAME, c_file.name, _MAX_FNAME);
			count++;
			Done = _findnext(hFile, &c_file);
		}
		_findclose(hFile);
	}

	if (count)
	{
		srand( (unsigned)time( NULL ) );
		sprintf(szFile, "%s\\%s", szDir, buf + (rand() % count) * _MAX_FNAME);
		ResetBackground(szFile);
	}

	free(buf);
}

void SetMainTitle(void)
{
	char version[50];
	char buffer[100];

	sscanf(build_version,"%s",version);
	sprintf(buffer,"%s %s",MAME32NAME,version);
	SetWindowText(hMain,buffer);
}

static BOOL Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
{
	extern int mame_validitychecks(int game);
	WNDCLASS	wndclass;
	RECT		rect;
	int i, nSplitterCount;
	extern FOLDERDATA g_folderData[];
	extern FILTER_ITEM g_filterList[];
	extern const char *history_filename;
	extern const char *mameinfo_filename;
	LONG common_control_version = GetCommonControlVersion();
	int validity_failed = 0;
	int argc = 0;
	char *args[4];
	char **argv = args;
	char pModule[_MAX_PATH];
	char name[_MAX_PATH];

	srand((unsigned)time(NULL));

	init_resource_tracking();
	begin_resource_tracking();

	strcpy( name, drivers[0]->name );
	GetModuleFileName(GetModuleHandle(NULL), pModule, _MAX_PATH);
	argv[argc++] = pModule;
	argv[argc++] = name;
	cli_frontend_init(argc, argv );



	// Count the number of games
	game_count = 0;
	while (drivers[game_count] != 0)
		game_count++;

	/* custom per-game icons */
	icon_index = auto_malloc(sizeof(int) * game_count);
	if (!icon_index)
		return FALSE;
	ZeroMemory(icon_index,sizeof(int) * game_count);

	/* sorted list of drivers by name */
	sorted_drivers = (driver_data_type *) auto_malloc(sizeof(driver_data_type) * game_count);
	if (!sorted_drivers)
		return FALSE;
	for (i=0;i<game_count;i++)
	{
		sorted_drivers[i].name = drivers[i]->name;
		sorted_drivers[i].index = i;
	}
	qsort(sorted_drivers,game_count,sizeof(driver_data_type),DriverDataCompareFunc );

	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAME32_ICON));
	wndclass.hCursor	   = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = "MainClass";

	RegisterClass(&wndclass);

#ifdef MESS
	uistring_init(NULL);
	DevView_RegisterClass();
#endif //MESS

	InitCommonControls();

	// Are we using an Old comctl32.dll?
	dprintf("common controlversion %i %i",common_control_version >> 16,
			common_control_version & 0xffff);
			 
	oldControl = (common_control_version < PACKVERSION(4,71));
	xpControl = (common_control_version >= PACKVERSION(6,0));
	if (oldControl)
	{
		char buf[] = MAME32NAME " has detected an old version of comctl32.dll\n\n"
					 "Game Properties, many configuration options and\n"
					 "features are not available without an updated DLL\n\n"
					 "Please install the common control update found at:\n\n"
					 "http://www.microsoft.com/msdownload/ieplatform/ie/comctrlx86.asp\n\n"
					 "Would you like to continue without using the new features?\n";

		if (IDNO == MessageBox(0, buf, MAME32NAME " Outdated comctl32.dll Warning", MB_YESNO | MB_ICONWARNING))
			return FALSE;
    }

	dprintf("about to init options");
	if (!OptionsInit())
		return FALSE;
	dprintf("options loaded");

	g_mame32_message = RegisterWindowMessage("MAME32");
	g_bDoBroadcast = GetBroadcast();

	HelpInit();

	strcpy(last_directory,GetInpDir());
	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);
	if (hMain == NULL)
	{
		dprintf("error creating main dialog, aborting");
		return FALSE;
	}

	s_pWatcher = DirWatcher_Init(hMain, WM_MAME32_FILECHANGED);
	if (s_pWatcher)
	{
		DirWatcher_Watch(s_pWatcher, 0, GetRomDirs(), TRUE);
		DirWatcher_Watch(s_pWatcher, 1, GetSampleDirs(), TRUE);
	}

	SetMainTitle();
	hTabCtrl = GetDlgItem(hMain, IDC_SSTAB);
	
	{
		struct TabViewOptions opts;

		static struct TabViewCallbacks s_tabviewCallbacks =
		{
			GetShowTabCtrl,			// pfnGetShowTabCtrl
			SetCurrentTab,			// pfnSetCurrentTab
			GetCurrentTab,			// pfnGetCurrentTab
			SetShowTab,				// pfnSetShowTab
			GetShowTab,				// pfnGetShowTab

			GetImageTabShortName,	// pfnGetTabShortName
			GetImageTabLongName,	// pfnGetTabLongName
			UpdateScreenShot		// pfnOnSelectionChanged
		};

		memset(&opts, 0, sizeof(opts));
		opts.pCallbacks = &s_tabviewCallbacks;
		opts.nTabCount = MAX_TAB_TYPES;

		if (!SetupTabView(hTabCtrl, &opts))
			return FALSE;
	}

	/* subclass history window */
	g_lpHistoryWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_WNDPROC, (LONG)HistoryWndProc);

	/* subclass picture frame area */
	g_lpPictureFrameWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_SSFRAME), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_SSFRAME), GWL_WNDPROC, (LONG)PictureFrameWndProc);

	/* subclass picture area */
	g_lpPictureWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_WNDPROC, (LONG)PictureWndProc);

	/* Load the pic for the default screenshot. */
	hMissing_bitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT));

	/* Stash hInstance for later use */
	hInst = hInstance;

	hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	/* In order to handle 'Large Fonts' as the Windows
	 * default setting, we need to make the dialogs small
	 * enough to fit in our smallest window size with
	 * large fonts, then resize the picker, tab and button
	 * controls to fill the window, no matter which font
	 * is currently set.  This will still look like bad
	 * if the user uses a bigger default font than 125%
	 * (Large Fonts) on the Windows display setting tab.
	 *
	 * NOTE: This has to do with Windows default font size
	 * settings, NOT our picker font size.
	 */

	GetClientRect(hMain, &rect);

	hTreeView = GetDlgItem(hMain, IDC_TREE);
	hwndList  = GetDlgItem(hMain, IDC_LIST);

	history_filename = mame_strdup(GetHistoryFileName());
	mameinfo_filename = mame_strdup(GetMAMEInfoFileName());

	if (!InitSplitters())
		return FALSE;

	nSplitterCount = GetSplitterCount();
	for (i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd;
		HWND hWndLeft;
		HWND hWndRight;

		hWnd = GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow);
		hWndLeft = GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow);
		hWndRight = GetDlgItem(hMain, g_splitterInfo[i].nRightWindow);

		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hMain);

	TabView_UpdateSelection(hTabCtrl);

	bDoGameCheck = GetGameCheck();
	idle_work    = TRUE;
	game_index   = 0;

	bShowTree      = GetShowFolderList();
	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl   = GetShowTabCtrl();

	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);

	if (oldControl)
	{
		EnableMenuItem(GetMenu(hMain), ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_GAME_PROPERTIES,	MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	/* Init DirectDraw */
	if (!DirectDraw_Initialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	if (GetRandomBackground())
		RandomSelectBackground();

	LoadBackgroundBitmap();

	dprintf("about to init tree");
	InitTree(g_folderData, g_filterList);
	dprintf("did init tree");
	FolderOptionsInit();
	PropertiesInit();

	/* Initialize listview columns */
#ifdef MESS
	InitMessPicker();
#endif
	InitListView();
	SetFocus(hwndList);

	/* Reset the font */
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		hFont = CreateFontIndirect(&logfont);
		if (hFont != NULL)
			SetAllWindowsFont(hMain, &main_resize, hFont, FALSE);
	}

	/* Init DirectInput */
	if (!DirectInputInitialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	AdjustMetrics();
	UpdateScreenShot();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TAB_KEYS));

	/* clear keyboard state */
	KeyboardStateClear();

	for (i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		input_seq *is1;
		input_seq *is2;
		is1 = &(GUISequenceControl[i].is);
		is2 = GUISequenceControl[i].getiniptr();
		seq_copy(is1, is2);
		//dprintf("seq =%s is: %4i %4i %4i %4i\n",GUISequenceControl[i].name, (*is1)[0], (*is1)[1], (*is1)[2], (*is1)[3]);
	}

	if (GetJoyGUI() == TRUE)
	{
		g_pJoyGUI = &DIJoystick;
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	if (GetHideMouseOnStartup())
	{    
		/*  For some reason the mouse is centered when a game is exited, which of
			course causes a WM_MOUSEMOVE event that shows the mouse. So we center
			it now, before the startup coords are initilized, and that way the mouse
			will still be hidden when exiting from a game (i hope) :)
		*/      
		SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2);
		
		// Then hide it
		ShowCursor(FALSE);
	}

	dprintf("about to show window\n");

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
	{
		nCmdShow = SW_RESTORE;
	}

	if (GetRunFullScreen())
	{ 
		LONG lMainStyle;

		// Remove menu
		SetMenu(hMain,NULL); 

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		nCmdShow = SW_MAXIMIZE;
	}

	ShowWindow(hMain, nCmdShow);

	
	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS :
		SetView(ID_VIEW_LARGE_ICON);
		break;
	case VIEW_SMALL_ICONS :
		SetView(ID_VIEW_SMALL_ICON);
		break;
	case VIEW_INLIST :
		SetView(ID_VIEW_LIST_MENU);
		break;
	case VIEW_REPORT :
		SetView(ID_VIEW_DETAIL);
		break;
	case VIEW_GROUPED :
	default :
		SetView(ID_VIEW_GROUPED);
		break;
	}

	if (GetCycleScreenshot() > 0)
	{
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to Seconds
	}

	if (validity_failed)
	{
		MessageBox(hMain, MAMENAME " has failed its validity checks.  The GUI will "
			"still work, but emulations will fail to execute", MAMENAME, MB_OK);
	}

	return TRUE;
}


static void Win32UI_exit()
{
    if (g_bDoBroadcast == TRUE)
    {
        ATOM a = GlobalAddAtom("");
        SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
        GlobalDeleteAtom(a);
    }

	if (g_pJoyGUI != NULL)
		g_pJoyGUI->exit();

	/* Free GDI resources */

	if (hMissing_bitmap)
	{
		DeleteObject(hMissing_bitmap);
		hMissing_bitmap = NULL;
	}

	if (hBackground)
	{
		DeleteObject(hBackground);
		hBackground = NULL;
	}
	
	if (hPALbg)
	{
		DeleteObject(hPALbg);
		hPALbg = NULL;
	}
	
	if (hFont)
	{
		DeleteObject(hFont);
		hFont = NULL;
	}
	
	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	DirectInputClose();
	DirectDraw_Close();

	SetSavedFolderID(GetCurrentFolderID());

	SaveOptions();

	FreeFolders();

    /* DestroyTree(hTreeView); */

	FreeScreenShot();

	OptionsExit();

	HelpExit();

	end_resource_tracking();
}

static long WINAPI MameWindowProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	MINMAXINFO	*mminfo;
#if MULTISESSION
	int nGame;
	HWND hGameWnd;
	long lGameWndStyle;
#endif // MULTISESSION


	int 		i;
	TCHAR szClass[128];

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
		if (hBackground && (HWND)lParam == GetDlgItem(hMain, IDC_HISTORY))
		{
			static HBRUSH hBrush=0;
			HDC hDC=(HDC)wParam;
			LOGBRUSH lb;

			if (hBrush)
				DeleteObject(hBrush);
			lb.lbStyle  = BS_HOLLOW;
			hBrush = CreateBrushIndirect(&lb);
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetListFontColor());
			return (LRESULT) hBrush;
		}
		break;

	case WM_INITDIALOG:
		/* Initialize info for resizing subitems */
		GetClientRect(hWnd, &main_resize.rect);
		return TRUE;

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETTINGCHANGE:
		AdjustMetrics();
		return 0;

	case WM_SIZE:
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(wParam));
		return TRUE;

	case WM_MENUSELECT:
		return Statusbar_MenuSelect(hWnd, wParam, lParam);

	case MM_PLAY_GAME:
		MamePlayGame();
		return TRUE;

	case WM_INITMENUPOPUP:
		UpdateMenu(GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if (HandleTreeContextMenu(hWnd, wParam, lParam)
		||	HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;

	case WM_COMMAND:
		return MameCommand(hWnd,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

	case WM_GETMINMAXINFO:
		/* Don't let the window get too small; it can break resizing */
		mminfo = (MINMAXINFO *) lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case JOYGUI_TIMER:
			PollGUIJoystick();
			break;
		case SCREENSHOT_TIMER:
			TabView_CalculateNextTab(hTabCtrl);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;
		case GAMEWND_TIMER:
#if MULTISESSION
			nGame = Picker_GetSelectedItem(hwndList);
			if( ! GetGameCaption() )
			{
				hGameWnd = GetGameWindow();
				if( hGameWnd )
				{
					lGameWndStyle = GetWindowLong(hGameWnd, GWL_STYLE);
					lGameWndStyle = lGameWndStyle & (WS_BORDER ^ 0xffffffff);
					SetWindowLong(hGameWnd, GWL_STYLE, lGameWndStyle);
					SetWindowPos(hGameWnd,0,0,0,0,0,SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
				}
			}
			if( SendIconToEmulationWindow(nGame)== TRUE);
				KillTimer(hMain, GAMEWND_TIMER);
#endif // MULTISESSION
			break;
		default:
			break;
		}
		return TRUE;

	case WM_CLOSE:
		{
			/* save current item */
			RECT rect;
			AREA area;
			int nItem;
			WINDOWPLACEMENT wndpl;
			UINT state;

			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hMain, &wndpl);
			state = wndpl.showCmd;

			/* Restore the window before we attempt to save parameters,
			 * This fixed the lost window on startup problem, among other problems
			 */
			if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED || state == SW_MAXIMIZE)
			{
				if( wndpl.flags & WPF_RESTORETOMAXIMIZED || state == SW_MAXIMIZE)
					state = SW_MAXIMIZE;
				else
				{
					state = SW_RESTORE;
					ShowWindow(hWnd, SW_RESTORE);
				}
			}
			for (i = 0; i < GetSplitterCount(); i++)
				SetSplitterPos(i, nSplitterOffset[i]);
			SetWindowState(state);

			for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
				Picker_SaveColumnWidths(GetDlgItem(hMain, s_nPickers[i]));

			GetWindowRect(hWnd, &rect);
			area.x		= rect.left;
			area.y		= rect.top;
			area.width	= rect.right  - rect.left;
			area.height = rect.bottom - rect.top;
			SetWindowArea(&area);

			/* Save the users current game options and default game */
			nItem = Picker_GetSelectedItem(hwndList);
			SetDefaultGame(ModifyThe(drivers[nItem]->name));

			/* hide window to prevent orphan empty rectangles on the taskbar */
			/* ShowWindow(hWnd,SW_HIDE); */
            DestroyWindow( hWnd );

			/* PostQuitMessage(0); */
			break;
		}

	case WM_DESTROY:
        PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

		/*
		  Check to see if the mouse has been moved by the user since
		  startup. I'd like this checking to be done only in the
		  main WinProc (here), but somehow the WM_MOUSEDOWN messages
		  are eaten up before they reach MameWindowProc. That's why
		  there is one check for each of the subclassed windows too.
    
		  POSSIBLE BUGS:
		  I've included this check in the subclassed windows, but a 
		  mose move in either the title bar, the menu, or the
		  toolbar will not generate a WM_MOUSEOVER message. At least
		  not one that I know how to pick up. A solution could maybe
		  be to subclass those too, but that's too much work :)
		*/
		
	case WM_MOUSEMOVE:
	{
		if (MouseHasBeenMoved())
			ShowCursor(TRUE);
		
		if (g_listview_dragging)
			MouseMoveListViewDrag(MAKEPOINTS(lParam));
		else
			/* for splitters */
			OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;
	}

	case WM_LBUTTONUP:
	    if (g_listview_dragging)
		    ButtonUpListViewDrag(MAKEPOINTS(lParam));
		else
		   /* for splitters */
		   OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_NOTIFY:
		/* Where is this message intended to go */
		{
			LPNMHDR lpNmHdr = (LPNMHDR)lParam;

			/* Fetch tooltip text */
			if (lpNmHdr->code == TTN_NEEDTEXT)
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				CopyToolTipText(lpttt);
			}

			if (lpNmHdr->hwndFrom == hTreeView)
				return TreeViewNotify(lpNmHdr);

			GetClassName(lpNmHdr->hwndFrom, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!strcmp(szClass, "SysListView32"))
				return Picker_HandleNotify(lpNmHdr);	
			if (!strcmp(szClass, "SysTabControl32"))
				return TabView_HandleNotify(lpNmHdr);
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;

			GetClassName(lpDis->hwndItem, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!strcmp(szClass, "SysListView32"))
				Picker_HandleDrawItem(GetDlgItem(hMain, lpDis->CtlID), lpDis);
		}
		break;

	case WM_MEASUREITEM :
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;

		// tell the list view that each row (item) should be just taller than our font

		//DefWindowProc(hWnd, message, wParam, lParam);
		//dprintf("default row height calculation gives %u\n",lpmis->itemHeight);

		TEXTMETRIC tm;
		HDC hDC = GetDC(NULL);
		HFONT hFontOld = (HFONT)SelectObject(hDC,hFont);

		GetTextMetrics(hDC,&tm);
		
		lpmis->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
		if (lpmis->itemHeight < 17)
			lpmis->itemHeight = 17;
		//dprintf("we would do %u\n",tm.tmHeight + tm.tmExternalLeading + 1);
		SelectObject(hDC,hFontOld);
		ReleaseDC(NULL,hDC);

		return TRUE;
	}

	case WM_MAME32_FILECHANGED:
		{
			char szFileName[32];
			char *s;
			int nGameIndex;
			int (*pfnGetAuditResults)(int driver_index) = NULL;
			void (*pfnSetAuditResults)(int driver_index, int audit_results) = NULL;

			switch(HIWORD(wParam))
			{
				case 0:
					pfnGetAuditResults = GetRomAuditResults;
					pfnSetAuditResults = SetRomAuditResults;
					break;

				case 1:
					pfnGetAuditResults = GetSampleAuditResults;
					pfnSetAuditResults = SetSampleAuditResults;
					break;
			}

			if (pfnGetAuditResults && pfnSetAuditResults)
			{
				int nParentIndex = -1;

				snprintf(szFileName, sizeof(szFileName), "%s", (LPCTSTR) lParam);
				s = strchr(szFileName, '.');
				if (s)
					*s = '\0';
				s = strchr(szFileName, '\\');
				if (s)
					*s = '\0';

				for (nGameIndex = 0; drivers[nGameIndex]; nGameIndex++)
				{
					for (nParentIndex = nGameIndex; nGameIndex == -1; nParentIndex = GetParentIndex(drivers[nParentIndex]))
					{
						if (!mame_stricmp(drivers[nParentIndex]->name, szFileName))
						{
							if (pfnGetAuditResults(nGameIndex) != UNKNOWN)
							{
								pfnSetAuditResults(nGameIndex, UNKNOWN);
								PostMessage(hMain, WM_MAME32_AUDITGAME, wParam, nGameIndex);
							}
							break;
						}
					}
				}
			}
		}
		break;

	case WM_MAME32_AUDITGAME:
		{
			LV_FINDINFO lvfi;
			int nGameIndex;

			nGameIndex = lParam;

			switch(HIWORD(wParam))
			{
				case 0:
					Mame32VerifyRomSet(nGameIndex);
					break;
				case 1:
					Mame32VerifySampleSet(nGameIndex);
					break;
			}

			memset(&lvfi, 0, sizeof(lvfi));
			lvfi.flags	= LVFI_PARAM;
			lvfi.lParam = nGameIndex;

			i = ListView_FindItem(hwndList, -1, &lvfi);
			if (i != -1)
			{
				ListView_RedrawItems(hwndList, i, i);
			}
		}
		break;

	default:

		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: /* List-View controls use this message for searching the items "as user types" */
			//MessageBox(NULL,"wm_char message arrived","TitleBox",MB_OK);
			return TRUE;

		case WM_KEYDOWN:
			KeyboardKeyDown(0, wParam, lParam);
			return TRUE;

		case WM_KEYUP:
			KeyboardKeyUp(0, wParam, lParam);
			return TRUE;

		case WM_SYSKEYDOWN:
			KeyboardKeyDown(1, wParam, lParam);
			return TRUE;

		case WM_SYSKEYUP:
			KeyboardKeyUp(1, wParam, lParam);
			return TRUE;
	}

	return FALSE;	/* message not processed */
}

static BOOL PumpMessage()
{
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
    {
		return FALSE;
    }

	if (IsWindow(hMain))
	{
		BOOL absorbed_key = FALSE;
		if (GetKeyGUI())
			absorbed_key = HandleKeyboardGUIMessage(msg.hwnd, msg.message, 
													msg.wParam, msg.lParam);
		else
			absorbed_key = TranslateAccelerator(hMain, hAccel, &msg);

		if (!absorbed_key)
		{
			if (!IsDialogMessage(hMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return TRUE;
}

static BOOL FolderCheck(void)
{
	
	char *pDescription = NULL;
	int nGameIndex = 0;
	int i=0;
	int iStep = 0;
	LV_FINDINFO lvfi;
	int nCount = ListView_GetItemCount(hwndList);
	BOOL changed = FALSE;

	MSG msg;
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		SetRomAuditResults(nGameIndex, UNKNOWN);
		SetSampleAuditResults(nGameIndex, UNKNOWN);
	}
	if( nCount > 0)
		ProgressBarShow();
	else
		return FALSE;
	if( nCount < 100 )
		iStep = 100 / nCount;
	else
		iStep = nCount/100;
	UpdateListView();
	UpdateWindow(hMain);
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		if (GetRomAuditResults(nGameIndex) == UNKNOWN)
		{
			Mame32VerifyRomSet(nGameIndex);
			changed = TRUE;
		}

		if (GetSampleAuditResults(nGameIndex) == UNKNOWN)
		{
			Mame32VerifySampleSet(nGameIndex);
			changed = TRUE;
		}

		lvfi.flags	= LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		i = ListView_FindItem(hwndList, -1, &lvfi);
		if (changed && i != -1);
		{
			ListView_RedrawItems(hwndList, i, i);
			while( PeekMessage( &msg, hwndList, 0, 0, PM_REMOVE ) != 0)
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			}
		}
		changed = FALSE;
		if ((i % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	pDescription = ModifyThe(drivers[Picker_GetSelectedItem(hwndList)]->description);
	SetStatusBarText(0, pDescription);
	UpdateStatusBar();
	return TRUE;
}

static BOOL GameCheck(void)
{
	LV_FINDINFO lvfi;
	int i;
	BOOL changed = FALSE;

	if (game_index == 0)
		ProgressBarShow();

	if (game_index >= game_count)
	{
		bDoGameCheck = FALSE;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		return FALSE;
	}

	if (GetRomAuditResults(game_index) == UNKNOWN)
	{
		Mame32VerifyRomSet(game_index);
		changed = TRUE;
	}

	if (GetSampleAuditResults(game_index) == UNKNOWN)
	{
		Mame32VerifySampleSet(game_index);
		changed = TRUE;
	}

	lvfi.flags	= LVFI_PARAM;
	lvfi.lParam = game_index;

	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (changed && i != -1);
		ListView_RedrawItems(hwndList, i, i);
	if ((game_index % progBarStep) == 0)
		ProgressBarStep();
	game_index++;

	return changed;
}

static BOOL OnIdle(HWND hWnd)
{
	static int bFirstTime = TRUE;
	static int bResetList = TRUE;

	char *pDescription;
	int driver_index;

	if (bFirstTime)
	{
		bResetList = FALSE;
		bFirstTime = FALSE;
	}
	if (bDoGameCheck)
	{
		bResetList |= GameCheck();
		return idle_work;
	}
	// NPW 17-Jun-2003 - Commenting this out because it is redundant
	// and it causes the game to reset back to the original game after an F5 
	// refresh
	//driver_index = GetGameNameIndex(GetDefaultGame());
	//SetSelectedPickItem(driver_index);

	// in case it's not found, get it back
	driver_index = Picker_GetSelectedItem(hwndList);

	pDescription = ModifyThe(drivers[driver_index]->description);
	SetStatusBarText(0, pDescription);
	idle_work = FALSE;
	UpdateStatusBar();
	bFirstTime = TRUE;

	if (!idle_work)
		PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, TRUE),(LPARAM)NULL);
	return idle_work;
}

static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static BOOL firstTime = TRUE;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (firstTime == FALSE)
		OnSizeSplitter(hMain);
	//firstTime = FALSE;
	/* Update the splitters structures as appropriate */
	RecalcSplitters();
	if (firstTime == FALSE)
		ResizePickerControls(hMain);
	firstTime = FALSE;
	UpdateScreenShot();
}



static HWND GetResizeItemWindow(HWND hParent, const ResizeItem *ri)
{
	HWND hControl;
	if (ri->type == RA_ID)
		hControl = GetDlgItem(hParent, ri->u.id);
	else
		hControl = ri->u.hwnd;
	return hControl;
}



static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hTheFont, BOOL bRedraw)
{
	int i;
	HWND hControl;

	for (i = 0; r->items[i].type != RA_END; i++)
	{
		if (r->items[i].setfont)
		{
			hControl = GetResizeItemWindow(hParent, &r->items[i]);
			SetWindowFont(hControl, hTheFont, bRedraw);
		}
	}
}



static void ResizeWindow(HWND hParent, Resize *r)
{
	int cmkindex = 0, dx, dy, dx1, dtempx;
	HWND hControl;
	RECT parent_rect, rect;
	ResizeItem *ri;
	POINT p = {0, 0};

	if (hParent == NULL)
		return;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	//dx = parent_rect.right - r->rect.right;
	dtempx = parent_rect.right - r->rect.right;
	dy = parent_rect.bottom - r->rect.bottom;
	dx = dtempx/2;
	dx1 = dtempx - dx;
	ClientToScreen(hParent, &p);

	while (r->items[cmkindex].type != RA_END)
	{
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;

		MoveWindow(hControl, rect.left, rect.top,
				   (rect.right - rect.left),
				   (rect.bottom - rect.top), TRUE);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow != NULL)
			ResizeWindow(hControl, ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}

static void ProgressBarShow()
{
	RECT rect;
	int  widths[2] = {150, -1};

	if (game_count < 100)
		progBarStep = 100 / game_count;
	else
		progBarStep = (game_count / 100);

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, game_count));
	SendMessage(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);

	StatusBar_GetItemRect(hStatusBar, 1, &rect);

	MoveWindow(hProgWnd, rect.left, rect.top,
			   rect.right - rect.left,
			   rect.bottom - rect.top, TRUE);

	bProgressShown = TRUE;
}

static void ProgressBarHide()
{
	RECT rect;
	int  widths[4];
	HDC  hDC;
	SIZE size;
	int  numParts = 4;

	if (hProgWnd == NULL)
	{
		return;
	}

    hDC = GetDC(hProgWnd);

	ShowWindow(hProgWnd, SW_HIDE);

	GetTextExtentPoint32(hDC, "MMX", 3 , &size);
	widths[3] = size.cx;
	GetTextExtentPoint32(hDC, "MMMM games", 10, &size);
	widths[2] = size.cx;
	//Just specify 24 instead of 30, gives us sufficient space to display the message, and saves some space
	GetTextExtentPoint32(hDC, "Screen flip support is missing", 24, &size);
	widths[1] = size.cx;

	ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
	UpdateStatusBar();

	bProgressShown = FALSE;
}

static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top, TRUE);
	}
	else
	{
		ProgressBarHide();
	}
}

static void ProgressBarStepParam(int iGameIndex, int nGameCount)
{
	SetStatusBarTextF(0, "Game search %d%% complete",
			((iGameIndex + 1) * 100) / nGameCount);
	if (iGameIndex == 0)
		ShowWindow(hProgWnd, SW_SHOW);
	SendMessage(hProgWnd, PBM_STEPIT, 0, 0);
}

static void ProgressBarStep()
{
	ProgressBarStepParam(game_index, game_count);
}

static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return CreateWindowEx(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			"Progress Bar",
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right	- rect.left,
			rect.bottom - rect.top,
			hParent,
			NULL,
			hInst,
			NULL);
}

static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int   i;
	int   iButton = lpttt->hdr.idFrom;
	static char String[1024];
	BOOL bConverted = FALSE;
	//LPSTR pDest = lpttt->lpszText;
	/* Map command ID to string index */
	for (i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = TRUE;
			break;
		}
	}
	if( bConverted )
	{
		/* Check for valid parameter */
		if(iButton > NUM_TOOLTIPS)
		{
			strcpy(String,"Invalid Button Index");
		}
		else
		{
			strcpy(String,szTbStrings[iButton]);
		}
	}
	else if ( iButton <= 2 )
	{
		//Statusbar
		SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 200);
		if( iButton != 1)
			SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)iButton, (LPARAM)(LPSTR) &String );
		else
			//for first pane we get the Status directly, to get the line breaks
			strcpy(String, GameInfoStatus(Picker_GetSelectedItem(hwndList), FALSE) );
	}
	else
		strcpy(String,"Invalid Button Index");
	//strcpy(pDest, (LPCTSTR)&String);
	lpttt->lpszText = String;
}

static HWND InitToolbar(HWND hParent)
{
	return CreateToolbarEx(hParent,
						   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						   CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						   1,
						   8,
						   hInst,
						   IDB_TOOLBAR,
						   tbb,
						   NUM_TOOLBUTTONS,
						   16,
						   16,
						   0,
						   0,
						   sizeof(TBBUTTON));
}

static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_UI_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
							  CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS,
							  "Ready",
							  hParent,
							  2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags	= (UINT)HIWORD(wParam);
	HMENU hMainMenu = NULL;
	int   iMenu 	= 0;

	/* Handle non-system popup menu descriptions. */
	if (  (fuFlags & MF_POPUP)
	&&	(!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		/* Display helpful text in status bar */
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst,
				 hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst,
				 hStatusBar, &nZero);
	}

	return 0;
}

static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int 		 games_shown = 0;
	int 		 i = -1;

	if (!lpFolder)
		return;

	while (1)
	{
		i = FindGame(lpFolder,i+1);
		if (i == -1)
			break;

		if (!GameFiltered(i, lpFolder->m_dwFlags))
			games_shown++;
	}

	/* Show number of games in the current 'View' in the status bar */
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	i = Picker_GetSelectedItem(hwndList);

	if (games_shown == 0)
		DisableSelection();
	else
	{
		const char* pStatus = GameInfoStatus(i, FALSE);
		SetStatusBarText(1, pStatus);
	}
}

static void UpdateHistory(void)
{
	HDC hDC;
	RECT rect;
	TEXTMETRIC     tm ;
	int nLines, nLineHeight;
	//DWORD dwStyle = GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = FALSE;

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(Picker_GetSelectedItem(hwndList));

		have_history = (histText && histText[0]) ? TRUE : FALSE;
		Edit_SetText(GetDlgItem(hMain, IDC_HISTORY), histText);
	}

	if (have_history && GetShowScreenShot()
		&& ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) || 
			(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ))
	{
		Edit_GetRect(GetDlgItem(hMain, IDC_HISTORY),&rect);
		nLines = Edit_GetLineCount(GetDlgItem(hMain, IDC_HISTORY) );
		hDC = GetDC(GetDlgItem(hMain, IDC_HISTORY));
		GetTextMetrics (hDC, &tm);
		nLineHeight = tm.tmHeight - tm.tmInternalLeading;
		if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) )
		{
			//more than one Page, so show Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY), SB_VERT, 0, nLines, TRUE); 
		}
		else
		{
			//hide Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY),SB_VERT, 0, 0, TRUE);

		}
 		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_SHOW);
	}
	else
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_HIDE);

}


static void DisableSelection()
{
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);
	BOOL			prev_have_selection = have_selection;

	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = (char *)"&Play";
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES,	   MF_GRAYED);

	SetStatusBarText(0, "No Selection");
	SetStatusBarText(1, "");
	SetStatusBarText(3, "");

	have_selection = FALSE;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();
}

static void EnableSelection(int nGame)
{
	char			buf[200];
	const char *	pText;
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);

#ifdef MESS
	MyFillSoftwareList(nGame, FALSE);
#endif

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), g_szPlayGameString, ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));
	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	pText = ModifyThe(drivers[nGame]->description);
	SetStatusBarText(0, pText);
	/* Add this game's status to the status bar */
	pText = GameInfoStatus(nGame, FALSE);
	SetStatusBarText(1, pText);
	SetStatusBarText(3, "");

	/* If doing updating game status and the game name is NOT pacman.... */

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_ENABLED);

	if (!oldControl)
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	if (bProgressShown && bListReady == TRUE)
	{
		SetDefaultGame(ModifyThe(drivers[nGame]->name));
	}
	have_selection = TRUE;

	UpdateScreenShot();
}

static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;
	HDC 		hDC = GetDC(hWnd);
	int 		i, j;
	HDC 		htempDC;
	HBITMAP 	oldBitmap;

	/* x and y are offsets within the background image that should be at 0,0 in hWnd */

	/* So we don't paint over the control's border */
	GetClientRect(hWnd, &rcClient);

	htempDC = CreateCompatibleDC(hDC);
	oldBitmap = SelectObject(htempDC, hBackground);

	if (hRgn == NULL)
	{
		/* create a region of our client area */
		rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);
	}
	else
	{
		/* use the passed in region */
		SelectClipRgn(hDC, hRgn);
	}

	hPAL = GetBackgroundPalette();
	if (hPAL == NULL)
		hPAL = CreateHalftonePalette(hDC);

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, FALSE);
		RealizePalette(htempDC);
	}

	for (i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);
	
	SelectObject(htempDC, oldBitmap);
	DeleteDC(htempDC);
	
	if (GetBackgroundPalette() == NULL)
	{
		DeleteObject(hPAL);
		hPAL = NULL;
	}

	ReleaseDC(hWnd, hDC);
}

static LPCSTR GetCloneParentName(int nItem)
{
	int nParentIndex = -1;

	if (DriverIsClone(nItem) == TRUE)
	{
		nParentIndex = GetParentIndex(drivers[nItem]);
		if( nParentIndex >= 0)
			return ModifyThe(drivers[nParentIndex]->description);
	}
	return "";
}

static BOOL PickerHitTest(HWND hWnd)
{
	RECT			rect;
	POINTS			p;
	DWORD			res = GetMessagePos();
	LVHITTESTINFO	htInfo;

    ZeroMemory(&htInfo,sizeof(LVHITTESTINFO));
	p = MAKEPOINTS(res);
	GetWindowRect(hWnd, &rect);
	htInfo.pt.x = p.x - rect.left;
	htInfo.pt.y = p.y - rect.top;
	ListView_HitTest(hWnd, &htInfo);

	return (! (htInfo.flags & LVHT_NOWHERE));
}

static BOOL TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
	case TVN_SELCHANGED :
	{
		HTREEITEM hti = TreeView_GetSelection(hTreeView);
		TVITEM	  tvi;

		tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
		tvi.hItem = hti;
		
		if (TreeView_GetItem(hTreeView, &tvi))
		{
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			if (bListReady)
			{
				ResetListView();
				UpdateScreenShot();
			}
		}
		return TRUE;
	}
	case TVN_BEGINLABELEDIT :
	{
		TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
		LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

		if (folder->m_dwFlags & F_CUSTOM)
		{
			// user can edit custom folder names
			g_in_treeview_edit = TRUE;
			return FALSE;
		}
		// user can't edit built in folder names
		return TRUE;
	}
	case TVN_ENDLABELEDIT :
	{
		TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
		LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

		g_in_treeview_edit = FALSE;

		if (ptvdi->item.pszText == NULL || strlen(ptvdi->item.pszText) == 0)
			return FALSE;

		return TryRenameCustomFolder(folder,ptvdi->item.pszText);
	}
	}
	return FALSE;
}



static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	// Right button was clicked on header
	HMENU hMenuLoad;
	HMENU hMenu;

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	hMenu = GetSubMenu(hMenuLoad, 0);
	lastColumnClick = nColumn;
	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}



char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
	   for setting text of window controls that don't allow us to disable
	   the ampersand underlining.
	 */
	/* returns a static buffer--use before calling again */

	static char buf[200];
	char *ptr;

	ptr = buf;
	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;
		*ptr++ = *s++;
	}
	*ptr = 0;

	return buf;
}

static int GUI_seq_pressed(input_code* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<SEQ_MAX;++j)
	{
		switch (code[j])
		{
			case CODE_NONE :
				return res && count;
			case CODE_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case CODE_NOT :
				invert = !invert;
				break;
			default:
				if (res)
				{
					int pressed = keyboard_state[code[j]];
					if ((pressed != 0) == invert)
						res = 0;
				}
				invert = 0;
				++count;
		}
	}
	return res && count;
}

static void check_for_GUI_action(void)
{
	int i;

	for (i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		input_seq *is = &(GUISequenceControl[i].is);

		if (GUI_seq_pressed(is->code))
		{
			dprintf("seq =%s pressed\n", GUISequenceControl[i].name);
			switch (GUISequenceControl[i].func_id)
			{
			case ID_GAME_AUDIT:
			case ID_GAME_PROPERTIES:
			case ID_CONTEXT_FILTERS:
			case ID_UI_START:
				KeyboardStateClear(); /* beacuse whe won't receive KeyUp mesage when we loose focus */
				break;
			default:
				break;
			}
			SendMessage(hMain, WM_COMMAND, GUISequenceControl[i].func_id, 0);
		}
	}
}

static void KeyboardStateClear(void)
{
	int i;

	for (i = 0; i < __code_max; i++)
		keyboard_state[i] = 0;

	dprintf("keyboard gui state cleared.\n");
}


static void KeyboardKeyDown(int syskey, int vk_code, int special)
{
	int i, found = 0;
	input_code icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_RALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_RCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_RSHIFT;
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_LALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_LCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_LSHIFT;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < __code_max; i++)
		{
			if ( vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code pressed not found =  %i\n",vk_code);
		//MessageBox(NULL,"keydown message arrived not processed","TitleBox",MB_OK);
		return;
	}
	dprintf("VK_code pressed found =  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special);
	keyboard_state[icode] = 1;
	check_for_GUI_action();
}

static void KeyboardKeyUp(int syskey, int vk_code, int special)
{
	int i, found = 0;
	input_code icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_RALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_RCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_RSHIFT;
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_LALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_LCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_LSHIFT;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < __code_max; i++)
		{
			if (vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code released not found =  %i",vk_code);
		//MessageBox(NULL,"keyup message arrived not processed","TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = 0;
	dprintf("VK_code released found=  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special );
	check_for_GUI_action();
}

static void PollGUIJoystick()
{
	// For the exec timer, will keep track of how long the button has been pressed  
	static int exec_counter = 0;
  
	if (in_emulation)
		return;

	if (g_pJoyGUI == NULL)
		return;

	g_pJoyGUI->poll_joysticks();


	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2),GetUIJoyUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_UP, 0);
	}
	
	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2),GetUIJoyDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_DOWN, 0);
	}
	
	// User pressed LEFT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2),GetUIJoyLeft(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_LEFT, 0);
	}

	// User pressed RIGHT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2),GetUIJoyRight(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_RIGHT, 0);
	}

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2),GetUIJoyStart(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_START, 0);
	}

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2),GetUIJoyPageUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGUP, 0);
	}

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2),GetUIJoyPageDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);
	}

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2),GetUIJoyHome(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HOME, 0);
	}

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2),GetUIJoyEnd(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_END, 0);
	}

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2),GetUIJoySSChange(3))))
	{
		SendMessage(hMain, WM_COMMAND, IDC_SSFRAME, 0);
	}
	
	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2),GetUIJoyHistoryUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);
	}
	
	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2),GetUIJoyHistoryDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);
	}
  
  // User pressed EXECUTE COMMANDLINE
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyExec(0), GetUIJoyExec(1), GetUIJoyExec(2),GetUIJoyExec(3))))
	{
		if (++exec_counter >= GetExecWait()) // Button has been pressed > exec timeout
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			
			// Reset counter
			exec_counter = 0;

			ZeroMemory( &si, sizeof(si) );
			ZeroMemory( &pi, sizeof(pi) );
			si.dwFlags = STARTF_FORCEONFEEDBACK;
			si.cb = sizeof(si);
			
			CreateProcess(NULL, GetExecCommand(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else
	{
		// Button has been released within the timeout period, reset the counter
		exec_counter = 0;
	}
}

static void PressKey(HWND hwnd, UINT vk)
{
	SendMessage(hwnd, WM_KEYDOWN, vk, 0);
	SendMessage(hwnd, WM_KEYUP,   vk, 0xc0000000);
}

static void SetView(int menu_id)
{
	BOOL force_reset = FALSE;
	int i;

	// first uncheck previous menu item, check new one
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_LARGE_ICON, ID_VIEW_GROUPED, menu_id, MF_CHECKED);
	ToolBar_CheckButton(hToolBar, menu_id, MF_CHECKED);

	if (Picker_GetViewID(hwndList) == VIEW_GROUPED || menu_id == ID_VIEW_GROUPED)
	{
		// this changes the sort order, so redo everything
		force_reset = TRUE;
	}

	for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetViewID(GetDlgItem(hMain, s_nPickers[i]), menu_id - ID_VIEW_LARGE_ICON);

	if (force_reset)
	{
		for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
			Picker_Sort(GetDlgItem(hMain, s_nPickers[i]));
	}
}

static void ResetListView()
{
	int 	i;
	int 	current_game;
	LV_ITEM lvi;
	BOOL	no_selection = FALSE;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (!lpFolder)
    {
		return;
    }

	/* If the last folder was empty, no_selection is TRUE */
	if (have_selection == FALSE)
    {
		no_selection = TRUE;
    }

	current_game = Picker_GetSelectedItem(hwndList);

	SetWindowRedraw(hwndList,FALSE);

	ListView_DeleteAllItems(hwndList);

	// hint to have it allocate it all at once
	ListView_SetItemCount(hwndList,game_count);

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;

	i = -1;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem	 = i;
			lvi.iSubItem = 0;
			lvi.lParam	 = i;
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iImage	 = I_IMAGECALLBACK;
			ListView_InsertItem(hwndList, &lvi);
		}
	} while (i != -1);

	Picker_Sort(hwndList);

	if (bListReady)
	{
	    /* If last folder was empty, select the first item in this folder */
	    if (no_selection)
		    Picker_SetSelectedPick(hwndList, 0);
		else
		    Picker_SetSelectedItem(hwndList, current_game);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
		 We now need to set the ViewMode for SmallIcon again,
		 for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_SMALL_ICONS)
		SetView(ID_VIEW_SMALL_ICON);

	SetWindowRedraw(hwndList, TRUE);

	UpdateStatusBar();

}

static void UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit)
{
	int i;

	for (i = 0; i < game_count; i++)
	{
		if (bUpdateRomAudit && DriverUsesRoms(i))
			SetRomAuditResults(i, UNKNOWN);
		if (bUpdateSampleAudit && DriverUsesSamples(i))
			SetSampleAuditResults(i, UNKNOWN);
	}

	idle_work	 = TRUE;
	bDoGameCheck = TRUE;
	game_index	 = 0;

	ReloadIcons();

	// Let REFRESH also load new background if found
	LoadBackgroundBitmap();
	InvalidateRect(hMain,NULL,TRUE);
	Picker_ResetIdle(hwndList);
}

UINT_PTR CALLBACK CFHookProc(
  HWND hdlg,      // handle to dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
{
	int iIndex, i;
	COLORREF cCombo, cList;
	switch (uiMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)"Custom");
			iIndex = SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0);
			cList = GetListFontColor();
			SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex-1,(LPARAM)cList );
			for( i = 0; i< iIndex; i++)
			{
				cCombo = SendDlgItemMessage(hdlg, cmb4, CB_GETITEMDATA,(WPARAM)i,0 );
				if( cList == cCombo)
				{
					SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)i,0 );
					break;
				}
			}
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == cmb4)
			{
				switch (HIWORD(wParam))
				{
					case CBN_SELCHANGE:  // The color ComboBox changed selection
						iIndex = (int)SendDlgItemMessage(hdlg, cmb4,
													  CB_GETCURSEL, 0, 0L);
						if( iIndex == SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0)-1)
						{
							//Custom color selected
 							cList = GetListFontColor();
 							PickColor(&cList);
							SendDlgItemMessage(hdlg, cmb4, CB_DELETESTRING, iIndex, 0);
							SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)"Custom");
							SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex,(LPARAM)cList);
							SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)iIndex,0 );
							return TRUE;
						}
				}
			}
			break;
	}
	return FALSE;
}

static void PickFont(void)
{
	LOGFONT font;
	CHOOSEFONT cf;
	TCHAR szClass[128];
	HWND hWnd;

	GetListFont(&font);
	font.lfQuality = DEFAULT_QUALITY;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.lpfnHook = &CFHookProc;
	cf.rgbColors   = GetListFontColor();
	cf.Flags	   = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_ENABLEHOOK;
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		DeleteObject(hFont);
	hFont = CreateFontIndirect(&font);
	if (hFont != NULL)
	{
		COLORREF textColor = cf.rgbColors;

		if (textColor == RGB(255,255,255))
		{
			textColor = RGB(240, 240, 240);
		}

		SetAllWindowsFont(hMain, &main_resize, hFont, TRUE);

		hWnd = GetWindow(hMain, GW_CHILD);
		while(hWnd)
		{
			if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
			{
				if (!strcmp(szClass, "SysListView32"))
				{
					ListView_SetTextColor(hWnd, textColor);
				}
				else if (!strcmp(szClass, "SysTreeView32"))
				{
					TreeView_SetTextColor(hTreeView, textColor);
				}
			}
			hWnd = GetWindow(hWnd, GW_HWNDNEXT);
		}
		SetListFontColor(cf.rgbColors);
		ResetListView();
	}
}

static void PickColor(COLORREF *cDefault)
{
 	CHOOSECOLOR cc;
 	COLORREF choice_colors[16];
 	int i;
	
 	for (i=0;i<16;i++)
 		choice_colors[i] = GetCustomColor(i);
 
 	cc.lStructSize = sizeof(CHOOSECOLOR);
 	cc.hwndOwner   = hMain;
 	cc.rgbResult   = *cDefault;
 	cc.lpCustColors = choice_colors;
 	cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
 	if (!ChooseColor(&cc))
 		return;
 	for (i=0;i<16;i++)
 		SetCustomColor(i,choice_colors[i]);
 	*cDefault = cc.rgbResult;
}
 
static void PickCloneColor(void)
{
 	COLORREF cClonecolor;
 	cClonecolor = GetListCloneColor();
 	PickColor( &cClonecolor);
 	SetListCloneColor(cClonecolor);
 	InvalidateRect(hwndList,NULL,FALSE);
}
 
static BOOL MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	int i;
	LPTREEFOLDER folder;
	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD:
		MamePlayRecordGame();
		return TRUE;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD_WAVE:
		MamePlayRecordWave();
		return TRUE;

	case ID_FILE_PLAY_RECORD_MNG:
		MamePlayRecordMNG();
		return TRUE;

	case ID_FILE_LOADSTATE :
		MameLoadState();
		return TRUE;

	case ID_FILE_AUDIT:
		AuditDialog(hMain);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_EXIT:
		PostMessage(hMain, WM_CLOSE, 0, 0);
		return TRUE;

	case ID_VIEW_LARGE_ICON:
		SetView(ID_VIEW_LARGE_ICON);
		return TRUE;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON);
		ResetListView();
		return TRUE;

	case ID_VIEW_LIST_MENU:
		SetView(ID_VIEW_LIST_MENU);
		return TRUE;

	case ID_VIEW_DETAIL:
		SetView(ID_VIEW_DETAIL);
		return TRUE;

	case ID_VIEW_GROUPED:
		SetView(ID_VIEW_GROUPED);
		return TRUE;

	/* Arrange Icons submenu */
	case ID_VIEW_BYGAME:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_GAMES);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYDIRECTORY:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_DIRECTORY);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYMANUFACTURER:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_MANUFACTURER);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTIMESPLAYED:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_PLAYED);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTYPE:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_TYPE);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYYEAR:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_YEAR);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_FOLDERS:
		bShowTree = !bShowTree;
		SetShowFolderList(bShowTree);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_PAGETAB:
		bShowTabCtrl = !bShowTabCtrl;
		SetShowTabCtrl(bShowTabCtrl);
		ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		InvalidateRect(hMain,NULL,TRUE);
		break;

		/*
		  Switches to fullscreen mode. No check mark handeling 
		  for this item cause in fullscreen mode the menu won't
		  be visible anyways.
		*/    
	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (!oldControl)
		{
			InitPropertyPageToPage(hInst, hwnd, Picker_GetSelectedItem(hwndList), GetSelectedPickItemIcon(), AUDIT_PAGE, -1, SRC_GAME);
			SaveGameOptions(Picker_GetSelectedItem(hwndList));
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
	   break;

	/* ListView Context Menu */
	case ID_CONTEXT_ADD_CUSTOM:
	{
	    int  nResult;

		nResult = DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CUSTOM_FILE),
								 hMain,AddCustomFileDialogProc,Picker_GetSelectedItem(hwndList));
		SetFocus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
	    RemoveCurrentGameCustomFolder();
		break;
	}

	/* Tree Context Menu */
	case ID_CONTEXT_FILTERS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == TRUE)
			ResetListView();
		SetFocus(hwndList);
		return TRUE;

		// ScreenShot Context Menu
		// select current tab
	case ID_VIEW_TAB_SCREENSHOT :
	case ID_VIEW_TAB_FLYER :
	case ID_VIEW_TAB_CABINET :
	case ID_VIEW_TAB_MARQUEE :
	case ID_VIEW_TAB_TITLE :
	case ID_VIEW_TAB_CONTROL_PANEL :
	case ID_VIEW_TAB_HISTORY:
		if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == FALSE)
			break;

		TabView_SetCurrentTab(hTabCtrl, id - ID_VIEW_TAB_SCREENSHOT);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

		// toggle tab's existence
	case ID_TOGGLE_TAB_SCREENSHOT :
	case ID_TOGGLE_TAB_FLYER :
	case ID_TOGGLE_TAB_CABINET :
	case ID_TOGGLE_TAB_MARQUEE :
	case ID_TOGGLE_TAB_TITLE :
	case ID_TOGGLE_TAB_CONTROL_PANEL :
	case ID_TOGGLE_TAB_HISTORY :
	{
		int toggle_flag = id - ID_TOGGLE_TAB_SCREENSHOT;

		if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == FALSE)
		{
			// attempt to hide the last tab
			// should show error dialog? hide picture area? or ignore?
			break;
		}

		SetShowTab(toggle_flag,!GetShowTab(toggle_flag));

		TabView_Reset(hTabCtrl);

		if (TabView_GetCurrentTab(hTabCtrl) == toggle_flag && GetShowTab(toggle_flag) == FALSE)
		{
			// we're deleting the tab we're on, so go to the next one
			TabView_CalculateNextTab(hTabCtrl);
		}


		// Resize the controls in case we toggled to another history
		// mode (and the history control needs resizing).

		ResizePickerControls(hMain);
		UpdateScreenShot();
    
		TabView_UpdateSelection(hTabCtrl);

		break;
	}

	/* Header Context Menu */
	case ID_SORT_ASCENDING:
		SetSortReverse(FALSE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_SORT_DESCENDING:
		SetSortReverse(TRUE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == TRUE)
			ResetColumnDisplay(FALSE);
		SetFocus(hwndList);
		return TRUE;

	/* View Menu */
	case ID_VIEW_LINEUPICONS:
		if( codeNotify == FALSE)
			ResetListView();
		else
		{
			/*it was sent after a refresh (F5) was done, we only reset the View if "available" is the selected folder
			  as it doesn't affect the others*/
			folder = GetSelectedFolder();
			if( folder )
			{
				if (folder->m_nFolderId == FOLDER_AVAILABLE )
					ResetListView();

			}
		}
		break;

	case ID_GAME_PROPERTIES:
		if (!oldControl)
		{
			//folder = GetSelectedFolder();
			folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
			InitPropertyPage(hInst, hwnd, Picker_GetSelectedItem(hwndList), GetSelectedPickItemIcon(), folder->m_nFolderId, SRC_GAME);
			SaveGameOptions(Picker_GetSelectedItem(hwndList));
#ifdef MESS
			{
				extern BOOL g_bModifiedSoftwarePaths;
				if (g_bModifiedSoftwarePaths) {
					g_bModifiedSoftwarePaths = FALSE;
					MessUpdateSoftwareList();
				}
			}
#endif
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_PROPERTIES:
		if (!oldControl)
		{
			folder = GetSelectedFolder();
			InitPropertyPage(hInst, hwnd, folder->m_nFolderId, GetSelectedFolderIcon(), Picker_GetSelectedItem(hwndList), SRC_FOLDER);
			SaveFolderOptions(folder->m_nFolderId, Picker_GetSelectedItem(hwndList) );
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;
		
	case ID_FOLDER_SOURCEPROPERTIES:
		if (!oldControl)
		{
			folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
			InitPropertyPage(hInst, hwnd, folder->m_nFolderId, GetSelectedFolderIcon(), Picker_GetSelectedItem(hwndList), SRC_FOLDER);
			SaveFolderOptions(folder->m_nFolderId, Picker_GetSelectedItem(hwndList) );
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_VECTORPROPERTIES:
		if (!oldControl)
		{
			folder = GetFolderByID( FOLDER_VECTOR );
			InitPropertyPage(hInst, hwnd, folder->m_nFolderId, GetSelectedFolderIcon(), Picker_GetSelectedItem(hwndList), SRC_FOLDER);
			SaveFolderOptions(folder->m_nFolderId, Picker_GetSelectedItem(hwndList) );
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;
		
	case ID_FOLDER_AUDIT:
		FolderCheck();
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_VIEW_PICTURE_AREA :
		ToggleScreenShot();
		break;

	case ID_UPDATE_GAMELIST:
		UpdateGameList(TRUE, TRUE);
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		return TRUE;

	case ID_OPTIONS_CLONE_COLOR:
		PickCloneColor();
		return TRUE;

	case ID_OPTIONS_DEFAULTS:
		/* Check the return value to see if changes were applied */
		if (!oldControl)
		{
			InitDefaultPropertyPage(hInst, hwnd);
			SaveDefaultOptions();
		}
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_DIR:
		{
			int  nResult;
			BOOL bUpdateRoms;
			BOOL bUpdateSamples;
#ifdef MESS
			BOOL bUpdateSoftware;
#endif

			nResult = DialogBox(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_DIRECTORIES),
								hMain,
								DirectoriesDialogProc);

			SaveDefaultOptions();

			bUpdateRoms    = ((nResult & DIRDLG_ROMS)	 == DIRDLG_ROMS)	? TRUE : FALSE;
			bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? TRUE : FALSE;
#ifdef MESS
			bUpdateSoftware = ((nResult & DIRDLG_SOFTWARE) == DIRDLG_SOFTWARE) ? TRUE : FALSE;

			if (bUpdateSoftware)
				MessUpdateSoftwareList();
#endif /* MESS */

			if (s_pWatcher)
			{
				if (bUpdateRoms)
					DirWatcher_Watch(s_pWatcher, 0, GetRomDirs(), TRUE);
				if (bUpdateSamples)
					DirWatcher_Watch(s_pWatcher, 1, GetSampleDirs(), TRUE);
			}

			/* update game list */
			if (bUpdateRoms == TRUE || bUpdateSamples == TRUE)
				UpdateGameList(bUpdateRoms, bUpdateSamples);

			SetFocus(hwndList);
		}
		return TRUE;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (DialogBox(GetModuleHandle(NULL),
					  MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == TRUE)
        {
			// these may have been changed
			SaveDefaultOptions();
			DestroyWindow(hwnd);
			PostQuitMessage(0);
        }
		return TRUE;

	case ID_OPTIONS_INTERFACE:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INTERFACE_OPTIONS),
				  hMain, InterfaceDialogProc);
		SaveDefaultOptions();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
		{
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds
		}

		return TRUE;

	case ID_OPTIONS_BG:
		{
			OPENFILENAMEA OpenFileName;
			static char szFile[MAX_PATH] = "\0";

			OpenFileName.lStructSize       = sizeof(OPENFILENAME);
			OpenFileName.hwndOwner         = hMain;
			OpenFileName.hInstance         = 0;
			OpenFileName.lpstrFilter       = "Image Files (*.png, *.bmp)\0*.PNG;*.BMP\0";
			OpenFileName.lpstrCustomFilter = NULL;
			OpenFileName.nMaxCustFilter    = 0;
			OpenFileName.nFilterIndex      = 1;
			OpenFileName.lpstrFile         = szFile;
			OpenFileName.nMaxFile          = sizeof(szFile);
			OpenFileName.lpstrFileTitle    = NULL;
			OpenFileName.nMaxFileTitle     = 0;
			OpenFileName.lpstrInitialDir   = GetBgDir();
			OpenFileName.lpstrTitle        = "Select a Background Image";
			OpenFileName.nFileOffset       = 0;
			OpenFileName.nFileExtension    = 0;
			OpenFileName.lpstrDefExt       = NULL;
			OpenFileName.lCustData         = 0;                                                 
			OpenFileName.lpfnHook 		   = NULL;
			OpenFileName.lpTemplateName    = NULL;                                    
			OpenFileName.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			if (GetOpenFileNameA(&OpenFileName))
			{
				ResetBackground(szFile);
				LoadBackgroundBitmap();
				InvalidateRect(hMain, NULL, TRUE);
				return TRUE;
			}
		}
		break;

	case ID_OPTIONS_LANGUAGE:
		DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_LANGUAGE),
				  hMain,
				  LanguageDialogProc);
		return TRUE;

	case ID_HELP_ABOUT:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT),
				  hMain, AboutDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case IDOK :
		/* cmk -- might need to check more codes here, not sure */
		if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
		{
			/* enter key */
			if (g_in_treeview_edit)
			{
				TreeView_EndEditLabelNow(hTreeView, FALSE);
				return TRUE;
			}
			else 
				if (have_selection)
					MamePlayGame();
		}
		break;

	case IDCANCEL : /* esc key */
		if (g_in_treeview_edit)
			TreeView_EndEditLabelNow(hTreeView, TRUE);
		break;

	case IDC_PLAY_GAME :
		if (have_selection)
			MamePlayGame();
		break;

	case ID_UI_START:
		SetFocus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
 		break;

	case ID_UI_DOWN:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
 		break;

	case ID_UI_PGUP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - ListView_GetCountPerPage(hwndList));
 		break;

	case ID_UI_PGDOWN:
		if ( (GetSelectedPick() + ListView_GetCountPerPage(hwndList)) < ListView_GetItemCount(hwndList) )
			Picker_SetSelectedPick(hwndList,  GetSelectedPick() + ListView_GetCountPerPage(hwndList) );
		else
			Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
 		break;

	case ID_UI_HOME:
		Picker_SetSelectedPick(hwndList, 0);
 		break;

	case ID_UI_END:
		Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
		break;
	case ID_UI_LEFT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINELEFT, 0);
 		break;

	case ID_UI_RIGHT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINERIGHT, 0);
 		break;
	case ID_UI_HISTORY_UP:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEUP, 0);
		}
 		break;

	case ID_UI_HISTORY_DOWN:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
		}
 		break;

	case IDC_SSFRAME:
		TabView_CalculateNextTab(hTabCtrl);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;

	case ID_CONTEXT_RESET_PLAYTIME:
		ResetPlayTime( Picker_GetSelectedItem(hwndList) );
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RESET_PLAYCOUNT:
		ResetPlayCount( Picker_GetSelectedItem(hwndList) );
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RENAME_CUSTOM :
		TreeView_EditLabel(hTreeView,TreeView_GetSelection(hTreeView));
		break;

	default:
		if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
		{
			ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
			break;
		}
		for (i = 0; g_helpInfo[i].nMenuItem > 0; i++)
		{
			if (g_helpInfo[i].nMenuItem == id)
			{
				if (g_helpInfo[i].bIsHtmlHelp)
					HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
				else
					DisplayTextFile(hMain, g_helpInfo[i].lpFile);
				return FALSE;
			}
		}
#ifdef MESS
		return MessCommand(hwnd, id, hwndCtl, codeNotify);
#else
		break;
#endif
	}

	return FALSE;
}

static void LoadBackgroundBitmap()
{
	HGLOBAL hDIBbg;
	char*	pFileName = 0;

	if (hBackground)
	{
		DeleteObject(hBackground);
		hBackground = 0;
	}

	if (hPALbg)
	{
		DeleteObject(hPALbg);
		hPALbg = 0;
	}

	/* Pick images based on number of colors avaliable. */
	if (GetDepth(hwndList) <= 8)
	{
		pFileName = (char *)"bkgnd16";
		/*nResource = IDB_BKGROUND16;*/
	}
	else
	{
		pFileName = (char *)"bkground";
		/*nResource = IDB_BKGROUND;*/
	}

	if (LoadDIB(pFileName, &hDIBbg, &hPALbg, BACKGROUND))
	{
		HDC hDC = GetDC(hwndList);
		hBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
		GlobalFree(hDIBbg);
		ReleaseDC(hwndList, hDC);
	}
}

static void ResetColumnDisplay(BOOL first_time)
{
	int driver_index;

	if (!first_time)
		Picker_ResetColumnDisplay(hwndList);

	ResetListView();

	driver_index = GetGameNameIndex(GetDefaultGame());
	Picker_SetSelectedItem(hwndList, driver_index);
}

int GamePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	return GetIconForDriver(nItem);
}

const TCHAR *GamePicker_GetItemString(HWND hwndPicker, int nItem, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength)
{
	const TCHAR *s = NULL;

	switch(nColumn)
	{
		case COLUMN_GAMES:
			/* Driver description */
			s = ModifyThe(drivers[nItem]->description);
			break;

		case COLUMN_ROMS:
			/* Has Roms */
			s = GetAuditString(GetRomAuditResults(nItem));
			break;

		case COLUMN_SAMPLES:
			/* Samples */
			if (DriverUsesSamples(nItem))
				s = GetAuditString(GetSampleAuditResults(nItem));
			break;

		case COLUMN_DIRECTORY:
			/* Driver name (directory) */
			s = drivers[nItem]->name;
			break;

		case COLUMN_SRCDRIVERS:
			/* Source drivers */
			s = GetDriverFilename(nItem);
			break;

        case COLUMN_PLAYTIME:
			/* Source drivers */
			GetTextPlayTime(nItem, pszBuffer);
			s = pszBuffer;
			break;

		case COLUMN_TYPE:
        {
            machine_config drv;
            expand_machine_driver(drivers[nItem]->drv,&drv);

			/* Vector/Raster */
			if (drv.video_attributes & VIDEO_TYPE_VECTOR)
				s = TEXT("Vector");
			else
				s = TEXT("Raster");
			break;
        }
		case COLUMN_TRACKBALL:
			/* Trackball */
			if (DriverUsesTrackball(nItem))
				s = TEXT("Yes");
			else
				s = TEXT("No");
			break;

		case COLUMN_PLAYED:
			/* times played */
			sprintf(pszBuffer, "%i", GetPlayCount(nItem));
			s = pszBuffer;
			break;

		case COLUMN_MANUFACTURER:
			/* Manufacturer */
			s = drivers[nItem]->manufacturer;
			break;

		case COLUMN_YEAR:
			/* Year */
			s = drivers[nItem]->year;
			break;

		case COLUMN_CLONE:
			s = GetCloneParentName(nItem);
			break;
	}
	return s;
}

static void GamePicker_LeavingItem(HWND hwndPicker, int nItem)
{
	// leaving item
	// printf("leaving %s\n",drivers[nItem]->name);
}

static void GamePicker_EnteringItem(HWND hwndPicker, int nItem)
{
	// printf("entering %s\n",drivers[nItem]->name);
	if (g_bDoBroadcast == TRUE)
	{
		ATOM a = GlobalAddAtom(drivers[nItem]->description);
		SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
		GlobalDeleteAtom(a);
	}

	EnableSelection(nItem);
#ifdef MESS
	MessReadMountedSoftware(nItem);
#endif
}

static int GamePicker_FindItemParent(HWND hwndPicker, int nItem)
{
	return GetParentRomSetIndex(drivers[nItem]);
}

/* Initialize the Picker and List controls */
static void InitListView()
{
	LVBKIMAGE bki;
	char path[MAX_PATH];

	static const struct PickerCallbacks s_gameListCallbacks =
	{
		SetSortColumn,				/* pfnSetSortColumn */
		GetSortColumn,				/* pfnGetSortColumn */
		SetSortReverse,				/* pfnSetSortReverse */
		GetSortReverse,				/* pfnGetSortReverse */
		SetViewMode,				/* pfnSetViewMode */
		GetViewMode,				/* pfnGetViewMode */
		SetColumnWidths,			/* pfnSetColumnWidths */
		GetColumnWidths,			/* pfnGetColumnWidths */
		SetColumnOrder,				/* pfnSetColumnOrder */
		GetColumnOrder,				/* pfnGetColumnOrder */
		SetColumnShown,				/* pfnSetColumnShown */
		GetColumnShown,				/* pfnGetColumnShown */
		GetOffsetClones,			/* pfnGetOffsetChildren */

		GamePicker_Compare,			/* pfnCompare */
		MamePlayGame,				/* pfnDoubleClick */
		GamePicker_GetItemString,	/* pfnGetItemString */
		GamePicker_GetItemImage,	/* pfnGetItemImage */
		GamePicker_LeavingItem,		/* pfnLeavingItem */
		GamePicker_EnteringItem,	/* pfnEnteringItem */
		BeginListViewDrag,			/* pfnBeginListViewDrag */
		GamePicker_FindItemParent,	/* pfnFindItemParent */
		OnIdle,							/* pfnIdle */
		GamePicker_OnHeaderContextMenu,	/* pfnOnHeaderContextMenu */
		GamePicker_OnBodyContextMenu	/* pfnOnBodyContextMenu */
	};

	struct PickerOptions opts;

	// subclass the list view
	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_gameListCallbacks;
	opts.nColumnCount = COLUMN_MAX;
	opts.ppszColumnNames = column_names;
	SetupPicker(hwndList, &opts);

	ListView_SetTextBkColor(hwndList, CLR_NONE);
	ListView_SetBkColor(hwndList, CLR_NONE);
	sprintf(path, "%s\\bkground.png", GetBgDir() );
	bki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	bki.pszImage = path;
	if( hBackground )	
		ListView_SetBkImage(hwndList, &bki);

	CreateIcons();

	ResetColumnDisplay(TRUE);

	// Allow selection to change the default saved game
	bListReady = TRUE;
}

static void AddDriverIcon(int nItem,int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	/* if already set to rom or clone icon, we've been here before */
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)drivers[nItem]->name);
	if (hIcon == NULL)
	{
		nParentIndex = GetParentIndex(drivers[nItem]);
		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)drivers[nParentIndex]->name);
			nParentIndex = GetParentIndex(drivers[nParentIndex]);
			if (hIcon == NULL && nParentIndex >= 0)
				hIcon = LoadIconFromFile((char *)drivers[nParentIndex]->name);
		}
	}

	if (hIcon != NULL)
	{
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		if (nIconPos != -1)
			icon_index[nItem] = nIconPos;
	}
	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}

static void DestroyIcons(void)
{
	if (hSmall != NULL)
	{
		ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index != NULL)
	{
		int i;
		for (i=0;i<game_count;i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge != NULL)
	{
		ImageList_Destroy(hLarge);
		hLarge = NULL;
	}

	if (hHeaderImages != NULL)
	{
		ImageList_Destroy(hHeaderImages);
		hHeaderImages = NULL;
	}

}

static void ReloadIcons(void)
{
	HICON hIcon;
	INT i;

	// clear out all the images
	ImageList_Remove(hSmall,-1);
	ImageList_Remove(hLarge,-1);

	if (icon_index != NULL)
	{
		for (i=0;i<game_count;i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	for (i = 0; g_iconData[i].icon_name; i++)
	{
		hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);
		if (hIcon == NULL)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(g_iconData[i].resource));

		ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
	}
}

static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize, dwLength = 512, dwType = REG_SZ;
	TCHAR  szBuffer[512];
	HKEY   hKey;

	/* Get the Key */
	RegOpenKey(HKEY_CURRENT_USER, "Control Panel\\desktop\\WindowMetrics", &hKey);
	/* Save the last size */
	RegQueryValueEx(hKey, "Shell Icon Size", NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	dwSize = atol(szBuffer);
	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}

static DWORD GetShellSmallIconSize(void)
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize < 48)
	{
		if (dwSize < 32)
			dwSize = 16;
		else
			dwSize = 32;
	}
	else
	{
		dwSize = 48;
	}
	return dwSize;
}

// create iconlist for Listview control
static void CreateIcons(void)
{
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	int icon_count;
	DWORD dwStyle;
	int i;

	icon_count = 0;
	while(g_iconData[icon_count].icon_name)
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list
	//
	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	dwStyle = GetWindowLong(hwndList,GWL_STYLE);
	SetWindowLong(hwndList,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(GetShellSmallIconSize(),GetShellSmallIconSize(),
							  ILC_COLORDDB | ILC_MASK, icon_count, 500);
	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize,
							  ILC_COLORDDB | ILC_MASK, icon_count, 500);

	ReloadIcons();

	// Associate the image lists with the list view control.
	ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
	ListView_SetImageList(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	SetWindowLong(hwndList,GWL_STYLE,dwStyle);

#ifdef MESS
	CreateMessIcons();
#endif

	// Now set up header specific stuff
	hHeaderImages = ImageList_Create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_UP));
	ImageList_AddIcon(hHeaderImages,hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_DOWN));
	ImageList_AddIcon(hHeaderImages,hIcon);

	for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetHeaderImageList(GetDlgItem(hMain, s_nPickers[i]), hHeaderImages);
}



static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem)
{
	int value;
	const char *name1 = NULL;
	const char *name2 = NULL;
	int nTemp1, nTemp2;

#ifdef DEBUG
	if (strcmp(drivers[index1]->name,"1941") == 0 && strcmp(drivers[index2]->name,"1942") == 0)
	{
		dprintf("comparing 1941, 1942");
	}

	if (strcmp(drivers[index1]->name,"1942") == 0 && strcmp(drivers[index2]->name,"1941") == 0)
	{
		dprintf("comparing 1942, 1941");
	}
#endif

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
		value = mame_stricmp(ModifyThe(drivers[index1]->description),
						ModifyThe(drivers[index2]->description));
		break;

	case COLUMN_ROMS:
		nTemp1 = GetRomAuditResults(index1);
		nTemp2 = GetRomAuditResults(index2);

		if (IsAuditResultKnown(nTemp1) == FALSE && IsAuditResultKnown(nTemp2) == FALSE)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		if (IsAuditResultKnown(nTemp1) == FALSE)
		{
			value = 1;
			break;
		}

		if (IsAuditResultKnown(nTemp2) == FALSE)
		{
			value = -1;
			break;
		}

		// ok, both are known

		if (IsAuditResultYes(nTemp1) && IsAuditResultYes(nTemp2))
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);
		
		if (IsAuditResultNo(nTemp1) && IsAuditResultNo(nTemp2))
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		if (IsAuditResultYes(nTemp1) && IsAuditResultNo(nTemp2))
			value = -1;
		else
			value = 1;
		break;

	case COLUMN_SAMPLES:
		nTemp1 = -1;
		if (DriverUsesSamples(index1))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp1 = 1;
				else 
					nTemp1 = 0;
			}
			else
				nTemp1 = 2;
		}

		nTemp2 = -1;
		if (DriverUsesSamples(index2))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp2 = 1;
				else 
					nTemp2 = 0;
			}
			else
				nTemp2 = 2;
		}

		if (nTemp1 == nTemp2)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = nTemp2 - nTemp1;
		break;

	case COLUMN_DIRECTORY:
		value = mame_stricmp(drivers[index1]->name, drivers[index2]->name);
		break;

   	case COLUMN_SRCDRIVERS:
		if (mame_stricmp(drivers[index1]->source_file+12, drivers[index2]->source_file+12) == 0)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = mame_stricmp(drivers[index1]->source_file+12, drivers[index2]->source_file+12);
		break;
	case COLUMN_PLAYTIME:
	   value = GetPlayTime(index1) - GetPlayTime(index2);
	   if (value == 0)
		  return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	   break;

	case COLUMN_TYPE:
    {
        machine_config drv1,drv2;
        expand_machine_driver(drivers[index1]->drv,&drv1);
        expand_machine_driver(drivers[index2]->drv,&drv2);

		if ((drv1.video_attributes & VIDEO_TYPE_VECTOR) ==
			(drv2.video_attributes & VIDEO_TYPE_VECTOR))
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = (drv1.video_attributes & VIDEO_TYPE_VECTOR) -
				(drv2.video_attributes & VIDEO_TYPE_VECTOR);
		break;
    }
	case COLUMN_TRACKBALL:
		if (DriverUsesTrackball(index1) == DriverUsesTrackball(index2))
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = DriverUsesTrackball(index1) - DriverUsesTrackball(index2);
		break;

	case COLUMN_PLAYED:
	   value = GetPlayCount(index1) - GetPlayCount(index2);
	   if (value == 0)
		  return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	   break;

	case COLUMN_MANUFACTURER:
		if (mame_stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer) == 0)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = mame_stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer);
		break;

	case COLUMN_YEAR:
		if (mame_stricmp(drivers[index1]->year, drivers[index2]->year) == 0)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		value = mame_stricmp(drivers[index1]->year, drivers[index2]->year);
		break;

	case COLUMN_CLONE:
		name1 = GetCloneParentName(index1);
		name2 = GetCloneParentName(index2);

		if (*name1 == '\0')
			name1 = NULL;
		if (*name2 == '\0')
			name2 = NULL;

		if (name1 == name2)
			return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

		if (name2 == NULL)
			value = -1;
		else if (name1 == NULL)
			value = 1;
		else
			value = mame_stricmp(name1, name2);
		break;

	default :
		return GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);
	}

#ifdef DEBUG
	if ((strcmp(drivers[index1]->name,"1941") == 0 && strcmp(drivers[index2]->name,"1942") == 0) ||
		(strcmp(drivers[index1]->name,"1942") == 0 && strcmp(drivers[index2]->name,"1941") == 0))
		dprintf("result: %i",value);
#endif

	return value;
}

static int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}

static HICON GetSelectedPickItemIcon()
{
	LV_ITEM lvi;

	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	ListView_GetItem(hwndList, &lvi);

	return ImageList_GetIcon(hLarge, lvi.iImage, ILD_TRANSPARENT);
}

static void SetRandomPickItem()
{
	int nListCount;

	nListCount = ListView_GetItemCount(hwndList);

	if (nListCount > 0)
	{
		Picker_SetSelectedPick(hwndList, rand() % nListCount);
	}
}

BOOL CommonFileDialog(common_file_dialog_proc cfd, char *filename, int filetype)
{
	BOOL success;
	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hMain;
	of.hInstance         = NULL;
	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		of.lpstrFilter   = MAMENAME " input files (*.inp,*.zip)\0*.inp;*.zip\0All files (*.*)\0*.*\0";
		break;
	case FILETYPE_SAVESTATE_FILES :
		of.lpstrFilter   = MAMENAME " savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0";
		break;
	case FILETYPE_WAVE_FILES :
		of.lpstrFilter   = "sounds (*.wav)\0*.wav;\0All files (*.*)\0*.*\0";
		break;
	case FILETYPE_MNG_FILES :
		of.lpstrFilter   = "videos (*.mng)\0*.mng;\0All files (*.*)\0*.*\0";
		break;
	case FILETYPE_EFFECT_FILES :
		of.lpstrFilter   = "effects (*.png)\0*.png;\0All files (*.*)\0*.*\0";
		break;
	}
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	if (filetype == FILETYPE_SAVESTATE_FILES)
		of.lpstrInitialDir = GetStateDir();
	else
	{
		if (filetype == FILETYPE_EFFECT_FILES)
			of.lpstrInitialDir = GetArtDir();
		else
			of.lpstrInitialDir   = last_directory;
	}
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		of.lpstrDefExt       = "inp";
		break;
	case FILETYPE_SAVESTATE_FILES :
		of.lpstrDefExt       = "sta";
		break;
	case FILETYPE_WAVE_FILES :
		of.lpstrDefExt       = "wav";
		break;
	case FILETYPE_MNG_FILES :
		of.lpstrDefExt       = "mng";
		break;
	case FILETYPE_EFFECT_FILES :
		of.lpstrDefExt       = "png";
		break;
	}
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	success = cfd(&of);
	if (success)
	{
		//dprintf("got filename %s nFileExtension %u\n",filename,of.nFileExtension);
		/*GetDirectory(filename,last_directory,sizeof(last_directory));*/
	}

	return success;
}

static BOOL SelectLanguageFile(HWND hWnd, TCHAR* filename)
{
	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hWnd;
	of.hInstance         = NULL;
	of.lpstrFilter       = MAMENAME " Language files (*.lng)\0*.lng\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	of.lpstrInitialDir   = NULL;
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	of.lpstrDefExt       = "lng";
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	return GetOpenFileName(&of);
}

static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR pLangFile[MAX_PATH];
	DWORD dwHelpIDs[] = { IDC_LANGUAGECHECK, HIDC_LANGUAGECHECK,
						  IDC_LANGUAGEEDIT,  HIDC_LANGUAGEEDIT,
						  0, 0};

	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			const char* pLang = GetLanguage();
			if (pLang == NULL || *pLang == '\0')
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), FALSE);
			}
			else
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLang);
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), TRUE);
			}

			return TRUE;
		}

	case WM_HELP:
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, (DWORD)dwHelpIDs);
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, (DWORD)dwHelpIDs);
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			SetLanguage(pLangFile);

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		case IDC_LANGUAGECHECK:
		{
			BOOL bCheck = Button_GetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK));
			EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   bCheck);
			EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), bCheck);
			if (bCheck == FALSE)
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
			return TRUE;
		}

		case IDC_LANGUAGEBROWSE:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			if (SelectLanguageFile(hDlg, pLangFile) == TRUE)
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile);
			break;
		}
		break;
	}
	return 0;
}

void SetStatusBarText(int part_index, const char *message)
{
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM) part_index, (LPARAM) message);
}

void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	SetStatusBarText(part_index, buf);
}

static void CLIB_DECL MameMessageBox(const char *fmt, ...)
{
	char buf[2048];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	MessageBox(GetMainWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);
	va_end(va);
}

static void MamePlayBackGame()
{
	int nGame;
	char filename[MAX_PATH];

	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_INPUT_FILES))
	{
		mame_file_error fileerr;
		mame_file* pPlayBack;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char bare_fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		char path[MAX_PATH];
		char fname[MAX_PATH];

		_splitpath(filename, drive, dir, bare_fname, ext);

		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		options_set_string(SEARCHPATH_INPUTLOG, path);
		fileerr = mame_fopen(SEARCHPATH_INPUTLOG, fname, OPEN_FLAG_READ, &pPlayBack);
		if (fileerr != FILERR_NONE)
		{
			MameMessageBox("Could not open '%s' as a valid input file.", filename);
			return;
		}

		// check for game name embedded in .inp header
		if (pPlayBack)
		{
			inp_header input_header;

			// read playback header
			mame_fread(pPlayBack, &input_header, sizeof(inp_header));

			if (!isalnum(input_header.name[0])) // If first byte is not alpha-numeric
				mame_fseek(pPlayBack, 0, SEEK_SET); // old .inp file - no header
			else
			{
				int i;
				for (i = 0; drivers[i] != 0; i++) // find game and play it
				{
					if (strcmp(drivers[i]->name, input_header.name) == 0)
					{
						nGame = i;
						break;
					}
				}
			}
		}
		mame_fclose(pPlayBack);

		g_pPlayBkName = fname;
		override_playback_directory = path;
		MamePlayGameWithOptions(nGame);
		g_pPlayBkName = NULL;
		override_playback_directory = NULL;
	}
}

static void MameLoadState()
{
	int nGame;
	char filename[MAX_PATH];
	char selected_filename[MAX_PATH];

	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
	{
		strcpy(filename, drivers[nGame]->name);
		strcpy(selected_filename, drivers[nGame]->name);
	}
	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_SAVESTATE_FILES))
	{
		mame_file* pSaveState;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char ext[_MAX_EXT];

		char path[MAX_PATH];
		char fname[MAX_PATH];
		char bare_fname[_MAX_FNAME];
		char *state_fname;
		int rc;

		_splitpath(filename, drive, dir, bare_fname, ext);

		// parse path
		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

#ifdef MESS
		{
			state_fname = filename;
		}
#else // !MESS
		{
			char *cPos=0;
			int  iPos=0;
			char romname[MAX_PATH];

			cPos = strchr(bare_fname, '-' );
			iPos = cPos ? cPos - bare_fname : strlen(bare_fname);
			strncpy(romname, bare_fname, iPos );
			romname[iPos] = '\0';
			if( strcmp(selected_filename,romname) != 0 )
			{
				MameMessageBox("'%s' is not a valid savestate file for game '%s'.", filename, selected_filename);
				return;
			}
			options_set_string(OPTION_STATE_DIRECTORY, path);
			state_fname = fname;
		}
#endif // MESS

		mame_fopen(SEARCHPATH_STATE, state_fname, OPEN_FLAG_READ, &pSaveState);
		if (pSaveState == NULL)
		{
			MameMessageBox("Could not open '%s' as a valid savestate file.", filename);
			return;
		}

		// call the MAME core function to check the save state file
		rc = state_save_check_file(pSaveState, selected_filename, TRUE, MameMessageBox);
		mame_fclose(pSaveState);
		if (rc)
			return;

#ifdef MESS
		g_pSaveStateName = state_fname;
#else
		{
			char *cPos;
			cPos = strrchr(bare_fname, '-' );
			cPos = cPos+1;
			if( strlen(cPos) >0)
			{
				g_pSaveStateName = cPos;
				override_savestate_directory = path;
			}
		}
#endif

		MamePlayGameWithOptions(nGame);
		g_pSaveStateName = NULL;
		override_savestate_directory = NULL;
	}
}

static void MamePlayRecordGame()
{
	int  nGame;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_INPUT_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		g_pRecordName = fname;
		override_playback_directory = path;
		MamePlayGameWithOptions(nGame);
		g_pRecordName = NULL;
		override_playback_directory = NULL;
	}
}

static void MamePlayGame(void)
{
	int nGame;

	nGame = Picker_GetSelectedItem(hwndList);

	g_pPlayBkName = NULL;
	g_pRecordName = NULL;

	MamePlayGameWithOptions(nGame);
}

static void MamePlayRecordWave()
{
	int  nGame;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_WAVE_FILES))
	{
		g_pRecordWaveName = filename;
		MamePlayGameWithOptions(nGame);
		g_pRecordWaveName = NULL;
	}	
}

static void MamePlayRecordMNG()
{
	int  nGame;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_MNG_FILES))
	{
		g_pRecordMNGName = filename;
		MamePlayGameWithOptions(nGame);
		g_pRecordMNGName = NULL;
	}	
}

static void MameGetErrorText(DWORD dwExitCode, LPTSTR pszBuffer, UINT nBufferLen)
{
	switch(dwExitCode)
	{
		case 1:
			_sntprintf(pszBuffer, nBufferLen, TEXT("Initialization error"));
			break;

		case 0xC0000005:
			_sntprintf(pszBuffer, nBufferLen, TEXT("Access violation"));
			break;

		default:
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwExitCode,
				0, pszBuffer, nBufferLen, NULL) == 0)
			{
				_sntprintf(pszBuffer, nBufferLen, TEXT("Error 0x%08X"), dwExitCode);
			}
			break;
	}
}

static void MamePlayGameWithOptions(int nGame)
{
	DWORD dwExitCode;
	TCHAR szBuffer[256];
	TCHAR szError[256];

#ifdef MESS
	if (!MessApproveImageList(hMain, nGame))
		return;
#endif

	memcpy(&playing_game_options, GetGameOptions(nGame, -1), sizeof(options_type));

	/* Deal with options that can be disabled. */
	EnablePlayOptions(nGame, &playing_game_options);

	if (g_pJoyGUI != NULL)
		KillTimer(hMain, JOYGUI_TIMER);
	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	g_bAbortLoading = FALSE;

	in_emulation = TRUE;

	dwExitCode = RunMAME(nGame);
	if (dwExitCode == 0)
	{
		IncrementPlayCount(nGame);
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
	}
	else
	{
		ShowWindow(hMain, SW_SHOW);

		// attempt to display a nice error message
		MameGetErrorText(dwExitCode, szError, sizeof(szError) / sizeof(szError[0]));
		_sntprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]),
			TEXT(MAME32NAME " encountered a fatal error: %s"),
			szError);
		MessageBox(hMain, szBuffer, MAME32NAME, MB_OK);
	}

	in_emulation = FALSE;

	// re-sort if sorting on # of times played
	if (GetSortColumn() == COLUMN_PLAYED)
		Picker_Sort(hwndList);

	UpdateStatusBar();

	ShowWindow(hMain, SW_SHOW);
	SetFocus(hwndList);

	if (g_pJoyGUI != NULL)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
}

/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot(void)
{
	BOOL showScreenShot = GetShowScreenShot();

	SetShowScreenShot((showScreenShot) ? FALSE : TRUE);
	UpdateScreenShot();

	/* Redraw list view */
	if (hBackground != NULL && showScreenShot)
		InvalidateRect(hwndList, NULL, FALSE);
}

static void AdjustMetrics(void)
{
	HDC hDC;
	TEXTMETRIC tm;
	int xtraX, xtraY;
	AREA area;
	int  offX, offY;
	int  maxX, maxY;
	COLORREF textColor;
	TCHAR szClass[128];
	HWND hWnd;

	/* WM_SETTINGCHANGE also */
	xtraX  = GetSystemMetrics(SM_CXFIXEDFRAME); /* Dialog frame width */
	xtraY  = GetSystemMetrics(SM_CYFIXEDFRAME); /* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE);	/* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION);	/* Caption Height */
	maxX   = GetSystemMetrics(SM_CXSCREEN); 	/* Screen Width */
	maxY   = GetSystemMetrics(SM_CYSCREEN); 	/* Screen Height */

	hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);

	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight 	  / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	hWnd = GetWindow(hMain, GW_CHILD);
	while(hWnd)
	{
		if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
		{
			if (!strcmp(szClass, "SysListView32"))
			{
				ListView_SetBkColor(hWnd, GetSysColor(COLOR_WINDOW));
				ListView_SetTextColor(hWnd, textColor);
			}
			else if (!strcmp(szClass, "SysTreeView32"))
			{
				TreeView_SetBkColor(hTreeView, GetSysColor(COLOR_WINDOW));
				TreeView_SetTextColor(hTreeView, textColor);
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}

	GetWindowArea(&area);

	offX = area.x + area.width;
	offY = area.y + area.height;

	if (offX > maxX)
	{
		offX = maxX;
		area.x = (offX - area.width > 0) ? (offX - area.width) : 0;
	}
	if (offY > maxY)
	{
		offY = maxY;
		area.y = (offY - area.height > 0) ? (offY - area.height) : 0;
	}

	SetWindowArea(&area);
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
}


/* Adjust options - tune them to the currently selected game */
static void EnablePlayOptions(int nIndex, options_type *o)
{
	if (!DIJoystick.Available())
		o->use_joystick = FALSE;
}

static int FindIconIndex(int nIconResource)
{
	int i;
	for(i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

static int GetIconForDriver(int nItem)
{
	int iconRoms;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
		if (IsAuditResultKnown(audit_result) == FALSE)
			return 2;

		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
	}
	else
		iconRoms = 1;

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	/* these are indices into icon_names, which maps into our image list
	 * also must match IDI_WIN_NOROMS + iconRoms
     */

	// Show Red-X if the ROMs are present and flagged as NOT WORKING
	if (iconRoms == 1 && DriverIsBroken(nItem))
		iconRoms = FindIconIndex(IDI_WIN_REDX);

	// show clone icon if we have roms and game is working
	if (iconRoms == 1 && DriverIsClone(nItem))
		iconRoms = FindIconIndex(IDI_WIN_CLONE);

	// if we have the roms, then look for a custom per-game icon to override
	if (iconRoms == 1 || iconRoms == 3)
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}

static BOOL HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hTreeMenu;
	HMENU hMenu;
	TVHITTESTINFO hti;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	/* select the item that was right clicked or shift-F10'ed */
	hti.pt = pt;
	ScreenToClient(hTreeView,&hti.pt);
	TreeView_HitTest(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		TreeView_SelectItem(hTreeView,hti.hItem);

	hTreeMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	hMenu = GetSubMenu(hTreeMenu, 0);
	
	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hTreeMenu);

	return TRUE;
}



static void GamePicker_OnBodyContextMenu(POINT pt)
{
	HMENU hMenuLoad;
	HMENU hMenu;

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	hMenu = GetSubMenu(hMenuLoad, 0);
	InitBodyContextMenu(hMenu);
	
	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}



static BOOL HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_SSPICTURE) && (HWND)wParam != GetDlgItem(hWnd, IDC_SSFRAME))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SCREENSHOT));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static void UpdateMenu(HMENU hMenu)
{
	char			buf[200];
	MENUITEMINFO	mItem;
	int 			nGame = Picker_GetSelectedItem(hwndList);
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int i;
	const char *pParent;

	if (have_selection)
	{
		snprintf(buf, sizeof(buf), g_szPlayGameString,
				 ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));

		mItem.cbSize	 = sizeof(mItem);
		mItem.fMask 	 = MIIM_TYPE;
		mItem.fType 	 = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch		 = strlen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mItem);

		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, 			MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (oldControl)
	{
		EnableMenuItem(hMenu, ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,  MF_GRAYED);
		EnableMenuItem(hMenu, ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
	    EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
	    EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if( (strcmp(lpFolder->m_lpTitle, "Vector" ) == 0) ||
		(pParent && (strcmp(pParent, "Source" ) == 0) ) )
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	CheckMenuRadioItem(hMenu, ID_VIEW_TAB_SCREENSHOT, ID_VIEW_TAB_HISTORY,
					   ID_VIEW_TAB_SCREENSHOT + TabView_GetCurrentTab(hTabCtrl), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_UNCHECKED);


	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items 
		if (GetShowTab(i))
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (GetShowFolder(i))
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

}

void InitTreeContextMenu(HMENU hTreeMenu)
{
	MENUITEMINFO mii;
	HMENU hMenu;
	int i;
	extern FOLDERDATA g_folderData[];

	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	hMenu = GetSubMenu(hTreeMenu, 0);

	if (GetMenuItemInfo(hMenu,3,TRUE,&mii) == FALSE)
	{
		dprintf("can't find show folders context menu");
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		dprintf("can't find submenu for show folders context menu");
		return;
	}

	hMenu = mii.hSubMenu;
	
	for (i=0;g_folderData[i].m_lpTitle != NULL;i++)
	{
		mii.fMask = MIIM_TYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.dwTypeData = (char *)g_folderData[i].m_lpTitle;
		mii.cch = strlen(mii.dwTypeData);
		mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;


		// menu in resources has one empty item (needed for the submenu to setup properly)
		// so overwrite this one, append after
		if (i == 0)
			SetMenuItemInfo(hMenu,ID_CONTEXT_SHOW_FOLDER_START,FALSE,&mii);
		else
			InsertMenuItem(hMenu,i,FALSE,&mii);
	}

}

void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	LPTREEFOLDER lpFolder;
	char tmp[30];
	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	if (GetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii) == FALSE)
	{
		dprintf("can't find show folders context menu");
		return;
	}
	lpFolder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
	snprintf(tmp,sizeof(tmp),"Properties for %s",lpFolder->m_lpTitle );
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = tmp;
	mii.cch = strlen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;


	// menu in resources has one default item
	// so overwrite this one
	SetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii);
	if( ! DriverIsVector(Picker_GetSelectedItem(hwndList) ) )
		EnableMenuItem(hBodyContextMenu, ID_FOLDER_VECTORPROPERTIES, MF_GRAYED);
}


void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();

	SetWindowRedraw(hwndList,FALSE);

	SetShowFolder(folder,!GetShowFolder(folder));

	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);

	SetWindowRedraw(hwndList,TRUE);
}

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hBackground)
	{
		switch (uMsg)
		{
	    case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}
    
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
		{
			POINT p = { 0, 0 };
  			
			/* get base point of background bitmap */
			MapWindowPoints(hWnd,hTreeView,&p,1);
			PaintBackgroundImage(hWnd, NULL, p.x, p.y);
			/* to ensure our parent procedure repaints the whole client area */
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		}
		}
	}
	return CallWindowProc(g_lpHistoryWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
    {
		if (MouseHasBeenMoved())
			ShowCursor(TRUE);
		break;
    }
  
    case WM_NCHITTEST :
	{
		POINT pt;
		RECT  rect;
		HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		GetWindowRect(hHistory, &rect);
		// check if they clicked on the picture area (leave 6 pixel no man's land
		// by the history window to reduce mistaken clicks)
		// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback
		if (have_history &&        
			( ( (TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) || 
			 (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ) &&
//			  (rect.top - 6) < pt.y && pt.y < (rect.bottom + 6) ) )
			  		PtInRect( &rect, pt ) ) )

		{
			return HTTRANSPARENT;
		}
		else
		{
			return HTCLIENT;
		}
	}
	break;
	case WM_CONTEXTMENU:
		if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;
	}

	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND :
			return TRUE;
		case WM_PAINT :
		{
			RECT rect,nodraw_rect;
			HRGN region,nodraw_region;
			POINT p = { 0, 0 };

			/* get base point of background bitmap */
			MapWindowPoints(hWnd,hTreeView,&p,1);

			/* get big region */
			GetClientRect(hWnd,&rect);
			region = CreateRectRgnIndirect(&rect);

			if (IsWindowVisible(GetDlgItem(hMain,IDC_HISTORY)))
			{
				/* don't draw over this window */
				GetWindowRect(GetDlgItem(hMain,IDC_HISTORY),&nodraw_rect);
				MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
				nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
				CombineRgn(region,region,nodraw_region,RGN_DIFF);
				DeleteObject(nodraw_region);
			}
			if (IsWindowVisible(GetDlgItem(hMain,IDC_SSPICTURE)))
			{
				/* don't draw over this window */
				GetWindowRect(GetDlgItem(hMain,IDC_SSPICTURE),&nodraw_rect);
				MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
				nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
				CombineRgn(region,region,nodraw_region,RGN_DIFF);
				DeleteObject(nodraw_region);
			}

			PaintBackgroundImage(hWnd,region,p.x,p.y);

			DeleteObject(region);

			/* to ensure our parent procedure repaints the whole client area */
			InvalidateRect(hWnd, NULL, FALSE);

			break;
		}
		}
	}
	return CallWindowProc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND :
		return TRUE;
	case WM_PAINT :
	{
		PAINTSTRUCT ps;
		HDC	hdc,hdc_temp;
		RECT rect;
		HBITMAP old_bitmap;

		int width,height;

		RECT rect2;
		HBRUSH hBrush;
		HBRUSH holdBrush;
		HRGN region1, region2;
		int nBordersize;
		nBordersize = GetScreenshotBorderSize();
		hBrush = CreateSolidBrush(GetScreenshotBorderColor());

		hdc = BeginPaint(hWnd,&ps);

		hdc_temp = CreateCompatibleDC(hdc);
		if (ScreenShotLoaded())
		{
			width = GetScreenShotWidth();
			height = GetScreenShotHeight();

			old_bitmap = SelectObject(hdc_temp,GetScreenShotHandle());
		}
		else
		{
			BITMAP bmp;

			GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);
			width = bmp.bmWidth;
			height = bmp.bmHeight;

			old_bitmap = SelectObject(hdc_temp,hMissing_bitmap);
		}

		GetClientRect(hWnd,&rect);

		rect2 = rect;
		//Configurable Borders around images
		rect.bottom -= nBordersize;
		if( rect.bottom < 0)
			rect.bottom = rect2.bottom;
		rect.right -= nBordersize;
		if( rect.right < 0)
			rect.right = rect2.right;
		rect.top += nBordersize;
		if( rect.top > rect.bottom )
			rect.top = rect2.top;
		rect.left += nBordersize;
		if( rect.left > rect.right )
			rect.left = rect2.left;
		region1 = CreateRectRgnIndirect(&rect);
		region2 = CreateRectRgnIndirect(&rect2);
		CombineRgn(region2,region2,region1,RGN_DIFF);
		holdBrush = SelectObject(hdc, hBrush); 

		FillRgn(hdc,region2, hBrush );
		SelectObject(hdc, holdBrush); 
		DeleteObject(hBrush); 

		SetStretchBltMode(hdc,STRETCH_HALFTONE);
		StretchBlt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top,
				   hdc_temp,0,0,width,height,SRCCOPY);
		SelectObject(hdc_temp,old_bitmap);
		DeleteDC(hdc_temp);

		EndPaint(hWnd,&ps);

		return TRUE;
	}
	}

	return CallWindowProc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}

// replaces function in src/windows/fileio.c:
int osd_display_loading_rom_message(const char *name, rom_load_data *romdata)
{
	int retval = 0;
/*
	if (use_gui_romloading)
	{
		options.gui_host = 1;
		retval = UpdateLoadProgress(name,romdata);
	}
	else
	{
		if (name)
			fprintf (stdout, "loading %-12s\r", name);
		else
			fprintf (stdout, "                    \r");
		fflush (stdout);
		retval = 0;
	}
*/	
	return retval;
}

static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG :
	{
		char buf[256];
		
		sprintf(buf, "Loading %s", Machine->gamedrv->description);
		SetWindowText(hDlg, buf);
		
		g_bCloseLoading = FALSE;
		g_bAbortLoading = FALSE;

		return 1;
	}

	case WM_CLOSE:
		if (!g_bCloseLoading)
			g_bAbortLoading = TRUE;
		EndDialog(hDlg, 0);
		return 1;

	case WM_COMMAND:
	   if (LOWORD(wParam) == IDCANCEL)
	   {
		   g_bAbortLoading = TRUE;
		   EndDialog(hDlg, IDCANCEL);
		   return 1;
	   }
	   if (LOWORD(wParam) == IDOK)
	   {
		   g_bCloseLoading = TRUE;
		   EndDialog(hDlg, IDOK);
		   return 1;
	   }
	}
	return 0;
}

int UpdateLoadProgress(const char* name, const rom_load_data *romdata)
{
	static HWND hWndLoad = 0;
	MSG Msg;

	int current = romdata->romsloaded;
	int total = romdata->romstotal;

	//dprintf("updateloadprogress %s %u %u %08x\n",name,current,total,hWndLoad);

	if (hWndLoad == NULL)
	{
		hWndLoad = CreateDialog(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_LOAD_PROGRESS),
								hMain,
								LoadProgressDialogProc);

		EnableWindow(GetDlgItem(hWndLoad,IDOK),FALSE);
		EnableWindow(GetDlgItem(hWndLoad,IDCANCEL),TRUE);

		ShowWindow(hWndLoad,SW_SHOW);
	}

	SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_STATUS),
				  ConvertToWindowsNewlines(romdata->errorbuf));

	SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, total));
	SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETPOS, current, 0);

	if (name == NULL)
	{
		// final call to us
		SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_ROMNAME), "");
		if (romdata->errors > 0 )
		{
			
			/*
			  Shows the load progress dialog if there is an error while
			  loading the game.
			*/

			ShowWindow(hWndLoad, SW_SHOW);
      
			EnableWindow(GetDlgItem(hWndLoad,IDOK),TRUE);
			if (romdata->errors != 0)
				EnableWindow(GetDlgItem(hWndLoad,IDCANCEL),FALSE);
			SetFocus(GetDlgItem(hWndLoad,IDOK));
			if (romdata->errors)
				SetWindowText(GetDlgItem(hWndLoad,IDC_ERROR_TEXT),
							  "ERROR: required files are missing, the game cannot be run.");
		}
	}
	else
		SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_ROMNAME), name);

	if (name == NULL && (romdata->errors > 0 ))
	{
		while (GetMessage(&Msg, NULL, 0, 0))
		{
			if (!IsDialogMessage(hWndLoad, &Msg))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}

			// make sure the user clicks-through on an error/warning
			if (g_bCloseLoading || g_bAbortLoading)
				break;
		}

	}

	if (name == NULL)
	{
		DestroyWindow(hWndLoad);
		hWndLoad = NULL;
	}

	// take care of any pending messages
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!IsDialogMessage(hWndLoad, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	// if abort with a warning, gotta exit abruptly
	if (name == NULL && g_bAbortLoading && romdata->errors == 0)
		return 1;

	// if abort along the way, tell 'em
	if (g_bAbortLoading && name != NULL)
	{
		return 1;
	}

	return 0;
}

void RemoveCurrentGameCustomFolder(void)
{
    RemoveGameCustomFolder(Picker_GetSelectedItem(hwndList));
}

void RemoveGameCustomFolder(int driver_index)
{
    int i;
	TREEFOLDER **folders;
	int num_folders;

	GetFolders(&folders,&num_folders);
	
	for (i=0;i<num_folders;i++)
	{
	    if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
		    int current_pick_index;

		    RemoveFromCustomFolder(folders[i],driver_index);

			if (driver_index == Picker_GetSelectedItem(hwndList))
			{
			   /* if we just removed the current game,
				  move the current selection so that when we rebuild the listview it
				  leaves the cursor on next or previous one */
			
			   current_pick_index = GetSelectedPick();
			   Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
			   if (current_pick_index == GetSelectedPick()) /* we must have deleted the last item */
				  Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
			}

			ResetListView();
			return;
		}
	}
	MessageBox(GetMainWindow(), "Error searching for custom folder", MAME32NAME, MB_OK | MB_ICONERROR);

}


void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
    LV_ITEM lvi;
	POINT pt;

	lvi.iItem = pnmv->iItem;
	lvi.mask	 = LVIF_PARAM;
	ListView_GetItem(hwndList, &lvi);

	game_dragged = lvi.lParam;

	pt.x = 0;
	pt.y = 0;

	/* Tell the list view control to create an image to use 
	   for dragging. */
    himl_drag = ListView_CreateDragImage(hwndList,pnmv->iItem,&pt);
 
    /* Start the drag operation. */
    ImageList_BeginDrag(himl_drag, 0, 0, 0); 

	pt = pnmv->ptAction;
	ClientToScreen(hwndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(),pt.x,pt.y);

    /* Hide the mouse cursor, and direct mouse input to the 
	   parent window. */
    SetCapture(hMain);

	prev_drag_drop_target = NULL;

    g_listview_dragging = TRUE; 

}

void MouseMoveListViewDrag(POINTS p)
{
   HTREEITEM htiTarget;
   TV_HITTESTINFO tvht;

   POINT pt;
   pt.x = p.x;
   pt.y = p.y;

   ClientToScreen(hMain,&pt);

   ImageList_DragMove(pt.x,pt.y);

   MapWindowPoints(GetDesktopWindow(),hTreeView,&pt,1);

   tvht.pt = pt;
   htiTarget = TreeView_HitTest(hTreeView,&tvht);

   if (htiTarget != prev_drag_drop_target)
   {
	   ImageList_DragShowNolock(FALSE);
	   if (htiTarget != NULL)
		   TreeView_SelectDropTarget(hTreeView,htiTarget);
	   else
		   TreeView_SelectDropTarget(hTreeView,NULL);
	   ImageList_DragShowNolock(TRUE);
	   
	   prev_drag_drop_target = htiTarget;
   }
}

void ButtonUpListViewDrag(POINTS p)
{
    POINT pt;
    HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	TVITEM tvi;
	
	ReleaseCapture();

    ImageList_DragLeave(hwndList);
    ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	TreeView_SelectDropTarget(hTreeView,NULL);

	g_listview_dragging = FALSE;

	/* see where the game was dragged */

	pt.x = p.x;
	pt.y = p.y;

	MapWindowPoints(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);
	if (htiTarget == NULL)
	{
	   LVHITTESTINFO lvhtti;
	   LPTREEFOLDER folder;
	   RECT rcList;

	   /* the user dragged a game onto something other than the treeview */
	   /* try to remove if we're in a custom folder */

	   /* see if it was dragged within the list view; if so, ignore */

	   MapWindowPoints(hTreeView,hwndList,&pt,1);
	   lvhtti.pt = pt;
	   GetWindowRect(hwndList, &rcList);
	   ClientToScreen(hwndList, &pt);
	   if( PtInRect(&rcList, pt) != 0 )
		   return;

	   folder = GetCurrentFolder();
	   if (folder->m_dwFlags & F_CUSTOM)
	   {
		   /* dragged out of a custom folder, so let's remove it */
		   RemoveCurrentGameCustomFolder();
	   }
	   return;
	}


	tvi.lParam = 0;
	tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (TreeView_GetItem(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder,game_dragged);
	}

}

static LPTREEFOLDER GetSelectedFolder(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	htree = TreeView_GetSelection(hTreeView);
	if(htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}
	return NULL;
}

static HICON GetSelectedFolderIcon(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	HIMAGELIST hSmall_icon;
	LPTREEFOLDER folder;
	htree = TreeView_GetSelection(hTreeView);

	if (htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);
		
		folder = (LPTREEFOLDER)tvi.lParam;
		//hSmall_icon = TreeView_GetImageList(hTreeView,(int)tvi.iImage);
		hSmall_icon = NULL;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return NULL;
}

/* Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	if( (GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_DETAILS ) )
		ListView_RedrawItems(hwndList,ListView_GetTopIndex(hwndList),
							 ListView_GetTopIndex(hwndList)+ ListView_GetCountPerPage(hwndList) );
}

void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height)
{
	int 	destX, destY;
	int 	destW, destH;
	int		nBorder;
	RECT	rect;
	/* for scaling */		 
	int x, y;
	int rWidth, rHeight;
	double scale;
	BOOL bReduce = FALSE;

	GetClientRect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;
		GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}
	rWidth	= (rect.right  - rect.left);
	rHeight = (rect.bottom - rect.top);

	/* Limit the screen shot to max height of 264 */
	if (restrict_height == TRUE && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	/* If the bitmap does NOT fit in the screenshot area */
	if (x > rWidth - 10 || y > rHeight - 10)
	{
		rect.right	-= 10;
		rect.bottom -= 10;
		rWidth	-= 10;
		rHeight -= 10;
		bReduce = TRUE;
		/* Try to scale it properly */
		/*	assumes square pixels, doesn't consider aspect ratio */
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		/* If it's still too big, scale again */
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth	/ destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		if (GetStretchScreenShotLarger())
		{
			rect.right	-= 10;
			rect.bottom -= 10;
			rWidth	-= 10;
			rHeight -= 10;
			bReduce = TRUE;
			// Try to scale it properly
			// assumes square pixels, doesn't consider aspect ratio
			if (x < y)
				scale = (double)rWidth / x;
			else
				scale = (double)rHeight / y;
			
			destW = (int)(x * scale);
			destH = (int)(y * scale);
			
			// If it's too big, scale again
			if (destW > rWidth || destH > rHeight)
			{
				if (destW > rWidth)
					scale = (double)rWidth	/ destW;
				else
					scale = (double)rHeight / destH;
				
				destW = (int)(destW * scale);
				destH = (int)(destH * scale);
			}
		}
		else
		{
			// Use the bitmaps size if it fits
			destW = x;
			destH = y;
		}

	}


	destX = ((rWidth  - destW) / 2);
	destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}
	nBorder = GetScreenshotBorderSize();
	if( destX > nBorder+1)
		pRect->left   = destX - nBorder;
	else
		pRect->left   = 2;
	if( destY > nBorder+1)
		pRect->top	  = destY - nBorder;
	else
		pRect->top	  = 2;
	if( rWidth >= destX + destW + nBorder)
		pRect->right  = destX + destW + nBorder;
	else
		pRect->right  = rWidth - pRect->left;
	if( rHeight >= destY + destH + nBorder)
		pRect->bottom = destY + destH + nBorder;
	else
		pRect->bottom = rHeight - pRect->top;
}

/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically. 
*/

void SwitchFullScreenMode(void)
{
	LONG lMainStyle;
	
	if (GetRunFullScreen())
	{
		// Return to normal

		// Restore the menu
		SetMenu(hMain, LoadMenu(hInst,MAKEINTRESOURCE(IDR_UI_MENU)));
		
		// Refresh the checkmarks
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, GetShowFolderList() ? MF_CHECKED : MF_UNCHECKED); 
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);    
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);
		
		// Add frame to dialog again
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle | WS_BORDER;
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		
		// Show the window maximized
		if( GetWindowState() == SW_MAXIMIZE )
		{
			ShowWindow(hMain, SW_NORMAL);
			ShowWindow(hMain, SW_MAXIMIZE);
		}
		else
			ShowWindow(hMain, SW_RESTORE);

		SetRunFullScreen(FALSE);
	}
	else
	{
		// Set to fullscreen
		
		// Remove menu
		SetMenu(hMain,NULL); 

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		if( IsMaximized(hMain) )
		{
			ShowWindow(hMain, SW_NORMAL);
			SetWindowState( SW_MAXIMIZE );
		}
		ShowWindow(hMain, SW_MAXIMIZE);
		
		SetRunFullScreen(TRUE);
	}
}

/*
  Checks to see if the mouse has been moved since this func
  was first called (which is at startup). The reason for 
  storing the startup coordinates of the mouse is that when
  a window is created it generates WM_MOUSEOVER events, even
  though the user didn't actually move the mouse. So we need
  to know when the WM_MOUSEOVER event is user-triggered.

  POSSIBLE BUGS:
  Gets polled at every WM_MOUSEMOVE so it might cause lag,
  but there's probably another way to code this that's 
  way better?
  
*/

BOOL MouseHasBeenMoved(void)
{
    static int mouse_x = -1;
    static int mouse_y = -1;
	POINT p;

	GetCursorPos(&p);

    if (mouse_x == -1) // First time
    {
		mouse_x = p.x;
		mouse_y = p.y;
    }
	
	return (p.x != mouse_x || p.y != mouse_y);       
}

/*
	The following two functions enable us to send Messages to the Game Window
*/
#if MULTISESSION

BOOL SendIconToEmulationWindow(int nGameIndex)
{
	HICON hIcon; 
	int nParentIndex = -1;
	hIcon = LoadIconFromFile(drivers[nGameIndex]->name); 
	if( hIcon == NULL ) 
	{ 
		//Check if clone, if so try parent icon first 
		if( DriverIsClone(nGameIndex) ) 
		{ 
			nParentIndex = GetParentIndex(drivers[nGameIndex]);
			if( nParentIndex >= 0)
				hIcon = LoadIconFromFile(drivers[nParentIndex]->name); 
			if( hIcon == NULL) 
			{ 
				hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAME32_ICON)); 
			} 
		} 
		else 
		{ 
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAME32_ICON)); 
		} 
	} 
	if( SendMessageToEmulationWindow( WM_SETICON, ICON_SMALL, (LPARAM)hIcon ) == FALSE)
		return FALSE;
	if( SendMessageToEmulationWindow( WM_SETICON, ICON_BIG, (LPARAM)hIcon ) == FALSE)
		return FALSE;
	return TRUE;
}

BOOL SendMessageToEmulationWindow(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	FINDWINDOWHANDLE fwhs;
	fwhs.ProcessInfo = NULL;
	fwhs.hwndFound  = NULL;

	EnumWindows(EnumWindowCallBack, (LPARAM)&fwhs);
	if( fwhs.hwndFound )
	{
		SendMessage(fwhs.hwndFound, Msg, wParam, lParam);
		//Fix for loosing Focus, we reset the Focus on our own Main window
		SendMessage(fwhs.hwndFound, WM_KILLFOCUS,(WPARAM) hMain, (LPARAM) NULL);
		return TRUE;
	}
	return FALSE;
}


static BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam) 
{ 
	FINDWINDOWHANDLE * pfwhs = (FINDWINDOWHANDLE * )lParam; 
	DWORD ProcessId, ProcessIdGUI; 
	char buffer[MAX_PATH]; 

	GetWindowThreadProcessId(hwnd, &ProcessId);
	GetWindowThreadProcessId(hMain, &ProcessIdGUI);

	// cmk--I'm not sure I believe this note is necessary
	// note: In order to make sure we have the MainFrame, verify that the title 
	// has Zero-Length. Under Windows 98, sometimes the Client Window ( which doesn't 
	// have a title ) is enumerated before the MainFrame 

	GetWindowText(hwnd, buffer, sizeof(buffer));
	if (ProcessId  == ProcessIdGUI &&
		 strncmp(buffer,MAMENAME,strlen(MAMENAME)) == 0 &&
		 hwnd != hMain) 
	{ 
		pfwhs->hwndFound = hwnd; 
		return FALSE; 
	} 
	else 
	{ 
		// Keep enumerating 
		return TRUE; 
	} 
}

HWND GetGameWindow(void)
{
	FINDWINDOWHANDLE fwhs;
	fwhs.ProcessInfo = NULL;
	fwhs.hwndFound  = NULL;

	EnumWindows(EnumWindowCallBack, (LPARAM)&fwhs);
	return fwhs.hwndFound;
}


#else

void SendIconToProcess(LPPROCESS_INFORMATION pi, int nGameIndex)
{
	HICON hIcon; 
	int nParentIndex = -1;
	hIcon = LoadIconFromFile(drivers[nGameIndex]->name); 
	if( hIcon == NULL ) 
	{ 
		//Check if clone, if so try parent icon first 
		if( DriverIsClone(nGameIndex) ) 
		{ 
			nParentIndex = GetParentIndex(drivers[nGameIndex]);
			if( nParentIndex >= 0)
				hIcon = LoadIconFromFile(drivers[nParentIndex]->name); 
			if( hIcon == NULL) 
			{ 
				hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAME32_ICON)); 
			} 
		} 
		else 
		{ 
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAME32_ICON)); 
		} 
	} 
	WaitForInputIdle( pi->hProcess, INFINITE ); 
	SendMessageToProcess( pi, WM_SETICON, ICON_SMALL, (LPARAM)hIcon ); 
	SendMessageToProcess( pi, WM_SETICON, ICON_BIG, (LPARAM)hIcon ); 
}

void SendMessageToProcess(LPPROCESS_INFORMATION lpProcessInformation, 
                                      UINT Msg, WPARAM wParam, LPARAM lParam)
{
	FINDWINDOWHANDLE fwhs;
	fwhs.ProcessInfo = lpProcessInformation;
	fwhs.hwndFound  = NULL;

	EnumWindows(EnumWindowCallBack, (LPARAM)&fwhs);

	SendMessage(fwhs.hwndFound, Msg, wParam, lParam);
	//Fix for loosing Focus, we reset the Focus on our own Main window
	SendMessage(fwhs.hwndFound, WM_KILLFOCUS,(WPARAM) hMain, (LPARAM) NULL);
}

static BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam) 
{ 
	FINDWINDOWHANDLE * pfwhs = (FINDWINDOWHANDLE * )lParam; 
	DWORD ProcessId; 
	char buffer[MAX_PATH]; 

	GetWindowThreadProcessId(hwnd, &ProcessId);

	// cmk--I'm not sure I believe this note is necessary
	// note: In order to make sure we have the MainFrame, verify that the title 
	// has Zero-Length. Under Windows 98, sometimes the Client Window ( which doesn't 
	// have a title ) is enumerated before the MainFrame 

	GetWindowText(hwnd, buffer, sizeof(buffer));
	if (ProcessId  == pfwhs->ProcessInfo->dwProcessId &&
		 strncmp(buffer,MAMENAME,strlen(MAMENAME)) == 0) 
	{ 
		pfwhs->hwndFound = hwnd; 
		return FALSE; 
	} 
	else 
	{ 
		// Keep enumerating 
		return TRUE; 
	} 
}

HWND GetGameWindow(LPPROCESS_INFORMATION lpProcessInformation)
{
	FINDWINDOWHANDLE fwhs;
	fwhs.ProcessInfo = lpProcessInformation;
	fwhs.hwndFound  = NULL;

	EnumWindows(EnumWindowCallBack, (LPARAM)&fwhs);
	return fwhs.hwndFound;
}

#endif

/* End of source file */
