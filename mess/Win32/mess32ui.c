#include "mame32.h"
#include "mess/mess.h"

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

#ifdef MESS_PICKER
typedef struct tagImageData {
	struct tagImageData *next;
	const char *name;
	char *fullname;
	int type;
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

static void OnMessIdle(void);

extern const char *osd_get_cwd(void);
extern void resetdir(void);
extern void osd_change_directory(const char *);
static void FillSoftwareList(int nGame);
static void InitMessPicker(void);
static void DrawMessItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
static BOOL MessPickerNotify(NMHDR *nm);
static void MessUpdateSoftwareList(void);
#else
static void MessImageConfig(HWND hMain, char *last_directory, int image);
#endif /* MESS_PICKER */

#include "win32ui.c"

/* Taken from src/mess/msdos.c */
static int requested_device_type(char *tchar)
{

	logerror("Requested device is %s\n", tchar);

	if      (!stricmp(tchar, "-cartridge")  || !stricmp(tchar, "-cart"))
			return(IO_CARTSLOT);
	else if (!stricmp(tchar, "-floppydisk") || !stricmp(tchar, "-flop"))
			return(IO_FLOPPY);
	else if (!stricmp(tchar, "-harddisk")   || !stricmp(tchar, "-hard"))
			return(IO_HARDDISK);
	else if (!stricmp(tchar, "-cassette")   || !stricmp(tchar, "-cass"))
			return(IO_CASSETTE);
	else if (!stricmp(tchar, "-printer")    || !stricmp(tchar, "-prin"))
			return(IO_PRINTER);
	else if (!stricmp(tchar, "-serial")     || !stricmp(tchar, "-serl"))
			return(IO_SERIAL);
	else if (!stricmp(tchar, "-snapshot")   || !stricmp(tchar, "-snap"))
			return(IO_SNAPSHOT);
	else if (!stricmp(tchar, "-quickload")  || !stricmp(tchar, "-quik"))
			return(IO_QUICKLOAD);
	else if (!stricmp(tchar, "-alias"))
			return(IO_ALIAS);
	/* all other switches set type to -1 */
	else
	{
        logerror("Requested Device not supported!!\n");
        return -1;
	}
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

typedef struct {
	int type;
	const char *ext;
} mess_image_type;

static void SetupImageTypes(mess_image_type *types, int count, BOOL bZip)
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
			if (num_extensions < count) {
				types[num_extensions].type = dev[i].type;
				types[num_extensions].ext = ext;
				num_extensions++;
			}
			ext += strlen(ext) + 1;
		}
	}
	types[num_extensions].type = 0;
	types[num_extensions].ext = NULL;
}

static int MessDiscoverImageType(const char *filename, mess_image_type *imagetypes)
{
	int type, i;
	char *lpExt;
	ZIP *pZip = NULL;
	
	lpExt = strrchr(filename, '.');
	type = IO_CARTSLOT;

	if (lpExt) {
		/* Are we a ZIP file? */
		if (!stricmp(lpExt, ".ZIP")) {
			pZip = openzip(filename);
			if (pZip) {
				struct zipent *pZipEnt = readzip(pZip);
				if (pZipEnt) {
					lpExt = strrchr(pZipEnt->name, '.');
				}
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
		}
	}
	options.image_count = j;
}

static void MessAddImage(int imagenum)
{
	char *filename;
	mess_image_type imagetypes[64];

	if (!mess_images_index || (imagenum >= mess_images_count))
		return;
	filename = strdup(mess_images_index[imagenum]->fullname);
	if (!filename)
		return;

	MessRemoveImage(imagenum);

	SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE);

	options.image_files[options.image_count].type = MessDiscoverImageType(filename, imagetypes);
	options.image_files[options.image_count].name = filename;
	mess_image_nums[options.image_count++] = imagenum;
}

static BOOL MessIsImageSelected(int imagenum)
{
	int i;
	for (i = 0; i < options.image_count; i++) {
		if (imagenum == mess_image_nums[i])
			return TRUE;
	}
	return FALSE;
}

/* ************************************************************************ */
/* UI                                                                       */
/* ************************************************************************ */

#ifdef MESS_PICKER

/* List view Column text */
static char *mess_column_names[MESS_COLUMN_MAX] = {
    "Image",
    "Manufacturer",
    "Year",
    "Playable"
};

