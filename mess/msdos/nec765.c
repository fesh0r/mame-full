/*****************************************************************************
 *
 *	nec765.c
 *
 *	allow direct access to a PCs NEC 765 floppy disc controller
 *
 *  KT - uses logerror now.
 *     - removed references to wd179x and wd179x error codes
 *	!!!! this is ALPHA - be careful !!!!
 *
 *****************************************************************************/

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <time.h>
#include "driver.h"
#include "includes/flopdrv.h"

#define NEC765_NOT_READY	0x001

#define PC_DOR_DMA_ENABLED (1<<3)
#define PC_DOR_FDC_ENABLED (1<<2)


#define RE_INTERLEAVE	0
#define VERB0
/* since most PC floppy disc controllers don't really support FM, */
/* set this to 0x40 to always format/read/write in MFM mode */
#define MFM_OVERRIDE	0x40

#if VERBOSE
#define LOG(msg) logerror(msg)
#else
#define LOG(msg)
#endif


/* base address of fdc controller i/o ports */
static UINT16 fdc_hw_base = 0x03f0;

/* offsets to each register (these are the only registers we are interested in */
const UINT16 FDC_DIGITAL_OUTPUT_REGISTER_OFFSET			= 2;
const UINT16 FDC_MAIN_STATUS_REGISTER_OFFSET			= 4;
const UINT16 FDC_DATA_REGISTER_OFFSET					= 5;
const UINT16 FDC_DATA_RATE_REGISTER_OFFSET				= 7;

/* step rate value programmed with specify command */
static unsigned long step_rate = 0x0d;
/* head unload time programmed with specify command */
static unsigned long head_unload_time = 0x0f;
/* head load time programmed with specify command */
static unsigned long head_load_time = 0x02;

/* the actual time that a step, head unload and head load operation
would take is dependant on the data-rate selected.*/

/* time for a single step */
static unsigned long step_rate_usec;
/* time for head to unload */
static unsigned long head_unload_time_usec;
/* time for head to load */
static unsigned long head_load_time_usec;


static int detect_floppy_drive(int);
static void delay_time_usec(unsigned long usecs);
static void delay_time_msec(unsigned long msecs);
static int out_nec(UINT8 b);
static void sense_interrupt_status(void);
static void set_density(int density);
static void reset_fdc(void);
static void specify(void);
static int detect_disk_in_drive(int physical_unit);
static int first_osd_get_fdc_status;

#define STM_DRIVE_0 		0x01
#define STM_DRIVE_1 		0x02
#define STM_DRIVE_2 		0x04
#define STM_DRIVE_3 		0x08
#define STM_IO_ACTIVE		0x10
#define STM_NON_DMA_MODE	0x20
#define STM_READ_FROM_FDC	0x40
#define STM_READY			0x80

#define ST0_UNIT_SELECT 	0x03
#define ST0_HEAD			0x04
#define ST0_NOT_READY		0x08
#define ST0_EQUIP_CHECK 	0x10
#define ST0_SEEK_END		0x20
#define ST0_INT_CODE		0xc0

#define ST1_MISSING_AM		0x01
#define ST1_NOT_WRITEABLE	0x02
#define ST1_NO_DATA 		0x04
#define ST1_UNUSED_3		0x08
#define ST1_OVERRUN 		0x10
#define ST1_DATA_ERROR		0x20
#define ST1_UNUSED_6		0x40
#define ST1_END_OF_CYLINDER 0x80

#define ST2_MISSING_DAM 	0x01
#define ST2_BAD_CYLINDER	0x02
#define ST2_SCAN_NOT_SATISF 0x04
#define ST2_SCAN_EQUAL_HIT	0x08
#define ST2_WRONG_CYLINDER	0x10
#define ST2_DATA_ERROR		0x20
#define ST2_CONTROL_MARK	0x40
#define ST2_UNUSED_7		0x80

#define ST3_UNIT_SELECT 	0x03
#define ST3_HEAD			0x04
#define ST3_TWO_SIDE		0x08
#define ST3_TRACK_0 		0x10
#define ST3_READY			0x20
#define ST3_WRITE_PROTECT	0x40
#define ST3_FAULT			0x80

struct NEC
{
	UINT8 unit; 		 /* unit number 0 = A:, 1 = B: */
	UINT8 type; 		 /* type of drive 0: 360K, 1: 720K, 2: 1.2M, 3: 1.44M */
	UINT8 eot;			 /* end of track */
	UINT8 secl; 		 /* sector length code (0:128, 1:256, 2:512 ... ) */
	UINT8 gap_a;		 /* read/write gap A*/
	UINT8 gap_b;		 /* format gap B */
	UINT8 dtl;			 /* data length (128 if secl == 0) */
	UINT8 filler;		 /* format filler UINT8 (E5) */
	UINT8 mfm;			 /* 0x00 = FM, 0x40 = MFM */
	UINT8 rcmd; 		 /* read command (0x26 normal, 0x2C deleted data address mark) */
	UINT8 wcmd; 		 /* write command (0x05 normal, 0x09 deleted data address mark) */
	UINT8 dstep;		 /* double step shift factor (1 for 40 track disk in 80 track drive) */
	UINT8 stm;			 /* main status */
	UINT8 results[16];	/* status file */
    int cur_track[4];
};

typedef struct {
	UINT8 density;
	UINT8 spt;
	UINT8 secl;
	UINT8 gap_a;
	UINT8 gap_b;
}	GAPS;

