#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION 1
#include <windows.h>
#include <string.h>

#include "windowsui/mame32.h"
#include "windowsui/Directories.h"
#include "mess/mess.h"
#include "mess/utils.h"

static void SoftwareDirectories_GetList(HWND hDlg, LPSTR lpBuf, UINT iBufLen);
static void SoftwareDirectories_InitList(HWND hDlg, LPCSTR lpList);
static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCSTR lpItem);
static BOOL SoftwareDirectories_OnDelete(HWND hDlg);
static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);

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


    /* Last item is placeholder for append */
    if (pItem->iItem == ListView_GetItemCount(GetDlgItem(hDlg, IDC_DIR_LIST)) - 1)
    {
        Edit_SetText(ListView_GetEditControl(GetDlgItem(hDlg, IDC_DIR_LIST)), "");
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

static INT_PTR CALLBACK GameSoftwareOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{

    switch (Msg)
    {
    case WM_INITDIALOG:
        /* Fill in the Game info at the top of the sheet */
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nGame));


		SoftwareDirectories_InitList(hDlg, pGameOpts->extra_software_paths);
		return 1;

	case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, SoftwareDirectories_OnCommand);
        return TRUE;

	case WM_NOTIFY:
        return (BOOL)HANDLE_WM_NOTIFY(hDlg, wParam, lParam, SoftwareDirectories_OnNotify);

	case WM_CLOSE:
		return 1;  //DirectoriesDialogProc(hDlg, WM_CLOSE, wParam, lParam);

	case WM_DESTROY:
		return 1;  //DirectoriesDialogProc(hDlg, WM_DESTROY, wParam, lParam);
	};

	return 0;
}

