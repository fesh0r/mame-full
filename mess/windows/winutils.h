//============================================================
//
//	winutils.h - Generic Win32 utility code
//
//============================================================

#ifndef WINUTILS_H
#define WINUTILS_H

#include <windows.h>

// UTF-8 wrappers
int win_message_box_utf8(HWND window, const char *text, const char *caption, UINT type);
BOOL win_set_window_text_utf8(HWND window, const char *text);

// Misc
void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line);

#endif // WINUTILS_H
