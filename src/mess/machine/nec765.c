/***************************************************************************

	machine/nec765.c

	Functions to emulate a NEC765 compatible floppy disk controller

	TODO:

    - overrun condition
	- Scan Commands
	- DMA interface (for PC driver)
	- deleted data/data and SKIP bit
	- disc not present, and no sectors on track for data, deleted data, write, write deleted,
		read a track etc
***************************************************************************/
#include "driver.h"
#include "mess/machine/nec765.h"

typedef enum
{
        NEC765_COMMAND_PHASE_FIRST_BYTE,
        NEC765_COMMAND_PHASE_BYTES,
        NEC765_RESULT_PHASE,
        NEC765_EXECUTION_PHASE_READ,
        NEC765_EXECUTION_PHASE_WRITE
} NEC765_PHASE;

/* uncomment the following line for verbose information */
//#define VERBOSE

#define FLOPPY_DRIVE_PRESENT					0x0008
#define FLOPPY_DRIVE_DISK_PRESENT				0x0001
#define FLOPPY_DRIVE_DISK_WRITE_PROTECTED		0x0002
#define FLOPPY_DRIVE_HEAD_AT_TRACK_0			0x0004

typedef struct floppy_drive
{
	/* flags */
	int flags;
	/* maximum track allowed */
	int max_track;
	/* num sides */
	int num_sides;
	/* current track - this may or may not relate to the present cylinder number
	stored by the fdc */
	int current_track;


	int id_index;
} floppy_drive;


/* Data Request (DRQ) output */
#define NEC765_DRQ	0x01
/* Interrupt (INT) output */
#define NEC765_INT	0x02
/* data rate for floppy discs (MFM data) */
#define NEC765_DATA_RATE	32
/* terminal count input state */
#define NEC765_TC	0x04

#define NEC765_DMA_MODE 0x08

#define NEC765_SEEK_ACTIVE 0x010

typedef struct nec765
{
	unsigned long	sector_counter;

	/* main status register */
	unsigned char    FDC_main;
	/* data register */
	unsigned char	nec765_data_reg;

	NEC765_PHASE    nec765_phase;
	unsigned int    nec765_command_bytes[16];
	unsigned int    nec765_result_bytes[16];
	unsigned int    nec765_transfer_bytes_remaining;
	unsigned int    nec765_transfer_bytes_count;
	unsigned int    nec765_status[4];
	/* present cylinder number per drive */
	unsigned int    pcn[4];
	/* drive being accessed. drive outputs from fdc */
	unsigned int    drive;
	/* side being accessed: side output from fdc */
	unsigned int	nec765_side;
	/* step rate time in us */
	unsigned long	srt_in_ms;

	unsigned int	ncn;

//	unsigned int    nec765_id_index;
	char *execution_phase_data;
	unsigned int	nec765_flags;

	void *seek_timer;
	void *timer;
	int timer_type;
} NEC765;

//static void nec765_setup_data_request(unsigned char Data);
static void     nec765_setup_command(void);
static void  nec765_idle(void);
static void 	nec765_continue_command(void);

static NEC765 fdc;
static floppy_drive	drives[4];

static floppy_drive *get_floppy_drive_ptr(int drive_index)
{
	return &drives[drive_index];
}

static int floppy_drive_motor_state;

void	floppy_drive_set_motor_state(int state)
{
	floppy_drive_motor_state = state;

}


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

void	floppy_drive_set_geometry(int drive, floppy_type type)
{
	floppy_drive *pDrive = get_floppy_drive_ptr(drive);

	switch (type)
	{
		/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		case FLOPPY_DRIVE_SS_40:
		{
			pDrive->max_track = 42;
			pDrive->num_sides = 1;
		}
		break;

		case FLOPPY_DRIVE_DS_80:
		{
			pDrive->max_track = 83;
			pDrive->num_sides = 2;
		}
		break;
	}

	pDrive->id_index = 0;
	pDrive->current_track = 0;

}


void	floppy_drive_seek(int drive, int signed_tracks)
{
	floppy_drive *pDrive = get_floppy_drive_ptr(drive);

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
}


