#include <assert.h>
#include <tchar.h>
#include "SmartListView.h"
#include "resource.h"

#if HAS_COLUMNEDIT
#include "ColumnEdit.h"
#endif

static void SmartListView_InternalResetColumnDisplay(struct SmartListView *pListView, BOOL bFirstTime);
static int SmartListView_InsertItem(struct SmartListView *pListView, int nItem);
static void SmartListView_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);

/* Add ... to Items in ListView if needed */
static LPCTSTR MakeShortString(HDC hDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
    static const TCHAR szThreeDots[] = TEXT("...");
    static TCHAR szShort[MAX_PATH];
    int nStringLen=lstrlen(lpszLong);
    int nAddLen;
    SIZE size;
    int i;

    GetTextExtentPoint32(hDC,lpszLong, nStringLen, &size);
    if(nStringLen==0 || size.cx + nOffset<=nColumnLen)
        return lpszLong;

    lstrcpy(szShort,lpszLong);
    GetTextExtentPoint32(hDC, szThreeDots, sizeof(szThreeDots) / sizeof(szThreeDots), &size);
    nAddLen=size.cx;

    for( i = nStringLen-1; i > 0; i--)
    {
        szShort[i]=0;
        GetTextExtentPoint32(hDC, szShort,i, &size);
        if(size.cx + nOffset + nAddLen <= nColumnLen)
            break;
    }

    lstrcat(szShort,szThreeDots);

    return szShort;
}
	
static BOOL PickerHitTest(HWND hWnd)
{
	RECT            rect;
	POINTS          p;
	DWORD           res = GetMessagePos();
	LVHITTESTINFO   htInfo;

	memset(&htInfo,'\0', sizeof(LVHITTESTINFO));
	p = *((POINTS *) &res);
	GetWindowRect(hWnd, &rect);
	htInfo.pt.x = p.x - rect.left;
	htInfo.pt.y = p.y - rect.top;
	ListView_HitTest(hWnd, &htInfo);

	return !(htInfo.flags & LVHT_NOWHERE);
}

static int GetNumColumns(struct SmartListView *pListView)
{
	int  nColumn = 0;
	int  i;
	HWND hwndHeader;
	int  *shown;

	shown = _alloca(pListView->pClass->nNumColumns * sizeof(int));
	SmartListView_GetColumnInfo(pListView, shown, NULL, NULL);
	hwndHeader = GetDlgItem(pListView->hwndListView, 0);

	if ((nColumn = Header_GetItemCount(hwndHeader)) < 1) {
		nColumn = 0;
		for (i = 0; i < pListView->pClass->nNumColumns; i++ ) {
			if (shown[i])
				nColumn++;
		}
	}
	return nColumn;
}

struct SmartListView *SmartListView_Init(struct SmartListViewOptions *pOptions)
{
	HWND hwndListView;
	struct SmartListView *pListView;
	RECT rParent, rListView;
	size_t nObjectSize;

	assert(pOptions);
	assert(pOptions->pClass);
	assert(pOptions->hwndParent);
	assert(pOptions->pClass->nNumColumns > 0);
	
	hwndListView = GetDlgItem(pOptions->hwndParent, pOptions->nIDDlgItem);
	assert(hwndListView);

	/* Figure out size of object */
	nObjectSize = pOptions->pClass->nObjectSize;
	if (nObjectSize == 0)
		nObjectSize = sizeof(struct SmartListView);

	/* Allocate it */
	pListView = malloc(nObjectSize);
	if (!pListView)
		return NULL;
	memset(pListView, 0, nObjectSize);

	/* Allocate column lists */
	pListView->piRealColumns = malloc(pOptions->pClass->nNumColumns * sizeof(int));
	if (!pListView->piRealColumns) {
		free(pListView);
		return NULL;
	}

	pListView->hwndListView = hwndListView;
	pListView->pClass = pOptions->pClass;
	pListView->hwndParent = pOptions->hwndParent;
	pListView->nIDDlgItem = pOptions->nIDDlgItem;
	pListView->hBackground = pOptions->hBackground;
	pListView->hPALbg = pOptions->hPALbg;
#if HAS_MYBITMAPINFO
	pListView->bmDesc = pOptions->bmDesc;
#endif
	pListView->bOldControl = pOptions->bOldControl;
	pListView->rgbListFontColor = pOptions->rgbListFontColor;
	pListView->nSortCondition = 0;
	pListView->nNumRows = 0;
	pListView->rowMapping = NULL;