static GAPS gaps[] = {
	{DEN_FM_LO,    1, 4, 200, 255},
    {DEN_FM_LO,    2, 3, 200, 255},
    {DEN_FM_LO,    4, 2,  70, 135},
    {DEN_FM_LO,    5, 2,  56,  98},
    {DEN_FM_LO,    5, 1, 200, 255},
    {DEN_FM_LO,    6, 1,  70, 135},
    {DEN_FM_LO,    7, 1,  56,  98},
    {DEN_FM_LO,    8, 1,  25,  48},
    {DEN_FM_LO,    9, 1,   7,   9},
    {DEN_FM_LO,   16, 0,  16,  25},
    {DEN_FM_LO,   17, 0,  11,  17},
    {DEN_FM_LO,   18, 0,   7,   9},

    {DEN_FM_HI,    2, 4, 200, 255},
    {DEN_FM_HI,    3, 4,  42,  80},
    {DEN_FM_HI,    4, 3, 128, 240},
    {DEN_FM_HI,    5, 3,  70, 135},
    {DEN_FM_HI,    8, 2,  42,  80},
    {DEN_FM_HI,    9, 2,  42,  80},
    {DEN_FM_HI,   10, 2,  10,  12},
    {DEN_FM_HI,   16, 1,  32,  50},
    {DEN_FM_HI,   18, 1,  10,  12},
    {DEN_FM_HI,   26, 0,   7,   9},

	{DEN_MFM_LO,   1, 5, 200, 255},
    {DEN_MFM_LO,   2, 4, 200, 255},
    {DEN_MFM_LO,   3, 4,  26,  46},
    {DEN_MFM_LO,   4, 3, 128, 240},
    {DEN_MFM_LO,   5, 3,  21,  31},
    {DEN_MFM_LO,   8, 2,  42,  80},
    {DEN_MFM_LO,   9, 2,  26,  46},
    {DEN_MFM_LO,  10, 2,  10,  12},
    {DEN_MFM_LO,  16, 1,  32,  50},
    {DEN_MFM_LO,  17, 1,  21,  31},
    {DEN_MFM_LO,  18, 1,  10,  12},
    {DEN_MFM_LO,  19, 1,   7,   9},

	{DEN_MFM_HI,  2,  5, 200, 255},
    {DEN_MFM_HI,  4,  4, 200, 255},
    {DEN_MFM_HI,  5,  4,  42,  80},
    {DEN_MFM_HI,  8,  3, 128, 240},
    {DEN_MFM_HI,  9,  3,  76, 148},
    {DEN_MFM_HI, 10,  3,  32,  50},
    {DEN_MFM_HI, 11,  3,  10,  12},
    {DEN_MFM_HI, 16,  2, 128, 240},
    {DEN_MFM_HI, 17,  2,  70, 135},
    {DEN_MFM_HI, 18,  2,  42,  80},
    {DEN_MFM_HI, 19,  2,  32,  50},
    {DEN_MFM_HI, 32,  1, 128, 240},
    {DEN_MFM_HI, 33,  1,  42,  80},
    {DEN_MFM_HI, 34,  1,  32,  50},
    {DEN_MFM_HI, 35,  1,  21,  31},
    {DEN_MFM_HI, 36,  1,  10,  12},
	{0,0,0,0,0}
};

static struct NEC nec;
static UINT8 unit_type[4] = {0xff, 0xff, 0xff, 0xff};

static __dpmi_paddr old_irq_E;	/* previous interrupt vector 0E */
static __dpmi_paddr new_irq_E;	/* new interrupt vector 0E */
static _go32_dpmi_seginfo nec_dma; /* for allocation of a sector DMA buffer */
/* declared as volatile because I want this to be re-read each time it is referenced.
Some C compiler implementations could optimise it out and it would not be re-read each time */
static UINT8 volatile irq_flag = 0;		 /* set by IRQ 6 (INT 0E) */
static UINT8 initialized = 0;
static UINT8 last_dor = 0;
static UINT8 selected_density = DEN_MFM_LO;

/*****************************************************************************
 * initialize dma controller to read count bytes to ofs
 *****************************************************************************/
static void dma_read(int count)
{
	int ofs = nec_dma.rm_segment * 16 + nec_dma.rm_offset;

	LOG(("REAL NEC765  dma_read %08X %04X\n", ofs, count));
	outportb(0x0a, 0x04 | 0x02);		/* mask DMA channel 2 */
	outportb(0x0b, 0x46);				/* DMA read command */
	outportb(0x0c, 0);					/* reset first/last flipflop */
	outportb(0x05, (count - 1) & 0xff); /* transfer size low */
	outportb(0x05, (count - 1) >> 8);	/* transfer size high */
	outportb(0x04, ofs & 0xff); 		/* transfer offset low */
	outportb(0x04, (ofs >> 8) & 0xff);	/* transfer offset high */
	outportb(0x81, (ofs >> 16) & 0xff); /* select 64k page */
	outportb(0x0a, 0x02);				/* start DMA channel 2 */
}

/*****************************************************************************
 * initialize dma controller to read count bytes to ofs
 *****************************************************************************/
static void dma_write(int count)
{
	int ofs = nec_dma.rm_segment * 16 + nec_dma.rm_offset;

	LOG(("REAL NEC765  dma_write %08X %04X\n", ofs, count));
	outportb(0x0a, 0x04 | 0x02);		/* mask DMA channel 2 */
	outportb(0x0b, 0x4a);				/* DMA write command */
	outportb(0x0c, 0);					/* reset first/last flipflop */
	outportb(0x05, (count - 1) & 0xff); /* transfer size low */
	outportb(0x05, (count - 1) >> 8);	/* transfer size high */
	outportb(0x04, ofs & 0xff); 		/* transfer offset low */
	outportb(0x04, (ofs >> 8) & 0xff);	/* transfer offset high */
	outportb(0x81, (ofs >> 16) & 0xff); /* select 64k page */
	outportb(0x0a, 0x02);				/* start DMA channel 2 */
}

/*****************************************************************************
 * read NECs main status
 *****************************************************************************/
static int main_status(void)
{
	uclock_t timeout;
	uclock_t utime = uclock();

	timeout = utime + UCLOCKS_PER_SEC;
	while (utime < timeout)
	{
		nec.stm = inportb(0x3f4);
		if (nec.stm & STM_READY)
			break;
		utime = uclock();
	}

	timeout -= uclock();

	if (timeout <= 0)
	{
		LOG(("REAL NEC765  STM: %02X %s%s%s%s%s%s%s%s\n",
			nec.stm,
			nec.stm & READY ? " READY" : "",
			nec.stm & READ_FROM_FDC ? " READ_FROM_FDC" : " WRITE_TO_FDC",
			nec.stm & IO_ACTIVE ? " IO_ACTIVE" : "",
			nec.stm & NON_DMA_MODE ? " NON_DMA_MODE" : "",
			nec.stm & DRIVE_0 ? " A" : "",
			nec.stm & DRIVE_1 ? " B" : "",
			nec.stm & DRIVE_2 ? " C" : "",
			nec.stm & DRIVE_3 ? " D" : ""));
	}
	return (timeout > 0);
}

