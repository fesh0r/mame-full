/* PC floppy disc image handling code */
/* - uses basic disc format */
/* - reads boot sector to determine geometry */
/* - or uses size of file */

#include "driver.h"
#include "devices/basicdsk.h"
#include "devices/pc_flopp.h"

static int common_length_spt_heads[][3] = {
    { 8*1*40*512,  8, 1},   /* 5 1/4 inch double density single sided */
    { 8*2*40*512,  8, 2},   /* 5 1/4 inch double density */
    { 9*1*40*512,  9, 1},   /* 5 1/4 inch double density single sided */
    { 9*2*40*512,  9, 2},   /* 5 1/4 inch double density */
    {10*2*40*512, 10, 2},   /* 5 1/4 inch double density single sided */
    { 9*2*80*512,  9, 2},   /* 80 tracks 5 1/4 inch drives rare in PCs */
    { 9*2*80*512,  9, 2},   /* 3 1/2 inch double density */
    {15*2*80*512, 15, 2},   /* 5 1/4 inch high density (or japanese 3 1/2 inch high density) */
    {18*2*80*512, 18, 2},   /* 3 1/2 inch high density */
    {36*2*80*512, 36, 2}};  /* 3 1/2 inch enhanced density */


DEVICE_LOAD(pc_floppy)
{
	if (device_load_basicdsk_floppy(image, file, open_mode) == INIT_PASS)
	{
		int i;
		int scl, spt,heads;

		/* find the sectors/track and bytes/sector values in the boot sector */
		if (file)
		{
			int length;

			/* tracks pre sector recognition with image size
			works only 512 byte sectors! and 40 or 80 tracks*/
			scl = heads = 2;
			length = mame_fsize(file);
			if (length==0) { // new image created
#if 0
			    logerror("image with heads per track:%d, heads:%d, tracks:%d created\n", 9, 2, 40);
			    basicdsk_set_geometry(id, 40, 2, 9, 512, 01, 0);
			    return INIT_PASS;
#else
			    // creation is annoying for new
			    logerror("no automatic creation of images\n");
			    return INIT_FAIL;
#endif
			}
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
			    if (length<0x1a) {
				logerror("image too short(%d)\n",length);
				return INIT_FAIL;
			    }
				/*
				 * get info from boot sector.
				 * not correct on all disks
				 */
				mame_fseek(file, 0x0c, SEEK_SET);
				mame_fread(file, &scl, 1);
				mame_fseek(file, 0x018, SEEK_SET);
				mame_fread(file, &spt, 1);
				mame_fseek(file, 0x01a, SEEK_SET);
				mame_fread(file, &heads, 1);
				
				if (scl*spt*heads*0x200!=length) { // seems neccessary for plain disk images
				    logerror("image doesn't match boot sector param. length %d, sectors:%d, heads:%d, tracks:%d\n",
					     length, spt, heads, scl);
				    return INIT_FAIL;
				}
			}

			basicdsk_set_geometry(image, 80, heads, spt, 512, 01, 0, FALSE);

			return INIT_PASS;
		}
	}
	return INIT_PASS;
}