	/* Do we automatically center on our parent? */
	if (pOptions->bCenterOnParent) {
		GetClientRect(pOptions->hwndParent, &rParent);

		rListView.left		= rParent.left		+ pOptions->nInsetPixels;
		rListView.top		= rParent.top		+ pOptions->nInsetPixels;
		rListView.right		= rParent.right		- pOptions->nInsetPixels;
		rListView.bottom	= rParent.bottom	- pOptions->nInsetPixels;

		SetWindowPos(hwndListView, HWND_TOP,
			rListView.left,
			rListView.top,
			rListView.right - rListView.left,
			rListView.bottom - rListView.top,
			SWP_DRAWFRAME);
	}

	SmartListView_AssociateImageLists(pListView, pOptions->hSmall, pOptions->hLarge);
	SmartListView_InternalResetColumnDisplay(pListView, TRUE);
	return pListView;
}

void SmartListView_Free(struct SmartListView *pListView)
{
	if (pListView->rowMapping)
		free(pListView->rowMapping);
	free(pListView->piRealColumns);
	free(pListView);
}

/* ------------------------------------------------------------------------ *
 * Liaison functions; for invoking callbacks                                *
 * ------------------------------------------------------------------------ */

static int SmartListView_LogicalRowToVisual(struct SmartListView *pListView, int nLogicalRow)
{
	assert(nLogicalRow >= 0);
	assert(nLogicalRow < pListView->nNumRows);
	return pListView->rowMapping[nLogicalRow].nLogicalToVisual;
}

static int SmartListView_VisualRowToLogical(struct SmartListView *pListView, int nVisualRow)
{
	assert(nVisualRow >= 0);
	assert(nVisualRow < pListView->nNumRows);
	return pListView->rowMapping[nVisualRow].nVisualToLogical;
}

static int SmartListView_VisualColumnToLogical(struct SmartListView *pListView, int nVisualColumn)
{
	assert(nVisualColumn >= 0);
	assert(nVisualColumn < pListView->pClass->nNumColumns);
	return pListView->piRealColumns[nVisualColumn];
}

static const TCHAR *SmartListView_GetText(struct SmartListView *pListView, int nLogicalRow, int nVisualColumn)
{
	int nLogicalColumn;
	LPCTSTR s = NULL;

	if (pListView->pClass->pfnGetText) {
		nLogicalColumn = SmartListView_VisualColumnToLogical(pListView, nVisualColumn);
		s = pListView->pClass->pfnGetText(pListView, nLogicalRow, nLogicalColumn);
	}
	return s ? s : TEXT("");
}

static BOOL SmartListView_IsItemSelected(struct SmartListView *pListView, int nVisualRow)
{
	int nLogicalRow;
	BOOL bSelected;

	if (pListView->pClass->pfnIsItemSelected) {
		nLogicalRow = SmartListView_VisualRowToLogical(pListView, nVisualRow);
		bSelected = pListView->pClass->pfnIsItemSelected(pListView, nLogicalRow);
	}
	else {
		bSelected = FALSE;
	}
	return bSelected;
}

static BOOL SmartListView_ItemChanged(struct SmartListView *pListView, NM_LISTVIEW *lpNmHdr)
{
	BOOL bWasSelected, bNowSelected;
	int nLogicalRow;

	if (!pListView->pClass->pfnItemChanged)
		return FALSE;

	bWasSelected = (lpNmHdr->uOldState & LVIS_SELECTED) ? TRUE : FALSE;
	bNowSelected = (lpNmHdr->uNewState & LVIS_SELECTED) ? TRUE : FALSE;
	nLogicalRow = lpNmHdr->lParam;

	return pListView->pClass->pfnItemChanged(pListView, bWasSelected, bNowSelected, nLogicalRow);
}

static int SmartListView_WhichIcon(struct SmartListView *pListView, int nLogicalRow)
{
	int nIcon;

	if (pListView->pClass->pfnWhichIcon) {
		nIcon = pListView->pClass->pfnWhichIcon(pListView, nLogicalRow);
	}
	else {
		nIcon = 0;
	}
	return nIcon;
}

static void SmartListView_GetColumnInfo(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths)
{
	int i;
	RECT r;
	int nWidth;

	if (pListView->pClass->pfnGetColumnInfo) {
		pListView->pClass->pfnGetColumnInfo(pListView, pShown, pOrder, pWidths);
	}
	else {
		if (pWidths) {
			GetClientRect(pListView->hwndListView, &r);
			nWidth = r.right - r.left;
		}

		/* No default GetColumnInfo function? */
		for (i = 0; i < pListView->pClass->nNumColumns; i++) {
			if (pShown)
				pShown[i] = 1;
			if (pOrder)
				pOrder[i] = i;
			if (pWidths)
				pWidths[i] = nWidth;
		}
	}
}

