/* PC floppy disc image handling code */
/* - uses basic disc format */
/* - reads boot sector to determine geometry */
/* - or uses size of file */

#include "driver.h"
#include "includes/basicdsk.h"


static int common_length_spt_heads[][3] = {
    { 8*1*40*512,  8, 1},   /* 5 1/4 inch double density single sided */
    { 8*2*40*512,  8, 2},   /* 5 1/4 inch double density */
    { 9*1*40*512,  9, 1},   /* 5 1/4 inch double density single sided */
    { 9*2*40*512,  9, 2},   /* 5 1/4 inch double density */
    { 9*2*80*512,  9, 2},   /* 80 tracks 5 1/4 inch drives rare in PCs */
    { 9*2*80*512,  9, 2},   /* 3 1/2 inch double density */
    {15*2*80*512, 15, 2},   /* 5 1/4 inch high density (or japanese 3 1/2 inch high density) */
    {18*2*80*512, 18, 2},   /* 3 1/2 inch high density */
    {36*2*80*512, 36, 2}};  /* 3 1/2 inch enhanced density */


int pc_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_OK)
	{
		int i;
		int scl, spt,heads;

		void *file;

		file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_R, OSD_FOPEN_READ);

		/* find the sectors/track and bytes/sector values in the boot sector */
		if( file )
		{
			int length;

			/* tracks pre sector recognition with image size
			works only 512 byte sectors! and 40 or 80 tracks*/
			scl = heads = 2;
			length=osd_fsize(file);
			for( i = sizeof(common_length_spt_heads)/sizeof(common_length_spt_heads[0])-1; i >= 0; --i )
			{
				if( length == common_length_spt_heads[i][0] )
				{
					spt = common_length_spt_heads[i][1];
					heads = common_length_spt_heads[i][2];
					break;
				}
			}
			if( i < 0 )
			{
				/*
				 * get info from boot sector.
				 * not correct on all disks
				 */
				osd_fseek(file, 0x0c, SEEK_SET);
				osd_fread(file, &scl, 1);
				osd_fseek(file, 0x018, SEEK_SET);
				osd_fread(file, &spt, 1);
				osd_fseek(file, 0x01a, SEEK_SET);
				osd_fread(file, &heads, 1);
			}

			basicdsk_set_geometry(id, 80, heads, spt, 512, 01);

			return INIT_OK;
		}
	}
	return INIT_FAILED;
}

void pc_floppy_exit(int id)
{
	 basicdsk_floppy_exit(id);
}

