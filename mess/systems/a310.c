/******************************************************************************
 *	Archimedes 310
 *
 *	system driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

void a310_init_machine(void)
{
	UINT8 *mem = memory_region(REGION_CPU1);

	cpu_setbank(1,&mem[0x00200000]);
	cpu_setbankhandler_r(1,MRA_BANK1);
	cpu_setbankhandler_w(1,MWA_ROM);

	cpu_setbank(2,&mem[0x00000000]);
	cpu_setbankhandler_r(2,MRA_RAM);
	cpu_setbankhandler_w(2,MWA_RAM);

    cpu_setbank(3,&mem[0x00200000]);
	cpu_setbankhandler_r(3,MRA_BANK3);
	cpu_setbankhandler_w(3,MWA_ROM);

    cpu_setbank(4,&mem[0x00200000]);
	cpu_setbankhandler_r(4,MRA_BANK4);
	cpu_setbankhandler_w(4,MWA_ROM);

    cpu_setbank(5,&mem[0x00200000]);
	cpu_setbankhandler_r(5,MRA_BANK5);
	cpu_setbankhandler_w(5,MWA_ROM);

    cpu_setbank(6,&mem[0x00200000]);
	cpu_setbankhandler_r(6,MRA_BANK6);
	cpu_setbankhandler_w(6,MWA_ROM);

    cpu_setbank(7,&mem[0x00200000]);
	cpu_setbankhandler_r(7,MRA_BANK7);
	cpu_setbankhandler_w(7,MWA_ROM);

    cpu_setbank(8,&mem[0x00200000]);
	cpu_setbankhandler_r(8,MRA_BANK8);
	cpu_setbankhandler_w(8,MWA_ROM);
}

void init_a310(void)
{
}

int a310_vh_start(void)
{
	if (generic_vh_start())
        return 1;
	return 0;
}

void a310_vh_stop(void)
{
	generic_vh_stop();
}

void a310_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < videoram_size; offs++ )
	{
		if( dirtybuffer[offs] )
		{
            int sx, sy, code;
			sx = (offs % 80) * 8;
			sy = (offs / 80) * 8;
			code = videoram[offs];
			drawgfx(bitmap,Machine->gfx[0],code,0,0,0,sx,sy,
                &Machine->visible_area,TRANSPARENCY_NONE,0);
        }
	}
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x00000000, 0x007fffff, MRA_BANK1 },
	{ 0x00800000, 0x00ffffff, MRA_BANK2 },
	{ 0x01000000, 0x017fffff, MRA_BANK3 },
	{ 0x01800000, 0x01ffffff, MRA_BANK4 },
	{ 0x02000000, 0x027fffff, MRA_BANK5 },
	{ 0x02800000, 0x02ffffff, MRA_BANK6 },
	{ 0x03000000, 0x037fffff, MRA_BANK7 },
	{ 0x03800000, 0x03ffffff, MRA_BANK8 },
    {-1}
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x00000000, 0x007fffff, MWA_BANK1 },
    { 0x001ff000, 0x001fffff, videoram_w, &videoram, &videoram_size },
	{ 0x00800000, 0x00ffffff, MWA_BANK2 },
	{ 0x01000000, 0x017fffff, MWA_BANK3 },
	{ 0x01800000, 0x01ffffff, MWA_BANK4 },
	{ 0x02000000, 0x027fffff, MWA_BANK5 },
	{ 0x02800000, 0x02ffffff, MWA_BANK6 },
	{ 0x03000000, 0x037fffff, MWA_BANK7 },
	{ 0x03800000, 0x03ffffff, MWA_BANK8 },
    {-1}
};

INPUT_PORTS_START( a310 )
	PORT_START /* DIP switches */
	PORT_BIT(0xfd, 0xfd, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "ESC",       KEYCODE_ESC,         IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1  !",      KEYCODE_1,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2  \"",     KEYCODE_2,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3  #",      KEYCODE_3,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4  $",      KEYCODE_4,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5  %",      KEYCODE_5,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6  &",      KEYCODE_6,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7  '",      KEYCODE_7,           IP_JOY_NONE )

    PORT_START /* KEY ROW 1 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "8  *",      KEYCODE_8,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "9  (",      KEYCODE_9,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0  )",      KEYCODE_0,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "-  _",      KEYCODE_MINUS,       IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "=  +",      KEYCODE_EQUALS,      IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "`  ~",      KEYCODE_TILDE,       IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "BACK SPACE",KEYCODE_BACKSPACE,   IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "TAB",       KEYCODE_TAB,         IP_JOY_NONE )

    PORT_START /* KEY ROW 2 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "q  Q",      KEYCODE_Q,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "w  W",      KEYCODE_W,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "e  E",      KEYCODE_E,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "r  R",      KEYCODE_R,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "t  T",      KEYCODE_T,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "y  Y",      KEYCODE_Y,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "u  U",      KEYCODE_U,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "i  I",      KEYCODE_I,           IP_JOY_NONE )

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "o  O",      KEYCODE_O,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "p  P",      KEYCODE_P,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "[  {",      KEYCODE_OPENBRACE,   IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "]  }",      KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "RETURN",    KEYCODE_ENTER,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "DEL",       KEYCODE_DEL,         IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "CTRL",      KEYCODE_LCONTROL,    IP_JOY_NONE )
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD | IPF_TOGGLE,
										"CAPS LOCK", KEYCODE_CAPSLOCK,    IP_JOY_NONE )

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "a  A",      KEYCODE_A,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "s  S",      KEYCODE_S,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "d  D",      KEYCODE_D,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "f  F",      KEYCODE_F,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "g  G",      KEYCODE_G,           IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "h  H",      KEYCODE_H,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "j  J",      KEYCODE_J,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "k  K",      KEYCODE_K,           IP_JOY_NONE )

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "l  L",      KEYCODE_L,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, ";  :",      KEYCODE_COLON,       IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "'  \"",     KEYCODE_QUOTE,       IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "\\  |",     KEYCODE_ASTERISK,    IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "SHIFT (L)", KEYCODE_LSHIFT,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "z  Z",      KEYCODE_Z,           IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "x  X",      KEYCODE_X,           IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "c  C",      KEYCODE_C,           IP_JOY_NONE )

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "v  V",      KEYCODE_V,           IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "b  B",      KEYCODE_B,           IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "n  N",      KEYCODE_N,           IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "m  M",      KEYCODE_M,           IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, ",  <",      KEYCODE_COMMA,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ".  >",      KEYCODE_STOP,        IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "/  ?",      KEYCODE_SLASH,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "SHIFT (R)", KEYCODE_RSHIFT,      IP_JOY_NONE )

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "LINE FEED", IP_KEY_NONE,         IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "SPACE",     KEYCODE_SPACE,       IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "- (KP)",    KEYCODE_MINUS_PAD,   IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, ", (KP)",    KEYCODE_PLUS_PAD,    IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "ENTER (KP)",KEYCODE_ENTER_PAD,   IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ". (KP)",    KEYCODE_DEL_PAD,     IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0 (KP)",    KEYCODE_0_PAD,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1 (KP)",    KEYCODE_1_PAD,       IP_JOY_NONE )

	PORT_START /* KEY ROW 8 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2 (KP)",    KEYCODE_2_PAD,       IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3 (KP)",    KEYCODE_3_PAD,       IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4 (KP)",    KEYCODE_4_PAD,       IP_JOY_NONE )
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5 (KP)",    KEYCODE_5_PAD,       IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6 (KP)",    KEYCODE_6_PAD,       IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7 (KP)",    KEYCODE_7_PAD,       IP_JOY_NONE )
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "8 (KP)",    KEYCODE_8_PAD,       IP_JOY_NONE )
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "9 (KP)",    KEYCODE_9_PAD,       IP_JOY_NONE )

	PORT_START /* VIA #1 PORT A */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1					 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2					 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1					 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2					 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START /* tape control */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "TAPE STOP", KEYCODE_F5,          IP_JOY_NONE )
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "TAPE PLAY", KEYCODE_F6,          IP_JOY_NONE )
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "TAPE REW",  KEYCODE_F7,          IP_JOY_NONE )
	PORT_BIT (0xf8, 0x80, IPT_UNUSED)
