#include "mesintrf.h"
#include "mame.h"
#include "ui_text.h"
#include "input.h"

extern void display_fps(struct mame_bitmap *bitmap);

extern int show_profiler;
int mess_pause_for_ui = 0;

int handle_mess_user_interface(struct mame_bitmap *bitmap)
{
	static int ui_active = 0, ui_toggle_key = 0;
	static int ui_display_count = 30;

	char buf[2048];
	int trying_to_quit;
	int type, id;
	const struct IODevice *dev;

	trying_to_quit = osd_trying_to_quit();
	if (!trying_to_quit)
	{
		if (options.disable_normal_ui)
		{
			if (show_profiler)
				profiler_show(bitmap);
			display_fps(bitmap);
		}
		else
		{
			if (Machine->gamedrv->flags & GAME_COMPUTER)
			{
				if( input_ui_pressed(IPT_UI_TOGGLE_UI) )
				{
					if( !ui_toggle_key )
					{
						ui_toggle_key = 1;
						ui_active = !ui_active;
						ui_display_count = 30;
						schedule_full_refresh();
					}
				}
				else
				{
					ui_toggle_key = 0;
				}

				if (ui_active)
				{
					if( ui_display_count > 0 )
					{
							buf[0] = 0;
							strcpy(buf,ui_getstring (UI_keyb1));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb2));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb3));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb5));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb2));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb7));
							strcat(buf,"\n");
							ui_displaymessagewindow(bitmap, buf);

						if( --ui_display_count == 0 )
							schedule_full_refresh();
					}
				}
				else
				{
					if( ui_display_count > 0 )
					{
							buf[0] = 0;
							strcpy(buf,ui_getstring (UI_keyb1));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb2));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb4));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb6));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb2));
							strcat(buf,"\n");
							strcat(buf,ui_getstring (UI_keyb7));
							strcat(buf,"\n");
							ui_displaymessagewindow(bitmap, buf);

						if( --ui_display_count == 0 )
							schedule_full_refresh();
					}
				}
			}
			if (((Machine->gamedrv->flags & GAME_COMPUTER) == 0) || ui_active)
				trying_to_quit = handle_user_interface(bitmap);
		}

		/* run display routine for device */
		for (type = 0; type < IO_COUNT; type++)
		{
			dev = device_find(Machine->gamedrv, type);
			if (dev && dev->display)
			{
				for (id = 0; id < MAX_DEV_INSTANCES; id++)
					dev->display(bitmap, id);
			}
		}
	}
	return trying_to_quit;
}
