#include "driver.h"

/* used to tell updatescreen() to clear the bitmap */
extern int need_to_clear_bitmap;    

int tapecontrol(int selected)
{
	static int id = 0;
	char timepos[32];
	int t0, t1;
    const char *menu_item[40];
    const char *menu_subitem[40];
	char flag[40];

    int sel;
    int total;
    int arrowize;

    total = 0;
    sel = selected - 1;

    menu_item[total] = device_typename_id(IO_CASSETTE,id);
	menu_subitem[total] = device_filename(IO_CASSETTE,id) ? device_filename(IO_CASSETTE,id) : "---";
	flag[total] = 0;
	total++;

	t0 = device_tell(IO_CASSETTE,id);
	t1 = device_length(IO_CASSETTE,id);
	if( t1 )
		sprintf(timepos, "%3d%%", t0*100/t1);
	else
		sprintf(timepos, "%3d%%", 0);
	menu_item[total] = device_status(IO_CASSETTE,id,-1) ? "playing" : "stopped";
	menu_subitem[total] = timepos;
    flag[total] = 0;
	total++;

    menu_item[total] = "Pause/Stop";
	menu_subitem[total] = 0;
    flag[total] = 0;
	total++;

	menu_item[total] = "Play";
	menu_subitem[total] = 0;
    flag[total] = 0;
	total++;

	menu_item[total] = "Rewind";
	menu_subitem[total] = 0;
	flag[total] = 0;
    total++;

	menu_item[total] = "Fast forward";
	menu_subitem[total] = 0;
	flag[total] = 0;
    total++;

    menu_item[total] = "Return to Main Menu";
    menu_subitem[total] = 0;
    flag[total] = 0;
    total++;

    menu_item[total] = 0;   /* terminate array */
    menu_subitem[total] = 0;
    flag[total] = 0;

    arrowize = 0;
    if (sel < total - 1)
        arrowize = 2;

    if (sel > 255)  /* are we waiting for a new key? */
    {
        /* display the menu */
		ui_displaymenu(menu_item,menu_subitem,flag,sel & 0xff,3);
        return sel + 1;
    }

	ui_displaymenu(menu_item,menu_subitem,flag,sel,arrowize);

    if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
    {
        if (sel < total - 1) sel++;
        else sel = 0;
    }

    if (input_ui_pressed_repeat(IPT_UI_UP,8))
    {
        if (sel > 0) sel--;
        else sel = total - 1;
    }


	if (input_ui_pressed(IPT_UI_LEFT))
    {
		switch (sel)
		{
		case 0:
			id = --id % device_count(IO_CASSETTE);
			break;
		}
		/* tell updatescreen() to clean after us (in case the window changes size) */
		need_to_clear_bitmap = 1;
    }

	if (input_ui_pressed(IPT_UI_RIGHT))
    {
		switch (sel)
		{
		case 0:
			id = ++id % device_count(IO_CASSETTE);
			break;
		}
		/* tell updatescreen() to clean after us (in case the window changes size) */
		need_to_clear_bitmap = 1;
    }

    if (input_ui_pressed(IPT_UI_SELECT))
    {
        if (sel == total - 1)
            sel = -1;
        else
        {
			switch (sel)
			{
			case 0:
                id = ++id % device_count(IO_CASSETTE);
				break;
			case 2:
				if (device_status(IO_CASSETTE,id,-1))
					device_status(IO_CASSETTE,id,0);
				else
				{
					device_seek(IO_CASSETTE,id,0,SEEK_SET);
					device_status(IO_CASSETTE,id,0);
				}
				break;
			case 3:
				device_status(IO_CASSETTE,id,1);
                break;
			case 4:
				device_seek(IO_CASSETTE,id,-11025,SEEK_CUR);
				break;
			case 5:
				device_seek(IO_CASSETTE,id,+11025,SEEK_CUR);
				break;
            }
            /* tell updatescreen() to clean after us (in case the window changes size) */
            need_to_clear_bitmap = 1;
        }
    }

    if (input_ui_pressed(IPT_UI_CANCEL))
        sel = -1;

    if (input_ui_pressed(IPT_UI_CONFIGURE))
        sel = -2;

    if (sel == -1 || sel == -2)
    {
        /* tell updatescreen() to clean after us */
        need_to_clear_bitmap = 1;
    }

    return sel + 1;
}


