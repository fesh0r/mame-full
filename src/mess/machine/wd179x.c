/***************************************************************************

  WD179X.c

  Functions to emulate a WD179x floppy disc controller

***************************************************************************/

#include "driver.h"
#include "mess/machine/wd179x.h"

#define VERBOSE 1

/* structure describing a double density track */
#define TRKSIZE_DD      6144
static UINT8 track_DD[][2] = {
	{16, 0x4e}, 	/* 16 * 4E (track lead in)				 */
	{ 8, 0x00}, 	/*	8 * 00 (pre DAM)					 */
	{ 3, 0xf5}, 	/*	3 * F5 (clear CRC)					 */

	{ 1, 0xfe}, 	/* *** sector *** FE (DAM)				 */
	{ 1, 0x80}, 	/*	4 bytes track,head,sector,seclen	 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{22, 0x4e}, 	/* 22 * 4E (sector lead in) 			 */
	{12, 0x00}, 	/* 12 * 00 (pre AM) 					 */
	{ 3, 0xf5}, 	/*	3 * F5 (clear CRC)					 */
	{ 1, 0xfb}, 	/*	1 * FB (AM) 						 */
	{ 1, 0x81}, 	/*	x bytes sector data 				 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{16, 0x4e}, 	/* 16 * 4E (sector lead out)			 */
	{ 8, 0x00}, 	/*	8 * 00 (post sector)				 */
	{ 0, 0x00}, 	/* end of data							 */
};

/* structure describing a single density track */
#define TRKSIZE_SD      3172
static UINT8 track_SD[][2] = {
	{16, 0xff}, 	/* 16 * FF (track lead in)				 */
	{ 8, 0x00}, 	/*	8 * 00 (pre DAM)					 */
	{ 1, 0xfc}, 	/*	1 * FC (clear CRC)					 */

	{11, 0xff}, 	/* *** sector *** 11 * FF				 */
	{ 6, 0x00}, 	/*	6 * 00 (pre DAM)					 */
	{ 1, 0xfe}, 	/*	1 * FE (DAM)						 */
	{ 1, 0x80}, 	/*	4 bytes track,head,sector,seclen	 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{10, 0xff}, 	/* 10 * FF (sector lead in) 			 */
	{ 4, 0x00}, 	/*	4 * 00 (pre AM) 					 */
	{ 1, 0xfb}, 	/*	1 * FB (AM) 						 */
	{ 1, 0x81}, 	/*	x bytes sector data 				 */
	{ 1, 0xf7}, 	/*	1 * F7 (CRC)						 */
	{ 0, 0x00}, 	/* end of data							 */
};

WD179X *wd[MAX_DRIVES];
static UINT8 drv = 0;

void wd179x_init(int active)
{
int i;

	osd_fdc_init();

    for (i = 0; i < MAX_DRIVES; i++)
	{
		wd[i] = malloc(sizeof(WD179X));
		if (!wd[i])
		{
			while (--i >= 0)
			{
				free(wd[i]);
				wd[i] = 0;
			}
			return;
		}
		memset(wd[i], 0, sizeof(WD179X));
		wd[i]->unit = 0;
        wd[i]->tracks = 40;
		wd[i]->heads = 1;
		wd[i]->density = DEN_MFM_LO;
		wd[i]->offset = 0;
		wd[i]->sec_per_track = 18;
		wd[i]->sector_length = 256;
		wd[i]->head = 0;
		wd[i]->track = 0;
		wd[i]->track_reg = 0;
		wd[i]->direction = 1;
		wd[i]->sector = 0;
		wd[i]->data = 0;
		wd[i]->status = (active) ? STA_1_TRACK0 : 0;
		wd[i]->status_drq = 0;
		wd[i]->status_ipl = 0;
		wd[i]->busy_count = 0;
		wd[i]->data_offset = 0;
		wd[i]->data_count = 0;
		wd[i]->image_name = 0;
		wd[i]->image_size = 0;
		wd[i]->dir_sector = 0;
		wd[i]->dir_length = 0;
		wd[i]->secmap = 0;

	}
}

void *wd179x_select_drive(UINT8 drive, UINT8 head, void (*callback) (int), const char *name)
{
WD179X *w = wd[drive];

	if (drive < MAX_DRIVES)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "WD179X select #%d head %d\n", drive, head);