/*****************************************************************************
 * NECs results phase: read everything into the status registers
 *****************************************************************************/
static void results(int sense_interrupt)
{
	int result_count = 0;
	
	do
	{
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x03f5);
			logerror("REAL NEC765 RESULT: %04x\n", nec.results[result_count]);
			result_count++;
		}
	}
	while ((nec.stm & STM_READ_FROM_FDC));
#if 0
	

    if (sense_interrupt)
	{
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765  ST3: 0x%02X unit select:%d head:%d%s%s%s%s%s\n",
				nec.results[result_count],
				nec.results[result_count] & ST3_UNIT_SELECT,
				nec.results[result_count] & ST3_HEAD ? 1 : 0,
				nec.results[result_count] & ST3_TWO_SIDE ? " TWO_SIDE" : "",
				nec.results[result_count] & ST3_TRACK_0 ? " TRACK_0" : "",
				nec.results[result_count] & ST3_READY ? " READY" : "",
				nec.results[result_count] & ST3_WRITE_PROTECT ? " WRITE_PROTECT" : "",
				nec.results[result_count] & ST3_FAULT ? " FAULT" : ""));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765  PCN: %02d\n", nec.results[result_count]));
			result_count++;
        }
	}
	else
	{
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765  ST0: 0x%02X unit_select:%d head:%d interrupt code:%d%s%s%s\n",
				nec.results[result_count],
				nec.results[result_count] & ST0_UNIT_SELECT,
				nec.results[result_count] & ST0_HEAD ? 1 : 0,
				(nec.results[result_count] & ST0_INT_CODE) >> 6,
				nec.results[result_count] & ST0_NOT_READY ? " NOT_READY" : "",
				nec.results[result_count] & ST0_EQUIP_CHECK ? " EQUIP_CHECK" : "",
				nec.results[result_count] & ST0_SEEK_END ? " SEEK_END" : ""));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765  ST1: 0x%02X%s%s%s%s%s%s\n",
				nec.results[result_count],
				nec.results[result_count] & ST1_MISSING_AM ? " MISSING_AM" : "",
				nec.results[result_count] & ST1_NOT_WRITEABLE ? " NOT_WRITEABLE" : "",
				nec.results[result_count] & ST1_NO_DATA ? " NO_DATA" : "",
				nec.results[result_count] & ST1_OVERRUN ? " OVERRUN" : "",
				nec.results[result_count] & ST1_DATA_ERROR ? " DATA_ERROR" : "",
				nec.results[result_count] & ST1_END_OF_CYLINDER ? " END_OF_CYLINDER" : ""));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765  ST2: 0x%02X %s%s%s%s%s%s%s\n",
				nec.results[result_count],
				nec.results[result_count] & ST2_MISSING_DAM ? " MISSING_DAM" : "",
				nec.results[result_count] & ST2_BAD_CYLINDER ? " BAD_CYLINDER" : "",
				nec.results[result_count] & ST2_SCAN_NOT_SATISFIED ? " SCAN_NOT_SATISFIED" : "",
				nec.results[result_count] & ST2_SCAN_EQUAL_HIT ? " SCAN_EQUAL_HIT" : "",
				nec.results[result_count] & ST2_WRONG_CYLINDER ? " WRONG_CYLINDER" : "",
				nec.results[result_count] & ST2_DATA_ERROR ? " DATA_ERROR" : "",
				nec.results[result_count] & ST2_CONTROL_MARK ? " CONTROL_MARK" : ""));
			result_count++;
        }

		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765       C:%02d\n", nec.results[result_count]));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765       H:%d", nec.results[result_count]));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765       S:%02d", nec.results[result_count]));
			result_count++;
        }
		if (main_status() && (nec.stm & STM_READ_FROM_FDC))
		{
			nec.results[result_count] = inportb(0x3F5);
			LOG(("REAL NEC765       N:%2d", nec.results[result_count]));
			result_count++;
        }
	}

	/* more data? shouldn't happen, but read it away */
	while (main_status() && (nec.stm & STM_READ_FROM_FDC))
	{
		nec.results[result_count] = inportb(0x3F5);
		LOG(("REAL NEC765  read unexpected: %02X\n", nec.results[result_count]));
		if (result_count < 16)
			result_count++;
	}
#endif
}

static void sense_interrupt_status(void)
{
    if (out_nec(0x08))      /* sense interrupt status */
		return;

	results(1); 			/* get sense interrupt status results */
}

static void select_drive(int drive_index, int motor_on)
{
	UINT8 data;

	/* fdc enabled and dma enabled */
	data = PC_DOR_FDC_ENABLED | PC_DOR_DMA_ENABLED;
	data |= (drive_index & 0x03);

	if (motor_on)
	{
		data |= (1<<(4+drive_index));
	}
	/* digital output register */
	outportb(fdc_hw_base + FDC_DIGITAL_OUTPUT_REGISTER_OFFSET, data);

	/* if the drive motor is on and drive has changed, wait for
	the drive motor to spin-up */
	if (motor_on)
	{
		/* did drive change? */
		if (((last_dor ^ data) & 0x03)!=0)
		{
			delay_time_msec(1000);
		}
	}

	last_dor = data;
}



/*****************************************************************************
 * delay time in microseconds without timeout
 *****************************************************************************/
static void delay_time_usec(unsigned long usecs)
{
	uclock_t start_time;
	uclock_t current_time;

    start_time = uclock();
    current_time = start_time;

	while ((current_time - start_time) < usecs)
    {
        current_time = uclock();
    }
}

/*****************************************************************************
 * delay time in milliseconds without timeout
 *****************************************************************************/
static void delay_time_msec(unsigned long msecs)
{
    delay_time_usec(msecs*1000);
}


/*****************************************************************************
 * reset fdc - used after recalibrate timeout or other operation where
 * fdc is in an infinite loop
 *****************************************************************************/
