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
#include "ui_text.h"

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
#define ID_JOYSTICK_0				12000

#define MAX_JOYSTICKS				((IPF_PLAYERMASK / IPF_PLAYER2) + 1)

enum
{
	DEVOPTION_MOUNT,
	DEVOPTION_UNMOUNT,
	DEVOPTION_CASSETTE_PLAYRECORD,
	DEVOPTION_CASSETTE_STOPPAUSE,
	DEVOPTION_CASSETTE_REWIND,
	DEVOPTION_CASSETTE_FASTFORWARD,
	DEVOPTION_MAX
};

#ifdef MAME_DEBUG
#define HAS_PROFILER	1
#else
#define HAS_PROFILER	0
#endif

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_use_natural_keyboard;
char *win_state_hack;

#if HAS_PROFILER
extern int show_profiler;
#endif


//============================================================
//	LOCAL VARIABLES
//============================================================

static HMENU win_menu_bar;
static int is_paused;


//============================================================
//	is_controller_input_type
//============================================================

int is_controller_input_type(UINT32 type)
{
	int result;
	switch(type & ~IPF_MASK) {
	case IPT_JOYSTICK_UP:
	case IPT_JOYSTICK_DOWN:
	case IPT_JOYSTICK_LEFT:
	case IPT_JOYSTICK_RIGHT:
	case IPT_JOYSTICKLEFT_UP:
	case IPT_JOYSTICKLEFT_DOWN:
	case IPT_JOYSTICKLEFT_LEFT:
	case IPT_JOYSTICKLEFT_RIGHT:
	case IPT_JOYSTICKRIGHT_UP:
	case IPT_JOYSTICKRIGHT_DOWN:
	case IPT_JOYSTICKRIGHT_LEFT:
	case IPT_JOYSTICKRIGHT_RIGHT:
	case IPT_BUTTON1:
	case IPT_BUTTON2:
	case IPT_BUTTON3:
	case IPT_BUTTON4:
	case IPT_BUTTON5:
	case IPT_BUTTON6:
	case IPT_BUTTON7:
	case IPT_BUTTON8:
	case IPT_BUTTON9:
	case IPT_BUTTON10:
	case IPT_AD_STICK_X:
	case IPT_AD_STICK_Y:
		result = 1;
		break;

	default:
		result = 0;
		break;
	}
	return result;
}

//============================================================
//	setjoystick
//============================================================

static void setjoystick(int joystick_num)
{
	void *dlg;
	int player;
	struct InputPort *in;
	int increment;

	player = joystick_num * IPF_PLAYER2;
	
	dlg = win_dialog_init("Joysticks/Controllers");
	if (!dlg)
		goto done;

	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		increment = 1;
		if (((in->type & IPF_PLAYERMASK) == player) && is_controller_input_type(in->type))
		{
			if (win_dialog_add_portselect(dlg, in, &increment))
				goto done;
		}
		in += increment;
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}

//============================================================
//	setdipswitches
//============================================================