#endif
		drv = drive;
		w->head = head;
		w->status_ipl = STA_1_IPL;
		w->callback = callback;

		if (w->image_name && strcmp(w->image_name, name))
		{
			if (w->image_file)
			{
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "WD179X close image #%d %s\n",
							drv, w->image_name);
#endif
				/* if it wasn't a real floppy disk drive */
				if (w->image_file != REAL_FDD)
					osd_fclose(w->image_file);
				w->image_file = NULL;
			}
		}

        /* do we have an image name ? */
        if (!strlen(name))
		{
			w->status = STA_1_NOT_READY;
			return 0;
		}

		w->image_name = name;

		if (w->image_file)
		{
			if (w->image_file == REAL_FDD)
				osd_fdc_motors(w->unit);
			return w->image_file;
		}

		/* check for fake name to access real floppy disk drives */
        if (stricmp(name, "fd0.dsk") == 0)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "WD179X open [fd0] #%d '%s'\n",
						drv, w->image_name);
#endif
			w->unit = 0;
			w->image_file = REAL_FDD;
			osd_fdc_motors(w->unit);
		}
		else
		/* check for fake name to access real floppy disk drives */
		if (stricmp(name, "fd1.dsk") == 0)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "WD179X open [fd1] #%d '%s'\n",
						drv, w->image_name);
#endif
			w->unit = 1;
			w->image_file = REAL_FDD;
			osd_fdc_motors(w->unit);
		}
        else
        {
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "WD179X open image #%d '%s'\n",
						drv, w->image_name);
#endif
			w->mode = 1;
			w->image_file = osd_fopen(Machine->gamedrv->name, w->image_name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
			if( !w->image_file )
			{
				w->mode = 0;
				w->image_file = osd_fopen(Machine->gamedrv->name, w->image_name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
				if( !w->image_file )
				{
					w->mode = 1;
					w->image_file = osd_fopen(Machine->gamedrv->name, w->image_name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
				}
            }
		}

		if (w->image_file)
		{
			w->track = 0;
			w->head = 0;
			w->sector = 0;
		}

		return w->image_file;
	}
	return 0;
}

void wd179x_stop_drive(void)
{
int i;

	for (i = 0; i < MAX_DRIVES; i++)
	{
	WD179X *w = wd[i];
		w->busy_count = 0;
		w->status = 0;
		w->status_drq = 0;
		if (w->callback)
			(*w->callback) (WD179X_DRQ_CLR);
		w->status_ipl = 0;
		if (w->image_file)
		{
			if (w->image_file == REAL_FDD)
			{
#if 0
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "WD179X drive #%d [fd0] %s closed\n", i, w->image_name);
#endif
				/* stop the motors */
				osd_fdc_interrupt(0);
#endif
            }
			else
			{
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "WD179X drive #%d image %s closed\n", i, w->image_name);
#endif
                osd_fclose(w->image_file);
			}
			w->image_file = NULL;
		}
	}
}

void wd179x_read_sectormap(UINT8 drive, UINT8 * tracks, UINT8 * heads, UINT8 * sec_per_track)
{
WD179X *w = wd[drive];
SECMAP *p;
UINT8 head;

    if (!w->secmap)
		w->secmap = malloc(0x2200);
	osd_fseek(w->image_file, 0, SEEK_SET);
	osd_fread(w->image_file, w->secmap, 0x2200);
	w->offset = 0x2200;
	w->tracks = 0;
	w->heads = 0;
	w->sec_per_track = 0;
	for (p = w->secmap; p->track != 0xff; p++)
	{
		if (p->track > w->tracks)
			w->tracks = p->track;
		if (p->sector > w->sec_per_track)
			w->sec_per_track = p->sector;
		head = (p->status >> 4) & 1;
		if (head > w->heads)
			w->heads = head;
	}
	*tracks = w->tracks++;
	*heads = w->heads++;
	*sec_per_track = w->sec_per_track++;
#if VERBOSE
	if (errorlog)
	{
		fprintf(errorlog, "WD179X geometry for drive #%d is %d tracks, %d heads, %d sec/track\n",
				drive, w->tracks, w->heads, w->sec_per_track);
	}
#endif
}

