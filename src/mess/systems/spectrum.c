/***************************************************************************
	Spectrum memory map

	CPU:
		0000-3fff ROM
		4000-ffff RAM

        Spectrum +3 memory mapped:

        CPU:
                0000-3fff Banked rom/ram
                4000-7fff Banked ram
                8000-bfff Banked ram
                c000-ffff Banked ram

Interrupts:

Changes:

29/1/2000       KT - Implemented initial +3 emulation
30/1/2000       KT - Improved input port decoding for reading
                     and therefore correct keyboard handling for Spectrum and +3
31/1/2000       KT - Implemented buzzer sound for Spectrum and +3.
                     Implementation copied from Paul Daniel's Jupiter driver.
                     Fixed screen display problems with dirty chars.
                     Added support to load .Z80 snapshots. 48k support so far.
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int	spectrum_rom_load(int id, const char *name);
extern int	spectrum_rom_id(const char *name, const char *gamename);
extern int  load_snap(void);

extern int  spectrum_vh_start(void);
extern void spectrum_vh_stop(void);
extern void spectrum_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern int  spectrum_plus3_vh_start(void);
extern void spectrum_plus3_vh_stop(void);
extern void spectrum_plus3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern void spectrum_init_machine(void);

extern void spectrum_characterram_w(int offset,int data);
extern int  spectrum_characterram_r(int offset);
extern void spectrum_colorram_w(int offset,int data);
extern int  spectrum_colorram_r(int offset);

extern int spectrum_sh_state;

void spectrum_port_fe_w (int offset,int data)
{
        /* DAC output state */
        spectrum_sh_state = (data>>4) & 0x01;
}


/* +3 hardware */
#include "mess/machine/nec765.h"
#include "cpuintrf.h"

static void spectrum_plus3_update_memory(void);

static int spectrum_plus3_port_7ffd_data = 0;
static int spectrum_plus3_port_1ffd_data = 0;
static unsigned char *spectrum_plus3_ram = NULL;
unsigned char *spectrum_plus3_screen_location = NULL;
/*
extern void amstrad_seek_callback(int, int);
extern void amstrad_get_id_callback(int, nec765_id *, int, int);
extern void amstrad_get_sector_ptr_callback(int, int,int, char **);
extern int amstrad_get_sectors_per_track(int, int);
*/

static nec765_interface spectrum_plus3_nec765_interface =
{
        NULL, NULL, NULL, NULL
        /*amstrad_seek_callback,
        amstrad_get_sectors_per_track,
        amstrad_get_id_callback,
        amstrad_get_sector_ptr_callback
*/
};





void spectrum_plus3_init_machine(void)
{
        spectrum_plus3_ram = (unsigned char *)malloc(128*1024);
        memset(spectrum_plus3_ram, 0, 128*1024);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);

        cpu_setbankhandler_w(5, MWA_BANK5);
        cpu_setbankhandler_w(6, MWA_BANK6);
        cpu_setbankhandler_w(7, MWA_BANK7);
        cpu_setbankhandler_w(8, MWA_BANK8);

        nec765_init(&spectrum_plus3_nec765_interface);

        spectrum_plus3_port_7ffd_data = 0;
        /* set initial ram config */
        spectrum_plus3_port_1ffd_data = 3;
        spectrum_plus3_update_memory();

        /* set initial rom config */
        spectrum_plus3_port_1ffd_data = 0;

        spectrum_plus3_update_memory();


        spectrum_init_machine();


}


static void spectrum_plus3_port_bffd_w(int offset, int data)
{
        AY8910_write_port_0_w(0, data);
}

static void spectrum_plus3_port_3ffd_w(int offset, int data)
{
        nec765_data_w(data);
}

static int spectrum_plus3_port_3ffd_r(int offset)
{
        return nec765_data_r();
}


static int spectrum_plus3_port_2ffd_r(int offset)
{
        return nec765_status_r();
}

static void spectrum_plus3_port_fffd_w(int offset, int data)
{
        AY8910_control_port_0_w(0, data);
}

/* +3 manual is confused about this */

static int spectrum_plus3_port_fffd_r(int offset)
{
        return AY8910_read_port_0_r(0);
}

static int spectrum_plus3_memory_selections[]=
{
        0,1,2,3,
        4,5,6,7,
        4,5,6,3,
        4,7,6,3
};

