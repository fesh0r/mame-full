/******************************************************************************

        nc.c

        z88/NC150/NC200 Notepad computer 


        system driver

        Documentation:
        z88 I/O Specification by Cliff Lawson,
        z88EM by Russell Marks


        Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "includes/z88.h"


static mem_read_handler z88_bankhandler_r[]={
MRA_BANK1, MRA_BANK2, MRA_BANK3, MRA_BANK4};

static mem_write_handler z88_bankhandler_w[]={
MWA_BANK5, MWA_BANK6, MWA_BANK7, MWA_BANK8};




static void nc_refresh_memory_config(void)
{
}

void z88_init_machine(void)
{
}


void z88_shutdown_machine(void)
{
}
 

static struct MemoryReadAddress readmem_z88[] =
{
        {0x00000, 0x03fff, MRA_BANK1},
        {0x04000, 0x07fff, MRA_BANK2},
        {0x08000, 0x0bfff, MRA_BANK3},
        {0x0c000, 0x0ffff, MRA_BANK4},
	{-1}							   /* end of table */
};


static struct MemoryWriteAddress writemem_z88[] =
{
        {0x00000, 0x03fff, MWA_BANK5},
        {0x04000, 0x07fff, MWA_BANK6},
        {0x08000, 0x0bfff, MWA_BANK7},
        {0x0c000, 0x0ffff, MWA_BANK8},
	{-1}							   /* end of table */
};

/* segment register write */
WRITE_HANDLER(blink_srx_w)
{

}

WRITE_HANDLER(z88_port_w)
{
	switch (offset & 0x0ff)
	{
		case 0x0b0:
		case 0x0b1:
		case 0x0b2:
		case 0x0b3:
			blink_srx_w(offset-0x0b0, data);
			return;
	}




}

READ_HANDLER(z88_port_r)
{
	switch (offset & 0x0ff)
	{
		case 0x0b2:
		{
			int lines;

			lines = offset>>8;

			if ((lines & 0x080)==0)
				return readinputport(7);
			
			if ((lines & 0x040)==0)
				return readinputport(6);

			if ((lines & 0x020)==0)
				return readinputport(5);
			
			if ((lines & 0x010)==0)
				return readinputport(4);
			
			if ((lines & 0x008)==0)
				return readinputport(3);

			if ((lines & 0x004)==0)
				return readinputport(2);

			if ((lines & 0x002)==0)
				return readinputport(1);
			
			if ((lines & 0x001)==0)
				return readinputport(0);
			
			if ((lines & 0x080)==0)
				return readinputport(7);


		}
		break;

	}


}


static struct IOReadPort readport_z88[] =
{
	{-1}							   /* end of table */
};

static struct IOWritePort writeport_z88[] =
{
	{0x0d0,0x0d3, blink_srx_w},
	{-1}                                                       /* end of table */
        
};

/*
------------------------------------------------------------------------- 
         | D7     D6      D5      D4      D3      D2      D1      D0 
------------------------------------------------------------------------- 
A15 (#7) | RSH    SQR     ESC     INDEX   CAPS    .       /       £ 
A14 (#6) | HELP   LSH     TAB     DIA     MENU    ,       ;       ' 
A13 (#5) | [      SPACE   1       Q       A       Z       L       0 
A12 (#4) | ]      LFT     2       W       S       X       M       P 
A11 (#3) | -      RGT     3       E       D       C       K       9 
A10 (#2) | =      DWN     4       R       F       V       J       O 
A9  (#1) | \      UP      5       T       G       B       U       I 
A8  (#0) | DEL    ENTER   6       Y       H       N       7       8 
------------------------------------------------------------------------- 
*/

INPUT_PORTS_START(z88)
	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"8", KEYCODE_8, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"I", KEYCODE_I, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"O", KEYCODE_O, IP_JOY_NONE)

	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"9", KEYCODE_9, IP_JOY_NONE)


	PORT_START
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD,"-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD,"RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD,"3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD,"E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD,"D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD,"C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD,"K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD,"9", KEYCODE_9, IP_JOY_NONE)


INPUT_PORTS_END

static struct MachineDriver machine_driver_z88 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
                        CPU_Z80 | CPU_16BIT_PORT ,  /* type */
                        4000000, /* clock: See Note Above */
                        readmem_z88,                   /* MemoryReadAddress */
                        writemem_z88,                  /* MemoryWriteAddress */
                        readport_z88,                  /* IOReadPort */
                        writeport_z88,                 /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
                        0, 0,   /* every scanline */
		},
	},
        50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
        z88_init_machine,                      /* init machine */
        z88_shutdown_machine,
	/* video hardware */
        Z88_SCREEN_WIDTH, /* screen width */
        Z88_SCREEN_HEIGHT,  /* screen height */
        {0, (Z88_SCREEN_WIDTH - 1), 0, (Z88_SCREEN_HEIGHT - 1)},        /* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
        Z88_NUM_COLOURS,                                                        /* total colours */
        Z88_NUM_COLOURS,                                                        /* color table len */
        z88_init_palette,                      /* init palette */

        VIDEO_TYPE_RASTER,                                  /* video attributes */
        0,                                                                 /* MachineLayer */
        z88_vh_start,
        z88_vh_stop,
        z88_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(z88)
        ROM_REGION(((64*1024)+(128*1024)), REGION_CPU1)
        ROM_LOAD("z88v4.rom", 0x010000, 0x020000, 0x0000)
ROM_END

static const struct IODevice io_z88[] =
{

	{IO_END}
};


/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY   FULLNAME */
COMP( 19??, z88,   0,                z88,  z88,      0,       "Acorn", "z88")

