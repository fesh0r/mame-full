/* This is a .D88 disc image format. */

#include "driver.h"
#include "includes/d88.h"
#include "includes/flopdrv.h"

#define d88image_MAX_DRIVES 4
#define VERBOSE 1
static d88image     d88image_drives[d88image_MAX_DRIVES];

static void d88image_seek_callback(int,int);
static int d88image_get_sectors_per_track(int,int);
static void d88image_get_id_callback(int, chrn_id *, int, int);
static void d88image_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length);
static void d88image_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length,int ddam);

floppy_interface d88image_floppy_interface=
{
	d88image_seek_callback,
	d88image_get_sectors_per_track,             /* done */
	d88image_get_id_callback,                   /* done */
	d88image_read_sector_data_into_buffer,      /* done */
	d88image_write_sector_data_from_buffer, /* done */
	NULL,
	NULL
};

/* attempt to insert a disk into the drive specified with id */
int d88image_floppy_init(int id)
{
	UINT8 tmp8;
	UINT16 tmp16;
	UINT32 tmp32;
	int i,j,k;
	unsigned long toffset;

	const char *name = device_filename(IO_FLOPPY, id);
	/* do we have an image name ? */
	if (!name)
		return INIT_PASS;

	if (id < d88image_MAX_DRIVES)
	{
		d88image *w = &d88image_drives[id];
		int effective_mode;

		w->image_file = image_fopen_new(IO_FLOPPY, id, &effective_mode);
		w->mode = (w->image_file) && is_effective_mode_writable(effective_mode);

		/* the following line is unsafe, but floppy_drives_init assumes we start on track 0,
		so we need to reflect this */
		w->track = 0;

		osd_fread(w->image_file, w->disk_name, 17);
		for(i=0;i<9;i++) osd_fread(w->image_file, &tmp8, 1);
		osd_fread(w->image_file, &tmp8, 1);
		w->write_protected = (tmp8&0x10 || !w->mode);
		osd_fread(w->image_file, &tmp8, 1);
		w->disktype = tmp8 >> 4;
		osd_fread_lsbfirst(w->image_file, &tmp32, 4);
		w->image_size=tmp32;

		for(i=0;i<D88_NUM_TRACK;i++) {
		  osd_fseek(w->image_file, 0x20 + i*4, SEEK_SET);
		  osd_fread_lsbfirst(w->image_file, &tmp32, 4);
		  toffset = tmp32;
		  if(toffset) {
		    osd_fseek(w->image_file, toffset + 4, SEEK_SET);
		    osd_fread_lsbfirst(w->image_file, &tmp16, 2);
		    w->num_sects[i] = tmp16;
		    w->sects[i]=malloc(sizeof(d88sect)*w->num_sects[i]);
		    osd_fseek(w->image_file, toffset, SEEK_SET);

		    for(j=0;j<w->num_sects[i];j++) {
		      osd_fread(w->image_file, &(w->sects[i][j].C), 1);
		      osd_fread(w->image_file, &(w->sects[i][j].H), 1);
		      osd_fread(w->image_file, &(w->sects[i][j].R), 1);
		      osd_fread(w->image_file, &(w->sects[i][j].N), 1);
		      osd_fread_lsbfirst(w->image_file, &tmp16, 2);
		      osd_fread(w->image_file, &tmp8, 1);
		      w->sects[i][j].den=tmp8&0x40 ?
			(w->disktype==2 ? DEN_FM_HI : DEN_FM_LO) :
			(w->disktype==2 ? DEN_MFM_HI : DEN_MFM_LO);
		      osd_fread(w->image_file, &tmp8, 1);
		      w->sects[i][j].flags=tmp8&0x10 ? ID_FLAG_DELETED_DATA : 0;
		      osd_fread(w->image_file, &tmp8, 1);
		      switch(tmp8 & 0xf0) {
		      case 0xa0:
			w->sects[i][j].flags|=ID_FLAG_CRC_ERROR_IN_ID_FIELD;
			break;
		      case 0xb0:
			w->sects[i][j].flags|=ID_FLAG_CRC_ERROR_IN_DATA_FIELD;
			break;
		      }
		      for(k=0;k<5;k++) osd_fread(w->image_file, &tmp8, 1);
		      osd_fread_lsbfirst(w->image_file, &tmp16, 2);
		      w->sects[i][j].offset = osd_ftell(w->image_file);
		      osd_fseek(w->image_file, tmp16, SEEK_CUR);
		    }
		  } else {
		    w->num_sects[i] = 0;
		    w->sects[i]=NULL;
		  }
		}

                floppy_drive_set_disk_image_interface(id,&d88image_floppy_interface);

		return  INIT_PASS;
	}

	return INIT_FAIL;
}

