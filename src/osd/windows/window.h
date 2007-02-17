//============================================================
//
//  window.h - Win32 window handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN_WINDOW__
#define __WIN_WINDOW__

#include "blit.h"
#include "video.h"
#include "render.h"


//============================================================
//  PARAMETERS
//============================================================

#ifndef MESS
#define HAS_WINDOW_MENU			FALSE
#else
#define HAS_WINDOW_MENU			TRUE
#endif



//============================================================
//  CONSTANTS
//============================================================

#define RESIZE_STATE_NORMAL		0
#define RESIZE_STATE_RESIZING	1
#define RESIZE_STATE_PENDING	2



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _win_window_info win_window_info;
struct _win_window_info
{
	win_window_info *	next;
	volatile int		init_state;

	// window handle and info
	HWND				hwnd;
	char				title[256];
	RECT				non_fullscreen_bounds;
	int					startmaximized;
	int					isminimized;
	int					ismaximized;
	int					resize_state;

	// monitor info
	win_monitor_info *	monitor;
	int					fullscreen;
	int					fullscreen_safe;
	int					maxwidth, maxheight;
	int					refresh;
	float				aspect;

	// rendering info
	osd_lock *			render_lock;
	render_target *		target;
	int					targetview;
	int					targetorient;
	int					targetlayerconfig;
	const render_primitive_list *primlist;

	// drawing data
	void *				drawdata;
};


typedef struct _win_draw_callbacks win_draw_callbacks;
struct _win_draw_callbacks
{
	void (*exit)(void);

	int (*window_init)(win_window_info *window);
	const render_primitive_list *(*window_get_primitives)(win_window_info *window);
	int (*window_draw)(win_window_info *window, HDC dc, int update);
	void (*window_destroy)(win_window_info *window);
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

// windows
extern win_window_info *win_window_list;

// visible bounds
extern RECT			win_visible_rect;
extern int			win_visible_width;
extern int			win_visible_height;

// raw mouse support
extern int			win_use_raw_mouse;



//============================================================
//  PROTOTYPES
//============================================================

// core initialization
int winwindow_init(running_machine *machine);

// creation/deletion of windows
int winwindow_video_window_create(int index, win_monitor_info *monitor, const win_window_config *config);

void winwindow_update_cursor_state(void);
void winwindow_video_window_update(win_window_info *window);
win_monitor_info *winwindow_video_window_monitor(win_window_info *window, const RECT *proposed);

LRESULT CALLBACK winwindow_video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
void winwindow_toggle_full_screen(void);

void winwindow_process_events_periodic(void);
void winwindow_process_events(int ingame);

void winwindow_ui_pause_from_window_thread(int pause);
void winwindow_ui_pause_from_main_thread(int pause);
int winwindow_ui_is_paused(void);

void winwindow_ui_exec_on_main_thread(void (*func)(void *), void *param);

#if HAS_WINDOW_MENU
int win_create_menu(HMENU *menus);
#endif



//============================================================
//  win_has_menu
//============================================================

INLINE BOOL win_has_menu(win_window_info *window)
{
	return GetMenu(window->hwnd) ? TRUE : FALSE;
}


//============================================================
//  rect_width / rect_height
//============================================================

INLINE int rect_width(const RECT *rect)
{
	return rect->right - rect->left;
}


INLINE int rect_height(const RECT *rect)
{
	return rect->bottom - rect->top;
}

#endif
