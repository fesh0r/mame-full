//============================================================
//
//	mess32ui.c - MESS extensions to src/ui/win32ui.c
//
//============================================================

#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>
#include <winuser.h>

#include "ui/screenshot.h"
#include "ui/bitmask.h"
#include "ui/mame32.h"
#include "ui/resourcems.h"
#include "ui/options.h"
#include "mess.h"
#include "configms.h"
#include "softwarelist.h"
#include "devview.h"
#include "windows/window.h"
#include "messwin.h"
#include "rc.h"
#include "utils.h"
#include "ui_text.h"

#ifdef _MSC_VER
#define alloca _alloca
#endif

static int requested_device_type(char *tchar);
static void MessCreateCommandLine(char *pCmdLine, options_type *pOpts, const struct GameDriver *gamedrv);

static int SoftwarePicker_GetItemImage(int nItem);
static void SoftwarePicker_LeavingItem(int nItem);
static void SoftwarePicker_EnteringItem(int nItem);
static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn);

static LPCTSTR SoftwareTabView_GetTabShortName(int tab);
static LPCTSTR SoftwareTabView_GetTabLongName(int tab);
static void SoftwareTabView_OnSelectionChanged(void);
static void SoftwareTabView_OnMoveSize(void);
static void SetupSoftwareTabView(void);

static const char *mess_column_names[] =
{
    "Software",
	"Goodname",
    "Manufacturer",
    "Year",
    "Playable",
	"CRC",
	"SHA-1",
	"MD5"
};

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

static BOOL DevView_GetOpenFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
static BOOL DevView_GetCreateFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
static void DevView_SetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const struct IODevice *dev, int nID, LPCTSTR pszFilename);
static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const struct IODevice *dev, int nID, LPTSTR pszBuffer, UINT nBufferLength);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
#endif /* MAME_DEBUG */

static int s_nCurrentGame;
static char s_szSelectedItem[MAX_PATH];

#include "ui/win32ui.c"

struct deviceentry
{
	int dev_type;
	const char *icon_name;
	const char *dlgname;
};

// The Device View is not fully implemented, so for the time being, the tab
// view will be there, but just hidden
#define HIDE_TABVIEW

#ifdef HIDE_TABVIEW
static BOOL AlwaysFalse(void)
{
	return FALSE;
}
#endif



static const struct PickerCallbacks s_softwareListCallbacks =
{
	SetMessSortColumn,					// pfnSetSortColumn
	GetMessSortColumn,					// pfnGetSortColumn
	SetMessSortReverse,					// pfnSetSortReverse
	GetMessSortReverse,					// pfnGetSortReverse
	NULL,								// pfnSetViewMode
	GetViewMode,						// pfnGetViewMode
	SetMessColumnWidths,				// pfnSetColumnWidths
	GetMessColumnWidths,				// pfnGetColumnWidths
	SetMessColumnOrder,					// pfnSetColumnOrder
	GetMessColumnOrder,					// pfnGetColumnOrder
	SetMessColumnShown,					// pfnSetColumnShown
	GetMessColumnShown,					// pfnGetColumnShown
	NULL,								// pfnGetOffsetChildren

	NULL,								// pfnCompare
	MamePlayGame,						// pfnDoubleClick
	SoftwarePicker_GetItemString,		// pfnGetItemString
	SoftwarePicker_GetItemImage,		// pfnGetItemImage
	SoftwarePicker_LeavingItem,			// pfnLeavingItem
	SoftwarePicker_EnteringItem,		// pfnEnteringItem
	NULL,								// pfnBeginListViewDrag
	NULL,								// pfnFindItemParent
	SoftwarePicker_OnIdle,				// pfnIdle
	SoftwarePicker_OnHeaderContextMenu,	// pfnOnHeaderContextMenu
	NULL								// pfnOnBodyContextMenu 
};



static const struct TabViewCallbacks s_softwareTabViewCallbacks =
{
#ifdef HIDE_TABVIEW
	AlwaysFalse,						// pfnGetShowTabCtrl
	NULL,								// pfnSetCurrentTab
	NULL,								// pfnGetCurrentTab
#else
	NULL,								// pfnGetShowTabCtrl
	SetCurrentSoftwareTab,				// pfnSetCurrentTab
	GetCurrentSoftwareTab,				// pfnGetCurrentTab
#endif
	NULL,								// pfnSetShowTab 
	NULL,								// pfnGetShowTab 

	SoftwareTabView_GetTabShortName,	// pfnGetTabShortName
	SoftwareTabView_GetTabLongName,		// pfnGetTabLongName

	SoftwareTabView_OnSelectionChanged,	// pfnOnSelectionChanged
	SoftwareTabView_OnMoveSize			// pfnOnMoveSize 
};