void wd179x_set_geometry(UINT8 drive, UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT16 dir_sector, UINT16 dir_length)
{
WD179X *w = wd[drive];

	if (drive >= MAX_DRIVES)
	{
		if (errorlog)
			fprintf(errorlog, "WD179X drive #%d not supported!\n", drive);
		return;
	}

#if VERBOSE
	if (errorlog)
	{
		fprintf(errorlog, "WD179X geometry for drive #%d is %d tracks, %d heads, %d sec/track\n",
				drive, tracks, heads, sec_per_track);
		if (dir_length)
		{
			fprintf(errorlog, "WD179X directory at sector # %d, %d sectors\n",
					dir_sector, dir_length);
		}
	}
#endif

	w->density = DEN_MFM_LO;	// !!!!!! for now !!!!!!
    w->tracks = tracks;
	w->heads = heads;
	w->sec_per_track = sec_per_track;
	w->sector_length = sector_length;
	w->dir_sector = dir_sector;
	w->dir_length = dir_length;

	w->image_size = w->tracks * w->heads * w->sec_per_track * w->sector_length;

	if (w->image_file == REAL_FDD)
		osd_fdc_density(w->unit, w->density, w->tracks, w->sec_per_track, w->sec_per_track, 1);
}

/* seek to track/head/sector relative position in image file */
static int seek(WD179X * w, UINT8 t, UINT8 h, UINT8 s)
{
unsigned long offset;
SECMAP *p;
UINT8 head;

    if (w->secmap)
	{
		offset = 0x2200;
		for (p = w->secmap; p->track != 0xff; p++)
		{
			if (p->track == t && p->sector == s)
			{
				head = (p->status & 0x10) >> 4;
				if (head == h)
				{
#if VERBOSE
					if (errorlog)
						fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d-> offset #0x%08lX\n",
								drv, t, h, s, offset);
#endif
					if (osd_fseek(w->image_file, offset, SEEK_SET) < 0)
					{
						if (errorlog)
							fprintf(errorlog, "WD179X seek failed\n");
						return STA_1_SEEK_ERR;
					}
					return 0;
				}
			}
			offset += 0x100;
		}
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d : seek err\n",
					drv, t, h, s);
#endif
		return STA_1_SEEK_ERR;
	}

	/* allow two additional tracks */
    if (t >= w->tracks + 2)
	{
		if (errorlog)
			fprintf(errorlog, "WD179X track %d >= %d\n", t, w->tracks + 2);
		return STA_1_SEEK_ERR;
	}

    if (w->image_file == REAL_FDD)
		return 0;

    if (h >= w->heads)
    {
		if (errorlog)
			fprintf(errorlog, "WD179X head %d >= %d\n", h, w->heads);
		return STA_1_SEEK_ERR;
	}

    if (s >= w->sec_per_track)
	{
		if (errorlog)
			fprintf(errorlog, "WD179X sector %d >= %d\n", w->sector, w->sec_per_track);
		return STA_2_REC_N_FND;
	}

	offset = t;
	offset *= w->heads;
	offset += h;
	offset *= w->sec_per_track;
	offset += s;
	offset *= w->sector_length;

#if VERBOSE
	if (errorlog)
		fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d-> offset #0x%08lX\n",
				drv, t, h, s, offset);
#endif

	if (offset > w->image_size)
	{
		if (errorlog)
			fprintf(errorlog, "WD179X seek offset %ld >= %ld\n", offset, w->image_size);
		return STA_1_SEEK_ERR;
	}

	if (osd_fseek(w->image_file, offset, SEEK_SET) < 0)
	{
		if (errorlog)
			fprintf(errorlog, "WD179X seek failed\n");
		return STA_1_SEEK_ERR;
	}

	return 0;
}

/* return STA_2_REC_TYPE depending on relative sector */
static int deleted_dam(WD179X * w)
{
unsigned rel_sector = (w->track * w->heads + w->head) * w->sec_per_track + w->sector;
SECMAP *p;
UINT8 head;

	if (w->secmap)
	{
		for (p = w->secmap; p->track != 0xff; p++)
		{
			if (p->track == w->track && p->sector == w->sector)
			{
				head = (p->status >> 4) & 1;
				if (w->head == head)
					return p->status & STA_2_REC_TYPE;
			}
		}
		return STA_2_REC_N_FND;
	}
	if (rel_sector >= w->dir_sector && rel_sector < w->dir_sector + w->dir_length)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "WD179X deleted DAM at sector #%d\n", rel_sector);
