//============================================================
//
//	wmain.h - Win32 GUI Imgtool main code
//
//============================================================

#include <windows.h>
#include <commctrl.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "../modules.h"

imgtool_library *library;


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPSTR command_line, int cmd_show)
{
	MSG msg;
	HWND window;
	BOOL b;
	int rc = -1;
	imgtoolerr_t err;
	HACCEL accel = NULL;
	

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

	accel = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_WIMGTOOL_MENU));

	// pump messages until the window is gone
	while(IsWindow(window))
	{
		b = GetMessage(&msg, NULL, 0, 0);
		if (b <= 0)
		{
			window = NULL;
		}
		else if (!TranslateAccelerator(window, accel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	rc = 0;

done:
	if (library)
		imgtool_library_close(library);
	if (accel)
		DestroyAcceleratorTable(accel);
	return rc;
}
