#include "driver.h"
#include "includes/flopdrv.h"

/* used to tell updatescreen() to clear the bitmap */
extern int need_to_clear_bitmap;

/* toggle a bit in the status */
void	disk_control_toggle_bit(int id, unsigned long bit_mask)
{
	int status;

	/* get current status */
	status = device_status(IO_FLOPPY,id,-1);

	/* toggle bit */
	status^=bit_mask;

	/* set new status */
	device_status(IO_FLOPPY, id, status);
}

/* disk control UI menu */
int diskcontrol(struct osd_bitmap *bitmap, int selected)
{
	static int id = 0;
	char device_index[2];
	const char *menu_item[40];
	const char *menu_subitem[40];
	char flag[40];
	const char *item;

    int sel;
    int total;
    int arrowize;
    int num_devices = device_count(IO_FLOPPY);
    total = 0;
    sel = selected - 1;

	if (num_devices!=0)
	{
		int status;

		status = device_status(IO_FLOPPY, id, -1);

		/**  display device type and number **/
		device_index[0] = id+'0';
		device_index[1] = '\0';
		menu_item[total] = device_typename_id(IO_FLOPPY,id);
		menu_subitem[total] = device_index;
		flag[total] = 0;
		total++;

		/** display connected status **/
		menu_item[total] = "Device Connected Status";
        
		/* is drive connected? - some drives can be connected and disconnected.
			e.g. an external 3.5" drive connected to an Amstrad CPC. */
		item = "disconnected";
		if (status & FLOPPY_DRIVE_CONNECTED)
		{
			item = "connected";
		}
		menu_subitem[total] = item;
		flag[total] = 0;
		total++;
        
		/** display write protect status **/
		/* is disc write protected? */
		menu_item[total] = "Write Protect Status";
         
		/* this is the status if drive is not active, or it is active but there is not a disc in the drive */
		item = "---";

		/* drive active? */
		if (status & FLOPPY_DRIVE_CONNECTED)
		{
			/* disk in drive */
			if (status & FLOPPY_DRIVE_DISK_INSERTED)
			{
				/* report status of disc */
				if (status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED)
				{
					item = "write disable";
				}
				else
				{
					item = "write enable";
				}
			}
		}
        
		menu_subitem[total] = item;
		flag[total] = 0;
		total++;

		/** display device type **/
		menu_item[total] = "Device Type";

		item = "---";

		/* real fdd or image? */
		if (status & FLOPPY_DRIVE_REAL_FDD)
		{
			item = "real fdd";
		}
		else
		{
			item = "image file";
		}

		menu_subitem[total] = item;
		flag[total] = 0;
		total++;
    }
    else
    {
		menu_item[total] = "System does not have disk drive(s)!";
		menu_subitem[total] = 0;
		flag[total] = 0;
		total++;
    }

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

    if (num_devices!=0)
    {
		if (input_ui_pressed(IPT_UI_LEFT))
		{
			switch (sel)
			{
			case 0:
				id = id--;

				if (id<0)
				{
				  id = num_devices-1;
				}
				break;
			case 1:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_CONNECTED);
				break;
			case 2:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_DISK_WRITE_PROTECTED);
				break;
			case 3:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_REAL_FDD);
				break;
			default:
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
				id = id++;

				if (id>=num_devices)
				{
				   id = 0;
				}
				break;
			case 1:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_CONNECTED);
				break;
			case 2:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_DISK_WRITE_PROTECTED);
				break;
			case 3:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_REAL_FDD);
				break;

			default:
				break;
			}
		/* tell updatescreen() to clean after us (in case the window changes size) */
		need_to_clear_bitmap = 1;
		}
    }

    if (input_ui_pressed(IPT_UI_SELECT))
    {
        if (sel == total - 1)
            sel = -1;
        else if (num_devices!=0)
        {


			switch (sel)
			{
			case 0:
				id = id++;

				if (id>=num_devices)
				{
				   id = num_devices-1;
				}
				break;
			case 1:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_CONNECTED);
				break;
			case 2:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_DISK_WRITE_PROTECTED);
				break;
			case 3:
				disk_control_toggle_bit(id, FLOPPY_DRIVE_REAL_FDD);
				break;
			default:
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


