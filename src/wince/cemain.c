#include <windows.h>

#define SHHandleWMSettingChange dummy_SHHandleWMSettingChange
#define SHCreateMenuBar			dummy_SHCreateMenuBar
#define SHInitDialog			dummy_SHInitDialog
#include <aygshell.h>
#undef SHHandleWMSettingChange
#undef SHCreateMenuBar
#undef SHInitDialog

WINSHELLAPI BOOL WINAPI SHHandleWMSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam, SHACTIVATEINFO* psai);
WINSHELLAPI BOOL WINAPI SHCreateMenuBar(SHMENUBARINFO *pmbi);
BOOL WINAPI SHInitDialog(PSHINITDLGINFO pshidi);

#include <commctrl.h>

#include "resource.h"
#include "driver.h"

// our dialog/configured options //
struct ui_options
{
	BOOL		enable_sound;		// Enable sound in games
	BOOL		show_all_games;		// Should we show all games?
	BOOL		show_clones;		// Don't show clone games
	BOOL		show_framerate;		// Display Frame Rate
	BOOL		show_profiler;		// Display the profiler
	BOOL		enable_flicker;		// Enable Flicker
	BOOL		enable_translucency;// Enable Translucency
	BOOL		enable_antialias;	// Enable AntiAlias
	BOOL		enable_dirtyline;	// Enable Dirty Line Drawing
	BOOL		disable_throttle;	// Disable Throttling
	BOOL		rotate_left;		// Rotate the Screen to the Left
	BOOL		rotate_right;		// Rotate the Screen to the Right
};

struct status_of_data
{
    BOOL has_roms;
    BOOL has_samples;
    BOOL is_neogeo_clone;
};

static void mamece3_init();
static BOOL InitInstance(int nCmdShow);
static void RefreshGameListBox();
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void Display_FAQ(HWND hWnd);
static void RefreshGameListBox();
static LRESULT CALLBACK Instructions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static HWND CreateRpCommandBar(HWND hwnd);

#define MAX_LOADSTRING 100

#define IDC_PLAYLIST	200
#define IDC_PLAY		201
#define MENU_HEIGHT 26

static int iPlay_Game;
static int*				pGame_Index;	// Index of available games
static struct status_of_data *ptGame_Data;	// Game Data Structure - such as hasRoms etc
static struct ui_options tUI;
static HWND		hGameList; 
static HBRUSH hBrush2; //Background Checkers
static HWND				hwndCB;					// The command bar handle
static HMENU				hMenu;					// The menu Handle
static SHACTIVATEINFO s_sai;

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
	opts->disable_throttle = 0;
	opts->rotate_left = 0;
	opts->rotate_right = 0;
}

