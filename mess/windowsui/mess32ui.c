#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>

#include "windowsui/mame32.h"
#include "windowsui/MessResource.h"
#include "mess.h"
#include "config.h"
#include "SmartListView.h"
#include "SoftwareList.h"
#include "windows/window.h"
#include "messwin.h"
#include "rc.h"

#ifdef _MSC_VER
#define alloca _alloca
#endif

/* #define INPROCESS_MAME */

/* from src/win32/Directories.c */
const char *GetMessSoftwarePath(int i);
int GetMessSoftwarePathCount(void);

static int requested_device_type(char *tchar);
static void MessCreateCommandLine(char *pCmdLine, options_type *pOpts, const struct GameDriver *gamedrv);

static int SoftwareListClass_WhichIcon(struct SmartListView *pListView, int nItem);
static void SoftwareListClass_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
static void SoftwareListClass_SetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
static void SoftwareListClass_Run(struct SmartListView *pListView);
static BOOL SoftwareListClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);

static const char *mess_column_names[] = {
    "Software",
	"Goodname",
    "Manufacturer",
    "Year",
    "Playable",
	"CRC"
};

static struct SmartListViewClass s_softwareListClass =
{
	0,
	SoftwareListClass_Run,
	SoftwareListClass_ItemChanged,
	SoftwareListClass_WhichIcon,
	SoftwareList_GetText,
	SoftwareListClass_GetColumnInfo,
	SoftwareListClass_SetColumnInfo,
	SoftwareList_IsItemSelected,
	Compare_TextCaseInsensitive,
	SoftwareList_CanIdle,
	SoftwareList_Idle,
	sizeof(mess_column_names) / sizeof(mess_column_names[0]),
	mess_column_names
};

static struct SmartListView *s_pSoftwareListView;
static int *mess_icon_index;

static void InitMessPicker(void);
static void MyFillSoftwareList(int nGame);
static void MessUpdateSoftwareList(void);
static void MessSetPickerDefaults(void);
static void MessOpenOtherSoftware(int iDevice);
static void MessCreateDevice(int iDevice);
static BOOL CreateMessIcons(void);

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
static void MessTestsDoneIdle(void);
#endif /* MAME_DEBUG */

/*
#define MAME32HELP "mess32.hlp"
*/

#ifdef bool
#undef bool
#endif

#include "windowsui/win32ui.c"

struct deviceentry {
	int icon;
	const char *dlgname;
};

/* ------------------------------------------------------------------------ *
 * Image types
 *
 * IO_END (0) is used for ZIP files
 * IO_ALIAS is used for unknown types
 * IO_COUNT is used for bad files
 * ------------------------------------------------------------------------ */

#define IO_ZIP		(IO_END)
#define IO_BAD		(IO_COUNT + 0)
#define IO_UNKNOWN	(IO_COUNT + 1)

/* TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape! */
static struct deviceentry s_devices[] =
{
	{ 1, "Cartridge images" },		/* IO_CARTSLOT */
	{ 6, "Floppy disk images" },	/* IO_FLOPPY */
	{ 11, "Hard disk images" },		/* IO_HARDDISK */
	{ 2, "Cylinders" },				/* IO_CYLINDER */
	{ 7, "Cassette images" },		/* IO_CASSETTE */
	{ 2, "Punchcard images" },		/* IO_PUNCHCARD */
	{ 2, "Punchtape images" },		/* IO_PUNCHTAPE */
	{ 10, "Printer Output" },		/* IO_PRINTER */
	{ 8, "Serial Output" },			/* IO_SERIAL */
	{ 2, "Parallel Output" },		/* IO_PARALLEL */
	{ 9, "Snapshots" },				/* IO_SNAPSHOT */
	{ 9, "Quickloads" }				/* IO_QUICKLOAD */
};

