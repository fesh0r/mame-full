/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string.h>
#include <sys/stat.h>
#include <wingdi.h>
#include <time.h>
#include <malloc.h>
#include <tchar.h>

#include "picker.h"
#include "resource.h"
#include "screenshot.h"
#include "win32ui.h"
#include "bitmask.h"
#include "options.h"


#if defined(__GNUC__)

/* fix warning: cast does not match function type */
#undef ListView_GetImageList
#define ListView_GetImageList(w,i) (HIMAGELIST)(LRESULT)(int)SendMessage((w),LVM_GETIMAGELIST,(i),0)

#undef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)(LRESULT)(int)SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))

#undef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
    SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))

#undef ListView_GetHeader
#define ListView_GetHeader(w) (HWND)(LRESULT)(int)SNDMSG((w),LVM_GETHEADER,0,0)

#define HDM_SETIMAGELIST        (HDM_FIRST + 8)
#define Header_SetImageList(h,i) (HIMAGELIST)(LRESULT)(int)SNDMSG((h), HDM_SETIMAGELIST, 0, (LPARAM)i)


#endif /* defined(__GNUC__) */

#ifndef HDF_SORTUP
#define HDF_SORTUP 0x400
#endif

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x200
#endif



struct PickerInfo
{
	const struct PickerCallbacks *pCallbacks;
	WNDPROC pfnParentWndProc;
	int nCurrentViewID;
	int nLastItem;
	int nColumnCount;
	int *pnColumnsShown;
	int *pnColumnsOrder;
	UINT_PTR nTimer;
	LPCTSTR *ppszColumnNames;
};


static struct PickerInfo *GetPickerInfo(HWND hWnd)
{
	LONG_PTR l;
	l = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return (struct PickerInfo *) l;
}



static LRESULT CallParentWndProc(WNDPROC pfnParentWndProc,
	HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT rc;

	if (!pfnParentWndProc)
		pfnParentWndProc = GetPickerInfo(hWnd)->pfnParentWndProc;

	if (IsWindowUnicode(hWnd))
		rc = CallWindowProcW(pfnParentWndProc, hWnd, message, wParam, lParam);
	else
		rc = CallWindowProcA(pfnParentWndProc, hWnd, message, wParam, lParam);
	return rc;
}



static BOOL ListCtrlOnPaint(HWND hWnd, UINT uMsg)
{
	PAINTSTRUCT ps;
	HDC 		hDC;
	RECT		rcClip, rcClient;
	HDC 		memDC;
	HBITMAP 	bitmap;
	HBITMAP 	hOldBitmap;
	HBITMAP		hBackground = GetBackgroundBitmap();
	HPALETTE	hPALbg = GetBackgroundPalette();

	hDC = BeginPaint(hWnd, &ps);
	rcClient = ps.rcPaint;
	GetClipBox(hDC, &rcClip);
	GetClientRect(hWnd, &rcClient);
	// Create a compatible memory DC
	memDC = CreateCompatibleDC(hDC);

	// Select a compatible bitmap into the memory DC
	bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
									rcClient.bottom - rcClient.top );
	hOldBitmap = SelectObject(memDC, bitmap);

	BitBlt(memDC, rcClip.left, rcClip.top,
		   rcClip.right - rcClip.left, rcClip.bottom - rcClip.top,
		   hDC, rcClip.left, rcClip.top, SRCCOPY);

	// First let the control do its default drawing.
	CallParentWndProc(NULL, hWnd, uMsg, (WPARAM)memDC, 0);

	// Draw bitmap in the background
	if( hBackground )
	{
		HPALETTE	hPAL;
		HDC maskDC;
		HBITMAP maskBitmap;
		HDC tempDC;
		HDC imageDC;
		HBITMAP bmpImage;
		HBITMAP hOldBmpImage;
		HBITMAP hOldMaskBitmap;
		HBITMAP hOldHBitmap;
		MYBITMAPINFO *pbmDesc;
		int i, j;
		POINT ptOrigin;
		POINT pt = {0,0};

		pbmDesc = GetBackgroundInfo();

		// Now create a mask
		maskDC = CreateCompatibleDC(hDC);

		// Create monochrome bitmap for the mask
		maskBitmap = CreateBitmap(rcClient.right  - rcClient.left,
								  rcClient.bottom - rcClient.top,
								  1, 1, NULL );

		hOldMaskBitmap = SelectObject(maskDC, maskBitmap);
		SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

		// Create the mask from the memory DC
		BitBlt(maskDC, 0, 0, rcClient.right - rcClient.left,
			   rcClient.bottom - rcClient.top, memDC,
			   rcClient.left, rcClient.top, SRCCOPY);

		tempDC = CreateCompatibleDC(hDC);
		hOldHBitmap = SelectObject(tempDC, hBackground);

		imageDC = CreateCompatibleDC(hDC);
		bmpImage = CreateCompatibleBitmap(hDC,
										  rcClient.right  - rcClient.left,
										  rcClient.bottom - rcClient.top);
		hOldBmpImage = SelectObject(imageDC, bmpImage);

		hPAL = (! hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(hDC, hPAL, FALSE);
			RealizePalette(hDC);
			SelectPalette(imageDC, hPAL, FALSE);
		}

		// Get x and y offset
		ClientToScreen(hWnd, &pt);
		GetDCOrgEx(hDC, &ptOrigin);
		ptOrigin.x -= pt.x;
		ptOrigin.y -= pt.y;
		ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
		ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

		// Draw bitmap in tiled manner to imageDC
		for (i = ptOrigin.x; i < rcClient.right; i += pbmDesc->bmWidth)
			for (j = ptOrigin.y; j < rcClient.bottom; j += pbmDesc->bmHeight)
				BitBlt(imageDC,  i, j, pbmDesc->bmWidth, pbmDesc->bmHeight,
					   tempDC, 0, 0, SRCCOPY);

		// Set the background in memDC to black. Using SRCPAINT with black
		// and any other color results in the other color, thus making black
		// the transparent color
		SetBkColor(memDC, RGB(0, 0, 0));
		SetTextColor(memDC, RGB(255, 255, 255));
		BitBlt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Set the foreground to black. See comment above.
		SetBkColor(imageDC, RGB(255, 255, 255));
		SetTextColor(imageDC, RGB(0, 0, 0));
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		// Combine the foreground with the background
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   memDC, rcClip.left, rcClip.top, SRCPAINT);

		// Draw the final image to the screen
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   imageDC, rcClip.left, rcClip.top, SRCCOPY);

		SelectObject(maskDC, hOldMaskBitmap);
		SelectObject(tempDC, hOldHBitmap);
		SelectObject(imageDC, hOldBmpImage);

		DeleteDC(maskDC);
		DeleteDC(imageDC);
		DeleteDC(tempDC);
		DeleteObject(bmpImage);
		DeleteObject(maskBitmap);
		if (!hPALbg)
		{
			DeleteObject(hPAL);
		}
	}
	SelectObject(memDC, hOldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	EndPaint(hWnd, &ps);
	return 0;
}



