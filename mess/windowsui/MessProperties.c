#define WIN32_LEAN_AND_MEAN

#ifndef _MSC_VER
#define NONAMELESSUNION 1
#endif

#include <windows.h>
#include <string.h>
#include <assert.h>

#include "windowsui/mame32.h"
#include "windowsui/Directories.h"
#include "mess.h"
#include "utils.h"

static void MessOptionsToProp(HWND hWnd, options_type *o);
static void MessPropToOptions(HWND hWnd, options_type *o);

static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCSTR lpItem);
static BOOL SoftwareDirectories_OnDelete(HWND hDlg);
static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);

static BOOL PropSheetFilter_Config(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv);

/* Include the actual Properties.c */
#include "../../src/windowsui/Properties.c"

extern BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
extern char *strncatz(char *dest, const char *source, size_t len);
BOOL g_bModifiedSoftwarePaths = FALSE;

static void AppendList(HWND hList, LPCSTR lpItem, int nItem)
{
    LV_ITEM Item;
	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;
	Item.pszText = (LPSTR) lpItem;
	Item.iItem = nItem;
	ListView_InsertItem(hList, &Item);
}

static void SoftwareDirectories_GetList(HWND hDlg, LPSTR lpBuf, UINT iBufLen)
{
	HWND hList;
    LV_ITEM Item;
	int iCount, i;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	if (!hList)
		return;
	memset(lpBuf, '\0', iBufLen);

	iCount = ListView_GetItemCount(hList);
	if (iCount)
		iCount--;

	memset(&Item, '\0', sizeof(Item));
	Item.mask = LVIF_TEXT;

	*lpBuf = '\0';

	for (i = 0; i < iCount; i++) {
		if (i > 0)
			strncatz(lpBuf, ";", iBufLen);

		Item.iItem = i;
		Item.pszText = lpBuf + strlen(lpBuf);
		Item.cchTextMax = iBufLen - strlen(lpBuf);
		ListView_GetItem(hList, &Item);
	}
}

static void SoftwareDirectories_InitList(HWND hDlg, LPCSTR lpList)
{
	HWND hList;
    RECT        rectClient;
    LVCOLUMN    LVCol;
	int nItem;
	int nLen;
	char buf[MAX_PATH];
	LPCSTR s;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	if (!hList)
		return;

	ListView_DeleteAllItems(hList);

	GetClientRect(hList, &rectClient);
	memset(&LVCol, 0, sizeof(LVCOLUMN));
	LVCol.mask    = LVCF_WIDTH;
	LVCol.cx      = rectClient.right - rectClient.left - GetSystemMetrics(SM_CXHSCROLL);
	ListView_InsertColumn(hList, 0, &LVCol);

	nItem = 0;
	while(*lpList) {
		s = strchr(lpList, ';');
		nLen = (s) ? (s - lpList) : (strlen(lpList));
		if (nLen >= sizeof(buf) / sizeof(buf[0]))
			nLen = (sizeof(buf) / sizeof(buf[0])) - 1;
		strncpy(buf, lpList, nLen);
		buf[nLen] = '\0';
		lpList += nLen;
		if (*lpList == ';')
			lpList++;

		AppendList(hList, buf, nItem++);
	}
	AppendList(hList, DIRLIST_NEWENTRYTEXT, nItem);

    ListView_SetItemState(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
}

static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCSTR lpItem)
{
    int nItem;
    char inbuf[MAX_PATH];
    char outbuf[MAX_PATH];
    HWND hList;
	LPSTR lpIn;

	g_bModifiedSoftwarePaths = TRUE;
	g_bUseDefaults = FALSE;

    hList = GetDlgItem(hDlg, IDC_DIR_LIST);
    nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (nItem == -1)
        return FALSE;

    /* Last item is placeholder for append */
    if (nItem == ListView_GetItemCount(hList) - 1)
    {
        bBrowse = FALSE;
    }

	if (!lpItem) {
		if (bBrowse) {
			ListView_GetItemText(hList, nItem, 0, inbuf, sizeof(inbuf) / sizeof(inbuf[0]));
			lpIn = inbuf;
		}
		else {
			lpIn = NULL;
		}

		if (!BrowseForDirectory(hDlg, lpIn, outbuf))
	        return FALSE;
		lpItem = outbuf;
	}

	AppendList(hList, lpItem, nItem);
	if (bBrowse)
		ListView_DeleteItem(hList, nItem+1);
	return TRUE;
}

static BOOL SoftwareDirectories_OnDelete(HWND hDlg)
{
    int     nCount;
    int     nSelect;
    int     nItem;
    HWND    hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	g_bModifiedSoftwarePaths = TRUE;

    nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED | LVNI_ALL);

    if (nItem == -1)
        return FALSE;

    /* Don't delete "Append" placeholder. */
    if (nItem == ListView_GetItemCount(hList) - 1)
        return FALSE;

	ListView_DeleteItem(hList, nItem);

    nCount = ListView_GetItemCount(hList);
    if (nCount <= 1)
        return FALSE;

    /* If the last item was removed, select the item above. */
    if (nItem == nCount - 1)
        nSelect = nCount - 2;
    else
        nSelect = nItem;

    ListView_SetItemState(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	return TRUE;
}