static void AssertValidDevice(int d)
{
	assert((sizeof(s_devices) / sizeof(s_devices[0])) + 1 == IO_COUNT);
	assert(((d > IO_END) && (d < IO_COUNT)) || (d == IO_UNKNOWN) || (d == IO_BAD));
}

static const struct deviceentry *lookupdevice(int d)
{
	AssertValidDevice(d);
	return &s_devices[d - 1];
}

/* ------------------------------------------------------------------------ */

static int requested_device_type(char *tchar)
{
	int device = -1;
	int i;

    logerror("Requested device is %s\n", tchar);

	if (*tchar == '-') {
		tchar++;

		for (i = 1; i < IO_COUNT; i++) {
			if (!stricmp(tchar, device_typename(i)) || !stricmp(tchar, device_brieftypename(i))) {
				device = i;
				break;
			}
		}
	}

	return device;
}

/* ************************************************************************ */
/* UI                                                                       */
/* ************************************************************************ */

static BOOL CreateMessIcons(void)
{
    int i;

    if (!mess_icon_index) {
        mess_icon_index = malloc(sizeof(int) * game_count * IO_COUNT);
        if (!mess_icon_index)
            return FALSE;
    }

    for (i = 0; i < (game_count * IO_COUNT); i++)
        mess_icon_index[i] = 0;

	if (s_pSoftwareListView)
		SmartListView_AssociateImageLists(s_pSoftwareListView, hSmall, hLarge);
    return TRUE;
}

static int GetMessIcon(int nGame, int nSoftwareType)
{
    int the_index;
    int nIconPos = 0;
    HICON hIcon = 0;
    const struct GameDriver *drv;
    char buffer[32];
	const char *iconname;

    if ((nSoftwareType > IO_END) && (nSoftwareType < IO_COUNT)) {
		iconname = device_brieftypename(nSoftwareType);
        the_index = (nGame * IO_COUNT) + nSoftwareType;

        nIconPos = mess_icon_index[the_index];
        if (!nIconPos) {
            for (drv = drivers[nGame]; drv; drv = drv->clone_of) {
                sprintf(buffer, "%s/%s", drv->name, iconname);
                hIcon = LoadIconFromFile(buffer);
                if (hIcon)
                    break;
            }

            if (hIcon) {
                nIconPos = ImageList_AddIcon(hSmall, hIcon);
                ImageList_AddIcon(hLarge, hIcon);
                if (nIconPos != -1)
                    mess_icon_index[the_index] = nIconPos;
            }
        }
    }
    return nIconPos;
}

static void MyFillSoftwareList(int nGame)
{
	int nBasePaths;
	int i;
	LPCSTR *plpBasePaths;
	LPCSTR lpExtraPath;
	const struct GameDriver *drv;

	drv = drivers[nGame];

	nBasePaths = GetMessSoftwarePathCount();
	plpBasePaths = alloca(sizeof(LPCSTR) * nBasePaths);
	for (i = 0; i < nBasePaths; i++)
		plpBasePaths[i] = GetMessSoftwarePath(i);

	lpExtraPath = GetGameOptions(nGame)->extra_software_paths;

	FillSoftwareList(s_pSoftwareListView, nGame, nBasePaths, plpBasePaths, lpExtraPath);
}

static void MessUpdateSoftwareList(void)
{
	MyFillSoftwareList(GetSelectedPickItem());
}

static void MessSetPickerDefaults(void)
{
    int i;
    size_t nDefaultSize = 0;
    char *default_software = NULL;
    char *s;

    for (i = 0; i < options.image_count; i++)
        nDefaultSize += strlen(options.image_files[i].name) + 1;

    if (nDefaultSize) {
        default_software = malloc(nDefaultSize);
        if (default_software) {
            s = NULL;
            for (i = 0; i < options.image_count; i++) {
                if (s)
                    *(s++) = '|';
                else
                    s = default_software;
                strcpy(s, options.image_files[i].name);
                s += strlen(s);
            }
        }
    }

    SetDefaultSoftware(default_software);

    if (default_software)
        free(default_software);
}

