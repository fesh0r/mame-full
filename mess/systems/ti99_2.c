/*
  Experimental ti99/2 driver

TODO :
  * find a TI99/2 ROM dump (some TI99/2 ARE in private hands)
  * test the driver !
  * understand the "viden" pin
  * implement cassette
  * implement Hex-Bus

  Raphael Nabet (who really has too much time to waste), december 1999, 2000
*/

/*
  TI99/2 facts :

References :
* TI99/2 main logic board schematics, 83/03/28-30 (on ftp.whtech.com, or just ask me)
  (Thanks to Charles Good for posting this)

general :
* prototypes in 1983
* uses a 10.7MHz TMS9995 CPU, with the following features :
  - 8-bit data bus
  - 256 bytes 16-bit RAM (0xff00-0xff0b & 0xfffc-0xffff)
  - only available int lines are INT4 (used by vdp), INT1*, and NMI* (both free for extension)
  - on-chip decrementer (0xfffa-0xfffb)
  - Unlike tms9900, CRU address range is full 0x0000-0xFFFE (A0 is not used as address).
    This is possible because tms9995 uses d0-d2 instead of the address MSBits to support external
    opcodes.
  - quite more efficient than tms9900, and a few additionnal instructions and features
* 24 or 32kb ROM (16kb plain (1kb of which used by vdp), 16kb split into 2 8kb pages)
* 4kb 8-bit RAM, 256 bytes 16-bit RAM
* custom vdp shares CPU RAM/ROM.  The display is quite alike to tms9928 graphics mode, except
  that colors are a static B&W, and no sprite is displayed. The config (particularily the
  table base addresses) cannot be changed.  Since TI located the pattern generator table in
  ROM, we cannot even redefine the char patterns (unless you insert a custom cartidge which
  overrides the system ROMs).  VBL int triggers int4 on tms9995.
* CRU is handled by one single custom chip, so the schematics don't show many details :-( .
* I/O :
  - 48-key keyboard.  Same as TI99/4a, without alpha lock, and with an additional break key.
    Note that the hardware can make the difference between the two shift keys.
  - cassette I/O (one unit)
  - ALC bus (must be another name for Hex-Bus)
* 60-pin expansion/cartidge port on the back

memory map :
* 0x0000-0x4000 : system ROM (0x1C00-0x2000 (?) : char ROM used by vdp)
* 0x4000-0x6000 : system ROM, mapped to either of two distinct 8kb pages according to the S0
  bit from the keyboard interface (!), which is used to select the first key row.
  [only on second-generation TI99/2 protos, first generation protos only had 24kb of ROM]
* 0x6000-0xE000 : free for expansion
* 0xE000-0xF000 : 8-bit "system" RAM (0xEC00-0xEF00 used by vdp)
* 0xF000-0xF0FB : 16-bit processor RAM (on tms9995)
* 0xF0FC-0xFFF9 : free for expansion
* 0xFFFA-0xFFFB : tms9995 internal decrementer
* 0xFFFC-0xFFFF : 16-bit processor RAM (provides the NMI vector)

CRU map :
* 0x0000-0x1EFC : reserved
* 0x1EE0-0x1EFE : tms9995 flag register
* 0x1F00-0x1FD8 : reserved
* 0x1FDA : tms9995 MID flag
* 0x1FDC-0x1FFF : reserved
* 0x2000-0xE000 : unaffected
* 0xE400-0xE40E : keyboard I/O (8 bits input, either 3 or 6 bit output)
* 0xE80C : cassette I/O
* 0xE80A : ALC BAV
* 0xE808 : ALC HSK
* 0xE800-0xE808 : ALC data (I/O)
* 0xE80E : video enable (probably input - seems to come from the VDP, and is present on the
  expansion connector)
* 0xF000-0xFFFE : reserved
Note that only A15-A11 and A3-A1 (A15 = MSB, A0 = LSB) are decoded in the console, so the keyboard
is actually mapped to 0xE000-0xE7FE, and other I/O bits to 0xE800-0xEFFE.
Also, ti99/2 does not support external instructions better than ti99/4(a).  This is crazy, it
would just have taken three extra tracks on the main board and a OR gate in an ASIC.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/tms9901.h"
#include "cpu/tms9900/tms9900.h"

static int ROM_paged;

static void init_ti99_2_24(void)
{
	/* no ROM paging */
	ROM_paged = 0;
}