// ------------------------------------------------------------------------
// Image types
//
// IO_ZIP is used for ZIP files
// IO_ALIAS is used for unknown types
// IO_COUNT is used for bad files
// ------------------------------------------------------------------------

#define IO_ZIP		(IO_COUNT + 0)
#define IO_BAD		(IO_COUNT + 1)
#define IO_UNKNOWN	(IO_COUNT + 2)

// TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape!
static struct deviceentry s_devices[] =
{
	{ IO_CARTSLOT,	"roms",		"Cartridge images" },
	{ IO_FLOPPY,	"floppy",	"Floppy disk images" },
	{ IO_HARDDISK,	"hard",		"Hard disk images" },
	{ IO_CYLINDER,	NULL,		"Cylinders" },			
	{ IO_CASSETTE,	NULL,		"Cassette images" },
	{ IO_PUNCHCARD,	NULL,		"Punchcard images" },
	{ IO_PUNCHTAPE,	NULL,		"Punchtape images" },
	{ IO_PRINTER,	NULL,		"Printer Output" },
	{ IO_SERIAL,	NULL,		"Serial Output" },
	{ IO_PARALLEL,	NULL,		"Parallel Output" },
	{ IO_SNAPSHOT,	"snapshot",	"Snapshots" },
	{ IO_QUICKLOAD,	"snapshot",	"Quickloads" },
	{ IO_MEMCARD,	NULL,		"Memory cards" }
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



// ------------------------------------------------------------------------

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



// ------------------------------------------------------------------------
// UI
// ------------------------------------------------------------------------

static BOOL CreateMessIcons(void)
{
    int i;

    if (!mess_icon_index)
	{
        mess_icon_index = auto_malloc(sizeof(int) * game_count * IO_COUNT);
        if (!mess_icon_index)
            return FALSE;
    }

    for (i = 0; i < (game_count * IO_COUNT); i++)
        mess_icon_index[i] = 0;

	// Associate the image lists with the list view control.
	ListView_SetImageList(GetDlgItem(hMain, IDC_SWLIST), hSmall, LVSIL_SMALL);
	ListView_SetImageList(GetDlgItem(hMain, IDC_SWLIST), hLarge, LVSIL_NORMAL);
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
        if (!nIconPos)
		{
            for (drv = drivers[nGame]; drv; drv = drv->clone_of)
			{
                sprintf(buffer, "%s/%s", drv->name, iconname);
                hIcon = LoadIconFromFile(buffer);
                if (hIcon)
                    break;
            }

            if (hIcon)
			{
                nIconPos = ImageList_AddIcon(hSmall, hIcon);
                ImageList_AddIcon(hLarge, hIcon);
                if (nIconPos != -1)
                    mess_icon_index[the_index] = nIconPos;
            }
        }
    }
    return nIconPos;
}



static void MessHashErrorProc(const char *message)
{
	SetStatusBarTextF(0, "Hash file: %s", message);
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

	DevView_SetDriver(GetDlgItem(hMain, IDC_SWDEVVIEW), Picker_GetSelectedItem(hwndList));

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

	FillSoftwareList(GetDlgItem(hMain, IDC_SWLIST), nGame, path_count, pathsv, extra_path,
		MessHashErrorProc);

	Picker_Sort(GetDlgItem(hMain, IDC_SWLIST));
}



static void MessUpdateSoftwareList(void)
{
	MyFillSoftwareList(Picker_GetSelectedItem(hwndList), TRUE);
}



static BOOL IsSoftwarePaneDevice(int devtype)
{
	assert(devtype >= 0);
	assert(devtype < IO_COUNT);
	return devtype != IO_PRINTER;
}



static void InternalSetSelectedSoftware(int nGame, int nDevType, const char *pszSoftware)
{
	if (!pszSoftware)
		pszSoftware = TEXT("");

	if (strcmp(GetSelectedSoftware(nGame, nDevType), pszSoftware))
	{
		SetSelectedSoftware(nGame, nDevType, pszSoftware);
		SetGameUsesDefaults(nGame, FALSE);
		SaveGameOptions(nGame);
	}
}