/* ------------------------------------------------------------------------ *
 * ColumnEdit Dialog                                                        *
 * ------------------------------------------------------------------------ */
#if HAS_COLUMNEDIT

static struct SmartListView *MyColumnDialogProc_pListView;
static int *MyColumnDialogProc_order;
static int *MyColumnDialogProc_shown;

static void MyGetRealColumnOrder(int *order)
{
	SmartListView_GetRealColumnOrder(MyColumnDialogProc_pListView, order);
}

static void MyGetColumnInfo(int *order, int *shown)
{
	MyColumnDialogProc_pListView->pClass->pfnGetColumnInfo(MyColumnDialogProc_pListView, shown, order, NULL);
}

static void MySetColumnInfo(int *order, int *shown)
{
	MyColumnDialogProc_pListView->pClass->pfnSetColumnInfo(MyColumnDialogProc_pListView, shown, order, NULL);
}

static INT_PTR CALLBACK MyColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return InternalColumnDialogProc(hDlg, Msg, wParam, lParam, MyColumnDialogProc_pListView->pClass->nNumColumns,
		MyColumnDialogProc_shown, MyColumnDialogProc_order, (char **) MyColumnDialogProc_pListView->pClass->ppColumnNames,
		MyGetRealColumnOrder, MyGetColumnInfo, MySetColumnInfo);
}

static BOOL RunColumnDialog(struct SmartListView *pListView)
{
	BOOL bResult;

	MyColumnDialogProc_pListView = pListView;
	MyColumnDialogProc_order = malloc(pListView->pClass->nNumColumns * sizeof(int));
	MyColumnDialogProc_shown = malloc(pListView->pClass->nNumColumns * sizeof(int));
	bResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), pListView->hwndParent, MyColumnDialogProc);
	free(MyColumnDialogProc_order);
	free(MyColumnDialogProc_shown);
	return bResult;
}

#endif /* HAS_COLUMNEDIT */
/* ------------------------------------------------------------------------ *
 * Event Handling                                                           *
 * ------------------------------------------------------------------------ */

BOOL SmartListView_IsEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam)
{
	BOOL bIsEvent = FALSE;
	LPNMHDR lpNmHdr;
	LPDRAWITEMSTRUCT lpDis;

	switch(message) {
    case WM_NOTIFY:
		lpNmHdr = (LPNMHDR)lParam;
		bIsEvent = lpNmHdr->hwndFrom == pListView->hwndListView;
		break;

	case WM_DRAWITEM:
		lpDis = (LPDRAWITEMSTRUCT)lParam;
		bIsEvent = lpDis->CtlID == (UINT)pListView->nIDDlgItem;
		break;

#if HAS_CONTEXTMENU
	case WM_CONTEXTMENU:
		bIsEvent = (HWND)wParam == pListView->hwndListView;
		break;
#endif /* HAS_CONTEXTMENU */
	}

	return bIsEvent;
}

