#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>

#include "mame32.h"
#include "mess/mess.h"
#include "config.h"
#include "SmartListView.h"

/* from src/mess/win32.c */
char *strncatz(char *dest, const char *source, size_t len);
char *strncpyz(char *dest, const char *source, size_t len);

static char crcfilename[MAX_PATH] = "";
static char pcrcfilename[MAX_PATH] = "";
const char *crcfile = crcfilename;
const char *pcrcfile = pcrcfilename;

/* from src/win32/Directories.c */
const char *GetMessSoftwarePath(int i);
int GetMessSoftwarePathCount(void);

static int requested_device_type(char *tchar);
static void MessSetupCrc(int game_index);

typedef struct {
    int type;
    const char *ext;
} mess_image_type;

typedef struct tagImageData {
    struct tagImageData *next;
    const char *name;
    char *fullname;
    int type;

	/* CRC info */
	char *crcline;
	int crc;
	const char *longname;
	const char *manufacturer;
	const char *year;
	const char *playable;
	const char *extrainfo;
} ImageData;

static ImageData *mess_images;
static ImageData **mess_images_index;
static int mess_images_count;

static int mess_image_nums[MAX_IMAGES];

static BOOL SoftwareListClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
static int SoftwareListClass_WhichIcon(struct SmartListView *pListView, int nItem);
static LPCSTR SoftwareListClass_GetText(struct SmartListView *pListView, int nRow, int nColumn);
static void SoftwareListClass_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
static void SoftwareListClass_SetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
static void SoftwareListClass_Run(struct SmartListView *pListView);
static BOOL SoftwareListClass_IsItemSelected(struct SmartListView *pListView, int nItem);
static BOOL SoftwareListClass_CanIdle(struct SmartListView *pListView);
static void SoftwareListClass_Idle(struct SmartListView *pListView);

static char *mess_column_names[] = {
    "Software",
	"Goodname",
    "Manufacturer",
    "Year",
    "Playable",
	"CRC"
};

static struct SmartListViewClass s_softwareListClass = 
{
	SoftwareListClass_Run,
	SoftwareListClass_ItemChanged,
	SoftwareListClass_WhichIcon,
	SoftwareListClass_GetText,
	SoftwareListClass_GetColumnInfo,
	SoftwareListClass_SetColumnInfo,
	SoftwareListClass_IsItemSelected,
	Compare_TextCaseInsensitive,
	SoftwareListClass_CanIdle,
	SoftwareListClass_Idle,
	sizeof(mess_column_names) / sizeof(mess_column_names[0]),
	mess_column_names
};

enum RealizeLevel {
	REALIZE_IMMEDIATE,	/* Calculate the file type when the extension is known */
	REALIZE_ZIPS,		/* Open up ZIPs, and calculate their extensions and CRCs */
	REALIZE_ALL			/* Open up all files, and calculate their CRCs */
};

static struct SmartListView *s_pSoftwareListView;
static BOOL mess_idle_work;
static UINT s_nIdleImageNum;
static enum RealizeLevel s_eRealizeLevel;
static int nTheCurrentGame;
static int *mess_icon_index;

static void *mess_crc_file;
static char mess_crc_category[16];

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);

static void InitMessPicker(void);
static void FillSoftwareList(int nGame);
static void MessUpdateSoftwareList(void);
static void MessSetPickerDefaults(void);
static void MessOpenOtherSoftware(int iDevice);
static void MessCreateDevice(int iDevice);
static BOOL CreateMessIcons(void);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
static void MessTestsDoneIdle(void);
#endif /* MAME_DEBUG */

#define MAME32HELP "mess32.hlp"

#define IsValidListControl(hwnd)    (((hwnd) == hwndList) || ((hwnd) == (s_pSoftwareListView->hwndListView)))

#include "win32ui.c"

struct deviceentry {
	int icon;
	const char *dlgname;
};

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

static const struct deviceentry *lookupdevice(int d)
{
	AssertValidDevice(d);
	return &s_devices[d - 1];
}

/* ------------------------------------------------------------------------ */

