#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <tchar.h>
#include <commctrl.h>

#include "ui/screenshot.h"
#include "ui/bitmask.h"
#include "ui/options.h"
#include "ui/mame32.h"
#include "ui/picker.h"

#include "SoftwareList.h"
#include "mame.h"
#include "driver.h"
#include "unzip.h"
#include "osd_cpu.h"
#include "strconv.h"
#include "snprintf.h"

#if HAS_HASH
#include "hashfile.h"
#endif /* HAS_HASH */

/* from src/mess/win32.c */
char *strncatz(char *dest, const char *source, size_t len);
char *strncpyz(char *dest, const char *source, size_t len);

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);

static int s_nGame;

#ifndef ZEXPORT
#ifdef _MSC_VER
#define ZEXPORT WINAPI
#define alloca _alloca
#else
#define ZEXPORT
#endif
#endif

extern unsigned int ZEXPORT crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

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

/* ----------------------------------------------------------------- *
 * Type declarations                                                 *
 * ----------------------------------------------------------------- */

enum RealizeLevel
{
	REALIZE_IMMEDIATE,	/* Calculate the file type when the extension is known */
	REALIZE_ZIPS,		/* Open up ZIPs, and calculate their extensions and CRCs */
	REALIZE_ALL,		/* Open up all files, and calculate their full hashes */
	REALIZE_DONE
};

struct deviceentry
{
	int icon;
	const char *dlgname;
};

typedef struct tagImageData
{
    struct tagImageData *next;
    const TCHAR *name;
    TCHAR *fullname;
    const struct IODevice *dev;
	int realized;

#if HAS_HASH
	char hash[HASH_BUF_SIZE];
	const struct hash_info *hashinfo;
#endif /* HAS_HASH */
} ImageData;

/* ----------------------------------------------------------------- *
 * Static variables                                                  *
 * ----------------------------------------------------------------- */

static ImageData *mess_images;
static ImageData **mess_images_index;
static int mess_images_count;
static int mess_image_nums[MAX_IMAGES];

#if HAS_IDLING
static BOOL mess_idle_work;
static UINT s_nIdleImageNum;
static enum RealizeLevel s_eRealizeLevel;
#endif /* HAS_IDLING */

#if HAS_HASH
static hash_file *mess_hash_file;
#endif /* HAS_HASH */

static void AssertValidDevice(int d)
{
	assert(((d >= 0) && (d < IO_COUNT)) || (d == IO_UNKNOWN) || (d == IO_BAD) || (d == IO_ZIP));
}



/* ************************************************************************ */
/* Code for manipulation of image list                                      */
/* ************************************************************************ */

/* Specify IO_COUNT for type if you want all types */
void SetupImageTypes(int nDriver, mess_image_type *types, int count, BOOL bZip, int type)
{
    const struct IODevice *dev;
    int num_extensions = 0;

	memset(types, 0, sizeof(*types) * count);
    count--;

    if (bZip)
	{
		types[num_extensions].ext = "zip";
		types[num_extensions].dev = NULL;
		num_extensions++;
    }

	for(dev = device_first(drivers[nDriver]); dev; dev = device_next(drivers[nDriver], dev))
	{
		if (dev->type != IO_PRINTER)
		{
			const char *ext = dev->file_extensions;
			if (ext)
			{
				while(*ext)
				{
					if ((type == IO_COUNT) || (type == dev->type))
					{
						if (num_extensions < count)
						{
							types[num_extensions].dev = dev;
							types[num_extensions].ext = ext;
							num_extensions++;
						}
					}
					ext += strlen(ext) + 1;
				}
			}
		}
    }
}



static mess_image_type *MessLookupImageType(mess_image_type *imagetypes, const char *extension)
{
	int i;
    for (i = 0; imagetypes[i].ext; i++) {
        if (!stricmp(extension, imagetypes[i].ext))
			return &imagetypes[i];
	}
	return NULL;
}