static void MessReadMountedSoftware(int nGame)
{
	HWND hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);
	const char *selected_software_const;
	char *this_software;
	char *selected_software;
	char *s;
	int devtype;
	int i;

	ListView_SetItemState(hwndSoftware, -1, 0, LVIS_SELECTED);

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
				{
					if (this_software == selected_software)
					{
						ListView_SetItemState(hwndSoftware, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						ListView_EnsureVisible(hwndSoftware, i, FALSE);
					}
					else
					{
						ListView_SetItemState(hwndSoftware, i, LVIS_SELECTED, LVIS_SELECTED);
					}
				}

				this_software = s;
			}
		}
	}
}



static void MessWriteMountedSoftware(int nGame)
{
	int i;
	int devtype;
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

		InternalSetSelectedSoftware(nGame, devtype, newsoftware[devtype]);

		if (newsoftware[devtype])
			free(newsoftware[devtype]);
	}
}



static void InitMessPicker(void)
{
	struct PickerOptions opts;
	HWND hwndSoftware;

	hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);

	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_softwareListCallbacks;
	opts.nColumnCount = MESS_COLUMN_MAX;
	opts.ppszColumnNames = mess_column_names;
	SetupPicker(hwndSoftware, &opts);

	SetWindowLong(hwndSoftware, GWL_STYLE, GetWindowLong(hwndSoftware, GWL_STYLE)
		| LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);

	s_nCurrentGame = -1;

	SetupSoftwareTabView();

	{
		static const struct DevViewCallbacks s_devViewCallbacks =
		{
			DevView_GetOpenFileName,
			DevView_GetCreateFileName,
			DevView_SetSelectedSoftware,
			DevView_GetSelectedSoftware
		};
		DevView_SetCallbacks(GetDlgItem(hMain, IDC_SWDEVVIEW), &s_devViewCallbacks);
	}
}



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

	if ((pOpts->mess.ram_size != 0) && ram_is_valid_option(gamedrv, pOpts->mess.ram_size))
		sprintf(&pCmdLine[strlen(pCmdLine)], " -ramsize %d", pOpts->mess.ram_size);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -%snewui", pOpts->mess.use_new_ui ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -writeconfig");
}



// ------------------------------------------------------------------------
// Open others dialog
// ------------------------------------------------------------------------

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
		if (!imagetypes[i].dev)
			typname = "Compressed images";
		else
			typname = lookupdevice(imagetypes[i].dev->type)->dlgname;

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

	SetupImageTypes(Picker_GetSelectedItem(hwndList), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, iDevice);

	if (CommonFileImageDialog(last_directory, cfd, filename, imagetypes))
		MessIntroduceItem(GetDlgItem(hMain, IDC_SWLIST), filename, imagetypes);
}



static void MessOpenOtherSoftware(int iDevice)
{
	MessSetupDevice(GetOpenFileName, iDevice);
}



static void MessCreateDevice(int iDevice)
{
	MessSetupDevice(GetSaveFileName, iDevice);
}



static BOOL DevView_GetOpenFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	mess_image_type imagetypes[64];
	SetupImageTypes(Picker_GetSelectedItem(hwndList), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, dev->type);
	return CommonFileImageDialog(last_directory, GetOpenFileName, pszFilename, imagetypes);
}



static BOOL DevView_GetCreateFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	mess_image_type imagetypes[64];
	SetupImageTypes(Picker_GetSelectedItem(hwndList), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, dev->type);
	return CommonFileImageDialog(last_directory, GetSaveFileName, pszFilename, imagetypes);
}



