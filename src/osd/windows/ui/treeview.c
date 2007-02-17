/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  TreeView.c

  TreeView support routines - MSH 11/19/1998

***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
#ifndef NONAMELESSUNION
#define NONAMELESSUNION 
#endif
#endif


#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>  /* for sprintf */
#include <stdlib.h> /* For malloc and free */
#include <ctype.h> /* For tolower */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include <direct.h>
#endif

#include <io.h>
#include <driver.h>
#include "hash.h"
#include "M32Util.h"
#include "bitmask.h"
#include "screenshot.h"
#include "mame32.h"
#include "TreeView.h"
#include "resource.h"
#include "properties.h"
#include "options.h"
#include "help.h"
#include "dialogs.h"

#ifdef _MSC_VER
#if _MSC_VER > 1200
#define HAS_DUMMYUNIONNAME
#endif
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define MAX_EXTRA_FOLDERS 256

/***************************************************************************
    public structures
 ***************************************************************************/

#define ICON_MAX (sizeof(treeIconNames) / sizeof(treeIconNames[0]))

/* Name used for user-defined custom icons */
/* external *.ico file to look for. */

typedef struct
{
	int		nResourceID;
	LPCSTR	lpName;
} TREEICON;

static TREEICON treeIconNames[] =
{
	{ IDI_FOLDER_OPEN,         "foldopen" },
	{ IDI_FOLDER,              "folder" },
	{ IDI_FOLDER_AVAILABLE,    "foldavail" },
	{ IDI_FOLDER_MANUFACTURER, "foldmanu" },
	{ IDI_FOLDER_UNAVAILABLE,  "foldunav" },
	{ IDI_FOLDER_YEAR,         "foldyear" },
    { IDI_FOLDER_SOURCE,       "foldsrc" },
	{ IDI_MANUFACTURER,        "manufact" },
	{ IDI_WORKING,             "working" },
	{ IDI_NONWORKING,          "nonwork" },
	{ IDI_YEAR,                "year" },
	{ IDI_SOUND,               "sound" },
    { IDI_CPU,                 "cpu" },
	{ IDI_HARDDISK,            "harddisk" },
    { IDI_SOURCE,              "source" }
};

/***************************************************************************
    private variables
 ***************************************************************************/

/* this has an entry for every folder eventually in the UI, including subfolders */
static TREEFOLDER **treeFolders = 0;
static UINT         numFolders  = 0;        /* Number of folder in the folder array */
static UINT         next_folder_id = MAX_FOLDERS;
static UINT         folderArrayLength = 0;  /* Size of the folder array */
static LPTREEFOLDER lpCurrentFolder = 0;    /* Currently selected folder */
static UINT         nCurrentFolder = 0;     /* Current folder ID */
static WNDPROC      g_lpTreeWndProc = 0;    /* for subclassing the TreeView */
static HIMAGELIST   hTreeSmall = 0;         /* TreeView Image list of icons */

/* this only has an entry for each TOP LEVEL extra folder + SubFolders*/
LPEXFOLDERDATA ExtraFolderData[MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS];
int			          numExtraFolders = 0;
static int            numExtraIcons = 0;
static char           *ExtraFolderIcons[MAX_EXTRA_FOLDERS];

// built in folders and filters
static LPFOLDERDATA  g_lpFolderData;
static LPFILTER_ITEM g_lpFilterList;	

/***************************************************************************
    private function prototypes
 ***************************************************************************/

extern BOOL InitFolders(void);
static BOOL         CreateTreeIcons(void);
static void         TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static const char* ParseManufacturer(const char *s, int *pParsedChars );
static const char* TrimManufacturer(const char *s);
static void CreateAllChildFolders(void);
static BOOL         AddFolder(LPTREEFOLDER lpFolder);
static LPTREEFOLDER NewFolder(const char *lpTitle,
                              UINT nFolderId, int nParent, UINT nIconId,
                              DWORD dwFlags);
static void         DeleteFolder(LPTREEFOLDER lpFolder);

static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int InitExtraFolders(void);
static void FreeExtraFolders(void);
static void SetExtraIcons(char *name, int *id);
static BOOL TryAddExtraFolderAndChildren(int parent_index);

static BOOL TrySaveExtraFolder(LPTREEFOLDER lpFolder);

/***************************************************************************
    public functions
 ***************************************************************************/

/**************************************************************************
 *      ci_strncmp - case insensitive character array compare
 *
 *      Returns zero if the first n characters of s1 and s2 are equal,
 *      ignoring case.
 *		stolen from datafile.c
 **************************************************************************/
static int ci_strncmp (const char *s1, const char *s2, int n)
{
        int c1, c2;

        while (n)
        {
                if ((c1 = tolower (*s1)) != (c2 = tolower (*s2)))
                        return (c1 - c2);
                else if (!c1)
                        break;
                --n;

                s1++;
                s2++;
        }
        return 0;
}



/* De-allocate all folder memory */
void FreeFolders(void)
{
	int i = 0;

	if (treeFolders != NULL)
	{
		if (numExtraFolders)
		{
			FreeExtraFolders();
			numFolders -= numExtraFolders;
		}

		for (i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = NULL;
			numFolders--;
		}
		free(treeFolders);
		treeFolders = NULL;
	}
	numFolders = 0;
}

/* Reset folder filters */
void ResetFilters(void)
{
	int i = 0;

	if (treeFolders != 0)
	{
		for (i = 0; i < (int)numFolders; i++)
		{
			treeFolders[i]->m_dwFlags &= ~F_MASK;
		}
	}
}

void InitTree(LPFOLDERDATA lpFolderData, LPFILTER_ITEM lpFilterList)
{
	g_lpFolderData = lpFolderData;
	g_lpFilterList = lpFilterList;

	InitFolders();

	/* this will subclass the treeview (where WM_DRAWITEM gets sent for
	   the header control) */
	g_lpTreeWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetTreeView(), GWL_WNDPROC);
	SetWindowLong(GetTreeView(), GWL_WNDPROC, (LONG)TreeWndProc);
}

void DestroyTree(HWND hWnd)
{
    if ( hTreeSmall )
    {
        ImageList_Destroy( hTreeSmall );
        hTreeSmall = NULL;
    }
}

void SetCurrentFolder(LPTREEFOLDER lpFolder)
{
	lpCurrentFolder = (lpFolder == 0) ? treeFolders[0] : lpFolder;
	nCurrentFolder = (lpCurrentFolder) ? lpCurrentFolder->m_nFolderId : 0;
}

LPTREEFOLDER GetCurrentFolder(void)
{
	return lpCurrentFolder;
}

UINT GetCurrentFolderID(void)
{
	return nCurrentFolder;
}

int GetNumFolders(void)
{
	return numFolders;
}

LPTREEFOLDER GetFolder(UINT nFolder)
{
	return (nFolder < numFolders) ? treeFolders[nFolder] : NULL;
}

LPTREEFOLDER GetFolderByID(UINT nID)
{
	UINT i;

	for (i = 0; i < numFolders; i++)
	{
		if (treeFolders[i]->m_nFolderId == nID)
		{
			return treeFolders[i];
		}
	}

	return (LPTREEFOLDER)0;
}

void AddGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	SetBit(lpFolder->m_lpGameBits, nGame);
}

void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	ClearBit(lpFolder->m_lpGameBits, nGame);
}

int FindGame(LPTREEFOLDER lpFolder, int nGame)
{
	return FindBit(lpFolder->m_lpGameBits, nGame, TRUE);
}