static void InitMessPicker(void)
{
	struct SmartListViewOptions opts;
	char *default_software;
	char *this_software;
	char *s;
	int i;

	memset(&opts, 0, sizeof(opts));
	opts.pClass = &s_softwareListClass;
	opts.hwndParent = hMain;
	opts.nIDDlgItem = IDC_LIST2;
	opts.hBackground = hBackground;
	opts.hPALbg = hPALbg;
	opts.bmDesc = bmDesc;
	opts.hSmall = hSmall;
	opts.hLarge = hLarge;
	opts.rgbListFontColor = GetListFontColor();
	opts.bOldControl = oldControl;
	s_pSoftwareListView = SmartListView_Init(&opts);

	SmartListView_SetTotalItems(s_pSoftwareListView, MessImageCount());
	SmartListView_SetSorting(s_pSoftwareListView, MESS_COLUMN_IMAGES, FALSE);

	/* subclass the list view */
	SetWindowLong(s_pSoftwareListView->hwndListView, GWL_WNDPROC, (LONG)ListViewWndProc);

	default_software = strdup(GetDefaultSoftware());

	if (default_software) {
		this_software = default_software;
		while(this_software && *this_software) {
			s = strchr(this_software, '|');
			if (s)
				*(s++) = '\0';
			else
				s = NULL;

			i = MessLookupByFilename(this_software);
			if (i >= 0)
				SmartListView_SelectItem(s_pSoftwareListView, i, this_software == default_software);

			this_software = s;
		}

		free(default_software);
	}
}

static void MessCreateCommandLine(char *pCmdLine, options_type *pOpts, const struct GameDriver *gamedrv)
{
	int i;

	for (i = 0; i < options.image_count; i++)
	{
		const char *optname = device_brieftypename(options.image_files[i].type);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%s \"%s\"", optname, options.image_files[i].name);
	}

	if ((pOpts->ram_size != 0) && ram_is_valid_option(gamedrv, pOpts->ram_size))
		sprintf(&pCmdLine[strlen(pCmdLine)], " -ramsize %d", pOpts->ram_size);
	if (pOpts->printer[0])
		sprintf(&pCmdLine[strlen(pCmdLine)], " -prin \"%s\"", pOpts->printer);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -%snewui", pOpts->use_new_ui ? "" : "no");
}

/* ------------------------------------------------------------------------ *
 * Open others dialog                                                       *
 * ------------------------------------------------------------------------ */

static BOOL CommonFileImageDialog(char *the_last_directory, common_file_dialog_proc cfd, char *filename, mess_image_type *imagetypes)
{
    BOOL success;
    OPENFILENAME of;
    char szFilter[2048];
    LPSTR s;
	const char *typname;
    int i;

	s = szFilter;
    *filename = 0;

    // Common image types
    strcpy(s, "Common image types");
    s += strlen(s) + 1;
    for (i = 0; imagetypes[i].ext; i++) {
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ';';
    }
    *(s++) = '\0';

    // All files
    strcpy(s, "All files (*.*)");
    s += strlen(s) + 1;
    strcpy(s, "*.*");
    s += strlen(s) + 1;

    // The others
    for (i = 0; imagetypes[i].ext; i++) {
		if (imagetypes[i].type == IO_ZIP)
			typname = "Compressed images";
		else
			typname = lookupdevice(imagetypes[i].type)->dlgname;

        strcpy(s, typname);
        //strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        strcpy(s, " (*.");
        //strcpy(s, " files (*.");
        s += strlen(s);
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ')';
        *(s++) = '\0';
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = '\0';
    }
    *(s++) = '\0';

    of.lStructSize = sizeof(of);
    of.hwndOwner = hMain;
    of.hInstance = NULL;
    of.lpstrFilter = szFilter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = filename;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = NULL;
    of.nMaxFileTitle = 0;
    of.lpstrInitialDir = last_directory;
    of.lpstrTitle = NULL;
    of.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = "rom";
    of.lCustData = 0;
    of.lpfnHook = NULL;
    of.lpTemplateName = NULL;

    success = cfd(&of);
    if (success)
    {
        //GetDirectory(filename,last_directory,sizeof(last_directory));
    }

    return success;
}

