//============================================================
//
//	menu.c - Win32 MESS menus handling
//
//============================================================

// standard windows headers
#include <windows.h>
#include <commdlg.h>
#include <winuser.h>
#include <ctype.h>

// MAME/MESS headers
#include "mame.h"
#include "windows/window.h"
#include "menu.h"
#include "messres.h"
#include "inputx.h"
#include "video.h"
#include "snprintf.h"
#include "dialog.h"
#include "ui_text.h"
#include "strconv.h"
#include "utils.h"
#include "artwork.h"
#include "tapedlg.h"
#include "artworkx.h"
#include "devices/cassette.h"

#ifdef UNDER_CE
#include "invokegx.h"
#else
#include "configms.h"
#endif

//============================================================
//	IMPORTS
//============================================================

// from input.c
extern UINT8 win_trying_to_quit;

// from mamedbg.c
extern int debug_key_pressed;

// from timer.c
extern void win_timer_enable(int enabled);


//============================================================
//	PARAMETERS
//============================================================

#define FRAMESKIP_LEVELS			12
#define ID_FRAMESKIP_0				10000
#define ID_DEVICE_0					11000
#define ID_JOYSTICK_0				12000

#define MAX_JOYSTICKS				((IPF_PLAYERMASK / IPF_PLAYER2) + 1)

#define USE_TAPEDLG	0

enum
{
	DEVOPTION_OPEN,
	DEVOPTION_CREATE,
	DEVOPTION_CLOSE,
	DEVOPTION_CASSETTE_PLAYRECORD,
	DEVOPTION_CASSETTE_STOPPAUSE,
	DEVOPTION_CASSETTE_PLAY,
	DEVOPTION_CASSETTE_RECORD,
#if USE_TAPEDLG
	DEVOPTION_CASSETTE_DIALOG,
#else
	DEVOPTION_CASSETTE_REWIND,
	DEVOPTION_CASSETTE_FASTFORWARD,
#endif
	DEVOPTION_MAX
};

#ifdef MAME_DEBUG
#define HAS_PROFILER	1
#define HAS_DEBUGGER	1
#else
#define HAS_PROFILER	0
#define HAS_DEBUGGER	0
#endif

#ifdef UNDER_CE
#define HAS_TOGGLEMENUBAR		0
#define HAS_TOGGLEFULLSCREEN	0
#else
#define HAS_TOGGLEMENUBAR		1
#define HAS_TOGGLEFULLSCREEN	1
#endif

#ifdef UNDER_CE
#define WM_INITMENU		WM_INITMENUPOPUP
#define MFS_GRAYED		MF_GRAYED
#define WMSZ_BOTTOM		6
#endif

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_use_natural_keyboard;


//============================================================
//	LOCAL VARIABLES
//============================================================

static HMENU win_menu_bar;
static int is_paused;
static HICON device_icons[IO_COUNT];

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions);


//============================================================
//	customize_input
//============================================================

