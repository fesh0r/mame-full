/******************************************************************************

  Exidy Sorcerer system driver

		Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/exidy.h"
#include "printer.h"


static OPBASE_HANDLER( exidy_opbaseoverride )
{
	memory_set_opbase_handler(0,0);

	cpu_set_reg(Z80_PC, 0x0e000);

	return (cpu_get_pc() & 0x0ffff);
}


void exidy_init_machine(void)
{
	/* this is temporary. Normally when a Z80 is reset, it will
	execute address 0. The exidy starts executing from 0x0e000 */
	memory_set_opbase_handler(0, exidy_opbaseoverride);

}

void exidy_shutdown_machine(void)
{
}


MEMORY_READ_START( readmem_exidy )
	{0x00000, 0x07fff, MRA_RAM},		/* ram 32k machine */
	{0x0c000, 0x0efff, MRA_ROM},		/* rom pac */
	{0x0f000, 0x0f7ff, MRA_RAM},		/* screen ram */	
	{0x0f800, 0x0fbff, MRA_ROM},		/* char rom */
	{0x0fc00, 0x0ffff, MRA_RAM},		/* programmable chars */
MEMORY_END


MEMORY_WRITE_START( writemem_exidy )
	{0x00000, 0x07fff, MWA_RAM},		/* ram 32k machine */
	{0x0c000, 0x0efff, MWA_ROM},			/* rom pac */	
	{0x0f000, 0x0f7ff, MWA_RAM},		/* screen ram */
	{0x0f800, 0x0fbff, MWA_ROM},		/* char rom */
	{0x0fc00, 0x0ffff, MWA_RAM},		/* programmable chars */
MEMORY_END

static READ_HANDLER(exidy_unmapped_port_r)
{
	return 0x0ff;
}

/* output port:

	bits 4-7 cassette 
	bits 3-0 keyboard scan

  input port:
	bits 5,6,7 handshake for user port (0x0ff)
	bits 4..0 keyboard data


  parallel output 5,6,7,8 chooses keyboard line
*/


/* port fc:
	uart data
   port fd:
	uart status 
	used to initialise cassette!
*/
/* 
	port fe:

	output:

	bit 7: rs232 enable (1=enable rs232, 0=disable rs232)
	bit 6: baud rate (1=1200, 0=600)
	
	  
	bit 3..0: keyboard line select (output);


  
	input:
	bit 5: vsync (input)
	bit 4..0: keyboard data (output);
*/

/* port ff:
	data
	bit 7: out: printer strobe (-ve) in: busy signal (+ve)

  parallel output ji* 

	port ff:
	parallel input 

  handshake resets data available latch
*/
static unsigned char exidy_fc;
static unsigned char exidy_fd;
static unsigned char exidy_fe;
static unsigned char exidy_ff;
static int exidy_keyboard_line;

static WRITE_HANDLER(exidy_fc_port_w)
{



}


static WRITE_HANDLER(exidy_fd_port_w)
{



}

static WRITE_HANDLER(exidy_fe_port_w)
{
	exidy_keyboard_line = data & 0x0f;

	exidy_fe = data;
}

static WRITE_HANDLER(exidy_ff_port_w)
{



}

static READ_HANDLER(exidy_fc_port_r)
{
	return 0x0ff;

}

static READ_HANDLER(exidy_fd_port_r)
{
	return 0x0ff;

}

static READ_HANDLER(exidy_fe_port_r)
{
	unsigned char data;

	data = 0;
	/* vsync */
	data |= (readinputport(0) & 0x01)<<5;
	/* keyboard data */
	data |= (readinputport(exidy_keyboard_line+1) & 0x01f);

	return data;
}

static READ_HANDLER(exidy_ff_port_r)
{
	return 0x0ff;

}


PORT_READ_START( readport_exidy )
	{0x000, 0x0fb, exidy_unmapped_port_r},
	{0x0fc, 0x0fc, exidy_fc_port_r},
	{0x0fd, 0x0fd, exidy_fd_port_r},
	{0x0fe, 0x0fe, exidy_fe_port_r},
	{0x0ff, 0x0ff, exidy_ff_port_r},
PORT_END

PORT_WRITE_START( writeport_exidy )
	{0x0fc, 0x0fc, exidy_fc_port_w},
	{0x0fd, 0x0fd, exidy_fd_port_w},
	{0x0fe, 0x0fe, exidy_fe_port_w},
	{0x0ff, 0x0ff, exidy_ff_port_w},
