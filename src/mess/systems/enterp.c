/***************************************************/
/* Enterprise 128k driver 		     	   */
/*                                                 */
/* Kevin Thacker 1999.                             */
/*                                                 */
/* EP Hardware: Z80 (CPU), Dave (Sound Chip + I/O) */
/* Nick (Graphics), WD1772 (FDC). 128k ram.        */
/*                                                 */
/* For an 8-bit machine, this kicks ass! A sound   */
/* Chip which is as powerful, or more powerful than */
/* the C64 SID, and a graphics chip capable of some */
/* really nice graphics. Pity it doesn't have HW sprites! */
/***************************************************/

#include "driver.h"
#include "mess/sndhrdw/dave.h"
#include "mess/machine/enterp.h"
#include "mess/vidhrdw/enterp.h"
//#include "vidhrdw/nick.h"
#include "mess/vidhrdw/epnick.h"
#include "mess/machine/wd179x.h"

/* there are 64us per line, although in reality
   about 50 are visible. */
/* there are 312 lines per screen, although in reality
   about 35*8 are visible */
#define ENTERPRISE_SCREEN_WIDTH	(50*16)
#define ENTERPRISE_SCREEN_HEIGHT	(35*8)

/* Enterprise bank allocations */
#define MEM_EXOS_0		0
#define	MEM_EXOS_1		1
#define MEM_CART_0		4
#define MEM_CART_1		5
#define MEM_CART_2		6
#define MEM_CART_3		7
#define MEM_EXDOS_0		0x020
#define	MEM_EXDOS_1		0x021
/* basic 64k ram */
#define MEM_RAM_0               ((unsigned int)0x0fc)
#define MEM_RAM_1               ((unsigned int)0x0fd)
#define MEM_RAM_2               ((unsigned int)0x0fe)
#define MEM_RAM_3               ((unsigned int)0x0ff)
/* additional 64k ram */
#define MEM_RAM_4               ((unsigned int)0x0f8)
#define MEM_RAM_5               ((unsigned int)0x0f9)
#define MEM_RAM_6               ((unsigned int)0x0fa)
#define MEM_RAM_7               ((unsigned int)0x0fb)

/* base of all ram allocated - 128k */
unsigned char *Enterprise_RAM;

/* current ram pages for 16k page 0..3 */
//static unsigned char *Enterprise_CPU_ReadPages[4];
//static unsigned char *Enterprise_CPU_WritePages[4];

static unsigned char KeyboardState_Previous[16]={0x0ff};


void	Nick_reg_w(int RegIndex, int Data);


static int Enterprise_KeyboardInterrupt(void)
{
	int int_state = 0;
	int i;

	for (i=0; i<10; i++)
	{
		unsigned char Data = readinputport(i);

		if (Data != KeyboardState_Previous[i])
		{
			int_state = 1;
		}

		KeyboardState_Previous[i] = Data;
	}

	return int_state;
}



/* The Page index for each 16k page is programmed into
Dave. This index is a 8-bit number. The array below
defines what data is pointed to by each of these page index's
that can be selected. If NULL, when reading return floating
bus byte, when writing ignore */
static unsigned char * Enterprise_Pages_Read[256];
static unsigned char * Enterprise_Pages_Write[256];

/* index of keyboard line to read */
static int Enterprise_KeyboardLine = 0;

/* set read/write pointers for CPU page */
void	Enterprise_SetMemoryPage(int CPU_Page, int EP_Page)
{
        //Enterprise_CPU_ReadPages[CPU_Page] =
        //                        Enterprise_Pages_Read[EP_Page & 0x0ff];
        //Enterprise_CPU_WritePages[CPU_Page] =
        //                        Enterprise_Pages_Write[EP_Page & 0x0ff];

        cpu_setbank((CPU_Page+1), Enterprise_Pages_Read[EP_Page & 0x0ff]);
        cpu_setbank((CPU_Page+5), Enterprise_Pages_Write[EP_Page & 0x0ff]);
}

