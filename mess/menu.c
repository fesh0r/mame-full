#include <assert.h>
#include "menu.h"
#include "menuentr.h"

#define MAX_ENTRIES 100

typedef struct {
	int id;
	int visible;
	MENU_ENTRY_TYPE type;
	union {
		MENU_TEXT text;
		MENU_BUTTON button;
		MENU_SELECTION selct;
	} t;
} MENU_ENTRY;

typedef struct _MENU {
	int used;
	MENU_ENTRY entries[MAX_ENTRIES];
} MENU;

static MENU setup= { 
	0
};
MENU *m_setup=&setup;

static int menu_entry_is_editable(MENU_ENTRY *entry)
{
	return (entry->type==MENU_ENTRY_SELECTION)||(entry->type==MENU_ENTRY_BUTTON);
}

void menu_add_entry(MENU *menu, int id, int visible, 
					MENU_ENTRY_TYPE type, void *entry)
{
	assert(menu->used<ARRAY_LENGTH(menu->entries));

	menu->entries[menu->used].visible=visible;
	menu->entries[menu->used].id=id;
	menu->entries[menu->used].type=type;
	switch(type) {
	case MENU_ENTRY_TEXT:
		menu->entries[menu->used].t.text=*(MENU_TEXT*)entry;
		break;
	case MENU_ENTRY_BUTTON:
		menu->entries[menu->used].t.button=*(MENU_BUTTON*)entry;
		break;
	case MENU_ENTRY_SELECTION:
		menu->entries[menu->used].t.selct=*(MENU_SELECTION*)entry;
		break;
	default:
		exit(1);
	}
	menu->used++;
}

void menu_write_config(struct _MENU *menu, void *filehandler)
{
	int i;
	UINT8 v;
	for (i=0; i<menu->used; i++) {
		if (menu->entries[i].type==MENU_ENTRY_SELECTION) {
			v=menu->entries[i].t.selct.value;
			osd_fwrite(filehandler, &v, 1);
		}
	}
}

void menu_read_config(struct _MENU *menu, void *filehandler)
{
	int i;
	UINT8 v;
	for (i=0; i<menu->used; i++) {
		if (menu->entries[i].type==MENU_ENTRY_SELECTION) {
			if (1!=osd_fread(filehandler, &v, 1)) break;
			menu->entries[i].t.selct.value=v;
		}
	}
	/* init of all selections */
	for (i=0; i<menu->used; i++) {
		if (menu->entries[i].type==MENU_ENTRY_SELECTION) {
			if (menu->entries[i].t.selct.changed)
				menu->entries[i].t.selct.changed(menu->entries[i].id, v);
		}
	}
}

static int menu_get_index(MENU *menu, int id)
{
	int i;
	for (i=0; i<menu->used; i++) {
		if (menu->entries[i].id==id) return i;
	}
	return -1;
}

int menu_get_value(MENU *menu, int id)
{
	int v=menu_get_index(menu, id);
	if (v!=-1) return menu->entries[v].t.selct.value;
	return -1;
}

void menu_set_value(MENU *menu, int id, int value)
{
	int v=menu_get_index(menu, id);
	if (v!=-1) 
		menu_selection_set_value(&menu->entries[v].t.selct, 
								 menu->entries[v].id, value);
}

void menu_set_visibility(MENU *menu, int id, int visible)
{
	int v=menu_get_index(menu, id);
	if (v!=-1) 
		menu->entries[v].visible=visible;
}

