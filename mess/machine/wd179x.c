/***************************************************************************

  WD179X.c

  Functions to emulate a WD179x floppy disc controller

  KT - Removed disk image code and replaced it with floppy drive functions.
	   Any disc image is now useable with this code.
	 - fixed write protect


 TODO:
	 - Multiple record read/write
	 - What happens if a track is read that doesn't have any id's on it?
	   (e.g. unformatted disc)
***************************************************************************/

#include "driver.h"
#include "includes/wd179x.h"
#include "devices/flopdrv.h"

#define VERBOSE 0
#define VERBOSE_DATA 0		/* This turns on and off the recording of each byte during read and write */

/* structure describing a double density track */
#define TRKSIZE_DD		6144
#if 0
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
#endif
/* structure describing a single density track */
#define TRKSIZE_SD		3172
#if 0
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
#endif

static void wd179x_complete_command(WD179X *, int);
static void wd179x_clear_data_request(void);
static void wd179x_set_data_request(void);
static void wd179x_timed_data_request(void);
static void wd179x_set_irq(WD179X *);
static void *busy_timer = NULL;

/* one wd controlling multiple drives */
static WD179X wd;

/* this is the drive currently selected */
static UINT8 current_drive;

/* this is the head currently selected */
static UINT8 hd = 0;

static mess_image *wd179x_current_image(void)
{
	return image_instance(IO_FLOPPY, current_drive);
}

/* use this to determine which drive is controlled by WD */
void wd179x_set_drive(UINT8 drive)
{
	current_drive = drive;
}

void wd179x_set_side(UINT8 head)
{
#if VERBOSE
	if( head != hd )
	logerror("wd179x_set_side: $%02x\n", head);
#endif

	hd = head;
}

void wd179x_set_density(DENSITY density)
{
	WD179X *w = &wd;

#if VERBOSE
	if( w->density != density )
		logerror("wd179x_set_density: $%02x\n", density);
#endif

	w->density = density;
}


static void	wd179x_busy_callback(int dummy)
{
	WD179X *w = (WD179X *)dummy;

	wd179x_set_irq(w);			
	timer_reset(busy_timer, TIME_NEVER);
}

static void wd179x_set_busy(WD179X *w, double milliseconds)
{
	w->status |= STA_1_BUSY;
	timer_adjust(busy_timer, TIME_IN_MSEC(milliseconds), (int)w, 0);
}



/* BUSY COUNT DOESN'T WORK PROPERLY! */

static void wd179x_restore(WD179X *w)
{
	UINT8 step_counter = 255;
		
#if 0
	w->status |= STA_1_BUSY;
#endif

	/* setup step direction */
	w->direction = -1;

	w->command_type = TYPE_I;

	/* reset busy count */
	w->busy_count = 0;

	/* keep stepping until track 0 is received or 255 steps have been done */
	while (!(floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_HEAD_AT_TRACK_0)) && (step_counter!=0))
	{
		/* update time to simulate seek time busy signal */
		w->busy_count++;
		floppy_drive_seek(wd179x_current_image(), w->direction);
		step_counter--;
	}

	/* update track reg */
	w->track_reg = 0;
#if 0
	/* simulate seek time busy signal */
	w->busy_count = 0;	//w->busy_count * ((w->data & FDC_STEP_RATE) + 1);
	
	/* when command completes set irq */
	wd179x_set_irq(w);
#endif
	wd179x_set_busy(w,0.1);
}

void	wd179x_reset(void)
{
	wd179x_restore(&wd);
}

static void	wd179x_busy_callback(int dummy);
static void	wd179x_misc_timer_callback(int code);
static void	wd179x_read_sector_callback(int code);
static void	wd179x_write_sector_callback(int code);