nec765_interface nec765_iface;



//int drive_state[4]=
//{
//        0,0,0,0
//};

static int nec765_cmd_size[32] = {
	1,1,9,3,2,9,9,2,1,9,2,1,9,6,1,3,
	1,9,1,1,1,1,9,1,1,9,1,1,1,9,1,1
};

static int nec765_sta_size[32] = {
	1,1,7,0,1,7,7,0,2,7,7,1,7,7,1,0,
	1,7,1,1,1,1,7,1,1,7,1,1,1,7,1,1
};

static void nec765_setup_drive_and_side(void)
{
	// drive index nec765 sees
	fdc.drive = fdc.nec765_command_bytes[1] & 0x03;
	// side index nec765 sees
	fdc.nec765_side = (fdc.nec765_command_bytes[1]>>2) & 0x01;
}


static void nec765_set_data_request(void)
{
	fdc.nec765_flags |= NEC765_DRQ;

	fdc.FDC_main |= 0x080;
}

static void nec765_clear_data_request(void)
{
	fdc.nec765_flags &= ~NEC765_DRQ;

	fdc.FDC_main &= ~0x080;
}

static void nec765_seek_complete(void)
{
	if (drives[fdc.drive ].flags & FLOPPY_DRIVE_DISK_PRESENT)
	{
		fdc.pcn[fdc.drive] = fdc.ncn;
		fdc.nec765_status[0] = 0x020;
		fdc.nec765_status[0] |= fdc.drive | (fdc.nec765_side<<2);


		if (nec765_iface.seek_callback)
		   nec765_iface.seek_callback(fdc.drive, fdc.pcn[fdc.drive]);

		nec765_set_int(1);
	}
	else
	{
		/* seek end, drive not ready, and command didn't succeed properly */

		fdc.nec765_status[0] = 0x020  | 0x08 | 0x040 |fdc.drive | (fdc.nec765_side<<2);
		fdc.pcn[fdc.drive] = 0;

		nec765_set_int(1);

	}

	fdc.nec765_flags &= ~NEC765_SEEK_ACTIVE;
}

static void nec765_seek_timer_callback(int param)
{
		/* seek complete */
		nec765_seek_complete();

		timer_reset(fdc.seek_timer, TIME_NEVER);
}

static void nec765_timer_callback(int param)
{
	/* type 0 = data transfer mode in execution phase */
	if (fdc.timer_type==0)
	{
		/* set data request */
		nec765_set_data_request();

		fdc.timer_type = 4;

		timer_reset(fdc.timer, TIME_IN_USEC(27));
	}
	else if (fdc.timer_type==2)
	{
		/* result phase begin */

		/* generate a int for specific commands */
		switch (fdc.nec765_command_bytes[0] & 0x01f)
		{
			/* read a track */
			case 2:
			/* write data */
			case 5:
			/* read data */
			case 6:
			/* write deleted data */
			case 9:
			/* read id */
			case 10:
			/* read deleted data */
			case 12:
			/* format at track */
			case 13:
			/* scan equal */
			case 17:
			/* scan low or equal */
			case 19:
			/* scan high or equal */
			case 29:
			{
				nec765_set_int(1);
			}
			break;

			default:
				break;
		}

		nec765_set_data_request();

		timer_reset(fdc.timer, TIME_NEVER);
	}
	else if (fdc.timer_type == 4)
	{
		/* if in dma mode, a int is not generated per byte. If not in  DMA mode
		a int is generated per byte */
		if (fdc.nec765_flags & NEC765_DMA_MODE)
		{
			/* issue drq for DMA controller */
		}
		else
		{
			if (fdc.nec765_flags & NEC765_DRQ)
			{
				/* set int to indicate data is ready */
				nec765_set_int(1);
			}
		}

		timer_reset(fdc.timer, TIME_NEVER);
	}

}

/* after (32-27) the DRQ is set, then 27 us later, the int is set.
I don't know if this is correct, but it is required for the PCW driver.
In this driver, the first NMI calls the handler function, furthur NMI's are
effectively disabled by reading the data before the NMI int can be set.
*/

