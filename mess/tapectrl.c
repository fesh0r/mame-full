#include "driver.h"
#include "image.h"
#include "ui_text.h"
#include "devices/cassette.h"

void tapecontrol_gettime(char *timepos, size_t timepos_size, mess_image *img, int *curpos, int *endpos)
{
	double t0, t1;

	t0 = cassette_get_position(img);
	t1 = cassette_get_length(img);

	if (t1)
		snprintf(timepos, timepos_size, "%04d/%04d", (int) t0, (int) t1);
	else
		snprintf(timepos, timepos_size, "%04d/%04d", 0, (int) t1);

	if (curpos)
		*curpos = t0;
	if (endpos)
		*endpos = t1;
}

int tapecontrol(struct mame_bitmap *bitmap, int selected)
{
	static int id = 0;
	char timepos[32];
	const char *menu_item[40];
	const char *menu_subitem[40];
	char flag[40];
	mess_image *img;

	int sel;
	int total;
	int arrowize;
	cassette_state state;

	if (!device_find(Machine->devices, IO_CASSETTE))
		return 0;

	total = 0;
	sel = selected - 1;

	img = image_from_devtype_and_index(IO_CASSETTE, id);
	menu_item[total] = image_typename_id(img);
	menu_subitem[total] = image_filename(img) ? image_filename(img) : "---";
	flag[total] = 0;
	total++;

	tapecontrol_gettime(timepos, sizeof(timepos) / sizeof(timepos[0]), img, NULL, NULL);

	state = cassette_get_state(img);
	menu_item[total] =
		ui_getstring(
			(state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED
				?	UI_stopped
				:	((state & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY
					? ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_playing : UI_playing_inhibited)
					: ((state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_ENABLED ? UI_recording : UI_recording_inhibited)
					));

	menu_subitem[total] = timepos;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_pauseorstop);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_play);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_record);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_rewind);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_fastforward);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_returntomain);
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
		ui_displaymenu(bitmap, menu_item,menu_subitem,flag,sel & 0xff,3);
		return sel + 1;
	}

	ui_displaymenu(bitmap, menu_item,menu_subitem,flag,sel,arrowize);

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
			id--;
			if (id < 0) id = device_count(IO_CASSETTE)-1;
			break;
		}
		/* tell updatescreen() to clean after us (in case the window changes size) */
		schedule_full_refresh();
	}

	if (input_ui_pressed(IPT_UI_RIGHT))
	{
		switch (sel)
		{
		case 0:
			id++;
			if (id > device_count(IO_CASSETTE)-1) id = 0;
			break;
		}
		/* tell updatescreen() to clean after us (in case the window changes size) */
		schedule_full_refresh();
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1)
			sel = -1;
		else
		{
			img = image_from_devtype_and_index(IO_CASSETTE, id);
			switch (sel)
			{
			case 0:
				id = (id + 1) % device_count(IO_CASSETTE);
				break;
			case 2:
				/* Pause/stop */
				cassette_change_state(img, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
				break;
			case 3:
				/* Play */
				cassette_change_state(img, CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
				break;
			case 4:
				/* Record */
				cassette_change_state(img, CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
				break;
			case 5:
				/* Rewind */
				cassette_seek(img, -1, SEEK_CUR);
				break;
			case 6:
				/* Fast forward */
				cassette_seek(img, +1, SEEK_CUR);
				break;
			}
			/* tell updatescreen() to clean after us (in case the window changes size) */
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
