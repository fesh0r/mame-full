/* 
	This code handles the floppy drives.
	All FDD actions should be performed using these functions.
	
	If a real drive is selected, the operations will be performed using osd_fdc functions,
	otherwise the functions are emulated and a disk image is used.

	Possible problems:
	1. Inconsistancies when switching between real and emulated drive.


  Disk image operation:
  - set disk image functions using floppy_drive_set_disk_image_interface

  Real disk operation:
  - set unit id

  TODO:
	- Disk change handling.
	- Override write protect if disk image has been opened in read mode
*/

#include "driver.h"
#include "includes/flopdrv.h"

#define MAX_DRIVES 4

static struct floppy_drive	drives[MAX_DRIVES];

/* this is called once in init_devices */
/* initialise all floppy drives */
/* and initialise real disc access */
void	floppy_drives_init(void)
{
	int i;

	/* if no floppies, no point setting this up */
	if (device_count(IO_FLOPPY)==0)
		return;

	/* initialise osd fdc hardware */
	osd_fdc_init();

	/* ensure first drive is present, all other drives are marked
	as not present - override in driver if more are to be made available */
	for (i=0; i<MAX_DRIVES; i++)
	{
		struct floppy_drive *pDrive = &drives[i];

		/* initialise flags */
		pDrive->flags = FLOPPY_DRIVE_HEAD_AT_TRACK_0;
		pDrive->index_pulse_callback = NULL;
		pDrive->index_timer = NULL;

		if (i==0)
		{
			/* set first drive connected */
			floppy_drive_set_flag_state(i, FLOPPY_DRIVE_CONNECTED, 1);
		}
		else
		{
			/* all remaining drives are not connected - can be overriden in driver */
			floppy_drive_set_flag_state(i, FLOPPY_DRIVE_CONNECTED, 0);
		}

		/* all drives are double-sided 80 track - can be overriden in driver! */
		floppy_drive_set_geometry(i, FLOPPY_DRIVE_DS_80);

		pDrive->fdd_unit = i;

		/* initialise id index - not so important */
		pDrive->id_index = 0;
		/* initialise track */
		pDrive->current_track = 0;
	}
}

void	floppy_drives_exit(void)
{
	int i;

	/* if no floppies, no point cleaning up*/
	if (device_count(IO_FLOPPY)==0)
		return;

	for (i=0; i<MAX_DRIVES; i++)
	{
		struct floppy_drive *pDrive;
	
		pDrive = &drives[i];
		
		/* remove timer for index pulse */
		if (pDrive->index_timer)
		{
			timer_remove(pDrive->index_timer);
			pDrive->index_timer = NULL;
		}
	}

	osd_fdc_exit();
}

/* this callback is executed every 300 times a second to emulate the index
pulse. What is the length of the index pulse?? */
static void	floppy_drive_index_callback(int id)
{
	/* check it's in range */
	if ((id>=0) && (id<MAX_DRIVES))
	{
		struct floppy_drive *pDrive;
	
		pDrive = &drives[id];

		if (pDrive->index_pulse_callback)
			pDrive->index_pulse_callback(id);
	}
}

/* set the callback for the index pulse */
void	floppy_drive_set_index_pulse_callback(int id, void (*callback)(int id))
{
	struct floppy_drive *pDrive;
	
	/* check it's in range */
	if ((id<0) || (id>=MAX_DRIVES))
		return;

	pDrive = &drives[id];

	pDrive->index_pulse_callback = callback;
}

/*************************************************************************/
/* IO_FLOPPY device functions */

/* return and set current status 
  use for setting:-
  1) write protect/enable
  2) real fdd/disk image
  3) drive present/missing
*/