static void DevView_SetSelectedSoftware(HWND hwndDevView, int nDriverIndex,
	const struct IODevice *dev, int nID, LPCTSTR pszFilename)
{
	LPCSTR pszSelection;
	LPSTR pszSelectionCopy, pszNewSelection;
	LPCSTR *ppszNewSelections;
	LPSTR s, s2;
	int i, nLength;
	int nSelectionCount, nNewSelectionCount;

	// normalize filename
	if (!pszFilename)
		pszFilename = TEXT("");
	
	// get the selection
	pszSelection = GetSelectedSoftware(nDriverIndex, dev->type);

	// copy the selection into something that we can modify
	pszSelectionCopy = (LPSTR) alloca((strlen(pszSelection) + 1) * sizeof(*pszSelection));
	strcpy(pszSelectionCopy, pszSelection);

	// count number of selections and replace the commas with NULs
	s = pszSelectionCopy;
	nSelectionCount = 0;
	while(s && s[0])
	{
		nSelectionCount++;
		s = strchr(s, ',');
		if (s)
		{
			*s = '\0';
			s++;
		}
	}

	// Allocate an array of selection parts
	nNewSelectionCount = MAX(nSelectionCount, nID + 1);
	ppszNewSelections = (LPCSTR *) alloca(sizeof(*ppszNewSelections) * nNewSelectionCount);
	s = pszSelectionCopy;
	nLength = 0;
	for (i = 0; i < nNewSelectionCount; i++)
	{
		if (i == nID)
			ppszNewSelections[i] = pszFilename;
		else if (s && s[0])
			ppszNewSelections[i] = s;
		else
			ppszNewSelections[i] = TEXT("");
		nLength += strlen(ppszNewSelections[i]) + 1;

		if (s)
		{
			s = strchr(s, ',');
			if (s)
				s++;
		}
	}

	// ...and finally assemble the new selection string
	pszNewSelection = (LPSTR) alloca(nLength * sizeof(*pszNewSelection));
	s = pszNewSelection;
	for (i = 0; i < nNewSelectionCount; i++)
	{
		if (i > 0)
			*(s++) = ',';
		strcpy(s, ppszNewSelections[i]);
		s += strlen(s);
	}

	InternalSetSelectedSoftware(nDriverIndex, dev->type, pszNewSelection);
	MessReadMountedSoftware(Picker_GetSelectedItem(hwndList));
}



static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex,
	const struct IODevice *dev, int nID, LPTSTR pszBuffer, UINT nBufferLength)
{
	LPCSTR pszSelection, s;
	
	pszSelection = GetSelectedSoftware(nDriverIndex, dev->type);

	// skip over irrelevant selections
	while(pszSelection && pszSelection[0] && (nID > 0))
	{
		nID--;
		pszSelection = strchr(pszSelection, ',');
		if (pszSelection)
			pszSelection++;

	}
	if (!pszSelection || !pszSelection[0])
	{
		pszSelection = NULL;
	}
	else
	{
		s = strchr(pszSelection, ',');
		if (s)
		{
			// extract the filename, minus the comma
			nBufferLength = MIN(nBufferLength, (s - pszSelection + 1));
			memcpy(pszBuffer, pszSelection, nBufferLength * sizeof(*pszSelection));
			if (nBufferLength)
				pszBuffer[nBufferLength] = '\0';
			pszSelection = pszBuffer;
		}
	}
	return pszSelection;
}



// ------------------------------------------------------------------------
// Software List Class
// ------------------------------------------------------------------------

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



static int SoftwarePicker_GetItemImage(int nItem)
{
    int nType;
    int nIcon;
	const char *icon_name;

    nType = GetImageType(nItem);

    nIcon = GetMessIcon(Picker_GetSelectedItem(GetDlgItem(hMain, IDC_SWLIST)), nType);
    if (!nIcon)
	{
		switch(nType)
		{
			case IO_UNKNOWN:
				// Unknowns 
				nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
				break;

			case IO_BAD:
			case IO_ZIP:
				// Bad files
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



static void SoftwarePicker_LeavingItem(int nItem)
{
	HWND hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);
	SoftwareList_ItemChanged(hwndSoftware, TRUE, FALSE, nItem);
}



static void SoftwarePicker_EnteringItem(int nItem)
{
	const char *name;
	char *s;
	HWND hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);
	SoftwareList_ItemChanged(hwndSoftware, FALSE, TRUE, nItem);

	name = GetImageName(nItem);
	s = strrchr(name, '\\');
	if (s)
		name = s + 1;

	strncpyz(s_szSelectedItem, name, sizeof(s_szSelectedItem) / sizeof(s_szSelectedItem[0]));
	s = strrchr(s_szSelectedItem, '.');
	if (s)
		*s = '\0';

	UpdateScreenShot();
}



// ------------------------------------------------------------------------
// Header context menu stuff
// ------------------------------------------------------------------------

static HWND MyColumnDialogProc_hwndPicker;
static int *MyColumnDialogProc_order;
static int *MyColumnDialogProc_shown;

static void MyGetRealColumnOrder(int *order)
{
	int i, nColumnCount;
	nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	for (i = 0; i < nColumnCount; i++)
		order[i] = Picker_GetRealColumnFromViewColumn(MyColumnDialogProc_hwndPicker, i);
}



static void MyGetColumnInfo(int *order, int *shown)
{
	const struct PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnGetColumnOrder(order);
	pCallbacks->pfnGetColumnShown(shown);
}



static void MySetColumnInfo(int *order, int *shown)
{
	const struct PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnSetColumnOrder(order);
	pCallbacks->pfnSetColumnShown(shown);
}



static INT_PTR CALLBACK MyColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	LPCTSTR *ppszColumnNames = Picker_GetColumnNames(MyColumnDialogProc_hwndPicker);
	return InternalColumnDialogProc(hDlg, Msg, wParam, lParam, nColumnCount,
		MyColumnDialogProc_shown, MyColumnDialogProc_order, ppszColumnNames,
		MyGetRealColumnOrder, MyGetColumnInfo, MySetColumnInfo);
}