/* setup data request */
static void nec765_setup_timed_data_request(int bytes)
{
	/* setup timer to trigger in NEC765_DATA_RATE us */
	fdc.timer_type = 0;
	if (fdc.timer)
	{
		/* disable the timer */
		timer_remove(fdc.timer);	//timer_enable(fdc.timer, 0);
		fdc.timer = 0;
	}
	fdc.timer = timer_set(TIME_IN_USEC(32-27)	/*NEC765_DATA_RATE)*bytes*/, 0, nec765_timer_callback);
}

/* setup result data request */
static void nec765_setup_timed_result_data_request(void)
{
	fdc.timer_type = 2;
	if (fdc.timer)
	{
		/* disable the timer */
		timer_remove(fdc.timer);
		fdc.timer = 0;
	}
	fdc.timer = timer_set(TIME_IN_USEC(NEC765_DATA_RATE)*2, 0, nec765_timer_callback);
}

/* setup data request */
static void nec765_setup_timed_int(int signed_tracks)
{
	if (drives[fdc.drive].flags & FLOPPY_DRIVE_DISK_PRESENT)
	{
		fdc.FDC_main |= (1<<fdc.drive);

		/* setup timer to trigger in NEC765_DATA_RATE us */
		if (fdc.seek_timer)
		{
			/* disable the timer */
			timer_remove(fdc.seek_timer);	//timer_enable(fdc.timer, 0);
			fdc.seek_timer = 0;
		}

		fdc.seek_timer = timer_pulse(TIME_IN_MSEC(fdc.srt_in_ms*abs(signed_tracks)), 0, nec765_seek_timer_callback);

	//	timer_reset(fdc.timer, fdc.srt_in_us*abs(signed_tracks));
	//	timer_enable(fdc.timer, 1);

	}
	else
	{
		nec765_seek_complete();
	}
}

static void nec765_seek_setup(int is_recalibrate)
{
	int signed_tracks;
	floppy_drive *pDrive;

	fdc.nec765_flags |= NEC765_SEEK_ACTIVE;

	if (is_recalibrate)
	{
		/* head cannot be specified with recalibrate */
		fdc.nec765_command_bytes[1] &=~0x04;
	}

	nec765_setup_drive_and_side();

	pDrive = get_floppy_drive_ptr(fdc.drive);

	if (is_recalibrate)
	{
		fdc.ncn = 0;
		signed_tracks = -pDrive->current_track;

		floppy_drive_seek(fdc.drive, signed_tracks);

		if (signed_tracks!=0)
		{
			nec765_setup_timed_int(signed_tracks);
		}
		else
		{
			nec765_seek_complete();
		}
	}
	else
	{

		fdc.ncn = fdc.nec765_command_bytes[2];

		/* get signed tracks */
		signed_tracks = fdc.ncn - fdc.pcn[fdc.drive];

		floppy_drive_seek(fdc.drive, signed_tracks);

		if (signed_tracks!=0)
		{
			/* seek complete - issue an interrupt */
			nec765_setup_timed_int(signed_tracks);
		}
		else
		{
			nec765_seek_complete();
		}


	}

    nec765_idle();

}



static void     nec765_setup_execution_phase_read(char *ptr, int size)
{
//        fdc.FDC_main |=0x080;                       /* DRQ */
        fdc.FDC_main |= 0x040;                     /* FDC->CPU */

        fdc.nec765_transfer_bytes_count = 0;
        fdc.nec765_transfer_bytes_remaining = size;
        fdc.execution_phase_data = ptr;
        fdc.nec765_phase = NEC765_EXECUTION_PHASE_READ;

		/* setup a data request with first byte */
//		fdc.nec765_data_reg = fdc.execution_phase_data[fdc.nec765_transfer_bytes_count];
//		fdc.nec765_transfer_bytes_count++;
//		fdc.nec765_transfer_bytes_remaining--;
		nec765_setup_timed_data_request(1);
}

