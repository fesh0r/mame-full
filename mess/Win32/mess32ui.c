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

/* from src/mess/win32.c */
char *strncatz(char *dest, const char *source, size_t len);
char *strncpyz(char *dest, const char *source, size_t len);
extern char crcfilename[MAX_PATH];
extern char pcrcfilename[MAX_PATH];

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

static HWND hwndSoftware;
/* Column Order as Displayed */
static BOOL messOldControl = FALSE;
static int  messRealColumn[MESS_COLUMN_MAX];
static BOOL mess_idle_work;
static UINT nIdleImageNum;
static int nTheCurrentGame;
static int *mess_icon_index;

static void *mess_crc_file;
static char mess_crc_category[16];

static void OnMessIdle(HWND hwndPicker);

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);
static void FillSoftwareList(int nGame);
static void InitMessPicker(HWND hWnd, BOOL firsttime);
static void DrawMessItem(LPDRAWITEMSTRUCT lpDrawItemStruct, HWND hWnd, HBITMAP hBackground, BOOL (*isitemselected)(int nItem));
static BOOL MessPickerNotify(HWND hWnd, NMHDR *nm, void (*runhandler)(void), BOOL (*itemchangeproc)(HWND, NM_LISTVIEW *));
static BOOL MainItemChangeProc(HWND hWnd, NM_LISTVIEW *pnmv);
static void MessUpdateSoftwareList(void);
static void MessSetPickerDefaults(void);
static void MessRetrievePickerDefaults(HWND hWnd);
static void MessOpenOtherSoftware(int iDevice);
static void MessCreateDevice(int iDevice);
static BOOL CreateMessIcons(void);
static BOOL MessIsImageSelected(int imagenum);

#define MAME32HELP "mess32.hlp"

#define IsValidListControl(hwnd)    (((hwnd) == hwndList) || ((hwnd) == hwndSoftware))

#include "win32ui.c"

struct deviceentry {
	int icon;
	const char *shortname;
	const char *longname;
	const char *dlgname;
};

static const struct deviceentry *lookupdevice(int d)
{
	/* TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape! */

	static struct deviceentry devices[] =
	{
		{ 1, "cart",	"cartridge",	"Cartridge images" },	/* IO_CARTSLOT */
		{ 4, "flop",	"floppy",		"Floppy disk images" },	/* IO_FLOPPY */
		{ 9, "hard",	"harddisk",		"Hard disk images" },	/* IO_HARDDISK */
		{ 2, "cyln",	"cylinder",		"Cylinders" },			/* IO_CYLINDER */
		{ 5, "cass",	"cassette",		"Cassette images" },	/* IO_CASSETTE */
		{ 2, "pncd",	"punchcard",	"Punchcard images" },	/* IO_PUNCHCARD */
		{ 2, "pntp",	"punchtape",	"Punchtape images" },	/* IO_PUNCHTAPE */
		{ 8, "prin",	"printer",		"Printer Output" },		/* IO_PRINTER */
		{ 6, "serl",	"serial",		"Serial Output" },		/* IO_SERIAL */
		{ 2, "parl",	"parallel",		"Parallel Output" },	/* IO_PARALLEL */
		{ 7, "snap",	"snapshot",		"Snapshots" },			/* IO_SNAPSHOT */
		{ 7, "quik",	"quickload",	"Quickloads" }			/* IO_QUICKLOAD */
	};

	assert((sizeof(devices) / sizeof(devices[0])) + 1 == IO_ALIAS);
	assert((d > 0) && (d < IO_ALIAS));

	return &devices[d - 1];
}