static void MessDiscoverImageType(ImageData *img, mess_image_type *imagetypes, BOOL bReadZip, char *hash)
{
	char *lpExt;
	ZIP *pZip = NULL;
	UINT32 zipcrc = 0;
	struct zipent *pZipEnt = NULL;
	mess_image_type *imgtype;
	const char *filename = T2A(img->fullname);

	if (hash)
		*hash = '\0';
	lpExt = strrchr(filename, '.');

	/* are we a ZIP file? */
	if (lpExt && !stricmp(lpExt, ".zip"))
	{
		lpExt = NULL;
		if (bReadZip)
		{
			img->realized = 1;
			pZip = openzip(0, 0, filename);
			if (pZip)
			{
				lpExt = NULL;
				pZipEnt = readzip(pZip);
				while (pZipEnt)
				{
					lpExt = strrchr(pZipEnt->name, '.');
					if( lpExt )
					{
						imgtype = MessLookupImageType(imagetypes, lpExt+1);
						if (imgtype)
						{
							zipcrc = pZipEnt->crc32;
							break;
						}
						lpExt = NULL;
					}
					pZipEnt = readzip(pZip);
				}
			}
		}
	}
	else
		img->realized = 1;

	if (lpExt)
	{
		imgtype = MessLookupImageType(imagetypes, lpExt+1);
		if (imgtype)
		{
			img->dev = imgtype->dev;
#if HAS_HASH
			/* while we have the ZIP file open, we have a convenient opportunity
			 * to specify the CRC */
			if (hash && zipcrc && (!imgtype->dev || !imgtype->dev->partialhash))
				sprintf(hash, "c:%08x#", zipcrc);
#endif /* HAS_HASH */
		}
	}

    if (pZip)
        closezip(pZip);
}



static void MessRemoveImage(int imagenum)
{
    int i, j;

    for (i = 0, j = 0; i < options.image_count; i++)
	{
        if ((imagenum >= 0) && (imagenum != mess_image_nums[i]))
		{
            if (i != j)
			{
                options.image_files[j] = options.image_files[i];
                mess_image_nums[j] = mess_image_nums[i];
            }
            j++;
        }
        else
		{
            free((char *) options.image_files[i].name);
			options.image_files[i].name = NULL;
        }
    }
    options.image_count = j;
}



static BOOL MessSetImage(int nDriver, int imagenum, int entry)
{
    char *filename;
    mess_image_type imagetypes[64];

    if (!mess_images_index || (imagenum >= mess_images_count))
        return FALSE;		/* Invalid image index */
    filename = strdup(T2A(mess_images_index[imagenum]->fullname));
    if (!filename)
        return FALSE;		/* Out of memory */

    SetupImageTypes(nDriver, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_COUNT);

	MessDiscoverImageType(mess_images_index[imagenum], imagetypes, TRUE, NULL);

	if (!mess_images_index[imagenum]->dev)
		return FALSE;

	if (options.image_files[entry].name)
		free((void *) options.image_files[entry].name);
    options.image_files[entry].type = mess_images_index[imagenum]->dev->type;
    options.image_files[entry].name = filename;

    mess_image_nums[entry] = imagenum;
	return TRUE;
}



static BOOL MessAddImage(int nGame, int imagenum)
{
	if (options.image_count >= MAX_IMAGES)
		return FALSE;		/* Too many images */

	MessRemoveImage(imagenum);

	if (!MessSetImage(nGame, imagenum, options.image_count))
		return FALSE;

	options.image_count++;
	return TRUE;
}



static ImageData *ImageData_Alloc(const char *fullname)
{
	ImageData *newimg;
    TCHAR *separator_pos;
	const TCHAR *fullname_t;

	fullname_t = A2T(fullname);

	newimg = malloc(sizeof(ImageData) + (_tcslen(fullname_t) + 1) * sizeof(TCHAR));
	if (!newimg)
		return NULL;
	memset(newimg, 0, sizeof(ImageData));

	newimg->fullname = (TCHAR *) (((char *) newimg) + sizeof(ImageData));
	_tcscpy(newimg->fullname, fullname_t);

    separator_pos = _tcsrchr(newimg->fullname, '\\');
    newimg->name = separator_pos ? (separator_pos+1) : newimg->fullname;
	return newimg;
}



static void ImageData_Free(ImageData *img)
{
	free((void *) img);
}



static BOOL ImageData_IsBad(ImageData *img)
{
	return img->realized && !img->dev;
}



