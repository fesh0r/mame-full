#ifndef PROPERTIESMS_H
#define PROPERTIESMS_H

#include "ui/properties.h"

INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PropSheetFilter_Config(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv);

#endif /* PROPERTIESMS_H */
