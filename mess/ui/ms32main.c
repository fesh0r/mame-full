#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui/screenshot.h"
#include "ui/win32ui.h"


int __declspec(dllexport) WINAPI GuiMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	return Mame32Main(hInstance, lpCmdLine, nCmdShow);
}