// Called to re-associate games with folders
void ResetWhichGamesInFolders(void)
{
	UINT	i, jj, k;
	BOOL b;
	int nGames = GetNumGames();

	for (i = 0; i < numFolders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		// setup the games in our built-in folders
		for (k = 0; g_lpFolderData[k].m_lpTitle; k++)
		{
			if (lpFolder->m_nFolderId == g_lpFolderData[k].m_nFolderId)
			{
				if (g_lpFolderData[k].m_pfnQuery || g_lpFolderData[k].m_bExpectedResult)
				{
					SetAllBits(lpFolder->m_lpGameBits, FALSE);
					for (jj = 0; jj < nGames; jj++)
					{
						// invoke the query function
						b = g_lpFolderData[k].m_pfnQuery ? g_lpFolderData[k].m_pfnQuery(jj) : TRUE;

						// if we expect FALSE, flip the result
						if (!g_lpFolderData[k].m_bExpectedResult)
							b = !b;

						// if we like what we hear, add the game
						if (b)
							AddGame(lpFolder, jj);
					}
				}
				break;
			}
		}
	}
}


/* Used to build the GameList */
BOOL GameFiltered(int nGame, DWORD dwMask)
{
	int i;
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	LPTREEFOLDER lpParent = NULL;

	//Filter out the Bioses on all Folders, except for the Bios Folder
	if( lpFolder->m_nFolderId != FOLDER_BIOS )
	{
		if( !( (drivers[nGame]->flags & NOT_A_DRIVER ) == 0) )
			return TRUE;
	}
 	// Filter games--return TRUE if the game should be HIDDEN in this view
	if( GetFilterInherit() )
	{
		if( lpFolder )
		{
			lpParent = GetFolder( lpFolder->m_nParent );
			if( lpParent )
			{
				/* Check the Parent Filters and inherit them on child,
				 * The inherited filters don't display on the custom Filter Dialog for the Child folder
				 * No need to promote all games to parent folder, works as is */
				dwMask |= lpParent->m_dwFlags;
			}
		}
	}
	/*Filter Text is already global*/

	if (MyStrStrI(drivers[nGame]->description,GetFilterText()) == NULL &&
		MyStrStrI(drivers[nGame]->name,GetFilterText()) == NULL && 
		MyStrStrI(drivers[nGame]->source_file,GetFilterText()) == NULL && 
		MyStrStrI(drivers[nGame]->manufacturer,GetFilterText()) == NULL)
	{
		return TRUE;
	}
	// Are there filters set on this folder?
	if ((dwMask & F_MASK) == 0)
		return FALSE;

	// Filter out clones?
	if (dwMask & F_CLONES && DriverIsClone(nGame))
		return TRUE;

	for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
	{
		if (dwMask & g_lpFilterList[i].m_dwFilterType)
		{
			if (g_lpFilterList[i].m_pfnQuery(nGame) == g_lpFilterList[i].m_bExpectedResult)
				return TRUE;
		}
	}
	return FALSE;
}

/* Get the parent of game in this view */
BOOL GetParentFound(int nGame)
{
	int nParentIndex = -1;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if( lpFolder )
	{
		nParentIndex = GetParentIndex(drivers[nGame]);

		/* return FALSE if no parent is there in this view */
		if( nParentIndex == -1)
			return FALSE;

		/* return FALSE if the folder should be HIDDEN in this view */
		if (TestBit(lpFolder->m_lpGameBits, nParentIndex) == 0)
			return FALSE;

		/* return FALSE if the game should be HIDDEN in this view */
		if (GameFiltered(nParentIndex, lpFolder->m_dwFlags))
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

LPFILTER_ITEM GetFilterList(void)
{
	return g_lpFilterList;
}

/***************************************************************************
	private functions
 ***************************************************************************/

void CreateSourceFolders(int parent_index)
{
	int i,jj, k=0;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	int *redirect_arr = GetFolderOptionsRedirectArr();
	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);
	if( redirect_arr == NULL)
	{
		redirect_arr = (int*)malloc(sizeof(int*) );
		memset(redirect_arr, '\0', sizeof (int*));
	}	
	//We also allow setting of options for the Vector Folder
	redirect_arr = realloc(redirect_arr, ((k+1)* sizeof(int)) );
	redirect_arr[k++] = FOLDER_VECTOR;
	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverFilename(jj);
                			
		if (s == NULL || s[0] == '\0')
			continue;

		// look for an existant source treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle,s) == 0)
			{
				AddGame(treeFolders[i],jj);
				break;
			}
		}
		if (i == start_folder-1)
		{
			// nope, it's a source file we haven't seen before, make it.
			LPTREEFOLDER lpTemp;
			lpTemp = NewFolder(s, next_folder_id, parent_index, IDI_SOURCE,
 							   GetFolderFlags(numFolders));
			ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
			memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

			ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
			ExtraFolderData[next_folder_id]->m_nIconId = IDI_SOURCE;
			ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
			ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
			strcpy( ExtraFolderData[next_folder_id]->m_szTitle, s );
			//Add it to the FolderOptionsRedirectArray
			redirect_arr = realloc(redirect_arr, ((k+1)* sizeof(int)) );
			redirect_arr[k++] = next_folder_id;
			ExtraFolderData[next_folder_id++]->m_dwFlags = 0;

			AddFolder(lpTemp);
			AddGame(lpTemp,jj);
		}
	}
	SetNumOptionFolders(k-1);
	SetFolderOptionsRedirectArr(redirect_arr);
}

void CreateManufacturerFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
 	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *manufacturer = drivers[jj]->manufacturer;
		int iChars = 0;
		while( manufacturer != NULL && manufacturer[0] != '\0' )
		{
			const char *s = ParseManufacturer(manufacturer, &iChars);
			manufacturer += iChars;
			//shift to next start char
			if( s != NULL && strlen(s) > 0 )
 			{
				const char *t = TrimManufacturer(s);
				for (i=numFolders-1;i>=start_folder;i--)
				{
					//RS Made it case insensitive
					if (ci_strncmp(treeFolders[i]->m_lpTitle,t,20) == 0 )
					{
						AddGame(treeFolders[i],jj);
						break;
					}
				}
				if (i == start_folder-1)
				{
					// nope, it's a manufacturer we haven't seen before, make it.
					lpTemp = NewFolder(t, next_folder_id, parent_index, IDI_MANUFACTURER,
									   GetFolderFlags(numFolders));
					ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
					memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

					ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
					ExtraFolderData[next_folder_id]->m_nIconId = IDI_MANUFACTURER;
					ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
					ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
					strcpy( ExtraFolderData[next_folder_id]->m_szTitle, s );
					ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
					AddFolder(lpTemp);
					AddGame(lpTemp,jj);
				}
			}
		}
	}
}

