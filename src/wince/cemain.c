#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "driver.h"
#include "mamece.h"
#include "../Win32/SmartListView.h"
#include "../Win32/SoftwareList.h"

struct status_of_data
{
    BOOL bHasRoms;
    LPTSTR lpDescription;
};

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

static int*				pGame_Index;	// Index of available games
static struct status_of_data *ptGame_Data;	// Game Data Structure - such as hasRoms etc
static struct ui_options tUI;
static HBRUSH hBrush2; //Background Checkers
static HWND				hwndCB;					// The command bar handle
static SHACTIVATEINFO s_sai;

static struct SmartListView *s_pGameListView;
static struct SmartListView *s_pSoftwareListView;

#ifdef MESS
/* ----------------------------------------------------------------------- *
 * Software list view class                                                *
 * ----------------------------------------------------------------------- */

static void SoftwareList_Run(struct SmartListView *pListView)
{
	int nGame;
	int nItem;
	int nError;
	size_t nFailedAlloc;
	LPTSTR lpError;
	LPTSTR lpMessError = NULL;
	TCHAR szBuffer[32];

	nItem = SingleItemSmartListView_GetSelectedItem(s_pGameListView);
	nGame = pGame_Index[nItem];

	nError = play_game(nGame, &tUI);
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
	software_path_ptr = T2A(software_path);

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

static void SetDefaultOptions(struct ui_options *opts)
{
	opts->enable_sound = 1; 
	opts->show_all_games = 0;
	opts->show_clones = 0;
	opts->show_framerate = 0;
	opts->show_profiler = 0;
	opts->enable_flicker = 0;
	opts->enable_translucency = 0;
	opts->enable_antialias = 0;
	opts->enable_dirtyline = 0;
	opts->enable_throttle = 1;
	opts->rotate_left = 0;
	opts->rotate_right = 0;
}

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	SetDefaultOptions(&tUI);

	nCmdShow = SW_SHOWNORMAL;

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
	TCHAR	szWindowClass[MAX_LOADSTRING];		// The window class name
	RECT	rect;
	HINSTANCE hInst;

	hInst = GetModuleHandle(NULL);

	// Initialize global strings
	tcscpy(szWindowClass, TEXT("MAMECE3"));
	LoadString(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	//If it is already running, then focus on the window
	hWnd = FindWindow(szWindowClass, szTitle);	
	if (hWnd) 
	{
		SetForegroundWindow ((HWND) (((DWORD)hWnd) | 0x01));    
		return 0;
	} 

	MyRegisterClass(hInst, szWindowClass);
	
	GetClientRect(hWnd, &rect);
	
	hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	
	if (!hWnd)
	{	
		return FALSE;
	}
	//When the main window is created using CW_USEDEFAULT the height of the menubar (if one
	// is created is not taken into account). So we resize the window after creating it
	// if a menubar is present
	{
		RECT rc;
		GetWindowRect(hWnd, &rc);
		rc.bottom -= MENU_HEIGHT;
		if (hwndCB)
			MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, FALSE);
	}


	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

struct MainWindowExtra
{
	HMENU hMenu;
};

static void CheckMenuOption(HWND hWnd, int nOptionID, BOOL *pbOption, BOOL bToggle)
{
	struct MainWindowExtra *pExtra;

	pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);

	if (bToggle)
		*pbOption = !(*pbOption);
	CheckMenuItem(pExtra->hMenu, nOptionID, MF_BYCOMMAND | (*pbOption ? MF_CHECKED : MF_UNCHECKED));
}

