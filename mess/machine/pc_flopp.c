/* PC floppy disc image handling code */
#include "driver.h"
#include "includes/pc_flopp.h"
#include "includes/flopdrv.h"
// temp to include floppy stuff
//#include "includes/nec765.h"

void *pc_fdc_file[2];			   /* up to two floppy disk images */
UINT8 pc_fdc_heads[2] = {2,2};	   /* 2 heads */
UINT8 pc_fdc_spt[2] = {9,9};	   /* 9 sectors per track */
/*UINT8 pc_gpl[2] = {42,42};*/	   /* gap III length */
/*UINT8 pc_fill[2] = {0xf6,0xf6}; */   /* filler byte */
UINT8 pc_fdc_scl[2] = {2,2};	   /* 512 bytes per sector */

//static UINT8 drv;				   /* drive number (0..3) */
//static UINT8 FDC_motor; 		   /* motor running flags */
static UINT8 pc_fdd_track[2] = {0,0};	   /* floppy current track numbers */
static UINT8 pc_fdd_head[2] = {0,0};	   /* floppy current head numbers */
static UINT8 sector[2] = {0,0};    /* floppy current sector numbers */
static int offset[2];			   /* current seek() offset into disk images */

static void pc_fdc_seek_callback(int drv, int track);
static int pc_fdc_get_spt_callback(int drive, int side);
static void pc_fdc_get_id_callback(int drive, struct chrn_id *id, int id_index, int side);
static void pc_fdc_format_sector_callback(int drive, int side, int,int c,int h, int r, int n, int filler);

static void pc_fdc_read_sector_into_buffer(int drive, int side, int id, char *,int);
static void pc_fdc_write_sector_from_buffer(int drive, int side, int id, char *,int);


static floppy_interface pc_floppy_interface=
{
	pc_fdc_seek_callback,
	pc_fdc_get_spt_callback,
	pc_fdc_get_id_callback,
	pc_fdc_read_sector_into_buffer,
	pc_fdc_write_sector_from_buffer,
	pc_fdc_format_sector_callback
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
		floppy_drive_set_interface(id, &pc_floppy_interface);

		floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_PRESENT,1);

	}
	return INIT_OK;
}

void pc_floppy_exit(int id)
{
	if( pc_fdc_file[id] )
		osd_fclose(pc_fdc_file[id]);
	pc_fdc_file[id] = NULL;
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_PRESENT,0);
}

/* called by nec765 when seek has been performed */
static void pc_fdc_seek_callback(int drv, int track)
{
//	void *f = pc_fdc_file[drv];
	pc_fdd_track[drv] = track;
//	//pc_fdd_head[drv] = side;
//
//	offset[drv] = (pc_fdd_track[drv] * pc_fdc_heads[drv] + pc_fdd_head[drv]) * pc_fdc_spt[drv] * pc_fdc_scl[drv] * 256;
//	FDC_LOG(1,"FDC_seek_execute",(errorlog, "T:%02d H:%d $%08x\n", pc_fdd_track[drv], pc_fdd_head[drv], offset[drv]));
//	if (f) 
//	{
//		osd_fseek(f, offset[drv], SEEK_SET);
//	}
}


/* temp */
static int pc_fdc_get_spt_callback(int drive, int side)
{
	return pc_fdc_spt[drive];
}

/* temp */
static void pc_fdc_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	/* temp */
	id->C = pc_fdd_track[drive];
	id->H = side;
	id->R = id_index+1;
	id->N = 2;
	id->data_id = id_index;
	id->flags = 0;
}

static void pc_fdc_read_sector_into_buffer(int drv, int side, int id, char *ptr, int size)
{
	void *f = pc_fdc_file[drv];

	pc_fdd_head[drv] = side;
	sector[drv] = id+1;

	/* seek to new one and read */
	offset[drv] = ((((pc_fdd_track[drv] * pc_fdc_heads[drv]) + pc_fdd_head[drv]) * pc_fdc_spt[drv]) + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;

	if (f) 
	{
		osd_fseek(f, offset[drv], SEEK_SET);
        osd_fread(f, ptr, size);
	}
}

static void pc_fdc_write_sector_from_buffer(int drv, int side, int id, char *ptr, int size)
{
	void *f = pc_fdc_file[drv];

	pc_fdd_head[drv] = side;
	sector[drv] = id+1;

	/* seek to new one and read */
	offset[drv] = ((((pc_fdd_track[drv] * pc_fdc_heads[drv]) + pc_fdd_head[drv]) * pc_fdc_spt[drv]) + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;

	if (f) 
	{
		osd_fseek(f, offset[drv], SEEK_SET);
        osd_fwrite(f, ptr, size);
	}
}


static void pc_fdc_format_sector_callback(int drv, int side, int sector_index,int c,int h, int r, int n, int filler)
{
	void *f = pc_fdc_file[drv];

	/* seek to new one and read */
	pc_fdd_head[drv] = side;
	sector[drv] = sector_index + 1;
	offset[drv] = ((((pc_fdd_track[drv] * pc_fdc_heads[drv]) + pc_fdd_head[drv]) * pc_fdc_spt[drv]) + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;

	if (f)
	{
		char sector_buffer[8192];

		osd_fseek(f, offset[drv], SEEK_SET);
		memset(sector_buffer, filler, 1<<(n+7));
		osd_fwrite(f, sector_buffer, 1<<(n+7));
	}
}