#endif
		return STA_2_REC_TYPE;
	}
	return 0;
}

/* calculate CRC for data address marks or sector data */
static void calc_crc(UINT16 * crc, UINT8 value)
{
UINT8 l, h;

	l = value ^ (*crc >> 8);
	*crc = (*crc & 0xff) | (l << 8);
	l >>= 4;
	l ^= (*crc >> 8);
	*crc <<= 8;
	*crc = (*crc & 0xff00) | l;
	l = (l << 4) | (l >> 4);
	h = l;
	l = (l << 2) | (l >> 6);
	l &= 0x1f;
	*crc = *crc ^ (l << 8);
	l = h & 0xf0;
	*crc = *crc ^ (l << 8);
	l = (h << 1) | (h >> 7);
	l &= 0xe0;
	*crc = *crc ^ l;
}

/* read the next data address mark */
static void read_dam(WD179X * w)
{
UINT16 crc = 0xffff;

	w->data_offset = 0;
	w->data_count = 6;
	w->buffer[0] = w->track;
	w->buffer[1] = w->head;
	w->buffer[2] = w->sector_dam;
	w->buffer[3] = w->sector_length >> 8;
	calc_crc(&crc, w->buffer[0]);
	calc_crc(&crc, w->buffer[1]);
	calc_crc(&crc, w->buffer[2]);
	calc_crc(&crc, w->buffer[3]);
	w->buffer[4] = crc & 255;
	w->buffer[5] = crc / 256;
	if (++w->sector_dam == w->sec_per_track)
		w->sector_dam = 0;
	w->status_drq = STA_2_DRQ;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_SET);
	w->status = STA_2_DRQ | STA_2_BUSY;
	w->busy_count = 50;
}

/* read a sector */
static void read_sector(WD179X * w)
{
    w->data_offset = 0;
	w->data_count = w->sector_length;

	/* if a track was just formatted */
	if (w->dam_cnt)
	{
		int i;
		for (i = 0; i < w->dam_cnt; i++)
		{
			if (w->track == w->dam_list[i][0] &&
				w->head == w->dam_list[i][1] &&
				w->sector == w->dam_list[i][2])
			{
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "WD179X reading formatted sector %d, track %d, head %d\n", w->sector, w->track, w->head);
#endif
				w->data_offset = w->dam_data[i];
				return;
			}
		}
		/* sector not found, now the track buffer is invalid */
		w->dam_cnt = 0;
	}

    /* if this is the real thing */
    if (w->image_file == REAL_FDD)
    {
	int tries = 3;
		do {
			w->status = osd_fdc_get_sector(w->track, w->head, w->head, w->sector, w->buffer);
			tries--;
		} while (tries && (w->status & (STA_2_REC_N_FND | STA_2_CRC_ERR | STA_2_LOST_DAT)));
		/* no error bits set ? */
		if ((w->status & (STA_2_REC_N_FND | STA_2_CRC_ERR | STA_2_LOST_DAT)) == 0)
		{
			/* start transferring data to the emulation now */
			w->status_drq = STA_2_DRQ;
			if (w->callback)
				(*w->callback) (WD179X_DRQ_SET);
			w->status |= STA_2_DRQ | STA_2_BUSY;
        }
        return;
    }
	else
	if (osd_fread(w->image_file, w->buffer, w->sector_length) != w->sector_length)
	{
		w->status = STA_2_LOST_DAT;
		return;
	}

	w->status_drq = STA_2_DRQ;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_SET);
	w->status = STA_2_DRQ | STA_2_BUSY;
	w->busy_count = 0;
}

