#include <string.h>
#include <assert.h>
#include <tchar.h>
#include "SmartListView.h"
#include "SoftwareList.h"
#include "mame.h"
#include "driver.h"
#include "unzip.h"
#include "osd_cpu.h"

#if HAS_CRC
#include "config.h"
#endif /* HAS_CRC */

#ifdef UNDER_CE
#include "mamece.h"
#else /* !UNDER_CE */
#define A2T(str)	(str)
#define T2A(str)	(str)
#define snprintf	_snprintf
#endif /* HAS_CRC */

/* from src/mess/win32.c */
char *strncatz(char *dest, const char *source, size_t len);
char *strncpyz(char *dest, const char *source, size_t len);

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);

static int s_nGame;

#ifdef _MSC_VER
#define ZEXPORT WINAPI
#else
#define ZEXPORT
#endif

extern unsigned int ZEXPORT crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

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

/* ----------------------------------------------------------------- *
 * Type declarations                                                 *
 * ----------------------------------------------------------------- */

enum RealizeLevel {
	REALIZE_IMMEDIATE,	/* Calculate the file type when the extension is known */
	REALIZE_ZIPS,		/* Open up ZIPs, and calculate their extensions and CRCs */
	REALIZE_ALL			/* Open up all files, and calculate their CRCs */
};

struct deviceentry {
	int icon;
	const char *dlgname;
};