static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL          bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM*       pItem = &pDispInfo->item;
	HWND    hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	/* Last item is placeholder for append */
	if (pItem->iItem == ListView_GetItemCount(hList) - 1)
	{
		HWND hEdit = (HWND) (int) SendMessage(hList, LVM_GETEDITCONTROL, 0, 0);
		Edit_SetText(hEdit, "");
	}

	return bResult;
}

static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
    BOOL          bResult = FALSE;
    NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
    LVITEM*       pItem = &pDispInfo->item;

    if (pItem->pszText != NULL)
    {
        struct stat file_stat;

        /* Don't allow empty entries. */
        if (!strcmp(pItem->pszText, ""))
        {
            return FALSE;
        }

        /* Check validity of edited directory. */
        if (stat(pItem->pszText, &file_stat) == 0
        &&  (file_stat.st_mode & S_IFDIR))
        {
            bResult = TRUE;
        }
        else
        {
            if (MessageBox(NULL, "Directory does not exist, continue anyway?", MAME32NAME, MB_OKCANCEL) == IDOK)
                bResult = TRUE;
        }
    }

    if (bResult == TRUE)
    {
		SoftwareDirectories_OnInsertBrowse(hDlg, TRUE, pItem->pszText);
    }

    return bResult;
}

/*
static void SoftwareDirectories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_DIR_BROWSE:
        if (codeNotify == BN_CLICKED)
			SoftwareDirectories_OnInsertBrowse(hDlg, TRUE, NULL);
        break;

    case IDC_DIR_INSERT:
        if (codeNotify == BN_CLICKED)
			SoftwareDirectories_OnInsertBrowse(hDlg, FALSE, NULL);
        break;

    case IDC_DIR_DELETE:
        if (codeNotify == BN_CLICKED)
            SoftwareDirectories_OnDelete(hDlg);
        break;
    }
}
*/

static BOOL SoftwareDirectories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR)
{
    switch (id)
    {
    case IDC_DIR_LIST:
        switch (pNMHDR->code)
        {
        case LVN_ENDLABELEDIT:
            return SoftwareDirectories_OnEndLabelEdit(hDlg, pNMHDR);

        case LVN_BEGINLABELEDIT:
			return SoftwareDirectories_OnBeginLabelEdit(hDlg, pNMHDR);
        }
    }
    return FALSE;
}

static void RamSize_InitList(HWND hDlg, UINT32 default_ram)
{
	char buf[RAM_STRING_BUFLEN];
	int i, ramopt_count, sel, default_index;
	UINT32 ramopt, default_ramopt;
	HWND hRamComboBox, hRamCaption;
	const struct GameDriver *gamedrv = NULL;

	/* locate the controls */
	hRamComboBox = GetDlgItem(hDlg, IDC_RAM_COMBOBOX);
	hRamCaption = GetDlgItem(hDlg, IDC_RAM_CAPTION);
	if (!hRamComboBox || !hRamCaption)
		return;

	/* figure out how many RAM options we have */
	if (g_nGame >= 0)
	{
		/* ask the driver */
		gamedrv = drivers[g_nGame];
		ramopt_count = ram_option_count(gamedrv);
	}
	else
	{
		/* default options; in which case we are disabled */
		ramopt_count = 0;
	}

	if (ramopt_count > 0)
	{
		/* we have RAM options */
		ComboBox_ResetContent(hRamComboBox);
		default_ramopt = ram_default(gamedrv);
		default_index = sel = -1;

		for (i = 0; i < ramopt_count; i++)
		{
			ramopt = ram_option(gamedrv, i);
			ram_string(buf, ramopt);
			ComboBox_AddString(hRamComboBox, buf);
			ComboBox_SetItemData(hRamComboBox, i, ramopt);

			if (sel < 0) {
				if (default_ramopt == ramopt)
					default_index = i;
				else if (default_ram == ramopt)
					sel = i;
			}
		}
		
		if (sel < 0)
		{
			/* there doesn't seem to be a default RAM option */
			assert(default_index >= 0);
			sel = default_index;
		}
		ComboBox_SetCurSel(hRamComboBox, sel);
	}
	else
	{
		/* we do not have RAM options */
		ShowWindow(hRamComboBox, SW_HIDE);
		ShowWindow(hRamCaption, SW_HIDE);
	}
}

static void MessOptionsToProp(HWND hWnd, options_type* o)
{
	SoftwareDirectories_InitList(hWnd, o->extra_software_paths);
	RamSize_InitList(hWnd, o->ram_size);
}

static void MessPropToOptions(HWND hWnd, options_type *o)
{
	HWND hRamComboBox;
	int nIndex;

	SoftwareDirectories_GetList(hWnd, o->extra_software_paths,
		sizeof(o->extra_software_paths) / sizeof(o->extra_software_paths[0]));

	hRamComboBox = GetDlgItem(hWnd, IDC_RAM_COMBOBOX);
	if (hRamComboBox)
	{
		nIndex = ComboBox_GetCurSel(hRamComboBox);
		if (nIndex != CB_ERR)
			o->ram_size = ComboBox_GetItemData(hRamComboBox, nIndex);
	}
}

static BOOL PropSheetFilter_Config(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv)
{
	return ram_option_count(gamedrv) > 0;
}
