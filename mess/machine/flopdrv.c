#include "driver.h"
#include "includes/flopdrv.h"

#define MAX_DRIVES 4

static floppy_drive	drives[MAX_DRIVES];

/* init all floppy drives */
void	floppy_drives_init(void)
{
/* KT - caused problems with disk missing etc */
//        memset(&drives[0], 0, sizeof(floppy_drive));
//        memset(&drives[1], 0, sizeof(floppy_drive));
//        memset(&drives[2], 0, sizeof(floppy_drive));
//        memset(&drives[3], 0, sizeof(floppy_drive));
//        drives[0].current_track = 10;
//        drives[1].current_track = 10;
//        drives[2].current_track = 10;
//        drives[3].current_track = 10;
}

void	floppy_drive_set_interface(int index1, floppy_interface *iface)
{
	if ((index1<0) || (index1>=MAX_DRIVES))
		return;

	memcpy(&drives[index1].interface, iface, sizeof(floppy_interface));
}

/* set flag state */
void	floppy_drive_set_flag_state(int drive, int flag, int state)
{
	drives[drive].flags &= ~flag;

	if (state)
	{
		drives[drive].flags |= flag;
	}

	if (flag==FLOPPY_DRIVE_DISK_PRESENT)
	{
		floppy_drive_seek(drive, 0);
	}

}

void	floppy_drive_set_motor_state(int drive, int state)
{
	floppy_drive_set_flag_state(drive, FLOPPY_DRIVE_MOTOR_ON, state);
}


void	floppy_drive_set_ready_state(int drive, int state, int flag)
{
	if (flag)
	{
		/* set ready only if drive is present, disk is in the drive,
		and disk motor is on - for Amstrad, Spectrum and PCW*/
	
		/* drive present? */
		if (drives[drive].flags & FLOPPY_DRIVE_PRESENT)
		{
			/* disk inserted? */
			if (drives[drive].flags & FLOPPY_DRIVE_DISK_PRESENT)
			{
				if (drives[drive].flags & FLOPPY_DRIVE_MOTOR_ON)
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
int		floppy_drive_get_flag_state(int drive, int flag)
{
	switch (flag)
	{
		case FLOPPY_DRIVE_HEAD_AT_TRACK_0:
		{
			/* drive present */
			if (drives[drive].flags & FLOPPY_DRIVE_PRESENT)
			{
				/* return real state of track 0 flag */
				return drives[drive].flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0;
			}
	
			/* return not at track 0 */
			return 0;
		}

		case FLOPPY_DRIVE_PRESENT:
			return drives[drive].flags & flag;
	

		/* return ready state - in case of CPC drive will not be ready if it is not present */
		/* in case of PC drive will be ready even if it is not present */
		case FLOPPY_DRIVE_READY:
			return drives[drive].flags & flag;
	
		case FLOPPY_DRIVE_DISK_WRITE_PROTECTED:
		{
			/* drive present */
			if (drives[drive].flags & FLOPPY_DRIVE_PRESENT)
			{
				/* return real state of write protected flag */
				return drives[drive].flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
			}
	
			/* drive not present. return write protected  */
			return FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
		}

		case FLOPPY_DRIVE_INDEX:
			return drives[drive].flags & flag;

		case FLOPPY_DRIVE_DISK_PRESENT:
			return drives[drive].flags & flag;

		default:
			break;
	}

	return 0;
}




//static int floppy_drive_motor_state;
//
//void	floppy_drive_set_motor_state(int state)
//{
//	floppy_drive_motor_state = state;
//
//}

#if 0
void	floppy_drive_init(void)
{
	int i;

	for (i=0; i<4; i++)
	{
		floppy_drive *pDrive = get_floppy_drive_ptr(i);

		pDrive->id_index = 0;
		pDrive->current_track = 0;
		pDrive->flags = FLOPPY_DRIVE_HEAD_AT_TRACK_0;

		/* 	When Drive is accessed but not fitted,
		Drive Ready and Write Protected status signals are false. */

		/* When Drive is  fitted and accessed with no disk inserted,
		Drive Ready status from Drive is false and Write Protected status
		from Drive 1 is true. */

		floppy_drive_set_geometry(i, FLOPPY_DRIVE_SS_40);
	}
}
#endif

void	floppy_drive_set_geometry(int drive, floppy_type type)
{
	switch (type)
	{
		/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		case FLOPPY_DRIVE_SS_40:
		{
			drives[drive].max_track = 42;
			drives[drive].num_sides = 1;
		}
		break;

		case FLOPPY_DRIVE_DS_80:
		{
			drives[drive].max_track = 83;
			drives[drive].num_sides = 2;
		}
		break;
	}

	drives[drive].id_index = 0;
	drives[drive].current_track = 0;
	drives[drive].flags |= FLOPPY_DRIVE_HEAD_AT_TRACK_0;

}


void	floppy_drive_seek(int drive, int signed_tracks)
{
	floppy_drive *pDrive = &drives[drive];

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

	if (floppy_drive_get_flag_state(drive, FLOPPY_DRIVE_PRESENT))
	{
		if (drives[drive].interface.seek_callback)

			drives[drive].interface.seek_callback(drive, pDrive->current_track);
	}
}

/* this is not accurate. But it will do for now */
void	floppy_drive_get_next_id(int drive, int side, chrn_id *id)
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
	if (drives[drive].interface.get_id_callback)
	{
		drives[drive].interface.get_id_callback(drive, id, drives[drive].id_index, side);
	}

	id->data_id = drives[drive].id_index;

	drives[drive].id_index++;
	if (spt!=0)
	{
		drives[drive].id_index %= spt;
	}
	else
	{
		drives[drive].id_index = 0;
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
	if (drives[drive].interface.read_sector_data_into_buffer)
                drives[drive].interface.read_sector_data_into_buffer(drive, side, index1, pBuffer,length);
	
}

void    floppy_drive_write_sector_data(int drive, int side, int index1, char *pBuffer,int length)
{
	if (drives[drive].interface.write_sector_data_from_buffer)
                drives[drive].interface.write_sector_data_from_buffer(drive, side, index1, pBuffer,length);
	
}