static void init_ti99_2_32(void)
{
	/* ROM paging enabled */
	ROM_paged = 1;
}

#define TI99_2_32_ROMPAGE0 (memory_region(REGION_CPU1)+0x4000)
#define TI99_2_32_ROMPAGE1 (memory_region(REGION_CPU1)+0x10000)

static void machine_init_ti99_2(void)
{
	if (! ROM_paged)
		cpu_setbank(1, memory_region(REGION_CPU1)+0x4000);
	else
		cpu_setbank(1, TI99_2_32_ROMPAGE0);
}

static void machine_stop_ti99_2(void)
{
}

static void ti99_2_vblank_interrupt(void)
{
	/* We trigger a level-4 interrupt.  The PULSE_LINE is a mere guess. */
	cpunum_set_input_line(0, 1, PULSE_LINE);
}


/*
  TI99/2 vdp emulation.

  Things could not be simpler.
  We display 24 rows and 32 columns of characters.  Each 8*8 pixel character pattern is defined
  in a 128-entry table located in ROM.  Character code for each screen position are stored
  sequentially in RAM.  Colors are a fixed Black on White.

	There is an EOL character that blanks the end of the current line, so that
	the CPU can get more bus time.
*/

static unsigned char ti99_2_palette[] =
{
	255, 255, 255,
	0, 0, 0
};

static unsigned short ti99_2_colortable[] =
{
	0, 1
};

#define TI99_2_PALETTE_SIZE sizeof(ti99_2_palette)/3
#define TI99_2_COLORTABLE_SIZE sizeof(ti99_2_colortable)/2

static PALETTE_INIT(ti99_2)
{
	palette_set_colors(0, ti99_2_palette, TI99_2_PALETTE_SIZE);
	memcpy(colortable, & ti99_2_colortable, sizeof(ti99_2_colortable));
}

static VIDEO_START(ti99_2)
{
	videoram_size = 768;

	return video_start_generic();
}

#define ti99_2_video_w videoram_w

static VIDEO_UPDATE(ti99_2)
{
	int i, sx, sy;


	sx = sy = 0;

	for (i = 0; i < 768; i++)
	{
		if (dirtybuffer[i])
		{
			dirtybuffer[i] = 0;

			/* Is the char code masked or not ??? */
			drawgfx(tmpbitmap, Machine->gfx[0], videoram[i] & 0x7F, 0,
			          0, 0, sx, sy, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		}

		sx += 8;
		if (sx == 256)
		{
			sx = 0;
			sy += 8;
		}
	}

	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}

static struct GfxLayout ti99_2_charlayout =
{
	8,8,        /* 8 x 8 characters */
	128,        /* 128 characters */
	1,          /* 1 bits per pixel */
	{ 0 },      /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, },
	8*8         /* every char takes 8 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x1c00, & ti99_2_charlayout, 0, 0 },
	{ -1 }    /* end of array */
};


/*
  Memory map - see description above
*/

