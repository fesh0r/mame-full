#include "mame.h"
#include "mamece.h"
#include "resource.h"
#include "cemenu.h"
#include "driver.h"

static HWND wince_menubar;
static HMENU wince_menus;

struct commandmap_entry
{
	int menuitem_id;
	int input_ui;
	int state;	// 0=unchecked, 1=checked
	const int *stateptr;
};

extern int throttle;
extern int showfps;
extern int wince_paused;

static struct commandmap_entry wince_commandmap[] =
{
	{ ID_FILE_PAUSE,				IPT_UI_PAUSE,			0,	&wince_paused},
	{ IDOK,							IPT_UI_CANCEL,			0,	NULL},
#ifdef MAME_DEBUG	
	{ ID_OPTIONS_SHOWPROFILER,		IPT_UI_SHOW_PROFILER,	0,	NULL},
#endif
	{ ID_OPTIONS_SHOWFRAMERATE,		IPT_UI_SHOW_FPS,		0,	&showfps},
	{ ID_OPTIONS_ENABLETHROTTLE,	IPT_UI_THROTTLE,		0,	&throttle}
};

void wince_create_menus(HWND main_window)
{
	SHMENUBARINFO menubar;
	int use_sip;
	int i;

	if (wince_menubar)
		return;

#ifdef MESS
	use_sip = Machine->gamedrv->flags & GAME_COMPUTER;
#else
	use_sip = 0;
#endif

	memset(&menubar, 0, sizeof(menubar));
	menubar.cbSize = sizeof(menubar);
	menubar.hwndParent = main_window;
	menubar.dwFlags = use_sip ? SHCMBF_HIDESIPBUTTON : SHCMBF_HIDESIPBUTTON;
	menubar.hInstRes = GetModuleHandle(NULL);
	menubar.nBmpId = 0;
	menubar.cBmpImages = 0;
	menubar.nToolBarId = IDM_RUNMENU;
	
	SHCreateMenuBar(&menubar);
	wince_menubar = menubar.hwndMB;
	wince_menus = (HMENU) SendMessage(wince_menubar, SHCMBM_GETMENU, 0, 0);

	for (i = 0; i < sizeof(wince_commandmap) / sizeof(wince_commandmap[0]); i++)
		wince_commandmap[i].state = 0;
}

void wince_destroy_menus(void)
{
	wince_menubar = NULL;
	wince_menus = NULL;
}

void wince_update_menus(void)
{
	int i;
	if (wince_menubar)
	{
		for (i = 0; i < sizeof(wince_commandmap) / sizeof(wince_commandmap[0]); i++)
		{
			struct commandmap_entry *e = &wince_commandmap[i];
			if (e->stateptr)
			{
				if (e->state != *(e->stateptr))
				{
					e->state = *(e->stateptr);
					CheckMenuItem(wince_menus, e->menuitem_id,
						MF_BYCOMMAND | (e->state ? MF_CHECKED : MF_UNCHECKED));
				}
			}
		}
	}
}

void wince_menu_command(int command)
{
	int i;
	for (i = 0; i < sizeof(wince_commandmap) / sizeof(wince_commandmap[0]); i++)
	{
		if (wince_commandmap[i].menuitem_id == command)
		{
			input_ui_post(wince_commandmap[i].input_ui);
			break;
		}
	}
}

