/*
 * printer.c : simple printer port implementation
 * "online/offline" and output only.
 */

#include "driver.h"
#include "devices/printer.h"
#include "image.h"

static mame_file *prn_ports[MAX_PRINTER]= { 0 };

int printer_init(int id, mame_file *fp, int open_mode)
{
	prn_ports[id] = fp;
	return INIT_PASS;
}

void printer_exit (int id)
{
	if (prn_ports[id])
	{
		mame_fclose (prn_ports[id]);
		prn_ports[id] = NULL;
	}
}

int printer_status (int id, int newstatus)
{
	/* if there is a file attached to it, it's online */
	return (prn_ports[id] != 0);
}

void printer_output (int id, int data)
{
	UINT8 d=data;

	if (!prn_ports[id])
	{
		logerror ("Printer port %d# data written while port not open\n", id);
		return;
	}

	if (1 != mame_fwrite (prn_ports[id], &d, 1) )
	{
		logerror ("Printer port %d# failed to write data\n",id );
		return;
	}

//	logerror ("Printer port %d# write %.2x data\n",id, d );
	return;
}

int printer_output_chunk (int id, void *src, int chunks)
{
	int chunks_written;

	if (!prn_ports[id])
	{
		logerror ("Printer port %d# data written while port not open\n", id);
		return 0;
	}

	chunks_written = mame_fwrite (prn_ports[id], src, chunks);
	if (chunks != chunks_written)
	{
		logerror ("Printer port %d# failed to write data\n", id);
	}

	return chunks_written;
}