/* read an entire track */
static void read_track(WD179X * w)
{
UINT8 *psrc;					   /* pointer to track format structure */
UINT8 *pdst;					   /* pointer to track buffer */
int cnt;					   /* number of bytes to fill in */
UINT16 crc;					   /* id or data CRC */
UINT8 d;						   /* data */
UINT8 t = w->track;			   /* track of DAM */
UINT8 h = w->head;			   /* head of DAM */
UINT8 s = w->sector_dam;		   /* sector of DAM */
UINT16 l = w->sector_length;	   /* sector length of DAM */
int i;

	for (i = 0; i < w->sec_per_track; i++)
	{
		w->dam_list[i][0] = t;
		w->dam_list[i][1] = h;
		w->dam_list[i][2] = i;
		w->dam_list[i][3] = l >> 8;
	}

	pdst = w->buffer;

	if (w->density)
	{
		psrc = track_DD[0];	   /* double density track format */
		cnt = TRKSIZE_DD;
	}
	else
	{
		psrc = track_SD[0];	   /* single density track format */
		cnt = TRKSIZE_SD;
	}

	while (cnt > 0)
	{
		if (psrc[0] == 0)	   /* no more track format info ? */
		{
			if (w->dam_cnt < w->sec_per_track) /* but more DAM info ? */
			{
				if (w->density)/* DD track ? */
					psrc = track_DD[3];
				else
					psrc = track_SD[3];
			}
		}

		if (psrc[0] != 0)	   /* more track format info ? */
		{
			cnt -= psrc[0];	   /* subtract size */
			d = psrc[1];

			if (d == 0xf5)	   /* clear CRC ? */
			{
				crc = 0xffff;
				d = 0xa1;	   /* store A1 */
			}

			for (i = 0; i < *psrc; i++)
				*pdst++ = d;   /* fill data */

			if (d == 0xf7)	   /* store CRC ? */
			{
				pdst--;		   /* go back one byte */
				*pdst++ = crc & 255;	/* put CRC low */
				*pdst++ = crc / 256;	/* put CRC high */
				cnt -= 1;	   /* count one more byte */
			}
			else if (d == 0xfe)/* address mark ? */
			{
				crc = 0xffff;   /* reset CRC */
			}
			else if (d == 0x80)/* sector ID ? */
			{
				pdst--;		   /* go back one byte */
				t = *pdst++ = w->dam_list[w->dam_cnt][0]; /* track number */
				h = *pdst++ = w->dam_list[w->dam_cnt][1]; /* head number */
				s = *pdst++ = w->dam_list[w->dam_cnt][2]; /* sector number */
				l = *pdst++ = w->dam_list[w->dam_cnt][3]; /* sector length code */
				w->dam_cnt++;
				calc_crc(&crc, t);	/* build CRC */
				calc_crc(&crc, h);	/* build CRC */
				calc_crc(&crc, s);	/* build CRC */
				calc_crc(&crc, l);	/* build CRC */
				l = (l == 0) ? 128 : l << 8;
			}
			else if (d == 0xfb)// data address mark ?
			{
				crc = 0xffff;   // reset CRC
			}
			else if (d == 0x81)// sector DATA ?
			{
				pdst--;		   /* go back one byte */
				if (seek(w, t, h, s) == 0)
				{
					if (osd_fread(w->image_file, pdst, l) != l)
					{
						w->status = STA_2_CRC_ERR;
						return;
					}
				}
				else
				{
					w->status = STA_2_REC_N_FND;
					return;
				}
				for (i = 0; i < l; i++)	// build CRC of all data
					calc_crc(&crc, *pdst++);
				cnt -= l;
			}
			psrc += 2;
		}
		else
		{
			*pdst++ = 0xff;	   /* fill track */
			cnt--;			   /* until end */
		}
	}

	w->data_offset = 0;
	w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

	w->status_drq = STA_2_DRQ;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_SET);
	w->status |= STA_2_DRQ | STA_2_BUSY;
	w->busy_count = 0;
}

/* write a sector */
static void write_sector(WD179X * w)
{
	if (w->image_file == REAL_FDD)
	{
		w->status = osd_fdc_put_sector(w->track, w->head, w->head, w->sector, w->buffer, w->write_cmd & FDC_DELETED_AM);
		return;
	}
    if (w->mode == 0)
	{
		w->status = STA_2_WRITE_PRO;
	}
	else
	{
		w->status = seek(w, w->track, w->head, w->sector);
		if (w->status == 0)
		{
			if (osd_fwrite(w->image_file, w->buffer, w->data_offset) != w->data_offset)
				w->status = STA_2_LOST_DAT;
		}
	}
}