static void MessSetupDevice(common_file_dialog_proc cfd, int iDevice)
{
	char filename[MAX_PATH];
	mess_image_type imagetypes[64];

	SetupImageTypes(GetSelectedPickItem(), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, iDevice);

	if (CommonFileImageDialog(last_directory, cfd, filename, imagetypes))
		MessIntroduceItem(s_pSoftwareListView, filename, imagetypes);
}

static void MessOpenOtherSoftware(int iDevice)
{
    MessSetupDevice(GetOpenFileName, iDevice);
}

static void MessCreateDevice(int iDevice)
{
    MessSetupDevice(GetSaveFileName, iDevice);
}

/* ------------------------------------------------------------------------ *
 * Software List Class                                                      *
 * ------------------------------------------------------------------------ */

static void SoftwareListClass_Run(struct SmartListView *pListView)
{
	MamePlayGame();
}

static int SoftwareListClass_WhichIcon(struct SmartListView *pListView, int nItem)
{
    int nType;
    int nIcon;

    nType = GetImageType(nItem);

    nIcon = GetMessIcon(GetSelectedPickItem(), nType);
    if (!nIcon) {
		switch(nType) {
		case IO_UNKNOWN:
			/* Unknowns */
			nIcon = 2;
			break;

		case IO_BAD:
			/* Bad files */
			nIcon = 3;
			break;

		default:
			nIcon = lookupdevice(nType)->icon;
			break;
		}
    }
    return nIcon;
}

static void SoftwareListClass_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths)
{
	if (pWidths)
		GetMessColumnWidths(pWidths);
	if (pOrder)
		GetMessColumnOrder(pOrder);
	if (pShown)
		GetMessColumnShown(pShown);
}

static void SoftwareListClass_SetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths)
{
	if (pWidths)
		SetMessColumnWidths(pWidths);
	if (pOrder)
		SetMessColumnOrder(pOrder);
	if (pShown)
		SetMessColumnShown(pShown);
}

static BOOL SoftwareListClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow)
{
	BOOL bResult;
	BOOL bNewScreenShot;
	const char *name;
	char *s;
	char *newname;

	bResult = SoftwareList_ItemChanged(pListView, bWasSelected, bNowSelected, nRow);

	if (!bWasSelected && bNowSelected) {
		name = GetImageName(nRow);
		s = strrchr(name, '\\');
		if (s)
			name = s + 1;
		newname = alloca(strlen(name) + 1);
		strcpy(newname, name);
		s = strrchr(newname, '.');
		if (s)
			*s = '\0';

		bNewScreenShot = LoadScreenShotEx(GetSelectedPickItem(), newname, nPictType);
		if (bNewScreenShot)
        {
            HWND hWnd;

            if (GetShowScreenShot() &&  (hWnd = GetDlgItem(hMain, IDC_SSFRAME)))
            {
                RECT    rect;
                HWND    hParent;
                POINT   p = {0, 0};

                hParent = GetParent(hWnd);

                GetWindowRect(hWnd,&rect);
                ClientToScreen(hParent, &p);
                OffsetRect(&rect, -p.x, -p.y);
                InvalidateRect(hParent, &rect, FALSE);
                UpdateWindow(hParent);
            }
        }
    }
	return bResult;
}

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id) {
	case ID_MESS_OPEN_SOFTWARE:
		MessOpenOtherSoftware(IO_END);
		break;

	case ID_MESS_CREATE_SOFTWARE:
		MessCreateDevice(IO_END);
		break;

#ifdef MAME_DEBUG
	case ID_MESS_RUN_TESTS:
		MessTestsBegin();
		break;
#endif /* MAME_DEBUG */
	}
	return FALSE;
}

