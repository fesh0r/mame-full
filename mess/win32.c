#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <strings.h>
#include <sys/stat.h>
#include <wingdi.h>
#include <tchar.h>
#include <assert.h>
#include <zlib.h>

#include "mess.h"
#include "unzip.h"
#include "osdepend.h"
#include "src/win32/mame32.h"
#include "src/win32/treeview.h"
#include "src/win32/resource.h"
#include "src/win32/FilePrivate.h"
#include "src/win32/DataMap.h"

/* from src/win32/win32ui.c */
typedef BOOL (WINAPI *common_file_dialog_proc)(LPOPENFILENAME lpofn);

typedef struct {
	HANDLE hDir;
	WIN32_FIND_DATAA finddata;
	BOOL bDone;
} OSD_WIN32_FIND_DATA;

static char szCurrentDirectory[MAX_PATH];
char crcfilename[MAX_PATH] = "";
char pcrcfilename[MAX_PATH] = "";
const char *crcfile = crcfilename;
const char *pcrcfile = pcrcfilename;

/* ************************************************************************ */

char *strncpyz(char *dest, const char *source, size_t len)
{
	char *s;
	if (len) {
		s = strncpy(dest, source, len - 1);
		dest[len-1] = '\0';
	}
	else {
		s = dest;
	}
	return s;
}

char *strncatz(char *dest, const char *source, size_t len)
{
	int l;
	l = strlen(dest);
	dest += l;
	if (len > l)
		len -= l;
	else
		len = 0;
	return strncpyz(dest, source, len);
}

/* ************************************************************************ */
/* Floppy disk controller                                                   */
/* ************************************************************************ */

int osd_fdc_init(void)
{
	return 0;
}

void osd_fdc_motors(unsigned char unit)
{
	return;
}


void osd_fdc_density(unsigned char unit, unsigned char density,unsigned char tracks, unsigned char spt, unsigned char eot, unsigned char secl)
{
	return;
}

unsigned char osd_fdc_step(int dir, unsigned char *track)
{
	return 0;
}


unsigned char osd_fdc_seek(unsigned char t, unsigned char *track)
{
	return 0;
}

unsigned char osd_fdc_recal(unsigned char *track)
{
	return 0;
}


unsigned char osd_fdc_put_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff, unsigned char ddam)
{
	return 0;
}

unsigned char osd_fdc_get_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff)
{
	return 0;
}

void osd_fdc_exit(void)
{
	return;
}

unsigned char osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char *fmt)
{
	return 0;
}

/* ************************************************************************ */
/* Directories                                                              */
/* ************************************************************************ */

static void checkinit_curdir(void)
{
	const char *p;

	if (szCurrentDirectory[0] == '\0') {
		GetCurrentDirectoryA(sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]), szCurrentDirectory);
		/* Make sure there is a trailing backslash */
		p = strrchr(szCurrentDirectory, '\\');
		if (!p || p[1])
			strncatz(szCurrentDirectory, "\\", sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]));
	}
}

/* This function takes in a pathname, and resolves it according to our 
 * current directory
 */
static const char *resolve_path(const char *path, char *buf, size_t buflen)
{
	char *s;

	checkinit_curdir();

	/* A bogus resolve? */
	if (!path || !path[0] || ((path[0] == '.') && (!path[1])))
		return szCurrentDirectory;

	/* Absolute pathname? */
	if ((path[0] == '\\') || (path[1] == ':')) {
		if ((path[1] == ':') || (path[1] == '\\')) {
			/* Absolute pathname with drive letter or UNC pathname */
			buf[0] = '\0';
		}
		else {
			/* Absolute pathname without drive letter */
			buf[0] = szCurrentDirectory[0];
			buf[1] = szCurrentDirectory[1];
			buf[2] = '\0';
		}
		strncatz(buf, path, buflen);
		return buf;
	}

	strncpyz(buf, szCurrentDirectory, buflen);

	while(path && (path[0] == '.')) {
		/* Assume that if it is not a ., then it is a .. */
		if (path[1] != '\\') {
			/* Up one directory */
			s = strrchr(buf, '\\');
			if (s && !s[1]) {
				/* A backslash at the very end doesn't count */
				*s = '\0';
				s = strrchr(buf, '\\');
			}
			
			if (s)
				*s = '\0';
			else
				strncatz(buf, "\\..", buflen);
		}
		path = strchr(path, '\\');
		if (path)
			path++;
	}

	if (path && *path) {
		strncatz(buf, path, buflen);
	}
	return buf;
}

int osd_num_devices(void)
{
	char szBuffer[128];
	char *p;
	int i;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	i = 0;
	for (p = szBuffer; *p; p += (strlen(p) + 1))
		i++;
	return i;
}

const char *osd_get_device_name(int idx)
{
	static char szBuffer[128];
	const char *p;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	p = szBuffer;
	while(idx--)
		p += strlen(p) + 1;

	return p;
}

void *osd_dir_open(const char *dirname, const char *filemask)
{
	char buf[MAX_PATH];
	OSD_WIN32_FIND_DATA *pfd = NULL;
	char *tmpbuf = NULL;
	char *s;
	
	dirname = resolve_path(dirname, buf, sizeof(buf) / sizeof(buf[0]));
	if (!dirname)
		goto error;

	pfd = malloc(sizeof(OSD_WIN32_FIND_DATA));
	if (!pfd)
		goto error;
	memset(pfd, 0, sizeof(*pfd));

	tmpbuf = malloc(strlen(dirname) + strlen(filemask) + 2);
	if (!tmpbuf)
		goto error;
	strcpy(tmpbuf, dirname);
	s = tmpbuf + strlen(tmpbuf);
	s[0] = '\\';
	strcpy(s + 1, filemask);

	pfd->hDir = FindFirstFileA(tmpbuf, &pfd->finddata);
	if (pfd->hDir == INVALID_HANDLE_VALUE)
		goto error;

	free(tmpbuf);
	return (void *) pfd;

error:
	if (pfd)
		free(pfd);
	if (tmpbuf)
		free(tmpbuf);
	return NULL;
}