#if HAS_HASH
extern char *mess_try_image_file_as_zip(int pathindex, const char *path,
	const struct IODevice *dev);

static void CalculateHash(char *hash, const char *filename, unsigned int functions, const struct IODevice *dev)
{
	UINT32 length;
	char *new_filename;
	unsigned char *data = NULL;
	mame_file *file = NULL;

	/* check to see if we are a ZIP file */
	new_filename = mess_try_image_file_as_zip(0, filename, dev);
	if (new_filename)
		filename = new_filename;

	/* open file */
	file = mame_fopen(drivers[s_nGame]->name, filename, FILETYPE_IMAGE, OSD_FOPEN_READ);
	if (!file)
		goto done;

	/* determine length of file */
	length = (UINT32) mame_fsize(file);

	/* allocate space for entire file */
	data = malloc(length);
	if (!data)
		goto done;

	/* read entire file into memory */
	if (mame_fread(file, data, length) != length)
		goto done;

	if (dev->partialhash)
		dev->partialhash(hash, data, length, functions);
	else
		hash_compute(hash, data, length, functions);

done:
	if (file)
		mame_fclose(file);
	if (new_filename)
		free(new_filename);
	if (data)
		free(data);
}
#endif /* HAS_HASH */



static BOOL ImageData_Realize(ImageData *img, enum RealizeLevel eRealize, mess_image_type *imagetypes)
{
	BOOL bLearnedSomething = FALSE;
	char *hash_ptr = NULL;

	/* Calculate image type */
	if (!img->realized)
	{
#if HAS_HASH
		hash_ptr = img->hash;
#endif
		MessDiscoverImageType(img, imagetypes, eRealize > REALIZE_IMMEDIATE, hash_ptr);
		if (img->realized)
			bLearnedSomething = TRUE;
	}

#if HAS_HASH
	/* Calculate a hash? */
	if (mess_hash_file && (eRealize >= REALIZE_ALL) && img->dev &&
		(hashfile_functions_used(mess_hash_file) & ~hash_data_used_functions(img->hash)))
	{
		CalculateHash(img->hash, img->fullname,
			hashfile_functions_used(mess_hash_file),
			img->dev);
		bLearnedSomething = TRUE;
	}

	/* Load hash information? */
	if (mess_hash_file && img->hash[0] && !img->hashinfo)
	{
		img->hashinfo = hashfile_lookup(mess_hash_file, img->hash);
	}
#endif /* HAS_HASH */

	return bLearnedSomething;
}
	


static BOOL AppendNewImage(const char *fullname, enum RealizeLevel eRealize, ImageData ***listend, mess_image_type *imagetypes)
{
    ImageData *newimg;

	newimg = ImageData_Alloc(fullname);
    if (!newimg)
        return FALSE;

	ImageData_Realize(newimg, eRealize, imagetypes);

	if (ImageData_IsBad(newimg))
	{
		/* Unknown type of software */
		ImageData_Free(newimg);
		return FALSE;
	}

    **listend = newimg;
    *listend = &newimg->next;
    return TRUE;
}

static void AddImagesFromDirectory(int nDriver, const char *dir, BOOL bRecurse, char *buffer, size_t buffersz, ImageData ***listend)
{
    void *d;
    int is_dir;
    size_t pathlen;
    mess_image_type imagetypes[64];
	const char *olddirc;
	char *olddir;

    SetupImageTypes(nDriver, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), FALSE, IO_COUNT);

    d = osd_dir_open(dir, "*.*");
    if (d)
	{
		/* Cache the old directory */
		olddirc = osd_get_cwd();
		olddir = alloca(strlen(olddirc) + 1);
		strcpy(olddir, olddirc);

        osd_change_directory(dir);

        strncpyz(buffer, osd_get_cwd(), buffersz);
        pathlen = strlen(buffer);

        while(osd_dir_get_entry(d, buffer + pathlen, buffersz - pathlen, &is_dir))
		{
            if (!is_dir)
			{
                /* Not a directory */
                if (AppendNewImage(buffer, REALIZE_IMMEDIATE, listend, imagetypes))
                    mess_images_count++;
            }
            else if (bRecurse && strcmp(buffer + pathlen, ".") && strcmp(buffer + pathlen, ".."))
			{
                AddImagesFromDirectory(nDriver, buffer + pathlen, bRecurse, buffer, buffersz, listend);

                strncpyz(buffer, osd_get_cwd(), buffersz);
                pathlen = strlen(buffer);
            }
        }
        osd_dir_close(d);

		/* Restore the old directory */
		osd_change_directory(olddir);
    }
}