/* EP specific handling of dave register write */
static void enterprise_dave_reg_write(int RegIndex, int Data)
{
	switch (RegIndex)
	{

		case 0x010:
		{
		  /* set CPU memory page 0 */
			Enterprise_SetMemoryPage(0, Data);
		}
		break;

		case 0x011:
		{
		  /* set CPU memory page 1 */
			Enterprise_SetMemoryPage(1, Data);
		}
		break;

		case 0x012:
		{
		  /* set CPU memory page 2 */
			Enterprise_SetMemoryPage(2, Data);
		}
		break;

		case 0x013:
		{
		  /* set CPU memory page 3 */
			Enterprise_SetMemoryPage(3, Data);
		}
		break;

		case 0x015:
		{
		  /* write keyboard line */
			Enterprise_KeyboardLine = Data & 15;
		}
		break;

                default:
                        break;
	}
}

static void enterprise_dave_reg_read(int RegIndex)
{
	switch (RegIndex)
	{
		case 0x015:
		{
		  /* read keyboard line */
			Dave_setreg(0x015,
				readinputport(Enterprise_KeyboardLine));
		}
		break;

                default:
                  break;
	}
}


/* enterprise interface to dave - ok, so Dave chip is unique
to Enterprise. But these functions make it nice and easy to see
whats going on. */
DAVE_INTERFACE	enterprise_dave_interface=
{
	enterprise_dave_reg_read,
	enterprise_dave_reg_write

};


void	Enterprise_Initialise()
{
	int i;

        for (i=0; i<256; i++)
	{
                /* reads to memory pages that are not set returns 0x0ff */
                Enterprise_Pages_Read[i] = Enterprise_RAM+0x020000;
                /* writes to memory pages that are not set are ignored */
                Enterprise_Pages_Write[i] = Enterprise_RAM+0x024000;
	}

        /* setup dummy read area so it will always return 0x0ff */
        memset(Enterprise_RAM+0x020000, 0x0ff, 0x04000);

	/* set read pointers */
        /* exos */
        Enterprise_Pages_Read[MEM_EXOS_0] = &memory_region(REGION_CPU1)[0x010000];
        Enterprise_Pages_Read[MEM_EXOS_1] = &memory_region(REGION_CPU1)[0x014000];
        /* basic */
        Enterprise_Pages_Read[MEM_CART_0] = &memory_region(REGION_CPU1)[0x018000];
        /* ram */
        Enterprise_Pages_Read[MEM_RAM_0] = Enterprise_RAM;
	Enterprise_Pages_Read[MEM_RAM_1] = Enterprise_RAM + 0x04000;
	Enterprise_Pages_Read[MEM_RAM_2] = Enterprise_RAM + 0x08000;
	Enterprise_Pages_Read[MEM_RAM_3] = Enterprise_RAM + 0x0c000;
	Enterprise_Pages_Read[MEM_RAM_4] = Enterprise_RAM + 0x010000;
	Enterprise_Pages_Read[MEM_RAM_5] = Enterprise_RAM + 0x014000;
	Enterprise_Pages_Read[MEM_RAM_6] = Enterprise_RAM + 0x018000;
	Enterprise_Pages_Read[MEM_RAM_7] = Enterprise_RAM + 0x01c000;
        /* exdos */
//        Enterprise_Pages_Read[MEM_EXDOS_0] = &memory_region(REGION_CPU1)[0x01c000];
  //      Enterprise_Pages_Read[MEM_EXDOS_1] = &memory_region(REGION_CPU1)[0x020000];

	/* set write pointers */
	Enterprise_Pages_Write[MEM_RAM_0] = Enterprise_RAM;
	Enterprise_Pages_Write[MEM_RAM_1] = Enterprise_RAM + 0x04000;
	Enterprise_Pages_Write[MEM_RAM_2] = Enterprise_RAM + 0x08000;
	Enterprise_Pages_Write[MEM_RAM_3] = Enterprise_RAM + 0x0c000;
	Enterprise_Pages_Write[MEM_RAM_4] = Enterprise_RAM + 0x010000;
	Enterprise_Pages_Write[MEM_RAM_5] = Enterprise_RAM + 0x014000;
	Enterprise_Pages_Write[MEM_RAM_6] = Enterprise_RAM + 0x018000;
	Enterprise_Pages_Write[MEM_RAM_7] = Enterprise_RAM + 0x01c000;

	Dave_SetIFace(&enterprise_dave_interface);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);

        cpu_setbankhandler_w(5, MWA_BANK5);
        cpu_setbankhandler_w(6, MWA_BANK6);
        cpu_setbankhandler_w(7, MWA_BANK7);
        cpu_setbankhandler_w(8, MWA_BANK8);


	Dave_reg_w(0x010,0);
	Dave_reg_w(0x011,0);
	Dave_reg_w(0x012,0);
	Dave_reg_w(0x013,0);

        wd179x_init(1);
}