/* Make a reasonable name out of the one found in the driver array */
static const char* ParseManufacturer(const char *s, int *pParsedChars )
{
	static char tmp[256];
    char* ptmp;
	char *t;
	*pParsedChars= 0;

	if ( *s == '?' || *s == '<' || s[3] == '?' )
    {
		(*pParsedChars) = strlen(s);
		return "<unknown>";
    }

    ptmp = tmp;
	/*if first char is a space, skip it*/
	if( *s == ' ' )
	{
		(*pParsedChars)++;
        ++s;
	}
	while( *s )
	{
        /* combinations where to end string */
		
		if ( 
            ( (*s == ' ') && ( s[1] == '(' || s[1] == '/' || s[1] == '+' ) ) ||
            ( *s == ']' ) ||
            ( *s == '/' ) ||
            ( *s == '?' )
            )
        {
		(*pParsedChars)++;
			if( s[1] == '/' || s[1] == '+' )
				(*pParsedChars)++;
			break;
        }
		if( s[0] == ' ' && s[1] == '?' )
		{
			(*pParsedChars) += 2;
			s+=2;
		}

        /* skip over opening braces*/

		if ( *s != '[' )
        {
			*ptmp++ = *s;
	    }
		(*pParsedChars)++;
		/*for "distributed by" and "supported by" handling*/
		if( ( (s[1] == ',') && (s[2] == ' ') && ( (s[3] == 's') || (s[3] == 'd') ) ) )
		{
			//*ptmp++ = *s;
			++s;
			break;
	}
        ++s;
	}
	*ptmp = '\0';
	t = tmp;
	if( tmp[0] == '(' || tmp[strlen(tmp)-1] == ')' || tmp[0] == ',')
	{
		ptmp = strchr( tmp,'(' );
		if ( ptmp == NULL )
		{
			ptmp = strchr( tmp,',' );
			if( ptmp != NULL)
			{
				//parse the new "supported by" and "distributed by"
				ptmp++;

				if (ci_strncmp(ptmp, " supported by", 13) == 0)
				{
					ptmp += 13;
				}
				else if (ci_strncmp(ptmp, " distributed by", 15) == 0)
				{
					ptmp += 15;
				}
				else
				{
					return NULL;
				}
			}
			else
			{
				ptmp = tmp;
				if ( ptmp == NULL )
				{
					return NULL;
				}
			}
		}
		if( tmp[0] == '(' || tmp[0] == ',')
		{
			ptmp++;
		}
		if (ci_strncmp(ptmp, "licensed from ", 14) == 0)
		{
			ptmp += 14;
		}
		// for the licenced from case
		if (ci_strncmp(ptmp, "licenced from ", 14) == 0)
		{
			ptmp += 14;
		}
		
		while ( (*ptmp != ')' ) && (*ptmp != '/' ) && *ptmp != '\0')
		{
			if (*ptmp == ' ' && ci_strncmp(ptmp, " license", 8) == 0)
			{
				break;
			}
			if (*ptmp == ' ' && ci_strncmp(ptmp, " licence", 8) == 0)
			{
				break;
			}
			*t++ = *ptmp++;
		}
		
		*t = '\0';
	}

	*ptmp = '\0';
	return tmp;
}

/* Analyze Manufacturer Names for typical patterns, that don't distinguish between companies (e.g. Co., Ltd., Inc., etc. */
static const char* TrimManufacturer(const char *s)
{
	//Also remove Country specific suffixes (e.g. Japan, Italy, America, USA, ...)
	int i=0;
	char strTemp[256];
	static char strTemp2[256];
	int j=0;
	int k=0;
	int l = 0;
	memset(strTemp, '\0', 256 );
	memset(strTemp2, '\0', 256 );
	//start analyzing from the back, as these are usually suffixes
	for(i=strlen(s)-1; i>=0;i-- )
	{
		
		l = strlen(strTemp);
		for(k=l;k>=0; k--)
			strTemp[k+1] = strTemp[k];
		strTemp[0] = s[i];
		strTemp[++l] = '\0';
		switch (l)
		{
			case 2:
				if( ci_strncmp(strTemp, "co", 2) == 0 )
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 3:
				if( ci_strncmp(strTemp, "co.", 3) == 0 || ci_strncmp(strTemp, "ltd", 3) == 0 || ci_strncmp(strTemp, "inc", 3) == 0  || ci_strncmp(strTemp, "SRL", 3) == 0 || ci_strncmp(strTemp, "USA", 3) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 4:
				if( ci_strncmp(strTemp, "inc.", 4) == 0 || ci_strncmp(strTemp, "ltd.", 4) == 0 || ci_strncmp(strTemp, "corp", 4) == 0 || ci_strncmp(strTemp, "game", 4) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 5:
				if( ci_strncmp(strTemp, "corp.", 5) == 0 || ci_strncmp(strTemp, "Games", 5) == 0 || ci_strncmp(strTemp, "Italy", 5) == 0 || ci_strncmp(strTemp, "Japan", 5) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 6:
				if( ci_strncmp(strTemp, "co-ltd", 6) == 0 || ci_strncmp(strTemp, "S.R.L.", 6) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 7:
				if( ci_strncmp(strTemp, "co. ltd", 7) == 0  || ci_strncmp(strTemp, "America", 7) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 8:
				if( ci_strncmp(strTemp, "co. ltd.", 8) == 0  )
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 9:
				if( ci_strncmp(strTemp, "co., ltd.", 9) == 0 || ci_strncmp(strTemp, "gmbh & co", 9) == 0 )
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 10:
				if( ci_strncmp(strTemp, "corp, ltd.", 10) == 0  || ci_strncmp(strTemp, "industries", 10) == 0  || ci_strncmp(strTemp, "of America", 10) == 0)
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 11:
				if( ci_strncmp(strTemp, "corporation", 11) == 0 || ci_strncmp(strTemp, "enterprises", 11) == 0 )
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			case 16:
				if( ci_strncmp(strTemp, "industries japan", 16) == 0 )
				{
					j=l;
					while( s[strlen(s)-j-1] == ' ' || s[strlen(s)-j-1] == ',' )
					{
						j++;
					}
					if( j!=l)
					{
						memset(strTemp2, '\0', 256 );
						strncpy(strTemp2, s, strlen(s)-j );	
					}
				}
				break;
			default:
				break;
		}
	}
	if( strlen(strTemp2) == 0 )
		return s;
	return strTemp2;
}



void CreateCPUFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int nFolder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER map[CPU_COUNT];

	ZeroMemory(map, sizeof(map));
	cpuintrf_init(NULL);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (i=1;i<CPU_COUNT;i++)
	{
		LPTREEFOLDER lpTemp;

		for (jj = 1; jj < i; jj++)
			if (!strcmp(cputype_name(i), cputype_name(jj)))
				break;

		if (i != jj)
		{
			map[i] = map[jj];
			continue;
		}
		if( strlen( cputype_name(i) ) <=0 )
			continue;
		lpTemp = NewFolder(cputype_name(i), next_folder_id, parent_index, IDI_CPU,
 						   GetFolderFlags(numFolders));
		ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
		memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

		ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
		ExtraFolderData[next_folder_id]->m_nIconId = IDI_CPU;
		ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
		ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
		strcpy( ExtraFolderData[next_folder_id]->m_szTitle, cputype_name(i) );
		ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
		AddFolder(lpTemp);
		map[i] = treeFolders[nFolder++];
	}

	for (jj = 0; jj < nGames; jj++)
	{
		int n;
		machine_config drv;
		expand_machine_driver(drivers[jj]->drv,&drv);
		for (n = 0; n < MAX_CPU; n++)
			if (drv.cpu[n].cpu_type != CPU_DUMMY)
			{
				// cpu type #'s are one-based
				AddGame(map[drv.cpu[n].cpu_type],jj);
			}
	}
}

void CreateSoundFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int nFolder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER map[SOUND_COUNT];

	sndintrf_init(NULL);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (i=1;i<SOUND_COUNT;i++)
	{
		LPTREEFOLDER lpTemp;

		// empty fields get filled in with SOUND_DUMMY, so check for this and
		// don't add it
		if (strcmp(sndtype_name(SOUND_DUMMY),sndtype_name(i)) == 0)
			continue;

		//dprintf("%i %s\n",i,sndtype_name(i));
		lpTemp = NewFolder(sndtype_name(i), next_folder_id, parent_index, IDI_SOUND,
 						   GetFolderFlags(numFolders));
		ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
		memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

		ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
		ExtraFolderData[next_folder_id]->m_nIconId = IDI_SOUND;
		ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
		ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
		strcpy( ExtraFolderData[next_folder_id]->m_szTitle, sndtype_name(i) );
		ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
		AddFolder(lpTemp);
		map[i] = treeFolders[nFolder++];
	}

	for (jj = 0; jj < nGames; jj++)
	{
		int n;
		machine_config drv;
		expand_machine_driver(drivers[jj]->drv,&drv);
		
		for (n = 0; n < MAX_SOUND; n++)
			if (drv.sound[n].sound_type != SOUND_DUMMY)
			{
				// sound type #'s are one-based, though that doesn't affect us here
				AddGame(map[drv.sound[n].sound_type],jj);
			}
	}

}

void CreateOrientationFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// create our two subfolders
	LPTREEFOLDER lpVert, lpHorz;
	lpVert = NewFolder("Vertical", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Vertical" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpVert);
	lpHorz = NewFolder("Horizontal", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Horizontal" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpHorz);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		if (drivers[jj]->flags & ORIENTATION_SWAP_XY)
		{
			AddGame(lpVert,jj);
		}
		else
		{
			AddGame(lpHorz,jj);
		}
	}
}

void CreateDeficiencyFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// create our two subfolders
	LPTREEFOLDER lpProt, lpWrongCol, lpImpCol, lpImpGraph, lpMissSnd, lpImpSnd, lpFlip;
	lpProt = NewFolder("Unemulated Protection", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Unemulated Protection" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpProt);
	lpWrongCol = NewFolder("Wrong Colors", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Wrong Colors" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpWrongCol);

	lpImpCol = NewFolder("Imperfect Colors", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Imperfect Colors" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpImpCol);

	lpImpGraph = NewFolder("Imperfect Graphics", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Imperfect Graphics" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpImpGraph);

	lpMissSnd = NewFolder("Missing Sound", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Missing Sound" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpMissSnd);

	lpImpSnd = NewFolder("Imperfect Sound", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Imperfect Sound" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpImpSnd);

	lpFlip = NewFolder("No Cocktail", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "No Cocktail" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpFlip);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		if (drivers[jj]->flags & GAME_WRONG_COLORS)
		{
			AddGame(lpWrongCol,jj);
		}
		if (drivers[jj]->flags & GAME_UNEMULATED_PROTECTION)
		{
			AddGame(lpProt,jj);
		}
		if (drivers[jj]->flags & GAME_IMPERFECT_COLORS)
		{
			AddGame(lpImpCol,jj);
		}
		if (drivers[jj]->flags & GAME_IMPERFECT_GRAPHICS)
		{
			AddGame(lpImpGraph,jj);
		}
		if (drivers[jj]->flags & GAME_NO_SOUND)
		{
			AddGame(lpMissSnd,jj);
		}
		if (drivers[jj]->flags & GAME_IMPERFECT_SOUND)
		{
			AddGame(lpImpSnd,jj);
		}
		if (drivers[jj]->flags & GAME_NO_COCKTAIL)
		{
			AddGame(lpFlip,jj);
		}
	}
}

void CreateDumpingFolders(int parent_index)
{
	int jj;
	BOOL bBadDump  = FALSE;
	BOOL bNoDump = FALSE;
	int nGames = GetNumGames();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	const rom_entry *region, *rom;
	const char *name;
	const game_driver *gamedrv;

	// create our two subfolders
	LPTREEFOLDER lpBad, lpNo;
	lpBad = NewFolder("Bad Dump", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "Bad Dump" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpBad);
	lpNo = NewFolder("No Dump", next_folder_id, parent_index, IDI_FOLDER,
 					   GetFolderFlags(numFolders));
	ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
	memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

	ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
	ExtraFolderData[next_folder_id]->m_nIconId = IDI_FOLDER;
	ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
	ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
	strcpy( ExtraFolderData[next_folder_id]->m_szTitle, "No Dump" );
	ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
	AddFolder(lpNo);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		gamedrv = drivers[jj];

		if (!gamedrv->rom) 
			continue;
		bBadDump = FALSE;
		bNoDump = FALSE;
		for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		{
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				if (ROMREGION_ISROMDATA(region) || ROMREGION_ISDISKDATA(region) )
				{
					name = ROM_GETNAME(rom);
					if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))				
						bBadDump = TRUE;
					if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))				
						bNoDump = TRUE;
				}
			}
		}
		if (bBadDump)
		{
			AddGame(lpBad,jj);
		}
		if (bNoDump)
		{
			AddGame(lpNo,jj);
		}
	}
}


void CreateYearFolders(int parent_index)
{
	int i,jj;
	int nGames = GetNumGames();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		char s[100];
		strcpy(s,drivers[jj]->year);

		if (s[0] == '\0')
			continue;

		if (s[4] == '?')
			s[4] = '\0';
		
		// look for an extant year treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (strncmp(treeFolders[i]->m_lpTitle,s,4) == 0)
			{
				AddGame(treeFolders[i],jj);
				break;
			}
		}
		if (i == start_folder-1)
		{
			// nope, it's a year we haven't seen before, make it.
			LPTREEFOLDER lpTemp;
			lpTemp = NewFolder(s, next_folder_id, parent_index, IDI_YEAR,
 							   GetFolderFlags(numFolders));
			ExtraFolderData[next_folder_id] = malloc(sizeof(EXFOLDERDATA) );
			memset(ExtraFolderData[next_folder_id], 0, sizeof(EXFOLDERDATA));

			ExtraFolderData[next_folder_id]->m_nFolderId = next_folder_id;
			ExtraFolderData[next_folder_id]->m_nIconId = IDI_YEAR;
			ExtraFolderData[next_folder_id]->m_nParent = lpFolder->m_nFolderId;
			ExtraFolderData[next_folder_id]->m_nSubIconId = -1;
			strcpy( ExtraFolderData[next_folder_id]->m_szTitle, s );
			ExtraFolderData[next_folder_id++]->m_dwFlags = 0;
			AddFolder(lpTemp);
			AddGame(lpTemp,jj);
		}
	}
}