static void     nec765_setup_execution_phase_write(char *ptr, int size)
{
//        fdc.FDC_main |=0x080;                       /* DRQ */
        fdc.FDC_main &= ~0x040;                     /* FDC->CPU */

        fdc.nec765_transfer_bytes_count = 0;
        fdc.nec765_transfer_bytes_remaining = size;
        fdc.execution_phase_data = ptr;
        fdc.nec765_phase = NEC765_EXECUTION_PHASE_WRITE;

		/* setup a data request with first byte */
		nec765_setup_timed_data_request(1);
}


static void     nec765_setup_result_phase(void)
{
	//fdc.nec765_flags &= ~NEC765_TC;

		fdc.FDC_main |= 0x040;                     /* FDC->CPU */
        fdc.FDC_main &= ~0x020;                    /* not execution phase */

        fdc.nec765_transfer_bytes_count = 0;
        fdc.nec765_transfer_bytes_remaining = nec765_sta_size[fdc.nec765_command_bytes[0] & 0x01f];
        fdc.nec765_phase = NEC765_RESULT_PHASE;

		nec765_setup_timed_result_data_request();
}

void nec765_idle(void)
{
	//fdc.nec765_flags &= ~NEC765_TC;

    fdc.FDC_main &= ~0x040;                     /* CPU->FDC */
    fdc.FDC_main &= ~0x020;                    /* not execution phase */
    fdc.FDC_main &= ~0x010;                     /* not busy */
    fdc.nec765_phase = NEC765_COMMAND_PHASE_FIRST_BYTE;

	nec765_set_data_request();
}

void	nec765_set_int(int state)
{
	fdc.nec765_flags &= ~NEC765_INT;

	if (state)
	{
		fdc.nec765_flags |= NEC765_INT;
	}

	if (nec765_iface.interrupt)
		nec765_iface.interrupt((fdc.nec765_flags & NEC765_INT));
}

void    nec765_init(nec765_interface *iface)
{
		fdc.timer = 0;	//timer_set(TIME_NEVER, 0, nec765_timer_callback);
		fdc.seek_timer = 0;
	memset(&nec765_iface, 0, sizeof(nec765_interface));

        if (iface)
        {
                memcpy(&nec765_iface, iface, sizeof(nec765_interface));
        }

		fdc.nec765_flags = 0;

		nec765_idle();

		nec765_set_int(0);

		//floppy_drive_init();
		//timer_enable(fdc.timer, 0);
}

void    floppy_drive_setup_drive_status(int drive)
{
        drives[drive].flags |= FLOPPY_DRIVE_DISK_PRESENT;
}

void	nec765_set_tc_state(int state)
{


	fdc.nec765_flags &= ~NEC765_TC;
	if (state)
	{
		fdc.nec765_flags |= NEC765_TC;
		if (fdc.timer)
		{
			if (fdc.timer_type==0)
			{
				timer_remove(fdc.timer);
				fdc.timer = 0;
			}
		}
		nec765_continue_command();
	}
}

READ_HANDLER(nec765_status_r)
{
	return fdc.FDC_main;
}


static void     nec765_read_data(void)
{
        int data_size;
        int idx;
        int spt;

        idx = -1;
        spt = 0;

        if (nec765_iface.get_sectors_per_track)
        {
                spt = nec765_iface.get_sectors_per_track(fdc.drive, fdc.nec765_side);
        }

        if (spt!=0)
        {
                nec765_id id;
                int i;

                for (i=0; i<spt; i++)
                {
                     nec765_iface.get_id_callback(fdc.drive, &id, i, fdc.nec765_side);

                     if ((id.R == fdc.nec765_command_bytes[4]) &&
						 (id.C == fdc.nec765_command_bytes[2]) &&
						 (id.H == fdc.nec765_command_bytes[3]) &&
                			 (id.N == fdc.nec765_command_bytes[5]))
			     {
		               idx = i;
                        break;
                     }
                }
        }

        if (idx!=-1)
        {
                char *ptr;

                nec765_iface.get_sector_data_callback(fdc.drive, idx, fdc.nec765_side, &ptr);


                /* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
                /* data_size = ((1<<(N+7)) */
                data_size = 1<<(fdc.nec765_command_bytes[5]+7);

                nec765_setup_execution_phase_read(ptr, data_size);
        }
        else
        {
                fdc.nec765_status[0] = 0x040 | fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0x04;
                fdc.nec765_status[2] = 0x00;

                fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

                nec765_setup_result_phase();
        }

}