static BOOL RunColumnDialog(HWND hwndPicker)
{
	BOOL bResult;
	int nColumnCount;

	MyColumnDialogProc_hwndPicker = hwndPicker;
	nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);

	MyColumnDialogProc_order = alloca(nColumnCount * sizeof(int));
	MyColumnDialogProc_shown = alloca(nColumnCount * sizeof(int));
	bResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), hMain, MyColumnDialogProc);
	return bResult;
}



static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	HMENU hMenu;
	HMENU hMenuLoad;
	HWND hwndPicker;
	int nMenuItem;

	hMenuLoad = LoadMenu(NULL, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	hMenu = GetSubMenu(hMenuLoad, 0);

	nMenuItem = (int) TrackPopupMenu(hMenu,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		pt.x,
		pt.y,
		0,
		hMain,
		NULL);

	DestroyMenu(hMenuLoad);

	hwndPicker = GetDlgItem(hMain, IDC_SWLIST);

	switch(nMenuItem) {
	case ID_SORT_ASCENDING:
		SetMessSortReverse(FALSE);
		SetMessSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
		Picker_Sort(hwndPicker);
		break;

	case ID_SORT_DESCENDING:
		SetMessSortReverse(TRUE);
		SetMessSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
		Picker_Sort(hwndPicker);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (RunColumnDialog(hwndPicker))
			Picker_ResetColumnDisplay(hwndPicker);
		break;
	}
}



// ------------------------------------------------------------------------
// MessCommand
// ------------------------------------------------------------------------

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



// ------------------------------------------------------------------------
// Software Tab View 
// ------------------------------------------------------------------------

static LPCTSTR s_tabs[] =
{
	TEXT("picker\0Picker"),
	TEXT("devview\0Device View")
};



static LPCTSTR SoftwareTabView_GetTabShortName(int tab)
{
	return s_tabs[tab];
}



static LPCTSTR SoftwareTabView_GetTabLongName(int tab)
{
	return s_tabs[tab] + _tcslen(s_tabs[tab]) + 1;
}



static void SoftwareTabView_OnSelectionChanged(void)
{
	int nTab;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;

	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(hMain, IDC_SWDEVVIEW);

	nTab = TabView_GetCurrentTab(GetDlgItem(hMain, IDC_SWTAB));

	switch(nTab)
	{
		case 0:
			MessReadMountedSoftware(Picker_GetSelectedItem(hwndList));
			ShowWindow(hwndSoftwarePicker, SW_SHOW);
			ShowWindow(hwndSoftwareDevView, SW_HIDE);
			break;

		case 1:
			MessWriteMountedSoftware(Picker_GetSelectedItem(hwndList));
			ShowWindow(hwndSoftwarePicker, SW_HIDE);
			ShowWindow(hwndSoftwareDevView, SW_SHOW);
			break;
	}
}



