#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>
#include <winuser.h>

#include "ui/mame32.h"
#include "ui/resourcems.h"
#include "mess.h"
#include "configms.h"
#include "SmartListView.h"
#include "SoftwareList.h"
#include "windows/window.h"
#include "messwin.h"
#include "rc.h"
#include "utils.h"
#include "optionsms.h"

#ifdef _MSC_VER
#define alloca _alloca
#endif

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
static void MyFillSoftwareList(int nGame, BOOL bForce);
static void MessUpdateSoftwareList(void);
static void MessOpenOtherSoftware(int iDevice);
static void MessCreateDevice(int iDevice);
static void MessReadMountedSoftware(int nGame);
static void MessWriteMountedSoftware(int nGame);
static BOOL CreateMessIcons(void);

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
static void MessTestsDoneIdle(void);
#endif /* MAME_DEBUG */

#ifdef bool
#undef bool
#endif

static int s_nCurrentGame;
static char s_szSelectedItem[256];

#include "ui/win32ui.c"

struct deviceentry
{
	const char *icon_name;
	const char *dlgname;
};

/* ------------------------------------------------------------------------ *
 * Image types
 *
 * IO_ZIP is used for ZIP files
 * IO_ALIAS is used for unknown types
 * IO_COUNT is used for bad files
 * ------------------------------------------------------------------------ */

#define IO_ZIP		(IO_COUNT + 0)
#define IO_BAD		(IO_COUNT + 1)
#define IO_UNKNOWN	(IO_COUNT + 2)

/* TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape! */
static struct deviceentry s_devices[] =
{
	{ "roms",		"Cartridge images" },	/* IO_CARTSLOT */
	{ "floppy",		"Floppy disk images" },	/* IO_FLOPPY */
	{ "hard",		"Hard disk images" },	/* IO_HARDDISK */
	{ NULL,			"Cylinders" },			/* IO_CYLINDER */
	{ NULL,			"Cassette images" },	/* IO_CASSETTE */
	{ NULL,			"Punchcard images" },	/* IO_PUNCHCARD */
	{ NULL,			"Punchtape images" },	/* IO_PUNCHTAPE */
	{ NULL,			"Printer Output" },		/* IO_PRINTER */
	{ NULL,			"Serial Output" },		/* IO_SERIAL */
	{ NULL,			"Parallel Output" },	/* IO_PARALLEL */
	{ "snapshot",	"Snapshots" },			/* IO_SNAPSHOT */
	{ "snapshot",	"Quickloads" }			/* IO_QUICKLOAD */
};

static void AssertValidDevice(int d)
{
	assert((sizeof(s_devices) / sizeof(s_devices[0])) == IO_COUNT);
	assert(((d >= 0) && (d < IO_COUNT)) || (d == IO_UNKNOWN) || (d == IO_BAD) || (d == IO_ZIP));
}

static const struct deviceentry *lookupdevice(int d)
{
	assert(d >= 0);
	assert(d < IO_COUNT);
	AssertValidDevice(d);
	return &s_devices[d];
}

/* ------------------------------------------------------------------------ */