// creates child folders of all the top level folders, including custom ones
void CreateAllChildFolders(void)
{
	int num_top_level_folders = numFolders;
	int i, j;

	for (i = 0; i < num_top_level_folders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		LPFOLDERDATA lpFolderData = NULL;

		for (j = 0; g_lpFolderData[j].m_lpTitle; j++)
		{
			if (g_lpFolderData[j].m_nFolderId == lpFolder->m_nFolderId)
			{
				lpFolderData = &g_lpFolderData[j];
				break;
			}
		}

		if (lpFolderData != NULL)
		{
			//dprintf("Found built-in-folder id %i %i",i,lpFolder->m_nFolderId);
			if (lpFolderData->m_pfnCreateFolders != NULL)
				lpFolderData->m_pfnCreateFolders(i);
		}
		else
		{
			if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
			{
				dprintf("Internal inconsistency with non-built-in folder, but not custom");
				continue;
			}

			//dprintf("Loading custom folder %i %i",i,lpFolder->m_nFolderId);

			// load the extra folder files, which also adds children
			if (TryAddExtraFolderAndChildren(i) == FALSE)
			{
				lpFolder->m_nFolderId = FOLDER_NONE;
			}
		}
	}
}

// adds these folders to the treeview
void ResetTreeViewFolders(void)
{
	HWND hTreeView = GetTreeView();
	int i;
	TVITEM tvi;
	TVINSERTSTRUCT	tvs;

	HTREEITEM shti; // for current child branches

	// currently "cached" parent
	HTREEITEM hti_parent = NULL;
	int index_parent = -1;			

	TreeView_DeleteAllItems(hTreeView);

	//dprintf("Adding folders to tree ui indices %i to %i",start_index,end_index);

	tvs.hInsertAfter = TVI_SORT;

	for (i=0;i<numFolders;i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		if (lpFolder->m_nParent == -1)
		{
			if (lpFolder->m_nFolderId < MAX_FOLDERS)
			{
				// it's a built in folder, let's see if we should show it
				if (GetShowFolder(lpFolder->m_nFolderId) == FALSE)
				{
					continue;
				}
			}

			tvi.mask	= TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvs.hParent = TVI_ROOT;
			tvi.pszText = lpFolder->m_lpTitle;
			tvi.lParam	= (LPARAM)lpFolder;
			tvi.iImage	= GetTreeViewIconIndex(lpFolder->m_nIconId);
			tvi.iSelectedImage = 0;

#if defined(__GNUC__) /* bug in commctrl.h */
			tvs.item = tvi;
#else
			tvs.DUMMYUNIONNAME.item = tvi;
#endif

			// Add root branch
			hti_parent = TreeView_InsertItem(hTreeView, &tvs);

			continue;
		}

		// not a top level branch, so look for parent
		if (treeFolders[i]->m_nParent != index_parent)
		{
			
			hti_parent = TreeView_GetRoot(hTreeView);
			while (1)
			{
				if (hti_parent == NULL)
				{
					// couldn't find parent folder, so it's a built-in but
					// not shown folder
					break;
				}

				tvi.hItem = hti_parent;
				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(hTreeView,&tvi);
				if (((LPTREEFOLDER)tvi.lParam) == treeFolders[treeFolders[i]->m_nParent])
					break;

				hti_parent = TreeView_GetNextSibling(hTreeView,hti_parent);
			}

			// if parent is not shown, then don't show the child either obviously!
			if (hti_parent == NULL)
				continue;

			index_parent = treeFolders[i]->m_nParent;
		}

		tvi.mask	= TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvs.hParent = hti_parent;
		tvi.iImage	= GetTreeViewIconIndex(treeFolders[i]->m_nIconId);
		tvi.iSelectedImage = 0;
		tvi.pszText = treeFolders[i]->m_lpTitle;
		tvi.lParam	= (LPARAM)treeFolders[i];
		
#if defined(__GNUC__) /* bug in commctrl.h */
		tvs.item = tvi;
#else
		tvs.DUMMYUNIONNAME.item = tvi;
#endif
		// Add it to this tree branch
		shti = TreeView_InsertItem(hTreeView, &tvs);

	}
}