static void CheckMenuOptionEx(HWND hWnd, int nOptionID, BOOL *pbOption, int nCounterOptionID, BOOL *pbCounterOption)
{
	if (*pbCounterOption)
		CheckMenuOption(hWnd, nCounterOptionID, pbCounterOption, TRUE);
	CheckMenuOption(hWnd, nOptionID, pbOption, TRUE);
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
	TCHAR szHello[MAX_LOADSTRING];
	
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
				case ID_OPTIONS_ENABLESOUND:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLESOUND, &tUI.enable_sound, TRUE);
					break;
				case ID_OPTIONS_SHOWCLONES:
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWCLONES, &tUI.show_clones, TRUE);
					RefreshGameListBox();
					break;
				case ID_OPTIONS_SHOWALLGAMES:
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWALLGAMES, &tUI.show_all_games, TRUE);
					RefreshGameListBox();
					break;
				case ID_OPTIONS_SHOWFRAMERATE:
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWFRAMERATE, &tUI.show_framerate, TRUE);
					break;
				case ID_OPTIONS_SHOWPROFILER:
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWPROFILER, &tUI.show_profiler, TRUE);
					break;
				case ID_OPTIONS_ENABLEANTIALIASING:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEANTIALIASING, &tUI.enable_antialias, TRUE);
					break;
				case ID_OPTIONS_ENABLETRANSLUCENCY:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLETRANSLUCENCY, &tUI.enable_translucency, TRUE);
					break;
				case ID_OPTIONS_ENABLEFLICKER:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEFLICKER, &tUI.enable_flicker, TRUE);
					break;
				case ID_OPTIONS_ENABLETHROTTLE:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLETHROTTLE, &tUI.enable_throttle, TRUE);
					break;
				case ID_OPTIONS_ENABLEDIRTYLINE:
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEDIRTYLINE, &tUI.enable_dirtyline, TRUE);
					break;
				case ID_OPTIONS_ROTATESCREENLEFT:
					CheckMenuOptionEx(hWnd, ID_OPTIONS_ROTATESCREENLEFT, &tUI.rotate_left, ID_OPTIONS_ROTATESCREENRIGHT, &tUI.rotate_right);
					break;				
				case ID_OPTIONS_ROTATESCREENRIGHT:
					CheckMenuOptionEx(hWnd, ID_OPTIONS_ROTATESCREENRIGHT, &tUI.rotate_right, ID_OPTIONS_ROTATESCREENLEFT, &tUI.rotate_left);
					break;

				case ID_OPTIONS_RESETOPTIONSTODEFAULT:
					// Set Defualt Variable States back to Defaults
					SetDefaultOptions(&tUI);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLESOUND, &tUI.enable_sound, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWCLONES, &tUI.show_clones, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWALLGAMES, &tUI.show_all_games, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWFRAMERATE, &tUI.show_framerate, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_SHOWPROFILER, &tUI.show_profiler, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEANTIALIASING, &tUI.enable_antialias, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLETRANSLUCENCY, &tUI.enable_translucency, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEFLICKER, &tUI.enable_flicker, TRUE);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLETHROTTLE, &tUI.enable_throttle, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ENABLEDIRTYLINE, &tUI.enable_dirtyline, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ROTATESCREENLEFT, &tUI.rotate_left, FALSE);
					CheckMenuOption(hWnd, ID_OPTIONS_ROTATESCREENRIGHT, &tUI.rotate_right, FALSE);
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

				hwndCB = CreateRpCommandBar(hWnd);
				tbbi.cbSize = sizeof(tbbi);
				tbbi.dwMask = TBIF_LPARAM;
				SendMessage(hwndCB,TB_GETBUTTONINFO,ID_OPTIONS,(LPARAM)&tbbi);

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
			//
				LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);
				DrawText(hdc, szHello, _tcslen(szHello), &rt, 
					DT_SINGLELINE | DT_VCENTER | DT_CENTER);
				EndPaint(hWnd, &ps);
			}
			break; 
		case WM_DESTROY:
			pExtra = (struct MainWindowExtra *) GetWindowLong(hWnd, GWL_USERDATA);
			DeleteObject(hBrush2);	// Background Checkers
			free(ptGame_Data);		// Delete Memory allocated in mamece_init or Play Loop
			free(pGame_Index);		// Delete Memory allocated in mamece_init or Play Loop
			CommandBar_Destroy(hwndCB);
			free(pExtra);
			PostQuitMessage(0);
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

	/* Clear out the list */
	SmartListView_ResetColumnDisplay(s_pGameListView);

	/* Add the drivers */
	for (i = 0; drivers[i]; i++) {
		if (!tUI.show_all_games && !ptGame_Data[i].bHasRoms)
			continue;
				
		if (!tUI.show_all_games && !tUI.show_clones && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
			continue;

		pGame_Index[pos++] = i;
	}

	/* Sort them */
	SmartListView_SetTotalItems(s_pGameListView, pos);
	SmartListView_SetSorting(s_pGameListView, 0, FALSE);
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
		if (!osd_faccess (gamedrv->name, OSD_FILETYPE_ROM)) {
			// if the game is a clone, try loading the ROM from the main version //
			if (gamedrv->clone_of->clone_of == 0 ||
				!osd_faccess(gamedrv->clone_of->name,OSD_FILETYPE_ROM))
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