static void reset_fdc(void)
{
	UINT16 port;

	last_dor = 0;

	port = fdc_hw_base + FDC_DIGITAL_OUTPUT_REGISTER_OFFSET;

    outportb(port, PC_DOR_DMA_ENABLED);      /* reset fdc */
    /* minimum reset duration is 100ns */
	/* this is overkill, but guarantees we wait long enough */
	delay_time_usec(1);
	outportb(port, PC_DOR_DMA_ENABLED | PC_DOR_FDC_ENABLED);

	/* issue a sense interrupt status - in case a interrupt was caused */
	sense_interrupt_status();

	/* issue specify */
	specify();

	/* re-set data rate */
	set_density(selected_density);
}



/*****************************************************************************
 * out_nec: send a UINT8 to the NEC
 *****************************************************************************/
static int out_nec(UINT8 b)
{
	int result = 1;

	if (main_status())
	{
		if (nec.stm & STM_READ_FROM_FDC)
		{
			LOG(("REAL NEC765: read_from_fdc!\n"));
			results(0);
		}
		else
		{
			LOG(("REAL NEC765: to fdc 0x%02X\n", b));
			outportb(0x3f5, b);
		
			/* this might not be necessary. On the Amstrad CPC, there is always a short delay
			after each command byte is written to allow the nec765 to process the data.
			On modern implementations this may not be necessary. I am including this "just in
			case". If this is required, then we cannot use a software delay because this
			will not work on faster cpus */
	//		delay_time_usec(27);

			result = 0;
		}
	}
	return result;
}

/*****************************************************************************
 * seek_exec: wait until seek is executed and sense interrupt status
 *****************************************************************************/
static void seek_exec(void)
{
	uclock_t timeout;
	uclock_t utime = uclock();

	timeout = utime + UCLOCKS_PER_SEC * 5;

	while (utime < timeout)
	{
		main_status();
		if (irq_flag)
			break;
		utime = uclock();
	}

	timeout -= uclock();

	if (nec.stm & STM_READ_FROM_FDC)
		results(0);

    sense_interrupt_status();
}

/*****************************************************************************
 * wait_int_rw: wait until a interrupt occurs or a timeout - used for read/write operations
 *****************************************************************************/

/* speed disc is rotating at when disc is spinning at full-speed */
#define DISK_ROTATION_SPEED_RPM 300
/* time for a single rotation in uclocks */
#define DISK_ROTATION_IN_UCLOCKS (UCLOCKS_PER_SEC/300)
/* time-out, interrupt not received */
#define WAIT_INT_FLAGS_TIMEOUT 0x00
/* interrupt received */
#define WAIT_INT_FLAGS_SUCCESS 0x01

/* wait for floppy disc controller interrupt, or timeout
after 3 disc revolutions. This function assumes floppy
disc device is present.
If a timeout occurs, the nec765 will require a reset to recover */
static int wait_int(void)
{
	uclock_t timeout;
	uclock_t utime;
    uclock_t start_time;

	/* after a read/write command has been issued, if the head is not loaded,
	the head is loaded and the fdc will wait the specified head settling time
	defined in the specify command before starting the read. */
	
	/* wait head settling time */
	delay_time_usec(head_load_time_usec);

	/* When MESS is running, 3 disc rotations is not enough for timeout!!!!!!! */

	/* now wait for the interrupt */
	utime = uclock();
    start_time = utime;
	/* a timeout value of 3 disc rotations is enough - if a track is not formatted,
	the nec765 will fail the command after 2 index pulses have been detected */
	timeout = (DISK_ROTATION_IN_UCLOCKS*30);
    while ((utime-start_time) < timeout)
	{
		if (irq_flag)
        {
            logerror("REAL NEC765 wait_int: caught int\n");
            return WAIT_INT_FLAGS_SUCCESS;
        }
        utime = uclock();
    }
 
	logerror("REAL NEC765 wait_int: timeout\n");
    return WAIT_INT_FLAGS_TIMEOUT;
}

/*****************************************************************************
 * specify drive parameters
 * step rate, head unload timing, head load timing, DMA mode
 *****************************************************************************/
static void specify(void)
{
	logerror("REAL NEC765  command: specify\n");

    
	if (out_nec(0x03))          /* specify command */
		return;
	if (out_nec(((step_rate & 0x0f)<<4) | (head_unload_time & 0x0f)))			/* steprate, head unload timing */
		return;
	if (out_nec((head_load_time & 0x07f)<<1))			/* head load timing, DMA mode */
		return;

}

/*****************************************************************************
 * read_sector:
 * read a sector number 'sector' from track 'track', side 'side' using head
 * number 'head' of the floppy disk. if the first try fails, change the
 * DDAM mode and try again
 *****************************************************************************/
static void read_sector(int unit, int side, int C, int H, int R, int N, int ddam)
{
//	int try;

	LOG(("REAL NEC765 read    unit:%d track:%02d side:%d head:%d sector:%d%s\n", unit, C, H, R, N, ddam ? " *DDAM*":""));

	logerror("read sector\n");
    select_drive(unit, 1);

	/* three tries to detect normal / deleted data address mark */
//	for (try = 0; try < (ddam ? 3 : 1); try++)
	{
        nec.dtl = 0x0ff;
        nec.secl = (1<<(N+7));
        nec.gap_a = 0x02a;
        dma_read((nec.secl) ? nec.secl << 8 : nec.dtl);

		irq_flag = 0;

		/* do standard read operation - if a deleted data sector is found, it will
		be read, but the control mark bit will be set! */
        if (out_nec(nec.mfm | 0x06))        /* read sector */
			return;
        if (out_nec((side << 2) + unit))    /* side * 4 + unit */
			return;
		if (out_nec(C)) 					/* track */
			return;
		if (out_nec(H)) 					/* head (can be 0 for side 1!) */
			return;
		if (out_nec(R)) 					/* sector */
			return;
		if (out_nec(N)) 					/* bytes per sector */
			return;
		if (out_nec(R))
			return;
//		if (out_nec(nec.eot))				/* end of track */
//			return;
		if (out_nec(nec.gap_a)) 			/* gap A */
			return;
		if (out_nec(nec.dtl))				/* data length */
			return;

		logerror("reading..\n");

		for ( ; ; )
        {
			main_status();
			if (irq_flag)
				break;
			if (!(nec.stm & STM_IO_ACTIVE))
				break;
        }

		results(0);

//		if (ddam && !(nec.results[2] & ST2_CONTROL_MARK))
//			break;
//
  //      /* if control mark is set, toggle DDAM mode and try again */
	//	if (ddam)
	//		nec.rcmd ^= 0x0a;	   /* toggle between 0x26 and 0x2a */
	}

//	if (ddam)
//	{
//		nec.results[2] &= ~ST2_CONTROL_MARK;
//		/* set status 2 control_mark if read deleted was successful */
//		if (nec.rcmd & 0x08)
//		{
//			LOG(("REAL NEC765 *DDAM* found\n"));
//			nec.results[2] |= ST2_CONTROL_MARK;
//			nec.rcmd ^= 0x0a;	   /* toggle back to non DDMA read */
//		}
//	}
}

