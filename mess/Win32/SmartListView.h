#include <windows.h>
#include <commctrl.h>
#include "Screenshot.h"

struct SmartListView;

struct SmartListViewClass {
	void (*pfnRun)(struct SmartListView *pListView);
	BOOL (*pfnItemChanged)(struct SmartListView *pListView, NM_LISTVIEW *pNm);
	int (*pfnWhichIcon)(struct SmartListView *pListView, int nItem);
	LPCSTR (*pfnGetText)(struct SmartListView *pListView, int nRow, int nColumn);
	void (*pfnGetColumnInfo)(struct SmartListView *pListView, int *pShown, int *pOrder, int *pWidths);
	BOOL (*pfnIsItemSelected)(struct SmartListView *pListView, int nItem);
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

struct SmartListView {
	const struct SmartListViewClass *pClass;
	HWND hwndListView;
	HWND hwndParent;
	int nIDDlgItem;
	HBITMAP hBackground;
	HPALETTE hPALbg;
	MYBITMAPINFO bmDesc;
	COLORREF rgbListFontColor;
	BOOL bOldControl;
	int *piRealColumns;
};

struct SmartListView *SmartListView_Init(struct SmartListViewOptions *pOptions);
void SmartListView_Free(struct SmartListView *pListView);
BOOL SmartListView_IsEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam);
BOOL SmartListView_HandleEvent(struct SmartListView *pListView, UINT message, UINT wParam, LONG lParam);
void SmartListView_ResetColumnDisplay(struct SmartListView *pListView);
void SmartListView_SetTotalItems(struct SmartListView *pListView, int nItemCount);
void SmartListView_InsertItem(struct SmartListView *pListView, int nItem);
void SmartListView_SelectItem(struct SmartListView *pListView, int nItem, BOOL bFocus);
void SmartListView_RedrawItems(struct SmartListView *pListView, int nBeginItem, int nEndItem);
void SmartListView_Update(struct SmartListView *pListView, int nItem);
BOOL SmartListView_CanIdle(struct SmartListView *pListView);
void SmartListView_Idle(struct SmartListView *pListView);
void SmartListView_IdleUntilMsg(struct SmartListView *pListView);