/* remove a disk from the drive specified by id */
void d88image_floppy_exit(int id)
{
	d88image *w;
	int i;

	/* sanity check */
	if ((id<0) || (id>=d88image_MAX_DRIVES))
		return;

	w = &d88image_drives[id];

	/* if file was opened, close it */
	if (w->image_file!=NULL)
	{
		osd_fclose(w->image_file);
		w->image_file = NULL;
	}

	/* free sector map */
	for(i=0;i<D88_NUM_TRACK;i++) {
	  if(w->sects[i]!=NULL) {
	    free(w->sects[i]);
	    w->sects[i]=NULL;
	  }
	}
}

/* seek to track/head/sector relative position in image file */
static int d88image_seek(d88image * w, UINT8 t, UINT8 h, UINT8 s)
{
unsigned long offset;
	/* allow two additional tracks */
    if (t >= D88_NUM_TRACK/2)
	{
		logerror("d88image track %d >= %d\n", t, D88_NUM_TRACK/2);
		return 0;
	}

    if (h >= 2)
    {
		logerror("d88image head %d >= %d\n", h, 2);
		return 0;
	}

    if (s >= w->num_sects[t*2+h])
	{
		logerror("d88image sector %d\n", w->num_sects[t*2+h]);
		return 0;
	}

	offset = w->sects[t*2+h][s].offset;


#if VERBOSE
    logerror("d88image seek track:%d head:%d sector:%d-> offset #0x%08lX\n",
             t, h, s, offset);
#endif

	if (offset > w->image_size)
	{
		logerror("d88image seek offset %ld >= %ld\n", offset, w->image_size);
		return 0;
	}

	if (osd_fseek(w->image_file, offset, SEEK_SET) < 0)
	{
		logerror("d88image seek failed\n");
		return 0;
	}

	return 1;
}

void    d88image_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	d88image *w = &d88image_drives[drive];
	d88sect *s = &(w->sects[w->track*2+side][id_index]);

	/* construct a id value */
	id->C = s->C;
	id->H = s->H;
	id->R = s->R;
	id->N = s->N;
	id->data_id = id_index;
	id->flags = s->flags;
}

int  d88image_get_sectors_per_track(int drive, int side)
{
	d88image *w = &d88image_drives[drive];

	/* attempting to access an invalid side or track? */
	if ((side>=2) || (w->track>=D88_NUM_TRACK/2))
	{
		/* no sectors */
		return 0;
	}
	/* return number of sectors per track */
	return w->num_sects[w->track*2+side];
}

void    d88image_seek_callback(int drive, int physical_track)
{
	d88image *w = &d88image_drives[drive];

	w->track = physical_track;
}

void d88image_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length, int ddam)
{
	d88image *w = &d88image_drives[drive];
	d88sect *s = &(w->sects[w->track*2+side][index1]);

	if (d88image_seek(w, w->track, side, index1))
	{
		osd_fwrite(w->image_file, ptr, length);
	}

	s->flags = ddam ? ID_FLAG_DELETED_DATA : 0;
}

void d88image_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length)
{
	d88image *w = &d88image_drives[drive];

	if (d88image_seek(w, w->track, side, index1))
	{
		osd_fread(w->image_file, ptr, length);
	}
}


