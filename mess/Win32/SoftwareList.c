#include <string.h>
#include <assert.h>
#include "SmartListView.h"
#include "SoftwareList.h"
#include "mame.h"
#include "driver.h"
#include "unzip.h"
#include "osd_cpu.h"

#ifdef UNDER_CE
#include "mamece.h"
#define HAS_CRC 0
#else /* !UNDER_CE */
#include "config.h"
#define HAS_CRC 1
#endif /* HAS_CRC */

typedef struct {
    int type;
    const char *ext;
} mess_image_type;

/* from src/mess/win32.c */
char *strncatz(char *dest, const char *source, size_t len);
char *strncpyz(char *dest, const char *source, size_t len);

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);

static int s_nGame;

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

/* ----------------------------------------------------------------- */

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

static ImageData *mess_images;
static ImageData **mess_images_index;
static int mess_images_count;

static int mess_image_nums[MAX_IMAGES];

enum RealizeLevel {
	REALIZE_IMMEDIATE,	/* Calculate the file type when the extension is known */
	REALIZE_ZIPS,		/* Open up ZIPs, and calculate their extensions and CRCs */
	REALIZE_ALL			/* Open up all files, and calculate their CRCs */
};

static BOOL mess_idle_work;
static UINT s_nIdleImageNum;
static enum RealizeLevel s_eRealizeLevel;

struct deviceentry {
	int icon;
	const char *dlgname;
};

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
static void SetupImageTypes(int nDriver, mess_image_type *types, int count, BOOL bZip, int type)
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

static BOOL ImageData_Realize(ImageData *img, enum RealizeLevel eRealize, mess_image_type *imagetypes)
{
	/* from src/Win32/file.c */
	int checksum_file(const char* file, unsigned char **p, unsigned int *size, unsigned int *crc);

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

#if HAVE_CRC
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

    mess_idle_work = TRUE;
    s_nIdleImageNum = 0;
	s_eRealizeLevel = REALIZE_ZIPS;
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
	if (lpExtraPath) {
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

/* ************************************************************************ *
 * SoftwareListView class code                                              *
 * ************************************************************************ */

LPCTSTR SoftwareList_GetText(struct SmartListView *pListView, int nRow, int nColumn)
{
	ImageData *imgd;

	imgd = mess_images_index[nRow];

	return imgd->name;
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