typedef struct tagImageData {
    struct tagImageData *next;
    const TCHAR *name;
    TCHAR *fullname;
    int type;

	/* CRC info */
#if HAS_CRC
	char *crcline;
	int crc;
	const char *longname;
	const char *manufacturer;
	const char *year;
	const char *playable;
	const char *extrainfo;
#endif /* HAS_CRC */
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

#if HAS_CRC
static void *mess_crc_file;
static char mess_crc_category[64];
#endif /* HAS_CRC */

/* TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape! */
static struct deviceentry s_devices[] =
{
	{ 1, "Cartridge images" },		/* IO_CARTSLOT */
	{ 4, "Floppy disk images" },	/* IO_FLOPPY */
	{ 9, "Hard disk images" },		/* IO_HARDDISK */
	{ 2, "Cylinders" },				/* IO_CYLINDER */
	{ 5, "Cassette images" },		/* IO_CASSETTE */
	{ 2, "Punchcard images" },		/* IO_PUNCHCARD */
	{ 2, "Punchtape images" },		/* IO_PUNCHTAPE */
	{ 8, "Printer Output" },		/* IO_PRINTER */
	{ 6, "Serial Output" },			/* IO_SERIAL */
	{ 2, "Parallel Output" },		/* IO_PARALLEL */
	{ 7, "Snapshots" },				/* IO_SNAPSHOT */
	{ 7, "Quickloads" }				/* IO_QUICKLOAD */
};

static void AssertValidDevice(int d)
{
	assert((sizeof(s_devices) / sizeof(s_devices[0])) + 1 == IO_COUNT);
	assert(((d > IO_END) && (d < IO_COUNT)) || (d == IO_UNKNOWN) || (d == IO_BAD));
}

/* ************************************************************************ */
/* Code for manipulation of image list                                      */
/* ************************************************************************ */

/* Specify IO_END for type if you want all types */
void SetupImageTypes(int nDriver, mess_image_type *types, int count, BOOL bZip, int type)
{
    const struct IODevice *dev;
    int num_extensions = 0;
    int i;

    count--;
    dev = drivers[nDriver]->dev;

    if (bZip) {
        types[num_extensions].type = 0;
        types[num_extensions].ext = "zip";
        num_extensions++;
    }

    for (i = 0; dev[i].type != IO_END; i++) {
        const char *ext = dev[i].file_extensions;
        while(*ext) {
            if ((type == 0) || (type == dev[i].type)) {
                if (num_extensions < count) {
                    types[num_extensions].type = dev[i].type;
                    types[num_extensions].ext = ext;
                    num_extensions++;
                }
            }
            ext += strlen(ext) + 1;
        }
    }
    types[num_extensions].type = 0;
    types[num_extensions].ext = NULL;
}

static int MessDiscoverImageType(const char *filename, mess_image_type *imagetypes, BOOL bReadZip, UINT32 *crc32)
{
    int type, i;
    char *lpExt;
    ZIP *pZip = NULL;
	UINT32 dummy;

	if (!crc32)
		crc32 = &dummy;
    
	*crc32 = 0;
    lpExt = strrchr(filename, '.');
    type = IO_COUNT;

    if (lpExt) {
        /* Are we a ZIP file? */
        if (!stricmp(lpExt, ".ZIP")) {
            if (bReadZip) {
                pZip = openzip(filename);
                if (pZip) {
                    struct zipent *pZipEnt = readzip(pZip);
                    if (pZipEnt) {
                        lpExt = strrchr(pZipEnt->name, '.');
						*crc32 = pZipEnt->crc32;
                    }
                }
            }
            else {
                /* IO_UNKNOWN represents uncalculated zips */
                type = IO_UNKNOWN;
            }
        }

        if (lpExt && stricmp(lpExt, ".ZIP")) {
            lpExt++;
            for (i = 0; imagetypes[i].ext; i++) {
                if (!stricmp(lpExt, imagetypes[i].ext)) {
                    type = imagetypes[i].type;
                    break;
                }
            }
        }

        if (pZip)
            closezip(pZip);
    }

	if ((type != IO_UNKNOWN) && (type != IO_ZIP))
		AssertValidDevice(type);
    return type;
}

static void MessRemoveImage(int imagenum)
{
    int i, j;
    
    for (i = 0, j = 0; i < options.image_count; i++) {
        if ((imagenum >= 0) && (imagenum != mess_image_nums[i])) {
            if (i != j) {
                options.image_files[j] = options.image_files[i];
                mess_image_nums[j] = mess_image_nums[i];
            }
            j++;
        }
        else {
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

    SetupImageTypes(nDriver, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

	if (options.image_files[entry].name)
		free((void *) options.image_files[entry].name);
    options.image_files[entry].type = MessDiscoverImageType(filename, imagetypes, TRUE, NULL);
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
	newimg->type = IO_UNKNOWN;
	return newimg;
}

static void ImageData_Free(ImageData *img)
{
#if HAS_CRC
	if (img->crcline)
		free(img->crcline);
#endif
	free((void *) img);
}

static BOOL ImageData_IsBad(ImageData *img)
{
	return img->type == IO_COUNT;
}

#if HAS_CRC
static BOOL ImageData_SetCrcLine(ImageData *img, UINT32 crc, const char *crcline)
{
	char *newcrcline;

	newcrcline = strdup(crcline);
	if (!newcrcline)
		return FALSE;

	if (img->crcline)
		free(img->crcline);
	img->crcline = newcrcline;
	img->crc = crc;
	img->longname = strtok(newcrcline, "|");
	img->year = strtok(NULL, "|");
	img->manufacturer = strtok(NULL, "|");
	img->playable = strtok(NULL, "|");
	img->extrainfo = strtok(NULL, "|");
	return TRUE;
}
#endif

/* stolen from mess/windows/fileio.c */
static int checksum_file (const char *file, unsigned char **p, unsigned int *size, unsigned int *crc)
{
	int length;
	unsigned char *data;
	FILE *f;

	f = fopen (file, "rb");
	if( !f )
		return -1;

	/* determine length of file */
	if( fseek (f, 0L, SEEK_END) != 0 )
	{
		fclose (f);
		return -1;
	}

	length = ftell (f);
	if( length == -1L )
	{
		fclose (f);
		return -1;
	}

	/* allocate space for entire file */
	data = (unsigned char *) malloc (length);
	if( !data )
	{
		fclose (f);
		return -1;
	}

	/* read entire file into memory */
	if( fseek (f, 0L, SEEK_SET) != 0 )
	{
		free (data);
		fclose (f);
		return -1;
	}

	if( fread (data, sizeof (unsigned char), length, f) != length )
	{
		free (data);
		fclose (f);
		return -1;
	}

	*size = length;
	*crc = crc32 (0L, data, length);
	if( p )
		*p = data;
	else
		free (data);

	fclose (f);

	return 0;
}

static BOOL ImageData_Realize(ImageData *img, enum RealizeLevel eRealize, mess_image_type *imagetypes)
{
	UINT32 crc32 = 0;
	BOOL bLearnedSomething = FALSE;

#if HAS_CRC
	char crcstr[9];
	char line[1024];
	unsigned int dummy;
#endif

	/* Calculate image type */
	if (img->type == IO_UNKNOWN) {
		img->type = MessDiscoverImageType(T2A(img->fullname), imagetypes, eRealize > REALIZE_IMMEDIATE, &crc32);
		if (img->type != IO_UNKNOWN)
			bLearnedSomething = TRUE;
	}

#if HAS_CRC
	/* Calculate a CRC file? */
	if ((eRealize >= REALIZE_ALL) && !crc32 && !img->crc) {
		checksum_file(img->fullname, NULL, &dummy, &crc32);
		bLearnedSomething = TRUE;
	}

	/* Load CRC information? */
	if (mess_crc_file && crc32 && !img->crc) {
		sprintf(crcstr, "%08x", crc32);
		config_load_string(mess_crc_file, mess_crc_category, 0, crcstr, line, sizeof(line));
		ImageData_SetCrcLine(img, crc32, line);
	}
#endif
	return bLearnedSomething;
}

static BOOL AppendNewImage(const char *fullname, enum RealizeLevel eRealize, ImageData ***listend, mess_image_type *imagetypes)
{
    ImageData *newimg;

	newimg = ImageData_Alloc(fullname);
    if (!newimg)
        return FALSE;

	ImageData_Realize(newimg, eRealize, imagetypes);

	if (ImageData_IsBad(newimg)) {
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

    SetupImageTypes(nDriver, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), FALSE, IO_END);

    d = osd_dir_open(dir, "*.*");   
    if (d) {
        osd_change_directory(dir);

        strncpyz(buffer, osd_get_cwd(), buffersz);
        pathlen = strlen(buffer);

        while(osd_dir_get_entry(d, buffer + pathlen, buffersz - pathlen, &is_dir)) {
            if (!is_dir) {
                /* Not a directory */
                if (AppendNewImage(buffer, REALIZE_IMMEDIATE, listend, imagetypes))
                    mess_images_count++;
            }
            else if (bRecurse && strcmp(buffer + pathlen, ".") && strcmp(buffer + pathlen, "..")) {
                AddImagesFromDirectory(nDriver, buffer + pathlen, bRecurse, buffer, buffersz, listend);
                osd_change_directory("..");

                strncpyz(buffer, osd_get_cwd(), buffersz);
                pathlen = strlen(buffer);
            }
        }
        osd_dir_close(d);
    }
}

#if HAS_CRC
static void *OpenCrcFile(const struct GameDriver *drv, char *outname)
{
	char buffer[32];
	strcpy(outname, drv->name);
	snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "crc\\%s.crc", drv->name);
	return config_open(buffer);
}
#endif /* HAS_CRC */

static void InternalFillSoftwareList(struct SmartListView *pSoftwareListView, int nGame, int nPaths, LPCSTR *plpPaths)
{
    int i;
    ImageData *imgd;
    ImageData **pimgd;
    char *s;
	const char *path;
    char buffer[2000];

	s_nGame = nGame;

	/* Update the CRC file */
#if HAS_CRC
	if (mess_crc_file)
		config_close(mess_crc_file);
	mess_crc_file = OpenCrcFile(drivers[nGame], mess_crc_category);
	if (!mess_crc_file)
		mess_crc_file = OpenCrcFile(drivers[nGame]->clone_of, mess_crc_category);
#endif

    /* This fixes any changes the file manager may have introduced */
    resetdir();

    /* Remove any currently selected images */
    MessRemoveImage(-1);

    /* Free the list */
    if (mess_images_index)
        free(mess_images_index);
    imgd = mess_images;
    while(imgd) {
        ImageData *next = imgd->next;
		ImageData_Free(imgd);
        imgd = next;
    }
    mess_images = NULL;

    /* Now build the linked list */
    mess_images_count = 0;
    pimgd = &mess_images;

	for (i = 0; i < nPaths; i++) {
		/* Do we have a semicolon? */
		s = strchr(plpPaths[i], ';');
		if (s) {
			int nLen = s - plpPaths[i];
			s = malloc((nLen + 1) * sizeof(*s));
			if (!s)
				return;
			memcpy(s, plpPaths[i], nLen * sizeof(*s));
			s[nLen] = '\0';
			path = s;
		}
		else {
			path = plpPaths[i];
			s = NULL;
		}

        AddImagesFromDirectory(nGame, path, TRUE, buffer, sizeof(buffer), &pimgd);

		if (s)
			free(s);
    }

    if (mess_images_count) {
        mess_images_index = (ImageData **) malloc(sizeof(ImageData *) * mess_images_count);
        if (mess_images_index) {
            imgd = mess_images;
            for (i = 0; i < mess_images_count; i++) {
                mess_images_index[i] = imgd;
                imgd = imgd->next;
            }
        }
        else {
            mess_images_count = 0;
        }
    }
    else {
        mess_images_index = NULL;
    }

	if (pSoftwareListView) {
		SmartListView_SetTotalItems(pSoftwareListView, mess_images_count);
		SmartListView_SetSorting(pSoftwareListView, 0, FALSE);
	}

#if HAS_IDLING
    mess_idle_work = TRUE;
    s_nIdleImageNum = 0;
	s_eRealizeLevel = REALIZE_ZIPS;
#endif /* HAS_IDLING */
}

void FillSoftwareList(struct SmartListView *pSoftwareListView, int nGame, int nBasePaths, LPCSTR *plpBasePaths, LPCSTR lpExtraPath)
{
	LPCSTR s;
	LPSTR *plpPaths;
	int nExtraPaths = 0;
	int nTotalPaths;
	int i;
	int nPath;
	const char *system_dir;
	const char *parent_dir;
	char buffer[MAX_PATH];

	/* Count the number of extra paths */
	if (lpExtraPath && *lpExtraPath) {
		s = lpExtraPath;
		while(s) {
			nExtraPaths++;
			s = strchr(s, ';');
			if (s)
				s++;
		}
	}

	system_dir = drivers[nGame]->name;
	parent_dir = (drivers[nGame]->clone_of && !(drivers[nGame]->clone_of->flags & NOT_A_DRIVER)) ? drivers[nGame]->clone_of->name : NULL;

	nTotalPaths = (nBasePaths * (parent_dir ? 2 : 1) + nExtraPaths);

	plpPaths = (LPSTR *) _alloca(nTotalPaths * sizeof(LPCSTR));
	memset(plpPaths, 0, nTotalPaths * sizeof(LPCSTR));

	/* Now fill the paths */
	nPath = 0;
	for (i = 0; i < nBasePaths; i++) {
		/* Add default directory for system */
		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s\\%s", plpBasePaths[i], system_dir);
		plpPaths[nPath] = _alloca((strlen(buffer) + 1) * sizeof(buffer[0]));
		strcpy(plpPaths[nPath], buffer);
		nPath++;

		/* If there is a parent, add that directory also */
		if (parent_dir) {
			snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s\\%s", plpBasePaths[i], parent_dir);
			plpPaths[nPath] = _alloca((strlen(buffer) + 1) * sizeof(buffer[0]));
			strcpy(plpPaths[nPath], buffer);
			nPath++;
		}
	}

	s = lpExtraPath;
	for (i = 0; i < nExtraPaths; i++) {
		plpPaths[nPath++] = (LPSTR) s;
		s = strchr(s, ';') + 1;
	}

	assert(nPath == nTotalPaths);

	InternalFillSoftwareList(pSoftwareListView, nGame, nTotalPaths, plpPaths);
}

int MessLookupByFilename(const TCHAR *filename)
{
    int i;

    for (i = 0; i < mess_images_count; i++) {
        if (!_tcscmp(filename, mess_images_index[i]->fullname))
            return i;
    }
    return -1;
}

void MessIntroduceItem(struct SmartListView *pListView, const char *filename, mess_image_type *imagetypes)
{
    ImageData       **pLastImageNext;
    ImageData       **pOldLastImageNext;
    ImageData       **pNewIndex;
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

	if (!SmartListView_AppendItem(pListView))
		goto outofmemory;
	SmartListView_SelectItem(pListView, i, TRUE);
    return;

unknownsoftware:
    MessageBox(NULL, TEXT("Unknown type of software"), TEXT(MAME32NAME), MB_OK);
    return;

outofmemory:
    MessageBox(NULL, TEXT("Out of memory"), TEXT(MAME32NAME), MB_OK);
    return;
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
	return mess_images_index[nItem]->type;
}

LPCTSTR GetImageName(int nItem)
{
	return mess_images_index[nItem]->name;
}

LPCTSTR GetImageFullName(int nItem)
{
	return mess_images_index[nItem]->fullname;
}

/* ************************************************************************ *
 * SoftwareListView class code                                              *
 * ************************************************************************ */

enum {
	MESS_COLUMN_IMAGES,
	MESS_COLUMN_GOODNAME,
	MESS_COLUMN_MANUFACTURER,
	MESS_COLUMN_YEAR,
	MESS_COLUMN_PLAYABLE,
	MESS_COLUMN_CRC,
	MESS_COLUMN_MAX
};

LPCTSTR SoftwareList_GetText(struct SmartListView *pListView, int nRow, int nColumn)
{
	LPCTSTR s;
	ImageData *imgd;
#if HAS_CRC
	static char crcstr[9];
#endif

	imgd = mess_images_index[nRow];

    switch (nColumn) {
	case MESS_COLUMN_IMAGES:
	    s = imgd->name;
		break;

#if HAS_CRC
	case MESS_COLUMN_GOODNAME:
		s = imgd->longname;
		break;

	case MESS_COLUMN_MANUFACTURER:
		s = imgd->manufacturer;
		break;

	case MESS_COLUMN_YEAR:
		s = imgd->year;
		break;

	case MESS_COLUMN_PLAYABLE:
		s = imgd->playable;
		break;

	case MESS_COLUMN_CRC:
		if (imgd->crc)
			snprintf(crcstr, sizeof(crcstr) / sizeof(crcstr[0]), "%08x", imgd->crc);
		else
			crcstr[0] = '\0';
		s = crcstr;
		break;
#endif /* HAS_CRC */
	}
	return s;
}

BOOL SoftwareList_IsItemSelected(struct SmartListView *pListView, int nItem)
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

BOOL SoftwareList_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow)
{
	int i;

    if (bWasSelected && !bNowSelected)
    {
        if (nRow >= 0) {
            if ((GetKeyState(VK_SHIFT) & 0xff00) == 0) {
                /* We are about to clear all images.  We have to go through
                 * and tell the other items to update */
                for (i = 0; i < options.image_count; i++) {
                    int imagenum = mess_image_nums[i];
                    mess_image_nums[i] = -1;
                    SmartListView_Update(pListView, imagenum);
                }
                MessRemoveImage(-1);
            }

        }
        /* leaving item */
        /* printf("leaving %s\n",drivers[pnmv->lParam]->name); */
    }

    if (!bWasSelected && bNowSelected)
    {
        /* entering item */
        MessAddImage(s_nGame, nRow);
    }
	return TRUE;
}

#if HAS_IDLING
BOOL SoftwareList_CanIdle(struct SmartListView *pListView)
{
	return mess_idle_work;
}

void SoftwareList_Idle(struct SmartListView *pListView)
{
    static mess_image_type imagetypes[64];
    ImageData *pImageData;
    int i;

    if (s_nIdleImageNum == 0)
        SetupImageTypes(s_nGame, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

    for (i = 0; (i < 10) && (s_nIdleImageNum < mess_images_count); i++) {
        pImageData = mess_images_index[s_nIdleImageNum];

        if (ImageData_Realize(pImageData, s_eRealizeLevel, imagetypes))
            SmartListView_RedrawItem(pListView, s_nIdleImageNum);
        s_nIdleImageNum++;
    }

    if (s_nIdleImageNum >= mess_images_count) {
        s_nIdleImageNum = 0;

		switch(s_eRealizeLevel) {
		case REALIZE_ZIPS:
			s_eRealizeLevel = REALIZE_ALL;
			break;

		case REALIZE_ALL:
			s_eRealizeLevel = REALIZE_ZIPS;
			mess_idle_work = FALSE;
			break;

		default:
			assert(0);
			break;
		}
    }
}
#endif /* HAS_IDLING */

/* ------------------------------------------------------------------------ *
 * Mess32 Diagnostics                                                       *
 * ------------------------------------------------------------------------ */

#ifdef MAME_DEBUG

void MessTestsFlex(struct SmartListView *pListView)
{
	/* We get called here when we are done idling */
	int i;
	int nItem;
	int nItemsToAdd = 5;		/* Arbitrary constant */
	int nItemsToAddSkip = 10;	/* Arbitrary constant */
	ImageData *img;
	mess_image_type imagetypes[64];

	SetupImageTypes(s_nGame, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

	/* Try appending an item to the list */
	for (i = 0; i < nItemsToAdd; i++) {
		nItem = i * nItemsToAddSkip;
		if ((nItem < mess_images_count) && (mess_images_index[nItem]->type != IO_COUNT)) {
			MessIntroduceItem(pListView, mess_images_index[nItem]->fullname, imagetypes);
		}
	}

	/* Assert that we have resolved all the types */
	for (i = 0; i < mess_images_count; i++) {
		img = mess_images_index[i];
		assert(img->type != IO_UNKNOWN);
	}
}

#endif /* MAME_DEBUG */