static void setdipswitches(void)
{
	void *dlg;
	struct InputPort *in;
	const char *dipswitch_name = NULL;
	
	dlg = win_dialog_init(ui_getstring(UI_dipswitches));
	if (!dlg)
		goto done;

	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		switch(in->type & ~IPF_MASK) {
		case IPT_DIPSWITCH_NAME:
			if ((in->type & IPF_UNUSED) == 0 && !(!options.cheat && (in->type & IPF_CHEAT)))
			{
				dipswitch_name = input_port_name(in);
				if (win_dialog_add_combobox(dlg, dipswitch_name, &in->default_value))
					goto done;
			}
			else
			{
				dipswitch_name = NULL;
			}
			break;

		case IPT_DIPSWITCH_SETTING:
			if (dipswitch_name)
			{
				if (win_dialog_add_combobox_item(dlg, input_port_name(in), in->default_value))
					goto done;
			}
			break;
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
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

static HMENU find_sub_menu(HMENU menu, LPCTSTR menutext, int create_sub_menu)
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
		if (!mii.hSubMenu && create_sub_menu)
		{
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_SUBMENU;
			mii.hSubMenu = CreateMenu();
			if (!SetMenuItemInfo(menu, i, TRUE, &mii))
			{
				i = GetLastError();
				return NULL;
			}
		}
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

static void set_command_state(HMENU menu_bar, UINT command, UINT state)
{
	MENUITEMINFO mii;
	BOOL result;
	int err;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	result = SetMenuItemInfo(menu_bar, command, FALSE, &mii);
	if (!result)
	{
		err = GetLastError();
		assert(FALSE);
	}
}

//============================================================
//	append_menu
//============================================================

static void append_menu(HMENU menu, UINT flags, UINT_PTR id, int uistring)
{
	LPCTSTR str;
	str = (uistring >= 0) ? ui_getstring(uistring) : NULL;
	AppendMenu(menu, flags, id, str);
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
	HMENU sub_menu;
	UINT_PTR new_item;
	UINT flags_for_exists;
	int status;

	if (!win_menu_bar)
		return;

	set_command_state(win_menu_bar, ID_EDIT_PASTE,			inputx_can_post()			? MFS_ENABLED : MFS_GRAYED);

	set_command_state(win_menu_bar, ID_OPTIONS_PAUSE,		is_paused					? MFS_CHECKED : MFS_ENABLED);
	set_command_state(win_menu_bar, ID_OPTIONS_THROTTLE,	throttle					? MFS_CHECKED : MFS_ENABLED);
	set_command_state(win_menu_bar, ID_OPTIONS_FULLSCREEN,	!win_window_mode			? MFS_CHECKED : MFS_ENABLED);
#if HAS_PROFILER
	set_command_state(win_menu_bar, ID_OPTIONS_PROFILER,	show_profiler				? MFS_CHECKED : MFS_ENABLED);
#endif

	set_command_state(win_menu_bar, ID_KEYBOARD_EMULATED,	!win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED);
	set_command_state(win_menu_bar, ID_KEYBOARD_NATURAL,	inputx_can_post() ?
															(win_use_natural_keyboard	? MFS_CHECKED : MFS_ENABLED)
																						: MFS_GRAYED);

	set_command_state(win_menu_bar, ID_FRAMESKIP_AUTO,		autoframeskip				? MFS_CHECKED : MFS_ENABLED);
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
		set_command_state(win_menu_bar, ID_FRAMESKIP_0 + i, (!autoframeskip && (frameskip == i)) ? MFS_CHECKED : MFS_ENABLED);

	// set up device menu
	device_menu = find_sub_menu(win_menu_bar, "&Devices\0", FALSE);
	while(GetMenuItemCount(device_menu) > 0)
		RemoveMenu(device_menu, 0, MF_BYPOSITION);

	for (dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		for (i = 0; i < dev->count; i++)
		{
			new_item = ID_DEVICE_0 + ((dev->type * MAX_DEV_INSTANCES) + i) * DEVOPTION_MAX;
			flags_for_exists = MF_STRING;
			if (!image_exists(dev->type, i))
				flags_for_exists |= MF_GRAYED;

			sub_menu = CreateMenu();
			append_menu(sub_menu, MF_STRING,		new_item + DEVOPTION_MOUNT,	UI_mount);
			append_menu(sub_menu, flags_for_exists,	new_item + DEVOPTION_UNMOUNT,	UI_unmount);

			if (dev->type == IO_CASSETTE)
			{
				status = device_status(IO_CASSETTE, i, -1);
				append_menu(sub_menu, MF_SEPARATOR, 0, -1);
				append_menu(sub_menu, flags_for_exists | (status & WAVE_STATUS_MOTOR_ENABLE) ? 0 : MF_CHECKED,	new_item + DEVOPTION_CASSETTE_STOPPAUSE,	UI_pauseorstop);
				append_menu(sub_menu, flags_for_exists | (status & WAVE_STATUS_MOTOR_ENABLE) ? MF_CHECKED : 0,	new_item + DEVOPTION_CASSETTE_PLAYRECORD,	(status & WAVE_STATUS_WRITE_ONLY) ? UI_record : UI_play);
				append_menu(sub_menu, flags_for_exists,															new_item + DEVOPTION_CASSETTE_REWIND,		UI_rewind);
				append_menu(sub_menu, flags_for_exists,															new_item + DEVOPTION_CASSETTE_FASTFORWARD,	UI_fastforward);
			}
			s = image_exists(dev->type, i) ? image_filename(dev->type, i) : ui_getstring(UI_emptyslot);

			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s: %s", device_typename_id(dev->type, i), s);
			AppendMenu(device_menu, MF_POPUP, (UINT_PTR) sub_menu, buf);
		}
	}
}


//============================================================
//	win_toggle_menubar
//============================================================

void win_toggle_menubar(void)
{
	SetMenu(win_video_window, GetMenu(win_video_window) ? NULL : win_menu_bar);
	
	if (win_window_mode)
	{
		RECT window;
		GetWindowRect(win_video_window, &window);
		win_constrain_to_aspect_ratio(&window, WMSZ_BOTTOM);
		SetWindowPos(win_video_window, HWND_TOP, window.left, window.top,
			window.right - window.left, window.bottom - window.top, SWP_NOZORDER);
	}

	win_adjust_window();
	RedrawWindow(win_video_window, NULL, NULL, 0);
}


//============================================================
//	device_command
//============================================================

static void device_command(const struct IODevice *dev, int id, int devoption)
{
	int status;

	switch(devoption) {
	case DEVOPTION_MOUNT:
		change_device(dev, id);
		break;

	case DEVOPTION_UNMOUNT:
		image_unload(dev->type, id);
		break;

	default:
		switch(dev->type) {
		case IO_CASSETTE:
			status = device_status(IO_CASSETTE, id, -1);
			switch(devoption) {
			case DEVOPTION_CASSETTE_PLAYRECORD:
				device_status(IO_CASSETTE, id, status | WAVE_STATUS_MOTOR_ENABLE);
				break;

			case DEVOPTION_CASSETTE_STOPPAUSE:
				if ((status & 1) == 0)
					device_seek(IO_CASSETTE,id,0,SEEK_SET);
				device_status(IO_CASSETTE, id, status & ~WAVE_STATUS_MOTOR_ENABLE);
				break;

			case DEVOPTION_CASSETTE_REWIND:
				device_seek(IO_CASSETTE, id, -11025, SEEK_CUR);
				break;

			case DEVOPTION_CASSETTE_FASTFORWARD:
				device_seek(IO_CASSETTE, id, +11025, SEEK_CUR);
				break;
			}

		}
	}
}

//============================================================
//	help_display
//============================================================

static void help_display(LPCTSTR chapter)
{
	typedef HWND (WINAPI *htmlhelpproc)(HWND hwndCaller, LPCTSTR pszFile, UINT uCommand, DWORD_PTR dwData);
	static htmlhelpproc htmlhelp;
	static DWORD htmlhelp_cookie;

	if (htmlhelp == NULL)
	{
		htmlhelp = (htmlhelpproc) GetProcAddress(LoadLibrary(TEXT("hhctrl.ocx")), TEXT("HtmlHelpA"));
		if (!htmlhelp)
			return;
		htmlhelp(NULL, NULL, 28 /*HH_INITIALIZE*/, (DWORD_PTR) &htmlhelp_cookie);
	}

	// if full screen, turn it off
	if (!win_window_mode)
		win_toggle_full_screen();

	htmlhelp(win_video_window, chapter, 0 /*HH_DISPLAY_TOPIC*/, 0);
}

//============================================================
//	help_about_mess
//============================================================

static void help_about_mess(void)
{
	help_display(TEXT("mess.chm::/html/mess_overview.htm"));
}

//============================================================
//	help_about_thissystem
//============================================================

static void help_about_thissystem(void)
{
	TCHAR buf[256];
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "mess.chm::/sysinfo/%s.htm", Machine->gamedrv->name);
	help_display(buf);
}

//============================================================
//	invoke_command
//============================================================

static int invoke_command(UINT command)
{
	const struct IODevice *dev;
	int handled = 1;
	int dev_id, dev_command;

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

#if HAS_PROFILER
	case ID_OPTIONS_PROFILER:
		show_profiler ^= 1;
		if (show_profiler)
			profiler_start();
		else
		{
			profiler_stop();
			schedule_full_refresh();
		}
		break;
#endif

	case ID_OPTIONS_DIPSWITCHES:
		setdipswitches();
		break;

	case ID_OPTIONS_FULLSCREEN:
		win_toggle_full_screen();
		break;

	case ID_OPTIONS_TOGGLEMENUBAR:
		win_toggle_menubar();
		break;

	case ID_FRAMESKIP_AUTO:
		autoframeskip = 1;
		break;

	case ID_HELP_ABOUT:
		help_about_mess();
		break;

	case ID_HELP_ABOUTSYSTEM:
		help_about_thissystem();
		break;

	default:
		if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + FRAMESKIP_LEVELS))
		{
			frameskip = command - ID_FRAMESKIP_0;
			autoframeskip = 0;
		}
		else if ((command >= ID_DEVICE_0) && (command < ID_DEVICE_0 + (MAX_DEV_INSTANCES*IO_COUNT*DEVOPTION_MAX)))
		{
			command -= ID_DEVICE_0;
			dev = device_find(Machine->gamedrv, command / MAX_DEV_INSTANCES / DEVOPTION_MAX);
			dev_id = (command / DEVOPTION_MAX) % MAX_DEV_INSTANCES;
			dev_command = command % DEVOPTION_MAX;
			device_command(dev, dev_id, dev_command);
		}
		else if ((command >= ID_JOYSTICK_0) && (command < ID_JOYSTICK_0 + MAX_JOYSTICKS))
		{
			setjoystick(command - ID_JOYSTICK_0);
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
//	count_joysticks
//============================================================

static int count_joysticks(void)
{
	const struct InputPortTiny *in;
	int joystick_count;

	assert(MAX_JOYSTICKS > 0);
	assert(MAX_JOYSTICKS < 8);

	joystick_count = 0;
	for (in = Machine->gamedrv->input_ports; in->type != IPT_END; in++)
	{
		if (is_controller_input_type(in->type))
		{
			if (joystick_count <= (in->type & IPF_PLAYERMASK) / IPF_PLAYER2)
				joystick_count = (in->type & IPF_PLAYERMASK) / IPF_PLAYER2 + 1;
		}
	}
	return joystick_count;
}

//============================================================
//	win_create_menus
//============================================================

HMENU win_create_menus(void)
{
	HMENU menu_bar;
	HMENU frameskip_menu;
	HMENU joystick_menu;
	HMODULE module;
	TCHAR buf[256];
	int i, joystick_count;
	MENUITEMINFO mii;

	assert((ID_DEVICE_0 + IO_COUNT * MAX_DEV_INSTANCES * DEVOPTION_MAX) < ID_JOYSTICK_0);
	is_paused = 0;
	
	module = GetModuleHandle(EMULATORDLL);
	menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
	if (!menu_bar)
		return NULL;

	// remove the profiler menu item if it doesn't exist
#if HAS_PROFILER
	show_profiler = 0;
#else
	DeleteMenu(menu_bar, ID_OPTIONS_PROFILER, MF_BYCOMMAND);
#endif

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0", FALSE);
	if (!frameskip_menu)
		return NULL;
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%i", i);
		AppendMenu(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf);
	}

	// set up joystick menu
	joystick_count = count_joysticks();
	set_command_state(menu_bar, ID_OPTIONS_JOYSTICKS, joystick_count ? MFS_ENABLED : MFS_GRAYED);
	if (joystick_count > 0)
	{
		joystick_menu = find_sub_menu(menu_bar, "&Options\0&Joysticks\0", TRUE);
		if (!joystick_menu)
			return NULL;
		for(i = 0; i < joystick_count; i++)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "Joystick %i", i + 1);
			AppendMenu(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, buf);
		}
	}

	// set the help menu to refer to this machine
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "About %s (%s)...", Machine->gamedrv->description, Machine->gamedrv->name);
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = buf;
	SetMenuItemInfo(menu_bar, ID_HELP_ABOUTSYSTEM, FALSE, &mii);	

	win_menu_bar = menu_bar;
	return menu_bar;
}

//============================================================
//	win_mess_window_proc
//============================================================

LRESULT win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	extern void win_timer_enable(int enabled);

	switch(message) {
	case WM_INITMENU:
		prepare_menus();
		break;

	case WM_CHAR:
		if (win_use_natural_keyboard)
			inputx_postc(wparam);
		break;

	// suspend sound and timer if we are resizing or a menu is coming up
	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
		osd_sound_enable(0);
		win_timer_enable(0);
		break;

	// resume sound and timer if we dome with resizing or a menu
	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		osd_sound_enable(1);
		win_timer_enable(1);
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

//============================================================
//	osd_keyboard_disabled
//============================================================

int osd_keyboard_disabled(void)
{
	return win_use_natural_keyboard;
}

//============================================================
//	osd_trying_to_quit
//============================================================

int osd_trying_to_quit(void)
{
	return win_trying_to_quit;
}