#if HAS_CONTEXTMENU
static void SmartListView_HandleContextMenu(struct SmartListView *pListView, POINT ptScreen)
{
	HWND hwndHeader;
	HMENU hMenu, hMenuLoad;
	int i, nMenuItem, nLogicalColumn;
	BOOL bFound = FALSE;
	RECT rcCol;
	POINT ptClient;

	hwndHeader = GetDlgItem(pListView->hwndListView, 0);

	ptClient = ptScreen;
	ScreenToClient(hwndHeader, &ptClient);

	for (i = 0; Header_GetItemRect(hwndHeader, i, &rcCol); i++ ) {
		if (PtInRect(&rcCol, ptClient)) {
			nLogicalColumn = SmartListView_VisualColumnToLogical(pListView, i);
			bFound = TRUE;
		}
	}

	if (bFound) {
		hMenuLoad = LoadMenu(NULL, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
		hMenu = GetSubMenu(hMenuLoad, 0);

		nMenuItem = (int) TrackPopupMenu(hMenu,
			TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
			ptScreen.x,
			ptScreen.y,
			0,
			pListView->hwndParent,
			NULL);

		DestroyMenu(hMenuLoad);

		switch(nMenuItem) {
		case ID_SORT_ASCENDING:
			SmartListView_SetSorting(pListView, nLogicalColumn, FALSE);
			break;
		case ID_SORT_DESCENDING:
			SmartListView_SetSorting(pListView, nLogicalColumn, TRUE);
			break;
		case ID_CUSTOMIZE_FIELDS:
			if (RunColumnDialog(pListView))
				SmartListView_ResetColumnDisplay(pListView);
			break;
		}
	}
}
#endif

static BOOL SmartListView_HandleNotify(struct SmartListView *pListView, LPNMHDR lpNmHdr)
{
	BOOL bReturn = FALSE;
	LV_DISPINFO *pLvDispinfo;
	NM_LISTVIEW *pNmListview;
	int nItem, nColumn;

	switch (lpNmHdr->code) {
	case NM_RCLICK:
	case NM_CLICK:
		/* don't allow selection of blank spaces in the listview */
		bReturn = !PickerHitTest(pListView->hwndListView);
		break;

	case NM_DBLCLK:
		if (PickerHitTest(pListView->hwndListView)) {
			if (pListView->pClass->pfnRun)
				pListView->pClass->pfnRun(pListView);
		}
		bReturn = TRUE;
		break;

	case LVN_GETDISPINFO:
		pLvDispinfo = (LV_DISPINFO *) lpNmHdr;
		nItem = pLvDispinfo->item.lParam;

		if (pLvDispinfo->item.mask & LVIF_IMAGE)
			pLvDispinfo->item.iImage = SmartListView_WhichIcon(pListView, nItem);

		if (pLvDispinfo->item.mask & LVIF_STATE)
			pLvDispinfo->item.state = 0;

		if (pLvDispinfo->item.mask & LVIF_TEXT) {
			char *pszText;
			pszText = (char *) SmartListView_GetText(pListView, nItem, pLvDispinfo->item.iSubItem);
			pLvDispinfo->item.pszText = pszText;
		}
		bReturn = TRUE;
		break;

	case LVN_ITEMCHANGED:
        pNmListview = (NM_LISTVIEW *) lpNmHdr;
		bReturn = SmartListView_ItemChanged(pListView, pNmListview);
		break;

    case LVN_COLUMNCLICK:
        pNmListview = (NM_LISTVIEW *) lpNmHdr;
		nColumn = SmartListView_VisualColumnToLogical(pListView, pNmListview->iSubItem);
		SmartListView_ToggleSorting(pListView, nColumn);
		bReturn = TRUE;
		break;
	}
	return bReturn;
}

static void SmartListView_HandleDrawItem(struct SmartListView *pListView, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    HDC         hDC = lpDrawItemStruct->hDC;
    RECT        rcItem = lpDrawItemStruct->rcItem;
    UINT        uiFlags = ILD_TRANSPARENT;
    HIMAGELIST  hImageList;
    int         nVisualItem = lpDrawItemStruct->itemID;
    COLORREF    clrTextSave, clrBkSave;
    COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
    static TCHAR szBuff[MAX_PATH];
    BOOL        bFocus = (GetFocus() == pListView->hwndListView);
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
    int         *order;
	int nItemCount;
	HBITMAP hBackground;

	order = _alloca(pListView->pClass->nNumColumns * sizeof(int));

	nColumnMax = GetNumColumns(pListView);

	if (pListView->bOldControl) {
		SmartListView_GetColumnInfo(pListView, NULL, order, NULL);
	}
	else {
		/* Get the Column Order and save it */
		ListView_GetColumnOrderArray(pListView->hwndListView, nColumnMax, order);

		/* Disallow moving column 0 */
		if (order[0] != 0) {
			for (i = 0; i < nColumnMax; i++) {
				if (order[i] == 0) {
					order[i] = order[0];
					order[0] = 0;
				}
			}
			ListView_SetColumnOrderArray(pListView->hwndListView, nColumnMax, order);
		}
	}

    // Labels are offset by a certain amount  
    // This offset is related to the width of a space character
    GetTextExtentPoint32(hDC, TEXT(" "), 1 , &size);
    offset = size.cx * 2;

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem = nVisualItem;
    lvi.iSubItem = order[0];
    lvi.pszText = szBuff;
    lvi.cchTextMax = sizeof(szBuff);
    lvi.stateMask = 0xFFFF;       // get all state flags
    ListView_GetItem(pListView->hwndListView, &lvi);

    // This makes NO sense, but doesn't work without it?
    _tcscpy(szBuff, lvi.pszText);

    bSelected = ((bFocus) || (GetWindowLong(pListView->hwndListView, GWL_STYLE) & LVS_SHOWSELALWAYS))
        && SmartListView_IsItemSelected(pListView, nVisualItem);

    ListView_GetItemRect(pListView->hwndListView, nVisualItem, &rcAllLabels, LVIR_BOUNDS);

    ListView_GetItemRect(pListView->hwndListView, nVisualItem, &rcLabel, LVIR_LABEL);
    rcAllLabels.left = rcLabel.left;

	nItemCount = ListView_GetItemCount(pListView->hwndListView);

	hBackground = pListView->hBackground;

    if (hBackground) {
        RECT        rcClient;
        HRGN        rgnBitmap;
        RECT        rcTmpBmp = rcItem;
        HDC         htempDC;
        HBITMAP     oldBitmap;
#if HAS_MYBITMAPINFO
        HPALETTE hPAL;
        RECT rcFirstItem;
		int j;
#endif

        htempDC = CreateCompatibleDC(hDC);

        oldBitmap = SelectObject(htempDC, hBackground);

        GetClientRect(pListView->hwndListView, &rcClient); 
        rcTmpBmp.right = rcClient.right;
        // We also need to check whether it is the last item
        // The update region has to be extended to the bottom if it is
        if ((nItemCount == 0) || (nVisualItem == nItemCount - 1))
            rcTmpBmp.bottom = rcClient.bottom;
        
        rgnBitmap = CreateRectRgnIndirect(&rcTmpBmp);
        SelectClipRgn(hDC, rgnBitmap);
        DeleteObject(rgnBitmap);

#if HAS_MYBITMAPINFO
        hPAL = (!pListView->hPALbg) ? CreateHalftonePalette(hDC) : pListView->hPALbg;

        if(GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL )
        {
            SelectPalette(htempDC, hPAL, FALSE );
            RealizePalette(htempDC);
        }
        
        ListView_GetItemRect(pListView->hwndListView, 0, &rcFirstItem, LVIR_BOUNDS);
       
        for( i = rcFirstItem.left; i < rcClient.right; i += pListView->bmDesc.bmWidth )
            for( j = rcFirstItem.top; j < rcClient.bottom; j += pListView->bmDesc.bmHeight )
                BitBlt(hDC, i, j, pListView->bmDesc.bmWidth, pListView->bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY );

		SelectObject(htempDC, oldBitmap);
        DeleteDC(htempDC);

        if (!pListView->bmDesc.bmColors)
        {
            DeleteObject(hPAL);
            hPAL = 0;
        }
#endif /* HAS_MYBITMAPINFO */
    }

    SetTextColor(hDC, pListView->rgbListFontColor);

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
    else if (!hBackground)
    {
        HBRUSH hBrush;

        hBrush = CreateSolidBrush( GetSysColor(COLOR_WINDOW));
        FillRect(hDC, &rcAllLabels, hBrush);
        DeleteObject(hBrush);
    }

    if (nItemCount == 0)
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
        hImageList = ListView_GetImageList(pListView->hwndListView, LVSIL_STATE);
        if(hImageList)
            ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
    }

    ListView_GetItemRect(pListView->hwndListView, nVisualItem, &rcIcon, LVIR_ICON);

    hImageList = ListView_GetImageList(pListView->hwndListView, LVSIL_SMALL);
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

    ListView_GetItemRect(pListView->hwndListView, nVisualItem, &rcItem,LVIR_LABEL);

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
        ListView_GetColumn(pListView->hwndListView, order[nColumn] , &lvc);

        lvi.mask = LVIF_TEXT;
        lvi.iItem = nVisualItem; 
        lvi.iSubItem = order[nColumn];
        lvi.pszText = szBuff;
        lvi.cchTextMax = sizeof(szBuff);

        if (ListView_GetItem(pListView->hwndListView, &lvi) == FALSE)
            continue;

        /* This shouldn't oughtta be, but it's needed!!! */
        _tcscpy(szBuff, lvi.pszText);

        rcItem.left = rcItem.right;
        rcItem.right += lvc.cx;

        nRetLen = _tcslen(szBuff);
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

BOOL SmartListView_HandleEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam)
{
	BOOL bReturn = FALSE;
	LPNMHDR lpNmHdr;
	LPDRAWITEMSTRUCT lpDis;
#if HAS_CONTEXTMENU
	POINT pt;
#endif

	assert(SmartListView_IsEvent(pListView, message, wParam, lParam));

	switch(message) {
    case WM_NOTIFY:
		lpNmHdr = (LPNMHDR)lParam;
		bReturn = SmartListView_HandleNotify(pListView, lpNmHdr);
		break;

	case WM_DRAWITEM:
		lpDis = (LPDRAWITEMSTRUCT)lParam;
		SmartListView_HandleDrawItem(pListView, lpDis);
		bReturn = TRUE;
		break;

#if HAS_CONTEXTMENU
	case WM_CONTEXTMENU:
		assert((HWND)wParam == pListView->hwndListView);
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		SmartListView_HandleContextMenu(pListView, pt);
		bReturn = TRUE;
		break;
#endif /* HAS_CONTEXTMENU */
	}
	return bReturn;
}

