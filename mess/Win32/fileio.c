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

/* from mess/Win32/dirio.c */
extern const char *resolve_path(const char *path, char *buf, size_t buflen);
extern char *strncpyz(char *dest, const char *source, size_t len);

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
				strcpy(s + (strlen(filename) - strlen(lpExt)), zipext);
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

int osd_select_file(int sel, char *filename)
{
	return 0;
}