PORT_END


INPUT_PORTS_START(exidy)
	PORT_START
	/* vblank */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* line 0 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Graphic", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Stop", IP_KEY_NONE, IP_JOY_NONE)
	/* line 1 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Sel)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Skip", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Repeat", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Clear", IP_KEY_NONE, IP_JOY_NONE)
	/* line 2 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	/* line 3 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	/* line 4 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	/* line 5 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", IP_KEY_NONE, IP_JOY_NONE)
	/* line 6 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	/* line 7 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	/* line 8 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	/* line 9 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	/* line 10 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	/* line 11 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "^", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LINE FEED", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", IP_KEY_NONE, IP_JOY_NONE)
	/* line 12 */
	PORT_START
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "- (PAD)", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ (PAD)", KEYCODE_SLASH_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "* (PAD)", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "+ (PAD)", KEYCODE_PLUS_PAD, IP_JOY_NONE)
	/* line 13 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 (PAD)", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 (PAD)", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 (PAD)", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 (PAD)", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 (PAD)", KEYCODE_0_PAD, IP_JOY_NONE)
	/* line 14 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (PAD)", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 (PAD)", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 (PAD)", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 (PAD)", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". (PAD)", KEYCODE_DEL_PAD, IP_JOY_NONE)
	/* line 15 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 (PAD)", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "? (PAD)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_BIT (0x04, 0x00, IPT_UNUSED)
	PORT_BIT (0x02, 0x00, IPT_UNUSED)
	PORT_BIT (0x01, 0x00, IPT_UNUSED)
INPUT_PORTS_END


/**********************************************************************************************************/

static struct MachineDriver machine_driver_exidy =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80 ,  /* type */
			2000000, /* clock - not correct */
			readmem_exidy,                   /* MemoryReadAddress */
			writemem_exidy,                  /* MemoryWriteAddress */
			readport_exidy,                  /* IOReadPort */
			writeport_exidy,                 /* IOWritePort */
			0,						   /* VBlank Interrupt */
			0,				   /* vblanks per frame */
			0, 0,   /* every scanline */
		},
	},
    50,                                                     /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	exidy_init_machine,                      /* init machine */
	exidy_shutdown_machine,
	/* video hardware */
	EXIDY_SCREEN_WIDTH, /* screen width */
	EXIDY_SCREEN_HEIGHT,  /* screen height */
	{0, (EXIDY_SCREEN_WIDTH - 1), 0, (EXIDY_SCREEN_HEIGHT - 1)},        /* rectangle: visible_area */
	0,								   /* graphics
										* decode info */
	EXIDY_NUM_COLOURS,                                                        /* total colours */
	EXIDY_NUM_COLOURS,                                                        /* color table len */
	exidy_init_palette,                      /* init palette */

	VIDEO_TYPE_RASTER,                                  /* video attributes */
	0,                                                                 /* MachineLayer */
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
	{
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(exidy)
		/* these are common to all because they are inside the machine */
        ROM_REGION(64*1024+32, REGION_CPU1,0)
		/* char rom */
		ROM_LOAD("exchr-1.dat",0x0f800, 1024, 0x4a7e1cdd)
		/* video prom */
        ROM_LOAD("bruce.dat", 0x010000, 32, 0xfae922cb)

		ROM_LOAD("exmo1-1.dat", 0x0e000, 0x0800, 0xac924f67)
		ROM_LOAD("exmo1-2.dat", 0x0e800, 0x0800, 0xead1d0f6)
		ROM_LOAD_OPTIONAL("exsb1-1.dat", 0x0c000, 0x0800, 0x1dd20d80)
		ROM_LOAD_OPTIONAL("exsb1-2.dat", 0x0c800, 0x0800, 0x1068a3f8)
		ROM_LOAD_OPTIONAL("exsb1-3.dat", 0x0d000, 0x0800, 0xe6332518)
		ROM_LOAD_OPTIONAL("exsb1-4.dat", 0x0d800, 0x0800, 0xa370cb19)		
ROM_END

static const struct IODevice io_exidy[] =
{
	IO_PRINTER_PORT(1,"prn\0"),
	{IO_END}
};


/*	  YEAR	NAME	 PARENT	MACHINE INPUT 	INIT COMPANY        FULLNAME */
COMP( 1979, exidy,   0,     exidy,  exidy,  0,   "Exidy Inc", "Sorcerer")
