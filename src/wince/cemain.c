//============================================================
//
//	cemain.c - Shell for MESSCE
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "resource.h"
#include "driver.h"
#include "mamece.h"
#include "ui/SmartListView.h"
#include "ui/SoftwareList.h"
#include "strconv.h"

struct status_of_data
{
    BOOL bHasRoms;
    LPTSTR lpDescription;
};

//============================================================
//	PARAMETERS
//============================================================

// window styles
#define SHELLWINDOW_STYLE			WS_VISIBLE
#define SHELLWINDOW_STYLE_EX		0

#define SHELLWINDOW_X				0
#define SHELLWINDOW_Y				0
#define SHELLWINDOW_WIDTH			CW_USEDEFAULT
#define SHELLWINDOW_HEIGHT			CW_USEDEFAULT

//============================================================
//	LOCAL VARIABLES
//============================================================

static int*				pGame_Index;	// Index of available games
static struct status_of_data *ptGame_Data;	// Game Data Structure - such as hasRoms etc
static HBRUSH hBrush2; //Background Checkers
static HWND				hwndCB;					// The command bar handle
static SHACTIVATEINFO s_sai;
static HWND s_hwndMain;

static struct SmartListView *s_pGameListView;
static struct SmartListView *s_pSoftwareListView;

//============================================================
//	PROTOTYPES
//============================================================

static void mamece3_init();
static BOOL InitInstance(int nCmdShow);
static void RefreshGameListBox();
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void RefreshGameListBox();
static LRESULT CALLBACK Instructions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static HWND CreateRpCommandBar(HWND hwnd);

#define MAX_LOADSTRING 100

#define IDC_PLAYLIST		200
#define IDC_PLAY			201
#define IDC_SOFTWARELIST	202

struct MainWindowExtra
{
	HMENU hMenu;
};

//============================================================

/* ------------------------------------------------------------------------ *
 * Menu handling code														*
 * ------------------------------------------------------------------------ */

static BOOL GetMenuOption(HWND hWnd, int nOptionID)
{
	struct MainWindowExtra *pExtra;
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;

	pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);
	if (pExtra)
		GetMenuItemInfo(pExtra->hMenu, nOptionID, FALSE, &mii);

	return (mii.fState & MFS_CHECKED) ? TRUE : FALSE;
}

static void ToggleDualMenuOption(HWND hWnd, int nOptionID, int nOtherOptionID)
{
	BOOL bIsChecked;
	struct MainWindowExtra *pExtra;

	bIsChecked = GetMenuOption(hWnd, nOptionID);

	bIsChecked = !bIsChecked;

	pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);
	CheckMenuItem(pExtra->hMenu, nOptionID, MF_BYCOMMAND | (bIsChecked ? MF_CHECKED : MF_UNCHECKED));
	if (nOtherOptionID != -1)
		CheckMenuItem(pExtra->hMenu, nOtherOptionID, MF_UNCHECKED);
}

static void ToggleMenuOption(HWND hWnd, int nOptionID)
{
	ToggleDualMenuOption(hWnd, nOptionID, -1);
}


static void SetDefaultOptions(HWND hWnd)
{
	struct MainWindowExtra *pExtra;
	pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);
	if (!pExtra)
		return;

	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLESOUND,			MF_CHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_SHOWFRAMERATE,		MF_UNCHECKED);
#ifdef MAME_DEBUG
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_SHOWPROFILER,		MF_UNCHECKED);
#endif
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLEFLICKER,		MF_CHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLETRANSLUCENCY,	MF_UNCHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLEANTIALIASING,	MF_UNCHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLEDIRTYLINE,		MF_CHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ENABLETHROTTLE,		MF_CHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ROTATESCREENLEFT,	MF_UNCHECKED);
	CheckMenuItem(pExtra->hMenu,	ID_OPTIONS_ROTATESCREENRIGHT,	MF_UNCHECKED);
}

/* ----------------------------------------------------------------------- *
 * Software list view class                                                *
 * ----------------------------------------------------------------------- */

