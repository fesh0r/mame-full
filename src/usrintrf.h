/*********************************************************************

  usrintrf.h

  Functions used to handle MAME's crude user interface.

MESS Changes:
        052898 - HJB
        . defined interface keys for easier remapping
        . moved F3 (reset) -> F9, F4 (display charset) -> F5
*********************************************************************/

#ifndef USRINTRF_H
#define USRINTRF_H

struct DisplayText
{
	const char *text;	/* 0 marks the end of the array */
	int color;	/* see #defines below */
	int x;
	int y;
};

#define DT_COLOR_WHITE 0
#define DT_COLOR_YELLOW 1
#define DT_COLOR_RED 2

#define UI_KEY_ESCAPE   OSD_KEY_ESC
#define UI_KEY_PAUSE	OSD_KEY_SCRLOCK

#define UI_KEY_CHARSET  OSD_KEY_F5      /* changed from F4 to F5 */

#define UI_KEY_SKIPFRAMES  OSD_KEY_F8
#define UI_KEY_RESET       OSD_KEY_F9      /* changed from F3 to F9 */
#define UI_KEY_THROTTLE    OSD_KEY_F10
#define UI_KEY_VIEW_FRAMES OSD_KEY_F11
#define UI_KEY_SNAPSHOT    OSD_KEY_F12

#define UI_KEY_MENU        OSD_KEY_TAB
#define UI_KEY_MENU_UP     OSD_KEY_UP
#define UI_KEY_MENU_DOWN   OSD_KEY_DOWN
#define UI_KEY_MENU_LEFT   OSD_KEY_LEFT
#define UI_KEY_MENU_RIGHT  OSD_KEY_RIGHT
#define UI_KEY_MENU_PGUP   OSD_KEY_PGUP
#define UI_KEY_MENU_PGDN   OSD_KEY_PGDN
#define UI_KEY_MENU_SELECT OSD_KEY_ENTER

#define UI_KEY_INC_VOLUME  OSD_KEY_PLUS_PAD
#define UI_KEY_DEC_VOLUME  OSD_KEY_MINUS_PAD

struct GfxElement *builduifont(void);
void displaytext(const struct DisplayText *dt,int erase);
int showcharset(void);
int showcopyright(void);
int showcredits(void);
int showgameinfo(void);
int setup_menu(void);
void set_ui_visarea (int xmin, int ymin, int xmax, int ymax);

#endif