/* ------------------------------------------------------------------------ */

void SmartListView_SaveColumnSettings(struct SmartListView *pListView)
{
	int nColumnMax, i;
	int *shown;
	int *order;
	int *widths;
	int *tmpOrder;

	shown = (int *) _alloca(pListView->pClass->nNumColumns * sizeof(*shown));
	order = (int *) _alloca(pListView->pClass->nNumColumns * sizeof(*order));
	widths = (int *) _alloca(pListView->pClass->nNumColumns * sizeof(*widths));
	tmpOrder = (int *) _alloca(pListView->pClass->nNumColumns * sizeof(*tmpOrder));

	SmartListView_GetColumnInfo(pListView, shown, order, widths);
	nColumnMax = GetNumColumns(pListView);

	if (pListView->bOldControl) {
		for (i = 0; i < nColumnMax; i++)
			widths[pListView->piRealColumns[i]] = ListView_GetColumnWidth(pListView->hwndListView, i);
	}
	else {
		/* Get the Column Order and save it */
		ListView_GetColumnOrderArray(pListView->hwndListView, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++) {
			widths[pListView->piRealColumns[i]] = ListView_GetColumnWidth(pListView->hwndListView, i);
			order[i] = pListView->piRealColumns[tmpOrder[i]];
		}
	}

	pListView->pClass->pfnSetColumnInfo(pListView, NULL, order, widths);
}