static void customize_input(const char *title, int cust_type, int player, int category)
{
	dialog_box *dlg;
	struct InputPort *in;
	struct png_info png;
	struct inputform_customization customizations[128];
	RECT r, *pr;
	int i;
	int this_category, this_player;

	artwork_get_inputscreen_customizations(&png, cust_type, customizations, sizeof(customizations) / sizeof(customizations[0]));

	dlg = win_dialog_init(title, NULL);
	if (!dlg)
		goto done;

	if (png.width > 0)
	{
		win_dialog_add_image(dlg, &png);
		win_dialog_add_separator(dlg);
	}

	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		this_category = input_categorize_port(in);
		this_player = input_player_number(in);

		if ((this_player == player) && (this_category == category))
		{
			pr = NULL;
			for (i = 0; customizations[i].ipt != IPT_END; i++)
			{
				if ((in->type & ~IPF_MASK) == customizations[i].ipt)
				{
					r.left = customizations[i].x;
					r.top = customizations[i].y;
					r.right = r.left + customizations[i].width;
					r.bottom = r.top + customizations[i].height;
					pr = &r;
					break;
				}
			}

			if (win_dialog_add_portselect(dlg, in, pr))
				goto done;
		}
		in++;
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//	setjoystick
//============================================================

static void setjoystick(int joystick_num)
{
	customize_input("Joysticks/Controllers", ARTWORK_CUSTTYPE_JOYSTICK, joystick_num, INPUT_CATEGORY_CONTROLLER);
}



//============================================================
//	setkeyboard
//============================================================

static void setkeyboard(void)
{
	customize_input("Emulated Keyboard", ARTWORK_CUSTTYPE_KEYBOARD, 0, INPUT_CATEGORY_KEYBOARD);
}



//============================================================
//	storeval_inputport
//============================================================

static void storeval_inputport(void *param, int val)
{
	struct InputPort *in = (struct InputPort *) param;
	in->default_value = (UINT16) val;
}



//============================================================
//	setswitchmenu
//============================================================

static void setswitchmenu(int title_string_num, UINT32 ipt_name, UINT32 ipt_setting)
{
	void *dlg;
	struct InputPort *in;
	const char *switch_name = NULL;
	UINT32 type;
	
	dlg = win_dialog_init(ui_getstring(title_string_num), NULL);
	if (!dlg)
		goto done;

	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		type = in->type & ~IPF_MASK;

		if (type == ipt_name)
		{
			if ((in->type & IPF_UNUSED) == 0 && !(!options.cheat && (in->type & IPF_CHEAT)))
			{
				switch_name = input_port_name(in);
				if (win_dialog_add_combobox(dlg, switch_name, in->default_value, storeval_inputport, in))
					goto done;
			}
			else
			{
				switch_name = NULL;
			}
		}
		else if (type == ipt_setting)
		{
			if (switch_name)
			{
				if (win_dialog_add_combobox_item(dlg, input_port_name(in), in->default_value))
					goto done;
			}
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
//	setdipswitches
//============================================================

static void setdipswitches(void)
{
	setswitchmenu(UI_dipswitches, IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
}



//============================================================
//	setconfiguration
//============================================================

static void setconfiguration(void)
{
	setswitchmenu(UI_configuration, IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
}



//============================================================
//	loadsave
//============================================================

static void loadsave(int type)
{
	static char filename[MAX_PATH];
#ifdef UNICODE
	WCHAR filenamew[MAX_PATH];
#endif
	OPENFILENAME ofn;
	char *dir;
	int result = 0;
	char *src;
	char *dst;

	if (filename[0])
	{
		dir = osd_dirname(filename);
	}
	else
	{
		snprintf(filename, sizeof(filename) / sizeof(filename[0]), "%s State.sta", Machine->gamedrv->description);
		dir = NULL;

		src = filename;
		dst = filename;
		do
		{
			if ((*src == '\0') || isalnum(*src) || isspace(*src) || strchr("(),.", *src))
				*(dst++) = *src;
		}
		while(*(src++));
	}

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = win_video_window;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrFilter = TEXT("State Files (*.sta)\0*.sta\0All Files (*.*);*.*\0");
	ofn.lpstrInitialDir = A2T(dir);
#ifdef UNICODE
	ofn.lpstrFile = filenamew;
	ofn.nMaxFile = sizeof(filenamew) / sizeof(filenamew[0]);
#else
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);
#endif

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
#ifdef UNICODE
		snprintf(filename, sizeof(filename) / sizeof(filename[0]), "%S", filenamew);
#endif
		cpu_loadsave_schedule_file(type, filename);
	}
	if (dir)
		free(dir);
}



//============================================================
//	format_combo_changed
//============================================================

struct file_dialog_params
{
	const struct IODevice *dev;
	int *create_format;
	option_resolution **create_args;
};

static void format_combo_changed(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *changed_param)
{
	HWND wnd;
	int format_combo_val;
	const struct IODevice *dev;
	const struct OptionGuide *guide;
	const char *optspec;
	char buf1[256];
	char buf2[256];
	struct OptionRange ranges[128];
	struct file_dialog_params *params;
	int has_option, default_value, default_index, current_index, option_count;
	int i, j;

	params = (struct file_dialog_params *) changed_param;

	// locate the format control
	format_combo_val = notification ? (((OFNOTIFY *) notification)->lpOFN->nFilterIndex - 1) : 0;
	if (format_combo_val < 0)
		format_combo_val = 0;
	*(params->create_format) = format_combo_val;

	// compute our parameters
	dev = params->dev;
	guide = dev->createimage_optguide;	
	optspec = dev->createimage_options[format_combo_val].optspec;

	wnd = NULL;
	while((wnd = FindWindowEx(dlgwnd, wnd, NULL, NULL)) != NULL)
	{
		// get label text, removing trailing NULL
		GetWindowText(wnd, buf1, sizeof(buf1) / sizeof(buf1[0]));
		assert(buf1[strlen(buf1)-1] == ':');
		buf1[strlen(buf1)-1] = '\0';

		// find guide entry
		while(guide->option_type && strcmp(buf1, guide->display_name))
			guide++;

		wnd = GetNextWindow(wnd, GW_HWNDNEXT);
		if (wnd && guide)
		{
			// we now have the handle to the window, and the guide entry
			has_option = option_resolution_contains(optspec, guide->parameter);

			SendMessage(wnd, CB_GETLBTEXT, SendMessage(wnd, CB_GETCURSEL, 0, 0), (LPARAM) buf1);
			SendMessage(wnd, CB_RESETCONTENT, 0, 0);

			if (has_option)
			{
				option_resolution_listranges(optspec, guide->parameter,
					ranges, sizeof(ranges) / sizeof(ranges[0]));
				option_resolution_getdefault(optspec, guide->parameter, &default_value);

				option_count = 0;
				default_index = -1;
				current_index = -1;

				for (i = 0; ranges[i].min >= 0; i++)
				{
					for (j = ranges[i].min; j <= ranges[i].max; j++)
					{
						snprintf(buf2, sizeof(buf2) / sizeof(buf2[0]), "%d", j);
						SendMessage(wnd, CB_ADDSTRING, 0, (LPARAM) buf2);
						SendMessage(wnd, CB_SETITEMDATA, option_count, j);

						if (j == default_value)
							default_index = option_count;
						if (!strcmp(buf1, buf2))
							current_index = option_count;
						option_count++;
					}
				}
				
				// if there is only one option, it is effectively disabled
				if (option_count <= 1)
					has_option = FALSE;

				if (current_index >= 0)
					SendMessage(wnd, CB_SETCURSEL, current_index, 0);
				else if (default_index >= 0)
					SendMessage(wnd, CB_SETCURSEL, default_index, 0);
			}
			else
			{
				// this item is non applicable
				SendMessage(wnd, CB_ADDSTRING, 0, (LPARAM) TEXT("N/A"));
				SendMessage(wnd, CB_SETCURSEL, 0, 0);
			}
			EnableWindow(wnd, has_option ? TRUE : FALSE);
		}
	}
}



//============================================================
//	storeval_option_resolution
//============================================================

struct storeval_optres_params
{
	struct file_dialog_params *fdparams;
	const struct OptionGuide *guide_entry;
};

static void storeval_option_resolution(void *storeval_param, int val)
{
	option_resolution *resolution;
	struct storeval_optres_params *params;
	const struct IODevice *dev;
	char buf[16];
	
	params = (struct storeval_optres_params *) storeval_param;
	dev = params->fdparams->dev;

	// create the resolution, if necessary
	resolution = *(params->fdparams->create_args);
	if (!resolution)
	{
		resolution = option_resolution_create(dev->createimage_optguide, dev->createimage_options[*(params->fdparams->create_format)].optspec);
		if (!resolution)
			return;
		*(params->fdparams->create_args) = resolution;
	}

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%d", val);
	option_resolution_add_param(resolution, params->guide_entry->identifier, buf);
}



//============================================================
//	build_option_dialog
//============================================================

static dialog_box *build_option_dialog(const struct IODevice *dev, char *filter, size_t filter_len, int *create_format, option_resolution **create_args)
{
	dialog_box *dialog;
	const struct OptionGuide *guide_entry;
	int found, i, pos;
	char buf[256];
	struct file_dialog_params *params;
	struct storeval_optres_params *storeval_params;
	const struct dialog_layout filedialog_layout = { 44, 220 };

	// make the filter
	pos = 0;
	for (i = 0; dev->createimage_options[i].optspec; i++)
	{
		pos += add_filter_entry(filter + pos, filter_len - pos,
			dev->createimage_options[i].description,
			dev->createimage_options[i].extensions);
	}

	// create the dialog
	dialog = win_dialog_init(NULL, &filedialog_layout);
	if (!dialog)
		goto error;

	// allocate the params
	params = (struct file_dialog_params *) win_dialog_malloc(dialog, sizeof(*params));
	if (!params)
		goto error;
	params->dev = dev;
	params->create_format = create_format;
	params->create_args = create_args;

	// set the notify handler; so that we get notified when the format dialog changed
	if (win_dialog_add_notification(dialog, CDN_TYPECHANGE, format_combo_changed, params))
		goto error;

	// loop through the entries
	for (guide_entry = dev->createimage_optguide; guide_entry->option_type != OPTIONTYPE_END; guide_entry++)
	{
		// make sure that this entry is present on at least one option specification
		found = FALSE;
		for (i = 0; dev->createimage_options[i].optspec; i++)
		{
			if (option_resolution_contains(dev->createimage_options[i].optspec, guide_entry->parameter))
			{
				found = TRUE;
				break;
			}
		}

		if (found)
		{
			storeval_params = (struct storeval_optres_params *) win_dialog_malloc(dialog, sizeof(*storeval_params));
			if (!storeval_params)
				goto error;
			storeval_params->fdparams = params;
			storeval_params->guide_entry = guide_entry;

			// this option is present on at least one of the specs
			switch(guide_entry->option_type) {
			case OPTIONTYPE_INT:
				snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s:", guide_entry->display_name);
				if (win_dialog_add_combobox(dialog, buf, 0, storeval_option_resolution, storeval_params))
					goto error;
				break;

			default:
				break;
			}
		}
	}

	return dialog;

error:
	if (dialog)
		win_dialog_exit(dialog);
	return NULL;
}



//============================================================
//	add_filter_entry
//============================================================

static int add_filter_entry(char *dest, size_t dest_len, const char *description, const char *extensions)
{
	const char *s;
	int pos = 0;

	// add the description
	pos += snprintf(dest + pos, dest_len - pos, "%s (", description);
	
	// add the extensions to the description
	s = extensions;
	while(*s)
	{
		pos += snprintf(dest + pos, dest_len - pos, "%s*.%s", (s == extensions) ? "" : ";", s);
		s += strlen(s) + 1;
	}

	// add the trailing rparen
	pos += snprintf(dest + pos, dest_len - pos, ")", s);

	// get past the \0
	if (dest_len > 0)
	{
		pos++;
		dest_len--;
	}

	// now add the extension list itself
	s = extensions;
	while(*s)
	{
		pos += snprintf(dest + pos, dest_len - pos, "*.%s;", s);
		s += strlen(s) + 1;
	}


	// get past the \0
	if (dest_len > 0)
	{
		pos++;
		dest_len--;
		if (dest_len > 0)
			dest[pos] = '\0';
	}

	return pos;
}



//============================================================
//	build_generic_filter
//============================================================

static void build_generic_filter(const struct IODevice *dev, int is_save, char *filter, size_t filter_len)
{
	char *s;

	s = filter;

	// common image types
	s += add_filter_entry(filter, filter_len, "Common image types", dev->file_extensions);

	// all files
	s += sprintf(s, "All files (*.*)") + 1;
	s += sprintf(s, "*.*") + 1;

	// compressed
	if (!is_save)
	{
		s += sprintf(s, "Compressed Images (*.zip)") + 1;
		s += sprintf(s, "*.zip") + 1;
	}

	*(s++) = '\0';
}



//============================================================
//	change_device
//============================================================

static void change_device(mess_image *img, int is_save)
{
	dialog_box *dialog = NULL;
	char filter[2048];
	char filename[MAX_PATH];
	char *s;
	const struct IODevice *dev = image_device(img);
	const char *initial_dir;
	BOOL result;
	int create_format;
	option_resolution *create_args = NULL;

	assert(dev);

	// get the file
	if (image_exists(img))
	{
		const char *imgname;
		imgname = image_basename(img);
		strncpyz(filename, imgname, sizeof(filename) / sizeof(filename[0]));
	}
	else
	{
		filename[0] = '\0';
	}

	// use image directory, if it is there
	if (image_exists(img))
		initial_dir = image_filedir(img);
	else
		initial_dir = get_devicedirectory(dev->type);

	// add custom dialog elements, if appropriate
	if (is_save && dev->createimage_optguide && dev->createimage_options[0].optspec)
	{
		dialog = build_option_dialog(dev, filter, sizeof(filter) / sizeof(filter[0]), &create_format, &create_args);
		if (!dialog)
			goto done;
	}
	else
	{
		// build a normal filter
		build_generic_filter(dev, is_save, filter, sizeof(filter) / sizeof(filter[0]));
	}

	// display the dialog
	result = win_file_dialog(win_video_window, is_save ? FILE_DIALOG_SAVE : FILE_DIALOG_OPEN,
		dialog, filter, initial_dir, filename, sizeof(filename) / sizeof(filename[0]));
	if (result)
	{
		// get the filename
		s = osd_dirname(filename);
		if (s)
		{
			set_devicedirectory(dev->type, s);
			free(s);
		}

		// mount the image
		if (is_save)
			image_create(img, filename, create_format, create_args);
		else
			image_load(img, filename);
	}

done:
	if (dialog)
		win_dialog_exit(dialog);
	if (create_args)
		option_resolution_close(create_args);
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
			mb_size = strlen(text);
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
		draw_screen();
		while(is_paused && !win_trying_to_quit)
		{
			update_video_and_audio();
			WaitMessage();
			win_process_events();
		}
		mame_pause(0);
	}
}



//============================================================
//	find_submenu
//============================================================

static HMENU find_sub_menu(HMENU menu, const char *menutext, int create_sub_menu)
{
	MENUITEMINFO mii;
	int i;
	TCHAR buf[128];

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
	mii.dwTypeData = buf;

	while(*menutext)
	{
		i = -1;
		do
		{
			i++;
			mii.dwTypeData = buf;
			mii.cch = sizeof(buf) / sizeof(buf[0]);
			if (!GetMenuItemInfo(menu, i, TRUE, &mii))
				return NULL;
		}
		while((mii.fType == MFT_SEPARATOR) || !mii.dwTypeData || strcmp(menutext, T2A(mii.dwTypeData)));

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
	BOOL result;

#ifdef UNDER_CE
	result = EnableMenuItem(menu_bar, command, (state & MFS_GRAYED ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
	if (result)
		result = CheckMenuItem(menu_bar, command, (state & MFS_CHECKED ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND) != 0xffffffff;
#else
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	result = SetMenuItemInfo(menu_bar, command, FALSE, &mii);
#endif
}



//============================================================
//	append_menu
//============================================================

static void append_menu(HMENU menu, UINT flags, UINT_PTR id, int uistring)
{
	const char *str;
	str = (uistring >= 0) ? ui_getstring(uistring) : NULL;
	AppendMenu(menu, flags, id, A2T(str));
}



//============================================================
//	prepare_menus
//============================================================

static void prepare_menus(void)
{
	int i;
	const struct IODevice *dev;
	char buf[MAX_PATH];
	const char *s;
	HMENU device_menu;
	HMENU sub_menu;
	UINT_PTR new_item;
	UINT flags_for_exists;
	UINT flags_for_writing;
	mess_image *img;
	int has_config, has_dipswitch, has_keyboard;

	if (!win_menu_bar)
		return;

	has_config		= input_has_input_category(INPUT_CATEGORY_CONFIG);
	has_dipswitch	= input_has_input_category(INPUT_CATEGORY_DIPSWITCH);
	has_keyboard	= input_has_input_category(INPUT_CATEGORY_KEYBOARD);

	set_command_state(win_menu_bar, ID_EDIT_PASTE,				inputx_can_post()							? MFS_ENABLED : MFS_GRAYED);

	set_command_state(win_menu_bar, ID_OPTIONS_PAUSE,			is_paused									? MFS_CHECKED : MFS_ENABLED);
	set_command_state(win_menu_bar, ID_OPTIONS_THROTTLE,		throttle									? MFS_CHECKED : MFS_ENABLED);
	set_command_state(win_menu_bar, ID_OPTIONS_CONFIGURATION,	has_config									? MFS_ENABLED : MFS_GRAYED);
	set_command_state(win_menu_bar, ID_OPTIONS_DIPSWITCHES,		has_dipswitch								? MFS_ENABLED : MFS_GRAYED);
#if HAS_TOGGLEFULLSCREEN
	set_command_state(win_menu_bar, ID_OPTIONS_FULLSCREEN,		!win_window_mode							? MFS_CHECKED : MFS_ENABLED);
#endif
	set_command_state(win_menu_bar, ID_OPTIONS_TOGGLEFPS,		ui_show_fps_get()							? MFS_CHECKED : MFS_ENABLED);
#if HAS_PROFILER
	set_command_state(win_menu_bar, ID_OPTIONS_PROFILER,		ui_show_profiler_get()						? MFS_CHECKED : MFS_ENABLED);
#endif

	set_command_state(win_menu_bar, ID_KEYBOARD_EMULATED,		(has_keyboard) ?
																(!win_use_natural_keyboard					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(win_menu_bar, ID_KEYBOARD_NATURAL,		(has_keyboard && inputx_can_post()) ?
																(win_use_natural_keyboard					? MFS_CHECKED : MFS_ENABLED)
																												: MFS_GRAYED);
	set_command_state(win_menu_bar, ID_KEYBOARD_CUSTOMIZE,		has_keyboard								? MFS_ENABLED : MFS_GRAYED);

	set_command_state(win_menu_bar, ID_FRAMESKIP_AUTO,			autoframeskip								? MFS_CHECKED : MFS_ENABLED);
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
		set_command_state(win_menu_bar, ID_FRAMESKIP_0 + i, (!autoframeskip && (frameskip == i))			? MFS_CHECKED : MFS_ENABLED);

	// set up device menu
	device_menu = find_sub_menu(win_menu_bar, "&Devices\0", FALSE);
	while(RemoveMenu(device_menu, 0, MF_BYPOSITION))
		;

	for (dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		for (i = 0; i < dev->count; i++)
		{
			img = image_from_device_and_index(dev, i);

			new_item = ID_DEVICE_0 + (image_absolute_index(img) * DEVOPTION_MAX);
			flags_for_exists = MF_STRING;

			if (!image_exists(img))
				flags_for_exists |= MF_GRAYED;

			flags_for_writing = flags_for_exists;
			if (!image_is_writable(img))
				flags_for_writing |= MF_GRAYED;

			sub_menu = CreateMenu();
			append_menu(sub_menu, MF_STRING,		new_item + DEVOPTION_OPEN,		UI_mount);

			switch(dev->open_mode) {
			case OSD_FOPEN_WRITE:
			case OSD_FOPEN_RW_CREATE:
			case OSD_FOPEN_RW_CREATE_OR_READ:
				append_menu(sub_menu, MF_STRING,	new_item + DEVOPTION_CREATE,	UI_create);
				break;
			}

			append_menu(sub_menu, flags_for_exists,	new_item + DEVOPTION_CLOSE,		UI_unmount);

#if HAS_WAVE
			if ((dev->type == IO_CASSETTE) && !strcmp(dev->file_extensions, "wav"))
			{
				cassette_state state;
				state = image_exists(img) ? (cassette_get_state(img) & CASSETTE_MASK_UISTATE) : CASSETTE_STOPPED;
				append_menu(sub_menu, MF_SEPARATOR, 0, -1);
				append_menu(sub_menu, flags_for_exists	| ((state == CASSETTE_STOPPED)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_STOPPAUSE,	UI_pauseorstop);
				append_menu(sub_menu, flags_for_exists	| ((state == CASSETTE_PLAY)		? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_PLAY,			UI_play);
				append_menu(sub_menu, flags_for_writing	| ((state == CASSETTE_RECORD)	? MF_CHECKED : 0),	new_item + DEVOPTION_CASSETTE_RECORD,		UI_record);
#if USE_TAPEDLG
				append_menu(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_DIALOG,		UI_tapecontrol);
#else
				append_menu(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_REWIND,		UI_rewind);
				append_menu(sub_menu, flags_for_exists,														new_item + DEVOPTION_CASSETTE_FASTFORWARD,	UI_fastforward);
#endif
			}
#endif /* HAS_WAVE */
			s = image_exists(img) ? image_filename(img) : ui_getstring(UI_emptyslot);

			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s: %s", device_typename_id(img), s);
			AppendMenu(device_menu, MF_POPUP, (UINT_PTR) sub_menu, A2T(buf));
		}
	}
}



//============================================================
//	win_toggle_menubar
//============================================================

#if HAS_TOGGLEMENUBAR
void win_toggle_menubar(void)
{
	extern void win_pause_input(int pause_);

	win_pause_input(1);
	SetMenu(win_video_window, GetMenu(win_video_window) ? NULL : win_menu_bar);
	win_pause_input(0);
	
	if (win_window_mode)
	{
		RECT window;
		GetWindowRect(win_video_window, &window);
		win_constrain_to_aspect_ratio(&window, WMSZ_BOTTOM, 0);
		SetWindowPos(win_video_window, HWND_TOP, window.left, window.top,
			window.right - window.left, window.bottom - window.top, SWP_NOZORDER);
	}

	win_adjust_window();
	RedrawWindow(win_video_window, NULL, NULL, 0);
}
#endif // HAS_TOGGLEMENUBAR



//============================================================
//	device_command
//============================================================

static void device_command(mess_image *img, int devoption)
{
	switch(devoption) {
	case DEVOPTION_OPEN:
		change_device(img, FALSE);
		break;

	case DEVOPTION_CREATE:
		change_device(img, TRUE);
		break;

	case DEVOPTION_CLOSE:
		image_unload(img);
		break;

	default:
		switch(image_devtype(img)) {
#if HAS_WAVE
		case IO_CASSETTE:
			switch(devoption) {
			case DEVOPTION_CASSETTE_STOPPAUSE:
				cassette_change_state(img, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
				break;

			case DEVOPTION_CASSETTE_PLAY:
				cassette_change_state(img, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
				break;

			case DEVOPTION_CASSETTE_RECORD:
				cassette_change_state(img, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
				break;

#if USE_TAPEDLG
			case DEVOPTION_CASSETTE_DIALOG:
				tapedialog_show(id);
				break;
#else
			case DEVOPTION_CASSETTE_REWIND:
				device_seek(img, +11025, SEEK_CUR);
				break;

			case DEVOPTION_CASSETTE_FASTFORWARD:
				device_seek(img, +11025, SEEK_CUR);
				break;
#endif
			}
			break;
#endif /* HAS_WAVE */
		}
	}
}



//============================================================
//	help_display
//============================================================

static void help_display(const char *chapter)
{
	typedef HWND (WINAPI *htmlhelpproc)(HWND hwndCaller, LPCTSTR pszFile, UINT uCommand, DWORD_PTR dwData);
	static htmlhelpproc htmlhelp;
	static DWORD htmlhelp_cookie;
	LPCTSTR htmlhelp_funcname;

	if (htmlhelp == NULL)
	{
#ifdef UNICODE
		htmlhelp_funcname = TEXT("HtmlHelpW");
#else
		htmlhelp_funcname = TEXT("HtmlHelpA");
#endif
		htmlhelp = (htmlhelpproc) GetProcAddress(LoadLibrary(TEXT("hhctrl.ocx")), htmlhelp_funcname);
		if (!htmlhelp)
			return;
		htmlhelp(NULL, NULL, 28 /*HH_INITIALIZE*/, (DWORD_PTR) &htmlhelp_cookie);
	}

	// if full screen, turn it off
	if (!win_window_mode)
		win_toggle_full_screen();

	htmlhelp(win_video_window, A2T(chapter), 0 /*HH_DISPLAY_TOPIC*/, 0);
}



//============================================================
//	help_about_mess
//============================================================

static void help_about_mess(void)
{
	help_display("mess.chm::/windows/main.htm");
}



//============================================================
//	help_about_thissystem
//============================================================

static void help_about_thissystem(void)
{
	char buf[256];
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "mess.chm::/sysinfo/%s.htm", Machine->gamedrv->name);
	help_display(buf);
}



//============================================================
//	decode_deviceoption
//============================================================

static mess_image *decode_deviceoption(int command, int *devoption)
{
	int absolute_index;
	
	command -= ID_DEVICE_0;
	absolute_index = command / DEVOPTION_MAX;

	if (devoption)
		*devoption = command % DEVOPTION_MAX;

	return image_from_absolute_index(absolute_index);
}



//============================================================
//	invoke_command
//============================================================

static int invoke_command(UINT command)
{
	int handled = 1;
	int dev_command;
	mess_image *img;

	switch(command) {
	case ID_FILE_LOADSTATE:
		loadsave(LOADSAVE_LOAD);
		break;

	case ID_FILE_SAVESTATE:
		loadsave(LOADSAVE_SAVE);
		break;

	case ID_FILE_SAVESCREENSHOT:
		save_screen_snapshot(artwork_get_ui_bitmap());
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

	case ID_KEYBOARD_CUSTOMIZE:
		setkeyboard();
		break;

	case ID_OPTIONS_PAUSE:
		pause();
		break;

	case ID_OPTIONS_HARDRESET:
		machine_hard_reset();
		break;

	case ID_OPTIONS_SOFTRESET:
		machine_reset();
		break;

	case ID_OPTIONS_THROTTLE:
		throttle = !throttle;
		break;

#if HAS_PROFILER
	case ID_OPTIONS_PROFILER:
		ui_show_profiler_set(!ui_show_profiler_get());
		break;
#endif

#if HAS_DEBUGGER
	case ID_OPTIONS_DEBUGGER:
		debug_key_pressed = 1;
		break;
#endif

	case ID_OPTIONS_CONFIGURATION:
		setconfiguration();
		break;

	case ID_OPTIONS_DIPSWITCHES:
		setdipswitches();
		break;

#if HAS_TOGGLEFULLSCREEN
	case ID_OPTIONS_FULLSCREEN:
		win_toggle_full_screen();
		break;
#endif

	case ID_OPTIONS_TOGGLEFPS:
		ui_show_fps_set(!ui_show_fps_get());
		break;

#if HAS_TOGGLEMENUBAR
	case ID_OPTIONS_TOGGLEMENUBAR:
		win_toggle_menubar();
		break;
#endif

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
			img = decode_deviceoption(command, &dev_command);
			device_command(img, dev_command);
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
//	set_menu_text
//============================================================

void set_menu_text(HMENU menu_bar, int command, const char *text)
{
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = (LPTSTR) A2T(text);
	SetMenuItemInfo(menu_bar, command, FALSE, &mii);	
}



//============================================================
//	win_setup_menus
//============================================================

int win_setup_menus(HMODULE module, HMENU menu_bar)
{
	HMENU frameskip_menu;
	HMENU joystick_menu;
	char buf[256];
	int i, joystick_count = 0;

	static const int bitmap_ids[][2] =
	{
		{ IO_CARTSLOT,	IDI_ICON_CART },
		{ IO_HARDDISK,	IDI_ICON_HARD },
		{ IO_CASSETTE,	IDI_ICON_CASS },
		{ IO_FLOPPY,	IDI_ICON_FLOP },
		{ IO_PRINTER,	IDI_ICON_PRIN },
		{ IO_SERIAL,	IDI_ICON_SERL },
		{ IO_SNAPSHOT,	IDI_ICON_SNAP }
	};

	// verify that our magic numbers work
	assert((ID_DEVICE_0 + IO_COUNT * MAX_DEV_INSTANCES * DEVOPTION_MAX) < ID_JOYSTICK_0);
	is_paused = 0;

	// get the device icons
	memset(device_icons, 0, sizeof(device_icons));
	for (i = 0; i < sizeof(bitmap_ids) / sizeof(bitmap_ids[0]); i++)
		device_icons[bitmap_ids[i][0]] = LoadIcon(module, MAKEINTRESOURCE(bitmap_ids[i][1]));

	// remove the profiler menu item if it doesn't exist
#if HAS_PROFILER
	ui_show_profiler_set(0);
#else
	DeleteMenu(menu_bar, ID_OPTIONS_PROFILER, MF_BYCOMMAND);
#endif

	if (!HAS_DEBUGGER || !mame_debug)
		DeleteMenu(menu_bar, ID_OPTIONS_DEBUGGER, MF_BYCOMMAND);

#if !HAS_TOGGLEFULLSCREEN
	DeleteMenu(menu_bar, ID_OPTIONS_FULLSCREEN, MF_BYCOMMAND);
#endif

#if !HAS_TOGGLEMENUBAR
	DeleteMenu(menu_bar, ID_OPTIONS_TOGGLEMENUBAR, MF_BYCOMMAND);
#endif

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0", FALSE);
	if (!frameskip_menu)
		return 1;
	for(i = 0; i < FRAMESKIP_LEVELS; i++)
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%i", i);
		AppendMenu(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, A2T(buf));
	}

	// set up the reset options
	if (ram_option_count(Machine->gamedrv) == 0)
	{
		RemoveMenu(menu_bar, ID_OPTIONS_HARDRESET, MF_BYCOMMAND);
		set_menu_text(menu_bar, ID_OPTIONS_SOFTRESET, "&Reset");
	}

	// set up joystick menu
#ifndef UNDER_CE
	joystick_count = input_count_players();
#endif
	set_command_state(menu_bar, ID_OPTIONS_JOYSTICKS, joystick_count ? MFS_ENABLED : MFS_GRAYED);
	if (joystick_count > 0)
	{
		joystick_menu = find_sub_menu(menu_bar, "&Options\0&Joysticks\0", TRUE);
		if (!joystick_menu)
			return 1;
		for(i = 0; i < joystick_count; i++)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "Joystick %i", i + 1);
			AppendMenu(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, A2T(buf));
		}
	}

	// set the help menu to refer to this machine
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "About %s (%s)...", Machine->gamedrv->description, Machine->gamedrv->name);
	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, buf);

	win_menu_bar = menu_bar;
	return 0;
}



//============================================================
//	win_create_menu
//============================================================

#ifndef UNDER_CE
int win_create_menu(HMENU *menus)
{
	HMENU menu_bar = NULL;
	HMODULE module;
	
	if (options.disable_normal_ui)
	{
		module = GetModuleHandle(EMULATORDLL);
		menu_bar = LoadMenu(module, MAKEINTRESOURCE(IDR_RUNTIME_MENU));
		if (!menu_bar)
			goto error;

		if (win_setup_menus(module, menu_bar))
			goto error;
	}

	*menus = menu_bar;
	return 0;

error:
	if (menu_bar)
		DestroyMenu(menu_bar);
	return 1;
}
#endif /* UNDER_CE */



//============================================================
//	win_mess_window_proc
//============================================================

LRESULT CALLBACK win_mess_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	int i;
	MSG msg;

	static WPARAM keytrans[][2] =
	{
		{ VK_ESCAPE,	UCHAR_MAMEKEY(ESC) },
		{ VK_F1,		UCHAR_MAMEKEY(F1) },
		{ VK_F2,		UCHAR_MAMEKEY(F2) },
		{ VK_F3,		UCHAR_MAMEKEY(F3) },
		{ VK_F4,		UCHAR_MAMEKEY(F4) },
		{ VK_F5,		UCHAR_MAMEKEY(F5) },
		{ VK_F6,		UCHAR_MAMEKEY(F6) },
		{ VK_F7,		UCHAR_MAMEKEY(F7) },
		{ VK_F8,		UCHAR_MAMEKEY(F8) },
		{ VK_F9,		UCHAR_MAMEKEY(F9) },
		{ VK_F10,		UCHAR_MAMEKEY(F10) },
		{ VK_F11,		UCHAR_MAMEKEY(F11) },
		{ VK_F12,		UCHAR_MAMEKEY(F12) },
		{ VK_NUMLOCK,	UCHAR_MAMEKEY(F12) },
		{ VK_SCROLL,	UCHAR_MAMEKEY(F12) },
		{ VK_NUMPAD0,	UCHAR_MAMEKEY(0_PAD) },
		{ VK_NUMPAD1,	UCHAR_MAMEKEY(1_PAD) },
		{ VK_NUMPAD2,	UCHAR_MAMEKEY(2_PAD) },
		{ VK_NUMPAD3,	UCHAR_MAMEKEY(3_PAD) },
		{ VK_NUMPAD4,	UCHAR_MAMEKEY(4_PAD) },
		{ VK_NUMPAD5,	UCHAR_MAMEKEY(5_PAD) },
		{ VK_NUMPAD6,	UCHAR_MAMEKEY(6_PAD) },
		{ VK_NUMPAD7,	UCHAR_MAMEKEY(7_PAD) },
		{ VK_NUMPAD8,	UCHAR_MAMEKEY(8_PAD) },
		{ VK_NUMPAD9,	UCHAR_MAMEKEY(9_PAD) },
		{ VK_DECIMAL,	UCHAR_MAMEKEY(DEL_PAD) },
		{ VK_ADD,		UCHAR_MAMEKEY(PLUS_PAD) },
		{ VK_SUBTRACT,	UCHAR_MAMEKEY(MINUS_PAD) },
		{ VK_INSERT,	UCHAR_MAMEKEY(INSERT) },
		{ VK_DELETE,	UCHAR_MAMEKEY(DEL) },
		{ VK_HOME,		UCHAR_MAMEKEY(HOME) },
		{ VK_END,		UCHAR_MAMEKEY(END) },
		{ VK_PRIOR,		UCHAR_MAMEKEY(PGUP) },
		{ VK_NEXT,		UCHAR_MAMEKEY(PGDN) }
	};

	if (win_use_natural_keyboard && (message == WM_KEYDOWN))
	{
		for (i = 0; i < sizeof(keytrans) / sizeof(keytrans[0]); i++)
		{
			if (wparam == keytrans[i][0])
			{
				inputx_postc(keytrans[i][1]);
				message = WM_NULL;

				/* check to see if there is a corresponding WM_CHAR in our
				 * future.  If so, remove it
				 */
				PeekMessage(&msg, wnd, 0, 0, PM_NOREMOVE);
				if ((msg.message == WM_CHAR) && (msg.lParam == lparam))
					PeekMessage(&msg, wnd, 0, 0, PM_REMOVE);
				break;
			}
		}
	}

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
		return win_video_window_proc(wnd, message, wparam, lparam);
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