static int requested_device_type(char *tchar)
{
	int device = -1;
	int i;

    logerror("Requested device is %s\n", tchar);

	if (*tchar == '-')
	{
		tchar++;

		for (i = 1; i < IO_COUNT; i++)
		{
			if (!stricmp(tchar, device_typename(i)) || !stricmp(tchar, device_brieftypename(i)))
			{
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

    if (!mess_icon_index)
	{
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

    if ((nSoftwareType >= 0) && (nSoftwareType < IO_COUNT))
	{
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

static void MyFillSoftwareList(int nGame, BOOL bForce)
{
	int i;
	const struct GameDriver *drv;
	const char *software_dirs;
	const char *extra_path;
	char *paths;
	int software_dirs_length;
	int path_count;
	LPCSTR *pathsv;

	if (!bForce && (nGame == s_nCurrentGame))
		return;
	s_nCurrentGame = nGame;

	drv = drivers[nGame];
	software_dirs = GetSoftwareDirs();
	extra_path = GetExtraSoftwarePaths(nGame);

	software_dirs_length = strlen(software_dirs);
	paths = alloca(software_dirs_length + 1);
	strcpy(paths, software_dirs);

	path_count = 1;
	for (i = 0; i < software_dirs_length; i++)
	{
		if (paths[i] == ';')
			path_count++;
	}

	pathsv = alloca(sizeof(LPCSTR) * path_count);
	path_count = 0;
	pathsv[path_count++] = paths;
	for (i = 0; i < software_dirs_length; i++)
	{
		if (paths[i] == ';')
		{
			paths[i] = '\0';
			pathsv[path_count++] = &paths[i+1];
		}
	}

	FillSoftwareList(s_pSoftwareListView, nGame, path_count, pathsv, extra_path);
}

static void MessUpdateSoftwareList(void)
{
	MyFillSoftwareList(GetSelectedPickItem(), TRUE);
}

static BOOL IsSoftwarePaneDevice(int devtype)
{
	assert(devtype >= 0);
	assert(devtype < IO_COUNT);
	return devtype != IO_PRINTER;
}

static void MessReadMountedSoftware(int nGame)
{
	const char *selected_software_const;
	char *this_software;
	char *selected_software;
	char *s;
	int devtype;
	int i;

	SmartListView_UnselectAll(s_pSoftwareListView);

	for (devtype = 0; devtype < IO_COUNT; devtype++)
	{
		if (!IsSoftwarePaneDevice(devtype))
			continue;

		selected_software_const = GetSelectedSoftware(nGame, devtype);
		if (selected_software_const && selected_software_const[0])
		{
			selected_software = alloca(strlen(selected_software_const) + 1);
			strcpy(selected_software, selected_software_const);

			this_software = selected_software;
			while(this_software && *this_software)
			{
				s = strchr(this_software, IMAGE_SEPARATOR);
				if (s)
					*(s++) = '\0';
				else
					s = NULL;

				i = MessLookupByFilename(this_software);
				if (i >= 0)
					SmartListView_SelectItem(s_pSoftwareListView, i, this_software == selected_software);

				this_software = s;
			}
		}
	}
}

static void MessWriteMountedSoftware(int nGame)
{
	int i;
	int devtype;
	BOOL dirty = FALSE;
	const char *softwarename;
	char *newsoftware[IO_COUNT];
	char *s;
	size_t pos;

	memset(newsoftware, 0, sizeof(newsoftware));

    for (i = 0; i < options.image_count; i++)
	{
		softwarename = options.image_files[i].name;
		devtype = options.image_files[i].type;

		if ((devtype == IO_ZIP) || (devtype == IO_UNKNOWN) || (devtype == IO_BAD))
			continue;

		assert(softwarename);
		assert(devtype >= 0);
		assert(devtype < IO_COUNT);

		if (newsoftware[devtype])
		{
			pos = strlen(newsoftware[devtype]);
			s = realloc(newsoftware[devtype], pos + 1 + strlen(softwarename) + 1);
		}
		else
		{
			pos = 0;
			s = malloc(strlen(softwarename) + 1);
		}
		if (!s)
			continue;

		if (pos > 0)
			s[pos++] = IMAGE_SEPARATOR;

		strcpy(&s[pos], softwarename);
		newsoftware[devtype] = s;
	}

	for (devtype = 0; devtype < IO_COUNT; devtype++)
	{
		if (!IsSoftwarePaneDevice(devtype))
			continue;

		softwarename = newsoftware[devtype] ? newsoftware[devtype] : "";
		if (strcmp(GetSelectedSoftware(nGame, devtype), softwarename))
		{
			SetSelectedSoftware(nGame, devtype, newsoftware[devtype]);
			dirty = TRUE;
		}

		if (newsoftware[devtype])
			free(newsoftware[devtype]);
	}

	if (dirty)
	{
		SetGameUsesDefaults(nGame, FALSE);
		SaveGameOptions(nGame);
	}
}

/*static void MessSetPickerDefaults(void)
{
    int i;
    size_t nDefaultSize = 0;
    char *default_software = NULL;
    char *s = NULL;

    for (i = 0; i < options.image_count; i++)
        nDefaultSize += strlen(options.image_files[i].name) + 1;

    if (nDefaultSize)
	{
        default_software = alloca(nDefaultSize);
        for (i = 0; i < options.image_count; i++)
		{
            if (s)
                *(s++) = '|';
            else
                s = default_software;
            strcpy(s, options.image_files[i].name);
            s += strlen(s);
        }
    }

    SetDefaultSoftware(default_software);
}*/

static void InitMessPicker(void)
{
	struct SmartListViewOptions opts;

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

	s_nCurrentGame = -1;
}

/*static void MessGetPickerDefaults(void)
{
	const char *default_software_const;
	char *default_software;
	char *this_software;
	char *s;
	int i;

	default_software_const = GetDefaultSoftware();

	if (default_software_const && *default_software_const)
	{
		default_software = alloca(strlen(default_software_const) + 1);
		strcpy(default_software, default_software_const);

		this_software = default_software;
		while(this_software && *this_software)
		{
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
	}
}*/

static void MessCreateCommandLine(char *pCmdLine, options_type *pOpts, const struct GameDriver *gamedrv)
{
	int i, imgtype;
	const char *optname;
	const char *software;

	for (i = 0; i < options.image_count; i++)
	{
		imgtype = options.image_files[i].type;
		assert(imgtype < IO_COUNT);
		optname = device_brieftypename(imgtype);
		software = options.image_files[i].name;
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%s \"%s\"", optname, software);
	}

	if ((pOpts->ram_size != 0) && ram_is_valid_option(gamedrv, pOpts->ram_size))
		sprintf(&pCmdLine[strlen(pCmdLine)], " -ramsize %d", pOpts->ram_size);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -%snewui", pOpts->use_new_ui ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -writeconfig");
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
    for (i = 0; imagetypes[i].ext; i++)
	{
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

static int LookupIcon(const char *icon_name)
{
	int i;
	for (i = 0; g_iconData[i].icon_name; i++)
	{
		if (!strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}

static int SoftwareListClass_WhichIcon(struct SmartListView *pListView, int nItem)
{
    int nType;
    int nIcon;
	const char *icon_name;

    nType = GetImageType(nItem);

    nIcon = GetMessIcon(GetSelectedPickItem(), nType);
    if (!nIcon) {
		switch(nType) {
		case IO_UNKNOWN:
			/* Unknowns */
			nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
			break;

		case IO_BAD:
		case IO_ZIP:
			/* Bad files */
			nIcon = FindIconIndex(IDI_WIN_REDX);
			break;

		default:
			icon_name = lookupdevice(nType)->icon_name;
			if (!icon_name)
				icon_name = device_typename(nType);
			nIcon = LookupIcon(icon_name);
			if (nIcon < 0)
				nIcon = LookupIcon("unknown");
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
	const char *name;
	char *s;

	bResult = SoftwareList_ItemChanged(pListView, bWasSelected, bNowSelected, nRow);

	if (!bWasSelected && bNowSelected) {
		name = GetImageName(nRow);
		s = strrchr(name, '\\');
		if (s)
			name = s + 1;

		strncpyz(s_szSelectedItem, name, sizeof(s_szSelectedItem) / sizeof(s_szSelectedItem[0]));
		s = strrchr(s_szSelectedItem, '.');
		if (s)
			*s = '\0';

		UpdateScreenShot();
    }
	return bResult;
}

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id) {
	case ID_MESS_OPEN_SOFTWARE:
		MessOpenOtherSoftware(IO_COUNT);
		break;

	case ID_MESS_CREATE_SOFTWARE:
		MessCreateDevice(IO_COUNT);
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
