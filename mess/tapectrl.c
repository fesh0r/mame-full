#include "driver.h"
#include "image.h"
#include "ui_text.h"
#include "snprintf.h"

#if HAS_WAVE

void tapecontrol_gettime(char *timepos, size_t timepos_size, mess_image *img, int *curpos, int *endpos)
{
	int t0, t1;

	t0 = device_tell(img);
	t1 = device_seek(img, 0, SEEK_END);
	device_seek(img, t0, SEEK_SET);

	if (t1)
		snprintf(timepos, timepos_size, "%04d/%04d", t0/11025, t1/11025);
	else
		snprintf(timepos, timepos_size, "%04d/%04d", 0, t1/11025);

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
	int status;

	if (!device_find(Machine->gamedrv, IO_CASSETTE))
		return 0;

	total = 0;
	sel = selected - 1;

	img = image_from_devtype_and_index(IO_CASSETTE, id);
	menu_item[total] = device_typename_id(img);
	menu_subitem[total] = image_filename(img) ? image_filename(img) : "---";
	flag[total] = 0;
	total++;

	tapecontrol_gettime(timepos, sizeof(timepos) / sizeof(timepos[0]), img, NULL, NULL);

	status = device_status(img, -1);
	menu_item[total] = ui_getstring((status & WAVE_STATUS_MOTOR_ENABLE)
							? (status & WAVE_STATUS_MOTOR_INHIBIT)
								? ((status & WAVE_STATUS_WRITE_ONLY) ? UI_recording_inhibited : UI_playing_inhibited)
								: ((status & WAVE_STATUS_WRITE_ONLY) ? UI_recording : UI_playing)
							: UI_stopped);
	menu_subitem[total] = timepos;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring(UI_pauseorstop);
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;

	menu_item[total] = ui_getstring((status & WAVE_STATUS_WRITE_ONLY) ? UI_record : UI_play);
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
			status = device_status(img, -1);
			switch (sel)
			{
			case 0:
				id = (id + 1) % device_count(IO_CASSETTE);
				break;
			case 2:
				/* Pause/stop */
				if ((status & 1) == 0)
					device_seek(img,0,SEEK_SET);
				device_status(img,status & ~WAVE_STATUS_MOTOR_ENABLE);
				break;
			case 3:
				/* Play/Record */
				device_status(img,status | WAVE_STATUS_MOTOR_ENABLE);
				break;
			case 4:
				/* Rewind */
				device_seek(img, -11025, SEEK_CUR);
				break;
			case 5:
				/* Fast forward */
				device_seek(img, +11025, SEEK_CUR);
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

#endif /* HAS_WAVE */