static int menu_control(MENU *menu, struct mame_bitmap *bitmap, int selected)
{
	MENU_ENTRY *entries[MAX_ENTRIES+1];
    const char *menu_item[MAX_ENTRIES+1];
    const char *menu_subitem[MAX_ENTRIES+1];
	char flag[MAX_ENTRIES+1];

    int sel;
    int total;
    int arrowize;
	int j;

    total = 0;
    sel = selected - 1;


	for (total=0, j=0; (j<menu->used); j++) {
		if (menu->entries[j].visible) {
			entries[total]=menu->entries+j;
			switch (entries[total]->type) {
			case MENU_ENTRY_TEXT:
				switch (entries[total]->t.text.pos) {
				case MENU_TEXT_LEFT:
					menu_item[total]=entries[total]->t.text.name;
					menu_subitem[total]="";
				case MENU_TEXT_RIGHT:
					menu_subitem[total]=entries[total]->t.text.name;
					menu_item[total]="";
				case MENU_TEXT_CENTER:
					menu_item[total]=entries[total]->t.text.name;
					menu_subitem[total]=NULL;
				}
				break;
			case MENU_ENTRY_BUTTON:
				switch (entries[total]->t.text.pos) {
				case MENU_TEXT_LEFT:
					menu_item[total]=entries[total]->t.button.name;
					menu_subitem[total]="";
				case MENU_TEXT_RIGHT:
					menu_subitem[total]=entries[total]->t.button.name;
					menu_item[total]="";
				case MENU_TEXT_CENTER:
					menu_item[total]=entries[total]->t.button.name;
					menu_subitem[total]=NULL;
				}
				break;
			case MENU_ENTRY_SELECTION:
				menu_item[total]=entries[total]->t.selct.name;
				menu_subitem[total]=entries[total]->t.selct.options[entries[total]->t.selct.value];
				break;
			}
			flag[total]=0;
			total++;
		}
	}
    menu_item[total] = 0;   /* terminate array */
    menu_subitem[total] = 0;
    flag[total] = 0;

	/* place sel at safe place! */
	for (;(sel<total)&&!menu_entry_is_editable(entries[sel]); sel++) ;
	if (sel>=total) sel=0;
	/* place sel at safe place! */
	for (;(sel<total)&&!menu_entry_is_editable(entries[sel]); sel++) ;
	if (sel>=total) return 0;

    arrowize = 0;
	switch (entries[sel]->type) {
	case MENU_ENTRY_SELECTION:
		if (!menu_selection_is_first(&entries[sel]->t.selct)) arrowize|=1;
		if (!menu_selection_is_last(&entries[sel]->t.selct)) arrowize|=2;
		break;
	default:
		break;
	}

    if (sel > 255)  /* are we waiting for a new key? */
    {
        /* display the menu */
		ui_displaymenu(bitmap, menu_item,menu_subitem,flag,sel & 0xff,3);
        return sel + 1;
    }

	ui_displaymenu(bitmap, menu_item,menu_subitem,flag,sel,arrowize);

    if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
    {
		sel++;
		for (;(sel<total)&&!menu_entry_is_editable(entries[sel]); sel++) ;
		if (sel==total) sel=0;
		/* place sel at safe place! */
		for (;(sel<total)&&!menu_entry_is_editable(entries[sel]); sel++) ;
    }

    if (input_ui_pressed_repeat(IPT_UI_UP,8))
    {
		sel--;
		while ((sel>=0)&&!menu_entry_is_editable(entries[sel])) sel--;
		if (sel<0) sel=total-1;
		while ((sel>=0)&&!menu_entry_is_editable(entries[sel])) sel--;
		if (sel<0) sel=0; // when nothing found
    }


	if (input_ui_pressed(IPT_UI_RIGHT)&&(entries[sel]->type==MENU_ENTRY_SELECTION))
    {
		menu_selection_plus(&entries[sel]->t.selct, entries[sel]->id);
		
		/* tell updatescreen() to clean after us (in case the window changes size) */
		schedule_full_refresh();
    }

	if (input_ui_pressed(IPT_UI_LEFT)&&(entries[sel]->type==MENU_ENTRY_SELECTION))
    {
		menu_selection_minus(&entries[sel]->t.selct, entries[sel]->id);

		/* tell updatescreen() to clean after us (in case the window changes size) */
		schedule_full_refresh();
    }

    if (input_ui_pressed(IPT_UI_SELECT)&&(entries[sel]->type==MENU_ENTRY_BUTTON))
    {
		if (entries[sel]->t.button.pressed) {
			entries[sel]->t.button.pressed(entries[sel]->id);
            schedule_full_refresh();
        }
    }

    if (input_ui_pressed(IPT_UI_CANCEL))
        sel = -1;

    if (input_ui_pressed(IPT_UI_CONFIGURE))
        sel = -2;

    if (sel == -1 || sel == -2)
    {
        /* tell updatescreen() to clean after us */
        schedule_full_refresh();
    }

    return sel + 1;
}

int setupcontrol(struct mame_bitmap *bitmap, int selected)
{
	return menu_control(&setup, bitmap, selected);
}