#ifdef MESS
static void SoftwareList_Run(struct SmartListView *pListView)
{
	int nGame;
	int nItem;
	int nError;
	size_t nFailedAlloc;
	LPTSTR lpError;
	LPTSTR lpMessError = NULL;
	TCHAR szBuffer[256];
	struct ui_options opts;

	opts.enable_sound			= GetMenuOption(s_hwndMain, ID_OPTIONS_ENABLESOUND);
	opts.show_framerate			= GetMenuOption(s_hwndMain, ID_OPTIONS_SHOWFRAMERATE);
#ifdef MAME_DEBUG
	opts.show_profiler			= GetMenuOption(s_hwndMain, ID_OPTIONS_SHOWPROFILER);
#endif
	opts.enable_flicker			= GetMenuOption(s_hwndMain, ID_OPTIONS_ENABLEFLICKER);
	opts.enable_translucency	= GetMenuOption(s_hwndMain, ID_OPTIONS_ENABLETRANSLUCENCY);
	opts.enable_antialias		= GetMenuOption(s_hwndMain, ID_OPTIONS_ENABLEANTIALIASING);
	opts.enable_throttle		= GetMenuOption(s_hwndMain, ID_OPTIONS_ENABLETHROTTLE);
	opts.rotate_left			= GetMenuOption(s_hwndMain, ID_OPTIONS_ROTATESCREENLEFT);
	opts.rotate_right			= GetMenuOption(s_hwndMain, ID_OPTIONS_ROTATESCREENRIGHT);

	nItem = SingleItemSmartListView_GetSelectedItem(s_pGameListView);
	nGame = pGame_Index[nItem];

	nError = play_game(nGame, &opts);
	if (nError) {
		/* an error occured; find out where */
		nFailedAlloc = outofmemory_occured();
		if (nFailedAlloc)
		{
			/* out of memory */
			wsprintf(szBuffer, TEXT("Out of memory (allocation for %i bytes failed)"), nFailedAlloc);
			lpError = szBuffer;
		}
		else if ((lpMessError = get_mess_output()) != NULL)
		{
			/* error reported with mess_printf() */
			lpError = lpMessError;
		}
		else if (GetLastError())
		{
			/* error from system */
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), NULL);
			lpError = szBuffer;
		}
		else
		{
			/* unknown error */
			lpError = TEXT("Failed to run");
		}
		MessageBox(pListView->hwndListView, lpError, NULL, MB_OK);
	}

	SmartListView_SetVisible(s_pGameListView, TRUE);
	SmartListView_SetVisible(pListView, FALSE);

	/* if this is returned to us, we have to free it */
	if (lpMessError)
		free(lpMessError);
}

static LPCTSTR s_lpSoftwareColumnNames[] = 
{
	TEXT("Software")
};

static struct SmartListViewClass s_SoftwareListClass =
{
	0,
	SoftwareList_Run,								/* pfnRun */
	SoftwareList_ItemChanged,						/* pfnItemChanged */
	NULL,											/* pfnWhichIcon */
	SoftwareList_GetText,							/* pfnGetText */
	NULL,											/* pfnGetColumnInfo */
	NULL,											/* pfnSetColumnInfo */
	SoftwareList_IsItemSelected,					/* pfnIsItemSelected */
	Compare_TextCaseInsensitive,
	NULL,											/* pfnCanIdle */
	NULL,											/* pfnIdle */
	sizeof(s_lpSoftwareColumnNames) / sizeof(s_lpSoftwareColumnNames[0]),
	s_lpSoftwareColumnNames
};
#endif /* MESS */

/* ----------------------------------------------------------------------- *
 * Game list view class                                                    *
 * ----------------------------------------------------------------------- */

static LPCTSTR s_lpGameColumnNames[] = 
{
	TEXT("System")
};

static void GameList_Run(struct SmartListView *pListView)
{
	int nGame;
	int nItem;
	char *software_path_ptr;
	TCHAR software_path[MAX_PATH];

	nItem = SingleItemSmartListView_GetSelectedItem(pListView);
	nGame = pGame_Index[nItem];
	SmartListView_SetVisible(s_pSoftwareListView, TRUE);
	SmartListView_SetVisible(pListView, FALSE);
	SmartListView_SetExtraColumnText(s_pSoftwareListView, A2T(drivers[nGame]->description));

	/* compute software path (based on where the executable is) */
	get_mame_root(software_path, sizeof(software_path) / sizeof(software_path[0]));
	tcscat(software_path, TEXT("\\Software"));
	software_path_ptr = (char *) T2A(software_path);

	FillSoftwareList(s_pSoftwareListView, nGame, 1, &software_path_ptr, NULL);
}

static LPCTSTR GameList_GetText(struct SmartListView *pListView, int nRow, int nColumn)
{
	return ptGame_Data[pGame_Index[nRow]].lpDescription;
}