INPUT_PORTS_END

static struct MachineDriver machine_driver_a310 =
{
	/* basic machine hardware */
	{
		{
			CPU_ARM,
			8000000,	/* 8 MHz */
			readmem,writemem,0,0,
			ignore_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* single CPU */
	a310_init_machine,
	NULL,					/* stop machine */

	/* video hardware - include overscan */
	32*8, 16*16, { 0*8, 32*8 - 1, 0*16, 16*16 - 1},
	NULL,
	2, 2,
	NULL,					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	a310_vh_start,
	a310_vh_stop,
	a310_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START(a310)
	ROM_REGION(0x00400000,REGION_CPU1)
		ROM_LOAD("ic24.rom", 0x00200000, 0x00080000, 0xc1adde84)
		ROM_LOAD("ic25.rom", 0x00280000, 0x00080000, 0x15d89664)
		ROM_LOAD("ic26.rom", 0x00300000, 0x00080000, 0xa81ceb7c)
		ROM_LOAD("ic27.rom", 0x00380000, 0x00080000, 0x707b0c6c)
	ROM_REGION(0x00800,REGION_GFX1)

ROM_END

static const struct IODevice io_a310[] = {
    { IO_END }
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	   FULLNAME */
COMP( 1988, a310,	  0,		a310,	  a310, 	a310,	  "Acorn","Archimedes 310" )



