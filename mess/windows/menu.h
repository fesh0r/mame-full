#ifndef MENU_H
#define MENU_H

#include <windows.h>

extern int win_use_natural_keyboard;

HMENU win_create_menus(void);
LRESULT win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);

#endif /* MENU_H */