static BOOL InsertSoftwareListItem(HWND hwndSoftware, int nItem)
{
	LV_ITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.lParam = nItem;
	lvi.pszText  = LPSTR_TEXTCALLBACK;
	lvi.iImage   = I_IMAGECALLBACK;
	return ListView_InsertItem(hwndSoftware, &lvi);
}



static void InternalFillSoftwareList(HWND hwndSoftware, int nGame,
	int nPaths, LPCSTR *plpPaths, void (*error_proc)(const char *message))
{
    int i;
    ImageData *imgd;
    ImageData **pimgd;
    char *s;
	const char *path;
    char buffer[2000];
	const struct GameDriver *drv;

	s_nGame = nGame;

#if HAS_HASH
	/* update the hash file */
	if (mess_hash_file)
	{
		hashfile_close(mess_hash_file);
		mess_hash_file = NULL;
	}

	for (drv = drivers[nGame]; !mess_hash_file && drv; drv = mess_next_compatible_driver(drv))
	{
		mess_hash_file = hashfile_open(drv->name, TRUE, error_proc);
	}
#endif /* HAS_HASH */

    /* This fixes any changes the file manager may have introduced */
    resetdir();

    /* Remove any currently selected images */
    MessRemoveImage(-1);

    /* Free the list */
    if (mess_images_index)
        free(mess_images_index);
    imgd = mess_images;
    while(imgd)
	{
        ImageData *next = imgd->next;
		ImageData_Free(imgd);
        imgd = next;
    }
    mess_images = NULL;

    /* Now build the linked list */
    mess_images_count = 0;
    pimgd = &mess_images;

	for (i = 0; i < nPaths; i++)
	{
		/* Do we have a semicolon? */
		s = strchr(plpPaths[i], ';');
		if (s)
		{
			int nLen = s - plpPaths[i];
			s = malloc((nLen + 1) * sizeof(*s));
			if (!s)
				return;
			memcpy(s, plpPaths[i], nLen * sizeof(*s));
			s[nLen] = '\0';
			path = s;
		}
		else
		{
			path = plpPaths[i];
			s = NULL;
		}

        AddImagesFromDirectory(nGame, path, TRUE, buffer, sizeof(buffer), &pimgd);

		if (s)
			free(s);
    }

    if (mess_images_count)
	{
		mess_images_index = (ImageData **) malloc(sizeof(ImageData *) * mess_images_count);
		if (mess_images_index)
		{
			imgd = mess_images;
			for (i = 0; i < mess_images_count; i++)
			{
				mess_images_index[i] = imgd;
				imgd = imgd->next;
			}
		}
		else
		{
			mess_images_count = 0;
		}
    }
    else
	{
        mess_images_index = NULL;
    }

	ListView_DeleteAllItems(hwndSoftware);
	ListView_SetItemCount(hwndSoftware, mess_images_count);
	for (i = 0; i < mess_images_count; i++)
		InsertSoftwareListItem(hwndSoftware, i);

#if HAS_IDLING
    mess_idle_work = TRUE;
    s_nIdleImageNum = 0;
	s_eRealizeLevel = REALIZE_ZIPS;
	Picker_ResetIdle(hwndSoftware);
#endif /* HAS_IDLING */
}