static int requested_device_type(char *tchar)
{
	int device = -1;
	int i;

    logerror("Requested device is %s\n", tchar);

	if (*tchar == '-') {
		tchar++;

		for (i = 1; i < IO_COUNT; i++) {
			if (!stricmp(tchar, device_typename(i)) || !stricmp(tchar, device_brieftypename(i))) {
				device = i;
				break;
			}
		}
	}

	return device;
}

static void MessSetupCrc(int game_index)
{
    const char *crcdir = "crc";

    /* Build the CRC database filename */
    sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
    if (drivers[game_index]->clone_of->name)
        sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
    else
        pcrcfilename[0] = 0;
}

/* ************************************************************************ */
/* Code for manipulation of image list                                      */
/* ************************************************************************ */

/* Specify IO_END for type if you want all types */
static void SetupImageTypes(mess_image_type *types, int count, BOOL bZip, int type)
{
    const struct IODevice *dev;
    int num_extensions = 0;
    int i;

    count--;
    dev = drivers[GetSelectedPickItem()]->dev;

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

static BOOL MessSetImage(int imagenum, int entry)
{
    char *filename;
    mess_image_type imagetypes[64];

    if (!mess_images_index || (imagenum >= mess_images_count))
        return FALSE;		/* Invalid image index */
    filename = strdup(mess_images_index[imagenum]->fullname);
    if (!filename)
        return FALSE;		/* Out of memory */

    SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

	if (options.image_files[entry].name)
		free((void *) options.image_files[entry].name);
    options.image_files[entry].type = MessDiscoverImageType(filename, imagetypes, TRUE, NULL);
    options.image_files[entry].name = filename;

    mess_image_nums[entry] = imagenum;
	return TRUE;
}

static BOOL MessAddImage(int imagenum)
{
	if (options.image_count >= MAX_IMAGES)
		return FALSE;		/* Too many images */

	MessRemoveImage(imagenum);

	if (!MessSetImage(imagenum, options.image_count))
		return FALSE;

	options.image_count++;
	return TRUE;
}

/* ************************************************************************ */
/* UI                                                                       */
/* ************************************************************************ */

static BOOL CreateMessIcons(void)
{
    int i;

    if (!mess_icon_index) {
        mess_icon_index = malloc(sizeof(int) * game_count * IO_COUNT);
        if (!mess_icon_index)
            return FALSE;
    }

    for (i = 0; i < (game_count * IO_COUNT); i++)
        mess_icon_index[i] = 0;

	if (s_pSoftwareListView)
		SmartListView_AssociateImageLists(s_pSoftwareListView, hSmall, hLarge);
    return TRUE;
}

static int GetMessIcon(int nGame, int nSoftwareType)
{
    int index;
    int nIconPos = 0;
    HICON hIcon;
    const struct GameDriver *drv;
    char buffer[32];
	const char *iconname;

    if ((nSoftwareType > IO_END) && (nSoftwareType < IO_COUNT)) {
		iconname = device_brieftypename(nSoftwareType);
        index = (nGame * IO_COUNT) + nSoftwareType;

        nIconPos = mess_icon_index[index];
        if (!nIconPos) {
            for (drv = drivers[nGame]; drv; drv = drv->clone_of) {
                sprintf(buffer, "%s/%s", drv->name, iconname);
                hIcon = LoadIconFromFile(buffer);
                if (hIcon)
                    break;
            }

            if (hIcon) {
                nIconPos = ImageList_AddIcon(hSmall, hIcon);
                ImageList_AddIcon(hLarge, hIcon);
                if (nIconPos != -1)
                    mess_icon_index[index] = nIconPos;
            }
        }
    }
    return nIconPos;
}

static ImageData *ImageData_Alloc(const char *fullname)
{
	ImageData *newimg;
    char *separator_pos;

	newimg = malloc(sizeof(ImageData) + strlen(fullname) + 1);
	if (!newimg)
		return NULL;
	memset(newimg, 0, sizeof(ImageData));

	newimg->fullname = ((char *) newimg) + sizeof(ImageData);
	strcpy(newimg->fullname, fullname);

    separator_pos = strrchr(newimg->fullname, '\\');
    newimg->name = separator_pos ? (separator_pos+1) : newimg->fullname;
	newimg->type = IO_UNKNOWN;
	return newimg;
}

static void ImageData_Free(ImageData *img)
{
	if (img->crcline)
		free(img->crcline);
	free((void *) img);
}

static BOOL ImageData_IsBad(ImageData *img)
{
	return img->type == IO_COUNT;
}

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
	img->manufacturer = strtok(NULL, "|");
	img->year = strtok(NULL, "|");
	img->playable = strtok(NULL, "|");
	img->extrainfo = strtok(NULL, "|");
	return TRUE;
}