static int requested_device_type(char *tchar)
{
	const struct deviceentry *ent;
	int device = -1;
	int i;

    logerror("Requested device is %s\n", tchar);

	if (*tchar == '-') {
		tchar++;

		for (i = IO_END+1; i < IO_ALIAS; i++) {
			ent = lookupdevice(i);
			if (!stricmp(tchar, ent->shortname) || !stricmp(tchar, ent->longname)) {
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
            if ((type == IO_END) || (type == dev[i].type)) {
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
                /* IO_ALIAS represents uncalculated zips */
                type = IO_ALIAS;
            }
        }

        if (lpExt) {
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

static BOOL MessIsImageSelected(int imagenum)
{
    int i;

	if (mess_images_count) {
		for (i = 0; i < options.image_count; i++) {
			if (imagenum == mess_image_nums[i])
				return TRUE;
		}
	}
    return FALSE;
}

/* ************************************************************************ */
/* UI                                                                       */
/* ************************************************************************ */

/* List view Column text */
static char *mess_column_names[] = {
    "Software",
	"Goodname",
    "Manufacturer",
    "Year",
    "Playable",
	"CRC"
};

static void MessInsertPickerItem(HWND hWnd, int i)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; 
    lvi.stateMask = 0;
    lvi.iItem    = i;
    lvi.iSubItem = 0; 
    lvi.lParam   = i;
    // Do not set listview to LVS_SORTASCENDING or LVS_SORTDESCENDING
    lvi.pszText  = LPSTR_TEXTCALLBACK;
    lvi.iImage   = I_IMAGECALLBACK;
    ListView_InsertItem(hWnd,&lvi);
}

static void ResetMessColumnDisplay(BOOL firstime, HWND hWnd)
{
    LV_COLUMN   lvc;
    int         i;
    int         nColumn;
    int         widths[MESS_COLUMN_MAX];
    int         order[MESS_COLUMN_MAX];
    int         shown[MESS_COLUMN_MAX];

	assert((sizeof(mess_column_names) / sizeof(mess_column_names[0])) == MESS_COLUMN_MAX);

    GetMessColumnWidths(widths);
    GetMessColumnOrder(order);
    GetMessColumnShown(shown);

/*
    if (!firstime)
    {
        nColumn = GetNumColumns(hWnd);

        // The first time thru this won't happen, on purpose
        for (i = 0; i < nColumn; i++)
        {
            widths[messRealColumn[i]] = ListView_GetColumnWidth(hWnd, i);
        }

        SetColumnWidths(widths);

        for (i = nColumn; i > 0; i--)
        {
            ListView_DeleteColumn(hWnd, i - 1);
        }
    }
*/
    ListView_SetExtendedListViewStyle(hWnd,
        LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.fmt = LVCFMT_LEFT; 

    nColumn = 0;
    for (i = 0; i < MESS_COLUMN_MAX; i++)
    {
        if (shown[order[i]])
        {
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;

            lvc.mask |= LVCF_TEXT;
            lvc.pszText = mess_column_names[order[i]];
            lvc.iSubItem = nColumn;
            lvc.cx = widths[order[i]]; 
            ListView_InsertColumn(hWnd, nColumn, &lvc);
            messRealColumn[nColumn] = order[i];
            nColumn++;
        }
    }

    // Fill this in so we can still sort on columns NOT shown
    for (i = 0; i < MESS_COLUMN_MAX && nColumn < MESS_COLUMN_MAX; i++)
    {
        if (!shown[order[i]])
        {
            messRealColumn[nColumn] = order[i];
            nColumn++;
        }
    }

    if (GetListFontColor() == RGB(255, 255, 255))
        ListView_SetTextColor(hWnd, RGB(240,240,240));
    else
        ListView_SetTextColor(hWnd, GetListFontColor());


	if (!firstime) {
		ListView_DeleteAllItems(hWnd);
		ListView_SetItemCount(hWnd, mess_images_count);
		for (i = 0; i < mess_images_count; i++) {
			MessInsertPickerItem(hWnd, i);
		}
	}

	// Set the default software
	MessRetrievePickerDefaults(hWnd);
}

static void InitMessPicker(HWND hWnd, BOOL firsttime)
{
    int order[MESS_COLUMN_MAX];
    int shown[MESS_COLUMN_MAX];
    int i;

    Header_Initialize(hWnd);

    // Disabled column customize with old Control
    if (oldControl)
    {
        for (i = 0; i < MESS_COLUMN_MAX ; i++)
        {
            order[i] = i;
            shown[i] = TRUE;
        }
        SetMessColumnOrder(order);
        SetMessColumnShown(shown);
    }

    /* Create IconsList for ListView Control */
    ListView_SetImageList (hWnd, hSmall, LVSIL_SMALL);
    ListView_SetImageList (hWnd, hLarge, LVSIL_NORMAL);

    GetMessColumnOrder(messRealColumn);

    ResetMessColumnDisplay(firsttime, hWnd);

    /* Allow selection to change the default saved game */
    bListReady = TRUE;
}

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

    if ((nSoftwareType > IO_END) && (nSoftwareType < IO_ALIAS)) {
		iconname = lookupdevice(nSoftwareType)->shortname;
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


static int WhichMessIcon(int nItem)
{
    int nType;
    int nIcon;

    nType = mess_images_index[nItem]->type;
    
    nIcon = GetMessIcon(nTheCurrentGame, nType);
    if (!nIcon) {
		switch(nType) {
		case IO_ALIAS:
			/* Unknowns */
			nIcon = 2;
			break;
		
		case IO_COUNT:
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

static BOOL MainItemChangeProc(HWND hWnd, NM_LISTVIEW *pnmv)
{
	int i;

    if ((pnmv->uOldState & LVIS_SELECTED) 
        && !(pnmv->uNewState & LVIS_SELECTED))
    {
        if (pnmv->lParam != -1) {
            if ((GetKeyState(VK_SHIFT) & 0xff00) == 0) {
                /* We are about to clear all images.  We have to go through
                 * and tell the other items to update */
                for (i = 0; i < options.image_count; i++) {
                    int imagenum = mess_image_nums[i];
                    mess_image_nums[i] = -1;
                    ListView_Update(hWnd, imagenum);
                }
                MessRemoveImage(-1);
            }
        }
        /* leaving item */
        /* printf("leaving %s\n",drivers[pnmv->lParam]->name); */
    }

    if (!(pnmv->uOldState & LVIS_SELECTED) 
        && (pnmv->uNewState & LVIS_SELECTED))
    {
        /* entering item */
        MessAddImage(pnmv->lParam);
    }
	return TRUE;
}

static BOOL MessPickerNotify(HWND hWnd, NMHDR *nm, void (*runhandler)(void), BOOL (*itemchangeproc)(HWND, NM_LISTVIEW *))
{
    switch (nm->code)
    {
    case NM_RCLICK:
    case NM_CLICK:
        /* don't allow selection of blank spaces in the listview */
        if (!PickerHitTest(hWnd))
        {
            /* we have no current game selected */
            return TRUE;
        }
        break;

    case NM_DBLCLK:
        /* Check here to make sure an item was selected */
        if (!PickerHitTest(hWnd))
            return TRUE;
        else if (runhandler)
            runhandler();
        return TRUE;

    case LVN_GETDISPINFO:
        {
            LV_DISPINFO* pnmv = (LV_DISPINFO*)nm;
            int nItem = pnmv->item.lParam;

            if (pnmv->item.mask & LVIF_IMAGE)
            {
               pnmv->item.iImage = WhichMessIcon(nItem);
            }

            if (pnmv->item.mask & LVIF_STATE)
                pnmv->item.state = 0;

            if (pnmv->item.mask & LVIF_TEXT)
            {
				ImageData *imgd;
				const char *s;

				imgd = mess_images_index[pnmv->item.lParam];
				s = NULL;

                switch (messRealColumn[pnmv->item.iSubItem]) {
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
				}
				pnmv->item.pszText = s ? (char *) s : "";
            }
        }
        return TRUE;

    case LVN_ITEMCHANGED :
        return itemchangeproc(hWnd, (NM_LISTVIEW *)nm);
    }
    return FALSE;
}

int DECL_SPEC CmpImageDataPtr(const void *elem1, const void *elem2)
{
    const ImageData *img1 = *((const ImageData **) elem1);
    const ImageData *img2 = *((const ImageData **) elem2);

    return stricmp(img1->name, img2->name);
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
	newimg->type = IO_ALIAS;
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

static void ImageData_Realize(ImageData *img, BOOL bActive, mess_image_type *imagetypes)
{
	UINT32 crc32;
	char crcstr[9];
	char line[1024];

	if (img->type == IO_ALIAS) {
		img->type = MessDiscoverImageType(img->fullname, imagetypes, bActive, &crc32);
		if (mess_crc_file && crc32) {
			sprintf(crcstr, "%08x", crc32);
			config_load_string(mess_crc_file, mess_crc_category, 0, crcstr, line, sizeof(line));
			ImageData_SetCrcLine(img, crc32, line);
		}
	}
}

static BOOL AppendNewImage(const char *fullname, BOOL bReadZip, ImageData ***listend, mess_image_type *imagetypes)
{
    ImageData *newimg;

	newimg = ImageData_Alloc(fullname);
    if (!newimg)
        return FALSE;

	ImageData_Realize(newimg, bReadZip, imagetypes);

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
                if (AppendNewImage(buffer, FALSE, listend, imagetypes))
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

    ListView_DeleteAllItems(hwndSoftware);
    ListView_SetItemCount(hwndSoftware, mess_images_count);

    for (i = 0; i < mess_images_count; i++) {
        MessInsertPickerItem(hwndSoftware, i);
    }
    mess_idle_work = TRUE;
    nIdleImageNum = 0;
    qsort(mess_images_index, mess_images_count, sizeof(ImageData *), CmpImageDataPtr);
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

static void MessPickerSelectSoftware(HWND hWnd, const char *software, BOOL bFocus)
{
	int i;

    i = MessLookupByFilename(software);
    if (i >= 0) {
        if (bFocus) {
            ListView_SetItemState(hWnd, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            ListView_EnsureVisible(hWnd, i, FALSE);
        }
        else {
            ListView_SetItemState(hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}

static void MessRetrievePickerDefaults(HWND hWnd)
{
    char *default_software = strdup(GetDefaultSoftware());
    char *this_software;
    char *s;

    if (!default_software)
        return;

    this_software = default_software;
    while(this_software && *this_software) {
        s = strchr(this_software, '|');
        if (s)
            *(s++) = '\0';
        else
            s = NULL;

		MessPickerSelectSoftware(hWnd, this_software, this_software == default_software);

        this_software = s;
    }

    free(default_software);
}

static void DrawMessItem(LPDRAWITEMSTRUCT lpDrawItemStruct, HWND hWnd, HBITMAP hBackground, BOOL (*isitemselected)(int nItem))
{
    HDC         hDC = lpDrawItemStruct->hDC;
    RECT        rcItem = lpDrawItemStruct->rcItem;
    UINT        uiFlags = ILD_TRANSPARENT;
    HIMAGELIST  hImageList;
    int         nItem = lpDrawItemStruct->itemID;
    COLORREF    clrTextSave, clrBkSave;
    COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
    static CHAR szBuff[MAX_PATH];
    BOOL        bFocus = (GetFocus() == hWnd);
    LPCTSTR     pszText;
    UINT        nStateImageMask;
    BOOL        bSelected;
    LV_COLUMN   lvc;
    LV_ITEM     lvi;
    RECT        rcAllLabels;
    RECT        rcLabel;
    RECT        rcIcon;
    int         offset;
    SIZE        size;
    int         i;
    int         nColumn;
    int         nColumnMax = 0;
    int         nResults = 0;
    int         order[COLUMN_MAX];

    nColumnMax = GetNumColumns(hWnd);

    if (oldControl)
    {
        GetColumnOrder(order);
    }
    else
    {
        /* Get the Column Order and save it */
        ListView_GetColumnOrderArray(hWnd, nColumnMax, order);

        /* Disallow moving column 0 */
        if (order[0] != 0)
        {
            for (i = 0; i < nColumnMax; i++)
            {
                if (order[i] == 0)
                {
                    order[i] = order[0];
                    order[0] = 0;
                }
            }
            ListView_SetColumnOrderArray(hWnd, nColumnMax, order);
        }
    }

    // Labels are offset by a certain amount  
    // This offset is related to the width of a space character
    GetTextExtentPoint32(hDC, " ", 1 , &size);
    offset = size.cx * 2;

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem = nItem;
    lvi.iSubItem = order[0];
    lvi.pszText = szBuff;
    lvi.cchTextMax = sizeof(szBuff);
    lvi.stateMask = 0xFFFF;       // get all state flags
    ListView_GetItem(hWnd, &lvi);

    // This makes NO sense, but doesn't work without it?
    strcpy(szBuff, lvi.pszText);

    bSelected = ((bFocus) || (GetWindowLong(hWnd, GWL_STYLE) & LVS_SHOWSELALWAYS))
        && isitemselected(nItem);

//    bSelected = mess_images_count && (((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED)
//      && ((bFocus) || (GetWindowLong(hWnd, GWL_STYLE) & LVS_SHOWSELALWAYS)))));

    ListView_GetItemRect(hWnd, nItem, &rcAllLabels, LVIR_BOUNDS);

    ListView_GetItemRect(hWnd, nItem, &rcLabel, LVIR_LABEL);
    rcAllLabels.left = rcLabel.left;

    if( hBackground != NULL )
    {
        RECT        rcClient;
        HRGN        rgnBitmap;
        RECT        rcTmpBmp = rcItem;
        RECT        rcFirstItem;
        int         i, j;
        HPALETTE    hPAL;
        HDC         htempDC;
        HBITMAP     oldBitmap;

        htempDC = CreateCompatibleDC(hDC);

        oldBitmap = SelectObject(htempDC, hBackground);

        GetClientRect(hWnd, &rcClient); 
        rcTmpBmp.right = rcClient.right;
        // We also need to check whether it is the last item
        // The update region has to be extended to the bottom if it is
        if ((mess_images_count == 0) || (nItem == ListView_GetItemCount(hWnd) - 1))
            rcTmpBmp.bottom = rcClient.bottom;
        
        rgnBitmap = CreateRectRgnIndirect(&rcTmpBmp);
        SelectClipRgn(hDC, rgnBitmap);
        DeleteObject(rgnBitmap);

        hPAL = (! hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

        if(GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL )
        {
            SelectPalette(htempDC, hPAL, FALSE );
            RealizePalette(htempDC);
        }
        
        ListView_GetItemRect(hWnd, 0, &rcFirstItem, LVIR_BOUNDS);
        
        for( i = rcFirstItem.left; i < rcClient.right; i += bmDesc.bmWidth )
            for( j = rcFirstItem.top; j < rcClient.bottom; j += bmDesc.bmHeight )
                BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY );

        SelectObject(htempDC, oldBitmap);
        DeleteDC(htempDC);

        if (! bmDesc.bmColors)
        {
            DeleteObject(hPAL);
            hPAL = 0;
        }
    }

    SetTextColor(hDC, GetListFontColor());

    if(bSelected)
    {
        HBRUSH hBrush;
        HBRUSH hOldBrush;

        if (bFocus)
        {
            clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            clrBkSave = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
            hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
        }
        else
        {
            clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
            clrBkSave = SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
            hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        }

        hOldBrush = SelectObject(hDC, hBrush);
        FillRect(hDC, &rcAllLabels, hBrush);
        SelectObject(hDC, hOldBrush);
        DeleteObject(hBrush);
    }
    else if( hBackground == NULL )
    {
        HBRUSH hBrush;

        hBrush = CreateSolidBrush( GetSysColor(COLOR_WINDOW));
        FillRect(hDC, &rcAllLabels, hBrush);
        DeleteObject(hBrush);
    }

    if (mess_images_count == 0)
        return;

    if(lvi.state & LVIS_CUT)
    {
        clrImage = GetSysColor(COLOR_WINDOW);
        uiFlags |= ILD_BLEND50;
    }
    else if(bSelected)
    {
        if (bFocus)
            clrImage = GetSysColor(COLOR_HIGHLIGHT);
        else
            clrImage = GetSysColor(COLOR_BTNFACE);

        uiFlags |= ILD_BLEND50;
    }

    nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;

    if(nStateImageMask)
    {
        int nImage = (nStateImageMask >> 12) - 1;
        hImageList = ListView_GetImageList(hWnd, LVSIL_STATE);
        if(hImageList)
            ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
    }

    ListView_GetItemRect(hWnd, nItem, &rcIcon, LVIR_ICON);

    hImageList = ListView_GetImageList(hWnd, LVSIL_SMALL);
    if(hImageList)
    {
        UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
        if(rcItem.left < rcItem.right-1) {
			if (!hBackground) {
				HBRUSH hBrush;
				hBrush = CreateSolidBrush( GetSysColor(COLOR_WINDOW));
				FillRect(hDC, &rcIcon, hBrush);
				DeleteObject(hBrush);
			}
            ImageList_DrawEx(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top,
                16, 16, GetSysColor(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
		}
    }

    ListView_GetItemRect(hWnd, nItem, &rcItem,LVIR_LABEL);

    pszText = MakeShortString(hDC, szBuff,rcItem.right - rcItem.left,  2 * offset);

    rcLabel = rcItem;
    rcLabel.left += offset;
    rcLabel.right -= offset;

    DrawText(hDC, pszText,-1, &rcLabel,
        DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

    for(nColumn = 1; nColumn < nColumnMax ; nColumn++)
    {
        int nRetLen;
        UINT nJustify;
        LV_ITEM lvi;
        
        lvc.mask = LVCF_FMT | LVCF_WIDTH;
        ListView_GetColumn(hWnd, order[nColumn] , &lvc);

        lvi.mask = LVIF_TEXT;
        lvi.iItem = nItem; 
        lvi.iSubItem = order[nColumn];
        lvi.pszText = szBuff;
        lvi.cchTextMax = sizeof(szBuff);

        if (ListView_GetItem(hWnd, &lvi) == FALSE)
            continue;

        /* This shouldn't oughtta be, but it's needed!!! */
        strcpy(szBuff, lvi.pszText);

        rcItem.left = rcItem.right;
        rcItem.right += lvc.cx;

        nRetLen = strlen(szBuff);
        if(nRetLen == 0)
            continue;

        pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2 * offset);

        nJustify = DT_LEFT;

        if(pszText == szBuff)
        {
            switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
            {
            case LVCFMT_RIGHT:
                nJustify = DT_RIGHT;
                break;
            case LVCFMT_CENTER:
                nJustify = DT_CENTER;
                break;
            default:
                break;
            }
        }

        rcLabel = rcItem;
        rcLabel.left += offset;
        rcLabel.right -= offset;
        DrawText(hDC, pszText, -1, &rcLabel,
            nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
    }

    if(lvi.state & LVIS_FOCUSED && bFocus)
        DrawFocusRect(hDC, &rcAllLabels);

    if(bSelected)
    {
        SetTextColor(hDC, clrTextSave);
        SetBkColor(hDC, clrBkSave);
    }
}

static void OnMessIdle(HWND hwndPicker)
{
    static mess_image_type imagetypes[64];
    ImageData *pImageData;
    int i;

    if (nIdleImageNum == 0)
        SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, IO_END);

    for (i = 0; (i < 10) && (nIdleImageNum < mess_images_count); i++) {
        pImageData = mess_images_index[nIdleImageNum];

        if (pImageData->type == IO_ALIAS) {
			ImageData_Realize(pImageData, TRUE, imagetypes);
            ListView_RedrawItems(hwndPicker,nIdleImageNum,nIdleImageNum);
        }
        nIdleImageNum++;
    }

    if (nIdleImageNum >= mess_images_count) {
        mess_idle_work = FALSE;
        nIdleImageNum = 0;
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
		if (imagetypes[i].type == IO_END)
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

static void MessSetupDevice(common_file_dialog_proc cfd, int iDevice)
{
    char filename[MAX_PATH];
    LPTREEFOLDER lpOldFolder = GetCurrentFolder();
    LPTREEFOLDER lpNewFolder = GetFolder(00);
    DWORD        dwOldFilters = 0;
    int          nOldPick = GetSelectedPickItem();

    HMENU           hMenu = GetMenu(hMain);
    mess_image_type imagetypes[64];
    ImageData       **pLastImageNext;
    ImageData       **pOldLastImageNext;
    ImageData       **pNewIndex;
    int i;

    SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, iDevice);

    if (!CommonFileImageDialog(last_directory, cfd, filename, imagetypes))
        return;

    pLastImageNext = &mess_images;
    while(*pLastImageNext)
        pLastImageNext = &(*pLastImageNext)->next;
    pOldLastImageNext = pLastImageNext;

    if (!AppendNewImage(filename, TRUE, &pLastImageNext, imagetypes))
        goto unknownsoftware;

    pNewIndex = (ImageData **) realloc(mess_images_index, (mess_images_count+1) * sizeof(ImageData *));
    if (!pNewIndex)
        goto outofmemory;
    i = mess_images_count++;
    pNewIndex[i] = (*pOldLastImageNext);
    mess_images_index = pNewIndex;

    MessInsertPickerItem(hwndSoftware, i);
    ListView_SetItemState(hwndSoftware, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    ListView_EnsureVisible(hwndSoftware, i, FALSE);

    return;

unknownsoftware:
    MessageBoxA(NULL, "Unknown type of software", MAME32NAME, MB_OK);
    return;

outofmemory:
    MessageBoxA(NULL, "Out of memory", MAME32NAME, MB_OK);
    return;
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
 * New File Manager                                                         *
 *                                                                          *
 * This code implements a MESS32 specific file manger.  However, it isn't   *
 * ready for prime time so it isn't enabled by default                      *
 * ------------------------------------------------------------------------ */

static const char *s_pInitialFileName;
static BOOL s_bChosen;
static long s_lSelectedItem;

static void FileManagerItemChosen(void)
{
	s_bChosen = TRUE;
}

static BOOL FileManagerItemChangeProc(HWND hWnd, NM_LISTVIEW *pnmv)
{
    if (!(pnmv->uOldState & LVIS_SELECTED) 
        && (pnmv->uNewState & LVIS_SELECTED))
    {
        /* entering item */
        s_lSelectedItem = pnmv->lParam;
    }
	return TRUE;
}

static BOOL FileManagerIsItemSelected(int nItem)
{
	return nItem == s_lSelectedItem;
}

static void EndFileManager(HWND hDlg, long lSelectedItem)
{
	s_lSelectedItem = lSelectedItem;
	PostMessage(hDlg, WM_QUIT, 0, 0);
}

static INT_PTR CALLBACK FileManagerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bResult;
	HWND hSoftwarePicker;
	RECT rDlg, rPicker;

	hSoftwarePicker = GetDlgItem(hDlg, IDC_LIST2);

	switch(message) {
	case WM_INITDIALOG:
		CenterSubDialog(MAME32App.m_hWnd, hDlg, FALSE);

		GetClientRect(hDlg, &rDlg);

		rPicker.left	= rDlg.left		+ 10;
		rPicker.top		= rDlg.top		+ 10;
		rPicker.right	= rDlg.right	- 10;
		rPicker.bottom	= rDlg.bottom	- 10;

		SetWindowPos(hSoftwarePicker, HWND_TOP,
			rPicker.left,
			rPicker.top,
			rPicker.right - rPicker.left,
			rPicker.bottom - rPicker.top,\
			SWP_DRAWFRAME);

		s_bChosen = FALSE;
		s_lSelectedItem = -1;

		/* Initialize */
		InitMessPicker(hSoftwarePicker, FALSE);
		if (s_pInitialFileName && *s_pInitialFileName)
			MessPickerSelectSoftware(hSoftwarePicker, s_pInitialFileName, TRUE);

		ShowWindow(hDlg, TRUE);
		break;

    case WM_NOTIFY:
        /* Where is this message intended to go */
        {
            LPNMHDR lpNmHdr = (LPNMHDR)lParam;            
            if (lpNmHdr->hwndFrom == hSoftwarePicker) {
                bResult = MessPickerNotify( hSoftwarePicker, lpNmHdr, FileManagerItemChosen, FileManagerItemChangeProc );
				if (s_bChosen) {
					s_bChosen = FALSE;
					EndFileManager(hDlg, s_lSelectedItem);
				}
				return bResult;
			}
        }
        break;

    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
            switch(lpDis->CtlID) {
			case IDC_LIST2:
                DrawMessItem((LPDRAWITEMSTRUCT)lParam, hSoftwarePicker, NULL, FileManagerIsItemSelected);
                break;
            }
        }
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
			while(mess_idle_work && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				OnMessIdle(hwndSoftware);

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