int	floppy_status(int id, int new_status)
{
	struct floppy_drive *pDrive;
	
	/* check it's in range */
	if ((id<0) || (id>=MAX_DRIVES))
		return 0;

	pDrive = &drives[id];

	/* return current status only? */
	if (new_status!=-1)
	{
		/* we don't set the flags directly.
		The flags are "cooked" when we do a floppy_drive_get_flag_state depending on
		if drive is connected etc. So if we wrote the flags back it would
		corrupt this information. Therefore we update the flags depending on new_status */

		floppy_drive_set_flag_state(id, FLOPPY_DRIVE_CONNECTED, (new_status & FLOPPY_DRIVE_CONNECTED));
		floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_WRITE_PROTECTED, (new_status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED));
		floppy_drive_set_flag_state(id, FLOPPY_DRIVE_REAL_FDD, (new_status & FLOPPY_DRIVE_REAL_FDD));
	}

	/* return current status */
	return floppy_drive_get_flag_state(id,0x0ff);
}

void	floppy_drive_set_real_fdd_unit(int id, UINT8 unit_id)
{
	if ((id<0) || (id>=MAX_DRIVES))
		return;

	drives[id].fdd_unit = unit_id;
}

/* set interface for image interface */
void	floppy_drive_set_disk_image_interface(int id, floppy_interface *iface)
{
	if ((id<0) || (id>=MAX_DRIVES))
		return;

	if (iface==NULL)
		return;

	memcpy(&drives[id].interface, iface, sizeof(floppy_interface));
}

/* set flag state */
void	floppy_drive_set_flag_state(int id, int flag, int state)
{
	if ((id<0) || (id>=MAX_DRIVES))
		return;

	drives[id].flags &= ~flag;

	if (state)
	{
		drives[id].flags |= flag;
	}
}

void	floppy_drive_set_motor_state(int drive, int state)
{
	int new_motor_state = 0;
	int previous_state = 0;

	/* previous state */
	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_MOTOR_ON))
	{
		previous_state = 1;
	}


	/* calc new state */

	/* drive present? */
	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_CONNECTED))
	{
		/* disk inserted? */
		if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_DISK_INSERTED))
		{
			/* drive present and disc inserted */
	
			/* state of motor is same as the programmed state */
			if (state)
			{
				new_motor_state = 1;
			}
		}
	}

	if ((new_motor_state^previous_state)!=0)
	{
		/* if timer already setup remove it */
		if ((drive>=0) && (drive<MAX_DRIVES))
		{
			struct floppy_drive *pDrive;
	
			pDrive = &drives[drive];

			if (pDrive->index_timer!=NULL)
			{
				timer_remove(pDrive->index_timer);
				pDrive->index_timer = NULL;
			}

			if (new_motor_state)
			{
				/* off->on */
				/* check it's in range */

				/* setup timer to trigger at 300 times a second = 300rpm */
				pDrive->index_timer = timer_pulse(TIME_IN_HZ(300), drive, floppy_drive_index_callback);
			}
			else
			{
				/* on->off */
			}
		}
	}


	floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_MOTOR_ON, new_motor_state);

//	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_REAL_FDD))
//	{
//		osd_fdc_motors(drive,state);
//	}


}

/* for pc, drive is always ready, for amstrad,pcw,spectrum it is only ready under
a fixed set of circumstances */
/* use this to set ready state of drive */
void	floppy_drive_set_ready_state(int drive, int state, int flag)
{
	if (flag)
	{
		/* set ready only if drive is present, disk is in the drive,
		and disk motor is on - for Amstrad, Spectrum and PCW*/
	
		/* drive present? */
		if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_CONNECTED))
		{
			/* disk inserted? */
			if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_DISK_INSERTED))
			{
				if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_MOTOR_ON))
				{
					/* set state */
					floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_READY, state);
                    return;
				}
			}
		}
		floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_READY, 0);
	}
	else
	{
		/* force ready state - for PC driver */
		floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_READY, state);
	}

}