/*****************************************************************************
 * write_sector:
 * write sector number 'sector' to track 'track', side 'side' of
 * the floppy disk using head number 'head'. if ddam in non zero,
 * write the sector with deleted data address mark (DDAM)
 *****************************************************************************/
static void write_sector(int unit, int side, int C, int H, int R, int N, int ddam)
{
	int i;

	LOG(("REAL NEC765 write unit:%d track:%02d side:%d head:%d sector:%d%s\n", unit,C, H, R, N, (ddam) ? " *DDAM*" : ""));

	irq_flag = 0;

	dma_write((nec.secl) ? nec.secl << 8 : nec.dtl);
	nec.wcmd = (ddam) ? 0x09 : 0x05;

	LOG(("REAL NEC765  command: write sector\n"));
	if (out_nec(nec.mfm | nec.wcmd))	/* write sector */
		return;
    if (out_nec((side << 2) + unit))    /* side * 4 + unit */
		return;
	if (out_nec(C)) 					/* track */
		return;
	if (out_nec(H)) 					/* head (can be 0 for side 1!) */
		return;
	if (out_nec(R)) 					/* sector */
		return;
	if (out_nec(N)) 					/* bytes per sector */
		return;
	if (out_nec(N)) 					/* end of track */
		return;
	if (out_nec(nec.gap_a)) 			/* gap_a */
		return;
	if (out_nec(nec.dtl))				/* data length */
		return;

	for (i = 0; i < 3; i++)
	{
		main_status();
		if (!(nec.stm & STM_IO_ACTIVE))
			break;
		if (irq_flag)
			break;
	}

	results(0);
}

/*****************************************************************************
 * real mode interrupt 0x0E
 *****************************************************************************/
static void irq_E(void)
{
	asm("nop                    \n" /* "nop" to find interrupt entry */
		"sti                    \n" /* enable interrupts */
		"pushl %eax             \n" /* save EAX */
		"pushw %ds              \n" /* save DS */
		"movw  $0x0000, %ax     \n" /* patched with _my_ds() */
		"movw  %ax, %ds         \n" /* transfer to DS */
		"orb   $0x80, _irq_flag \n" /* set IRQ occured flag */
		"popw  %ds              \n" /* get back DS */
		"movb  $0x20, %al       \n" /* end of interrupt */
		"outb  %al, $0x20       \n" /* to PIC */
		"popl  %eax             \n" /* get back EAX */
		"iret                   \n");
}

int osd_fdc_init(void)
{
	__dpmi_meminfo meminfo;
	unsigned long _my_cs_base;
	unsigned long _my_ds_base;
	UINT8 *p;
    int i;

	if (initialized)
		return 1;

	for (p = (UINT8 *) & irq_E; p; p++)
	{
		/* found "nop" ? */
        if (p[0] == 0x90)
			new_irq_E.offset32 = (int) p + 1;
		/* found "movw $0000,%ax" ? */
		if ((p[0] == 0xb8) && (p[1] == 0x00) && (p[2] == 0x00))
		{
			p++;
			*(short *)p = _my_ds();
			break;
		}
	}

    __dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
	__dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
	__dpmi_get_protected_mode_interrupt_vector(0x0E, &old_irq_E);

	meminfo.handle = _my_cs();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_cs_base + (unsigned long) &irq_E;
	if (__dpmi_lock_linear_region(&meminfo))
	{
		LOG(("REAL NEC765 init: could not lock code %lx addr:%08lx size:%ld\n",
					meminfo.handle, meminfo.address, meminfo.size));
		return 0;
	}

	meminfo.handle = _my_ds();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_ds_base + (unsigned long) &old_irq_E;
	if (__dpmi_lock_linear_region(&meminfo))
	{
		LOG(("REAL NEC765 init: could not lock data\n"));
		return 0;
	}

	nec_dma.size = 4096;		/* request a 4K buffer for NEC DMA */
	if (_go32_dpmi_allocate_dos_memory(&nec_dma))
	{
		LOG(("REAL NEC765 init: could not alloc DOS memory size:%ld\n", nec_dma.size));
		return 0;
	}

    new_irq_E.selector = _my_cs();
	if (__dpmi_set_protected_mode_interrupt_vector(0x0E, &new_irq_E))
	{
		LOG(("REAL NEC765 init: could not set vector 0x0E"));
		return 0;
	}

    initialized = 1;

	atexit(osd_fdc_exit);
	nec.unit = 0;
	
	reset_fdc();

    /* recalibrate all drives, so that we know where the disc head is
    for signed seeks */
    for (i=0; i<2; i++)
    {
        if (detect_floppy_drive(i))
		{
			logerror("drive %04x exists!\r\n", i);
		}
		else
		{
			logerror("drive %04x not detected!\r\n", i);
		}

        nec.cur_track[i] = 0;
    }
	set_density(DEN_MFM_LO);

	first_osd_get_fdc_status = 1;


    return 1;
}