static void SoftwareTabView_OnMoveSize(void)
{
	HWND hwndSoftwareTabView;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;
	RECT rMain, rSoftwareTabView, rClient, rTab;

	hwndSoftwareTabView = GetDlgItem(hMain, IDC_SWTAB);
	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(hMain, IDC_SWDEVVIEW);

	GetWindowRect(hwndSoftwareTabView, &rSoftwareTabView);
	GetClientRect(hMain, &rMain);
	ClientToScreen(hMain, &((POINT *) &rMain)[0]);
	ClientToScreen(hMain, &((POINT *) &rMain)[1]);

	// Calculate rClient from rSoftwareTabView in terms of rMain coordinates
	rClient.left = rSoftwareTabView.left - rMain.left;
	rClient.top = rSoftwareTabView.top - rMain.top;
	rClient.right = rSoftwareTabView.right - rMain.left;
	rClient.bottom = rSoftwareTabView.bottom - rMain.top;

	// If the tabs are visible, then make sure that the tab view's tabs are
	// not being covered up
	if (GetWindowLong(hwndSoftwareTabView, GWL_STYLE) & WS_VISIBLE)
	{
		TabCtrl_GetItemRect(hwndSoftwareTabView, 0, &rTab);
		rClient.top += rTab.bottom - rTab.top + 4;
	}

	/* Now actually move the controls */
	MoveWindow(hwndSoftwarePicker, rClient.left, rClient.top,
		rClient.right - rClient.left, rClient.bottom - rClient.top,
		TRUE);
	MoveWindow(hwndSoftwareDevView, rClient.left, rClient.top,
		rClient.right - rClient.left, rClient.bottom - rClient.top,
		TRUE);
}



static void SetupSoftwareTabView(void)
{
	struct TabViewOptions opts;

	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_softwareTabViewCallbacks;
	opts.nTabCount = sizeof(s_tabs) / sizeof(s_tabs[0]);

	SetupTabView(GetDlgItem(hMain, IDC_SWTAB), &opts);
}



// ------------------------------------------------------------------------
// Mess32 Diagnostics
// ------------------------------------------------------------------------

#ifdef MAME_DEBUG

static int s_nOriginalPick;
static UINT_PTR s_nTestingTimer;
static BOOL s_bInTimerProc;

static void MessTestsColumns(void)
{
	int i, j;
	int oldshown[MESS_COLUMN_MAX];
	int shown[MESS_COLUMN_MAX];

	GetMessColumnShown(oldshown);

	shown[0] = 1;
	for (i = 0; i < (1<<(MESS_COLUMN_MAX-1)); i++)
	{
		for (j = 1; j < MESS_COLUMN_MAX; j++)
			shown[j] = (i & (1<<(j-1))) ? 1 : 0;

		SetMessColumnShown(shown);
		Picker_ResetColumnDisplay(GetDlgItem(hMain, IDC_SWLIST));
	}

	SetMessColumnShown(oldshown);
	Picker_ResetColumnDisplay(GetDlgItem(hMain, IDC_SWLIST));
}




static void CALLBACK MessTestsTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	int nNewGame;

	// if either of the pickers have further idle work to do, or we are
	// already in this timerproc, do not do anything
	if (s_bInTimerProc || Picker_IsIdling(GetDlgItem(hMain, IDC_LIST))
		|| Picker_IsIdling(GetDlgItem(hMain, IDC_SWLIST)))
		return;
	s_bInTimerProc = TRUE;

	nNewGame = GetSelectedPick() + 1;

	if (nNewGame >= game_count)
	{
		/* We are done */
		Picker_SetSelectedPick(hwndList, s_nOriginalPick);

		KillTimer(NULL, s_nTestingTimer);
		s_nTestingTimer = 0;

		MessageBox(hMain, "Tests successfully completed!", MAME32NAME, MB_OK);
	}
	else
	{
		MessTestsFlex(GetDlgItem(hMain, IDC_SWLIST), drivers[Picker_GetSelectedItem(hwndList)]);
		Picker_SetSelectedPick(hwndList, nNewGame);
	}
	s_bInTimerProc = FALSE;
}



static void MessTestsBegin(void)
{
	int nOriginalPick;

	nOriginalPick = GetSelectedPick();

	// If we are not currently running tests, set up the timer and keep track
	// of the original selected pick item
	if (!s_nTestingTimer)
	{
		s_nTestingTimer = SetTimer(NULL, 0, 50, MessTestsTimerProc);
		if (!s_nTestingTimer)
			return;
		s_nOriginalPick = nOriginalPick;
	}

	MessTestsColumns();

	if (nOriginalPick == 0)
		Picker_SetSelectedPick(hwndList, 1);
	Picker_SetSelectedPick(hwndList, 0);
}



#endif // MAME_DEBUG
