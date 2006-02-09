#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include "driver.h"
#include "mess.h"
#include "unzip.h"
#include "osdepend.h"
#include "utils.h"
#include "strconv.h"

/* ************************************************************************ */
/* Directories                                                              */
/* ************************************************************************ */

typedef struct {
	HANDLE hDir;
	WIN32_FIND_DATA finddata;
	BOOL bDone;
} OSD_WIN32_FIND_DATA;

static char szCurrentDirectory[MAX_PATH];

static void checkinit_curdir(void)
{
	if (szCurrentDirectory[0] == '\0') {
#ifdef UNDER_CE
		WCHAR szBuffer[MAX_PATH];
		GetModuleFileName(NULL, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
		wcsrchr(szBuffer, '\\')[1] = '\0';
		_snprintf(szCurrentDirectory, sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]), "%S", szBuffer);
#else
		const char *p;
		GetCurrentDirectoryA(sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]), szCurrentDirectory);
		/* Make sure there is a trailing backslash */
		p = strrchr(szCurrentDirectory, '\\');
		if (!p || p[1])
			strncatz(szCurrentDirectory, "\\", sizeof(szCurrentDirectory) / sizeof(szCurrentDirectory[0]));
#endif
	}
}

void resetdir(void)
{
	szCurrentDirectory[0] = '\0';
}

static int preserve_current_drive_letter(const char *path)
{
#ifdef UNDER_CE
	return 0;
#else
	return (path[1] != ':') && (path[1] != '\\');
#endif
}

/* This function takes in a pathname, and resolves it according to our
 * current directory
 */
const char *resolve_path(const char *path, char *buf, size_t buflen)
{
	char *s;

	checkinit_curdir();

	/* A bogus resolve? */
	if (!path || !path[0] || ((path[0] == '.') && (!path[1])))
		return szCurrentDirectory;

	/* Absolute pathname? */
	if ((path[0] == '\\') || (path[1] == ':')) {
		if (preserve_current_drive_letter(path)) {
			/* Absolute pathname without drive letter */
			buf[0] = szCurrentDirectory[0];
			buf[1] = szCurrentDirectory[1];
			buf[2] = '\0';
		}
		else {
			/* Absolute pathname with drive letter or UNC pathname */
			buf[0] = '\0';
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

#ifdef UNDER_CE
int osd_num_devices(void)
{
	return 1;
}

const char *osd_get_device_name(int idx)
{
	return "\\";
}
#else
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
#endif

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

#ifdef UNICODE
	pfd->hDir = FindFirstFile(A2W(tmpbuf), &pfd->finddata);
#else
	pfd->hDir = FindFirstFile(tmpbuf, &pfd->finddata);
#endif
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
		if (name) {
#ifdef UNICODE
			_snprintf(name, namelength, "%S", pfd->finddata.cFileName);
#else
			strncpyz(name, pfd->finddata.cFileName, namelength);
#endif
		}
#ifdef UNICODE
		result = wcslen(pfd->finddata.cFileName);
#else
		result = strlen(pfd->finddata.cFileName);
#endif

		if (is_dir)
			*is_dir = (pfd->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

		pfd->bDone = FindNextFile(pfd->hDir, &pfd->finddata) == 0;
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
		if ((s == buf) && (buf[strlen(buf)-1] != '\\') && (buf[strlen(buf)-1] != '/'))
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