static void SmartListView_InternalResetColumnDisplay(struct SmartListView *pListView, BOOL bFirstTime)
{
	LV_COLUMN lvc;
	int *shown;
	int *order;
	int *widths;
	int i;
	int nColumn;
	int nNumColumns;

	nNumColumns = pListView->pClass->nNumColumns;

	shown = (int *) _alloca(nNumColumns * sizeof(*shown));
	order = (int *) _alloca(nNumColumns * sizeof(*order));
	widths = (int *) _alloca(nNumColumns * sizeof(*widths));

	SmartListView_GetColumnInfo(pListView, shown, order, widths);

	if (!bFirstTime) {
		nColumn = GetNumColumns(pListView);

		/* The first time thru this won't happen, on purpose */
		for (i = 0; i < nColumn; i++)
			widths[pListView->piRealColumns[i]] = ListView_GetColumnWidth(pListView->hwndListView, i);

		if (pListView->pClass->pfnSetColumnInfo)
			pListView->pClass->pfnSetColumnInfo(pListView, NULL, NULL, widths);

		for (i = nColumn; i > 0; i--)
			ListView_DeleteColumn(pListView->hwndListView, i - 1);
	}

	ListView_SetExtendedListViewStyle(pListView->hwndListView, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	nColumn = 0;

	for (i = 0; i < nNumColumns; i++) {
		if (shown[order[i]]) {
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT; 
			lvc.pszText = (TCHAR *) pListView->pClass->ppColumnNames[order[i]];
			lvc.iSubItem = nColumn;
			lvc.cx = widths[order[i]]; 
			ListView_InsertColumn(pListView->hwndListView, nColumn, &lvc);
			pListView->piRealColumns[nColumn] = order[i];
			nColumn++;
		}
	}

	// Fill this in so we can still sort on columns NOT shown
	for (i = 0; i < nNumColumns && nColumn < nNumColumns; i++) {
		if (!shown[order[i]]) {
			pListView->piRealColumns[nColumn] = order[i];
			nColumn++;
		}
	}

	ListView_SetTextColor(pListView->hwndListView, pListView->rgbListFontColor);
}

void SmartListView_ResetColumnDisplay(struct SmartListView *pListView)
{
	SmartListView_InternalResetColumnDisplay(pListView, FALSE);
}

static int SmartListView_InsertItem(struct SmartListView *pListView, int nItem)
{
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; 
	lvi.stateMask = 0;
	lvi.iItem = nItem;
	lvi.iSubItem = 0; 
	lvi.lParam = nItem;
	lvi.pszText  = LPSTR_TEXTCALLBACK;
	lvi.iImage   = I_IMAGECALLBACK;
	return ListView_InsertItem(pListView->hwndListView, &lvi);
}

BOOL SmartListView_AppendItem(struct SmartListView *pListView)
{
	int nNewIndex;
	struct RowMapping *newRowMapping;

	newRowMapping = realloc(pListView->rowMapping, sizeof(struct RowMapping) * (pListView->nNumRows + 1));
	if (!newRowMapping)
		return FALSE;
	pListView->rowMapping = newRowMapping;

	nNewIndex = SmartListView_InsertItem(pListView, pListView->nNumRows);
	if (nNewIndex < 0)
		return FALSE;

	newRowMapping[pListView->nNumRows].nVisualToLogical = pListView->nNumRows;
	newRowMapping[pListView->nNumRows].nLogicalToVisual = pListView->nNumRows;
	pListView->nNumRows++;
	return TRUE;
}

BOOL SmartListView_SetTotalItems(struct SmartListView *pListView, int nItemCount)
{
	int i;
	struct RowMapping *newRowMapping;

	assert(nItemCount >= 0);

	/* Create a new map */
	if (nItemCount > 0) {
		newRowMapping = (struct RowMapping *) malloc(nItemCount * sizeof(struct RowMapping));
		if (!newRowMapping)
			return FALSE;
		for (i = 0; i < nItemCount; i++) {
			newRowMapping[i].nVisualToLogical = i;
			newRowMapping[i].nLogicalToVisual = i;
		}
	}
	else {
		newRowMapping = NULL;
	}
	if (pListView->rowMapping)
		free(pListView->rowMapping);
	pListView->rowMapping = newRowMapping;
	
	ListView_DeleteAllItems(pListView->hwndListView);
	ListView_SetItemCount(pListView->hwndListView, nItemCount);

	for (i = 0; i < nItemCount; i++) {
		SmartListView_InsertItem(pListView, i);
	}

	pListView->nNumRows = nItemCount;

	return TRUE;
}

void SmartListView_SelectItem(struct SmartListView *pListView, int nItem, BOOL bFocus)
{
	int nVisualRow;
	nVisualRow = SmartListView_LogicalRowToVisual(pListView, nItem);

	if (bFocus) {
		ListView_SetItemState(pListView->hwndListView, nVisualRow, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		ListView_EnsureVisible(pListView->hwndListView, nVisualRow, FALSE);
	}
	else {
		ListView_SetItemState(pListView->hwndListView, nVisualRow, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void SmartListView_Update(struct SmartListView *pListView, int nItem)
{
	int nVisualRow;
	nVisualRow = SmartListView_LogicalRowToVisual(pListView, nItem);

	ListView_Update(pListView->hwndListView, nVisualRow);
}

void SmartListView_RedrawItem(struct SmartListView *pListView, int nItem)
{
	int nVisualRow;
	nVisualRow = SmartListView_LogicalRowToVisual(pListView, nItem);
	ListView_RedrawItems(pListView->hwndListView, nVisualRow, nVisualRow);
}

void SmartListView_GetRealColumnOrder(struct SmartListView *pListView, int *pnOrder)
{
	int *tmpOrder;
	int nColumnMax;
	int i;

	tmpOrder = (int *) _alloca(pListView->pClass->nNumColumns * sizeof(int));

	nColumnMax = GetNumColumns(pListView);

	/* Get the Column Order and save it */
	if (!pListView->bOldControl) {
		ListView_GetColumnOrderArray(pListView->hwndListView, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++)
			pnOrder[i] = pListView->piRealColumns[tmpOrder[i]];
	}
}

void SmartListView_SetTextColor(struct SmartListView *pListView, COLORREF clrText)
{
	pListView->rgbListFontColor = clrText;
	ListView_SetTextColor(pListView->hwndListView, clrText);
}

/* ------------------------------------------------------------------------ *
 * Sorting                                                                  *
 * ------------------------------------------------------------------------ */

struct sort_Info {
	struct SmartListView *pListView;
	int nColumn;
	BOOL bReverse;
};

static int CALLBACK sort_Compare(LPARAM nLogicalRow1, LPARAM nLogicalRow2, int nParam)
{
	struct sort_Info *pSortInfo;
	struct SmartListView *pListView;
	int nColumn;
	BOOL bReverse;
	int nResult;

	pSortInfo = (struct sort_Info *) nParam;
	pListView = pSortInfo->pListView;
	nColumn = pSortInfo->nColumn;
	bReverse = pSortInfo->bReverse;

	nResult = pListView->pClass->pfnCompare(pListView, nLogicalRow1, nLogicalRow2, nColumn); 
	return bReverse ? -nResult : nResult;
}

void SmartListView_SetSorting(struct SmartListView *pListView, int nColumn, BOOL bReverse)
{
	struct sort_Info si;
	int i;
	LVITEM lvi;

	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_PARAM;

	si.pListView = pListView;
	si.nColumn = nColumn;
	si.bReverse = bReverse;

	ListView_SortItems(pListView->hwndListView, sort_Compare, &si);

	/* Update sort condition */
	pListView->nSortCondition = (nColumn + 1) * (bReverse ? -1 : 1);

	/* Update logical -> visual row mappings */
	for (i = 0; i < pListView->nNumRows; i++) {
		lvi.iItem = i;
		ListView_GetItem(pListView->hwndListView, &lvi);

		pListView->rowMapping[i].nVisualToLogical = lvi.lParam;
		pListView->rowMapping[lvi.lParam].nLogicalToVisual = i;
	}
}

void SmartListView_GetSorting(struct SmartListView *pListView, int *nColumn, BOOL *bReverse)
{
	if (pListView->nSortCondition >= 0) {
		*nColumn = pListView->nSortCondition - 1;
		*bReverse = FALSE;
	}
	else {
		*nColumn = -pListView->nSortCondition + 1;
		*bReverse = TRUE;
	}
}

void SmartListView_ToggleSorting(struct SmartListView *pListView, int nColumn)
{
	int nCurrentSorting;
	BOOL bCurrentReverse;

	SmartListView_GetSorting(pListView, &nCurrentSorting, &bCurrentReverse);
	SmartListView_SetSorting(pListView, nColumn, (nCurrentSorting == nColumn) && !bCurrentReverse);
}

void SmartListView_AssociateImageLists(struct SmartListView *pListView, HIMAGELIST hSmall, HIMAGELIST hLarge)
{
	/* Create IconsList for ListView Control */
	if (hSmall)
		ListView_SetImageList(pListView->hwndListView, hSmall, LVSIL_SMALL);
	if (hLarge)
		ListView_SetImageList(pListView->hwndListView, hLarge, LVSIL_NORMAL);
}

/* ------------------------------------------------------------------------ *
 * Comparators                                                              *
 * ------------------------------------------------------------------------ */

int Compare_TextCaseInsensitive(struct SmartListView *pListView, int nRow1, int nRow2, int nColumn)
{
	LPCTSTR s1;
	LPCTSTR s2;
	s1 = pListView->pClass->pfnGetText(pListView, nRow1, nColumn);
	s2 = pListView->pClass->pfnGetText(pListView, nRow2, nColumn);
	s1 = s1 ? s1 : TEXT("");
	s2 = s2 ? s2 : TEXT("");
	return _tcsicmp(s1, s2);
}

/* ------------------------------------------------------------------------ *
 * Idling                                                                   *
 * ------------------------------------------------------------------------ */

BOOL SmartListView_CanIdle(struct SmartListView *pListView)
{
	if (pListView->pClass->pfnCanIdle)
		return pListView->pClass->pfnCanIdle(pListView) ? TRUE : FALSE;
	return FALSE;
}

void SmartListView_Idle(struct SmartListView *pListView)
{
	if (pListView->pClass->pfnIdle)
		pListView->pClass->pfnIdle(pListView);
}

/* Returns TRUE if done */
BOOL SmartListView_IdleUntilMsg(struct SmartListView *pListView)
{
	MSG msg;
	BOOL bCanIdle;

	while(((bCanIdle = SmartListView_CanIdle(pListView)) == TRUE) && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		SmartListView_Idle(pListView);

	return !bCanIdle;
}

/* ------------------------------------------------------------------------ *
 * Single Item List View Class                                              *
 * ------------------------------------------------------------------------ */

BOOL SingleItemSmartListViewClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow)
{
	struct SingleItemSmartListView *pSiListView;

	pSiListView = (struct SingleItemSmartListView *) pListView;

	if (!bWasSelected && bNowSelected) {
		/* entering item */
		pSiListView->nSelectedItem = nRow;
	}
	return TRUE;
}

BOOL SingleItemSmartListViewClass_IsItemSelected(struct SmartListView *pListView, int nItem)
{
	struct SingleItemSmartListView *pSiListView;
	pSiListView = (struct SingleItemSmartListView *) pListView;
	return nItem == pSiListView->nSelectedItem;
}