void SelectTreeViewFolder(int folder_id)
{
	HWND hTreeView = GetTreeView();
	HTREEITEM hti;
	TVITEM tvi;

	memset(&tvi,0,sizeof(tvi));

	hti = TreeView_GetRoot(hTreeView);

	while (hti != NULL)
	{
		HTREEITEM hti_next;

		tvi.hItem = hti;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);

		if (((LPTREEFOLDER)tvi.lParam)->m_nFolderId == folder_id)
		{
			TreeView_SelectItem(hTreeView,tvi.hItem);
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			return;
		}
		
		hti_next = TreeView_GetChild(hTreeView,hti);
		if (hti_next == NULL)
		{
			hti_next = TreeView_GetNextSibling(hTreeView,hti);
			if (hti_next == NULL)
			{
				hti_next = TreeView_GetParent(hTreeView,hti);
				if (hti_next != NULL)
					hti_next = TreeView_GetNextSibling(hTreeView,hti_next);
			}
		}
		hti = hti_next;
	}

	// could not find folder to select
	// make sure we select something
	tvi.hItem = TreeView_GetRoot(hTreeView);
	tvi.mask = TVIF_PARAM;
	TreeView_GetItem(hTreeView,&tvi);

	TreeView_SelectItem(hTreeView,tvi.hItem);
	SetCurrentFolder((LPTREEFOLDER)tvi.lParam);

}

/* Add a folder to the list.  Does not allocate */
static BOOL AddFolder(LPTREEFOLDER lpFolder)
{
	if (numFolders + 1 >= folderArrayLength)
	{
		folderArrayLength += 500;
		treeFolders = realloc(treeFolders,sizeof(TREEFOLDER **) * folderArrayLength);
	}

	treeFolders[numFolders] = lpFolder;
	numFolders++;

	return TRUE;
}

/* Allocate and initialize a NEW TREEFOLDER */
static LPTREEFOLDER NewFolder(const char *lpTitle,
					   UINT nFolderId, int nParent, UINT nIconId, DWORD dwFlags)
{
	LPTREEFOLDER lpFolder = (LPTREEFOLDER)malloc(sizeof(TREEFOLDER));

	memset(lpFolder, '\0', sizeof (TREEFOLDER));
	lpFolder->m_lpTitle = (LPSTR)malloc(strlen(lpTitle) + 1);
	strcpy((char *)lpFolder->m_lpTitle,lpTitle);
	lpFolder->m_lpGameBits = NewBits(GetNumGames());
	lpFolder->m_nFolderId = nFolderId;
	lpFolder->m_nParent = nParent;
	lpFolder->m_nIconId = nIconId;
	lpFolder->m_dwFlags = dwFlags;

	return lpFolder;
}

/* Deallocate the passed in LPTREEFOLDER */
static void DeleteFolder(LPTREEFOLDER lpFolder)
{
	if (lpFolder)
	{
		if (lpFolder->m_lpGameBits)
		{
			DeleteBits(lpFolder->m_lpGameBits);
			lpFolder->m_lpGameBits = 0;
		}
		free(lpFolder->m_lpTitle);
		lpFolder->m_lpTitle = 0;
		free(lpFolder);
		lpFolder = 0;
	}
}

/* Can be called to re-initialize the array of treeFolders */
BOOL InitFolders(void)
{
	int 			i = 0;
	DWORD			dwFolderFlags;
	LPFOLDERDATA	fData = 0;

	if (treeFolders != NULL)
	{
		for (i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = 0;
			numFolders--;
		}
	}
	numFolders = 0;
	if (folderArrayLength == 0)
	{
		folderArrayLength = 200;
		treeFolders = (TREEFOLDER **)malloc(sizeof(TREEFOLDER **) * folderArrayLength);
		if (!treeFolders)
		{
			folderArrayLength = 0;
			return 0;
		}
		else
		{
			memset(treeFolders,'\0', sizeof(TREEFOLDER **) * folderArrayLength);
		}
	}
	// built-in top level folders
	for (i = 0; g_lpFolderData[i].m_lpTitle; i++)
	{
		fData = &g_lpFolderData[i];
		/* get the saved folder flags */
		dwFolderFlags = GetFolderFlags(numFolders);
		/* create the folder */
		AddFolder(NewFolder(fData->m_lpTitle, fData->m_nFolderId, -1,
							fData->m_nIconId, dwFolderFlags));
	}
	
	numExtraFolders = InitExtraFolders();

	for (i = 0; i < numExtraFolders; i++)
	{
		LPEXFOLDERDATA  fExData = ExtraFolderData[i];

		// OR in the saved folder flags
		dwFolderFlags = fExData->m_dwFlags | GetFolderFlags(numFolders);
		// create the folder
		//dprintf("creating top level custom folder with icon %i",fExData->m_nIconId);
		AddFolder(NewFolder(fExData->m_szTitle,fExData->m_nFolderId,fExData->m_nParent,
							fExData->m_nIconId,dwFolderFlags));
	}

	CreateAllChildFolders();

	CreateTreeIcons();

	ResetWhichGamesInFolders();

	ResetTreeViewFolders();

	SelectTreeViewFolder(GetSavedFolderID());

	return TRUE;
}

