/*
 * MSX disk emulation
 *
 * Simple file I/O, with multidsk support.
 */

#include "driver.h"
#include "includes/msx.h"

struct MSX_DSK {
	void *fh;
    int	readonly;
	int size;
    int seek;
	int disk;
    int wp;
};

static struct MSX_DSK dsk[2];

int msx_dsk_id (int id)
	{
    void *f;
    int size, ret;
    
   	f = image_fopen (IO_FLOPPY, id, OSD_FILETYPE_IMAGE_R, 0);

    ret = 0;
	if (f)
		{
		size = osd_fsize (f);
		if (size == 360*1024) /* 1DD .dsk format */
			ret = 1;
		else if (size && !(size % 720*1024) ) /* 2DD .dsk (including multidisks) */
			ret = 1;

		osd_fclose (f);
		}

	return (ret ? INIT_OK : INIT_FAILED);
	}

int msx_dsk_init (int id)
	{
	void *f;
	int size, ret;

	dsk[id].readonly = 0;
	dsk[id].wp = 0;
    f = image_fopen (IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if (!f)
		{
		f = image_fopen (IO_FLOPPY, id, OSD_FILETYPE_IMAGE_R, 0);
		dsk[id].readonly = 1;
		}

    ret = 0;
	if (f)
		{
		size = osd_fsize (f);
		if (size == 360*1024) /* 1DD .dsk format */
			ret = 1;
		else if (size && !(size % 720*1024) ) /* 2DD .dsk (including multidisks) */
			ret = 1;

		if (ret)
			{
			dsk[id].fh = f;
			dsk[id].size = size;
			}
		else
			osd_fclose (f);
		}

	return (ret ? INIT_OK : INIT_FAILED);
	}

void msx_dsk_exit (int id)
	{
	if (dsk[id].fh)
		{
		osd_fclose (dsk[id].fh);
		dsk[id].fh = NULL;
		}
	}

int msx_dsk_status (int id, int newstatus)
	{
	int i;

	if (id > 1) return -1;

	if (newstatus == -1)
		return dsk[id].fh != NULL;

	i = dsk[id].wp;
	dsk[id].wp = newstatus;
	return i;
	}

int msx_dsk_seek (int id, int offset, int whence)
	{
	int size;

	if (id > 1) return -1;

	switch (whence)
		{
		case SEEK_SET:
			dsk[id].seek = offset;
			break;
		case SEEK_CUR:
			dsk[id].seek += offset;
			break;
		case SEEK_END:
			size = (dsk[id].size > 720*1024 ? 720*1024 : dsk[id].size);
			dsk[id].seek = size + offset;
			break;
		}

	return dsk[id].seek;
	}

int msx_dsk_tell (int id)
	{
	return dsk[id].seek;
	}

int msx_dsk_output_chunk (int id, void *src, int chunks)
	{
	int size, seek, multidsk, err = 0;
	/* write to disk */
	if (id > 1 || !dsk[id].fh) return MSX_DSK_ERR_OFFLINE;
	if (dsk[id].readonly) return MSX_DSK_ERR_WRITEPROTECTED;

	if (dsk[id].size > 720*1024)
		{
		size = 720*1024;
		multidsk = 1;
		}
	else
		{
		size = dsk[id].size;
		multidsk = 0;
		}
	if (dsk[id].seek + chunks > size)
		{
		chunks = size - dsk[id].seek; 
		err = MSX_DSK_ERR_SECTORNOTFOUND;
		}
	if (chunks <= 0) return err;

	seek = (multidsk ? dsk[id].disk * 720*1024 : 0) + dsk[id].seek;
	
	osd_fseek (dsk[id].fh, seek, 0);
	
   	return osd_fwrite (dsk[id].fh, src, chunks)|err;
	}

int msx_dsk_input (int id)
	{
	return dsk[id].disk;
	}

void msx_dsk_output (int id, int i)
	{
	dsk[id].disk = i;
	}

int msx_dsk_input_chunk (int id, void *dst, int chunks)
	{
	int size, seek, multidsk, err = 0;

	if (id > 1 || !dsk[id].fh) return MSX_DSK_ERR_OFFLINE;
	/* read from disk */

	if (dsk[id].size > 720*1024)
		{
		size = 720*1024;
		multidsk = 1;
		}
	else
		{
		size = dsk[id].size;
		multidsk = 0;
		}

	if (dsk[id].seek + chunks > size)
		{
		chunks = size - dsk[id].seek; 
		err = MSX_DSK_ERR_SECTORNOTFOUND;
		}
	if (chunks <= 0) return err;

	seek = (multidsk ? dsk[id].disk * 720*1024 : 0) + dsk[id].seek;
	if (seek >= dsk[id].size) return MSX_DSK_ERR_OFFLINE;

	osd_fseek (dsk[id].fh, seek, 0);

   	return osd_fread (dsk[id].fh, dst, chunks)|err;
	}