void osd_fdc_exit(void)
{
	__dpmi_meminfo meminfo;
	unsigned long _my_cs_base;
	unsigned long _my_ds_base;

	if (!initialized)
		return;

	outportb(0x3f2, 0x0c);

	__dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
	__dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
	__dpmi_set_protected_mode_interrupt_vector(0x0E, &old_irq_E);

	meminfo.handle = _my_cs();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_cs_base + (unsigned long) &irq_E;
	__dpmi_unlock_linear_region(&meminfo);

	meminfo.handle = _my_ds();
	meminfo.size = 128;		   /* should be enough */
	meminfo.address = _my_ds_base + (unsigned long) &old_irq_E;
	__dpmi_unlock_linear_region(&meminfo);

	if (nec_dma.rm_segment)
	   _go32_dpmi_free_dos_memory(&nec_dma);

	initialized = 0;
}

void osd_fdc_motors(int unit, int motor_status)
{
	nec.unit = unit;
	select_drive(unit, motor_status);
}


static void set_density(int density)
{
	int data_rate_value = 2;
	float step_rate_multiplier = 1.0;
	float head_unload_multiplier = 1.0;
	float head_load_multiplier = 1.0;

	switch (density)
	{
		case DEN_FM_LO:
			nec.mfm = MFM_OVERRIDE; /* FM commands */
			data_rate_value = 0x02; /* how do we set 125 kbps ?? */
			break;
		case DEN_FM_HI:
			nec.mfm = MFM_OVERRIDE; /* FM commands */
			data_rate_value = 0x02;	/* set 250/300 kbps */
            break;
		case DEN_MFM_LO:
			nec.mfm = 0x40; 		/* MFM commands */
			data_rate_value = 0x02;	/* set 250/300 kbps */
			break;
		case DEN_MFM_HI:
			nec.mfm = 0x40; 		/* MFM commands */
			data_rate_value = 0x01; /* set 500 kbps */
            break;
		default:
			break;
    }

	selected_density = density;

	/* select data rate */
	outportb(fdc_hw_base + FDC_DATA_RATE_REGISTER_OFFSET, data_rate_value);	

	/* calculate result time in usec */

	/* on the smc37c78 there is a drive rate selection. This assumes it is set to 0 */

	/* data rate:
		0 = 500 kbps 
		1 = 300 kbps 
		2 = 250 kbps 
		3 = 1000 kbps
	*/

	switch (data_rate_value)
	{
		case 0:
		{
			step_rate_multiplier = 1;
			head_unload_multiplier = 16;
			head_load_multiplier = 2;
		}
		break;

		case 1:
		{
			step_rate_multiplier = 1.67;
			head_unload_multiplier = 26.7;
			head_load_multiplier = 3.3;
		}
		break;

		case 2:
		{
			step_rate_multiplier = 2;
			head_unload_multiplier = 32;
			head_load_multiplier = 4;
		}
		break;

		case 3:
		{
			step_rate_multiplier = 0.5;
			head_unload_multiplier = 8;
			head_load_multiplier = 1;
		}
		break;

		default:
			break;
	}

	step_rate_usec = ((16-step_rate)*step_rate_multiplier)*1000;
	head_load_time_usec = ((128 - head_load_time)*head_load_multiplier)*1000;
	head_unload_time_usec = ((16 - head_unload_time)*head_unload_multiplier)*1000;
}

/*****************************************************************************
 * specify density, sectors per track and sector length
 *****************************************************************************/