static struct SmartListViewClass s_GameListClass =
{
	sizeof(struct SingleItemSmartListView),
	GameList_Run,									/* pfnRun */
	SingleItemSmartListViewClass_ItemChanged,		/* pfnItemChanged */
	NULL,											/* pfnWhichIcon */
	GameList_GetText,								/* pfnGetText */
	NULL,											/* pfnGetColumnInfo */
	NULL,											/* pfnSetColumnInfo */
	SingleItemSmartListViewClass_IsItemSelected,	/* pfnIsItemSelected */
	Compare_TextCaseInsensitive,
	NULL,											/* pfnCanIdle */
	NULL,											/* pfnIdle */
	sizeof(s_lpGameColumnNames) / sizeof(s_lpGameColumnNames[0]),
	s_lpGameColumnNames
};

/* ----------------------------------------------------------------------- *
 * Dialogs                                                                 *
 * ----------------------------------------------------------------------- */

static LRESULT CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SHINITDLGINFO shidi;

	switch (message) {
	case WM_INITDIALOG:
		// Create a Done button and size it.  
		shidi.dwMask = SHIDIM_FLAGS;
		shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
		shidi.hDlg = hDlg;
		SHInitDialog(&shidi);
		return TRUE; 

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static void DisplayDialog(HWND hWndParent, int nDialogResource)
{
	DialogBox(GetModuleHandle(NULL), (LPCTSTR) nDialogResource, hWndParent, (DLGPROC) InfoDialogProc);
}

static void Display_About(HWND hWndParent)
{
	DisplayDialog(hWndParent, IDD_ABOUTBOX);
}


static void Display_Instructions(HWND hWndParent)
{
	DisplayDialog(hWndParent, IDD_INSTRUCTIONS);
}


static void Display_FAQ(HWND hWndParent)
{
	DisplayDialog(hWndParent, IDD_FAQ);
}

/* ----------------------------------------------------------------------- *
 * Blah                                                                    *
 * ----------------------------------------------------------------------- */

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	mamece3_init();

	// Perform application initialization:
	if (!InitInstance(nCmdShow)) 
		return FALSE;

	hAccelTable = LoadAccelerators(GetModuleHandle(NULL), (LPCTSTR)IDC_MAMECE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    It is important to call this function so that the application 
//    will get 'well formed' small icons associated with it.
//
static ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS	wc;

	// Load Checkerboard Backgroup Brush from Bitmap
	HBITMAP hBitmap;
	hBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_BITMAP2)); //Background Checkers
	hBrush2 = CreatePatternBrush (hBitmap);
	DeleteObject(hBitmap);
	//

    wc.style			= CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc		= (WNDPROC) WndProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= 0;
    wc.hInstance		= hInstance;
    wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAMECE));
    wc.hCursor			= 0;
    wc.hbrBackground	= hBrush2; //Background Checkers //Was - (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName		= 0;
    wc.lpszClassName	= szWindowClass;

	return RegisterClass(&wc);
}