static void spectrum_plus3_update_memory(void)
{
        if (spectrum_plus3_port_7ffd_data & 8)
        {
                if (errorlog) fprintf(errorlog,"SCREEN 1: BLOCK 7\r\n");

                spectrum_plus3_screen_location = spectrum_plus3_ram + (7<<14);
        }
        else
        {
                if (errorlog) fprintf(errorlog, "SCREEN 2: BLOCK 5\r\n");

                spectrum_plus3_screen_location = spectrum_plus3_ram + (5<<14);
        }


        /* default is to write data to 0x0000-0x03fff */
        cpu_setbankhandler_w(5, MWA_BANK5);

        /* select ram at 0x0c000-0x0ffff - this will get overriden
        with Extended memory paging! */
        {
                int ram_page;
                unsigned char *ram_data;

                ram_page = spectrum_plus3_port_7ffd_data & 0x07;
                ram_data = spectrum_plus3_ram + (ram_page<<14);

                cpu_setbank(4, ram_data);
                cpu_setbank(8, ram_data);

                if (errorlog) fprintf(errorlog, "RAM at 0xc000: %02x\r\n",ram_page);

        }


        if ((spectrum_plus3_port_1ffd_data & 0x01)==0)
        {
                /* ROM switching */
                unsigned char *ChosenROM;
                int ROMSelection;

                ROMSelection = ((spectrum_plus3_port_7ffd_data>>4) & 0x01) |
                    ((spectrum_plus3_port_1ffd_data>>1) & 0x02);

                /* rom 0 is editor, rom 1 is syntax, rom 2 is DOS, rom 3 is 48 BASIC */

                ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

                cpu_setbank(1, ChosenROM);

                cpu_setbankhandler_w(5, MWA_NOP);


                if (errorlog) fprintf(errorlog,"rom switch: %02x\r\n", ROMSelection);
        }
        else
        {
                /* Extended memory paging */

                int *memory_selection;
                int MemorySelection;
                unsigned char *ram_data;

                MemorySelection = (spectrum_plus3_port_1ffd_data>>1) & 0x03;

                memory_selection = &spectrum_plus3_memory_selections[(MemorySelection<<2)];

                ram_data = spectrum_plus3_ram + (memory_selection[0]<<14);
                cpu_setbank(1, ram_data);
                cpu_setbank(5, ram_data);

                ram_data = spectrum_plus3_ram + (memory_selection[1]<<14);
                cpu_setbank(2, ram_data);
                cpu_setbank(6, ram_data);

                ram_data = spectrum_plus3_ram + (memory_selection[2]<<14);
                cpu_setbank(3, ram_data);
                cpu_setbank(7, ram_data);

                ram_data = spectrum_plus3_ram + (memory_selection[3]<<14);
                cpu_setbank(4, ram_data);
                cpu_setbank(8, ram_data);

                if (errorlog) fprintf(errorlog,"extended memory paging: %02x\r\n",MemorySelection);

         }



}


static void spectrum_plus3_port_7ffd_w(int offset, int data)
{
       /* D0-D2: RAM page located at 0x0c000-0x0ffff */
       /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
       /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
       /* D5 - Disable paging */

        /* disable paging? */
  //      if (spectrum_plus3_port_7ffd_data & 0x020)
    //            return;

        /* store new state */
        spectrum_plus3_port_7ffd_data = data;

        /* update memory */
        spectrum_plus3_update_memory();
}

static void spectrum_plus3_port_1ffd_w(int offset, int data)
{

        /* D0-D1: ROM/RAM paging */
        /* D2: Affects if d0-d1 work on ram/rom */
        /* D3 - Disk motor on/off */
        /* D4 - parallel port strobe */

        spectrum_plus3_port_1ffd_data = data;

        /* disable paging? */
//        if ((spectrum_plus3_port_7ffd_data & 0x020)==0)
        {
                /* no */
                spectrum_plus3_update_memory();
        }
}