static BOOL ImageData_Realize(ImageData *img, enum RealizeLevel eRealize, mess_image_type *imagetypes)
{
	/* from src/Win32/file.c */
	int checksum_file(const char* file, unsigned char **p, unsigned int *size, unsigned int *crc);

	UINT32 crc32 = 0;
	char crcstr[9];
	char line[1024];
	unsigned int dummy;
	BOOL bLearnedSomething = FALSE;

	/* Calculate image type */
	if (img->type == IO_UNKNOWN) {
		img->type = MessDiscoverImageType(img->fullname, imagetypes, eRealize > REALIZE_IMMEDIATE, &crc32);
		if (img->type != IO_UNKNOWN)
			bLearnedSomething = TRUE;
	}

	if ((eRealize >= REALIZE_ALL) && !crc32 && !img->crc) {
		checksum_file(img->fullname, NULL, &dummy, &crc32);
		bLearnedSomething = TRUE;
	}

	if (mess_crc_file && crc32 && !img->crc) {
		sprintf(crcstr, "%08x", crc32);
		config_load_string(mess_crc_file, mess_crc_category, 0, crcstr, line, sizeof(line));
		ImageData_SetCrcLine(img, crc32, line);
	}
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

static void AddImagesFromDirectory(const char *dir, BOOL bRecurse, char *buffer, size_t buffersz, ImageData ***listend)
{
    void *d;
    int is_dir;
    size_t pathlen;
    mess_image_type imagetypes[64];

    SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), FALSE, IO_END);

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
                AddImagesFromDirectory(buffer + pathlen, bRecurse, buffer, buffersz, listend);
                osd_change_directory("..");

                strncpyz(buffer, osd_get_cwd(), buffersz);
                pathlen = strlen(buffer);
            }
        }
        osd_dir_close(d);
    }
}

static void *OpenCrcFile(const struct GameDriver *drv, char *outname)
{
	char buffer[32];
	strcpy(outname, drv->name);
	_snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "crc\\%s.crc", drv->name);
	return config_open(buffer);
}