/* ------------------------------------------------------------------------ *
 * New File Manager                                                         *
 *                                                                          *
 * This code implements a MESS32 specific file manger.  However, it isn't   *
 * ready for prime time so it isn't enabled by default                      *
 * ------------------------------------------------------------------------ */

//static const char *s_pInitialFileName;
static BOOL s_bChosen;
static struct SmartListView *s_pFileMgrListView;
static long s_lSelectedItem;

static void FileMgrListClass_Run(struct SmartListView *pListView)
{
	s_bChosen = TRUE;
}

/*
static struct SmartListViewClass s_filemgrListClass =
{
	sizeof(struct SingleItemSmartListView),
	FileMgrListClass_Run,
	SingleItemSmartListViewClass_ItemChanged,
	SoftwareListClass_WhichIcon,
	SoftwareList_GetText,
	SoftwareListClass_GetColumnInfo,
	SoftwareListClass_SetColumnInfo,
	SingleItemSmartListViewClass_IsItemSelected,
	Compare_TextCaseInsensitive,
	SoftwareList_CanIdle,
	SoftwareList_Idle,
	sizeof(mess_column_names) / sizeof(mess_column_names[0]),
	mess_column_names
};
*/
static void EndFileManager(HWND hDlg, long lSelectedItem)
{
	s_lSelectedItem = lSelectedItem;
	PostMessage(hDlg, WM_QUIT, 0, 0);

	if (s_pFileMgrListView) {
		SmartListView_Free(s_pFileMgrListView);
		s_pFileMgrListView = NULL;
	}
}

