/*
 * printer.c : simple printer port implementation
 * "online/offline" and output only.
 */

#include "driver.h"
#include "devices/printer.h"
#include "image.h"

int printer_load(int id, mame_file *fp, int open_mode)
{
	return INIT_PASS;
}

int printer_status (int id, int newstatus)
{
	/* if there is a file attached to it, it's online */
	return (image_exists(IO_PRINTER, id) != 0);
}

void printer_output (int id, int data)
{
	UINT8 d=data;
	mame_file *fp;

	fp = image_fp(IO_PRINTER, id);

	if (!fp)
	{
		logerror ("Printer port %d# data written while port not open\n", id);
		return;
	}

	if (1 != mame_fwrite (fp, &d, 1) )
	{
		logerror ("Printer port %d# failed to write data\n",id );
		return;
	}
}