/* get flag state */
int		floppy_drive_get_flag_state(int id, int flag)
{       
	int drive_flags;
	int flags;

	flags = 0;

	/* check it is within range */
	if ((id<0) || (id>=MAX_DRIVES))
		return flags;

	drive_flags = drives[id].flags;

	/* these flags are independant of a real drive/disk image */
    flags |= drive_flags & (FLOPPY_DRIVE_CONNECTED | FLOPPY_DRIVE_REAL_FDD | FLOPPY_DRIVE_READY | FLOPPY_DRIVE_MOTOR_ON | FLOPPY_DRIVE_INDEX);

	/* real drive? */
	if (drive_flags & FLOPPY_DRIVE_REAL_FDD)
	{
		/* real drive */
        if (flag & (
            FLOPPY_DRIVE_HEAD_AT_TRACK_0 |
            FLOPPY_DRIVE_DISK_WRITE_PROTECTED |
            FLOPPY_DRIVE_DISK_INSERTED))
        {
    
            /* this assumes that the real fdd has a single read/write head. When the head
            is moved, both sides are moved at the same time */
            int fdd_status;
    
    /*      logerror("real fdd status\r\n"); */
    
            /* get state from real drive */
            fdd_status = osd_fdc_get_status(drives[id].fdd_unit);
    
      //      if (flag & FLOPPY_DRIVE_HEAD_AT_TRACK_0)
          //  {
                flags |= fdd_status & FLOPPY_DRIVE_HEAD_AT_TRACK_0;
          //  }
    
        //    if (flag & FLOPPY_DRIVE_DISK_WRITE_PROTECTED)
          //  {
                flags |= fdd_status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
          //  }
             
          //  if (flag & FLOPPY_DRIVE_DISK_INSERTED)
          //  {
                flags |= fdd_status & FLOPPY_DRIVE_DISK_INSERTED;
          //  }
        }
    }
	else
	{

		/* emulated disk drive */
/*		logerror("emulated fdd status\r\n"); */

		/* emulated fdd drive head at track 0? */
	//	if (flag & FLOPPY_DRIVE_HEAD_AT_TRACK_0)
	//	{
			/* return state of track 0 flag */
			flags |= drive_flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0;
	//	}

		/* disk image inserted into drive? */
	//	if (flag & FLOPPY_DRIVE_DISK_INSERTED)
	//	{
			flags |= drive_flags & FLOPPY_DRIVE_DISK_INSERTED;
	//	}

	//	if (flag & FLOPPY_DRIVE_DISK_WRITE_PROTECTED)
		{
			/* if disk image is read-only return write protected all the time */
			if (drive_flags & FLOPPY_DRIVE_DISK_IMAGE_READ_ONLY)
			{
				flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
			}
			else
			{
				/* return real state of write protected flag */
				flags |= drive_flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
			}
		}
	}

	/* drive present not */
	if (!(drive_flags & FLOPPY_DRIVE_CONNECTED))
	{
		/* adjust some flags if drive is not present */
		flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;
		flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
		flags &= ~FLOPPY_DRIVE_DISK_INSERTED;
	}

    flags &= flag;

	return flags;
}


void	floppy_drive_set_geometry(int id, floppy_type type)
{
	if ((id<0) || (id>=MAX_DRIVES))
		return;

	switch (type)
	{
		/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		case FLOPPY_DRIVE_SS_40:
		{
			drives[id].max_track = 42;
			drives[id].num_sides = 1;
		}
		break;

		case FLOPPY_DRIVE_DS_80:
		{
			drives[id].max_track = 83;
			drives[id].num_sides = 2;
		}
		break;
	}
}

void    floppy_drive_seek(int id, signed int signed_tracks)
{
	struct floppy_drive *pDrive;

	if ((id<0) || (id>=MAX_DRIVES))
		return;

	pDrive = &drives[id];

	if (floppy_drive_get_flag_state(id,FLOPPY_DRIVE_REAL_FDD))
	{
		/* real drive */
        osd_fdc_seek(pDrive->fdd_unit,signed_tracks);
	}
	else
	{
		/* update position */
        pDrive->current_track+=signed_tracks;

		if (pDrive->current_track<0)
		{
			pDrive->current_track = 0;
		}
		else
		if (pDrive->current_track>=pDrive->max_track)
		{
			pDrive->current_track = pDrive->max_track-1;
		}

		/* set track 0 flag */
		pDrive->flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;

		if (pDrive->current_track==0)
		{
			pDrive->flags |= FLOPPY_DRIVE_HEAD_AT_TRACK_0;
		}

		/* inform disk image of step operation so it can cache information */
		if (pDrive->interface.seek_callback)
			pDrive->interface.seek_callback(id, pDrive->current_track);
	}
}