static struct MemoryReadAddress spectrum_readmem[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_r },
	{ 0x5800, 0x5aff, spectrum_colorram_r },
	{ 0x5b00, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spectrum_writemem[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_w },
	{ 0x5800, 0x5aff, spectrum_colorram_w },
	{ 0x5b00, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress spectrum_plus3_readmem[] = {
        { 0x0000, 0x3fff, MRA_BANK1 },
        { 0x4000, 0x7fff, MRA_BANK2 },
        { 0x8000, 0xbfff, MRA_BANK3 },
        { 0xc000, 0xffff, MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spectrum_plus3_writemem[] = {
        { 0x0000, 0x3fff, MWA_BANK5 },
        { 0x4000, 0x7fff, MWA_BANK6 },
        { 0x8000, 0xbfff, MWA_BANK7 },
        { 0xc000, 0xffff, MWA_BANK8 },
	{ -1 }  /* end of table */
};

/* KT: more accurate keyboard reading */
int spectrum_port_fe_r(int offset)
{
   int lines = offset>>8;
   int data = 0x01f;

   if ((lines & 1)==0)
        data &= (readinputport(0) & 0x01f);
   if ((lines & 2)==0)
        data &= (readinputport(1) & 0x01f);
   if ((lines & 4)==0)
        data &= (readinputport(2) & 0x01f);
   if ((lines & 8)==0)
        data &= (readinputport(3) & 0x01f);
   if ((lines & 16)==0)
        data &= (readinputport(4) & 0x01f);
   if ((lines & 32)==0)
        data &= (readinputport(5) & 0x01f);
   if ((lines & 64)==0)
        data &= (readinputport(6) & 0x01f);
   if ((lines & 128)==0)
        data &= (readinputport(7) & 0x01f);

   /* for now these bits are set to 1's */
   data |= (0x0ff^0x01f);

   return data;
}

int spectrum_port_r(int offset)
{
        if ((offset & 1)==0)
            return spectrum_port_fe_r(offset);

        return 0x0ff;
}

void  spectrum_port_w(int offset, int data)
{
        if ((offset & 1)==0)
            spectrum_port_fe_w(offset,data);
}

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropiatly */
static struct IOReadPort spectrum_readport[] = {
        {0, 0x0ffff, spectrum_port_r},
	{ -1 }
};

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropiatly */
static struct IOWritePort spectrum_writeport[] = {
        {0x0000, 0x0ffff, spectrum_port_w},
	{ -1 }
};


int    spectrum_plus3_port_r(int offset)
{
     if ((offset & 1)==0)
     {
         return spectrum_port_fe_r(offset);
     }

     /* KT: the following is not decoded exactly, need to check what
     is correct */
     if ((offset &  2)==0)
     {
         switch ((offset>>8) & 0x0ff)
         {
                case 0x0ff:
                        return spectrum_plus3_port_fffd_r(offset);
                case 0x02f:
                        return spectrum_plus3_port_2ffd_r(offset);
                case 0x03f:
                        return spectrum_plus3_port_3ffd_r(offset);
         }

     }

     return 0x0ff;
}

void    spectrum_plus3_port_w(int offset, int data)
{
        if ((offset & 1)==0)
                spectrum_port_fe_w(offset,data);

        /* the following is not decoded exactly, need to check
        what is correct! */
        if ((offset & 2)==0)
        {
                switch ((offset>>8) & 0x0ff)
                {
                        case 0x07f:
                                spectrum_plus3_port_7ffd_w(offset, data);
                                break;
                        case 0x0bf:
                                spectrum_plus3_port_bffd_w(offset, data);
                                break;
                        case 0x0ff:
                                spectrum_plus3_port_fffd_w(offset, data);
                                break;
                        case 0x01f:
                                spectrum_plus3_port_1ffd_w(offset, data);
                                break;
                        case 0x03f:
                                spectrum_plus3_port_3ffd_w(offset, data);
                }
        }
}

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropiatly */
static struct IOReadPort spectrum_plus3_readport[] = {
        {0x0000, 0xffff, spectrum_plus3_port_r},
        { -1 }
};

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropiatly */
static struct IOWritePort spectrum_plus3_writeport[] = {
        {0x0000, 0xffff, spectrum_plus3_port_w},
        { -1 }
};


static struct AY8910interface spectrum_plus3_ay_interface =
{
        1,
        1000000,
        {25,25},
        {0},
        {0},
        {0}
};

static struct DACinterface      spectrum_sh_interface =
{
        1,
        {100},
};


static struct GfxLayout spectrum_charlayout = {
	8,8,
	256,
	1,                      /* 1 bits per pixel */

	{ 0 },                  /* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8		        /* every char takes 1 consecutive byte */
};

static struct GfxDecodeInfo spectrum_gfxdecodeinfo[] = {
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( spectrum )
	PORT_START /* 0xFEFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",     KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",     KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",     KEYCODE_C,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",     KEYCODE_V,           IP_JOY_NONE )

	PORT_START /* 0xFDFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",      KEYCODE_A,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",      KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",      KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",      KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",      KEYCODE_G,           IP_JOY_NONE )

	PORT_START /* 0xFBFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",      KEYCODE_Q,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",      KEYCODE_W,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",      KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",      KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",      KEYCODE_T,           IP_JOY_NONE )

	PORT_START /* 0xF7FE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1",      KEYCODE_1,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2",      KEYCODE_2,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3",      KEYCODE_3,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4",      KEYCODE_4,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5",      KEYCODE_5,           IP_JOY_NONE )

	PORT_START /* 0xEFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0",      KEYCODE_0,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9",      KEYCODE_9,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8",      KEYCODE_8,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7",      KEYCODE_7,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6",      KEYCODE_6,           IP_JOY_NONE )

	PORT_START /* 0xDFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",      KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",      KEYCODE_O,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",      KEYCODE_I,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",      KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",      KEYCODE_Y,           IP_JOY_NONE )

	PORT_START /* 0xBFFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER",  KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",      KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",      KEYCODE_K,           IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",      KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",      KEYCODE_H,           IP_JOY_NONE )

	PORT_START /* 0x7FFE */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",     KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYM SHFT",  KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",         KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",         KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",         KEYCODE_B,        IP_JOY_NONE )
INPUT_PORTS_END


static unsigned char spectrum_palette[16*3] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xbf,
	0xbf, 0x00, 0x00, 0xbf, 0x00, 0xbf,
	0x00, 0xbf, 0x00, 0x00, 0xbf, 0xbf,
	0xbf, 0xbf, 0x00, 0xbf, 0xbf, 0xbf,

	0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
	0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
};

static unsigned short spectrum_colortable[128*2] = {
	0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7,

	 8,8,  8,9,  8,10,  8,11,  8,12,  8,13,  8,14,  8,15,
	 9,8,  9,9,  9,10,  9,11,  9,12,  9,13,  9,14,  9,15,
	10,8, 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8, 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8, 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8, 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8, 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8, 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};
/* Initialise the palette */
static void spectrum_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,spectrum_palette,sizeof(spectrum_palette));
	memcpy(sys_colortable,spectrum_colortable,sizeof(spectrum_colortable));
}


static struct MachineDriver machine_driver_spectrum =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,        /* 3.5 Mhz */
            spectrum_readmem,spectrum_writemem,
			spectrum_readport,spectrum_writeport,
			interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
	spectrum_init_machine,
	0,

	/* video hardware */
	32*8,                                /* screen width */
	24*8,                                /* screen height */
	{ 0, 32*8-1, 0, 24*8-1 },             /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	spectrum_vh_start,
	spectrum_vh_stop,
	spectrum_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* standard spectrum sound */
                {
                        SOUND_DAC,
                        &spectrum_sh_interface
                }
        }

};