static void     nec765_format_track(void)
{
	char * ptr;

	/* get ids */
 	 ptr = (char*)&fdc.nec765_result_bytes[0];
       nec765_setup_execution_phase_write(ptr, 4);
}

static void     nec765_read_a_track(void)
{
        int data_size;
        int idx;
        int spt;

        idx = -1;
        spt = 0;

        if (nec765_iface.get_sectors_per_track)
        {
                spt = nec765_iface.get_sectors_per_track(fdc.drive, fdc.nec765_side);
        }

     	  idx = fdc.sector_counter % spt;
		  {
		char *ptr;

        nec765_iface.get_sector_data_callback(fdc.drive, idx, fdc.nec765_side, &ptr);

        /* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
        /* data_size = ((1<<(N+7)) */
        data_size = 1<<(fdc.nec765_command_bytes[5]+7);

        nec765_setup_execution_phase_read(ptr, data_size);
		}
}

static void     nec765_write_data(void)
{
        int data_size;
        int idx;
        int spt;

        idx = -1;
        spt = 0;

        if (nec765_iface.get_sectors_per_track)
        {
                spt = nec765_iface.get_sectors_per_track(fdc.drive, fdc.nec765_side);
        }

        if (spt!=0)
        {
                nec765_id id;
                int i;

                for (i=0; i<spt; i++)
                {
                     nec765_iface.get_id_callback(fdc.drive, &id, i, fdc.nec765_side);

                     if ((id.R == fdc.nec765_command_bytes[4]) &&
						 (id.C == fdc.nec765_command_bytes[2]) &&
						 (id.H == fdc.nec765_command_bytes[3]) &&
						 (id.N == fdc.nec765_command_bytes[5]))
                     {
                        idx = i;
                        break;
                     }
                }
        }

        if (idx!=-1)
        {
                char *ptr;

                nec765_iface.get_sector_data_callback(fdc.drive, idx, fdc.nec765_side, &ptr);
                //ptr=&ptr;
                /* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
                /* data_size = ((1<<(N+7)) */
                data_size = 1<<(fdc.nec765_command_bytes[5]+7);

                nec765_setup_execution_phase_write(ptr, data_size);
        }
        else
        {
                fdc.nec765_status[0] = 0x040 | fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0x04;
                fdc.nec765_status[2] = 0x00;

                fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

                nec765_setup_result_phase();
        }

}



static void     nec765_continue_command(void)
{
        switch (fdc.nec765_command_bytes[0] & 0x01f)
        {
			/* read a track */
			case 0x02:
			{
				/* sector counter == EOT */
				if ((fdc.sector_counter == fdc.nec765_command_bytes[6]) || (fdc.nec765_flags & NEC765_TC))
				{
				        fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

				        nec765_setup_result_phase();
				}
				else
				{
					fdc.sector_counter++;

					nec765_read_a_track();
				}
			}
			break;

			/* format track */
			case 0x0d:
			{
				/* sector_counter = SC */
				if ((fdc.sector_counter == fdc.nec765_command_bytes[3]) || (fdc.nec765_flags & NEC765_TC))
				{
				        fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

				        nec765_setup_result_phase();
				}
				else
				{
					fdc.sector_counter++;

					nec765_format_track();
				}
			}
			break;



			/* write data, write deleted data */
			case 0x09:
                case 0x05:
				/* sector id == EOT */

				if ((fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6]) || (fdc.nec765_flags & NEC765_TC))
                        {
				        fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

                                nec765_setup_result_phase();
				}
				else
				{
					fdc.nec765_command_bytes[4]++;

					nec765_write_data();
				}
				break;

			/* read data, read deleted data */
			case 0x0c:
                case 0x06:
                {
                        /* read all sectors? */

				/* sector id == EOT */
				if ((fdc.nec765_command_bytes[4]==fdc.nec765_command_bytes[6]) || (fdc.nec765_flags & NEC765_TC))
                        {
					  fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                                fdc.nec765_result_bytes[3] = fdc.nec765_command_bytes[2]; /* C */
                                fdc.nec765_result_bytes[4] = fdc.nec765_command_bytes[3]; /* H */
                                fdc.nec765_result_bytes[5] = fdc.nec765_command_bytes[4]; /* R */
                                fdc.nec765_result_bytes[6] = fdc.nec765_command_bytes[5]; /* N */

                                nec765_setup_result_phase();
                        }
                        else
                        {
                                fdc.nec765_command_bytes[4]++;

                                nec765_read_data();
                        }
                }
                break;


                default:
                        break;
       }
}