/* this is not accurate. But it will do for now */
int	floppy_drive_get_next_id(int drive, int side, chrn_id *id)
{
	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_REAL_FDD))
	{
		osd_fdc_read_id(drive, side, drives[drive].id_buffer);

		id->C = drives[drive].id_buffer[0];
		id->H = drives[drive].id_buffer[1];
		id->R = drives[drive].id_buffer[2];
		id->N = drives[drive].id_buffer[3];
		id->flags = 0;
		id->data_id = 0;
		return 1;

	//	id.flags = 0;
	//	id.data_id = i;
	}
	else
	{
		int spt;

		/* get sectors per track */
		spt = 0;
		if (drives[drive].interface.get_sectors_per_track)
			spt = drives[drive].interface.get_sectors_per_track(drive, side);

		/* set index */
		if ((drives[drive].id_index==(spt-1)) || (spt==0))
		{
			floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_INDEX, 1);
		}
		else
		{
			floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_INDEX, 0);
		}

		/* get id */
		if (spt!=0)
		{
			if (drives[drive].interface.get_id_callback)
			{
				drives[drive].interface.get_id_callback(drive, id, drives[drive].id_index, side);
			}
		}

		drives[drive].id_index++;
		if (spt!=0)
		{
			drives[drive].id_index %= spt;
		}
		else
		{
			drives[drive].id_index = 0;
		}

		if (spt==0)
			return 0;
		
		return 1;
	}
}

int	floppy_drive_get_current_track(int drive)
{
	return drives[drive].current_track;
}

void	floppy_drive_format_sector(int drive, int side, int sector_index,int c,int h, int r, int n, int filler)
{
	if (drives[drive].interface.format_sector)
		drives[drive].interface.format_sector(drive, side, sector_index,c, h, r, n, filler);
}

void    floppy_drive_read_sector_data(int drive, int side, int index1, char *pBuffer, int length)
{
	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_REAL_FDD))
	{
		struct floppy_drive *pDrive = &drives[drive];

		logerror("real floppy read\n");

        /* track, head, sector */
		osd_fdc_get_sector(pDrive->fdd_unit,side, pDrive->id_buffer[0],
			pDrive->id_buffer[1], pDrive->id_buffer[2], pDrive->id_buffer[3],
			(unsigned char *)pBuffer, 0);
	}
	else
	{
		if (drives[drive].interface.read_sector_data_into_buffer)
                drives[drive].interface.read_sector_data_into_buffer(drive, side, index1, pBuffer,length);
	}
}

void    floppy_drive_write_sector_data(int drive, int side, int index1, char *pBuffer,int length, int ddam)
{
	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_REAL_FDD))
	{
		struct floppy_drive *pDrive = &drives[drive];

        /* track, head, sector */
		osd_fdc_put_sector(pDrive->fdd_unit,side, pDrive->id_buffer[0],
			pDrive->id_buffer[1], pDrive->id_buffer[2], pDrive->id_buffer[3],
			(unsigned char *)pBuffer, ddam);
	}
	else
	{
		if (drives[drive].interface.write_sector_data_from_buffer)
                drives[drive].interface.write_sector_data_from_buffer(drive, side, index1, pBuffer,length,ddam);
	}
}

int floppy_drive_get_datarate_in_us(DENSITY density)
{
	int usecs;
	/* 64 for single density */
	switch (density)
	{
		case DEN_FM_LO:
		{
			usecs = 128;
		}
		break;

		case DEN_FM_HI:
		{
			usecs = 64;
		}
		break;

		default:
		case DEN_MFM_LO:
		{
			usecs = 32;
		}
		break;

		case DEN_MFM_HI:
		{
			usecs = 16;
		}
		break;
	}

	return usecs;
}