static ADDRESS_MAP_START(ti99_2_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)		/*system ROM*/
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)	/*system ROM, banked on 32kb ROMs protos*/
	AM_RANGE(0x6000, 0xdfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/*free for expansion*/
	AM_RANGE(0xe000, 0xebff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/*system RAM*/
	AM_RANGE(0xec00, 0xeeff) AM_READWRITE(MRA8_RAM, ti99_2_video_w) AM_BASE(& videoram)	/*system RAM: used for video*/
	AM_RANGE(0xef00, 0xefff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/*system RAM*/
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/*free for expansion (and internal processor RAM)*/

ADDRESS_MAP_END


/*
  CRU map - see description above
*/

/* current keyboard row */
static int KeyRow = 0;

/* write the current keyboard row */
static WRITE8_HANDLER ( ti99_2_write_kbd )
{
	offset &= 0x7;  /* other address lines are not decoded */

	if (offset <= 2)
	{
		/* this implementation is just a guess */
		if (data)
			KeyRow |= 1 << offset;
		else
			KeyRow &= ~ (1 << offset);
	}
	/* now, we handle ROM paging */
	if (ROM_paged)
	{	/* if we have paged ROMs, page according to S0 keyboard interface line */
		cpu_setbank(1, (KeyRow == 0) ? TI99_2_32_ROMPAGE1 : TI99_2_32_ROMPAGE0);
	}
}

static WRITE8_HANDLER ( ti99_2_write_misc_cru )
{
	offset &= 0x7;  /* other address lines are not decoded */

	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		/* ALC I/O */
		break;
	case 4:
		/* ALC HSK */
		break;
	case 5:
		/* ALC BAV */
		break;
	case 6:
		/* cassette output */
		break;
	case 7:
		/* video enable */
		break;
	}
}

static ADDRESS_MAP_START(ti99_2_writecru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x7000, 0x73ff) AM_WRITE(ti99_2_write_kbd)
	AM_RANGE(0x7400, 0x77ff) AM_WRITE(ti99_2_write_misc_cru)

ADDRESS_MAP_END

/* read keys in the current row */
static  READ8_HANDLER ( ti99_2_read_kbd )
{
	return readinputport(KeyRow);
}

static  READ8_HANDLER ( ti99_2_read_misc_cru )
{
	return 0;
}

static ADDRESS_MAP_START(ti99_2_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x0E00, 0x0E7f) AM_READ(ti99_2_read_kbd)
	AM_RANGE(0x0E80, 0x0Eff) AM_READ(ti99_2_read_misc_cru)

ADDRESS_MAP_END


/* ti99/2 : 54-key keyboard */
INPUT_PORTS_START(ti99_2)

	PORT_START    /* col 0 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)

	PORT_START    /* col 1 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)

	PORT_START    /* col 2 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)

	PORT_START    /* col 3 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

	PORT_START    /* col 4 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT/*KEYCODE_CAPSLOCK*/, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START    /* col 5 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_ESC, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)

	PORT_START    /* col 6 */
		PORT_BITX(0xFF, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 7 */
		PORT_BITX(0xFF, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

INPUT_PORTS_END


static struct tms9995reset_param ti99_2_processor_config =
{
#if 0
	REGION_CPU1,/* region for processor RAM */
	0xf000,     /* offset : this area is unused in our region, and matches the processor address */
	0xf0fc,		/* offset for the LOAD vector */
	NULL,       /* no IDLE callback */
	1,          /* use fast IDLE */
#endif
	1           /* enable automatic wait state generation */
};

static MACHINE_DRIVER_START(ti99_2)

	/* basic machine hardware */
	/* TMS9995 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10700000)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(ti99_2_processor_config)
	MDRV_CPU_PROGRAM_MAP(ti99_2_memmap, 0)
	MDRV_CPU_IO_MAP(ti99_2_readcru, ti99_2_writecru)
	MDRV_CPU_VBLANK_INT(ti99_2_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99_2 )
	MDRV_MACHINE_STOP( ti99_2 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	/*MDRV_TMS9928A( &tms9918_interface )*/
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 192-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(TI99_2_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(TI99_2_COLORTABLE_SIZE)
	MDRV_PALETTE_INIT(ti99_2)

	MDRV_VIDEO_START(ti99_2)
	MDRV_VIDEO_UPDATE(ti99_2)

	/* no sound! */

MACHINE_DRIVER_END



/*
  ROM loading
*/
ROM_START(ti99_224)
	/*CPU memory space*/
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("992rom.bin", 0x0000, 0x6000, NO_DUMP)      /* system ROMs */
ROM_END

ROM_START(ti99_232)
	/*64kb CPU memory space + 8kb to read the extra ROM page*/
	ROM_REGION(0x12000,REGION_CPU1,0)
	ROM_LOAD("992rom32.bin", 0x0000, 0x6000, NO_DUMP)    /* system ROM - 32kb */
	ROM_CONTINUE(0x10000,0x2000)
ROM_END

SYSTEM_CONFIG_START(ti99_2)
	/* one expansion/cartridge port on the back */
	/* one cassette unit port */
	/* Hex-bus disk controller: supports up to 4 floppy disk drives */
	/* None of these is supported (tape should be easy to emulate) */
SYSTEM_CONFIG_END

/*		YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT	INIT		CONFIG		COMPANY					FULLNAME */
COMP(	1983,	ti99_224,	0,			0,		ti99_2,		ti99_2,	ti99_2_24,	ti99_2,		"Texas Instruments",	"TI-99/2 BASIC Computer (24kb ROMs)" )
COMP(	1983,	ti99_232,	ti99_224,	0,		ti99_2,		ti99_2,	ti99_2_32,	ti99_2,		"Texas Instruments",	"TI-99/2 BASIC Computer (32kb ROMs)" )
