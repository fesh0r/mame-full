/*
 * printer.c : simple printer port implementation
 * "online/offline" and output only.
 */

#include "driver.h"
#include "printer.h"

static void *prn_ports[MAX_PRINTER]= { 0 };

int printer_init (int id)
{
	void *f;

	if (!device_filename(IO_PRINTER,id) || !strlen(device_filename(IO_PRINTER,id) ))
		return 0;

	f = image_fopen (IO_PRINTER, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_WRITE);
	if (!f)
		return INIT_FAIL;

	prn_ports[id] = f;
	return INIT_PASS;
}

void printer_exit (int id)
{
	if (prn_ports[id])
	{
		osd_fclose (prn_ports[id]);
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

	if (1 != osd_fwrite (prn_ports[id], &d, 1) )
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

	chunks_written = osd_fwrite (prn_ports[id], src, chunks);
	if (chunks != chunks_written)
	{
		logerror ("Printer port %d# failed to write data\n");
		return chunks_written;
	}

	return chunks;
}

