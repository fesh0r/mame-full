/***************************************************************************

  layoutms.c

  MESS specific TreeView definitions (and maybe more in the future)

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>  /* for sprintf */
#include <stdlib.h> /* For malloc and free */
#include <string.h>

#include "windowsui/TreeView.h"
#include "windowsui/M32Util.h"
#include "windowsui/resource.h"
#include "windowsui/MessResource.h"
#include "windowsui/ms32util.h"

FOLDERDATA g_folderData[] =
{
	{"All Systems",     FOLDER_ALLGAMES,     IDI_FOLDER,				0,             0,            NULL,                       NULL,              TRUE },
	{"Available",       FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,            NULL,                       NULL,              TRUE },
#ifdef SHOW_UNAVAILABLE_FOLDER
	{"Unavailable",     FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,	0,             F_AVAILABLE,  NULL,                       NULL,              FALSE },
#endif
	{"Console",         FOLDER_CONSOLE,      IDI_FOLDER,				F_CONSOLE,     F_COMPUTER,   NULL,                       DriverIsComputer,  FALSE },
	{"Computer",        FOLDER_COMPUTER,     IDI_FOLDER,				F_COMPUTER,    F_CONSOLE,    NULL,                       DriverIsComputer,  TRUE },
	{"Modified/Hacked", FOLDER_MODIFIED,     IDI_FOLDER,				0,             0,            NULL,                       DriverIsModified,  TRUE },
	{"Manufacturer",    FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            CreateManufacturerFolders },
	{"Year",            FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            CreateYearFolders },
	{"Source",          FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            CreateSourceFolders },
	{"CPU",             FOLDER_CPU,          IDI_FOLDER,               0,             0,            CreateCPUFolders },
	{"SND",             FOLDER_SND,          IDI_FOLDER,               0,             0,            CreateSoundFolders },
	{"Working",         FOLDER_WORKING,      IDI_WORKING,				F_WORKING,     F_NONWORKING, NULL,                       DriverIsBroken,    FALSE },
	{"Non-Working",     FOLDER_NONWORKING,   IDI_NONWORKING,			F_NONWORKING,  F_WORKING,    NULL,                       DriverIsBroken,    TRUE },
	{"Originals",       FOLDER_ORIGINAL,     IDI_FOLDER,				F_ORIGINALS,   F_CLONES,     NULL,                       DriverIsClone,     FALSE },
	{"Clones",          FOLDER_CLONES,       IDI_FOLDER,				F_CLONES,      F_ORIGINALS,  NULL,                       DriverIsClone,     TRUE },
	{"Raster",          FOLDER_RASTER,       IDI_FOLDER,				F_RASTER,      F_VECTOR,     NULL,                       DriverIsVector,    FALSE },
	{"Vector",          FOLDER_VECTOR,       IDI_FOLDER,				F_VECTOR,      F_RASTER,     NULL,                       DriverIsVector,    TRUE },
	{"Trackball",       FOLDER_TRACKBALL,    IDI_FOLDER,				0,             0,            NULL,                       DriverUsesTrackball,	TRUE },
	{"Stereo",          FOLDER_STEREO,       IDI_SOUND,	                0,             0,            NULL,                       DriverIsStereo,    TRUE },
	{ NULL }
};

/* list of filter/control Id pairs */
FILTER_ITEM g_filterList[] =
{
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer, TRUE },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsComputer, FALSE },
	{ F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified, TRUE },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, TRUE },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, TRUE },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, GetHasRoms, FALSE },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, FALSE },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, TRUE },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, FALSE },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, FALSE },
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   GetHasRoms, TRUE },
	{ 0 }
};
