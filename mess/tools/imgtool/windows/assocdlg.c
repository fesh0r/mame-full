#include "assoc.h"
#include "../imgtool.h"

static INT_PTR CALLBACK win_association_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	imgtool_library *library;

	switch(message)
	{
		case WM_INITDIALOG:
			library = (imgtool_library *) lparam;
			break;
	}
	return rc;
}



void win_association_dialog(HWND parent, imgtool_library *library)
{
	DLGTEMPLATE dlgtemp;

	memset(&dlgtemp, 0, sizeof(dlgtemp));
	dlgtemp.style = DS_CENTER;
	dlgtemp.cx = 100;
	dlgtemp.cy = 100;
	DialogBoxIndirectParam(GetModuleHandle(NULL), &dlgtemp, parent,
		win_association_dialog_proc, (LPARAM) library);
	GetLastError();
}