static void FillSoftwareList(int nGame)
{
    int i;
    ImageData *imgd;
    ImageData **pimgd;
    const char *extrapaths;
    char *olddir;
    char *s;
    char buffer[2000];

	/* Update the CRC file */
	if (mess_crc_file)
		config_close(mess_crc_file);
	mess_crc_file = OpenCrcFile(drivers[nGame], mess_crc_category);
	if (!mess_crc_file)
		mess_crc_file = OpenCrcFile(drivers[nGame]->clone_of, mess_crc_category);

    /* This fixes any changes the file manager may have introduced */
    resetdir();

    nTheCurrentGame = nGame;

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
    olddir = strdup(osd_get_cwd());
    if (olddir) {
        /* Global paths */
        for (i = 0; i < GetMessSoftwarePathCount(); i++) {
            const char *dir = GetMessSoftwarePath(i);
            const struct GameDriver *drv = drivers[nGame];

            while(drv) {
                osd_change_directory(dir);
                AddImagesFromDirectory(drv->name, TRUE, buffer, sizeof(buffer), &pimgd);
                osd_change_directory(olddir);
                drv = drv->clone_of;
            }
        }

        /* Game-specific paths */
        extrapaths = GetGameOptions(nGame)->extra_software_paths;
        while(extrapaths && *extrapaths) {
            s = strchr(extrapaths, ';');
            if (s)
                *s = '\0';

            AddImagesFromDirectory(extrapaths, TRUE, buffer, sizeof(buffer), &pimgd);

            if (s) {
                *s = ';';
                extrapaths = s + 1;
            }
            else {
                extrapaths = NULL;
            }
        }

        free(olddir);
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

	if (s_pSoftwareListView) {
		SmartListView_SetTotalItems(s_pSoftwareListView, mess_images_count);
		SmartListView_SetSorting(s_pSoftwareListView, MESS_COLUMN_IMAGES, FALSE);
	}

    mess_idle_work = TRUE;
    s_nIdleImageNum = 0;
	s_eRealizeLevel = REALIZE_ZIPS;
}

static void MessUpdateSoftwareList(void)
{
    FillSoftwareList(nTheCurrentGame);
}

static int MessLookupByFilename(const char *filename)
{
    int i;

    for (i = 0; i < mess_images_count; i++) {
        if (!strcmp(filename, mess_images_index[i]->fullname))
            return i;
    }
    return -1;
}

static void MessSetPickerDefaults(void)
{
    int i;
    size_t nDefaultSize = 0;
    char *default_software = NULL;
    char *s;

    for (i = 0; i < options.image_count; i++)
        nDefaultSize += strlen(options.image_files[i].name) + 1;
    
    if (nDefaultSize) {
        default_software = malloc(nDefaultSize);
        if (default_software) {
            s = NULL;
            for (i = 0; i < options.image_count; i++) {
                if (s)
                    *(s++) = '|';
                else
                    s = default_software;
                strcpy(s, options.image_files[i].name);
                s += strlen(s);
            }
        }
    }

    SetDefaultSoftware(default_software);

    if (default_software)
        free(default_software);
}

static void InitMessPicker(void)
{
	struct SmartListViewOptions opts;
	char *default_software;
	char *this_software;
	char *s;
	int i;

	memset(&opts, 0, sizeof(opts));
	opts.pClass = &s_softwareListClass;
	opts.hwndParent = hPicker;
	opts.nIDDlgItem = IDC_LIST2;
	opts.hBackground = hBitmap;
	opts.hPALbg = hPALbg;
	opts.bmDesc = bmDesc;
	opts.hSmall = hSmall;
	opts.hLarge = hLarge;
	opts.rgbListFontColor = GetListFontColor();
	opts.bOldControl = oldControl;
	s_pSoftwareListView = SmartListView_Init(&opts);

	SmartListView_SetTotalItems(s_pSoftwareListView, mess_images_count);
	SmartListView_SetSorting(s_pSoftwareListView, MESS_COLUMN_IMAGES, FALSE);
	Header_Initialize(s_pSoftwareListView->hwndListView);

	default_software = strdup(GetDefaultSoftware());

	if (default_software) {
		this_software = default_software;
		while(this_software && *this_software) {
			s = strchr(this_software, '|');
			if (s)
				*(s++) = '\0';
			else
				s = NULL;

			i = MessLookupByFilename(this_software);
			if (i >= 0)
				SmartListView_SelectItem(s_pSoftwareListView, i, this_software == default_software);

			this_software = s;
		}

		free(default_software);
	}
}

/* ------------------------------------------------------------------------ *
 * Open others dialog                                                       *
 * ------------------------------------------------------------------------ */

static BOOL CommonFileImageDialog(char *last_directory, common_file_dialog_proc cfd, char *filename, mess_image_type *imagetypes)
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
    for (i = 0; imagetypes[i].ext; i++) {
		if (imagetypes[i].type == IO_ZIP)
			typname = "Compressed images";
		else
			typname = lookupdevice(imagetypes[i].type)->dlgname;

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

static void MessIntroduceItem(const char *filename, mess_image_type *imagetypes)
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

	if (!SmartListView_AppendItem(s_pSoftwareListView))
		goto outofmemory;
	SmartListView_SelectItem(s_pSoftwareListView, i, TRUE);
    return;

unknownsoftware:
    MessageBoxA(NULL, "Unknown type of software", MAME32NAME, MB_OK);
    return;

outofmemory:
    MessageBoxA(NULL, "Out of memory", MAME32NAME, MB_OK);
    return;
}

static void MessSetupDevice(common_file_dialog_proc cfd, int iDevice)
{
    char filename[MAX_PATH];
    mess_image_type imagetypes[64];

    SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, iDevice);

    if (CommonFileImageDialog(last_directory, cfd, filename, imagetypes))
        MessIntroduceItem(filename, imagetypes);
}

