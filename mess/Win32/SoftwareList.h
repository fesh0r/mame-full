#ifndef SOFTWARELIST_H
#define SOFTWARELIST_H

#include "SmartListView.h"

/* SoftwareListView Class calls */
LPCTSTR SoftwareList_GetText(struct SmartListView *pListView, int nRow, int nColumn);
BOOL SoftwareList_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
BOOL SoftwareList_IsItemSelected(struct SmartListView *pListView, int nItem);

/* External calls */
void FillSoftwareList(struct SmartListView *pSoftwareListView, int nGame, int nBasePaths, LPCSTR *plpBasePaths, LPCSTR lpExtraPath);

#endif /* SOFTWARELIST_H */