void wd179x_init(int type,void (*callback)(int))
{
	memset(&wd, 0, sizeof(WD179X));
	wd.status = STA_1_TRACK0;
	wd.type = type;
	wd.callback = callback;
//	wd.status_ipl = STA_1_IPL;
	wd.density = DEN_MFM_LO;
	busy_timer = timer_alloc(wd179x_busy_callback);
	wd.timer = timer_alloc(wd179x_misc_timer_callback);
	wd.timer_rs = timer_alloc(wd179x_read_sector_callback);
	wd.timer_ws = timer_alloc(wd179x_write_sector_callback);

	wd179x_reset();

#if 0

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
		wd[i]->first_sector_id = 0;
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
		wd[i]->timer = NULL;
		wd[i]->timer_rs = NULL;
		wd[i]->timer_ws = NULL;
	}
#endif
}

static void write_track(WD179X * w)
{



}

/* read an entire track */
static void read_track(WD179X * w)
{
#if 0
	UINT8 *psrc;		/* pointer to track format structure */
	UINT8 *pdst;		/* pointer to track buffer */
	int cnt;			/* number of bytes to fill in */
	UINT16 crc; 		/* id or data CRC */
	UINT8 d;			/* data */
	UINT8 t = w->track; /* track of DAM */
	UINT8 h = w->head;	/* head of DAM */
	UINT8 s = w->sector_dam;		/* sector of DAM */
	UINT16 l = w->sector_length;	/* sector length of DAM */
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
		psrc = track_DD[0];    /* double density track format */
		cnt = TRKSIZE_DD;
	}
	else
	{
		psrc = track_SD[0];    /* single density track format */
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
			cnt -= psrc[0];    /* subtract size */
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
				pdst--; 	   /* go back one byte */
				*pdst++ = crc & 255;	/* put CRC low */
				*pdst++ = crc / 256;	/* put CRC high */
				cnt -= 1;	   /* count one more byte */
			}
			else if (d == 0xfe)/* address mark ? */
			{
				crc = 0xffff;	/* reset CRC */
			}
			else if (d == 0x80)/* sector ID ? */
			{
				pdst--; 	   /* go back one byte */
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
				crc = 0xffff;	// reset CRC
			}
			else if (d == 0x81)// sector DATA ?
			{
				pdst--; 	   /* go back one byte */
				if (seek(w, t, h, s) == 0)
				{
					if (mame_fread(w->image_file, pdst, l) != l)
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
				for (i = 0; i < l; i++) // build CRC of all data
					calc_crc(&crc, *pdst++);
				cnt -= l;
			}
			psrc += 2;
		}
		else
		{
			*pdst++ = 0xff;    /* fill track */
			cnt--;			   /* until end */
		}
	}
#endif

	w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

	floppy_drive_read_track_data_info_buffer( wd179x_current_image(), hd, (char *)w->buffer, &(w->data_count) );
	
	w->data_offset = 0;

	wd179x_set_data_request();
	w->status |= STA_2_BUSY;
	w->busy_count = 0;
}

#if 0
void wd179x_stop_drive(void)
{
	WD179X *w = &wd;

	w->busy_count = 0;
	w->status = 0;
	w->status_drq = 0;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_CLR);
	w->status_ipl = 0;
}
#endif

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
static void wd179x_read_id(WD179X * w)
{
	chrn_id id;

	w->status &= ~(STA_2_CRC_ERR | STA_2_REC_N_FND);

	/* get next id from disc */
	if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
	{
		UINT16 crc = 0xffff;

		w->data_offset = 0;
		w->data_count = 6;

		/* for MFM */
		/* crc includes 3x0x0a1, and 1x0x0fe (id mark) */
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0a1);
		calc_crc(&crc,0x0fe);

		w->buffer[0] = id.C;
		w->buffer[1] = id.H;
		w->buffer[2] = id.R;
		w->buffer[3] = id.N;
		calc_crc(&crc, w->buffer[0]);
		calc_crc(&crc, w->buffer[1]);
		calc_crc(&crc, w->buffer[2]);
		calc_crc(&crc, w->buffer[3]);
		/* crc is stored hi-byte followed by lo-byte */
		w->buffer[4] = crc>>8;
		w->buffer[5] = crc & 255;
		

		w->sector = id.C;
		w->status |= STA_2_BUSY;
		w->busy_count = 50;

		wd179x_set_data_request();
		logerror("read id succeeded.\n");
	}
	else
	{
		/* record not found */
		w->status |= STA_2_REC_N_FND;
		//w->sector = w->track_reg;
		logerror("read id failed\n");

		wd179x_complete_command(w, 1);
	}
}


