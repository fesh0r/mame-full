//============================================================
//
//	guimain.c - Win32 GUI entry point
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int __declspec(dllimport) WINAPI GuiMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow);

int WINAPI WinMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	return GuiMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