void FillSoftwareList(HWND hwndSoftware, int nGame, int nBasePaths,
	LPCSTR *plpBasePaths, LPCSTR lpExtraPath, void (*error_proc)(const char *message))
{
	LPCSTR s;
	LPSTR *plpPaths;
	int nExtraPaths = 0;
	int nTotalPaths;
	int i;
	int nPath;
	int nChainCount;
	const struct GameDriver *drv;
	char buffer[MAX_PATH];

	/* Count the number of extra paths */
	if (lpExtraPath && *lpExtraPath)
	{
		s = lpExtraPath;
		while(s) {
			nExtraPaths++;
			s = strchr(s, ';');
			if (s)
				s++;
		}
	}

	nChainCount = 0;
	drv = drivers[nGame];
	nChainCount = mess_count_compatible_drivers(drv);

	nTotalPaths = (nBasePaths * nChainCount + nExtraPaths);

	plpPaths = (LPSTR *) alloca(nTotalPaths * sizeof(LPCSTR));
	memset(plpPaths, 0, nTotalPaths * sizeof(LPCSTR));

	/* Now fill the paths */
	nPath = 0;
	for (i = 0; i < nBasePaths; i++)
	{
		drv = drivers[nGame];
		while(drv)
		{
			/* Add default directory for system */
			snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s\\%s", plpBasePaths[i], drv->name);
			plpPaths[nPath] = alloca((strlen(buffer) + 1) * sizeof(buffer[0]));
			strcpy(plpPaths[nPath], buffer);
			nPath++;

			drv = mess_next_compatible_driver(drv);
		}
	}

	s = lpExtraPath;
	for (i = 0; i < nExtraPaths; i++)
	{
		plpPaths[nPath++] = (LPSTR) s;
		s = strchr(s, ';') + 1;
	}

	assert(nPath == nTotalPaths);

	InternalFillSoftwareList(hwndSoftware, nGame, nTotalPaths, (LPCSTR*)plpPaths,
		error_proc);
}



int MessLookupByFilename(const TCHAR *filename)
{
    int i;
	const char *this_fullname;
	size_t this_fullname_len;
	size_t filename_len = tcslen(filename);
	ZIP *zipfile;
	struct zipent *zipentry;
	BOOL good_zip;

    for (i = 0; i < mess_images_count; i++)
	{
		this_fullname = mess_images_index[i]->fullname;

		if (!tcsicmp(filename, this_fullname))
            return i;

		this_fullname_len = tcslen(this_fullname);
		if (this_fullname_len < filename_len)
		{
			if (!tcsnicmp(filename, this_fullname, this_fullname_len) && (filename[this_fullname_len] == PATH_SEPARATOR))
			{
				good_zip = FALSE;
				zipfile = openzip(FILETYPE_IMAGE, 0, T2A(this_fullname));
				if (zipfile)
				{
					zipentry = readzip(zipfile);
					if (zipentry)
					{
						if (!tcsicmp(A2T(zipentry->name), &filename[this_fullname_len + 1]))
							good_zip = TRUE;
					}
					closezip(zipfile);
				}
				if (good_zip)
					return i;
			}
		}
    }
    return -1;
}

void MessIntroduceItem(HWND hwndSoftware, const char *filename, mess_image_type *imagetypes)
{
    ImageData **pLastImageNext;
    ImageData **pOldLastImageNext;
    ImageData **pNewIndex;
    int i;

	assert(filename);
	assert(imagetypes);

    pLastImageNext = &mess_images;
    while(*pLastImageNext)
        pLastImageNext = &(*pLastImageNext)->next;
    pOldLastImageNext = pLastImageNext;

    if (!AppendNewImage(filename, REALIZE_ALL, &pLastImageNext, imagetypes))
        goto unknownsoftware;

    pNewIndex = (ImageData **) realloc(mess_images_index, (mess_images_count+1) * sizeof(ImageData *));
    if (!pNewIndex)
        goto outofmemory;
    i = mess_images_count++;
    pNewIndex[i] = (*pOldLastImageNext);
    mess_images_index = pNewIndex;

	if (!InsertSoftwareListItem(hwndSoftware, i))
		goto outofmemory;

	ListView_SetItemState(hwndSoftware, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hwndSoftware, i, FALSE);
    return;

unknownsoftware:
    MessageBox(hwndSoftware, TEXT("Unknown type of software"), TEXT(MAME32NAME), MB_OK);
    return;

outofmemory:
    MessageBox(hwndSoftware, TEXT("Out of memory"), TEXT(MAME32NAME), MB_OK);
    return;
}

