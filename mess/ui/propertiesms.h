#ifndef PROPERTIESMS_H
#define PROPERTIESMS_H

#include "ui/properties.h"

void MessOptionsToProp(int nGame, HWND hWnd, options_type *o);
BOOL MessPropertiesCommand(int nGame, HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed);
void MessPropToOptions(int nGame, HWND hWnd, options_type *o);
void MessSetPropEnabledControls(HWND hWnd, options_type *o);

INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PropSheetFilter_Config(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv);

#endif /* PROPERTIESMS_H */
