#include "mame.h"
#include "mamece.h"
#include "cemenu.h"
#include "driver.h"
#include "menu.h"
#include "messres.h"

int wince_create_menus(HWND main_window)
{
	SHMENUBARINFO menubar;
	HMENU menu;

	memset(&menubar, 0, sizeof(menubar));
	menubar.cbSize = sizeof(menubar);
	menubar.hwndParent = main_window;
	menubar.dwFlags = (Machine->gamedrv->flags & GAME_COMPUTER) ? SHCMBF_HIDESIPBUTTON : SHCMBF_HIDESIPBUTTON;
	menubar.hInstRes = GetModuleHandle(NULL);
	menubar.nBmpId = 0;
	menubar.cBmpImages = 0;
	menubar.nToolBarId = IDR_RUNTIME_MENU;

	SHCreateMenuBar(&menubar);
	menu = (HMENU) SendMessage(menubar.hwndMB, SHCMBM_GETMENU, 0, 0);
	if (win_setup_menus(menu))
		return 1;
	return 0;
}