static BOOL ListViewOnErase(HWND hWnd, HDC hDC)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;

	int 		i, j;
	HDC 		htempDC;
	POINT		ptOrigin;
	POINT		pt = {0,0};
	HBITMAP 	hOldBitmap;
	MYBITMAPINFO *pbmDesc = GetBackgroundInfo();
	HBITMAP		hBackground = GetBackgroundBitmap();
	HPALETTE	hPALbg = GetBackgroundPalette();

	// this does not draw the background properly in report view

	GetClientRect(hWnd, &rcClient);

	htempDC = CreateCompatibleDC(hDC);
	hOldBitmap = SelectObject(htempDC, hBackground);

	rgnBitmap = CreateRectRgnIndirect(&rcClient);
	SelectClipRgn(hDC, rgnBitmap);
	DeleteObject(rgnBitmap);

	hPAL = (!hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, FALSE);
		RealizePalette(htempDC);
	}

	// Get x and y offset
	MapWindowPoints(hWnd, GetTreeView(), &pt, 1);
	GetDCOrgEx(hDC, &ptOrigin);
	ptOrigin.x -= pt.x;
	ptOrigin.y -= pt.y;
	ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
	ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

	if (pbmDesc->bmWidth && pbmDesc->bmHeight)
	{
		for (i = ptOrigin.x; i < rcClient.right; i += pbmDesc->bmWidth)
			for (j = ptOrigin.y; j < rcClient.bottom; j += pbmDesc->bmHeight)
				BitBlt(hDC, i, j, pbmDesc->bmWidth, pbmDesc->bmHeight, htempDC, 0, 0, SRCCOPY);
	}

	SelectObject(htempDC, hOldBitmap);
	DeleteDC(htempDC);

	if (!pbmDesc->bmColors)
	{
		DeleteObject(hPAL);
		hPAL = 0;
	}

	return TRUE;
}



static BOOL ListViewNotify(HWND hWnd, LPNMHDR lpNmHdr)
{
	RECT rcClient;
	DWORD dwPos;
	POINT pt;

	// This code is for using bitmap in the background
	// Invalidate the right side of the control when a column is resized			
	if (lpNmHdr->code == HDN_ITEMCHANGINGA || lpNmHdr->code == HDN_ITEMCHANGINGW)
	{
		dwPos = GetMessagePos();
		pt.x = LOWORD(dwPos);
		pt.y = HIWORD(dwPos);

		GetClientRect(hWnd, &rcClient);
		ScreenToClient(hWnd, &pt);
		rcClient.left = pt.x;
		InvalidateRect(hWnd, &rcClient, FALSE);
	}
	return FALSE;
}



static void Picker_Free(struct PickerInfo *pPickerInfo)
{
	if (pPickerInfo->pnColumnsShown)
		free(pPickerInfo->pnColumnsShown);
	if (pPickerInfo->pnColumnsOrder)
		free(pPickerInfo->pnColumnsOrder);
	free(pPickerInfo);
}



static LRESULT CALLBACK ListViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct PickerInfo *pPickerInfo;
	LRESULT rc = 0;
	BOOL bHandled = FALSE;
	WNDPROC pfnParentWndProc;

	pPickerInfo = GetPickerInfo(hWnd);
	pfnParentWndProc = pPickerInfo->pfnParentWndProc;

	switch(message)
	{
	    case WM_MOUSEMOVE:
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;

		case WM_PAINT:
			if (pPickerInfo->nCurrentViewID != VIEW_REPORT && pPickerInfo->nCurrentViewID != VIEW_GROUPED)
			{
				rc = ListCtrlOnPaint(hWnd, message);
				if (rc == 0)
					bHandled = TRUE;
			}
			break;

		case WM_ERASEBKGND:
			if (GetBackgroundBitmap())
			{
				rc = ListViewOnErase(hWnd, (HDC) wParam);
				bHandled = TRUE;
			}
			break;

		case WM_NOTIFY:
			bHandled = ListViewNotify(hWnd, (LPNMHDR) lParam);
			break;

		case WM_DESTROY:
			// Received WM_DESTROY; time to clean up
			if (pPickerInfo->pCallbacks->pfnSetViewMode)
				pPickerInfo->pCallbacks->pfnSetViewMode(pPickerInfo->nCurrentViewID);
			Picker_Free(pPickerInfo);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) pfnParentWndProc);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) NULL);
			break;
	}

	if (!bHandled)
		rc = CallParentWndProc(pfnParentWndProc, hWnd, message, wParam, lParam);
	return rc;
}