static void PlayGame(const struct GameDriver *drv, struct ui_options *opts)
{
	extern int DECL_SPEC main(int argc, char **argv);
	int argc = 0;
	char *argv[50];
	WCHAR szFileNameWide[MAX_PATH];
	char szFileName[MAX_PATH];

	GetModuleFileName(NULL, szFileNameWide, sizeof(szFileNameWide) / sizeof(szFileNameWide[0]));
	_snprintf(szFileName, sizeof(szFileName) / sizeof(szFileName[0]), "%S", szFileNameWide);

	// Set up command line args
	argv[argc++] = szFileName;
	argv[argc++] = (char *) drv->name;
	argv[argc++] = "-rompath";
	argv[argc++] = "\\ROMs";
	argv[argc++] = "-samplepath";
	argv[argc++] = "\\Samples";
	argv[argc++] = "-cfg_directory";
	argv[argc++] = "\\Cfg";
	argv[argc++] = "-nvram_directory";
	argv[argc++] = "\\Nvram";
	argv[argc++] = "-input_directory";
	argv[argc++] = "\\inp";
	argv[argc++] = "-hiscore_directory";
	argv[argc++] = "\\hiscore";
	argv[argc++] = "-state_directory";
	argv[argc++] = "\\Sta";
	argv[argc++] = "-artwork_directory";
	argv[argc++] = "\\Artwork";
	argv[argc++] = "-snapshot_directory";
	argv[argc++] = "\\Snapshot";

	if (opts->rotate_left)
		argv[argc++] = "-rol";
	else if (opts->rotate_right)
		argv[argc++] = "-ror";

	argv[argc++] = (opts->enable_translucency) ? "-translucency" : "-notranslucency";
	argv[argc++] = (opts->enable_antialias) ? "-antialias" : "-noantialias";
	argv[argc++] = (opts->enable_dirtyline) ? "-dirty" : "-nodirty";
	argv[argc++] = (opts->disable_throttle) ? "-nothrottle" : "-throttle";

	argv[argc++] = "-sound";
	argv[argc++] = (opts->enable_sound) ? "-sound" : "-nosound";

	argv[argc++] = "-flicker";
	argv[argc++] = (opts->enable_flicker) ? "100.0" : "0.0";

	//opts->show_framerate = 0;
	//opts->show_profiler = 0;

	// Invoke the game
	main(argc, argv);
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

	hAccelTable = LoadAccelerators(GetModuleHandle(NULL), (LPCTSTR)IDC_MAMECE3);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (iPlay_Game >= 0)
			{
				PlayGame(drivers[iPlay_Game], &tUI);
				iPlay_Game = -1;
				RefreshGameListBox();
			}
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
    wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAMECE3));
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
	LoadString(hInst, IDC_MAMECE3, szWindowClass, MAX_LOADSTRING);
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

	hInst = GetModuleHandle(NULL);
	
	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{	
				case IDM_HELP_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				    break;
				case IDOK:
					SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
					SendMessage (hWnd, WM_CLOSE, 0, 0);
					break;
				case ID_OPTIONS_ENABLESOUND:
					{
						tUI.enable_sound = !tUI.enable_sound;
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLESOUND,
							MF_BYCOMMAND | (tUI.enable_sound ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_SHOWCLONES:
					{
						tUI.show_clones = !tUI.show_clones;
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWCLONES,
							MF_BYCOMMAND | (tUI.show_clones ? MF_CHECKED : MF_UNCHECKED) );
						RefreshGameListBox();
					}
					break;
				case ID_OPTIONS_SHOWALLGAMES:
					{
						tUI.show_all_games = !tUI.show_all_games;
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWALLGAMES,
							MF_BYCOMMAND | (tUI.show_all_games ? MF_CHECKED : MF_UNCHECKED) );
						RefreshGameListBox();
					}
					break;
				case ID_OPTIONS_SHOWFRAMERATE:
					{
						tUI.show_framerate = !tUI.show_framerate;
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWFRAMERATE,
							MF_BYCOMMAND | (tUI.show_framerate ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_SHOWPROFILER:
					{
						tUI.show_profiler = !tUI.show_profiler;
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWPROFILER,
							MF_BYCOMMAND | (tUI.show_profiler ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_ENABLEANTIALIASING:
					{
						tUI.enable_antialias = !tUI.enable_antialias;
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEANTIALIASING,
							MF_BYCOMMAND | (tUI.enable_antialias ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_ENABLETRANSLUCENCY:
					{
						tUI.enable_translucency = !tUI.enable_translucency;
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLETRANSLUCENCY,
							MF_BYCOMMAND | (tUI.enable_translucency ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_ENABLEFLICKER:
					{
						tUI.enable_flicker = !tUI.enable_flicker;
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEFLICKER,
							MF_BYCOMMAND | (tUI.enable_flicker ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_DISABLETHROTTLE:
					{
						tUI.disable_throttle = !tUI.disable_throttle;
						CheckMenuItem(hMenu, ID_OPTIONS_DISABLETHROTTLE,
							MF_BYCOMMAND | (tUI.disable_throttle ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_ENABLEDIRTYLINE:
					{
						tUI.enable_dirtyline = !tUI.enable_dirtyline;
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEDIRTYLINE,
							MF_BYCOMMAND | (tUI.enable_dirtyline ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_ROTATESCREENLEFT:
					{
						if (tUI.rotate_right == 1)
						{
							tUI.rotate_right = !tUI.rotate_right;
							CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENRIGHT,
							MF_BYCOMMAND | (tUI.rotate_right ? MF_CHECKED : MF_UNCHECKED) );
						}
						tUI.rotate_left = !tUI.rotate_left;
						CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENLEFT,
							MF_BYCOMMAND | (tUI.rotate_left ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;				
				case ID_OPTIONS_ROTATESCREENRIGHT:
					{
						if (tUI.rotate_left == 1)
						{
							tUI.rotate_left = !tUI.rotate_left;
							CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENLEFT,
							MF_BYCOMMAND | (tUI.rotate_left ? MF_CHECKED : MF_UNCHECKED) );
						}
						tUI.rotate_right = !tUI.rotate_right;
						CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENRIGHT,
							MF_BYCOMMAND | (tUI.rotate_right ? MF_CHECKED : MF_UNCHECKED) );
					}
					break;
				case ID_OPTIONS_RESETOPTIONSTODEFAULT:
					{
						// Reset Check Boxes back to Defaults
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLESOUND,		MF_BYCOMMAND | MF_CHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWALLGAMES,		MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWCLONES,			MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWPROFILER,		MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_SHOWFRAMERATE,		MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEANTIALIASING,	MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLETRANSLUCENCY,	MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEFLICKER,		MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_DISABLETHROTTLE,	MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ENABLEDIRTYLINE,	MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENLEFT,	MF_BYCOMMAND | MF_UNCHECKED);
						CheckMenuItem(hMenu, ID_OPTIONS_ROTATESCREENRIGHT,	MF_BYCOMMAND | MF_UNCHECKED);
						// Set Defualt Variable States back to Defaults
						SetDefaultOptions(&tUI);

						RefreshGameListBox();
					}
				case IDC_PLAY:
					if (wmEvent == BN_CLICKED)
					{
						iPlay_Game = SendMessage(hGameList, LB_GETCURSEL, 0, 0);
					}
					if (iPlay_Game >= 0)
						iPlay_Game = pGame_Index[iPlay_Game];
					break;
				case IDC_PLAYLIST:
					if (wmEvent == LBN_DBLCLK)
					{
						iPlay_Game = SendMessage(hGameList, LB_GETCURSEL, 0, 0);
					}
					if (iPlay_Game >= 0)
						iPlay_Game = pGame_Index[iPlay_Game];
				   break;
				case ID_FILE_INSTRUCTIONS:
					DialogBox(hInst, (LPCTSTR)IDD_INSTRUCTIONBOX, hWnd, (DLGPROC)Instructions);
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

				hwndCB = CreateRpCommandBar(hWnd);
				tbbi.cbSize = sizeof(tbbi);
				tbbi.dwMask = TBIF_LPARAM;
				SendMessage(hwndCB,TB_GETBUTTONINFO,ID_OPTIONS,(LPARAM)&tbbi);
				hMenu = (HMENU)tbbi.lParam;	//Grab Menu Selection
				
				//Turn Sound on by default and mark it Checked
				//	CheckMenuItem(hMenu, ID_OPTIONS_ENABLESOUND,
				//			MF_BYCOMMAND | ((enable_sound = 0) ? MF_CHECKED : MF_UNCHECKED) );

				GetClientRect(hWnd, &rBtns);
				hGameList = CreateWindowEx(0, _T("LISTBOX"), _T(""),
					WS_VISIBLE | WS_CHILD | WS_TABSTOP | LBS_STANDARD | LBS_NOINTEGRALHEIGHT, 
					9, 61, rBtns.right - 18, rBtns.bottom - 100 - 30,
					hWnd, (HMENU)IDC_PLAYLIST, hInst, NULL);
				CreateWindowEx(0, _T("BUTTON"), _T("Play"),
					WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
					9, rBtns.bottom - 33 - 30, 108, 26, 
					hWnd, (HMENU)IDC_PLAY, hInst, NULL); 
				CreateWindowEx(0, _T("BUTTON"), _T("Exit"),
					WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
					123, rBtns.bottom - 33 - 30, 108, 26, 
					hWnd, (HMENU)IDOK, hInst, NULL);
			
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
			DeleteObject(hBrush2);	// Background Checkers
			free(ptGame_Data);		// Delete Memory allocated in mamece_init or Play Loop
			free(pGame_Index);		// Delete Memory allocated in mamece_init or Play Loop
			CommandBar_Destroy(hwndCB);
			PostQuitMessage(0);
			break;
		case WM_SETTINGCHANGE:
			//SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
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

// Mesage handler for the About box.
static LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SHINITDLGINFO shidi;

	switch (message)
	{
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

// Mesage handler for the Instructions box.
static LRESULT CALLBACK Instructions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SHINITDLGINFO shidi;

	switch (message)
	{
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

static void RefreshGameListBox()
{
	int i, j, pos;
	int	iGame_Index_Count = 0;
	WCHAR szBuffer[256];

	SendMessage(hGameList, LB_RESETCONTENT, 0, 0);

	for (i = 0; drivers[i]; i++)
	{
		if (!tUI.show_all_games && !ptGame_Data[i].has_roms)
			continue;
				
		if (!tUI.show_all_games && !tUI.show_clones && drivers[i]->clone_of->clone_of != 0)
			continue;

		_snwprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), TEXT("%S"), drivers[i]->description);

		pos = SendMessage(hGameList, LB_ADDSTRING, 0, (LPARAM)szBuffer);

		for (j = iGame_Index_Count - 1; j >= pos; j--)
			pGame_Index[j + 1] = pGame_Index[j];
		
		pGame_Index[pos] = i;
		iGame_Index_Count++;
	}
}

static void Display_FAQ(HWND hWnd)
{
	TCHAR buf[] = TEXT("A license agreement will appear the first time you run each specific game. \
		Click [LEFT] then [RIGHT] on the Gamepad to agree to it. \
		\n\nDifference in versions:\nMameCE3 -  100+ Games\nMegaCE3 - 1000+ Games\nPC MAME - 2000+ Games \
		\n\nNO Version includes any Game ROMS!");
	MessageBox(hWnd, buf, TEXT("FAQ - Top 3 Answers"), MB_ICONINFORMATION | MB_OK);
}


// Checks if all ROMs are available for 'game' and returns results
// Returns TRUE if all ROMs found, FALSE if any ROMs are missing.
static BOOL FindRomSet(int game)
{
	const struct RomModule  *romp;
	const struct GameDriver *gamedrv;

	gamedrv = drivers[game];
	romp = gamedrv->rom;

	if (!osd_faccess (gamedrv->name, OSD_FILETYPE_ROM))
    {
		// if the game is a clone, try loading the ROM from the main version //
		if (gamedrv->clone_of->clone_of == 0 ||
			!osd_faccess(gamedrv->clone_of->name,OSD_FILETYPE_ROM))
          return FALSE; 
	}

    return TRUE;
}

static void mamece3_init()
{
	int i;
	int nGameCount;
	extern int rompathc;
	extern char **rompathv;
	char *normal_rompath = "\\ROMs";

	iPlay_Game = -1;	

	rompathc = 1;
	rompathv = &normal_rompath;

	nGameCount = 0;	//Empty Game Count
	while (drivers[nGameCount] != NULL)
		nGameCount++;

	ptGame_Data = malloc(nGameCount * sizeof(struct status_of_data));	 // Allocate Memory for Game Rom Status
	pGame_Index = malloc(nGameCount * sizeof(int));	 // Allocate Memory for Game Index

	for (i = 0; i < nGameCount; i++)
	{
		ptGame_Data[i].has_roms = FindRomSet(i);
		ptGame_Data[i].has_samples = TRUE; //FindSampleSet(i);
	}
	
}