/* write an entire track by extracting the sectors */
static void write_track(WD179X * w)
{
UINT8 *f;
int cnt;

	w->dam_cnt = 0;
    if (w->image_file != REAL_FDD && w->mode == 0)
    {
#if VERBOSE
		if (errorlog) fprintf(errorlog, "WD179X write_track write protected image\n");
#endif
        w->status = STA_2_WRITE_PRO;
        return;
    }

	memset(w->dam_list, 0xff, sizeof(w->dam_list));
	memset(w->dam_data, 0x00, sizeof(w->dam_data));

	f = w->buffer;
#if VERBOSE
	if (errorlog) fprintf(errorlog, "WD179X write_track %s_LOW\n", (w->density) ? "MFM" : "FM" );
#endif
    cnt = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

	do
	{
		while ((--cnt > 0) && (*f != 0xfe))	/* start of DAM ?? */
			f++;

		if (cnt > 4)
		{
		int seclen;
			cnt -= 5;
			f++;			   /* skip FE */
			w->dam_list[w->dam_cnt][0] = *f++;	  /* copy track number */
			w->dam_list[w->dam_cnt][1] = *f++;	  /* copy head number */
			w->dam_list[w->dam_cnt][2] = *f++;	  /* copy sector number */
			w->dam_list[w->dam_cnt][3] = *f++;	  /* copy sector length */
			/* sector length in bytes */
			seclen = 128 << w->dam_list[w->dam_cnt][3];
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "WD179X write_track FE @%5d T:%02X H:%02X S:%02X L:%02X\n",
					(int)(f - w->buffer),
					w->dam_list[w->dam_cnt][0],w->dam_list[w->dam_cnt][1],
					w->dam_list[w->dam_cnt][2],w->dam_list[w->dam_cnt][3]);
#endif
			/* search start of DATA */
			while ((--cnt > 0) && (*f != 0xf9) && (*f != 0xfa) && (*f != 0xfb))
				f++;
			if (cnt > seclen)
			{
				cnt--;
				/* skip data address mark */
                f++;
                /* set pointer to DATA to later write the sectors contents */
				w->dam_data[w->dam_cnt] = (int)(f - w->buffer);
				w->dam_cnt++;
#if VERBOSE
				if (errorlog)
					fprintf(errorlog, "WD179X write_track %02X @%5d data: %02X %02X %02X %02X ... %02X %02X %02X %02X\n",
						f[-1],
						(int)(f - w->buffer),
						f[0], f[1], f[2], f[3],
						f[seclen-4], f[seclen-3], f[seclen-2], f[seclen-1]);
#endif
				f += seclen;
				cnt -= seclen;
			}
        }
	} while (cnt > 0);

	if (w->image_file == REAL_FDD)
	{
		w->status = osd_fdc_format(w->track, w->head, w->dam_cnt, w->dam_list[0]);
#if 0
        if ((w->status & 0xfc) == 0)
		{
			/* now put all sectors contained in the format buffer */
			for (cnt = 0; cnt < w->dam_cnt; cnt++)
			{
				w->status = osd_fdc_put_sector(w->track, w->head, cnt, w->buffer[dam_data[cnt]], 0);
				/* bail out if an error occured */
				if (w->status & 0xfc)
					break;
			}
        }
#endif
    }
	else
	{
		/* now put all sectors contained in the format buffer */
		for (cnt = 0; cnt < w->dam_cnt; cnt++)
		{
			w->status = seek(w, w->track, w->head, w->dam_list[cnt][2]);
			if (w->status == 0)
			{
				if (osd_fwrite(w->image_file, &w->buffer[w->dam_data[cnt]], w->sector_length) != w->sector_length)
				{
					w->status = STA_2_LOST_DAT;
					return;
				}
			}
		}
	}
}


/* read the FDC status register. This clears IRQ line too */
int wd179x_status_r(int offset)
{
WD179X *w = wd[drv];
int result = w->status;

	if (w->callback)
		(*w->callback) (WD179X_IRQ_CLR);
	if (w->busy_count)
	{
		if (!--w->busy_count)
			w->status &= ~STA_1_BUSY;
	}
/* eventually toggle index pulse bit */
	w->status ^= w->status_ipl;
/* eventually set data request bit */
	w->status |= w->status_drq;

#if VERBOSE
    if (errorlog)
		if (w->data_count < 4)
			fprintf(errorlog, "wd179x_status_r: $%02X (data_count %d)\n", result, w->data_count);
#endif
	return result;
}

/* read the FDC track register */
int wd179x_track_r(int offset)
{
WD179X *w = wd[drv];

#if VERBOSE
	if (errorlog)
		fprintf(errorlog, "wd179x_track_r: $%02X\n", w->track_reg);
#endif
	return w->track_reg;
}