#ifdef __NOT_USED__
static INT_PTR CALLBACK FileManagerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct SmartListViewOptions opts;
	int i;

	if (s_pFileMgrListView && SmartListView_IsEvent(s_pFileMgrListView, message, wParam, lParam)) {
		return SmartListView_HandleEvent(s_pFileMgrListView, message, wParam, lParam);
	}

	switch(message) {
	case WM_INITDIALOG:
		/* Initialize */
		CenterSubDialog(win_video_window, hDlg, FALSE); /* FIXME */

		s_bChosen = FALSE;
		s_lSelectedItem = -1;

		assert((sizeof(mess_column_names) / sizeof(mess_column_names[0])) == MESS_COLUMN_MAX);

		memset(&opts, 0, sizeof(opts));
		opts.pClass = &s_filemgrListClass;
		opts.hwndParent = hDlg;
		opts.nIDDlgItem = IDC_LIST2;
		opts.hBackground = NULL;
		opts.hPALbg = hPALbg;
		opts.hSmall = hSmall;
		opts.hLarge = hLarge;
		opts.rgbListFontColor = GetListFontColor();
		opts.bOldControl = oldControl;
		opts.bCenterOnParent = TRUE;
		opts.nInsetPixels = 10;

		s_pFileMgrListView = SmartListView_Init(&opts);
		if (!s_pFileMgrListView) {
			/* PANIC */
			EndFileManager(hDlg, -1);
			return FALSE;
		}

		SmartListView_SetTotalItems(s_pFileMgrListView, MessImageCount());

		if (s_pInitialFileName && *s_pInitialFileName) {
			i = MessLookupByFilename(s_pInitialFileName);
			if (i >= 0)
				SmartListView_SelectItem(s_pFileMgrListView, i, TRUE);
		}

		ShowWindow(hDlg, TRUE);
		break;

    case WM_COMMAND:
		switch(wParam) {
		case IDOK:
			EndFileManager(hDlg, SingleItemSmartListView_GetSelectedItem(s_pFileMgrListView));
			break;

		case IDCANCEL:
			EndFileManager(hDlg, -1);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
#endif

/* osd_select_file allows MESS32 to override the default file manager
 *
 * sel indexes an entry in options.image_files[]
 *
 * Returns:
 *	0 if the core should handle it
 *  1 if we successfully selected a file
 * -1 if we cancelled
 */
/*
int osd_select_file(int sel, char *filename)
{
	MSG msg;
	HWND hDlg;
	int nSelectedItem;
	int result;
	BOOL bDialogDone;

	if (GetUseNewFileMgr(GetSelectedPickItem())) {
		s_pInitialFileName = filename;

		ShowCursor(TRUE);

		if (MAME32App.m_pDisplay->AllowModalDialog)
			MAME32App.m_pDisplay->AllowModalDialog(TRUE);

		hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FILEMGR), win_video_window, FileManagerProc);

		bDialogDone = FALSE;
		while(!bDialogDone) {
			SmartListView_IdleUntilMsg(s_pFileMgrListView);

			do {
				if (GetMessage(&msg, NULL, 0, 0) && msg.message != WM_CLOSE) {
					if (!IsDialogMessage(hDlg, &msg)) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				else {
					bDialogDone = TRUE;
				}
			}
			while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
		}

		DestroyWindow(hDlg);
		nSelectedItem = s_lSelectedItem; //DialogBox(hInst, MAKEINTRESOURCE(IDD_FILEMGR), MAME32App.m_hWnd, FileManagerProc);

		if (MAME32App.m_pDisplay->AllowModalDialog)
			MAME32App.m_pDisplay->AllowModalDialog(FALSE);

		ShowCursor(FALSE);

		result = -1;
		if (nSelectedItem > -1) {
			strcpy(filename, GetImageFullName(nSelectedItem));
			result = 1;
		}
		else {
			result = -1;
		}
	}
	else {
		result = 0;
	}
	return result;
}
*/
/* ------------------------------------------------------------------------ *
 * Mess32 Diagnostics                                                       *
 * ------------------------------------------------------------------------ */

#ifdef MAME_DEBUG

static int s_nOriginalPick;
static BOOL s_bRunningTests = FALSE;

static void MessTestsColumns(void)
{
	int i, j;
	int oldshown[MESS_COLUMN_MAX];
	int shown[MESS_COLUMN_MAX];

	GetMessColumnShown(oldshown);

	shown[0] = 1;
	for (i = 0; i < (1<<(MESS_COLUMN_MAX-1)); i++) {
		for (j = 1; j < MESS_COLUMN_MAX; j++)
			shown[j] = (i & (1<<(j-1))) ? 1 : 0;

		SetMessColumnShown(shown);
		SmartListView_ResetColumnDisplay(s_pSoftwareListView);
	}

	SetMessColumnShown(oldshown);
	SmartListView_ResetColumnDisplay(s_pSoftwareListView);
}

static void MessTestsBegin(void)
{
	int nOriginalPick;

	nOriginalPick = GetSelectedPick();

	/* If we are running already, keep our old defaults */
	if (!s_bRunningTests) {
		s_bRunningTests = TRUE;
		s_nOriginalPick = nOriginalPick;
	}

	MessTestsColumns();

	if (nOriginalPick == 0)
		SetSelectedPick(1);
	SetSelectedPick(0);
}

static void MessTestsCompleted(void)
{
	/* We are done */
	SetSelectedPick(s_nOriginalPick);
	s_bRunningTests = FALSE;
	MessageBoxA(hMain, "Tests successfully completed!", MAME32NAME, MB_OK);
}

static void MessTestsDoneIdle(void)
{
	int nNewGame;

	if (s_bRunningTests) {
		nNewGame = GetSelectedPick() + 1;
		if (nNewGame >= game_count) {
			MessTestsCompleted();
		}
		else {
			MessTestsFlex(s_pSoftwareListView, drivers[GetSelectedPickItem()]);
			SetSelectedPick(nNewGame);
		}
	}
}

#endif /* MAME_DEBUG */