// create iconlist and Treeview control
static BOOL CreateTreeIcons()
{
	HICON	hIcon;
	INT 	i;
	HINSTANCE hInst = GetModuleHandle(0);

	int numIcons = ICON_MAX + numExtraIcons;

	hTreeSmall = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, numIcons, numIcons);

	//dprintf("Trying to load %i normal icons",ICON_MAX);
	for (i = 0; i < ICON_MAX; i++)
	{
		hIcon = LoadIconFromFile(treeIconNames[i].lpName);
		if (!hIcon)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(treeIconNames[i].nResourceID));

		if (ImageList_AddIcon (hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on regular folder, %i %i",i,hIcon != NULL);
			return FALSE;
		}
	}

	//dprintf("Trying to load %i extra custom-folder icons",numExtraIcons);
	for (i = 0; i < numExtraIcons; i++)
	{
		if ((hIcon = LoadIconFromFile(ExtraFolderIcons[i])) == 0)
			hIcon = LoadIcon (hInst, MAKEINTRESOURCE(IDI_FOLDER));

		if (ImageList_AddIcon(hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on extra folder, %i %i",i,hIcon != NULL);
			return FALSE;
		}
	}

	// Be sure that all the small icons were added.
	if (ImageList_GetImageCount(hTreeSmall) < numIcons)
	{
		ErrorMsg("Error with icon list--too few images.  %i %i",
				ImageList_GetImageCount(hTreeSmall),numIcons);
		return FALSE;
	}

	// Be sure that all the small icons were added.

	if (ImageList_GetImageCount (hTreeSmall) < ICON_MAX)
	{
		ErrorMsg("Error with icon list--too few images.  %i < %i",
				 ImageList_GetImageCount(hTreeSmall),ICON_MAX);
		return FALSE;
	}

	// Associate the image lists with the list view control.
	TreeView_SetImageList(GetTreeView(), hTreeSmall, TVSIL_NORMAL);

	return TRUE;
}


static void TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC 		hDC;
	RECT		rcClip, rcClient;
	HDC 		memDC;
	HBITMAP 	bitmap;
	HBITMAP 	hOldBitmap;

	HBITMAP hBackground = GetBackgroundBitmap();
	MYBITMAPINFO *bmDesc = GetBackgroundInfo();

	hDC = BeginPaint(hWnd, &ps);

	GetClipBox(hDC, &rcClip);
	GetClientRect(hWnd, &rcClient);
	
	// Create a compatible memory DC
	memDC = CreateCompatibleDC(hDC);

	// Select a compatible bitmap into the memory DC
	bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
									rcClient.bottom - rcClient.top);
	hOldBitmap = SelectObject(memDC, bitmap);
	
	// First let the control do its default drawing.
	CallWindowProc(g_lpTreeWndProc, hWnd, uMsg, (WPARAM)memDC, 0);

	// Draw bitmap in the background

	{
		HPALETTE hPAL;		 
		HDC maskDC;
		HBITMAP maskBitmap;
		HDC tempDC;
		HDC imageDC;
		HBITMAP bmpImage;
		HBITMAP hOldBmpImage;
		HBITMAP hOldMaskBitmap;
		HBITMAP hOldHBitmap;
		int i, j;
		RECT rcRoot;

		// Now create a mask
		maskDC = CreateCompatibleDC(hDC);	
		
		// Create monochrome bitmap for the mask
		maskBitmap = CreateBitmap(rcClient.right - rcClient.left,
								  rcClient.bottom - rcClient.top, 
								  1, 1, NULL);

		hOldMaskBitmap = SelectObject(maskDC, maskBitmap);
		SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

		// Create the mask from the memory DC
		BitBlt(maskDC, 0, 0, rcClient.right - rcClient.left,
			   rcClient.bottom - rcClient.top, memDC, 
			   rcClient.left, rcClient.top, SRCCOPY);

		tempDC = CreateCompatibleDC(hDC);
		hOldHBitmap = SelectObject(tempDC, hBackground);

		imageDC = CreateCompatibleDC(hDC);
		bmpImage = CreateCompatibleBitmap(hDC,
										  rcClient.right - rcClient.left, 
										  rcClient.bottom - rcClient.top);
		hOldBmpImage = SelectObject(imageDC, bmpImage);

		hPAL = GetBackgroundPalette();
		if (hPAL == NULL)
			hPAL = CreateHalftonePalette(hDC);

		if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(hDC, hPAL, FALSE);
			RealizePalette(hDC);
			SelectPalette(imageDC, hPAL, FALSE);
		}
		
		// Get x and y offset
		TreeView_GetItemRect(hWnd, TreeView_GetRoot(hWnd), &rcRoot, FALSE);
		rcRoot.left = -GetScrollPos(hWnd, SB_HORZ);

		// Draw bitmap in tiled manner to imageDC
		for (i = rcRoot.left; i < rcClient.right; i += bmDesc->bmWidth)
			for (j = rcRoot.top; j < rcClient.bottom; j += bmDesc->bmHeight)
				BitBlt(imageDC,  i, j, bmDesc->bmWidth, bmDesc->bmHeight, 
					   tempDC, 0, 0, SRCCOPY);

		// Set the background in memDC to black. Using SRCPAINT with black and any other
		// color results in the other color, thus making black the transparent color
		SetBkColor(memDC, RGB(0,0,0));
		SetTextColor(memDC, RGB(255,255,255));
		BitBlt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Set the foreground to black. See comment above.
		SetBkColor(imageDC, RGB(255,255,255));
		SetTextColor(imageDC, RGB(0,0,0));
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left, 
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Combine the foreground with the background
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top, 
			   memDC, rcClip.left, rcClip.top, SRCPAINT);

		// Draw the final image to the screen
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top, 
			   imageDC, rcClip.left, rcClip.top, SRCCOPY);
		
		SelectObject(maskDC, hOldMaskBitmap);
		SelectObject(tempDC, hOldHBitmap);
		SelectObject(imageDC, hOldBmpImage);

		DeleteDC(maskDC);
		DeleteDC(imageDC);
		DeleteDC(tempDC);
		DeleteObject(bmpImage);
		DeleteObject(maskBitmap);

		if (GetBackgroundPalette() == NULL)
		{
			DeleteObject(hPAL);
			hPAL = 0;
		}
	}

	SelectObject(memDC, hOldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	EndPaint(hWnd, &ps);
	ReleaseDC(hWnd, hDC);
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (GetBackgroundBitmap() != NULL)
	{
		switch (uMsg)
		{	
	    case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}

		case WM_KEYDOWN :
			if (wParam == VK_F2)
			{
				if (lpCurrentFolder->m_dwFlags & F_CUSTOM)
				{
					TreeView_EditLabel(hWnd,TreeView_GetSelection(hWnd));
					return TRUE;
				}
			}
			break;

		case WM_ERASEBKGND:
			return TRUE;
			break;

		case WM_PAINT:
			TreeCtrlOnPaint(hWnd, uMsg, wParam, lParam);
			UpdateWindow(hWnd);
			break;
		}
	}

	/* message not handled */
	return CallWindowProc(g_lpTreeWndProc, hWnd, uMsg, wParam, lParam);
}

/*
 * Filter code - should be moved to filter.c/filter.h
 * Added 01/09/99 - MSH <mhaaland@hypertech.com>
 */

/* find a FOLDERDATA by folderID */
LPFOLDERDATA FindFilter(DWORD folderID)
{
	int i;

	for (i = 0; g_lpFolderData[i].m_lpTitle; i++)
	{
		if (g_lpFolderData[i].m_nFolderId == folderID)
			return &g_lpFolderData[i];
	}
	return (LPFOLDERDATA) 0;
}

/***************************************************************************
    private structures
 ***************************************************************************/

/***************************************************************************
    private functions prototypes
 ***************************************************************************/

/***************************************************************************
    private functions
 ***************************************************************************/

/**************************************************************************/

LPTREEFOLDER GetFolderByName(int nParentId, const char *pszFolderName)
{
	int i = 0, nParent;

	//First Get the Parent TreeviewItem
	//Enumerate Children
	for(i = 0; i < numFolders/* ||treeFolders[i] != NULL*/; i++)
	{
		if (!strcmp(treeFolders[i]->m_lpTitle, pszFolderName))
		{
			nParent = treeFolders[i]->m_nParent;
			if ((nParent >= 0) && treeFolders[nParent]->m_nFolderId == nParentId)
				return treeFolders[i];
		}
	}
	return NULL;
}