/* read the FDC sector register */
int wd179x_sector_r(int offset)
{
WD179X *w = wd[drv];

#if VERBOSE
	if (errorlog)
		fprintf(errorlog, "wd179x_sector_r: %02X\n", w->sector);
#endif
	return w->sector;
}

/* read the FDC data register */
int wd179x_data_r(int offset)
{
WD179X *w = wd[drv];

	if (w->data_count > 0)
	{
		w->status &= ~STA_2_DRQ;
		if (--w->data_count <= 0)
		{
			/* clear busy bit */
			w->status &= ~STA_2_BUSY;
			/* no more setting of data request bit */
			w->status_drq = 0;
			if (w->callback)
				(*w->callback) (WD179X_DRQ_CLR);
			if (w->image_file != REAL_FDD)
			{
				/* read normal or deleted data address mark ? */
				w->status |= deleted_dam(w);
			}
			/* generate an IRQ */
			if (w->callback)
				(*w->callback) (WD179X_IRQ_SET);
		}
		w->data = w->buffer[w->data_offset++];
	}
#if VERBOSE
	else
	{
		if (errorlog)
			fprintf(errorlog, "wd179x_data_r: $%02X (data_count 0)\n", w->data);
    }
#endif
	return w->data;
}

/* write the FDC command register */
void wd179x_command_w(int offset, int data)
{
WD179X *w = wd[drv];

	if ((data | 1) == 0xff)	   /* change single/double density ? */
	{
		/* only supports FM/LO and MFM/LO */
		w->density = (data & 1) ? DEN_MFM_LO : DEN_FM_LO;
		if (w->image_file == REAL_FDD)
			osd_fdc_density(w->unit, w->density, w->tracks, w->sec_per_track, w->sec_per_track, 1);
		return;
	}

	if ((data & ~FDC_MASK_TYPE_IV) == FDC_FORCE_INT)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X FORCE_INT (data_count %d)\n", data, w->data_count);
#endif
		w->data_count = 0;
		w->data_offset = 0;
		w->status &= ~(STA_2_DRQ | STA_2_BUSY);
		w->status_drq = 0;
		if (w->callback)
			(*w->callback) (WD179X_DRQ_CLR);
//		w->status_ipl = STA_1_IPL;
		w->status_ipl = 0;
		if (w->callback)
			(*w->callback) (WD179X_IRQ_CLR);
		w->busy_count = 0;
		return;
	}

	if (data & 0x80)
	{
		w->status_ipl = 0;

		if ((data & ~FDC_MASK_TYPE_II) == FDC_READ_SEC)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "wd179x_command_w $%02X READ_SEC\n", data);
#endif
			w->read_cmd = data;
            w->command = data & ~FDC_MASK_TYPE_II;
			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_sector(w);
			return;
		}

		if ((data & ~FDC_MASK_TYPE_II) == FDC_WRITE_SEC)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "wd179x_command_w $%02X WRITE_SEC\n", data);
#endif
			w->write_cmd = data;
			w->command = data & ~FDC_MASK_TYPE_II;
			w->data_offset = 0;
			w->data_count = w->sector_length;
			w->status_drq = STA_2_DRQ;
			if (w->callback)
				(*w->callback) (WD179X_DRQ_SET);
			w->status = STA_2_DRQ | STA_2_BUSY;
			w->busy_count = 0;
			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_TRK)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "wd179x_command_w $%02X READ_TRK\n", data);
#endif
			w->command = data & ~FDC_MASK_TYPE_III;
			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_track(w);
			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_WRITE_TRK)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "wd179x_command_w $%02X WRITE_TRK\n", data);
#endif
			w->command = data & ~FDC_MASK_TYPE_III;
			w->data_offset = 0;
			w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;
			w->status_drq = STA_2_DRQ;
			if (w->callback)
				(*w->callback) (WD179X_DRQ_SET);
			w->status = STA_2_DRQ | STA_2_BUSY;
			w->busy_count = 0;
			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_DAM)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "wd179x_command_w $%02X READ_DAM\n", data);
#endif
			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_dam(w);
			return;
		}

#if VERBOSE
        if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X unknown\n", data);
#endif
        return;
	}


	if ((data & ~FDC_MASK_TYPE_I) == FDC_RESTORE)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X RESTORE\n", data);
