/* PC floppy disc image handling code */
#include "driver.h"
#include "mess/includes/pc_flopp.h"
// temp to include floppy stuff
#include "mess/includes/nec765.h"

void *pc_fdc_file[2];			   /* up to two floppy disk images */
UINT8 pc_fdc_heads[2] = {2,2};	   /* 2 heads */
UINT8 pc_fdc_spt[2] = {9,9};	   /* 9 sectors per track */
UINT8 pc_gpl[2] = {42,42};		   /* gap III length */
UINT8 pc_fill[2] = {0xf6,0xf6};    /* filler byte */
UINT8 pc_fdc_scl[2] = {2,2};	   /* 512 bytes per sector */

//static UINT8 drv;				   /* drive number (0..3) */
//static UINT8 FDC_motor; 		   /* motor running flags */
static UINT8 pc_fdd_track[2] = {0,0};	   /* floppy current track numbers */
static UINT8 pc_fdd_head[2] = {0,0};	   /* floppy current head numbers */
static UINT8 sector[2] = {0,0};    /* floppy current sector numbers */
static int offset[2];			   /* current seek() offset into disk images */

//static int FDC_floppy[2] =			/* floppy drive status flags */
//	{ FAULT, FAULT };

static void pc_fdc_seek_callback(int drv, int track);
static int pc_fdc_get_spt_callback(int drive, int side);
static void pc_fdc_get_id_callback(int drive, nec765_id *id, int id_index, int side);
static void pc_fdc_get_sector_data_callback(int drive, int id, int side, char **ptr);


static floppy_interface pc_floppy_interface=
{
	pc_fdc_seek_callback,
	pc_fdc_get_spt_callback,
	pc_fdc_get_id_callback,
	pc_fdc_get_sector_data_callback,
};

int pc_floppy_init(int id)
{
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
	int i;

    pc_fdc_file[id] = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	/* find the sectors/track and bytes/sector values in the boot sector */
	if( pc_fdc_file[id] )
	{
		int length;

		/* tracks pre sector recognition with image size
		   works only 512 byte sectors! and 40 or 80 tracks*/
		pc_fdc_scl[id]=2;
		pc_fdc_heads[id]=2;
		length=osd_fsize(pc_fdc_file[id]);
		for( i = sizeof(common_length_spt_heads)/sizeof(common_length_spt_heads[0])-1; i >= 0; --i )
		{
			if( length == common_length_spt_heads[i][0] )
			{
				pc_fdc_spt[id] = common_length_spt_heads[i][1];
				pc_fdc_heads[id] = common_length_spt_heads[i][2];
				break;
			}
		}
		if( i < 0 )
		{
			/*
			 * get info from boot sector.
			 * not correct on all disks
			 */
			osd_fseek(pc_fdc_file[id], 0x0c, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_scl[id], 1);
			osd_fseek(pc_fdc_file[id], 0x018, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_spt[id], 1);
			osd_fseek(pc_fdc_file[id], 0x01a, SEEK_SET);
			osd_fread(pc_fdc_file[id], &pc_fdc_heads[id], 1);
		}

		floppy_drive_setup_drive_status(id); /* mark disk as inserted */
		floppy_set_interface(id, &pc_floppy_interface);
		floppy_drive_set_geometry(id, FLOPPY_DRIVE_DS_80);

	}
	return INIT_OK;
}

void pc_floppy_exit(int id)
{
	if( pc_fdc_file[id] )
		osd_fclose(pc_fdc_file[id]);
	pc_fdc_file[id] = NULL;
}

/* called by nec765 when seek has been performed */
static void pc_fdc_seek_callback(int drv, int track)
{
	void *f = pc_fdc_file[drv];
	pc_fdd_track[drv] = track;
	//pc_fdd_head[drv] = side;

	offset[drv] = (pc_fdd_track[drv] * pc_fdc_heads[drv] + pc_fdd_head[drv]) * pc_fdc_spt[drv] * pc_fdc_scl[drv] * 256;
//	FDC_LOG(1,"FDC_seek_execute",(errorlog, "T:%02d H:%d $%08x\n", pc_fdd_track[drv], pc_fdd_head[drv], offset[drv]));
	if (f) 
	{
		osd_fseek(f, offset[drv], SEEK_SET);
	}
}


/* temp */
static int pc_fdc_get_spt_callback(int drive, int side)
{
	return pc_fdc_spt[drive];
}

/* temp */
static void pc_fdc_get_id_callback(int drive, nec765_id *id, int id_index, int side)
{
	/* temp */
	id->C = pc_fdd_track[drive];
	id->H = side;
	id->R = id_index+1;
	id->N = 2;
}

/* 8k enough for a sector? */
static char sector_buffer[8*1024];
static int cached_drive, cached_side, cached_id;

/* temp */
//static void pc_fdc_get_sector_data_callback(int drive, int id, int side, char **ptr)
static void pc_fdc_get_sector_data_callback(int drv, int id, int side, char **ptr)
{
	void *f = pc_fdc_file[drv];

	pc_fdd_head[drv] = side;
	sector[drv] = id+1;

	/* reading a different sector from a different drive or side? */
#if 0
	if (
		((drive!=cached_drive) && (cached_drive!=-1)) ||
		((side!=cached_side) && (cached_side!=-1)) ||
		((id!=cached_id) && (cached_id!=-1)))
	{
		if (f)
		{
			/* write back buffer */
			osd_fseek(f, offset[drv], SEEK_SET);
			osd_fwrite(f, sector_buffer, 512 /* temp */);
		}
	}
#endif
	/* seek to new one and read */
	offset[drv] = ((((pc_fdd_track[drv] * pc_fdc_heads[drv]) + pc_fdd_head[drv]) * pc_fdc_spt[drv]) + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;

//	cached_drive = drive;
	cached_drive = drv;
	cached_id = id;
	cached_side = side;

	if (f) 
	{
		osd_fseek(f, offset[drv], SEEK_SET);
        osd_fread(f, sector_buffer, 512 /* temp */);
	}

	*ptr = sector_buffer;

}