// Re/initialize the ListControl Columns
static void Picker_InternalResetColumnDisplay(HWND hWnd, BOOL bFirstTime)
{
	LV_COLUMN   lvc;
	int         i;
	int         nColumn;
	int         *widths;
	int         *order;
	int         *shown;
	int shown_columns;
	struct PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hWnd);

	widths = malloc(pPickerInfo->nColumnCount * sizeof(*widths));
	order = malloc(pPickerInfo->nColumnCount * sizeof(*order));
	shown = malloc(pPickerInfo->nColumnCount * sizeof(*shown));
	if (!widths || !order || !shown)
		goto done;

	pPickerInfo->pCallbacks->pfnGetColumnWidths(widths);
	pPickerInfo->pCallbacks->pfnGetColumnOrder(order);
	pPickerInfo->pCallbacks->pfnGetColumnShown(shown);

	if (!bFirstTime)
	{
		DWORD style = GetWindowLong(hWnd, GWL_STYLE);

		// switch the list view to LVS_REPORT style so column widths reported correctly
		SetWindowLong(hWnd, GWL_STYLE,
					  (GetWindowLong(hWnd, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_REPORT);

		nColumn = Picker_GetNumColumns(hWnd);

		// The first time thru this won't happen, on purpose
		// because the column widths will be in the negative millions,
		// since it's been created but not setup properly yet
		for (i = 0; i < nColumn; i++)
		{
			widths[Picker_GetRealColumnFromViewColumn(hWnd, i)] =
				ListView_GetColumnWidth(hWnd, i);
		}

		pPickerInfo->pCallbacks->pfnSetColumnWidths(widths);

		SetWindowLong(hWnd, GWL_STYLE, style);

		while(ListView_DeleteColumn(hWnd, 0))
			;
	}

	nColumn = 0;
	for (i = 0; i < pPickerInfo->nColumnCount; i++)
	{
		if (shown[order[i]])
		{
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
			lvc.pszText = (LPSTR) pPickerInfo->ppszColumnNames[order[i]];
			lvc.iSubItem = nColumn;
			lvc.cx = widths[order[i]];
			lvc.fmt = LVCFMT_LEFT;
			ListView_InsertColumn(hWnd, nColumn, &lvc);
			pPickerInfo->pnColumnsOrder[nColumn] = order[i];
			nColumn++;
		}
	}

	shown_columns = nColumn;

	/* Fill this in so we can still sort on columns NOT shown */
	for (i = 0; i < pPickerInfo->nColumnCount && nColumn < pPickerInfo->nColumnCount; i++)
	{
		if (!shown[order[i]])
		{
			pPickerInfo->pnColumnsOrder[nColumn] = order[i];
			nColumn++;
		}
	}

	if (GetListFontColor() == RGB(255, 255, 255))
		ListView_SetTextColor(hWnd, RGB(240, 240, 240));
	else
		ListView_SetTextColor(hWnd, GetListFontColor());

done:		
	if (widths)
		free(widths);
	if (order)
		free(order);
	if (shown)
		free(shown);
}



void Picker_ResetColumnDisplay(HWND hWnd)
{
	Picker_InternalResetColumnDisplay(hWnd, FALSE);
}



static void Picker_ClearIdle(HWND hwndPicker)
{
	struct PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);
	if (pPickerInfo->nTimer)
	{
		KillTimer(hwndPicker, 0);
		pPickerInfo->nTimer = 0;
	}
}



static void CALLBACK Picker_TimerProc(HWND hwndPicker, UINT uMsg,
  UINT_PTR idEvent, DWORD dwTime)
{
	struct PickerInfo *pPickerInfo;
	BOOL bContinueIdle;
	DWORD nTickCount, nBaseTickCount;
	DWORD nMaxIdleTicks = 50;

	pPickerInfo = GetPickerInfo(hwndPicker);
	bContinueIdle = FALSE;
	nBaseTickCount = GetTickCount();

	// This idle procedure will loop until either idling is over, or until
	// a specified amount of time elapses (in this case, 50ms).  This frees
	// idle callbacks of any responsibility for balancing their workloads; the
	// picker code will 
	do
	{
		if (pPickerInfo->pCallbacks->pfnOnIdle)
			bContinueIdle = pPickerInfo->pCallbacks->pfnOnIdle(hwndPicker);
		nTickCount = GetTickCount();
	}
	while(bContinueIdle && ((nTickCount - nBaseTickCount) < nMaxIdleTicks));

	if (!bContinueIdle)
		Picker_ClearIdle(hwndPicker);
}



// Instructs this picker to reset idling; idling will continue until the
// idle function returns FALSE
void Picker_ResetIdle(HWND hwndPicker)
{
	struct PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);

	Picker_ClearIdle(hwndPicker);
	if (pPickerInfo->pCallbacks->pfnOnIdle)
		pPickerInfo->nTimer = SetTimer(hwndPicker, 0, 0, Picker_TimerProc);
}



BOOL Picker_IsIdling(HWND hwndPicker)
{
	struct PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);
	return pPickerInfo->nTimer != 0;
}