static int InitExtraFolders(void)
{
	struct stat     stat_buffer;
	struct _finddata_t files;
	int             i, count = 0;
	long            hLong;
	char*           ext;
	char            buf[256];
	char            curdir[MAX_PATH];
	const char*     dir = GetFolderDir();

	memset(ExtraFolderData, 0, (MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS)* sizeof(LPEXFOLDERDATA));

	/* NPW 9-Feb-2003 - MSVC stat() doesn't like stat() called with an empty
	 * string
	 */
	if (dir[0] == '\0')
		dir = ".";

	if (stat(dir, &stat_buffer) != 0)
    {
		_mkdir(dir);
    }

	_getcwd(curdir, MAX_PATH);

	_chdir(dir);

	hLong = _findfirst("*", &files);

	for (i = 0; i < MAX_EXTRA_FOLDERS; i++)
    {
		ExtraFolderIcons[i] = NULL;
    }

	numExtraIcons = 0;

	while (!_findnext(hLong, &files))
	{
		if ((files.attrib & _A_SUBDIR) == 0)
		{
			FILE *fp;

			fp = fopen(files.name, "r");
			if (fp != NULL)
			{
				int icon[2] = { 0, 0 };
				char *p, *name;

				while (fgets(buf, 256, fp))
				{
					if (buf[0] == '[')
					{
						p = strchr(buf, ']');
						if (p == NULL)
							continue;

						*p = '\0';
						name = &buf[1];
						if (!strcmp(name, "FOLDER_SETTINGS"))
						{
							while (fgets(buf, 256, fp))
							{
								name = strtok(buf, " =\r\n");
								if (name == NULL)
									break;

								if (!strcmp(name, "RootFolderIcon"))
								{
									name = strtok(NULL, " =\r\n");
									if (name != NULL)
										SetExtraIcons(name, &icon[0]);
								}
								if (!strcmp(name, "SubFolderIcon"))
								{
									name = strtok(NULL, " =\r\n");
									if (name != NULL)
										SetExtraIcons(name, &icon[1]);
								}
							}
							break;
						}
					}
				}
				fclose(fp);

				strcpy(buf, files.name);
				ext = strrchr(buf, '.');

				if (ext && *(ext + 1) && !mame_stricmp(ext + 1, "ini"))
				{
					ExtraFolderData[count] = malloc(sizeof(EXFOLDERDATA));
					if (ExtraFolderData[count]) 
					{
						*ext = '\0';

						memset(ExtraFolderData[count], 0, sizeof(EXFOLDERDATA));

						strncpy(ExtraFolderData[count]->m_szTitle, buf, 63);
						ExtraFolderData[count]->m_nFolderId   = next_folder_id++;
						ExtraFolderData[count]->m_nParent	  = -1;
						ExtraFolderData[count]->m_dwFlags	  = F_CUSTOM;
						ExtraFolderData[count]->m_nIconId	  = icon[0] ? -icon[0] : IDI_FOLDER;
						ExtraFolderData[count]->m_nSubIconId  = icon[1] ? -icon[1] : IDI_FOLDER;
						//dprintf("extra folder with icon %i, subicon %i",
						//ExtraFolderData[count]->m_nIconId,
						//ExtraFolderData[count]->m_nSubIconId);
						count++;
					}
				}
			}
		}
	}

	_chdir(curdir);
	return count;
}

void FreeExtraFolders(void)
{
	int i;

	for (i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i])
		{
			free(ExtraFolderData[i]);
			ExtraFolderData[i] = NULL;
		}
	}

	for (i = 0; i < numExtraIcons; i++)
    {
		free(ExtraFolderIcons[i]);
    }

	numExtraIcons = 0;

}


static void SetExtraIcons(char *name, int *id)
{
	char *p = strchr(name, '.');

	if (p != NULL)
		*p = '\0';

	ExtraFolderIcons[numExtraIcons] = malloc(strlen(name) + 1);
	if (ExtraFolderIcons[numExtraIcons])
	{
		*id = ICON_MAX + numExtraIcons;
		strcpy(ExtraFolderIcons[numExtraIcons], name);
		numExtraIcons++;
	}
}


// Called to add child folders of the top level extra folders already created
BOOL TryAddExtraFolderAndChildren(int parent_index)
{
    FILE*   fp = NULL;
    char    fname[MAX_PATH];
    char    readbuf[256];
    char*   p;
    char*   name;
    int     id, current_id;
    LPTREEFOLDER lpTemp = NULL;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

    current_id = lpFolder->m_nFolderId;
    
    id = lpFolder->m_nFolderId - MAX_FOLDERS;

    /* "folder\title.ini" */

    sprintf( fname, "%s\\%s.ini", 
        GetFolderDir(), 
        ExtraFolderData[id]->m_szTitle);
    
    fp = fopen(fname, "r");
    if (fp == NULL)
        return FALSE;
    
    while ( fgets(readbuf, 256, fp) )
    {
        /* do we have [...] ? */

        if (readbuf[0] == '[')
        {
            p = strchr(readbuf, ']');
            if (p == NULL)
            {
                continue;
            }
            
            *p = '\0';
            name = &readbuf[1];
     
            /* is it [FOLDER_SETTINGS]? */

            if (strcmp(name, "FOLDER_SETTINGS") == 0)
            {
                current_id = -1;
                continue;
            }
            else
            {
                /* it it [ROOT_FOLDER]? */

                if (!strcmp(name, "ROOT_FOLDER"))
                {
                    current_id = lpFolder->m_nFolderId;
                    lpTemp = lpFolder;

                }
                else
                {
                    /* must be [folder name] */

                    current_id = next_folder_id++;
					/* create a new folder with this name,
					   and the flags for this folder as read from the registry */
					lpTemp = NewFolder(name,current_id,parent_index,
 									   ExtraFolderData[id]->m_nSubIconId,
 									   GetFolderFlags(numFolders) | F_CUSTOM);
					ExtraFolderData[current_id] = malloc(sizeof(EXFOLDERDATA) );
					memset(ExtraFolderData[current_id], 0, sizeof(EXFOLDERDATA));

					ExtraFolderData[current_id]->m_nFolderId = current_id - MAX_EXTRA_FOLDERS;
					ExtraFolderData[current_id]->m_nIconId = ExtraFolderData[id]->m_nSubIconId;
					ExtraFolderData[current_id]->m_nParent = ExtraFolderData[id]->m_nFolderId;
					ExtraFolderData[current_id]->m_nSubIconId = -1;
					strcpy( ExtraFolderData[current_id]->m_szTitle, name );
					ExtraFolderData[current_id]->m_dwFlags = ExtraFolderData[id]->m_dwFlags;
                    AddFolder(lpTemp);
                }
            }
        }
        else if (current_id != -1)
        {
            /* string on a line by itself -- game name */

            name = strtok(readbuf, " \t\r\n");
            if (name == NULL)
            {
                current_id = -1;
                continue;
            }

            /* IMPORTANT: This assumes that all driver names are lowercase! */
            strlwr( name );

			if (lpTemp == NULL)
			{
				ErrorMsg("Error parsing %s: missing [folder name] or [ROOT_FOLDER]",
						 fname);
				current_id = lpFolder->m_nFolderId;
				lpTemp = lpFolder;
			}
			AddGame(lpTemp,GetGameNameIndex(name));
        }
    }

    if ( fp )
    {
        fclose( fp );
    }

    return TRUE;
}


void GetFolders(TREEFOLDER ***folders,int *num_folders)
{
	*folders = treeFolders;
	*num_folders = numFolders;
}

