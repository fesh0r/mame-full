#include "driver.h"

typedef enum { 
	MENU_TEXT_LEFT, MENU_TEXT_RIGHT, MENU_TEXT_CENTER 
} MENU_TEXT_POS;

typedef struct {
	char *name;
	MENU_TEXT_POS pos;
} MENU_TEXT;

typedef struct {
	char *name;
	MENU_TEXT_POS pos;
	void (*pressed)(int id);
} MENU_BUTTON;

typedef struct {
	char *name;
	int value;
	void (*changed)(int id, int value);
	char *options[10]; // only not 0 options are selectable
	int editable;
} MENU_SELECTION;

int menu_selection_is_first(MENU_SELECTION *entry);
int menu_selection_is_last(MENU_SELECTION *entry);
void menu_selection_plus(MENU_SELECTION *entry, int id);
void menu_selection_minus(MENU_SELECTION *entry, int id);
void menu_selection_set_value(MENU_SELECTION *entry, int id, int value);

