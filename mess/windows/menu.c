//============================================================
//
//	menu.c - Win32 MESS menus handling
//
//============================================================

#include <windows.h>

#include "mame.h"
#include "../../src/windows/window.h"
#include "menu.h"
#include "messres.h"
#include "inputx.h"
#include "video.h"
#include "snprintf.h"
#include "dialog.h"

//============================================================
//	IMPORTS
//============================================================

// from input.c
extern UINT8 win_trying_to_quit;


//============================================================
//	PARAMETERS
//============================================================

#define FRAMESKIP_LEVELS			12
#define ID_FRAMESKIP_0				10000
#define ID_DEVICE_0					11000

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_use_natural_keyboard;
char *win_state_hack;


//============================================================
//	LOCAL VARIABLES
//============================================================

static HMENU win_menu_bar;
static int is_paused;


//============================================================
//	dipswitches
//============================================================

static void dipswitches(void)
{
	void *dlg;
	
	dlg = win_dialog_init(L"DIP Switches");
	if (dlg)
	{
		win_dialog_add_label(dlg, L"Foo");
		win_dialog_runmodal(dlg);
		win_dialog_exit(dlg);
	}
}

//============================================================
//	loadsave
//============================================================

static void loadsave(int type)
{
	static char filename[MAX_PATH];
	OPENFILENAME ofn;
	char *dir;
	int result = 0;

	if (filename[0])
		dir = osd_dirname(filename);
	else
	{
		sprintf(filename, "machinestate.sta");
		dir = NULL;
	}

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = win_video_window;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrFilter = "State Files (*.sta)\0*.sta\0All Files (*.*);*.*\0";
	ofn.lpstrFile = filename;
	ofn.lpstrInitialDir = dir;
	ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);

	switch(type) {
	case LOADSAVE_LOAD:
		ofn.Flags |= OFN_FILEMUSTEXIST;
		result = GetOpenFileName(&ofn);
		break;

	case LOADSAVE_SAVE:
		result = GetSaveFileName(&ofn);
		break;

	default:
		assert(0);
		break;
	}

	if (result)
	{
		win_state_hack = filename;
		cpu_loadsave_schedule(type, '\1');
	}
	if (dir)
		free(dir);
}

//============================================================
//	change_device
//============================================================

static void change_device(const struct IODevice *dev, int id)
{
	OPENFILENAME ofn;
	TCHAR filter[2048];
	TCHAR filename[MAX_PATH];
	TCHAR *s;
	const char *ext;

	assert(dev);

	s = filter;
	s += sprintf(s, "Common image types (");
	ext = dev->file_extensions;
	while(*ext)
	{
		s += sprintf(s, "*.%s;", ext);
		ext += strlen(ext) + 1;
	}
	s += sprintf(s, "*.zip)");
	s++;
	ext = dev->file_extensions;
	while(*ext)
	{
		s += sprintf(s, "*.%s;", ext);
		ext += strlen(ext) + 1;
	}
	s += sprintf(s, "*.zip");
	s++;

	// All files
	s += sprintf(s, "All files (*.*)") + 1;
	s += sprintf(s, "*.*") + 1;

	// Compressed
	s += sprintf(s, "Compressed Images (*.zip)") + 1;
	s += sprintf(s, "*.zip") + 1;

	*(s++) = '\0';

	if (image_exists(dev->type, id))
		snprintf(filename, sizeof(filename) / sizeof(filename[0]), image_basename(dev->type, id));
	else
		filename[0] = '\0';

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = win_video_window;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.lpstrInitialDir = image_filedir(dev->type, id);
	ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	switch(dev->open_mode) {
	case OSD_FOPEN_WRITE:
	case OSD_FOPEN_RW_CREATE:
	case OSD_FOPEN_RW_CREATE_OR_READ:
		ofn.Flags |= OFN_FILEMUSTEXIST;
		break;

	case OSD_FOPEN_READ:
	case OSD_FOPEN_RW:
	case OSD_FOPEN_RW_OR_READ:
	case OSD_FOPEN_READ_OR_WRITE:
	default:
		ofn.Flags |= 0;
		break;
	}

	if (!GetOpenFileName(&ofn))
		return;

	image_load(dev->type, id, filename);
}

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
//	pause
//============================================================

static void pause(void)
{
	if (is_paused)
	{
		is_paused = 0;
	}
	else
	{
		is_paused = 1;
		mame_pause(1);
		while(is_paused && !win_trying_to_quit)
		{
			draw_screen();
			update_video_and_audio();
			reset_partial_updates();
		}
		mame_pause(0);
	}
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
	TCHAR buf[MAX_PATH];
	const char *s;
	HMENU device_menu;

	set_command_state(ID_EDIT_PASTE,		inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);

	set_command_state(ID_OPTIONS_PAUSE,		is_paused					? MFS_CHECKED : MFS_ENABLED);
	set_command_state(ID_OPTIONS_THROTTLE,	throttle					? MFS_CHECKED : MFS_ENABLED);

	set_command_state(ID_OPTIONS_KEYBOARD,	inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);
	set_command_state(ID_KEYBOARD_EMULATED,	!win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);
	set_command_state(ID_KEYBOARD_NATURAL,	win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);

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
			AppendMenu(device_menu, MF_STRING, ID_DEVICE_0 + (dev->type * MAX_DEV_INSTANCES) + i, buf);
		}
	}
}

//============================================================
//	invoke_command
//============================================================

static int invoke_command(UINT command)
{
	const struct IODevice *dev;
	int handled = 1;

	switch(command) {
	case ID_FILE_LOADSTATE:
		loadsave(LOADSAVE_LOAD);
		break;

	case ID_FILE_SAVESTATE:
		loadsave(LOADSAVE_SAVE);
		break;

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

	case ID_OPTIONS_PAUSE:
		pause();
		break;

	case ID_OPTIONS_RESET:
		machine_reset();
		break;

	case ID_OPTIONS_THROTTLE:
		throttle = !throttle;
		break;

	case ID_OPTIONS_DIPSWITCHES:
		dipswitches();
		break;

	case ID_FRAMESKIP_AUTO:
		autoframeskip = 1;
		break;

	case ID_HELP_ABOUT:
		MessageBox(win_video_window, TEXT("MESS"), TEXT("MESS"), MB_OK);
		break;

	default:
		if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + FRAMESKIP_LEVELS))
		{
			frameskip = command - ID_FRAMESKIP_0;
			autoframeskip = 0;
		}
		else if ((command >= ID_DEVICE_0) && (command < ID_DEVICE_0 + (MAX_DEV_INSTANCES*IO_COUNT)))
		{
			command -= ID_DEVICE_0;
			dev = device_find(Machine->gamedrv, command / MAX_DEV_INSTANCES);
			change_device(dev, command % MAX_DEV_INSTANCES);
		}
		else
		{
			handled = 0;
		}
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

	is_paused = 0;
	
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

