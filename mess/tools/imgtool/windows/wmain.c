//============================================================
//
//	wmain.h - Win32 GUI Imgtool main code
//
//============================================================

#include <windows.h>
#include <commctrl.h>

#include "wimgtool.h"
#include "../modules.h"

imgtool_library *library;


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPSTR command_line, int cmd_show)
{
	MSG msg;
	HWND window;
	int rc = -1;
	imgtoolerr_t err;

	// Initialize Windows classes
	InitCommonControls();
	if (!wimgtool_registerclass())
		goto done;

	// Initialize the Imgtool library
	err = imgtool_create_cannonical_library(&library);
	if (!library)
		goto done;

	window = CreateWindow(wimgtool_class, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	if (!window)
		goto done;

	// pump messages until the window is gone
	while(IsWindow(window) && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	rc = 0;

done:
	if (library)
		imgtool_library_close(library);
	return rc;
}