static int wd179x_find_sector(WD179X *w)
{
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	w->status &= ~STA_2_REC_N_FND;

	while (revolution_count!=4)
	{
		if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
		{
			/* compare track */
			if (id.C == w->track_reg)
			{
				/* compare id */
				if (id.R == w->sector)
				{
					w->sector_length = 1<<(id.N+7);
					w->sector_data_id = id.data_id;
					/* get ddam status */
					w->ddam = id.flags & ID_FLAG_DELETED_DATA;
					/* got record type here */
#if VERBOSE
	logerror("sector found! C:$%02x H:$%02x R:$%02x N:$%02x%s\n", id.C, id.H, id.R, id.N, w->ddam ? " DDAM" : "");
#endif
					return 1;
				}
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

	/* record not found */
	w->status |= STA_2_REC_N_FND;

#if VERBOSE
	logerror("track %d sector %d not found!\n", w->track_reg, w->sector);
#endif
	wd179x_complete_command(w, 1);

	return 0;
}

/* read a sector */
static void wd179x_read_sector(WD179X *w)
{
	w->data_offset = 0;

	if (wd179x_find_sector(w))
	{
		w->data_count = w->sector_length;

		/* read data */
		floppy_drive_read_sector_data(wd179x_current_image(), hd, w->sector_data_id, (char *)w->buffer, w->sector_length);

		wd179x_timed_data_request();

		w->status |= STA_2_BUSY;
		w->busy_count = 0;
	}
}

static void	wd179x_set_irq(WD179X *w)
{
	w->status &= ~STA_2_BUSY;
	/* generate an IRQ */
	if (w->callback)
		(*w->callback) (WD179X_IRQ_SET);
}

/* 0=command callback; 1=data callback */
enum
{
	MISCCALLBACK_COMMAND,
	MISCCALLBACK_DATA
};

static void wd179x_misc_timer_callback(int callback_type)
{
	WD179X *w = &wd;

	switch(callback_type) {
	case MISCCALLBACK_COMMAND:
		/* command callback */
		wd179x_set_irq(w);
		break;

	case MISCCALLBACK_DATA:
		/* data callback */
		/* ok, trigger data request now */
		wd179x_set_data_request();
		break;
	}

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer, TIME_NEVER); 
}

/* called on error, or when command is actually completed */
/* KT - I have used a timer for systems that use interrupt driven transfers.
A interrupt occurs after the last byte has been read. If it occurs at the time
when the last byte has been read it causes problems - same byte read again
or bytes missed */
/* TJL - I have add a parameter to allow the emulation to specify the delay
*/
static void wd179x_complete_command(WD179X *w, int delay)
{
	int usecs;

	w->data_count = 0;

	/* clear busy bit */
	w->status &= ~STA_2_BUSY;

	usecs = floppy_drive_get_datarate_in_us(w->density);
	usecs *= delay;

	/* set new timer */
	timer_adjust(w->timer, TIME_IN_USEC(usecs), MISCCALLBACK_COMMAND, 0);
}

static void wd179x_write_sector(WD179X *w)
{
	/* at this point, the disc is write enabled, and data
	 * has been transfered into our buffer - now write it to
	 * the disc image or to the real disc
	 */

	/* find sector */
	if (wd179x_find_sector(w))
	{
		w->data_count = w->sector_length;

		/* write data */
		floppy_drive_write_sector_data(wd179x_current_image(), hd, w->sector_data_id, (char *)w->buffer, w->sector_length,w->write_cmd & 0x01);
	}
}


/* verify the seek operation by looking for a id that has a matching track value */
static void wd179x_verify_seek(WD179X *w)
{
	UINT8 revolution_count;
	chrn_id id;

	revolution_count = 0;

	logerror("doing seek verify\n");

	w->status &= ~STA_1_SEEK_ERR;

	/* must be found within 5 revolutions otherwise error */
	while (revolution_count!=5)
	{
		if (floppy_drive_get_next_id(wd179x_current_image(), hd, &id))
		{
			/* compare track */
			if (id.C == w->track_reg)
			{
				logerror("seek verify succeeded!\n");
				return;
			}
		}

		 /* index set? */
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_INDEX))
		{
			/* update revolution count */
			revolution_count++;
		}
	}

	w->status |= STA_1_SEEK_ERR;

	logerror("failed seek verify!");
}