void osd_fdc_density(int unit, int density, int tracks, int spt, int eot, int secl)
{
	__dpmi_regs r = {{0,}, };
	int i;

	nec.unit = unit;
    nec.eot = eot;
	nec.secl = secl;
	nec.dstep = 0;

	/* BIOS get current drive parameters */
    r.x.ax = 0x0800;
	r.h.dl = nec.unit;
	__dpmi_int(0x13, &r);
	nec.type = r.h.bl;

	if (nec.type != unit_type[nec.unit])
    {
		switch (nec.type)
		{
			case 1: /* 5.25" 360K */
				LOG(("REAL NEC765 FDD %c: type 5.25\" 360K, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1701;
				break;
			case 2: /* 5.25" 1.2M */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG(("REAL NEC765 FDD %c: type 5.25\" 1.2M, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1703;
				break;
			case 3: /* 3.5"  720K */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG(("REAL NEC765 FDD %c: type 3.5\" 720K, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1704;
				break;
			case 4: /* 3.5"  1.44M */
				if (tracks < 50)
                    nec.dstep = 1;
				LOG(("REAL NEC765 FDD %c: type 3.5\" 1.44M, sides %d, DOS spt %d%s\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f, (nec.dstep) ? " (double steps)":""));
				r.x.ax = 0x1703;
				break;
			default: /* unknown */
				LOG(("REAL NEC765 FDD %c: unknown type! sides %d, DOS spt %d\n",
						'A' + nec.unit, r.h.dh + 1, r.h.cl & 0x1f));
				r.x.ax = 0x1703;
                break;
        }

		unit_type[nec.unit] = nec.type;
		__dpmi_int(0x13, &r);

		for (i = 0; gaps[i].spt; i++)
		{
			if (gaps[i].density == density &&
				gaps[i].secl == secl &&
				spt <= gaps[i].spt)
			{
				nec.gap_a = gaps[i].gap_a;
				nec.gap_b = gaps[i].gap_b;
				break;
			}
		}
		if (gaps[i].spt)
		{
			LOG(("REAL NEC765  using gaps entry #%d {%d,%d,%d,%d,%d} for %d,%d,%d\n",
                i,gaps[i].density,gaps[i].spt,gaps[i].secl,gaps[i].gap_a,gaps[i].gap_b,density,spt,secl));
        }
		else
		{
			LOG(("REAL NEC765  ERR den:%d spt:%d secl:%d not found!\n",density,spt,secl));
            nec.gap_a = 10;
			nec.gap_b = 12;
		}
		nec.dtl = (secl)?0xff:0x80; /* data length */
		nec.filler = 0xe5;			/* format filler UINT8 */

		LOG(("REAL NEC765  FDD %c: den:%d trks:%d spt:%d eot:%d secl:%d gap_a:%d gap_b:%d\n",
			'A' + nec.unit, density, tracks, spt, eot, secl, nec.gap_a, nec.gap_b));

        nec.rcmd = 0x26;    /* read command (0x26 normal, 0x2C deleted data address mark) */
		nec.wcmd = 0x05;	/* write command (0x05 normal, 0x09 deleted data address mark) */
    }

	set_density(density);
}

/*****************************************************************************
 * detect if a floppy drive is present
 *****************************************************************************/

 /* This code assumes that the floppy disc controller is present.
	This code assumes that the disc drive has no more than 80 tracks.

	1. The pc disk drives are always ready so a recalibrate command will succeed.
	2. Recalibrate will seek to track 0. It will fail after 77 or 79 or 80 tracks depending
	on the nec765 implementation.
	3. Recalibrate will not cause the nec765 to execute an endless loop, and therefore doesn't require
	a timeout.
	
	To detect if a floppy drive is present we do the following:

	1. Issue a recalibrate command.
		If this succeeds (track 0 is detected) the drive is present.
		If it fails then it is possible the drive is present, but not enough tracks were
		seeked.
	2. Issue a second recalibrate command.
		If this succeeds (track 0 is detected) the drive is present.
		If it fails the drive is not present.
*/
static int detect_floppy_drive(int unit)
{
	/* select drive and turn on motors */
	select_drive(unit, 1);

	/* clear irq flag */
	irq_flag = 0;

	/* issue 1st recalibrate command */
	logerror("REAL NEC765: detect floppy drive: 1st recalibrate\n");
	if (out_nec(0x07))                  /* 1st recal command */
		return 0;
	if (out_nec(unit))
		return 0;

	
	/* wait for irq to change */
	while (irq_flag==0);
	
	/* issue a sense interrupt status command */
	sense_interrupt_status();

	/* if command completed successfully and seek end was set, the drive exists */
	if (((nec.results[0] & 0x0c0)==0) && ((nec.results[0] & ST0_SEEK_END)!=0))
		return 1;

	/* the drive could exist, but this implementation of the nec765 didn't move the
	drive head enough */

	/* clear irq flag */
	irq_flag = 0;

	/* issue 2nd recalibrate command */
	logerror("REAL NEC765: detect floppy drive: 2nd recalibrate\n");
	if (out_nec(0x07))                  /* 1st recal command */
		return 0;
	if (out_nec(unit))
		return 0;

	/* wait for irq to change */
	while (irq_flag==0);

	/* issue a sense interrupt status command */
	sense_interrupt_status();

	/* if command completed successfully and seek end was set, the drive exists */
	if (((nec.results[0] & 0x0c0)==0) && ((nec.results[0] & ST0_SEEK_END)!=0))
		return 1;

	/* this may not be necessary, checking just in case */
	if (
		((nec.results[0] & (ST0_EQUIP_CHECK | ST0_SEEK_END))==(ST0_EQUIP_CHECK | ST0_SEEK_END)) &&
		((nec.results[0] & 0x0c0)!=0)
		
		)
		return 0;

	return 0;
}


/*****************************************************************************
 * detect if a disk is in the drive
 *****************************************************************************/

/* 
	The following assumes that the nec765 compatible floppy disc controller
	is present.

	1. On modern PC's, the floppy drives are always ready.
	2. If a disk is present, index pulses will be detected. 
	3. A interrupt is generated when the result phase of the command has
	begun.

	a) DISC NOT IN DRIVE
	
	If a read id is done, and the disc is not in the drive,
	no index pulses will be detected. The NEC765 will wait forever
	for index pulses to occur. If there are no index pulses, a interrupt
	will not be generated, and the command will not end.
	
	The command should therefore timeout after at least 3 revolutions of the disc.
	(The fdc looks for 2 index pulses before it stops looking for id marks, so
	we cannot timeout before then)

    The NEC765 must be reset if a timeout occurs.
	
	b) DISC IN DRIVE BUT TRACK IS NOT FORMATTED

	If a read id is done, and the disc is in the drive, but the track is not
	formatted the following will occur:

	The nec765 will detect the index pulse twice before a id can be found.
	The command will end and a interrupt will be generated.
	In the result data it will report "NO DATA".
	
	c) DISC IN DRIVE AND TRACK IS FORMATTED

	If a read id is done, and the disc is in the drive and the track is
	formatted, the following will occur:

	The nec765 will detect a id mark, the command will end and a interrupt
	will be generated. The result data will report the sector id.
*/

static int detect_disk_in_drive(int physical_unit)
{
    select_drive(physical_unit,1);
	
	irq_flag = 0;
    if (out_nec(nec.mfm | 0x0a))       /* read id */
       return 0;
    if (out_nec(physical_unit))
       return 0;

	/* wait for int */
    if (wait_int())
	{
		/* caught int - read result */
		results(0);
		return 1;
	}

	/* timeout before int, reset fdc */
	reset_fdc();
	return 0;
}


/*****************************************************************************
 * osd_fdc_get_status
 *
 * get status of specified drive
 * this will update the status a minimum of 4 seconds between status checks.
 *****************************************************************************/
static int do_status(int unit)
{
	int status;

	select_drive(unit, 0);

	status = 0;
	
	/* detect if disk is in the drive */
	if (detect_disk_in_drive(unit))
	{
		status |= FLOPPY_DRIVE_DISK_INSERTED;
	}

	if (out_nec(0x04))					/* sense drive status */
		return status;
	if (out_nec(unit))
		return status;

	results(0);

	if (nec.results[0] & ST3_WRITE_PROTECT)
		status |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	if (nec.results[0] & ST3_TRACK_0)
		status |= FLOPPY_DRIVE_HEAD_AT_TRACK_0;
	if (nec.results[0] & ST3_READY)
		status |= FLOPPY_DRIVE_READY;

	return status;
}

static int fdd_status;
static uclock_t	uclock_of_last_status;

/* assumes drive is actually present for now */
/* this will detect the status of a real drive every 4 seconds */
int osd_fdc_get_status(int unit)
{
	uclock_t uclock_of_this_status;

	/* get current uclock */
	uclock_of_this_status = uclock();

	/* if this is the first time into the call, record current uclock */
	if (first_osd_get_fdc_status)
	{
		uclock_of_last_status = uclock_of_this_status;
		fdd_status = do_status(unit);
		first_osd_get_fdc_status = 0;
	}

	/* at least 4 seconds must pass between status checks */
	if ((uclock_of_this_status - uclock_of_last_status)<(UCLOCKS_PER_SEC*4))
		return fdd_status;

	fdd_status = do_status(unit);
	
	uclock_of_last_status = uclock_of_this_status;

	return fdd_status;
}


/* seek signed tracks from current position */
void osd_fdc_seek(int unit, int signed_tracks)
{
	if (signed_tracks == 0)
        return;

    select_drive(unit, 1);
    delay_time_msec(1000);

    /* calc dest track */
    nec.cur_track[unit] = nec.cur_track[unit] + signed_tracks;

    /* ensure track is valid. we do not want to break the drive! */
    if (nec.cur_track[unit]<0)
        nec.cur_track[unit] = 0;
    if (nec.cur_track[unit]>82)
        nec.cur_track[unit] = 82;

    irq_flag = 0;

	LOG(("REAL NEC765  command: seek unit:%d track:%02d\n", unit, nec.cur_track[unit]));
	if (out_nec(0x0f))		/* seek command */
		return;
	if (out_nec(unit))		/* unit number */
		return;
    if (out_nec(nec.cur_track[unit] << nec.dstep))        /* track number */
		return;
    seek_exec();
}

void osd_fdc_format(int t, int h, int spt, UINT8 * fmt)
{
	int i;

	LOG(("REAL NEC765 format  unit:%d track:%02d head:%d sec/track:%d\n", nec.unit, t, h, spt));

#if RE_INTERLEAVE
	/* reorder the sector sequence for PC optimized reading (interleave) */
	{
		int sec, sec_min = 255, sec_max = 0;

        for (i = 0; i < spt; i++)
		{
			sec = fmt[i * 4 + 2];
			if (sec < sec_min)
				sec_min = sec;
			if (sec > sec_max)
				sec_max = sec;
		}
		/* only reorder if all sectors from min to max are in the track! */
		if (sec_max - sec_min + 1 == spt)
		{
			LOG(("REAL NEC765 reordered sectors:"));
            sec = sec_min;
			for (i = 0; i < spt; i++)
			{
				LOG((" %3d", sec));
                fmt[i * 4 + 2] = sec;
				sec += spt / RE_INTERLEAVE;
				if (sec > sec_max)
					sec = sec - spt + 1 - spt % RE_INTERLEAVE;
			}
			LOG(("\n"));
        }
	}
#endif

    nec.eot = 0;
	for (i = 0; i < spt; i++)
	{
		if (fmt[i * 4 + 2] > nec.eot)
			nec.eot = fmt[i * 4 + 2];
	}

	movedata(_my_ds(), (unsigned) fmt, nec_dma.pm_selector, nec_dma.pm_offset, spt * 4);

	irq_flag = 0;

	dma_write(spt * 4);

	LOG(("REAL NEC765  command: format track\n"));
	if (out_nec(nec.mfm | 0x0d))	   /* format track */
		return;
	if (out_nec((h << 2) + nec.unit))	/* head * 4 + unit select */
		return;
	if (out_nec(nec.secl))				/* bytes per sector */
		return;
	if (out_nec(spt))					/* sectors / track */
		return;
	if (out_nec(nec.gap_b)) 			/* gap_b */
		return;
	if (out_nec(nec.filler))			/* format filler byte */
		return;

	for (i = 0; i < 3; i++)
	{
		main_status();
		if (!(nec.stm & STM_IO_ACTIVE))
			break;
		if (irq_flag)
			break;
	}

	results(0);
}

void osd_fdc_put_sector(int unit, int side, int C, int H, int R, int N, UINT8 *buff, int ddam)
{
	LOG(("write sector %02x %02x %02x %02x", C, H, R, N));
    movedata(
		_my_ds(), (unsigned) buff,
		nec_dma.pm_selector, nec_dma.pm_offset,
		((nec.secl) ? nec.secl << 8 : nec.dtl));
        write_sector(unit, side, C, H, R, N, ddam);
}

void osd_fdc_get_sector(int unit, int side, int C, int H, int R, int N, UINT8 *buff, int ddam)
{
	LOG(("read sector %02x %02x %02x %02x", C, H, R, N));

	read_sector(unit, side, C, H, R, N, ddam);
	movedata(
		nec_dma.pm_selector, nec_dma.pm_offset,
		_my_ds(), (unsigned) buff,
		((nec.secl) ? nec.secl << 8 : nec.dtl));
}


  
void osd_fdc_read_id(int physical_unit, int physical_side, unsigned char *pBuffer)
{
    select_drive(physical_unit,1);
	
	irq_flag = 0;
    if (out_nec(nec.mfm | 0x0a))       /* read id */
       return;
    if (out_nec(physical_unit | (physical_side<<2)))
       return;

	/* wait for int */
    if (wait_int())
	{
		/* caught int - read result */
		results(0);
		pBuffer[0] = nec.results[3];
		pBuffer[1] = nec.results[4];
		pBuffer[2] = nec.results[5];
		pBuffer[3] = nec.results[6];
	
		logerror("C: %04x H: %04x R: %04x N: %04x\n", nec.results[3], nec.results[4], nec.results[5], nec.results[6]);

		return;
	}

	/* timeout before int, reset fdc */
	reset_fdc();
}

#if 0
int main(int argc, char *argv[])
{
    osd_fdc_init();

	select_drive(0, 1);
    delay_time_msec(1000);

    while (1==1)
    {
//		osd_fdc_read_id(0,0);

//      fprintf(stdout,"C: %02x H: %02x R: %02x N: %02x\n", nec.results[3], nec.results[4], nec.results[5],nec.results[6]);

        fprintf(stdout,"%02x\n", inportb(0x03f7) & 0x080);

//		irq_flag = 0;
//		outportb(0x3f2, 0x18);		/* reset fdc */
//		delay_time_msec(1000);
//		outportb(0x03f2, 0x01c);
//		osd_fdc_get_drive_status(0);
    }

    exit(-1);
}
#endif