BOOL SetupPicker(HWND hwndPicker, const struct PickerOptions *pOptions)
{
	struct PickerInfo *pPickerInfo;
	int i;
	LONG_PTR l;

	// Allocate the list view struct
	pPickerInfo = (struct PickerInfo *) malloc(sizeof(struct PickerInfo));
	if (!pPickerInfo)
		return FALSE;

	// And fill it out
	memset(pPickerInfo, 0, sizeof(*pPickerInfo));
	pPickerInfo->pCallbacks = pOptions->pCallbacks;
	pPickerInfo->nColumnCount = pOptions->nColumnCount;
	pPickerInfo->ppszColumnNames = pOptions->ppszColumnNames;
	pPickerInfo->nLastItem = -1;

	if (pPickerInfo->nColumnCount)
	{
		// Allocate space for the column order and columns shown array
		pPickerInfo->pnColumnsOrder = malloc(pPickerInfo->nColumnCount *
			sizeof(*pPickerInfo->pnColumnsOrder));
		pPickerInfo->pnColumnsShown = malloc(pPickerInfo->nColumnCount *
			sizeof(*pPickerInfo->pnColumnsShown));
		if (!pPickerInfo->pnColumnsOrder || !pPickerInfo->pnColumnsShown)
			goto error;

		// set up initial values
		for (i = 0; i < pPickerInfo->nColumnCount; i++)
		{
			pPickerInfo->pnColumnsOrder[i] = i;
			pPickerInfo->pnColumnsShown[i] = TRUE;
		}

		if (GetUseOldControl())
		{
			if (pPickerInfo->pCallbacks->pfnSetColumnOrder)
				pPickerInfo->pCallbacks->pfnSetColumnOrder(pPickerInfo->pnColumnsOrder);
			if (pPickerInfo->pCallbacks->pfnSetColumnShown)
				pPickerInfo->pCallbacks->pfnSetColumnShown(pPickerInfo->pnColumnsShown);
		}
		else
		{
			if (pPickerInfo->pCallbacks->pfnGetColumnOrder)
				pPickerInfo->pCallbacks->pfnGetColumnOrder(pPickerInfo->pnColumnsOrder);
			if (pPickerInfo->pCallbacks->pfnGetColumnShown)
				pPickerInfo->pCallbacks->pfnGetColumnShown(pPickerInfo->pnColumnsShown);
		}
	}

	// Hook in our wndproc and userdata pointer
	l = GetWindowLongPtr(hwndPicker, GWLP_WNDPROC);
	pPickerInfo->pfnParentWndProc = (WNDPROC) l;
	SetWindowLongPtr(hwndPicker, GWLP_USERDATA, (LONG_PTR) pPickerInfo);
	SetWindowLongPtr(hwndPicker, GWLP_WNDPROC, (LONG_PTR) ListViewWndProc);

	ListView_SetExtendedListViewStyle(hwndPicker, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP |
		LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD | LVS_EX_LABELTIP);

	Picker_InternalResetColumnDisplay(hwndPicker, TRUE);
	Picker_ResetIdle(hwndPicker);
	return TRUE;

error:
	if (pPickerInfo)
		Picker_Free(pPickerInfo);
	return FALSE;
}



int Picker_GetViewID(HWND hwndPicker)
{
	struct PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);
	return pPickerInfo->nCurrentViewID;
}



void Picker_SetViewID(HWND hwndPicker, int nViewID)
{
	struct PickerInfo *pPickerInfo;
	LONG_PTR nListViewStyle;

	pPickerInfo = GetPickerInfo(hwndPicker);

	// Change the nCurrentViewID member
	pPickerInfo->nCurrentViewID = nViewID;
	if (pPickerInfo->pCallbacks->pfnSetViewMode)
		pPickerInfo->pCallbacks->pfnSetViewMode(pPickerInfo->nCurrentViewID);

	// Change the ListView flags in accordance
	switch(nViewID)
	{
		case VIEW_LARGE_ICONS:
			nListViewStyle = LVS_ICON;
			break;
		case VIEW_SMALL_ICONS:
			nListViewStyle = LVS_SMALLICON;
			break;
		case VIEW_INLIST:
			nListViewStyle = LVS_LIST;
			break;
		case VIEW_GROUPED:
		case VIEW_REPORT:
		default:
			nListViewStyle = LVS_REPORT;
			break;
	}
	SetWindowLong(hwndPicker, GWL_STYLE, (GetWindowLong(hwndPicker, GWL_STYLE)
		& ~LVS_TYPEMASK) | nListViewStyle);
}



static BOOL PickerHitTest(HWND hWnd)
{
	RECT			rect;
	POINTS			p;
	DWORD			res = GetMessagePos();
	LVHITTESTINFO	htInfo;

    memset(&htInfo, 0, sizeof(htInfo));
	p = MAKEPOINTS(res);
	GetWindowRect(hWnd, &rect);
	htInfo.pt.x = p.x - rect.left;
	htInfo.pt.y = p.y - rect.top;
	ListView_HitTest(hWnd, &htInfo);

	return (! (htInfo.flags & LVHT_NOWHERE));
}