static void MessOpenOtherSoftware(int iDevice)
{
    MessSetupDevice(GetOpenFileName, iDevice);
}

static void MessCreateDevice(int iDevice)
{
    MessSetupDevice(GetSaveFileName, iDevice);
}

/* ------------------------------------------------------------------------ *
 * Software List Class                                                      *
 * ------------------------------------------------------------------------ */

static void SoftwareListClass_Run(struct SmartListView *pListView)
{
	MamePlayGame();
}

static BOOL SoftwareListClass_IsItemSelected(struct SmartListView *pListView, int nItem)
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

static int SoftwareListClass_WhichIcon(struct SmartListView *pListView, int nItem)
{
    int nType;
    int nIcon;

    nType = mess_images_index[nItem]->type;
    
    nIcon = GetMessIcon(nTheCurrentGame, nType);
    if (!nIcon) {
		switch(nType) {
		case IO_UNKNOWN:
			/* Unknowns */
			nIcon = 2;
			break;
		
		case IO_BAD:
			/* Bad files */
			nIcon = 3;
			break;

		default:
			nIcon = lookupdevice(nType)->icon;
			break;
		}
    }
    return nIcon;
}

static LPCSTR SoftwareListClass_GetText(struct SmartListView *pListView, int nRow, int nColumn)
{
	ImageData *imgd;
	LPCSTR s = NULL;
	static char crcstr[32];

	imgd = mess_images_index[nRow];
    switch (nColumn) {
	case MESS_COLUMN_IMAGES:
	    s = imgd->name;
		break;

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
			_snprintf(crcstr, sizeof(crcstr) / sizeof(crcstr[0]), "%08x", imgd->crc);
		else
			crcstr[0] = '\0';
		s = crcstr;
		break;
	}
	return s;
}

static void SoftwareListClass_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths)
{
	if (pWidths)
		GetMessColumnWidths(pWidths);
	if (pOrder)
		GetMessColumnOrder(pOrder);
	if (pShown)
		GetMessColumnShown(pShown);
}

static void SoftwareListClass_SetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths)
{
	if (pWidths)
		SetMessColumnWidths(pWidths);
	if (pOrder)
		SetMessColumnOrder(pOrder);
	if (pShown)
		SetMessColumnShown(pShown);
}

static BOOL SoftwareListClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow)
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
        MessAddImage(nRow);

		{
            BOOL bNewScreenShot;
			const char *name;
			char *s;
			char *newname;

			name = mess_images_index[nRow]->name;
			s = strrchr(name, '\\');
			if (s)
				name = s + 1;
			newname = _alloca(strlen(name) + 1);
			strcpy(newname, name);
			s = strrchr(newname, '.');
			if (s)
				*s = '\0';

			bNewScreenShot = LoadScreenShot(GetSelectedPickItem(), newname, nPictType);
			if (bNewScreenShot || bScreenShotAvailable)
            {
                HWND hWnd;

                if (GetShowScreenShot()
                &&  (hWnd = GetDlgItem(hPicker, IDC_SSFRAME)))
                {
                    RECT    rect;
                    HWND    hParent;
                    POINT   p = {0, 0};

                    hParent = GetParent(hWnd);

                    GetWindowRect(hWnd,&rect);
                    ClientToScreen(hParent, &p);
                    OffsetRect(&rect, -p.x, -p.y);
                    InvalidateRect(hParent, &rect, FALSE);
                    UpdateWindow(hParent);
                }
            }
            bScreenShotAvailable = bNewScreenShot;
		}

    }
	return TRUE;
}

static BOOL SoftwareListClass_CanIdle(struct SmartListView *pListView)
{
	return mess_idle_work;
}