//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
static BOOL InitInstance(int nCmdShow)
{
	HWND	hWnd = NULL;
	TCHAR	szTitle[MAX_LOADSTRING];			// The title bar text
	RECT	rect;
	HINSTANCE hInst;

	TCHAR szWindowClass[] = TEXT("MAMECE");

	hInst = GetModuleHandle(NULL);

	// Initialize global strings
	LoadString(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	//If it is already running, then focus on the window
	hWnd = FindWindow(szWindowClass, szTitle);	
	if (hWnd) 
	{
		SetForegroundWindow ((HWND) (((DWORD)hWnd) | 0x01));    
		return 0;
	} 

	MyRegisterClass(hInst, szWindowClass);
	
	hWnd = CreateWindowEx(SHELLWINDOW_STYLE_EX, szWindowClass, szTitle, SHELLWINDOW_STYLE,
		SHELLWINDOW_X, SHELLWINDOW_Y, SHELLWINDOW_WIDTH, SHELLWINDOW_HEIGHT, 
		NULL, NULL, hInst, NULL);
	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, hdcMem;
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	
	HBITMAP		hBitmap;
	BITMAP		bitmap;
	HINSTANCE hInst;
	struct MainWindowExtra *pExtra;
	struct SmartListViewOptions opts;

	if (s_pGameListView && SmartListView_IsEvent(s_pGameListView, message, wParam, lParam))
		return SmartListView_HandleEvent(s_pGameListView, message, wParam, lParam);

	if (s_pSoftwareListView && SmartListView_IsEvent(s_pSoftwareListView, message, wParam, lParam))
		return SmartListView_HandleEvent(s_pSoftwareListView, message, wParam, lParam);

	hInst = GetModuleHandle(NULL);
	
	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{	
				case IDOK:
					SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
					SendMessage (hWnd, WM_CLOSE, 0, 0);
					break;

				case ID_OPTIONS_SHOWALLGAMES:
				case ID_OPTIONS_SHOWCLONES:
					RefreshGameListBox();
					/* fall through */

				case ID_OPTIONS_ENABLESOUND:
				case ID_OPTIONS_SHOWFRAMERATE:
#ifdef MAME_DEBUG
				case ID_OPTIONS_SHOWPROFILER:
#endif
				case ID_OPTIONS_ENABLEANTIALIASING:
				case ID_OPTIONS_ENABLETRANSLUCENCY:
				case ID_OPTIONS_ENABLEFLICKER:
				case ID_OPTIONS_ENABLETHROTTLE:
				case ID_OPTIONS_ENABLEDIRTYLINE:
					ToggleMenuOption(hWnd, wmId);
					break;

				case ID_OPTIONS_ROTATESCREENLEFT:
					ToggleDualMenuOption(hWnd, ID_OPTIONS_ROTATESCREENLEFT, ID_OPTIONS_ROTATESCREENRIGHT);
					break;				

				case ID_OPTIONS_ROTATESCREENRIGHT:
					ToggleDualMenuOption(hWnd, ID_OPTIONS_ROTATESCREENRIGHT, ID_OPTIONS_ROTATESCREENLEFT);
					break;

				case ID_OPTIONS_RESETOPTIONSTODEFAULT:
					SetDefaultOptions(hWnd);
					RefreshGameListBox();
					break;

				case IDC_PLAY:
					if (wmEvent == BN_CLICKED)
					{
						if (SmartListView_GetVisible(s_pGameListView))
							GameList_Run(s_pGameListView);
						else if (SmartListView_GetVisible(s_pSoftwareListView))
							SoftwareList_Run(s_pSoftwareListView);
					}
					break;

					// Dialog boxes
				case IDM_HELP_ABOUT:
					Display_About(hWnd);
				    break;

				case ID_FILE_INSTRUCTIONS:
					Display_Instructions(hWnd);
					break;
				
				case ID_FILE_FAQ:
					Display_FAQ(hWnd);
					break;


				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_CREATE:
			{
				RECT rBtns;
				TBBUTTONINFO tbbi;
				int x, y, width, height;

				memset(&tbbi, 0, sizeof(tbbi));

				hwndCB = CreateRpCommandBar(hWnd);
				if (hwndCB)
				{
					tbbi.cbSize = sizeof(tbbi);
					tbbi.dwMask = TBIF_LPARAM;
					SendMessage(hwndCB,TB_GETBUTTONINFO,IDS_CAP_OPTIONS,(LPARAM)&tbbi);
				}
				// Initialize the shell activate info structure
				memset (&s_sai, 0, sizeof(s_sai));
				s_sai.cbSize = sizeof(s_sai);

				pExtra = malloc(sizeof(struct MainWindowExtra));
				pExtra->hMenu = (HMENU) tbbi.lParam;
				SetWindowLong(hWnd, GWL_USERDATA, (LONG_PTR) pExtra);
				
				//Turn Sound on by default and mark it Checked
				//	CheckMenuItem(g_hMenu, ID_OPTIONS_ENABLESOUND,
				//			MF_BYCOMMAND | ((enable_sound = 0) ? MF_CHECKED : MF_UNCHECKED) );

				GetClientRect(hWnd, &rBtns);
				CreateWindowEx(0, TEXT("BUTTON"), TEXT("Play"),
					WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
					9, rBtns.bottom - 33 - 30, 108, 26, 
					hWnd, (HMENU)IDC_PLAY, hInst, NULL); 
				CreateWindowEx(0, TEXT("BUTTON"), TEXT("Exit"),
					WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
					123, rBtns.bottom - 33 - 30, 108, 26, 
					hWnd, (HMENU)IDOK, hInst, NULL);
			
				memset(&opts, 0, sizeof(opts));

				opts.pClass = &s_SoftwareListClass;
				opts.hwndParent = hWnd;
				opts.nIDDlgItem = IDC_SOFTWARELIST;
				x = 10;
				y = 56;
				width = rBtns.right - 18;
				height = rBtns.bottom - 125;
//				x = 10;
//				y = 92;
//				width = rBtns.right - 18;
//				height = rBtns.bottom - 161;
				s_pSoftwareListView = SmartListView_Create(&opts, FALSE, TRUE, x, y, width, height, hInst);

				opts.pClass = &s_GameListClass;
				opts.hwndParent = hWnd;
				opts.nIDDlgItem = IDC_PLAYLIST;
				x = 10;
				y = 56;
				width = rBtns.right - 18;
				height = rBtns.bottom - 125;
				s_pGameListView = SmartListView_Create(&opts, TRUE, TRUE, x, y, width, height, hInst);

				s_hwndMain = hWnd;

				SetDefaultOptions(hWnd);

				RefreshGameListBox();
			}
			break;

		case WM_PAINT:
			{
				RECT rt;
				hdc = BeginPaint(hWnd, &ps);
				GetClientRect(hWnd, &rt);
				
				//Load MameCE3 Bitmap
				hdcMem = CreateCompatibleDC (hdc);
				hBitmap = LoadBitmap (hInst, MAKEINTRESOURCE(IDB_BITMAP1));
					GetObject(hBitmap, sizeof(BITMAP), &bitmap);
					SelectObject(hdcMem, hBitmap);
					BitBlt(hdc,5,5,bitmap.bmWidth,bitmap.bmHeight,hdcMem,0,0,SRCCOPY);
				DeleteObject (hBitmap);
				DeleteDC (hdcMem);

				EndPaint(hWnd, &ps);
			}
			break; 
		case WM_DESTROY:
			pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);
			DeleteObject(hBrush2);	// Background Checkers
			free(ptGame_Data);		// Delete Memory allocated in mamece_init or Play Loop
			free(pGame_Index);		// Delete Memory allocated in mamece_init or Play Loop
			if (hwndCB)
				CommandBar_Destroy(hwndCB);
			free(pExtra);
			PostQuitMessage(0);
			break;
		case WM_ACTIVATE:
            // Notify shell of our activate message
			SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
     		break;
		case WM_SETTINGCHANGE:
			SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
     		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

static HWND CreateRpCommandBar(HWND hwnd)
{
	SHMENUBARINFO mbi;

	memset(&mbi, 0, sizeof(SHMENUBARINFO));
	mbi.cbSize     = sizeof(SHMENUBARINFO);
	mbi.hwndParent = hwnd;
	mbi.dwFlags	   = SHCMBF_HIDESIPBUTTON;
	mbi.nToolBarId = IDM_MENU;
	mbi.hInstRes   = GetModuleHandle(NULL);
	mbi.nBmpId     = 0;
	mbi.cBmpImages = 0;

	if (!SHCreateMenuBar(&mbi)) 
		return NULL;

	return mbi.hwndMB;
}

static void RefreshGameListBox()
{
	int i, pos = 0;
	BOOL bShowAllGames;
	BOOL bShowClones;

	bShowAllGames = GetMenuOption(s_hwndMain, ID_OPTIONS_SHOWALLGAMES);
	bShowClones = GetMenuOption(s_hwndMain, ID_OPTIONS_SHOWCLONES);

	/* Clear out the list */
	if (s_pGameListView)
		SmartListView_ResetColumnDisplay(s_pGameListView);

	/* Add the drivers */
	for (i = 0; drivers[i]; i++) {
		if (!bShowAllGames && !ptGame_Data[i].bHasRoms)
			continue;
				
		if (!bShowAllGames && !bShowClones && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
			continue;

		pGame_Index[pos++] = i;
	}

	/* Sort them */
	if (s_pGameListView)
	{
		SmartListView_SetTotalItems(s_pGameListView, pos);
		SmartListView_SetSorting(s_pGameListView, 0, FALSE);
	}
}


// Checks if all ROMs are available for 'game' and returns results
// Returns TRUE if all ROMs found, FALSE if any ROMs are missing.
static BOOL FindRomSet(int game)
{
	const struct RomModule  *romp;
	const struct GameDriver *gamedrv;

	gamedrv = drivers[game];
	romp = gamedrv->rom;

	if (romp && rom_first_file(romp)) {
		if (!mame_faccess (gamedrv->name, FILETYPE_ROM)) {
			// if the game is a clone, try loading the ROM from the main version //
			if (gamedrv->clone_of->clone_of == 0 ||
				!mame_faccess(gamedrv->clone_of->name, FILETYPE_ROM))
	          return FALSE; 
		}
	}

    return TRUE;
}

static void mamece3_init()
{
	int i;
	int nGameCount;

	setup_paths();

	nGameCount = 0;	//Empty Game Count
	while (drivers[nGameCount] != NULL)
		nGameCount++;

	ptGame_Data = malloc(nGameCount * sizeof(struct status_of_data));	 // Allocate Memory for Game Rom Status
	pGame_Index = malloc(nGameCount * sizeof(int));	 // Allocate Memory for Game Index

	for (i = 0; i < nGameCount; i++)
	{
		ptGame_Data[i].bHasRoms = FindRomSet(i);
		ptGame_Data[i].lpDescription = _tcsdup(A2T(drivers[i]->description));
	}
	
}

void osd_parallelize(void (*task)(void *param, int task_num, int
	task_count), void *param, int max_tasks)
{
	task(param, 0, 1);
}