BOOL MessApproveImageList(HWND hParent, int nDriver)
{
	int image_count[IO_COUNT];
	int devtype;
	int i;
	const struct IODevice *dev;
	const struct GameDriver *gamedrv;
	char buf[256];
	LPCTSTR msg;

	memset(image_count, 0, sizeof(image_count));

	for (i = 0; i < options.image_count; i++)
	{
		devtype = options.image_files[i].type;
		assert(devtype >= 0);
		assert(devtype < IO_COUNT);
		image_count[devtype]++;
	}

	gamedrv = drivers[nDriver];

	for (dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		if ((dev->flags & DEVICE_MUST_BE_LOADED) && (image_count[dev->type] != dev->count))
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "Driver requires that device %s must have an image to load\n", device_typename(dev->type));
			msg = A2T(buf);
			MessageBox(hParent, msg, NULL, MB_OK);
			return FALSE;
		}
	}
	return TRUE;
}



/* ************************************************************************ *
 * Accessors                                                                *
 * ************************************************************************ */

int MessImageCount(void)
{
	return mess_images_count;
}



int GetImageType(int nItem)
{
	const ImageData *img = mess_images_index[nItem];
	return img->dev ? img->dev->type : img->realized ? IO_BAD : IO_UNKNOWN;
}



LPCTSTR GetImageName(int nItem)
{
	return mess_images_index[nItem]->name;
}



LPCTSTR GetImageFullName(int nItem)
{
	return mess_images_index[nItem]->fullname;
}



static void TstringFromUtf8(TCHAR *buf, size_t bufsize, const char *utf8string)
{
	unicode_char_t c;
	utf16_char_t wc[UTF16_CHAR_MAX];
	int wclen, rc;

	if (!utf8string)
	{
		buf[0] = 0;
		return;
	}

	while(*utf8string)
	{
		rc = uchar_from_utf8(&c, utf8string, UTF8_CHAR_MAX);
		if (rc < 0)
		{
			c = '?';
			rc = 1;
		}
		utf8string += rc;

		wclen = utf16_from_uchar(wc, sizeof(wc) / sizeof(wc[0]), c);

		rc = WideCharToMultiByte(CP_ACP, 0, wc, wclen, buf, bufsize, NULL, NULL);
		buf += rc;
		bufsize -= rc;
	}

	/* terminate the string */
	if (bufsize == 0)
	{
		buf--;
		bufsize++;
	}
	*buf = 0;
}



/* ************************************************************************ *
 * Software picker class code                                               *
 * ************************************************************************ */

const TCHAR *SoftwarePicker_GetItemString(int nRow, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength)
{
	LPCTSTR s = NULL;
	ImageData *imgd;
#if HAS_HASH
	unsigned int hash_function = 0;
#endif /* HAS_HASH */

	imgd = mess_images_index[nRow];

    switch (nColumn) {
	case MESS_COLUMN_IMAGES:
	    s = imgd->name;
		break;

#if HAS_HASH
	case MESS_COLUMN_GOODNAME:
		TstringFromUtf8(pszBuffer, nBufferLength, imgd->hashinfo ? imgd->hashinfo->longname : NULL);
		break;

	case MESS_COLUMN_MANUFACTURER:
		TstringFromUtf8(pszBuffer, nBufferLength, imgd->hashinfo ? imgd->hashinfo->manufacturer : NULL);
		break;

	case MESS_COLUMN_YEAR:
		TstringFromUtf8(pszBuffer, nBufferLength, imgd->hashinfo ? imgd->hashinfo->year : NULL);
		break;

	case MESS_COLUMN_PLAYABLE:
		TstringFromUtf8(pszBuffer, nBufferLength, imgd->hashinfo ? imgd->hashinfo->playable : NULL);
		break;

	case MESS_COLUMN_CRC:
	case MESS_COLUMN_MD5:
	case MESS_COLUMN_SHA1:
		switch (nColumn)
		{
			case MESS_COLUMN_CRC:	hash_function = HASH_CRC;	break;
			case MESS_COLUMN_MD5:	hash_function = HASH_MD5;	break;
			case MESS_COLUMN_SHA1:	hash_function = HASH_SHA1;	break;
		}

		if (imgd->hashinfo)
			hash_data_extract_printable_checksum(imgd->hashinfo->hash, hash_function, pszBuffer);
		break;

#endif /* HAS_HASH */
	}
	return s;
}