static void SoftwareListClass_Idle(struct SmartListView *pListView)
{
    static mess_image_type imagetypes[64];
    ImageData *pImageData;
    int i;

    if (s_nIdleImageNum == 0)
        SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

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

/* ------------------------------------------------------------------------ *
 * New File Manager                                                         *
 *                                                                          *
 * This code implements a MESS32 specific file manger.  However, it isn't   *
 * ready for prime time so it isn't enabled by default                      *
 * ------------------------------------------------------------------------ */

static const char *s_pInitialFileName;
static BOOL s_bChosen;
static long s_lSelectedItem;
static struct SmartListView *s_pFileMgrListView;

static void FileMgrListClass_Run(struct SmartListView *pListView)
{
	s_bChosen = TRUE;
}

static BOOL FileMgrListClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow)
{
	if (!bWasSelected && bNowSelected) {
		/* entering item */
		s_lSelectedItem = nRow;
	}
	return TRUE;
}

static BOOL FileMgrListClass_IsItemSelected(struct SmartListView *pListView, int nItem)
{
	return nItem == s_lSelectedItem;
}

static struct SmartListViewClass s_filemgrListClass = 
{
	FileMgrListClass_Run,
	FileMgrListClass_ItemChanged,
	SoftwareListClass_WhichIcon,
	SoftwareListClass_GetText,
	SoftwareListClass_GetColumnInfo,
	SoftwareListClass_SetColumnInfo,
	FileMgrListClass_IsItemSelected,
	Compare_TextCaseInsensitive,
	SoftwareListClass_CanIdle,
	SoftwareListClass_Idle,
	sizeof(mess_column_names) / sizeof(mess_column_names[0]),
	mess_column_names
};

static void EndFileManager(HWND hDlg, long lSelectedItem)
{
	s_lSelectedItem = lSelectedItem;
	PostMessage(hDlg, WM_QUIT, 0, 0);

	if (s_pFileMgrListView) {
		SmartListView_Free(s_pFileMgrListView);
		s_pFileMgrListView = NULL;
	}
}