static struct MachineDriver machine_driver_spectrum_plus3 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,        /* 3.5 Mhz */
                        spectrum_plus3_readmem,spectrum_plus3_writemem,
                        spectrum_plus3_readport,spectrum_plus3_writeport,
			interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
        spectrum_plus3_init_machine,
	0,

	/* video hardware */
	32*8,                                /* screen width */
	24*8,                                /* screen height */
	{ 0, 32*8-1, 0, 24*8-1 },             /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
        spectrum_plus3_vh_start,
        spectrum_plus3_vh_stop,
        spectrum_plus3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* +3 Ay-3-8912 sound */
                {
                        SOUND_AY8910,
                        &spectrum_plus3_ay_interface,
                },
                /* standard spectrum buzzer sound */
                {
                        SOUND_DAC,
                        &spectrum_sh_interface,
                }
        }
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("spectrum.rom", 0x0000, 0x4000, 0xddee531f)
ROM_END

ROM_START(specpls3)
        ROM_REGION(0x20000,REGION_CPU1)
        ROM_LOAD("spectrum.rom",0x010000,0x10000, 0x096e3c17a)
ROM_END

static const struct IODevice io_spectrum[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"sna\0z80\0",		/* file extensions */
        NULL,               /* private */
		spectrum_rom_id,	/* id */
		spectrum_rom_load,	/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

#define io_specpls3 io_spectrum

/*    YEAR  NAME      PARENT    MACHINE   		INPUT     INIT		COMPANY               	FULLNAME */
COMP( 1982, spectrum, 0,		spectrum, 		spectrum, 0,		"Sinclair Research",  	"ZX-Spectrum 48k" )
COMP( 1982, specpls3, 0,		spectrum_plus3, spectrum, 0,		"Amstrad plc",			"Spectrum +3" )