int Picker_GetSelectedItem(HWND hWnd)
{
	int nItem;
	LV_ITEM lvi;

	nItem = ListView_GetNextItem(hWnd, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (nItem < 0)
		return 0;

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = nItem;
	lvi.mask = LVIF_PARAM;
	ListView_GetItem(hWnd, &lvi);
	return lvi.lParam;
}



void Picker_SetSelectedPick(HWND hWnd, int nIndex)
{
	if (nIndex < 0)
		nIndex = 0;

	ListView_SetItemState(hWnd, nIndex, LVIS_FOCUSED | LVIS_SELECTED,
		LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hWnd, nIndex, FALSE);
}



void Picker_SetSelectedItem(HWND hWnd, int nItem)
{
	int i;
	LV_FINDINFO lvfi;

	if (nItem < 0)
		return;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = nItem;
	i = ListView_FindItem(hWnd, -1, &lvfi);
	if (i == -1)
	{
		POINT p = {0,0};
		lvfi.flags = LVFI_NEARESTXY;
		lvfi.pt = p;
		i = ListView_FindItem(hWnd, -1, &lvfi);
	}
	Picker_SetSelectedPick(hWnd, (i == -1) ? 0 : i);
}



static const TCHAR *Picker_CallGetItemString(struct PickerInfo *pPickerInfo,
	int nItem, int nColumn, TCHAR *pszBuffer, UINT nBufferLength)
{
	// this call wraps the pfnGetItemString callback to properly set up the
	// buffers, and normalize the results
	const TCHAR *s;

	pszBuffer[0] = '\0';
	s = NULL;

	if (pPickerInfo->pCallbacks->pfnGetItemString)
	{
		s = pPickerInfo->pCallbacks->pfnGetItemString(nItem, nColumn,
			pszBuffer, nBufferLength);
	}
	if (!s)
		s = pszBuffer;
	return s;
}



// put the arrow on the new sort column
static void Picker_ResetHeaderSortIcon(HWND hWnd)
{
	struct PickerInfo *pPickerInfo;
	HWND hwndHeader;
	HD_ITEM hdi;
	int i;

	pPickerInfo = GetPickerInfo(hWnd);

	hwndHeader = ListView_GetHeader(hWnd);

	// take arrow off non-current columns
	hdi.mask = HDI_FORMAT;
	hdi.fmt = HDF_STRING;
	for (i = 0; i < pPickerInfo->nColumnCount; i++)
	{
		if (i != pPickerInfo->pCallbacks->pfnGetSortColumn())
			Header_SetItem(hwndHeader, Picker_GetViewColumnFromRealColumn(hWnd, i), &hdi);
	}

	if (GetUseXPControl())
	{
		// use built in sort arrows
		hdi.mask = HDI_FORMAT;
		hdi.fmt = HDF_STRING | (pPickerInfo->pCallbacks->pfnGetSortReverse() ? HDF_SORTDOWN : HDF_SORTUP);
	}
	else
	{
		// put our arrow icon next to the text
		hdi.mask = HDI_FORMAT | HDI_IMAGE;
		hdi.fmt = HDF_STRING | HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
		hdi.iImage = pPickerInfo->pCallbacks->pfnGetSortReverse() ? 1 : 0;
	}
	Header_SetItem(hwndHeader, Picker_GetViewColumnFromRealColumn(hWnd, pPickerInfo->pCallbacks->pfnGetSortColumn()), &hdi);
}



struct CompareProcParams
{
	struct PickerInfo *pPickerInfo;
	int nSortColumn;
	int nViewMode;
	BOOL bReverse;
};

static int CALLBACK Picker_CompareProc(LPARAM index1, LPARAM index2, LPARAM nParamSort)
{
	struct CompareProcParams *pcpp = (struct CompareProcParams *) nParamSort;
	struct PickerInfo *pPickerInfo = pcpp->pPickerInfo;
	BOOL bCallCompare = TRUE;
	int nResult = 0, nParent1, nParent2;
	TCHAR szBuffer1[256], szBuffer2[256];
	const TCHAR *s1, *s2;

	if (pcpp->nViewMode == VIEW_GROUPED)
	{
		// do our fancy compare, with clones grouped under parents
		// first thing we need to do is identify both item's parents
		if (pPickerInfo->pCallbacks->pfnFindItemParent)
		{
			nParent1 = pPickerInfo->pCallbacks->pfnFindItemParent(index1);
			nParent2 = pPickerInfo->pCallbacks->pfnFindItemParent(index2);
		}
		else
		{
			nParent1 = nParent2 = -1;
		}

		if ((nParent1 < 0) && (nParent2 < 0))
		{
			// if both entries are both parents, we just do a basic sort
		}
		else if ((nParent1 >= 0) && (nParent2 >= 0))
		{
			// if both entries are children and the parents are different,
			// sort on their parents
			if (nParent1 != nParent2)
			{
				index1 = nParent1;
				index2 = nParent2;
			}
		}
		else
		{
			// one parent, one child
			if (nParent1 >= 0)
			{
				// first one is a child
				if (nParent1 == index2)
				{
					// if this is a child and its parent, put child after
					nResult = 1;
					bCallCompare = FALSE;
				}
				else
				{
					// sort on parent
					index1 = nParent1;
				}
			}
			else
			{
				// second one is a child
				if (nParent2 == index1)
				{
					// if this is a child and its parent, put child after
					nResult = -1;
					bCallCompare = FALSE;
				}
				else
				{
					// sort on parent
					index2 = nParent2;
				}
			}
		}
	}

	if (bCallCompare)
	{
		if (pPickerInfo->pCallbacks->pfnCompare)
		{
			nResult = pPickerInfo->pCallbacks->pfnCompare(index1, index2, pcpp->nSortColumn);
		}
		else
		{
			// no default sort proc, just get the string and compare them
			s1 = Picker_CallGetItemString(pPickerInfo, index1, pcpp->nSortColumn,
				szBuffer1, sizeof(szBuffer1) / sizeof(szBuffer1[0]));
			s2 = Picker_CallGetItemString(pPickerInfo, index2, pcpp->nSortColumn,
				szBuffer2, sizeof(szBuffer2) / sizeof(szBuffer2[0]));
			nResult = _tcsicmp(s1, s2);
		}

		if (pcpp->bReverse)
			nResult = -nResult;
	}
	return nResult;
}



void Picker_Sort(HWND hWnd)
{
	LV_FINDINFO lvfi;
	struct PickerInfo *pPickerInfo;
	struct CompareProcParams params;

	pPickerInfo = GetPickerInfo(hWnd);

	// populate the CompareProcParams structure
	memset(&params, 0, sizeof(params));
	params.pPickerInfo = pPickerInfo;
	params.nViewMode = pPickerInfo->pCallbacks->pfnGetViewMode();
	if (pPickerInfo->pCallbacks->pfnGetSortColumn)
		params.nSortColumn = pPickerInfo->pCallbacks->pfnGetSortColumn();
	if (pPickerInfo->pCallbacks->pfnGetSortReverse)
		params.bReverse = pPickerInfo->pCallbacks->pfnGetSortReverse();

	ListView_SortItems(hWnd, Picker_CompareProc, (LPARAM) &params);

	Picker_ResetHeaderSortIcon(hWnd);

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = Picker_GetSelectedItem(hWnd);
	ListView_EnsureVisible(hWnd, ListView_FindItem(hWnd, -1, &lvfi), FALSE);
}



int Picker_GetRealColumnFromViewColumn(HWND hWnd, int nViewColumn)
{
	struct PickerInfo *pPickerInfo;
	int nRealColumn = 0;

	pPickerInfo = GetPickerInfo(hWnd);
	if (nViewColumn >= 0 && nViewColumn < pPickerInfo->nColumnCount)
		nRealColumn = pPickerInfo->pnColumnsOrder[nViewColumn];
	return nRealColumn;
}



int Picker_GetViewColumnFromRealColumn(HWND hWnd, int nRealColumn)
{
	struct PickerInfo *pPickerInfo;
	int i;

	pPickerInfo = GetPickerInfo(hWnd);
	for (i = 0; i < pPickerInfo->nColumnCount; i++)
	{
		if (pPickerInfo->pnColumnsOrder[i] == nRealColumn)
			return i;
	}

	// major error, shouldn't be possible, but no good way to warn
	return 0;
}



BOOL Picker_HandleNotify(LPNMHDR lpNmHdr)
{
	struct PickerInfo *pPickerInfo;
	HWND hWnd;
	BOOL bResult = FALSE;
	NM_LISTVIEW *pnmv;
	LV_DISPINFO *pDispInfo;
	int nItem, nColumn;
	const TCHAR *s;
	BOOL bReverse;

	hWnd = lpNmHdr->hwndFrom;
	pPickerInfo = GetPickerInfo(hWnd);
	pnmv = (NM_LISTVIEW *) lpNmHdr;

	switch(lpNmHdr->code)
	{
		case NM_RCLICK:
		case NM_CLICK:
		case NM_DBLCLK:
			// don't allow selection of blank spaces in the listview
			if (!PickerHitTest(hWnd))
			{
				// we have no current item selected
				if (pPickerInfo->nLastItem != -1)
				{
					Picker_SetSelectedItem(hWnd, pPickerInfo->nLastItem);
				}
				bResult = TRUE;
			}
			else if ((lpNmHdr->code == NM_DBLCLK) && (pPickerInfo->pCallbacks->pfnDoubleClick))
			{
				// double click!
				pPickerInfo->pCallbacks->pfnDoubleClick();
				bResult = TRUE;
			}
			break;

		case LVN_GETDISPINFO:
			pDispInfo = (LV_DISPINFO *) lpNmHdr;
			nItem = (int) pDispInfo->item.lParam;

			if (pDispInfo->item.mask & LVIF_IMAGE)
			{
				// retrieve item image
				if (pPickerInfo->pCallbacks->pfnGetItemImage)
					pDispInfo->item.iImage = pPickerInfo->pCallbacks->pfnGetItemImage(nItem);
				else
					pDispInfo->item.iImage = 0;
				bResult = TRUE;
			}

			if (pDispInfo->item.mask & LVIF_STATE)
			{
				pDispInfo->item.state = 0;
				bResult = TRUE;
			}

			if (pDispInfo->item.mask & LVIF_TEXT)
			{
				// retrieve item text
				nColumn = Picker_GetRealColumnFromViewColumn(hWnd, pDispInfo->item.iSubItem);
				
				s = Picker_CallGetItemString(pPickerInfo, nItem, nColumn,
					pDispInfo->item.pszText, pDispInfo->item.cchTextMax);

				pDispInfo->item.pszText = (TCHAR *) s;
				bResult = TRUE;
			}
			break;

		case LVN_ITEMCHANGED:
			if ((pnmv->uOldState & LVIS_SELECTED)
				&& !(pnmv->uNewState & LVIS_SELECTED))
			{
				if (pnmv->lParam != -1)
					pPickerInfo->nLastItem = pnmv->lParam;
				if (pPickerInfo->pCallbacks->pfnLeavingItem)
					pPickerInfo->pCallbacks->pfnLeavingItem(pnmv->lParam);
			}
			if (!(pnmv->uOldState & LVIS_SELECTED)
				&& (pnmv->uNewState & LVIS_SELECTED))
			{
				if (pPickerInfo->pCallbacks->pfnEnteringItem)
					pPickerInfo->pCallbacks->pfnEnteringItem(pnmv->lParam);
			}
			bResult = TRUE;
			break;

		case LVN_COLUMNCLICK:
			// if clicked on the same column we're sorting by, reverse it
			if (pPickerInfo->pCallbacks->pfnGetSortColumn() == Picker_GetRealColumnFromViewColumn(hWnd, pnmv->iSubItem))
				bReverse = !pPickerInfo->pCallbacks->pfnGetSortReverse();
			else
				bReverse = FALSE;
			pPickerInfo->pCallbacks->pfnSetSortReverse(bReverse);
			pPickerInfo->pCallbacks->pfnSetSortColumn(Picker_GetRealColumnFromViewColumn(hWnd, pnmv->iSubItem));
			Picker_Sort(hWnd);
			bResult = TRUE;
			break;

		case LVN_BEGINDRAG:
			if (pPickerInfo->pCallbacks->pfnBeginListViewDrag)
				pPickerInfo->pCallbacks->pfnBeginListViewDrag(pnmv);
			break;
	}
	return bResult;
}



int Picker_GetNumColumns(HWND hWnd)
{
	int  nColumnCount = 0;
	int  i;
	HWND hwndHeader;
	int  *shown;
	struct PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hWnd);

	shown = malloc(pPickerInfo->nColumnCount * sizeof(*shown));
	if (!shown)
		return -1;

	pPickerInfo->pCallbacks->pfnGetColumnShown(shown);
	hwndHeader = ListView_GetHeader(hWnd);

	if (GetUseOldControl() || (nColumnCount = Header_GetItemCount(hWnd)) < 1)
	{
		nColumnCount = 0;
		for (i = 0; i < pPickerInfo->nColumnCount ; i++ )
		{
			if (shown[i])
				nColumnCount++;
		}
	}
	
	free(shown);
	return nColumnCount;
}