int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir)
{
	int result;
	OSD_WIN32_FIND_DATA *pfd = (OSD_WIN32_FIND_DATA *) dir;

	if (pfd->bDone) {
		if (name && namelength)
			*name = '\0';
		result = 0;
	}
	else {
		if (name)
			strncpyz(name, pfd->finddata.cFileName, namelength);
		result = strlen(pfd->finddata.cFileName);

		if (is_dir)
			*is_dir = (pfd->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

		pfd->bDone = FindNextFileA(pfd->hDir, &pfd->finddata) == 0;
	}
	return result;
}

void osd_dir_close(void *dir)
{
	OSD_WIN32_FIND_DATA *pfd = (OSD_WIN32_FIND_DATA *) dir;

	FindClose(pfd->hDir);
	free(pfd);
}

static BOOL dir_exists(const char *s)
{
	void *dp;
	int rc = 0;

	dp = osd_dir_open(s, "*.*");
	if (dp) {
		rc = osd_dir_get_entry(dp, NULL, 0, NULL);
		osd_dir_close(dp);
	}
	return rc > 0;
}

void osd_change_directory(const char *dir)
{
	char buf[MAX_PATH];
	const char *s;

	s = resolve_path(dir, buf, sizeof(buf) / sizeof(buf[0]));
	if (s && (s != szCurrentDirectory)) {
		/* If there is no trailing backslash, add one */
		if ((s == buf) && (buf[strlen(buf)-1] != '\\'))
			strncatz(buf, "\\", sizeof(buf) / sizeof(buf[0]));

		if (dir_exists(s))
			strncpyz(szCurrentDirectory, s, sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]));
	}
}

void osd_change_device(const char *device)
{
	char szBuffer[3];
	szBuffer[0] = device[0];
	szBuffer[1] = ':';
	szBuffer[2] = '\0';
	osd_change_directory(szBuffer);
}

const char *osd_get_cwd(void)
{
	checkinit_curdir();
	return szCurrentDirectory;
}

static int MessImageFopenZip(const char *filename, mame_file *mf, int write)
{
	char buf1[MAX_PATH];
	char buf2[MAX_PATH];
	LPSTR lpSlashPos;
	ZIP *pZip;
	struct zipent *pZipEnt;

#if 0
	if (write)
		return 0;	/* Can't write to a ZIP file */
#endif

	filename = resolve_path(filename, buf1, sizeof(buf1) / sizeof(buf1[0]));
	if (!filename)
		return 0;

	pZip = openzip(filename);
	if (!pZip)
		return 0;

	pZipEnt = readzip(pZip);
	if (!pZipEnt) {
		closezip(pZip);
		return 0;
	}

	lpSlashPos = strrchr(pZipEnt->name, '/');
	strncpyz(buf2, lpSlashPos ? lpSlashPos + 1 : pZipEnt->name, sizeof(buf2) / sizeof(buf2[0]));

	if (load_zipped_file(filename, buf2, &mf->file_data, &mf->file_length)) {
		closezip(pZip);
		return 0;
	}

	mf->access_type = ACCESS_ZIP;
	mf->crc = crc32(0L, mf->file_data, mf->file_length);
	closezip(pZip);
	return 1;
}

/* MessImageFopen
 *
 * This is the code used to open a file of type OSD_IMAGE_FOPEN_R and
 * OSD_IMAGE_FOPEN_RW
 *
 * ZIP files will be loaded in memory, and normal images will be loaded on
 * disk.  Note that non-ZIP files will be loaded into memory briefly to do a
 * checkum.  There will be a potential problem when image files can be too big
 * to reside in memory.
 */
int MessImageFopen(const char *filename, mame_file *mf, int write, int (*checksum_file)(const char* file, unsigned char **p, unsigned int *size, unsigned int *crc))
{
	static char zipext[] = ".ZIP";
	static char *write_modes[] = {"rb","wb","r+b","r+b","w+b"};
	LPSTR lpExt;
	unsigned int dummy;
	int found;
	char *s;

	assert(write < (sizeof(write_modes) / sizeof(write_modes[0])));

	lpExt = strrchr(filename, '.');
	if (lpExt && !stricmp(lpExt, zipext)) {
		/* ZIP file */
		if (!MessImageFopenZip(filename, mf, write))
			return 0;
	}
	else {
		char buf1[MAX_PATH];

		filename = resolve_path(filename, buf1, sizeof(buf1) / sizeof(buf1[0]));
		if (!filename)
			return 0;

		/* Real file */
		if (checksum_file(filename, NULL, &dummy, &mf->crc)) {
			/* Try renaming the file */
			s = malloc(strlen(filename) - strlen(lpExt) + sizeof(zipext));
			if (!s)
				return 0;
			strcpy(s, filename);
			if (lpExt)
				strcpy(s + (lpExt - filename), zipext);
			else
				strcat(s, zipext);
			found = MessImageFopenZip(s, mf, write);
			free(s);
			if (found)
				return 1;
		}
		mf->access_type = ACCESS_FILE;
		mf->fptr = fopen(filename, write_modes[write]);
		if (!mf->fptr) {
			if (write != OSD_FOPEN_RW_CREATE)
				return 0;
			mf->fptr = fopen(filename, "wb");
			if (!mf->fptr)
				return 0;
		}
	}
	return 1;
}

