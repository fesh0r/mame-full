#ifndef SMARTLISTVIEW_H
#define SMARTLISTVIEW_H

#include <windows.h>
#include <commctrl.h>

#ifdef UNDER_CE
#define HAS_MYBITMAPINFO	0
#define HAS_COLUMNEDIT		0
#define HAS_CONTEXTMENU		0
#define HAS_EXTRACOLUMNTEXT	1
#else
#define HAS_MYBITMAPINFO	1
#define HAS_COLUMNEDIT		1
#define HAS_CONTEXTMENU		1
#define HAS_EXTRACOLUMNTEXT	0
#include "ui/screenshot.h"
#endif

struct SmartListView;

struct SmartListViewClass
{
	size_t nObjectSize;
	void (*pfnRun)(struct SmartListView *pListView);
	BOOL (*pfnItemChanged)(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
	int (*pfnWhichIcon)(struct SmartListView *pListView, int nItem);
	LPCTSTR (*pfnGetText)(struct SmartListView *pListView, int nRow, int nColumn);
	void (*pfnGetColumnInfo)(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
	void (*pfnSetColumnInfo)(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
	BOOL (*pfnIsItemSelected)(struct SmartListView *pListView, int nItem);
	int (*pfnCompare)(struct SmartListView *pListView, int nRow1, int nRow2, int nColumn);
	BOOL (*pfnCanIdle)(struct SmartListView *pListView);
	void (*pfnIdle)(struct SmartListView *pListView);
	int nNumColumns;
	const TCHAR **ppColumnNames;
};

struct SmartListViewOptions
{
	const struct SmartListViewClass *pClass;
	HWND hwndParent;
	int nIDDlgItem;
	HBITMAP hBackground;
	HPALETTE hPALbg;
#if HAS_MYBITMAPINFO
	MYBITMAPINFO bmDesc;
#endif /* HAS_MYBITMAPINFO */
	COLORREF rgbListFontColor;
	HIMAGELIST hSmall;
	HIMAGELIST hLarge;
	BOOL bOldControl;
	BOOL bCenterOnParent;
	int nInsetPixels;
};

struct RowMapping
{
	int nVisualToLogical;
	int nLogicalToVisual;
};

struct SmartListView
{
	/* The underlying list view */
	HWND hwndListView;

	/* These come from the initial options */
	const struct SmartListViewClass *pClass;
	HWND hwndParent;
	int nIDDlgItem;
	HBITMAP hBackground;
	HPALETTE hPALbg;
#if HAS_MYBITMAPINFO
	MYBITMAPINFO bmDesc;
#endif /* HAS_MYBITMAPINFO */
	COLORREF rgbListFontColor;
	BOOL bOldControl;

	/* These are used at runtime */
	int nNumRows;
	struct RowMapping *rowMapping;
	int *piRealColumns;
	int nSortCondition;

#if HAS_EXTRACOLUMNTEXT
	LPTSTR lpExtraColumnText;
#endif /* HAS_EXTRACOLUMNTEXT */
};

struct SmartListView *SmartListView_Create(struct SmartListViewOptions *pOptions,
	BOOL bVisible, BOOL bSingleSel, int x, int y, int nWidth, int nHeight, HINSTANCE hInstance);

struct SmartListView *SmartListView_Init(struct SmartListViewOptions *pOptions);
void SmartListView_Free(struct SmartListView *pListView);

BOOL SmartListView_IsEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam);
BOOL SmartListView_HandleEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam);
void SmartListView_ResetColumnDisplay(struct SmartListView *pListView);
BOOL SmartListView_SetTotalItems(struct SmartListView *pListView, int nItemCount);
BOOL SmartListView_AppendItem(struct SmartListView *pListView);
void SmartListView_SelectItem(struct SmartListView *pListView, int nItem, BOOL bFocus);
void SmartListView_RedrawItem(struct SmartListView *pListView, int nItem);
void SmartListView_Update(struct SmartListView *pListView, int nItem);
BOOL SmartListView_CanIdle(struct SmartListView *pListView);
void SmartListView_Idle(struct SmartListView *pListView);
BOOL SmartListView_IdleUntilMsg(struct SmartListView *pListView);
void SmartListView_GetSorting(struct SmartListView *pListView, int *nColumn, BOOL *bReverse);
void SmartListView_SetSorting(struct SmartListView *pListView, int nColumn, BOOL bReverse);
void SmartListView_ToggleSorting(struct SmartListView *pListView, int nColumn);
void SmartListView_GetRealColumnOrder(struct SmartListView *pListView, int *pnOrder);
void SmartListView_SaveColumnSettings(struct SmartListView *pListView);
void SmartListView_AssociateImageLists(struct SmartListView *pListView, HIMAGELIST hSmall, HIMAGELIST hLarge);
void SmartListView_SetTextColor(struct SmartListView *pListView, COLORREF clrText);
void SmartListView_SetVisible(struct SmartListView *pListView, BOOL bVisible);
BOOL SmartListView_GetVisible(struct SmartListView *pListView);
void SmartListView_ScrollTo(struct SmartListView *pListView, int nItem);
void SmartListView_SetBackground(struct SmartListView *pListView, HBITMAP hBackground);
#if HAS_MYBITMAPINFO
void SmartListView_SetMyBitmapInfo(struct SmartListView *pListView, const MYBITMAPINFO *pbmDesc);
#endif /* HAS_MYBITMAPINFO */

#if HAS_EXTRACOLUMNTEXT
void SmartListView_SetExtraColumnText(struct SmartListView *pListView, LPCTSTR lpExtraColumnText);
#endif /* HAS_EXTRACOLUMNTEXT */

int Compare_TextCaseInsensitive(struct SmartListView *pListView, int nRow1, int nRow2, int nColumn);

/* -------------------------------------------------------------------------------------------- */

struct SingleItemSmartListView
{
	struct SmartListView base;
	int nSelectedItem;
};

/* Class functions */
BOOL SingleItemSmartListViewClass_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
BOOL SingleItemSmartListViewClass_IsItemSelected(struct SmartListView *pListView, int nItem);

/* Callable functions */
int SingleItemSmartListView_GetSelectedItem(struct SmartListView *pListView);

#endif /* SMARTLISTVIEW_H */