/* Add ... to Items in ListView if needed */
static LPCTSTR MakeShortString(HDC hDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const CHAR szThreeDots[] = "...";
	static CHAR szShort[MAX_PATH];
	int nStringLen = lstrlen(lpszLong);
	int nAddLen;
	SIZE size;
	int i;

	GetTextExtentPoint32(hDC, lpszLong, nStringLen, &size);
	if (nStringLen == 0 || size.cx + nOffset <= nColumnLen)
		return lpszLong;

	lstrcpy(szShort, lpszLong);
	GetTextExtentPoint32(hDC, szThreeDots, sizeof(szThreeDots), &size);
	nAddLen = size.cx;

	for (i = nStringLen - 1; i > 0; i--)
	{
		szShort[i] = 0;
		GetTextExtentPoint32(hDC, szShort, i, &size);
		if (size.cx + nOffset + nAddLen <= nColumnLen)
			break;
	}

	lstrcat(szShort, szThreeDots);

	return szShort;
}



void Picker_HandleDrawItem(HWND hWnd, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	struct PickerInfo *pPickerInfo;
	HDC         hDC = lpDrawItemStruct->hDC;
	RECT        rcItem = lpDrawItemStruct->rcItem;
	UINT        uiFlags = ILD_TRANSPARENT;
	HIMAGELIST  hImageList;
	int         nItem = lpDrawItemStruct->itemID;
	COLORREF    clrTextSave = 0;
	COLORREF    clrBkSave = 0;
	COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
	static TCHAR szBuff[MAX_PATH];
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
	int         i, j;
	int         nColumn;
	int         nColumnMax = 0;
	int         *order;
	BOOL        bDrawAsChild;
	int indent_space;
	BOOL		bColorChild = FALSE;
	int nParent;
	HBITMAP		hBackground = GetBackgroundBitmap();
	MYBITMAPINFO *pbmDesc = GetBackgroundInfo();

	pPickerInfo = GetPickerInfo(hWnd);

	order = malloc(pPickerInfo->nColumnCount * sizeof(*order));
	if (!order)
		return;
	nColumnMax = Picker_GetNumColumns(hWnd);

	if (GetUseOldControl())
	{
		pPickerInfo->pCallbacks->pfnGetColumnOrder(order);
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

	/* Labels are offset by a certain amount */
	/* This offset is related to the width of a space character */
	GetTextExtentPoint32(hDC, " ", 1 , &size);
	offset = size.cx;

	lvi.mask	   = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem	   = nItem;
	lvi.iSubItem   = order[0];
	lvi.pszText	   = szBuff;
	lvi.cchTextMax = sizeof(szBuff) / sizeof(szBuff[0]);
	lvi.stateMask  = 0xFFFF;	   /* get all state flags */
	ListView_GetItem(hWnd, &lvi);

	bSelected = ((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED)
		&& ((bFocus) || (GetWindowLong(hWnd, GWL_STYLE) & LVS_SHOWSELALWAYS))));

	/* figure out if we indent and draw grayed */
	if (pPickerInfo->pCallbacks->pfnFindItemParent)
		nParent = pPickerInfo->pCallbacks->pfnFindItemParent(lvi.lParam);
	else
		nParent = -1;
	bDrawAsChild = (pPickerInfo->pCallbacks->pfnGetViewMode() == VIEW_GROUPED && (nParent >= 0));

	if (pPickerInfo->pCallbacks->pfnGetOffsetChildren && pPickerInfo->pCallbacks->pfnGetOffsetChildren())
	{
		if ((nParent < 0) && bDrawAsChild)
		{
			/*Reset it, as no Parent is there*/
			bDrawAsChild = FALSE;
			bColorChild = TRUE;
		}
		else
		{
			nParent = -1;
		}
	}

	ListView_GetItemRect(hWnd, nItem, &rcAllLabels, LVIR_BOUNDS);

	ListView_GetItemRect(hWnd, nItem, &rcLabel, LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;

	if (hBackground != NULL)
	{
		RECT		rcClient;
		HRGN		rgnBitmap;
		RECT		rcTmpBmp = rcItem;
		RECT		rcFirstItem;
		HPALETTE	hPAL;
		HDC 		htempDC;
		HBITMAP 	oldBitmap;

		htempDC = CreateCompatibleDC(hDC);

		oldBitmap = SelectObject(htempDC, hBackground);

		GetClientRect(hWnd, &rcClient);
		rcTmpBmp.right = rcClient.right;
		/* We also need to check whether it is the last item
		   The update region has to be extended to the bottom if it is */
		if (nItem == ListView_GetItemCount(hWnd) - 1)
			rcTmpBmp.bottom = rcClient.bottom;

		rgnBitmap = CreateRectRgnIndirect(&rcTmpBmp);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);

		hPAL = GetBackgroundPalette();
		if (hPAL == NULL)
			hPAL = CreateHalftonePalette(hDC);

		if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(htempDC, hPAL, FALSE);
			RealizePalette(htempDC);
		}

		ListView_GetItemRect(hWnd, 0, &rcFirstItem, LVIR_BOUNDS);

		for (i = rcFirstItem.left; i < rcClient.right; i += pbmDesc->bmWidth)
			for (j = rcFirstItem.top; j < rcClient.bottom; j +=  pbmDesc->bmHeight)
				BitBlt(hDC, i, j, pbmDesc->bmWidth, pbmDesc->bmHeight, htempDC, 0, 0, SRCCOPY);

		SelectObject(htempDC, oldBitmap);
		DeleteDC(htempDC);

		if (GetBackgroundPalette() == NULL)
		{
			DeleteObject(hPAL);
			hPAL = NULL;
		}
	}

	indent_space = 0;

	if (bDrawAsChild)
	{
		RECT rect;
		ListView_GetItemRect(hWnd, nItem, &rect, LVIR_ICON);

		/* indent width of icon + the space between the icon and text
		 * so left of clone icon starts at text of parent
         */
		indent_space = rect.right - rect.left + offset;
	}

	rcAllLabels.left += indent_space;

	if (bSelected)
	{
		HBRUSH hBrush;
		HBRUSH hOldBrush;

		if (bFocus)
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		}

		hOldBrush = SelectObject(hDC, hBrush);
		FillRect(hDC, &rcAllLabels, hBrush);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}
	else
	{
		if (hBackground == NULL)
		{
			HBRUSH hBrush;
			
			hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			FillRect(hDC, &rcAllLabels, hBrush);
			DeleteObject(hBrush);
		}
		
		if (pPickerInfo->pCallbacks->pfnGetOffsetChildren && pPickerInfo->pCallbacks->pfnGetOffsetChildren())
		{
			if (bDrawAsChild || bColorChild)
				clrTextSave = SetTextColor(hDC, GetListCloneColor());
			else
				clrTextSave = SetTextColor(hDC, GetListFontColor());
		}
		else
		{
			if (bDrawAsChild)
				clrTextSave = SetTextColor(hDC, GetListCloneColor());
			else
				clrTextSave = SetTextColor(hDC, GetListFontColor());
		}

		clrBkSave = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
	}


	if (lvi.state & LVIS_CUT)
	{
		clrImage = GetSysColor(COLOR_WINDOW);
		uiFlags |= ILD_BLEND50;
	}
	else if (bSelected)
	{
		if (bFocus)
			clrImage = GetSysColor(COLOR_HIGHLIGHT);
		else
			clrImage = GetSysColor(COLOR_BTNFACE);

		uiFlags |= ILD_BLEND50;
	}

	nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;

	if (nStateImageMask)
	{
		int nImage = (nStateImageMask >> 12) - 1;
		hImageList = ListView_GetImageList(hWnd, LVSIL_STATE);
		if (hImageList)
			ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
	}

	ListView_GetItemRect(hWnd, nItem, &rcIcon, LVIR_ICON);

	rcIcon.left += indent_space;

	ListView_GetItemRect(hWnd, nItem, &rcItem, LVIR_LABEL);

	hImageList = ListView_GetImageList(hWnd, LVSIL_SMALL);
	if (hImageList)
	{
		UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
		if (rcIcon.left + 16 + indent_space < rcItem.right)
		{
			ImageList_DrawEx(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top, 16, 16,
							 GetSysColor(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
		}
	}

	ListView_GetItemRect(hWnd, nItem, &rcItem, LVIR_LABEL);

	pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2*offset + indent_space);

	rcLabel = rcItem;
	rcLabel.left  += offset + indent_space;
	rcLabel.right -= offset;

	DrawText(hDC, pszText, -1, &rcLabel,
			 DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

	for (nColumn = 1; nColumn < nColumnMax; nColumn++)
	{
		int 	nRetLen;
		UINT	nJustify;
		LV_ITEM lvItem;

		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		ListView_GetColumn(hWnd, order[nColumn], &lvc);

		lvItem.mask 	  = LVIF_TEXT;
		lvItem.iItem	  = nItem;
		lvItem.iSubItem   = order[nColumn];
		lvItem.pszText	  = szBuff;
		lvItem.cchTextMax = sizeof(szBuff) / sizeof(szBuff[0]);

		if (ListView_GetItem(hWnd, &lvItem) == FALSE)
			continue;

		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;

		nRetLen = strlen(szBuff);
		if (nRetLen == 0)
			continue;

		pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2 * offset);

		nJustify = DT_LEFT;

		if (pszText == szBuff)
		{
			switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
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
		rcLabel.left  += offset;
		rcLabel.right -= offset;
		DrawText(hDC, pszText, -1, &rcLabel,
				 nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
	}

	if (lvi.state & LVIS_FOCUSED && bFocus)
		DrawFocusRect(hDC, &rcAllLabels);

	SetTextColor(hDC, clrTextSave);
	SetBkColor(hDC, clrBkSave);
	free(order);
}




