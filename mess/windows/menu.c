#include <windows.h>

#include "mame.h"
#include "../../src/windows/window.h"
#include "menu.h"
#include "messres.h"
#include "inputx.h"

int win_use_natural_keyboard;
static HMENU win_menu_bar;

//============================================================
//	paste
//============================================================

static void paste(void)
{
	HANDLE h;
	LPSTR text;
	size_t mb_size;
	size_t w_size;
	LPWSTR wtext;

	if (!OpenClipboard(NULL))
		return;
	
	h = GetClipboardData(CF_TEXT);
	if (h)
	{
		text = GlobalLock(h);
		if (text)
		{
			mb_size = GlobalSize(h);
			w_size = MultiByteToWideChar(CP_ACP, 0, text, mb_size, NULL, 0);
			wtext = alloca(w_size * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, text, mb_size, wtext, w_size);
			inputx_postn_utf16(wtext, w_size);
			GlobalUnlock(h);
		}
	}

	CloseClipboard();
}

//============================================================
//	set_command_state
//============================================================

static void set_command_state(UINT command, UINT state)
{
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	SetMenuItemInfo(win_menu_bar, command, FALSE, &mii);
}

//============================================================
//	prepare_menus
//============================================================

static void prepare_menus(void)
{
	set_command_state(ID_EDIT_PASTE,		inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);
	set_command_state(ID_OPTIONS_KEYBOARD,	inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);
	set_command_state(ID_KEYBOARD_EMULATED,	!win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);
	set_command_state(ID_KEYBOARD_NATURAL,	win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);
}

//============================================================
//	invoke_command
//============================================================

static int invoke_command(UINT command)
{
	int handled = 1;

	switch(command) {
	case ID_FILE_EXIT:
		PostMessage(win_video_window, WM_DESTROY, 0, 0);
		break;

	case ID_EDIT_PASTE:
		paste();
		break;

	case ID_KEYBOARD_NATURAL:
		win_use_natural_keyboard = 1;
		break;

	case ID_KEYBOARD_EMULATED:
		win_use_natural_keyboard = 0;
		break;

	case ID_HELP_ABOUT:
		MessageBox(win_video_window, TEXT("MESS"), TEXT("MESS"), MB_OK);
		break;

	default:
		handled = 0;
		break;
	}
	return handled;
}

//============================================================
//	win_create_menus
//============================================================

HMENU win_create_menus(void)
{
	HMODULE module = GetModuleHandle(EMULATORDLL);
	win_menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
	return win_menu_bar;
}

//============================================================
//	win_mess_window_proc
//============================================================

LRESULT win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
	case WM_INITMENU:
		prepare_menus();
		break;

	case WM_CHAR:
		if (win_use_natural_keyboard)
			inputx_postc(wparam);
		break;

	case WM_COMMAND:
		if (invoke_command(wparam))
			break;
		/* fall through */

	default:
		return DefWindowProc(wnd, message, wparam, lparam);
	}
	return 0;
}