BOOL SoftwareList_IsItemSelected(HWND hwndSoftware, int nItem)
{
	int i;

	if (mess_images_count) {
		for (i = 0; i < options.image_count; i++) {
			if (nItem == mess_image_nums[i])
				return TRUE;
		}
	}
	return FALSE;
}

BOOL SoftwareList_ItemChanged(HWND hwndSoftware, BOOL bWasSelected, BOOL bNowSelected, int nRow)
{
	int i;

    if (bWasSelected && !bNowSelected)
    {
        if (nRow >= 0) {
            if ((GetKeyState(VK_SHIFT) & 0xff00) == 0)
			{
                /* We are about to clear all images.  We have to go through
                 * and tell the other items to update */
                for (i = 0; i < options.image_count; i++)
				{
                    int imagenum = mess_image_nums[i];
                    mess_image_nums[i] = -1;
					ListView_Update(hwndSoftware, imagenum);
                }
                MessRemoveImage(-1);
            }

        }
    }

    if (!bWasSelected && bNowSelected)
    {
        /* entering item */
        MessAddImage(s_nGame, nRow);
    }
	return TRUE;
}



#if HAS_IDLING
BOOL SoftwareList_Idle(HWND hwndSoftware)
{
    static mess_image_type imagetypes[64];
    ImageData *pImageData;

	if (!mess_images_index)
		return mess_idle_work;

    if (s_nIdleImageNum == 0)
        SetupImageTypes(s_nGame, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_COUNT);

    pImageData = mess_images_index[s_nIdleImageNum];

	/* Realize one image */
    if (ImageData_Realize(pImageData, s_eRealizeLevel, imagetypes))
        ListView_RedrawItems(hwndSoftware, s_nIdleImageNum, s_nIdleImageNum);
    s_nIdleImageNum++;

    if (s_nIdleImageNum >= mess_images_count)
	{
        s_nIdleImageNum = 0;

		switch(s_eRealizeLevel) {
		case REALIZE_ZIPS:
			s_eRealizeLevel = REALIZE_ALL;
			break;

		case REALIZE_ALL:
			s_eRealizeLevel = REALIZE_DONE;
			break;

		case REALIZE_DONE:
			/* we are done */
			break;

		default:
			assert(0);
			break;
		}
		mess_idle_work = (s_eRealizeLevel != REALIZE_DONE);
    }
	return mess_idle_work;
}
#endif /* HAS_IDLING */



/* ------------------------------------------------------------------------ *
 * MESS GUI Diagnostics                                                     *
 * ------------------------------------------------------------------------ */

#ifdef MAME_DEBUG

static const char *MessGui_getfodderimage(unsigned int i, int *foddertype)
{
	if (i < mess_images_count)
	{
		*foddertype = mess_images_index[i]->dev->type;
		return mess_images_index[i]->name;
	}
	return NULL;
}

void MessTestsFlex(HWND hwndSoftware, const struct GameDriver *gamedrv)
{
	/* We get called here when we are done idling
	 *
	 * The first thing we do is we select a few arbitrary items from the list,
	 * and add them again to test item adding with MessIntroduceItem()
	 */
	int i;
	ImageData *img;
	mess_image_type imagetypes[64];

	SetupImageTypes(s_nGame, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_COUNT);

	/* Try appending an item to the list */
//	for (i = 0; i < nItemsToAdd; i++)
//	{
//		nItem = i * nItemsToAddSkip;
//		if ((nItem < mess_images_count) && (GetImageType(nItem) < IO_COUNT))
//		{
//			MessIntroduceItem(hwndSoftware, T2A(mess_images_index[nItem]->fullname), imagetypes);
//		}
//	}

	/* Assert that we have resolved all the types */
	for (i = 0; i < mess_images_count; i++)
	{
		img = mess_images_index[i];
		assert(img->realized);
	}

	/* Now lets try to see if we can load everything */
	for (i = 0; i < mess_images_count; i++)
	{
		ListView_SetItemState(hwndSoftware, i, LVIS_SELECTED, LVIS_SELECTED);
		messtestdriver(gamedrv, MessGui_getfodderimage);
	}
}

#endif /* MAME_DEBUG */