static INT_PTR CALLBACK FileManagerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct SmartListViewOptions opts;
	int i;

	if (s_pFileMgrListView && SmartListView_IsEvent(s_pFileMgrListView, message, wParam, lParam)) {
		return SmartListView_HandleEvent(s_pFileMgrListView, message, wParam, lParam);
	}

	switch(message) {
	case WM_INITDIALOG:
		/* Initialize */
		CenterSubDialog(MAME32App.m_hWnd, hDlg, FALSE);

		s_bChosen = FALSE;
		s_lSelectedItem = -1;

		assert((sizeof(mess_column_names) / sizeof(mess_column_names[0])) == MESS_COLUMN_MAX);

		memset(&opts, 0, sizeof(opts));
		opts.pClass = &s_filemgrListClass;
		opts.hwndParent = hDlg;
		opts.nIDDlgItem = IDC_LIST2;
		opts.hBackground = NULL;
		opts.hPALbg = hPALbg;
		opts.hSmall = hSmall;
		opts.hLarge = hLarge;
		opts.rgbListFontColor = GetListFontColor();
		opts.bOldControl = oldControl;
		opts.bCenterOnParent = TRUE;
		opts.nInsetPixels = 10;

		s_pFileMgrListView = SmartListView_Init(&opts);
		if (!s_pFileMgrListView) {
			/* PANIC */
			EndFileManager(hDlg, -1);
			return FALSE;
		}

		SmartListView_SetTotalItems(s_pFileMgrListView, mess_images_count);

		if (s_pInitialFileName && *s_pInitialFileName) {
			i = MessLookupByFilename(s_pInitialFileName);
			if (i >= 0)
				SmartListView_SelectItem(s_pFileMgrListView, i, TRUE);
		}

		ShowWindow(hDlg, TRUE);
		break;

    case WM_COMMAND:
		switch(wParam) {
		case IDOK:
			EndFileManager(hDlg, s_lSelectedItem);
			break;

		case IDCANCEL:
			EndFileManager(hDlg, -1);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/* osd_select_file allows MESS32 to override the default file manager
 *
 * sel indexes an entry in options.image_files[]
 *
 * Returns:
 *	0 if the core should handle it
 *  1 if we successfully selected a file
 * -1 if we cancelled
 */
int osd_select_file(int sel, char *filename)
{
	MSG msg;
	HWND hDlg;
	int nSelectedItem;
	int result;
	BOOL bDialogDone;

	if (GetUseNewFileMgr(GetSelectedPickItem())) {
		s_pInitialFileName = filename;

		ShowCursor(TRUE);

		if (MAME32App.m_pDisplay->AllowModalDialog)
			MAME32App.m_pDisplay->AllowModalDialog(TRUE);

		hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FILEMGR), MAME32App.m_hWnd, FileManagerProc);

		bDialogDone = FALSE;
		while(!bDialogDone) {
			SmartListView_IdleUntilMsg(s_pFileMgrListView);

			do {
				if (GetMessage(&msg, NULL, 0, 0) && msg.message != WM_CLOSE) {
					if (!IsDialogMessage(hDlg, &msg)) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				else {
					bDialogDone = TRUE;
				}
			}
			while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
		}

		DestroyWindow(hDlg);
		nSelectedItem = s_lSelectedItem; //DialogBox(hInst, MAKEINTRESOURCE(IDD_FILEMGR), MAME32App.m_hWnd, FileManagerProc);

		if (MAME32App.m_pDisplay->AllowModalDialog)
			MAME32App.m_pDisplay->AllowModalDialog(FALSE);

		ShowCursor(FALSE);

		result = -1;
		if (nSelectedItem > -1) {
			strcpy(filename, mess_images_index[nSelectedItem]->fullname);
			result = 1;
		}
		else {
			result = -1;
		}
	}
	else {
		result = 0;
	}
	return result;
}

/* ------------------------------------------------------------------------ *
 * Mess32 Diagnostics                                                       *
 * ------------------------------------------------------------------------ */

#ifdef MAME_DEBUG

static int s_nOriginalPick;
static BOOL s_bRunningTests = FALSE;

static void MessTestsColumns(void)
{
	int i, j;
	int oldshown[MESS_COLUMN_MAX];
	int shown[MESS_COLUMN_MAX];

	GetMessColumnShown(oldshown);

	shown[0] = 1;
	for (i = 0; i < (1<<(MESS_COLUMN_MAX-1)); i++) {
		for (j = 1; j < MESS_COLUMN_MAX; j++)
			shown[j] = (i & (1<<(j-1))) ? 1 : 0;

		SetMessColumnShown(shown);
		SmartListView_ResetColumnDisplay(s_pSoftwareListView);
	}

	SetMessColumnShown(oldshown);
	SmartListView_ResetColumnDisplay(s_pSoftwareListView);
}

static void MessTestsBegin(void)
{
	int nOriginalPick;

	nOriginalPick = GetSelectedPick();

	/* If we are running already, keep our old defaults */
	if (!s_bRunningTests) {
		s_bRunningTests = TRUE;
		s_nOriginalPick = nOriginalPick;
	}

	MessTestsColumns();

	if (nOriginalPick == 0)
		SetSelectedPick(1);
	SetSelectedPick(0);
}

static void MessTestsCompleted(void)
{
	/* We are done */
	SetSelectedPick(s_nOriginalPick);
	s_bRunningTests = FALSE;
	MessageBoxA(hPicker, "Tests successfully completed!", MAME32NAME, MB_OK);
}

static void MessTestsFlex(void)
{
	/* We get called here when we are done idling */
	int i;
	int nItem;
	int nItemsToAdd = 5;		/* Arbitrary constant */
	int nItemsToAddSkip = 10;	/* Arbitrary constant */
	ImageData *img;
	mess_image_type imagetypes[64];

	SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

	/* Try appending an item to the list */
	for (i = 0; i < nItemsToAdd; i++) {
		nItem = i * nItemsToAddSkip;
		if ((nItem < mess_images_count) && (mess_images_index[nItem]->type != IO_COUNT)) {
			MessIntroduceItem(mess_images_index[nItem]->fullname, imagetypes);
		}
	}

	/* Assert that we have resolved all the types */
	for (i = 0; i < mess_images_count; i++) {
		img = mess_images_index[i];
		assert(img->type != IO_UNKNOWN);
	}
}

static void MessTestsDoneIdle(void)
{
	int nNewGame;

	if (s_bRunningTests) {
		nNewGame = GetSelectedPick() + 1;
		if (nNewGame >= game_count) {
			MessTestsCompleted();
		}
		else {
			MessTestsFlex();
			SetSelectedPick(nNewGame);
		}
	}
}

#endif /* MAME_DEBUG */