void	nec765_update_state(void)
{
    switch (fdc.nec765_phase)
    {
         case NEC765_RESULT_PHASE:
         {
             /* set data reg */
			 fdc.nec765_data_reg = fdc.nec765_result_bytes[fdc.nec765_transfer_bytes_count];

			 if (fdc.nec765_transfer_bytes_count==0)
			 {
				/* clear int for specific commands */
				switch (fdc.nec765_command_bytes[0] & 0x01f)
				{
					/* read a track */
					case 2:
					/* write data */
					case 5:
					/* read data */
					case 6:
					/* write deleted data */
					case 9:
					/* read id */
					case 10:
					/* read deleted data */
					case 12:
					/* format at track */
					case 13:
					/* scan equal */
					case 17:
					/* scan low or equal */
					case 19:
					/* scan high or equal */
					case 29:
					{
						nec765_set_int(0);
					}
					break;

					default:
						break;
				}
			 }

#ifdef VERBOSE
             logerror("NEC765: RESULT: %02x\r\n", fdc.nec765_data_reg);
#endif

             fdc.nec765_transfer_bytes_count++;
             fdc.nec765_transfer_bytes_remaining--;

            if (fdc.nec765_transfer_bytes_remaining==0)
            {
				nec765_idle();
            }
			else
			{
				nec765_set_data_request();
			}
		 }
		 break;

         case NEC765_EXECUTION_PHASE_READ:
         {
			 /* setup data register */
             fdc.nec765_data_reg = fdc.execution_phase_data[fdc.nec765_transfer_bytes_count];
             fdc.nec765_transfer_bytes_count++;
             fdc.nec765_transfer_bytes_remaining--;

#ifdef VERBOSE
			logerror("EXECUTION PHASE READ: %02x\r\n", fdc.nec765_data_reg);
#endif

            if (fdc.nec765_transfer_bytes_remaining==0)
            {
                nec765_continue_command();
            }
			else
			{
				// trigger int
				nec765_setup_timed_data_request(1);
			}
		 }
		 break;

	    case NEC765_COMMAND_PHASE_FIRST_BYTE:
        {
                fdc.FDC_main |= 0x10;                      /* set BUSY */
#ifdef VERBOSE
                logerror("NEC765: COMMAND: %02x\r\n",fdc.nec765_data_reg);
#endif
				/* seek in progress? */
				if (fdc.nec765_flags & NEC765_SEEK_ACTIVE)
				{
					/* any command results in a invalid - I think that seek, recalibrate and
					sense interrupt status may work*/
					fdc.nec765_data_reg = 0;
				}


				fdc.nec765_command_bytes[0] = fdc.nec765_data_reg;
                fdc.nec765_transfer_bytes_remaining = nec765_cmd_size[fdc.nec765_data_reg & 0x01f];
                fdc.nec765_transfer_bytes_count = 1;

                fdc.nec765_transfer_bytes_remaining--;

                if (fdc.nec765_transfer_bytes_remaining==0)
                {
                        nec765_setup_command();
                }
                else
                {
						/* request more data */
						nec765_set_data_request();
                        fdc.nec765_phase = NEC765_COMMAND_PHASE_BYTES;
                }
        }
        break;

                case NEC765_COMMAND_PHASE_BYTES:
                {
#ifdef VERBOSE
                        logerror("NEC765: COMMAND: %02x\r\n",fdc.nec765_data_reg);
#endif
                        fdc.nec765_command_bytes[fdc.nec765_transfer_bytes_count] = fdc.nec765_data_reg;
                        fdc.nec765_transfer_bytes_count++;
                        fdc.nec765_transfer_bytes_remaining--;

                        if (fdc.nec765_transfer_bytes_remaining==0)
                        {
                                nec765_setup_command();
                        }
						else
						{
							/* request more data */
							nec765_set_data_request();
						}

                }
                break;

            case NEC765_EXECUTION_PHASE_WRITE:
            {
                fdc.execution_phase_data[fdc.nec765_transfer_bytes_count]=fdc.nec765_data_reg;
                fdc.nec765_transfer_bytes_count++;
                fdc.nec765_transfer_bytes_remaining--;

                if (fdc.nec765_transfer_bytes_remaining==0)
                {

                        nec765_continue_command();
                }
				else
				{
					nec765_setup_timed_data_request(1);
				}
            }
		    break;

	}
}