BOOL TryRenameCustomFolderIni(LPTREEFOLDER lpFolder,const char *old_name,const char *new_name)
{
	char filename[MAX_PATH];
	char new_filename[MAX_PATH];
	LPTREEFOLDER lpParent = NULL;
	if (lpFolder->m_nParent >= 0)
	{
		//it is a custom SubFolder
		lpParent = GetFolder( lpFolder->m_nParent );
		if( lpParent )
		{
			snprintf(filename,sizeof(filename),"%s\\%s\\%s.ini",GetIniDir(),lpParent->m_lpTitle, old_name );
			snprintf(new_filename,sizeof(new_filename),"%s\\%s\\%s.ini",GetIniDir(),lpParent->m_lpTitle, new_name );
			MoveFile(filename,new_filename);
		}
	}
	else
	{
		//Rename the File, if it exists
		snprintf(filename,sizeof(filename),"%s\\%s.ini",GetIniDir(),old_name );
		snprintf(new_filename,sizeof(new_filename),"%s\\%s.ini",GetIniDir(), new_name );
		MoveFile(filename,new_filename);
		//Rename the Directory, if it exists
		snprintf(filename,sizeof(filename),"%s\\%s",GetIniDir(),old_name );
		snprintf(new_filename,sizeof(new_filename),"%s\\%s",GetIniDir(), new_name );
		MoveFile(filename,new_filename);
	}
	return TRUE;
}

BOOL TryRenameCustomFolder(LPTREEFOLDER lpFolder,const char *new_name)
{
	BOOL retval;
	char filename[MAX_PATH];
	char new_filename[MAX_PATH];
	
	if (lpFolder->m_nParent >= 0)
	{
		// a child extra folder was renamed, so do the rename and save the parent

		// save old title
		char *old_title = lpFolder->m_lpTitle;

		// set new title
		lpFolder->m_lpTitle = (char *)malloc(strlen(new_name) + 1);
		strcpy(lpFolder->m_lpTitle,new_name);

		// try to save
		if (TrySaveExtraFolder(lpFolder) == FALSE)
		{
			// failed, so free newly allocated title and restore old
			free(lpFolder->m_lpTitle);
			lpFolder->m_lpTitle = old_title;
			return FALSE;
		}
		TryRenameCustomFolderIni(lpFolder, old_title, new_name);
		// successful, so free old title
		free(old_title);
		return TRUE;
	}
	
	// a parent extra folder was renamed, so rename the file

    snprintf(new_filename,sizeof(new_filename),"%s\\%s.ini",GetFolderDir(),new_name);
    snprintf(filename,sizeof(filename),"%s\\%s.ini",GetFolderDir(),lpFolder->m_lpTitle);

	retval = MoveFile(filename,new_filename);

	if (retval)
	{
		TryRenameCustomFolderIni(lpFolder, lpFolder->m_lpTitle, new_name);
		free(lpFolder->m_lpTitle);
		lpFolder->m_lpTitle = (char *)malloc(strlen(new_name) + 1);
		strcpy(lpFolder->m_lpTitle,new_name);
	}
	else
	{
		char buf[500];
		snprintf(buf,sizeof(buf),"Error while renaming custom file %s to %s",
				 filename,new_filename);
		MessageBox(GetMainWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);
	}
	return retval;
}

void AddToCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
    if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
	    MessageBox(GetMainWindow(),"Unable to add game to non-custom folder",
				   MAME32NAME,MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) == 0)
	{
		AddGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == FALSE)
			RemoveGame(lpFolder,driver_index); // undo on error
	}
}

void RemoveFromCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
    if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
	    MessageBox(GetMainWindow(),"Unable to remove game from non-custom folder",
				   MAME32NAME,MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) != 0)
	{
		RemoveGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == FALSE)
			AddGame(lpFolder,driver_index); // undo on error
	}
}

BOOL TrySaveExtraFolder(LPTREEFOLDER lpFolder)
{
    char fname[MAX_PATH];
	FILE *fp;
	BOOL error = FALSE;
    int i,j;

	LPTREEFOLDER root_folder = NULL;
	LPEXFOLDERDATA extra_folder = NULL;

	for (i=0;i<numExtraFolders;i++)
	{
	    if (ExtraFolderData[i]->m_nFolderId == lpFolder->m_nFolderId)
		{
		    root_folder = lpFolder;
		    extra_folder = ExtraFolderData[i];
			break;
		}

		if (lpFolder->m_nParent >= 0 &&
			ExtraFolderData[i]->m_nFolderId == treeFolders[lpFolder->m_nParent]->m_nFolderId)
		{
			root_folder = treeFolders[lpFolder->m_nParent];
			extra_folder = ExtraFolderData[i];
			break;
		}
	}

	if (extra_folder == NULL || root_folder == NULL)
	{
	   MessageBox(GetMainWindow(), "Error finding custom file name to save", MAME32NAME, MB_OK | MB_ICONERROR);
	   return FALSE;
	}
    /* "folder\title.ini" */

    snprintf( fname, sizeof(fname), "%s\\%s.ini", GetFolderDir(), extra_folder->m_szTitle);

    fp = fopen(fname, "wt");
    if (fp == NULL)
	   error = TRUE;
	else
	{
	   TREEFOLDER *folder_data;


	   fprintf(fp,"[FOLDER_SETTINGS]\n");
	   // negative values for icons means it's custom, so save 'em
	   if (extra_folder->m_nIconId < 0)
	   {
		   fprintf(fp, "RootFolderIcon %s\n",
				   ExtraFolderIcons[(-extra_folder->m_nIconId) - ICON_MAX]);
	   }
	   if (extra_folder->m_nSubIconId < 0)
	   {
		   fprintf(fp,"SubFolderIcon %s\n",
				   ExtraFolderIcons[(-extra_folder->m_nSubIconId) - ICON_MAX]);
	   }

	   /* need to loop over all our TREEFOLDERs--first the root one, then each child.
		  start with the root */

	   folder_data = root_folder;

	   fprintf(fp,"\n[ROOT_FOLDER]\n");

	   for (i=0;i<GetNumGames();i++)
	   {
		   int driver_index = GetIndexFromSortedIndex(i); 
		   if (TestBit(folder_data->m_lpGameBits,driver_index))
		   {
			   fprintf(fp,"%s\n",drivers[driver_index]->name);
		   }
	   }

	   /* look through the custom folders for ones with our root as parent */
	   for (j=0;j<numFolders;j++)
	   {
		   folder_data = treeFolders[j];

		   if (folder_data->m_nParent >= 0 &&
			   treeFolders[folder_data->m_nParent] == root_folder)
		   {
			   fprintf(fp,"\n[%s]\n",folder_data->m_lpTitle);
			   
			   for (i=0;i<GetNumGames();i++)
			   {
				   int driver_index = GetIndexFromSortedIndex(i); 
				   if (TestBit(folder_data->m_lpGameBits,driver_index))
				   {
					   fprintf(fp,"%s\n",drivers[driver_index]->name);
				   }
			   }
		   }
	   }
	   if (fclose(fp) != 0)
		   error = TRUE;
    }

	if (error)
	{
		char buf[500];
		snprintf(buf,sizeof(buf),"Error while saving custom file %s",fname);
		MessageBox(GetMainWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);
	}
	return !error;
}

HIMAGELIST GetTreeViewIconList(void)
{
    return hTreeSmall;
}

int GetTreeViewIconIndex(int icon_id)
{
	int i;

	if (icon_id < 0)
		return -icon_id;

	for (i = 0; i < sizeof(treeIconNames) / sizeof(treeIconNames[0]); i++)
	{
		if (icon_id == treeIconNames[i].nResourceID)
			return i;
	}

	return -1;
}

/* End of source file */
