#include <windows.h>
#include <commctrl.h>
#include "Screenshot.h"

struct SmartListView;

struct SmartListViewClass {
	void (*pfnRun)(struct SmartListView *pListView);
	BOOL (*pfnItemChanged)(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
	int (*pfnWhichIcon)(struct SmartListView *pListView, int nItem);
	LPCSTR (*pfnGetText)(struct SmartListView *pListView, int nRow, int nColumn);
	void (*pfnGetColumnInfo)(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
	void (*pfnSetColumnInfo)(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
	BOOL (*pfnIsItemSelected)(struct SmartListView *pListView, int nItem);
	int (*pfnCompare)(struct SmartListView *pListView, int nRow1, int nRow2, int nColumn);
	BOOL (*pfnCanIdle)(struct SmartListView *pListView);
	void (*pfnIdle)(struct SmartListView *pListView);
	int nNumColumns;
	const char **ppColumnNames;
};

struct SmartListViewOptions {
	const struct SmartListViewClass *pClass;
	HWND hwndParent;
	int nIDDlgItem;
	HBITMAP hBackground;
	HPALETTE hPALbg;
	MYBITMAPINFO bmDesc;
	COLORREF rgbListFontColor;
	HIMAGELIST hSmall;
	HIMAGELIST hLarge;
	BOOL bOldControl;
	BOOL bCenterOnParent;
	int nInsetPixels;
};

struct RowMapping {
	int nVisualToLogical;
	int nLogicalToVisual;
};

struct SmartListView {
	/* The underlying list view */
	HWND hwndListView;

	/* These come from the initial options */
	const struct SmartListViewClass *pClass;
	HWND hwndParent;
	int nIDDlgItem;
	HBITMAP hBackground;
	HPALETTE hPALbg;
	MYBITMAPINFO bmDesc;
	COLORREF rgbListFontColor;
	BOOL bOldControl;

	/* These are used at runtime */
	int nNumRows;
	struct RowMapping *rowMapping;
	int *piRealColumns;
	int nSortCondition;
};

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

int Compare_TextCaseInsensitive(struct SmartListView *pListView, int nRow1, int nRow2, int nColumn);