READ_HANDLER(nec765_data_r)
{
//	int data;

	/* get data we will return */
//	data = fdc.nec765_data_reg;


	if ((fdc.FDC_main & 0x0c0)==0x0c0)
	{
		if (
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_READ) ||
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_WRITE))
		{

			/* reading the data byte clears the interrupt */
			nec765_set_int(0);
		}

		/* reset data request */
		nec765_clear_data_request();

		/* update state */
		nec765_update_state();
	}

#ifdef VERBOSE
	logerror("DATA R: %02x\r\n", fdc.nec765_data_reg);
#endif

	return fdc.nec765_data_reg;
}

WRITE_HANDLER(nec765_data_w)
{
#ifdef VERBOSE
	logerror("DATA W: %02x\r\n", data);
#endif

	/* write data to data reg */
	fdc.nec765_data_reg = data;

	if ((fdc.FDC_main & 0x0c0)==0x080)
	{
		if (
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_READ) ||
			(fdc.nec765_phase == NEC765_EXECUTION_PHASE_WRITE))
		{

			/* reading the data byte clears the interrupt */
			nec765_set_int(0);
		}

		/* reset data request */
		nec765_clear_data_request();

		/* update state */
		nec765_update_state();
	}
}

static void nec765_setup_invalid(void)
{
	fdc.nec765_command_bytes[0] = 0;
	fdc.nec765_result_bytes[0] = 0x080;
	nec765_setup_result_phase();
}

