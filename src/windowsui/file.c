/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  file.c

	File handling code.

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <sys/stat.h>
#include <assert.h>
#include <osdepend.h>
#include <unzip.h>
#include "options.h"
#include "ScreenShot.h"
#include "file.h"
#include "rc.h"
#include "m32util.h"

/* Verbose outputs to error.log ? */
#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/***************************************************************************
 Internal structures
***************************************************************************/

#define MAXPATHS 256

typedef struct
{
	char* m_Paths[MAXPATHS];
	int   m_NumPaths;
	char* m_Buf;
} tDirPaths;

typedef enum
{
	kPlainFile,
	kRAMFile,
	kZippedFile
} eFileType;

typedef struct
{
	FILE *file;
	unsigned char *data;
	unsigned int offset;
	unsigned int length;
	eFileType type;
	unsigned int crc;
}	FakeFileHandle;

/***************************************************************************
 function prototypes
***************************************************************************/

static void File_SetPaths(tDirPaths* pDirPath, const char* path);

/***************************************************************************
 External function prototypes
***************************************************************************/

/***************************************************************************
 External variables
***************************************************************************/

/***************************************************************************
 Internal variables
***************************************************************************/

static tDirPaths RomDirPath;
static tDirPaths SampleDirPath;
static tDirPaths IniDirPath;

#ifdef MESS
static tDirPaths SoftwareDirPath;

int GetMessSoftwarePathCount(void)
{
	return SoftwareDirPath.m_NumPaths;
}

const char *GetMessSoftwarePath(int i)
{
	return SoftwareDirPath.m_Paths[i];
}
#endif

/**************************************************************************
 External functions
***************************************************************************/

int File_Init(void)
{
	memset(&RomDirPath,    0, sizeof(RomDirPath));
	memset(&SampleDirPath, 0, sizeof(SampleDirPath));
	memset(&IniDirPath, 0, sizeof(IniDirPath));
#ifdef MESS
	memset(&SoftwareDirPath, 0, sizeof(SampleDirPath));
#endif

	File_UpdatePaths();

	return 0;
}

void File_Exit(void)
{
	if (RomDirPath.m_Buf != NULL)
	{
		free(RomDirPath.m_Buf);
		RomDirPath.m_Buf = NULL;
	}
	if (SampleDirPath.m_Buf != NULL)
	{
		free(SampleDirPath.m_Buf);
		SampleDirPath.m_Buf = NULL;
	}
#ifdef MESS
	if (SoftwareDirPath.m_Buf != NULL)
	{
		free(SoftwareDirPath.m_Buf);
		SoftwareDirPath.m_Buf = NULL;
	}
#endif
}

/* Only checks for a .zip file */
BOOL File_ExistZip(const char* gamename, int filetype)
{
	char		name[FILENAME_MAX];
	struct stat file_stat;
	tDirPaths*	pDirPaths = NULL;
	int 		the_index;
	const char* dirname = NULL;

	switch (filetype)
	{
	case FILETYPE_ROM:
	case FILETYPE_SAMPLE:
	case FILETYPE_INI:

		if (filetype == FILETYPE_ROM)
			pDirPaths = &RomDirPath;
		if (filetype == FILETYPE_SAMPLE)
			pDirPaths = &SampleDirPath;
		if (filetype == FILETYPE_INI)
			pDirPaths = &IniDirPath;

		for (the_index = 0; the_index < pDirPaths->m_NumPaths; the_index++)
		{
			dirname = pDirPaths->m_Paths[the_index];

			sprintf(name, "%s/%s.zip", dirname, gamename);
			if (stat(name, &file_stat) == 0 && !(file_stat.st_mode & S_IFDIR))
				return TRUE;
		}
		break;

	case FILETYPE_HIGHSCORE:
	case FILETYPE_CONFIG:
	case FILETYPE_INPUTLOG:
		return FALSE;
	}

	return FALSE;
}

