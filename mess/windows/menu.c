#include <windows.h>

#include "mame.h"
#include "../../src/windows/window.h"
#include "menu.h"
#include "messres.h"
#include "inputx.h"
#include "video.h"
#include "snprintf.h"

int win_use_natural_keyboard;
static HMENU win_menu_bar;

#define FRAMESKIP_LEVELS			12

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
//	find_submenu
//============================================================

static HMENU find_sub_menu(HMENU menu, LPCTSTR menutext)
{
	MENUITEMINFO mii;
	int item_count;
	int i;
	TCHAR buf[128];

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
	mii.dwTypeData = buf;

	while(*menutext)
	{
		item_count = GetMenuItemCount(menu);
		for(i = 0; i < item_count; i++)
		{
			mii.dwTypeData = buf;
			mii.cch = sizeof(buf) / sizeof(buf[0]);
			if (!GetMenuItemInfo(menu, i, TRUE, &mii))
				return NULL;
			if (mii.dwTypeData && !strcmp(menutext, mii.dwTypeData))
				break;
		}
		if (i >= item_count)
			return NULL;
		menu = mii.hSubMenu;
		if (!menu)
			return NULL;
		menutext += strlen(menutext) + 1;
	}
	return menu;
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
	int i;
	const struct IODevice *dev;
	TCHAR buf[32];
	const char *s;
	HMENU device_menu;

	set_command_state(ID_EDIT_PASTE,		inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);
	set_command_state(ID_OPTIONS_KEYBOARD,	inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);
	set_command_state(ID_KEYBOARD_EMULATED,	!win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);
	set_command_state(ID_KEYBOARD_NATURAL,	win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);

	set_command_state(ID_OPTIONS_THROTTLE,	throttle					? MFS_CHECKED : MFS_ENABLED);
	set_command_state(ID_FRAMESKIP_AUTO,	autoframeskip				? MFS_CHECKED : MFS_ENABLED);
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
		set_command_state(ID_FRAMESKIP_0 + i, (!autoframeskip && (frameskip == i)) ? MFS_CHECKED : MFS_ENABLED);

	// set up device menu
	device_menu = find_sub_menu(win_menu_bar, "&Devices\0");
	while(GetMenuItemCount(device_menu) > 0)
		RemoveMenu(device_menu, 0, MF_BYPOSITION);

	for (dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		for (i = 0; i < dev->count; i++)
		{
			s = image_exists(dev->type, i) ? image_filename(dev->type, i) : "<empty>";
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s: %s", device_typename_id(dev->type, i), s);
			AppendMenu(device_menu, MF_STRING, 0, buf);
		}
	}
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

	case ID_OPTIONS_RESET:
		machine_reset();
		break;

	case ID_OPTIONS_THROTTLE:
		throttle = !throttle;
		break;

	case ID_FRAMESKIP_AUTO:
		autoframeskip = 1;
		break;

	case ID_FRAMESKIP_0:
	case ID_FRAMESKIP_1:
	case ID_FRAMESKIP_2:
	case ID_FRAMESKIP_3:
	case ID_FRAMESKIP_4:
	case ID_FRAMESKIP_5:
	case ID_FRAMESKIP_6:
	case ID_FRAMESKIP_7:
	case ID_FRAMESKIP_8:
	case ID_FRAMESKIP_9:
	case ID_FRAMESKIP_10:
	case ID_FRAMESKIP_11:
		frameskip = command - ID_FRAMESKIP_0;
		autoframeskip = 0;
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
	HMENU menu_bar;
	HMENU frameskip_menu;
	HMODULE module;
	TCHAR buf[32];
	int i;
	
	module = GetModuleHandle(EMULATORDLL);
	menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
	if (!menu_bar)
		return NULL;

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0");
	if (!frameskip_menu)
		return NULL;
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%i", i);
		AppendMenu(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf);
	}

	win_menu_bar = menu_bar;
	return menu_bar;
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

