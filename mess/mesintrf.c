#include "mesintrf.h"
#include "mame.h"
#include "ui_text.h"
#include "input.h"

int mess_pause_for_ui = 0;
static int ui_active = 0;

int mess_ui_active(void)
{
	return ui_active;
}

int handle_mess_user_interface(struct mame_bitmap *bitmap)
{
	static int ui_toggle_key = 0;
	static int ui_display_count = 30;

	char buf[2048];
	int trying_to_quit;
	int id;
	const struct IODevice *dev;

	trying_to_quit = osd_trying_to_quit();
	if (!trying_to_quit)
	{
		if (options.disable_normal_ui)
		{
			/* we are using the new UI; just do the minimal stuff here */
			if (ui_show_profiler_get())
				profiler_show(bitmap);
			ui_display_fps(bitmap);
		}
		else
		{
			/* traditional MESS interface */
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
		if (devices_inited)
		{
			for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
			{
				if (dev->display)
				{
					for (id = 0; id < device_count(dev->type); id++)
					{
						mess_image *img = image_from_devtype_and_index(dev->type, id);
						dev->display(img, bitmap);
					}
				}
			}
		}
	}
	return trying_to_quit;
}

int displayimageinfo(struct mame_bitmap *bitmap, int selected)
{
	char buf[2048], *dst = buf;
	const struct IODevice *dev;
	int id, sel = selected - 1;

	dst += sprintf(dst, "%s\n\n", Machine->gamedrv->description);

	if (options.ram)
	{
		char buf2[RAM_STRING_BUFLEN];
		dst += sprintf(dst, "RAM: %s\n\n", ram_string(buf2, options.ram));
	}

	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		for (id = 0; id < dev->count; id++)
		{
			mess_image *img = image_from_device_and_index(dev, id);
			const char *name = image_filename(img);
			if( name )
			{
				const char *base_filename;
				const char *info;
				char *base_filename_noextension;

				base_filename = image_basename(img);
				base_filename_noextension = strip_extension((char *) base_filename);

				/* display device type and filename */
				dst += sprintf(dst,"%s: %s\n", image_typename_id(img), base_filename);

				/* display long filename, if present and doesn't correspond to name */
				info = image_longname(img);
				if (info && (!base_filename_noextension || strcmpi(info, base_filename_noextension)))
					dst += sprintf(dst,"%s\n", info);

				/* display manufacturer, if available */
				info = image_manufacturer(img);
				if (info)
				{
					dst += sprintf(dst,"%s", info);
					info = stripspace(image_year(img));
					if (info && *info)
						dst += sprintf(dst,", %s", info);
					dst += sprintf(dst,"\n");
				}

				/* display playable information, if available */
				info = image_playable(img);
				if (info)
					dst += sprintf(dst,"%s\n", info);

// why is extrainfo printed? only MSX and NES use it that i know of ... Cowering
//				info = device_extrainfo(type,id);
//				if( info )
//					dst += sprintf(dst,"%s\n", info);

				if (base_filename_noextension)
					free(base_filename_noextension);
			}
			else
			{
				dst += sprintf(dst,"%s: ---\n", image_typename_id(img));
			}
		}
	}

	if (sel == -1)
	{
		/* startup info, print MAME version and ask for any key */

		strcat(buf,"\n\t");
		strcat(buf,ui_getstring(UI_anykey));
		ui_drawbox(bitmap,0,0,Machine->uiwidth,Machine->uiheight);
		ui_displaymessagewindow(bitmap, buf);

		sel = 0;
		if (code_read_async() != CODE_NONE)
			sel = -1;
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t");
		strcat(buf,ui_getstring(UI_lefthilight));
		strcat(buf," ");
		strcat(buf,ui_getstring(UI_returntomain));
		strcat(buf," ");
		strcat(buf,ui_getstring(UI_righthilight));

		ui_displaymessagewindow(bitmap,buf);

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		schedule_full_refresh();
	}

	return sel + 1;
}