static void ResetMessColumnDisplay(BOOL firstime)
{
    LV_COLUMN   lvc;
    int         i;
    int         nColumn;
    int         widths[MESS_COLUMN_MAX];
    int         order[MESS_COLUMN_MAX];
    int         shown[MESS_COLUMN_MAX];

	GetMessColumnWidths(widths);
    GetMessColumnOrder(order);
    GetMessColumnShown(shown);

    if (!firstime)
    {
        nColumn = GetNumColumns(hwndSoftware);

        // The first time thru this won't happen, on purpose
        for (i = 0; i < nColumn; i++)
        {
            widths[messRealColumn[i]] = ListView_GetColumnWidth(hwndSoftware, i);
        }

        SetColumnWidths(widths);

        for (i = nColumn; i > 0; i--)
        {
            ListView_DeleteColumn(hwndSoftware, i - 1);
        }
    }

    ListView_SetExtendedListViewStyle(hwndSoftware,
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
            ListView_InsertColumn(hwndSoftware, nColumn, &lvc);
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
        ListView_SetTextColor(hwndSoftware, RGB(240,240,240));
    else
        ListView_SetTextColor(hwndSoftware, GetListFontColor());
}

static void InitMessPicker()
{
	int order[MESS_COLUMN_MAX];
	int shown[MESS_COLUMN_MAX];
	int i;

	Header_Initialize(hwndSoftware);

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

	/* Create IconsList for ListView Control
	 *
	 * Note that CreateIcons() overrides the hLarge and hSmall globals, so we
	 * have to use this hack to preserve them
	 */
	{
		HIMAGELIST hOldLarge, hOldSmall;
		hOldLarge = hLarge;
		hOldSmall = hSmall;

		CreateIcons(hwndSoftware);

		hLarge = hOldLarge;
		hSmall = hOldSmall;
	}
	GetMessColumnOrder(messRealColumn);

	ResetMessColumnDisplay(TRUE);

	/* Allow selection to change the default saved game */
	bListReady = TRUE;
}

static int WhichMessIcon(int nItem)
{
	static const int nMessImageIcons[] = {
		1, /* IO_END */
		1, /* IO_CARTSLOT */
		4, /* IO_FLOPPY */
		9, /* IO_HARDDISK */
		5, /* IO_CASSETTE */
		8, /* IO_PRINTER */
		6, /* IO_SERIAL */
		7, /* IO_SNAPSHOT */
		7, /* IO_QUICKLOAD */
		2, /* IO_ALIAS (actually, unknowns) */
		1  /* IO_COUNT */
	};

	int type = mess_images_index[nItem]->type;
	if (type > (sizeof(nMessImageIcons) / sizeof(nMessImageIcons[0])))
		type = IO_END;

	return nMessImageIcons[type];
}

static BOOL MessPickerNotify(NMHDR *nm)
{
    NM_LISTVIEW *pnmv;
    static int nLastState = -1;
    static int nLastItem  = -1;
	int i;

    switch (nm->code)
    {
	case NM_RCLICK:
	case NM_CLICK:
		/* don't allow selection of blank spaces in the listview */
		if (!PickerHitTest(hwndSoftware))
		{
			/* we have no current game selected */
			return TRUE;
		}
		break;

    case NM_DBLCLK:
		/* Check here to make sure an item was selected */
		if (!PickerHitTest(hwndSoftware))
		{
			return TRUE;
		}
		else
			MamePlayGame();
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
				pnmv->item.pszText = (char *) mess_images_index[pnmv->item.lParam]->name;
            }
        }
        return TRUE;

    case LVN_ITEMCHANGED :
        pnmv = (NM_LISTVIEW *)nm;

        if ((pnmv->uOldState & LVIS_SELECTED) 
            && !(pnmv->uNewState & LVIS_SELECTED))
        {
            if (pnmv->lParam != -1) {
                nLastItem = pnmv->lParam;

				if ((GetKeyState(VK_SHIFT) & 0xff00) == 0) {
					/* We are about to clear all images.  We have to go through
					 * and tell the other items to update */
					for (i = 0; i < options.image_count; i++) {
						int imagenum = mess_image_nums[i];
						mess_image_nums[i] = -1;
						ListView_Update(hwndSoftware, imagenum);
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
    return FALSE;
}

int DECL_SPEC CmpImageDataPtr(const void *elem1, const void *elem2)
{
	const ImageData *img1 = *((const ImageData **) elem1);
	const ImageData *img2 = *((const ImageData **) elem2);

	return stricmp(img1->name, img2->name);
}

static void AddImagesFromDirectory(const char *dir, BOOL bRecurse, char *buffer, size_t buffersz, ImageData ***listend)
{
	ImageData *newimg;
	void *d;
	int is_dir;
	size_t pathlen;
	mess_image_type imagetypes[64];

	SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), FALSE);

	d = osd_dir_open(dir, "*.*");	
	if (d) {
		osd_change_directory(dir);

		strncpyz(buffer, osd_get_cwd(), buffersz);
		pathlen = strlen(buffer);

		while(osd_dir_get_entry(d, buffer + pathlen, buffersz - pathlen, &is_dir)) {
			if (!is_dir) {
				/* Not a directory */
				newimg = malloc(sizeof(ImageData));
				if (newimg && (newimg->fullname = strdup(buffer))) {
					newimg->name = newimg->fullname + pathlen;
					newimg->next = NULL;
					newimg->type = IO_ALIAS; /* We are using IO_ALIAS for unknowns */
					**listend = newimg;
					*listend = &newimg->next;
				}
				else if (newimg)
					free(newimg);

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

static void FillSoftwareList(int nGame)
{
	LV_ITEM lvi;
	int i;
	ImageData *imgd;
	ImageData **pimgd;
	char *olddir;
	char buffer[2000];

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
		if (imgd->name)
			free(imgd->fullname);
		free(imgd);
		imgd = next;
	}
	mess_images = NULL;

	/* Now build the linked list */
	mess_images_count = 0;
	pimgd = &mess_images;
	olddir = strdup(osd_get_cwd());
	if (olddir) {
		const struct GameDriver *drv = drivers[nGame];

		for (i = 0; i < GetMessSoftwarePathCount(); i++) {
			const char *dir = GetMessSoftwarePath(i);

			osd_change_directory(dir);
			AddImagesFromDirectory(drv->name, TRUE, buffer, sizeof(buffer), &pimgd);
			if (drv->clone_of) {
				osd_change_directory(olddir);
				osd_change_directory(dir);
				AddImagesFromDirectory(drv->clone_of->name, TRUE, buffer, sizeof(buffer), &pimgd);
			}
			osd_change_directory(olddir);
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

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; 
	lvi.stateMask = 0;
	for (i = 0; i < mess_images_count; i++) {
		lvi.iItem    = i;
		lvi.iSubItem = 0; 
		lvi.lParam   = i;
		// Do not set listview to LVS_SORTASCENDING or LVS_SORTDESCENDING
		lvi.pszText  = LPSTR_TEXTCALLBACK;
		lvi.iImage   = I_IMAGECALLBACK;
		ListView_InsertItem(hwndSoftware,&lvi);
	}
	mess_idle_work = TRUE;
	nIdleImageNum = 0;
	qsort(mess_images_index, mess_images_count, sizeof(ImageData *), CmpImageDataPtr);
}

static void MessUpdateSoftwareList(void)
{
	FillSoftwareList(nTheCurrentGame);
}

static void DrawMessItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    HDC         hDC = lpDrawItemStruct->hDC;
    RECT        rcItem = lpDrawItemStruct->rcItem;
    UINT        uiFlags = ILD_TRANSPARENT;
    HIMAGELIST  hImageList;
    int         nItem = lpDrawItemStruct->itemID;
    COLORREF    clrTextSave, clrBkSave;
    COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
    static CHAR szBuff[MAX_PATH];
    BOOL        bFocus = (GetFocus() == hwndSoftware);
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

    nColumnMax = GetNumColumns(hwndSoftware);

    if (oldControl)
    {
        GetColumnOrder(order);
    }
    else
    {
        /* Get the Column Order and save it */
        ListView_GetColumnOrderArray(hwndSoftware, nColumnMax, order);

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
            ListView_SetColumnOrderArray(hwndSoftware, nColumnMax, order);
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
    ListView_GetItem(hwndSoftware, &lvi);

    // This makes NO sense, but doesn't work without it?
    strcpy(szBuff, lvi.pszText);


	bSelected = mess_images_count
		&& ((bFocus) || (GetWindowLong(hwndSoftware, GWL_STYLE) & LVS_SHOWSELALWAYS))
		&& MessIsImageSelected(nItem);

//    bSelected = mess_images_count && (((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED)
//		&& ((bFocus) || (GetWindowLong(hwndSoftware, GWL_STYLE) & LVS_SHOWSELALWAYS)))));

    ListView_GetItemRect(hwndSoftware, nItem, &rcAllLabels, LVIR_BOUNDS);

    ListView_GetItemRect(hwndSoftware, nItem, &rcLabel, LVIR_LABEL);
    rcAllLabels.left = rcLabel.left;

    if( hBitmap != NULL )
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

        oldBitmap = SelectObject(htempDC, hBitmap);

        GetClientRect(hwndSoftware, &rcClient); 
        rcTmpBmp.right = rcClient.right;
        // We also need to check whether it is the last item
        // The update region has to be extended to the bottom if it is
        if ((mess_images_count == 0) || (nItem == ListView_GetItemCount(hwndSoftware) - 1))
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
        
        ListView_GetItemRect(hwndSoftware, 0, &rcFirstItem, LVIR_BOUNDS);
        
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
    else if( hBitmap == NULL )
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
        hImageList = ListView_GetImageList(hwndSoftware, LVSIL_STATE);
        if(hImageList)
            ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
    }

    ListView_GetItemRect(hwndSoftware, nItem, &rcIcon, LVIR_ICON);

    hImageList = ListView_GetImageList(hwndSoftware, LVSIL_SMALL);
    if(hImageList)
    {
        UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
        if(rcItem.left < rcItem.right-1)
            ImageList_DrawEx(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top,
                16, 16, GetSysColor(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
    }

    ListView_GetItemRect(hwndSoftware, nItem, &rcItem,LVIR_LABEL);

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
        ListView_GetColumn(hwndSoftware, order[nColumn] , &lvc);

        lvi.mask = LVIF_TEXT;
        lvi.iItem = nItem; 
        lvi.iSubItem = order[nColumn];
        lvi.pszText = szBuff;
        lvi.cchTextMax = sizeof(szBuff);

        if (ListView_GetItem(hwndSoftware, &lvi) == FALSE)
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

static void OnMessIdle()
{
	static mess_image_type imagetypes[64];
	ImageData *pImageData;
	int i;

	if (nIdleImageNum == 0)
		SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE);

	for (i = 0; (i < 10) && (nIdleImageNum < mess_images_count); i++) {
		pImageData = mess_images_index[nIdleImageNum];

		if (pImageData->type == IO_ALIAS) {
			pImageData->type = MessDiscoverImageType(pImageData->fullname, imagetypes);
			ListView_RedrawItems(hwndSoftware,nIdleImageNum,nIdleImageNum);
		}
		nIdleImageNum++;
	}

	if (nIdleImageNum >= mess_images_count) {
		mess_idle_work = FALSE;
		nIdleImageNum = 0;
	}
}

#else /* !MESS_PICKER */

static BOOL CommonFileImageDialog(HWND hMain, char *last_directory, common_file_dialog_proc cfd, char *filename, mess_image_type *imagetypes)
{
    BOOL success;
    OPENFILENAME of;
	char szFilter[2048];
	LPSTR s;
	int i;

	s = szFilter;

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
		strcpy(s, imagetypes[i].ext);
		s += strlen(s);
		strcpy(s, " files (*.");
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

void MessImageConfig(HWND hMain, char *last_directory, int image)
{
    INP_HEADER inpHeader;
    char filename[MAX_PATH];
    LPTREEFOLDER lpOldFolder = GetCurrentFolder();
    LPTREEFOLDER lpNewFolder = GetFolder(00);
    DWORD        dwOldFilters = 0;
    int          nOldPick = GetSelectedPickItem();

	MENUITEMINFO	im_mmi;
	int im;
    HMENU           hMenu = GetMenu(hMain);
	char buf[200];
	mess_image_type imagetypes[64];

	SetupImageTypes(imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE);

    *filename = 0;
    memset(&inpHeader,'\0', sizeof(INP_HEADER));

    if (CommonFileImageDialog(hMain, last_directory, GetOpenFileName, filename, imagetypes))
    {
		options.image_files[image].type = MessDiscoverImageType(filename, imagetypes);
		options.image_files[image].name = strdup(filename);
		if (options.image_count <= image)
			options.image_count = image + 1;
    }

	sprintf(buf,"Image %d: %s", image, filename);
	im_mmi.cbSize = sizeof(im_mmi);
	im_mmi.fMask = MIIM_TYPE;
	im_mmi.fType = MFT_STRING;
	im_mmi.dwTypeData = buf;
	im_mmi.cch = strlen(im_mmi.dwTypeData);

	switch(image) {
	case 0:
		im=ID_IMAGE0_CONFIG;
		break;
	case 1:
		im=ID_IMAGE1_CONFIG;
		break;
	case 2:
		im=ID_IMAGE2_CONFIG;
		break;
	case 3:
		im=ID_IMAGE3_CONFIG;
		break;
	}
	SetMenuItemInfo(hMenu,im,FALSE,&im_mmi);

	EnableMenuItem(hMenu, im, MF_ENABLED);
}

#endif /* MESS_PICKER */

