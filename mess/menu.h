#ifndef __MENU_H__
#define __MENU_H__

#include "driver.h"

/*
  interface for a pseudo dynamic MAME/MESS menu
  should be created at game/system init time

  advantages:
  callback function when something changes
  documentation lines
  visibility of a line could be changed
  you must not know how mame displays your menu

  use m_setup to access "Machine Setup" menu
*/

struct _MENU;
extern struct _MENU *m_setup;

typedef enum {
	MENU_ENTRY_TEXT, // display line of text
	MENU_ENTRY_BUTTON, // execute callback when enter pressed
	MENU_ENTRY_SELECTION // use left, right to select item
#if 0
	MENU_ENTRY_INPUT, // define input key
	MENU_ENTRY_DIRECTORY, // enter directory
	MENU_ENTRY_FILE // enter filename
#endif
} MENU_ENTRY_TYPE;

// look into menuentr.h for the definition of the entry types
void menu_add_entry(struct _MENU *menu, int id, int visible, MENU_ENTRY_TYPE type, void *entry);

void menu_set_visibility(struct _MENU *menu, int id, int visible);

int menu_get_value(struct _MENU *menu, int id);
void menu_set_value(struct _MENU *menu, int id, int value);

// internal
extern int setupcontrol(struct mame_bitmap *bitmap, int selected);

void menu_write_config(struct _MENU *menu, void *filehandler);
void menu_read_config(struct _MENU *menu, void *filehandler);

#endif