/* gamename holds the driver name, filename is only used for ROMs and samples. */
BOOL File_Status(const char* gamename, const char* filename, int filetype)
{
	struct stat  sbuf;
	char		 name[FILENAME_MAX];
	int 		 i;
	unsigned int len = 0, crc = 0;
	const char*  dirname;
	BOOL		 found = FALSE;
	tDirPaths*	 pDirPaths;

	if (filetype == FILETYPE_ROM)
		pDirPaths = &RomDirPath;
	else
	if (filetype == FILETYPE_SAMPLE)
		pDirPaths = &SampleDirPath;
	else
	if (filetype == FILETYPE_INI)
		pDirPaths = &IniDirPath;
	else
		return FALSE; /* Only used for ROMS and Samples */

	found = FALSE;

	for (i = 0; i < pDirPaths->m_NumPaths; i++)
	{
		dirname = pDirPaths->m_Paths[i];

		/*
			1) dirname/gamename.zip
			2) dirname/gamename
			3) dirname/gamename.zif - ZipFolders
			4) dirname/gamename.zip - ZipMagic
		 */

		/* zip file. */
		sprintf(name, "%s/%s.zip", dirname, gamename);
		found = (checksum_zipped_file(filetype,0,name, filename, &len, &crc) == 0);
		if (found)
			break;

		/* rom directory */
		sprintf(name, "%s/%s/%s", dirname, gamename, filename);
		found = (stat(name, &sbuf) == 0 && !(sbuf.st_mode & S_IFDIR));
		if (found)
			break;

		/* ZipMagic */
		sprintf(name, "%s/%s.zip/%s", dirname, gamename, filename);
		found = (stat(name, &sbuf) == 0 && !(sbuf.st_mode & S_IFDIR));
		if (found)
			break;
	}
	return found;
}

void File_UpdatePaths(void)
{
	extern struct rc_option fileio_opts[];

	File_SetPaths(&RomDirPath,	  GetRomDirs());
	File_SetPaths(&SampleDirPath, GetSampleDirs());
#ifdef MESS
	File_SetPaths(&SoftwareDirPath, GetSoftwareDirs());
#endif

#ifdef MESS
	rc_set_option2(fileio_opts, "biospath",			  GetRomDirs(), 		 MAXINT_PTR);
	rc_set_option2(fileio_opts, "softwarepath",       GetSoftwareDirs(),     MAXINT_PTR);
	rc_set_option2(fileio_opts, "CRC_directory",      GetCrcDir(),           MAXINT_PTR);
#else
	rc_set_option2(fileio_opts, "rompath",			  GetRomDirs(), 		 MAXINT_PTR);
#endif
	rc_set_option2(fileio_opts, "samplepath",		  GetSampleDirs(),		 MAXINT_PTR);
	rc_set_option2(fileio_opts, "inipath",			  GetIniDirs(),			 MAXINT_PTR);
	rc_set_option2(fileio_opts, "cfg_directory",	  GetCfgDir(),			 MAXINT_PTR);
	rc_set_option2(fileio_opts, "nvram_directory",	  GetNvramDir(),		 MAXINT_PTR);
	rc_set_option2(fileio_opts, "memcard_directory",  GetMemcardDir(),		 MAXINT_PTR);
	rc_set_option2(fileio_opts, "input_directory",	  GetInpDir(),			 MAXINT_PTR);
	rc_set_option2(fileio_opts, "hiscore_directory",  GetHiDir(),			 MAXINT_PTR);
	rc_set_option2(fileio_opts, "state_directory",	  GetStateDir(),		 MAXINT_PTR);
	rc_set_option2(fileio_opts, "artwork_directory",  GetArtDir(),			 MAXINT_PTR);
	rc_set_option2(fileio_opts, "snapshot_directory", GetImgDir(),			 MAXINT_PTR);
	//rc_set_option2(fileio_opts, "cheat_directory",	GetCheatDir(),		   MAXINT_PTR);
	rc_set_option2(fileio_opts, "cheat_file",		  GetCheatFileName(),	 MAXINT_PTR);
	rc_set_option2(fileio_opts, "history_file", 	  GetHistoryFileName(),  MAXINT_PTR);
	rc_set_option2(fileio_opts, "mameinfo_file",	  GetMAMEInfoFileName(), MAXINT_PTR);
	rc_set_option2(fileio_opts, "ctrlr_directory",    GetCtrlrDir(),         MAXINT_PTR);
}

/***************************************************************************
 Internal functions
***************************************************************************/

static void File_SetPaths(tDirPaths* pDirPath, const char* path)
{
	char* token;

	if (path == NULL)
		return;

	if (pDirPath->m_Buf != NULL)
	{
		free(pDirPath->m_Buf);
		pDirPath->m_Buf = NULL;
	}

	pDirPath->m_Buf = strdup(path);

	pDirPath->m_NumPaths = 0;
	token = strtok(pDirPath->m_Buf, ";");
	while ((pDirPath->m_NumPaths < MAXPATHS) && token)
	{
		pDirPath->m_Paths[pDirPath->m_NumPaths] = token;
		pDirPath->m_NumPaths++;
		token = strtok(NULL, ";");
	}
}