int enterprise_timer_interrupt(void)
{
	if (Enterprise_KeyboardInterrupt())
	{
		Dave_SetInt(DAVE_INT_INT1);
	}

	if (Dave_Interrupt())
	{
		return interrupt();
	}

	return 0;
}

int enterprise_frame_interrupt(void)
{
	return 0;
}


static int	enterprise_wd177x_read(int Offset)
{
        switch (Offset & 0x03)
	{
		case 0:
                        return wd179x_status_r(Offset);
		case 1:
                        return wd179x_track_r(Offset);
		case 2:
                        return wd179x_sector_r(Offset);
		case 3:
                        return wd179x_data_r(Offset);
                default:
                        break;
	}

        return 0x0ff;
}

static void	enterprise_wd177x_write(int Offset, int Data)
{
        switch (Offset & 0x03)
	{
		case 0:
                        wd179x_command_w(Offset, Data);
                        return;
		case 1:
                        wd179x_track_w(Offset, Data);
                        return;
		case 2:
                        wd179x_sector_w(Offset, Data);
                        return;
		case 3:
                        wd179x_data_w(Offset, Data);
                        return;
                default:
                        break;
	}
}



/* I've done this because the ram is banked in 16k blocks, and
the rom can be paged into bank 0 and bank 3. */
static struct MemoryReadAddress readmem_enterprise[] =
{
    { 0x0000, 0x03fff, MRA_BANK1},
        { 0x04000, 0x07fff, MRA_BANK2 },
        { 0x08000, 0x0bfff, MRA_BANK3 },
        { 0x0c000, 0x0ffff, MRA_BANK4 },
        { 0x010000, 0x017fff, MRA_ROM}, /* exos */
        { 0x018000, 0x01bfff, MRA_ROM}, /* basic */
        { 0x01c000, 0x023fff, MRA_ROM}, /* exdos */
        { -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem_enterprise[] =
{
       { 0x0000, 0x3fff, MWA_BANK5 },
        { 0x4000, 0x7fff, MWA_BANK6 },
           {0x08000,0x0bfff, MWA_BANK7 },
           {0x0c000,0x0ffff, MWA_BANK8 },
        { -1 }  /* end of table */
};

/* bit 0 - select drive 0,
   bit 1 - select drive 1,
   bit 2 - select drive 2,
   bit 3 - select drive 3
   bit 4 - side
   bit 5 - mfm/fm select
   bit 6 - disk change reset
   bit 7 - in use
*/

int EXDOS_GetDriveSelection(int Data)
{
     if (Data & 0x01)
     {
        return 0;
     }

     if (Data & 0x02)
     {
        return 1;
     }

     if (Data & 0x04)
     {
        return 2;
     }

     if (Data & 0x08)
     {
        return 3;
     }

     return 0;
}

static char EXDOS_CARD_R = 0;

static void     wd177x_callback(int State)
{
   if (State==WD179X_IRQ_CLR)
   {
        EXDOS_CARD_R &= ~0x02;
   }

   if (State==WD179X_IRQ_SET)
   {
        EXDOS_CARD_R |= 0x02;
   }

   if (State==WD179X_DRQ_CLR)
   {
        EXDOS_CARD_R &= ~0x080;
   }

   if (State==WD179X_DRQ_SET)
   {
        EXDOS_CARD_R |= 0x080;
   }
}



static void	exdos_card_w(int Offset, int Data)
{
        /* drive side */
        int head = (Data>>4) & 0x01;

        int drive = EXDOS_GetDriveSelection(Data);

        fprintf(errorlog,"exdos card w: %02x\r\n",Data);

        wd179x_select_drive(drive, head, wd177x_callback,"fd0.dsk");
}

/* bit 0 - ??
   bit 1 - IRQ from WD1772
   bit 2 - ??
   bit 3 - ??
   bit 4 - ??
   bit 5 - ??
   bit 6 - Disk change signal from disk drive
   bit 7 - DRQ from WD1772
*/


static int exdos_card_r(int Offset)
{
        fprintf(errorlog,"EXDOS CARD R\r\n");
        return EXDOS_CARD_R;
}

static struct IOReadPort readport_enterprise[] =
{
        {0x010, 0x017, enterprise_wd177x_read},
        {0x018, 0x018, exdos_card_r},
        {0x01c, 0x01c, exdos_card_r},
        {0x0a0, 0x0bf, Dave_reg_r},
	{ -1 } /* end of table */
};


static struct IOWritePort writeport_enterprise[] =
{
        {0x010, 0x017, enterprise_wd177x_write},
	{0x018, 0x018, exdos_card_w},
        {0x01c, 0x01c, exdos_card_w},
        {0x080, 0x08f, Nick_reg_w},
	{0x0a0, 0x0bf, Dave_reg_w},
	{ -1 } /* end of table */
};

/*
Enterprise Keyboard Matrix

        Bit
Line    0    1    2    3    4    5    6    7
0       n    \    b    c    v    x    z    SHFT
1       h    N/C  g    d    f    s    a    CTRL
2       u    q    y    r    t    e    w    TAB
3       7    1    6    4    5    3    2    N/C
4       F4   F8   F3   F6   F5   F7   F2   F1
5       8    N/C  9    -    0    ^    DEL  N/C
6       j    N/C  k    ;    l    :    ]    N/C
7       STOP DOWN RGHT UP   HOLD LEFT RETN ALT
8       m    ERSE ,    /    .    SHFT SPCE INS
9       i    N/C  o    @    p    [    N/C  N/C

N/C - Not connected or just dont know!
*/


INPUT_PORTS_START(enterprise)
	/* keyboard line 0 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "n", KEYCODE_N,IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "\\", KEYCODE_SLASH,IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "b", KEYCODE_B,IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "c", KEYCODE_C, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "v", KEYCODE_V, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "x", KEYCODE_X, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "z", KEYCODE_Z, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)

     /* keyboard line 1 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "h", KEYCODE_H, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "g", KEYCODE_G, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "d", KEYCODE_D, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "f", KEYCODE_F, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "s", KEYCODE_S, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "a", KEYCODE_A, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)

     /* keyboard line 2 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "u", KEYCODE_U, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "q", KEYCODE_Q, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "y", KEYCODE_Y, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "r", KEYCODE_R, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "t", KEYCODE_T, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "e", KEYCODE_E, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "w", KEYCODE_W, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "TAB", KEYCODE_TAB, IP_JOY_NONE)

     /* keyboard line 3 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "7", KEYCODE_7, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "1", KEYCODE_1, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "6", KEYCODE_6, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "4", KEYCODE_4, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "5", KEYCODE_5, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "3", KEYCODE_3, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "2", KEYCODE_2, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)

     /* keyboard line 4 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "f4", KEYCODE_F4, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "f8", KEYCODE_F8, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "f3", KEYCODE_F3, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "f6", KEYCODE_F6, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "f5", KEYCODE_F5, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "f7", KEYCODE_F7, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "f2", KEYCODE_F2, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "f1", KEYCODE_F1, IP_JOY_NONE)

     /* keyboard line 5 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "8", KEYCODE_8, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "9", KEYCODE_9, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "-", KEYCODE_MINUS, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "0", KEYCODE_0, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "^", KEYCODE_EQUALS, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c",IP_KEY_NONE, IP_JOY_NONE)

     /* keyboard line 6 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "j", KEYCODE_J,IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "k", KEYCODE_K, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, ";", KEYCODE_COLON, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "l", KEYCODE_L, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, ":", KEYCODE_QUOTE, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)

     /* keyboard line 7 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "STOP", KEYCODE_END, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "DOWN", KEYCODE_DOWN, IPT_JOYSTICK_DOWN)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "RIGHT", KEYCODE_RIGHT, IPT_JOYSTICK_RIGHT)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "UP", KEYCODE_UP, IPT_JOYSTICK_UP)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "HOLD", KEYCODE_HOME, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "LEFT", KEYCODE_LEFT, IPT_JOYSTICK_LEFT)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "ALT", KEYCODE_LALT, IP_JOY_NONE)


     /* keyboard line 8 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "m", KEYCODE_M, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "ERASE", KEYCODE_DEL, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, ",", KEYCODE_COMMA, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "/", KEYCODE_SLASH, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, ".", KEYCODE_STOP, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "INSERT", KEYCODE_INSERT, IP_JOY_NONE)


     /* keyboard line 9 */
     PORT_START
     PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "i", KEYCODE_I, IP_JOY_NONE)
     PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "o", KEYCODE_O, IP_JOY_NONE)
     PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "@", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "p", KEYCODE_P, IP_JOY_NONE)
     PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
     PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)
     PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "n/c", IP_KEY_NONE, IP_JOY_NONE)