/* clear a data request */
static void wd179x_clear_data_request(void)
{
	WD179X *w = &wd;

//	w->status_drq = 0;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_CLR);
	w->status &= ~STA_2_DRQ;
}

/* set data request */
static void wd179x_set_data_request(void)
{
	WD179X *w = &wd;

	if (w->status & STA_2_DRQ)
	{
		w->status |= STA_2_LOST_DAT;
//		return;
	}

	/* set drq */
//	w->status_drq = STA_2_DRQ;
	if (w->callback)
		(*w->callback) (WD179X_DRQ_SET);
	w->status |= STA_2_DRQ;
}

/* callback to initiate read sector */
static void	wd179x_read_sector_callback(int code)
{
	WD179X *w = &wd;

	/* ok, start that read! */

#if VERBOSE
	logerror("wd179x: Read Sector callback.\n");
#endif

	if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
		wd179x_complete_command(w, 1);
	else
		wd179x_read_sector(w);

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer_rs, TIME_NEVER); 
}

/* callback to initiate write sector */

static void	wd179x_write_sector_callback(int code)
{
	WD179X *w = &wd;

	/* ok, start that write! */

#if VERBOSE
	logerror("wd179x: Write Sector callback.\n");
#endif

	if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
		wd179x_complete_command(w, 1);
	else
	{

		/* drive write protected? */
		if (floppy_drive_get_flag_state(wd179x_current_image(),FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
		{
			w->status |= STA_2_WRITE_PRO;

			wd179x_complete_command(w, 1);
		}
		else
		{
			/* attempt to find it first before getting data from cpu */
			if (wd179x_find_sector(w))
			{
				/* request data */
				w->data_offset = 0;
				w->data_count = w->sector_length;

				wd179x_set_data_request();

				w->status |= STA_2_BUSY;
				w->busy_count = 0;
			}
		}
	}

	/* stop it, but don't allow it to be free'd */
	timer_reset(w->timer_ws, TIME_NEVER); 
}

/* setup a timed data request - data request will be triggered in a few usecs time */
static void wd179x_timed_data_request(void)
{
	int usecs;
	WD179X *w = &wd;

	usecs = floppy_drive_get_datarate_in_us(w->density);

	/* set new timer */
	timer_adjust(w->timer, TIME_IN_USEC(usecs), MISCCALLBACK_DATA, 0);
}

/* setup a timed read sector - read sector will be triggered in a few usecs time */
static void wd179x_timed_read_sector_request(void)
{
	int usecs;
	WD179X *w = &wd;

	usecs = 20; /* How long should we wait? How about 20 micro seconds? */

	/* set new timer */
	timer_reset(w->timer_rs, TIME_IN_USEC(usecs));
}

/* setup a timed write sector - write sector will be triggered in a few usecs time */
static void wd179x_timed_write_sector_request(void)
{
	int usecs;
	WD179X *w = &wd;

	usecs = 20; /* How long should we wait? How about 20 micro seconds? */

	/* set new timer */
	timer_reset(w->timer_ws, TIME_IN_USEC(usecs));
}


/* read the FDC status register. This clears IRQ line too */
READ_HANDLER ( wd179x_status_r )
{
	WD179X *w = &wd;
	int result = w->status;

	if (w->callback)
		(*w->callback) (WD179X_IRQ_CLR);
//	if (w->busy_count)
//	{
//		if (!--w->busy_count)
//			w->status &= ~STA_1_BUSY;
//	}

	/* type 1 command or force int command? */
	if ((w->command_type==TYPE_I) || (w->command_type==TYPE_IV))
	{

		/* if disc present toggle index pulse */
		if (image_exists(wd179x_current_image()))
		{
			/* eventually toggle index pulse bit */
			w->status ^= STA_1_IPL;
		}

		/* set track 0 state */
		result &=~STA_1_TRACK0;
		if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_HEAD_AT_TRACK_0))
			result |= STA_1_TRACK0;

	//	floppy_drive_set_ready_state(wd179x_current_image(), 1,1);
		w->status &= ~STA_1_NOT_READY;
		
		if (w->type == WD_TYPE_179X)
		{
			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
				w->status |= STA_1_NOT_READY;
		}
		else
		{
			if (floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
				w->status |= STA_1_NOT_READY;
		}
	}
	
	/* eventually set data request bit */
//	w->status |= w->status_drq;

#if VERBOSE
	if (w->data_count < 4)
		logerror("wd179x_status_r: $%02X (data_count %d)\n", result, w->data_count);
#endif


	return result;
}

/* read the FDC track register */
READ_HANDLER ( wd179x_track_r )
{
	WD179X *w = &wd;

#if VERBOSE
	logerror("wd179x_track_r: $%02X\n", w->track_reg);
#endif
	return w->track_reg;
}

/* read the FDC sector register */
READ_HANDLER ( wd179x_sector_r )
{
	WD179X *w = &wd;

#if VERBOSE
	logerror("wd179x_sector_r: $%02X\n", w->sector);
#endif
	return w->sector;
}

/* read the FDC data register */
READ_HANDLER ( wd179x_data_r )
{
	WD179X *w = &wd;

	if (w->data_count >= 1)
	{
		/* clear data request */
		wd179x_clear_data_request();

		/* yes */
		w->data = w->buffer[w->data_offset++];

#if VERBOSE_DATA
		logerror("wd179x_data_r: $%02X (data_count %d)\n", w->data, w->data_count);
#endif
		/* any bytes remaining? */
		if (--w->data_count < 1)
		{
			/* no */
			w->data_offset = 0;

			/* clear ddam type */
			w->status &=~STA_2_REC_TYPE;
			/* read a sector with ddam set? */
			if (w->command_type == TYPE_II && w->ddam != 0)
			{
				/* set it */
				w->status |= STA_2_REC_TYPE;
			}

			/* not incremented after each sector - only incremented in multi-sector
			operation. If this remained as it was oric software would not run! */
		//	w->sector++;
			/* Delay the INTRQ 3 byte times becuase we need to read two CRC bytes and
			   compare them with a calculated CRC */
			wd179x_complete_command(w, 3);
		}
		else
		{
			/* issue a timed data request */
			wd179x_timed_data_request();		
		}
	}
	else
	{
		logerror("wd179x_data_r: (no new data) $%02X (data_count %d)\n", w->data, w->data_count);
	}
	return w->data;
}

/* write the FDC command register */
WRITE_HANDLER ( wd179x_command_w )
{
	WD179X *w = &wd;

	floppy_drive_set_motor_state(wd179x_current_image(), 1);
	floppy_drive_set_ready_state(wd179x_current_image(), 1,0);
	/* also cleared by writing command */
	if (w->callback)
			(*w->callback) (WD179X_IRQ_CLR);

	/* clear write protected. On read sector, read track and read dam, write protected bit is clear */
	w->status &= ~((1<<6) | (1<<5) | (1<<4));

	if ((data & ~FDC_MASK_TYPE_IV) == FDC_FORCE_INT)
	{
#if VERBOSE
		logerror("wd179x_command_w $%02X FORCE_INT (data_count %d)\n", data, w->data_count);
#endif
		w->data_count = 0;
		w->data_offset = 0;
		w->status &= ~(STA_2_BUSY);
		
		wd179x_clear_data_request();

		if (data & 0x0f)
		{



		}


//		w->status_ipl = STA_1_IPL;
/*		w->status_ipl = 0; */
		
		w->busy_count = 0;
		w->command_type = TYPE_IV;
        return;
	}

	if (data & 0x80)
	{
		/*w->status_ipl = 0;*/

		if ((data & ~FDC_MASK_TYPE_II) == FDC_READ_SEC)
		{
#if VERBOSE
			logerror("wd179x_command_w $%02X READ_SEC\n", data);
#endif
			w->read_cmd = data;
			w->command = data & ~FDC_MASK_TYPE_II;
			w->command_type = TYPE_II;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			wd179x_timed_read_sector_request();

			return;
		}

		if ((data & ~FDC_MASK_TYPE_II) == FDC_WRITE_SEC)
		{
#if VERBOSE
			logerror("wd179x_command_w $%02X WRITE_SEC\n", data);
#endif
			w->write_cmd = data;
			w->command = data & ~FDC_MASK_TYPE_II;
			w->command_type = TYPE_II;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			wd179x_timed_write_sector_request();

			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_TRK)
		{
#if VERBOSE
			logerror("wd179x_command_w $%02X READ_TRK\n", data);
#endif
			w->command = data & ~FDC_MASK_TYPE_III;
			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();
#if 1
//			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_track(w);
#endif
			return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_WRITE_TRK)
		{
#if VERBOSE
			logerror("wd179x_command_w $%02X WRITE_TRK\n", data);
#endif
			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
			wd179x_clear_data_request();

			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
            {
				wd179x_complete_command(w, 1);
            }
            else
            {
    
                /* drive write protected? */
                if (floppy_drive_get_flag_state(wd179x_current_image(),FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
                {
                    /* yes */
                    w->status |= STA_2_WRITE_PRO;
                    /* quit command */
                    wd179x_complete_command(w, 1);
                }
                else
                {
                    w->command = data & ~FDC_MASK_TYPE_III;
                    w->data_offset = 0;
                    w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;
                    wd179x_set_data_request();

                    w->status |= STA_2_BUSY;
                    w->busy_count = 0;
                }
            }
            return;
		}

		if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_DAM)
		{
#if VERBOSE
			logerror("wd179x_command_w $%02X READ_DAM\n", data);
#endif
			w->command_type = TYPE_III;
			w->status &= ~STA_2_LOST_DAT;
  			wd179x_clear_data_request();

			if (!floppy_drive_get_flag_state(wd179x_current_image(), FLOPPY_DRIVE_READY))
            {
				wd179x_complete_command(w, 1);
            }
            else
            {
                wd179x_read_id(w);
            }
			return;
		}

#if VERBOSE
		logerror("wd179x_command_w $%02X unknown\n", data);
#endif
		return;
	}

	w->status |= STA_1_BUSY;
	
	/* clear CRC error */
	w->status &=~STA_1_CRC_ERR;

	if ((data & ~FDC_MASK_TYPE_I) == FDC_RESTORE)
	{

#if VERBOSE
		logerror("wd179x_command_w $%02X RESTORE\n", data);
#endif
		wd179x_restore(w);
	}

	if ((data & ~FDC_MASK_TYPE_I) == FDC_SEEK)
	{
		UINT8 newtrack;

		logerror("old track: $%02x new track: $%02x\n", w->track_reg, w->data);
		w->command_type = TYPE_I;

		/* setup step direction */
		if (w->track_reg < w->data)
		{
			#if VERBOSE
			logerror("direction: +1\n");
			#endif
			w->direction = 1;
		}
		else
		if (w->track_reg > w->data)
        {
        	#if VERBOSE
			logerror("direction: -1\n");
			#endif
			w->direction = -1;
		}

		newtrack = w->data;
#if VERBOSE
		logerror("wd179x_command_w $%02X SEEK (data_reg is $%02X)\n", data, newtrack);
#endif

		/* reset busy count */
		w->busy_count = 0;

		/* keep stepping until reached track programmed */
		while (w->track_reg != newtrack)
		{
			/* update time to simulate seek time busy signal */
			w->busy_count++;

			/* update track reg */
			w->track_reg += w->direction;

			floppy_drive_seek(wd179x_current_image(), w->direction);
		}

		/* simulate seek time busy signal */
		w->busy_count = 0;	//w->busy_count * ((data & FDC_STEP_RATE) + 1);
#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);

	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP)
	{
#if VERBOSE
		logerror("wd179x_command_w $%02X STEP dir %+d\n", data, w->direction);
#endif
		w->command_type = TYPE_I;
        /* if it is a real floppy, issue a step command */
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;

#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);


	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_IN)
	{
#if VERBOSE
		logerror("wd179x_command_w $%02X STEP_IN\n", data);
#endif
		w->command_type = TYPE_I;
        w->direction = +1;
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;
#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);

	}

	if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_OUT)
	{
#if VERBOSE
		logerror("wd179x_command_w $%02X STEP_OUT\n", data);
#endif
		w->command_type = TYPE_I;
        w->direction = -1;
		/* simulate seek time busy signal */
		w->busy_count = 0;	//((data & FDC_STEP_RATE) + 1);

		/* for now only allows a single drive to be selected */
		floppy_drive_seek(wd179x_current_image(), w->direction);

		if (data & FDC_STEP_UPDATE)
			w->track_reg += w->direction;

#if 0
		wd179x_set_irq(w);
#endif
		wd179x_set_busy(w,0.1);
	}

//	if (w->busy_count==0)
//		w->status &= ~STA_1_BUSY;

//	/* toggle index pulse at read */
//	w->status_ipl = STA_1_IPL;

	/* 0 is enable spin up sequence, 1 is disable spin up sequence */
	if ((data & FDC_STEP_HDLOAD)==0)
		w->status |= STA_1_HD_LOADED;

	if (data & FDC_STEP_VERIFY)
	{
		/* verify seek */
		wd179x_verify_seek(w);
	}
}

/* write the FDC track register */
WRITE_HANDLER ( wd179x_track_w )
{
	WD179X *w = &wd;
	w->track_reg = data;

#if VERBOSE
	logerror("wd179x_track_w $%02X\n", data);
#endif
}

/* write the FDC sector register */
WRITE_HANDLER ( wd179x_sector_w )
{
	WD179X *w = &wd;
	w->sector = data;
#if VERBOSE
	logerror("wd179x_sector_w $%02X\n", data);
#endif
}

/* write the FDC data register */
WRITE_HANDLER ( wd179x_data_w )
{
	WD179X *w = &wd;

	if (w->data_count > 0)
	{
		/* clear data request */
		wd179x_clear_data_request();

		/* put byte into buffer */
#if VERBOSE_DATA
		logerror("WD179X buffered data: $%02X at offset %d.\n", data, w->data_offset);
#endif
	
		w->buffer[w->data_offset++] = data;
		
		if (--w->data_count < 1)
		{
			w->data_offset = 0;

			if (w->command == FDC_WRITE_TRK)
				write_track(w);
			else
				wd179x_write_sector(w);

			wd179x_complete_command(w, 3);
		}
		else
		{
			/* yes... setup a timed data request */
			wd179x_timed_data_request();
		}

	}
#if VERBOSE
	else
	{
		logerror("wd179x_data_w $%02X\n", data);
	}
#endif
	w->data = data;
}