#endif
		/* simulate seek time busy signal */
		w->busy_count = w->track * ((data & FDC_STEP_RATE) + 1);
		/* if it is a real floppy, issue a recal command */
        if (w->image_file == REAL_FDD)
		{
			osd_fdc_recal(&w->track);
		}
		else
		{
			w->track = 0;	 /* set track number 0 */
		}
		w->track_reg = w->track;
    }

	if ((data & ~FDC_MASK_TYPE_I) == FDC_SEEK)
	{
	UINT8 newtrack = w->data;
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X SEEK (data_reg is $%02X)\n", data, newtrack);
#endif
		/* if it is a real floppy, issue a seek command */
        /* simulate seek time busy signal */
		w->busy_count = abs(newtrack - w->track) * ((data & FDC_STEP_RATE) + 1);
        if (w->image_file == REAL_FDD)
			osd_fdc_seek(newtrack, &w->track);
		else
			w->track = newtrack;	/* get track number from data register */
		w->track_reg = w->track;
	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X STEP dir %+d\n", data, w->direction);
#endif
		/* if it is a real floppy, issue a step command */
        /* simulate seek time busy signal */
		w->busy_count = ((data & FDC_STEP_RATE) + 1);
		if (w->image_file == REAL_FDD)
            osd_fdc_step(w->direction, &w->track);
		else
			w->track += w->direction;	/* adjust track number */
	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_IN)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X STEP_IN\n", data);
#endif
        w->direction = +1;
		/* simulate seek time busy signal */
		w->busy_count = ((data & FDC_STEP_RATE) + 1);
		/* if it is a real floppy, issue a step command */
        if (w->image_file == REAL_FDD)
			osd_fdc_step(w->direction, &w->track);
		else
			w->track += w->direction;	/* adjust track number */
	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_OUT)
	{
#if VERBOSE
		if (errorlog)
			fprintf(errorlog, "wd179x_command_w $%02X STEP_OUT\n", data);
#endif
        w->direction = -1;
		/* simulate seek time busy signal */
		w->busy_count = ((data & FDC_STEP_RATE) + 1);
		/* if it is a real floppy, issue a step command */
        if (w->image_file == REAL_FDD)
			osd_fdc_step(w->direction, &w->track);
		else
			w->track += w->direction;	/* adjust track number */
	}

	if (w->busy_count)
		w->status = STA_1_BUSY;

/* toggle index pulse at read */
	w->status_ipl = STA_1_IPL;

	if (w->track >= w->tracks)
		w->status |= STA_1_SEEK_ERR;

	if (w->track == 0)
		w->status |= STA_1_TRACK0;

    if (w->mode == 0)
        w->status |= STA_1_WRITE_PRO;

    if (data & FDC_STEP_UPDATE)
		w->track_reg = w->track;

	if (data & FDC_STEP_HDLOAD)
		w->status |= STA_1_HD_LOADED;

	if (data & FDC_STEP_VERIFY)
		if (w->track_reg != w->track)
			w->status |= STA_1_SEEK_ERR;
}

/* write the FDC track register */
void wd179x_track_w(int offset, int data)
{
WD179X *w = wd[drv];
	w->track = w->track_reg = data;
#if VERBOSE
	if (errorlog)
		fprintf(errorlog, "wd179x_track_w $%02X\n", data);
#endif
}

/* write the FDC sector register */
void wd179x_sector_w(int offset, int data)
{
WD179X *w = wd[drv];

	w->sector = data;
#if VERBOSE
	if (errorlog)
		fprintf(errorlog, "wd179x_sector_w $%02X\n", data);
#endif
}

/* write the FDC data register */
void wd179x_data_w(int offset, int data)
{
WD179X *w = wd[drv];

	if (w->data_count > 0)
	{
		w->buffer[w->data_offset++] = data;
		if (--w->data_count <= 0)
		{
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "WD179X buffered %d byte\n", w->data_offset);
#endif
			w->status_drq = 0;
			if (w->callback)
				(*w->callback) (WD179X_DRQ_CLR);
			if (w->command == FDC_WRITE_TRK)
				write_track(w);
			else
				write_sector(w);
			w->data_offset = 0;
			if (w->callback)
				(*w->callback) (WD179X_IRQ_SET);
		}
	}
#if VERBOSE
	else
	{
		if (errorlog)
			fprintf(errorlog, "wd179x_data_w $%02X\n", data);
	}
#endif
	w->data = data;
}