static void     nec765_setup_command(void)
{
//	nec765_clear_data_request();

        fdc.FDC_main |= 0x020;              /* execution phase */

        switch (fdc.nec765_command_bytes[0] & 0x01f)
        {
            case 0x03:      /* specify */
			{
				/* setup step rate */
				fdc.srt_in_ms = 16-((fdc.nec765_command_bytes[1]>>4) & 0x0f);

				fdc.nec765_flags &= ~NEC765_DMA_MODE;

				if ((fdc.nec765_command_bytes[2] & 0x01)==0)
				{
					fdc.nec765_flags |= NEC765_DMA_MODE;
				}

                nec765_idle();
            }
			break;

            case 0x04:  /* sense drive status */
			{
				floppy_drive *pDrive;

				nec765_setup_drive_and_side();

				pDrive = get_floppy_drive_ptr(fdc.drive);

                fdc.nec765_status[3] = fdc.drive | (fdc.nec765_side<<2);

				/* write protected is set if disc is present and has write
				protect setting, or if drive is present without a disk inserted */
				if ((pDrive->flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED) ||
					((pDrive->flags & FLOPPY_DRIVE_DISK_PRESENT)==0))
				{
					fdc.nec765_status[3] |= 0x040;
				}

				/* assume drive is always ready if present - not always true because
				it should only be ready if drive motor is active */
				if (pDrive->flags & FLOPPY_DRIVE_DISK_PRESENT)
				{
					if (floppy_drive_motor_state==1)
					{
						fdc.nec765_status[3] |= 0x020;
					}
				}

				if (pDrive->flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0)
				{
					fdc.nec765_status[3] |= 0x010;
				}

				fdc.nec765_status[3] |= 0x08;


				/* two side and fault not set but should be? */

                fdc.nec765_result_bytes[0] = fdc.nec765_status[3];

                nec765_setup_result_phase();
			}
			break;

            case 0x07:          /* recalibrate */
                nec765_seek_setup(1);
                break;
            case 0x0f:          /* seek */
				nec765_seek_setup(0);
				break;
            case 0x0a:      /* read id */
            {
                int spt;
                nec765_id id;

				nec765_setup_drive_and_side();

                spt = 0;

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;

                if (nec765_iface.get_sectors_per_track)
                {
                        spt = nec765_iface.get_sectors_per_track(fdc.drive, fdc.nec765_side);
                }

                if (spt!=0)
                {
                        drives[fdc.drive].id_index++;
                        drives[fdc.drive].id_index = drives[fdc.drive].id_index % spt;

                        if (nec765_iface.get_id_callback)
                                nec765_iface.get_id_callback(fdc.drive, &id, drives[fdc.drive].id_index, fdc.nec765_side);
                 }
                 else
                 {
                        fdc.nec765_status[0] = 0x040 | fdc.drive | (fdc.nec765_side<<2);
                        fdc.nec765_status[1] |= 0x04;       /* no data */
                 }

                fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
                fdc.nec765_result_bytes[1] = fdc.nec765_status[1];
                fdc.nec765_result_bytes[2] = fdc.nec765_status[2];
                fdc.nec765_result_bytes[3] = id.C; /* C */
                fdc.nec765_result_bytes[4] = id.H; /* H */
                fdc.nec765_result_bytes[5] = id.R; /* R */
                fdc.nec765_result_bytes[6] = id.N; /* N */


                 nec765_setup_result_phase();
            }
            break;


		case 0x08: /* sense interrupt status */
  			/* interrupt pending? */
			if (fdc.nec765_flags & NEC765_INT)
			{
				/* yes. Clear int */
				nec765_set_int(0);

				/* clear drive seek bits */
				fdc.FDC_main &= ~(1 | 2 | 4 | 8);

				/* return status */
				fdc.nec765_result_bytes[0] = fdc.nec765_status[0];
           		/* return pcn */
				fdc.nec765_result_bytes[1] = fdc.pcn[fdc.drive];
			}
			else
			{
				/* no int */
				nec765_setup_invalid();
			}

			/* return result */
			nec765_setup_result_phase();
            break;

		  case 0x06:  /* read data */
            {

				nec765_setup_drive_and_side();

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;


                nec765_read_data();
            }
	    	break;

		/* read deleted data */
		case 0x0c:
		{

			nec765_setup_drive_and_side();

            fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
			fdc.nec765_status[1] = 0;
			fdc.nec765_status[2] = 0;


			/* .. for now */
			nec765_read_data();
		}
		break;

		/* write deleted data */
		case 0x09:
		{
				nec765_setup_drive_and_side();

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;

				nec765_setup_drive_and_side();

			/* ... for now */
                nec765_write_data();
            }
            break;

		/* read a track */
		case 0x02:
		{
				nec765_setup_drive_and_side();

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;

				nec765_setup_drive_and_side();

			fdc.sector_counter = 0;

                nec765_read_a_track();
            }
            break;

            case 0x05:  /* write data */
            {
				nec765_setup_drive_and_side();

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;

				nec765_setup_drive_and_side();

                nec765_write_data();
            }
            break;

		/* format a track */
		case 0x0d:
		{
				nec765_setup_drive_and_side();

                fdc.nec765_status[0] = fdc.drive | (fdc.nec765_side<<2);
                fdc.nec765_status[1] = 0;
                fdc.nec765_status[2] = 0;


			nec765_setup_drive_and_side();

			fdc.sector_counter = 0;

			nec765_format_track();
		}
		break;

			/* invalid */
            default:
				nec765_setup_invalid();
                break;
        }
}