INPUT_PORTS_END


                                     /* TEMP! */
void enterprise_sh_start(void)
{

}

void	enterprise_sh_stop(void)
{
}

void	enterprise_sh_update(void)
{
}

/*
static struct CustomSound_interface enterprise_custom_sound=
{
	NULL;
	//enterprise_sh_start,
	//enterprise_sh_stop,
	//enterprise_sh_update

};
*/

/* 4Mhz clock, although it can be changed to 8 Mhz! */

static struct MachineDriver enterprise_machine_driver =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,							/* type */
			4000000,							/* clock: See Note Above */
			readmem_enterprise, 				/* MemoryReadAddress */
			writemem_enterprise,				/* MemoryWriteAddress */
			readport_enterprise,				/* IOReadPort */
			writeport_enterprise,				/* IOWritePort */
			enterprise_frame_interrupt, 		/* VBlank Interrupt */
			1,									/* vblanks per frame */
			enterprise_timer_interrupt, 		/* timer interrupt */
			1000								/* timers per second */
		},
	},
	50, 										/* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,				/* vblank duration */
	1,											/* cpu slices per frame */
	enterprise_init_machine,					/* init machine */
	NULL,
	/* video hardware */
	ENTERPRISE_SCREEN_WIDTH,					/* screen width */
	ENTERPRISE_SCREEN_HEIGHT,					/* screen height */
	{ 0,(ENTERPRISE_SCREEN_WIDTH-1),0,(ENTERPRISE_SCREEN_HEIGHT-1)}, /* rectangle: visible_area */
	0, /*enterprise_gfxdecodeinfo,*/			/* graphics decode info */
	NICK_PALETTE_SIZE,							/* total colours */
	NICK_COLOURTABLE_SIZE,						/* color table len */
	nick_init_palette,							/* convert color prom */

	VIDEO_TYPE_RASTER,							/* video attributes */
	0,											/* MachineLayer */
	enterprise_vh_start,
	enterprise_vh_stop,
	enterprise_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* MachineSound */
		{
			/* change to dave eventually */
			0,/* SOUND_CUSTOM, */
			0,/* &enterprise_custom_sound */
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(enterprise)
        /* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
        ROM_REGIONX(0x24000,REGION_CPU1)
        ROM_LOAD("exos.rom",0x10000,0x8000,  0xd421795f)
        ROM_LOAD("exbas.rom",0x18000,0x4000, 0x683cf455)
        ROM_LOAD("exdos.rom",0x1c000,0x8000, 0xd1d7e157)
ROM_END

/* Enterprise 128k game driver */
struct GameDriver ep128_driver =
{
	__FILE__,
	0,
    "ep128",
	"Enterprise 128K",			/* description */
	"1985",
	"Intelligent Software",
	"Kevin Thacker [MESS driver]\n \
	James Boulton [EP help]\n \
	Jean-Pierre Malisse [EP help]", /*credits */
	0,
    &enterprise_machine_driver,		/* MachineDriver */
	0,
    rom_enterprise,
	enterprise_rom_load,
	enterprise_rom_id,         /* load rom_file images */
    0,                                              /* default file extensions */
	0,                      /* number of ROM slots */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	0,                      /* number of cassette drives supported */
	0,                      /* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_enterprise,

	0,                      /* color_prom */
	/*enterprise_colour_palette*/0,          /* color palette */
	/*enterprise_colour_table*/0,       /* color lookup table */

	GAME_COMPUTER|ORIENTATION_DEFAULT,    /* orientation */

	0,                      /* hiscore load */
	0                       /* hiscore save */
};
