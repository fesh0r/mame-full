/***************************************************************************
	vtech1.c

	Video Technology Models (series 1)
	Laser 110 monochrome
	Laser 210
		Laser 200 (same hardware?)
		aka VZ 200 (Australia)
		aka Salora Fellow (Finland)
		aka Texet8000 (UK)
	Laser 310
        aka VZ 300 (Australia)

    system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

	Laser 110 memory map (preliminary)

        0000-1FFF ROM 0 8K (VZ-200)
		2000-3FFF ROM 1 8K (VZ-200)

		4000-5FFF optional DOS ROM 8K
		6000-67FF reserved for rom cartridges (2k)
		6800-6FFF memory mapped I/O 2k
				  R: keyboard
				  W: cassette I/O, speaker, VDP control
		7000-77FF video RAM 2K
		7800-7FFF internal user RAM 2K (?)
		8800-C7FF 16K expansion

    Laser 210 / VZ 200 memory map (preliminary)

        0000-1FFF ROM 0 8K (VZ-200)
		2000-3FFF ROM 1 8K (VZ-200)

		4000-5FFF optional DOS ROM 8K
		6000-67FF reserved for rom cartridges (2k)
		6800-6FFF memory mapped I/O 2k
				  R: keyboard
				  W: cassette I/O, speaker, VDP control
		7000-77FF video RAM 2K
		7800-8FFF internal user RAM 6K
		9000-CFFF 16K expansion

	Laser 310 / VZ 300 memory map (preliminary)

		0000-3FFF ROM
		4000-5FFF optional DOS ROM 8K
		6000-67FF reserved for rom cartridges
		6800-6FFF memory mapped I/O
				  R: keyboard
				  W: cassette I/O, speaker, VDP control
		7000-77FF video RAM 2K
		7800-B7FF internal user RAM 16K
		B800-F7FF 16K expansion

    TODO:
		Add support for the 64K banked RAM extension.
		Add ROMs and drivers for the Laser100, 110, 210 and 310
		machines and the Texet 8000.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

/* from machine/vz.c */

extern int vtech1_latch;

extern void init_vtech1(void);
extern void laser110_init_machine(void);
extern void laser210_init_machine(void);
extern void laser310_init_machine(void);
extern void vtech1_shutdown_machine(void);

extern int vtech1_floppy_id(const char *name, const char * gamename);
extern int vtech1_floppy_init(int id, const char *name);
extern void vtech1_floppy_exit(int id);

extern int vtech1_cassette_id(const char *name, const char * gamename);
extern int vtech1_cassette_init(int id, const char *name);
extern void vtech1_cassette_exit(int id);

extern int vtech1_fdc_r(int offset);
extern void vtech1_fdc_w(int offset, int data);

extern int vtech1_joystick_r(int offset);
extern int vtech1_keyboard_r(int offset);
extern void vtech1_latch_w(int offset, int data);

/* from vidhrdw/vz.c */
extern int	vtech1_vh_start(void);
extern void vtech1_vh_stop(void);
extern void vtech1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static struct MemoryReadAddress readmem_laser110[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_ROM },
	{ 0x6800, 0x6fff, vtech1_keyboard_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0x7fff, MRA_RAM },
//	{ 0x8000, 0xbfff, MRA_RAM },	/* opt. installed */
	{ 0xc000, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_laser110[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_ROM },
	{ 0x6800, 0x6fff, vtech1_latch_w },
	{ 0x7000, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0x7fff, MWA_RAM },
//	{ 0x8000, 0xbfff, MWA_RAM },	/* opt. installed */
	{ 0xc000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_laser210[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_ROM },
	{ 0x6800, 0x6fff, vtech1_keyboard_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0x8fff, MRA_RAM },
//	{ 0x9000, 0xcfff, MRA_RAM },	/* opt. installed */
    { 0xd000, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_laser210[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_ROM },
	{ 0x6800, 0x6fff, vtech1_latch_w },
	{ 0x7000, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0x8fff, MWA_RAM },
//	{ 0x9000, 0xcfff, MWA_RAM },	/* opt. installed */
	{ 0xd000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_laser310[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_ROM },
	{ 0x6800, 0x6fff, vtech1_keyboard_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0xb7ff, MRA_RAM },
//	{ 0xb800, 0xf7ff, MRA_RAM },	/* opt. installed */
	{ 0xf800, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_laser310[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_ROM },
	{ 0x6800, 0x6fff, vtech1_latch_w },
	{ 0x7000, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0xb7ff, MWA_RAM },
//	{ 0xb800, 0xf7ff, MWA_RAM },	/* opt. installed */
    { 0xf800, 0xffff, MRA_NOP },
    { -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x10, 0x1f, vtech1_fdc_r },
	{ 0x20, 0x2f, vtech1_joystick_r },
    { -1 }
};

static struct IOWritePort writeport[] =
{
	{ 0x10, 0x1f, vtech1_fdc_w },
    { -1 }
};

INPUT_PORTS_START( vtech1 )
	PORT_START /* IN0 */
	PORT_DIPNAME( 0x80, 0x80, "16K RAM module")
	PORT_DIPSETTING(	0x00, DEF_STR( No ))
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ))
	PORT_DIPNAME( 0x40, 0x40, "DOS extension")
	PORT_DIPSETTING(	0x00, DEF_STR( No ))
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ))
	PORT_BIT(	  0x3e, 0x0f, IPT_UNUSED )
    PORT_BITX(    0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset", KEYCODE_F3, IP_JOY_NONE )

	PORT_START /* IN1 KEY ROW 0 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R       RETURN  LEFT$",   KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q       FOR     CHR$",    KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E       NEXT    LEN(",    KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W       TO      VAL(",    KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "T       THEN    MID$",    KEYCODE_T,          IP_JOY_NONE )

	PORT_START /* IN2 KEY ROW 1 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F       GOSUB   RND(",    KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A       MODE(   ASC(",    KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D               RESTORE", KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-CTRL",                  KEYCODE_LCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-CTRL",                  KEYCODE_RCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S       STEP    SINS(",   KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G       GOTO    STOP",    KEYCODE_G,          IP_JOY_NONE )

	PORT_START /* IN3 KEY ROW 2 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V       LPRINT  USR",     KEYCODE_V,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z       PEEK(   INP",     KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C       CONT    COPY",    KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT",                 KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT",                 KEYCODE_RSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "X       POKE    OUT",     KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "B       LLIST   SOUND",   KEYCODE_B,          IP_JOY_NONE )

	PORT_START /* IN4 KEY ROW 3 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $    VERFY   ATN(",    KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !    CSAVE   SIN(",    KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #    CALL??  TAN(",    KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                                         IP_KEY_NONE,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"    CLOAD   COS(",   KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %    LIST    LOG(",    KEYCODE_5,          IP_JOY_NONE )

	PORT_START /* IN5 KEY ROW 4 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M       [Left]",          KEYCODE_M,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE   [Down]",          KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ",       [Right]",         KEYCODE_COMMA,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".       [Up]",            KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N       COLOR   USING",   KEYCODE_N,          IP_JOY_NONE )

	PORT_START /* IN6 KEY ROW 5 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '    END     ???",     KEYCODE_7,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0  @    ????    INT(",    KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (    NEW     SQR(",    KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =    [Break]",         KEYCODE_MINUS,      IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )    READ    ABS(",    KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &    RUN     EXP(",    KEYCODE_6,          IP_JOY_NONE )

	PORT_START /* IN7 KEY ROW 6 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U       IF      INKEY$",  KEYCODE_U,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P       PRINT   NOT",     KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "I       INPUT   AND",     KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN  [Function]",      KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O       LET     OR",      KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y       ELSE    RIGHT$",  KEYCODE_Y,          IP_JOY_NONE )

	PORT_START /* IN8 KEY ROW 7 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "J       REM     RESET",   KEYCODE_J,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ";       [Rubout]",        KEYCODE_QUOTE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K       TAB(    PRINT",   KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":       [Inverse]",       KEYCODE_COLON,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L       [Insert]",        KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H       CLS     SET",     KEYCODE_H,          IP_JOY_NONE )

    PORT_START /* IN9 EXTRA KEYS */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Inverse)",               KEYCODE_LALT,       IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Rubout)",                KEYCODE_DEL,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor left)",           KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor down)",           KEYCODE_DOWN,       IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor right)",          KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Backspace)",             KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor up)",             KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Insert)",                KEYCODE_INSERT,     IP_JOY_NONE )

	PORT_START /* IN10 JOYSTICK #1 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_UP )

	PORT_START /* IN11 JOYSTICK #1 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON2 )
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN12 JOYSTICK #2 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP )

	PORT_START /* IN13 JOYSTICK #2 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,12,					/* 8 x 12 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,
	   6*8,  7*8,  8*8,  9*8, 10*8, 11*8 },
	8*12					/* every char takes 12 bytes */
};

static struct GfxLayout gfxlayout =
{
	8,3,					/* 4 times 2x3 pixels */
	256,					/* 256 codes */
	2,						/* 2 bits per pixel */
	{ 0,1 },				/* two bitplanes */
	/* x offsets */
	{ 0,0, 2,2, 4,4, 6,6 },
	/* y offsets */
	{  0, 0, 0	},
	8						/* one byte per code */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,	0, 10 },
	{ 1, 0x0c00, &gfxlayout, 2*10, 2 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	  0,  0,  0,	/* black (block graphics) */
	  0,224,  0,	/* green */
	208,255,  0,	/* yellow (greenish) */
	  0,  0,255,	/* blue */
	255,  0,  0,	/* red */
	224,224,144,	/* buff */
	  0,255,160,	/* cyan (greenish) */
	255,  0,255,	/* magenta */
	240,112,  0,	/* orange */
	  0, 64,  0,	/* dark green (alphanumeric characters) */
	  0,224, 24,	/* bright green (alphanumeric characters) */
	 64, 16,  0,	/* dark orange (alphanumeric characters) */
	255,196, 24,	/* bright orange (alphanumeric characters) */
};

static unsigned short colortable[] =
{
/* block graphics in text mode */
	 0, 1,		/* green */
	 0, 2,		/* yellow */
	 0, 3,		/* blue */
	 0, 4,		/* red */
	 0, 5,		/* white */
	 0, 6,		/* cyan */
	 0, 7,		/* magenta */
	 0, 8,		/* orange */
/* text in text mode */
	 9,10,		/* green on dark green */
	11,12,		/* yellow on dark orange */
/* graphics mode */
	1,2,3,4,	/* green, yellow, blue, red */
	5,6,8,7 	/* buff, cyan, orange, magenta */
};


/* Initialise the palette */
static void init_palette_monochrome(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	int i;
	for (i = 0; i < sizeof(palette)/sizeof(palette[0])/3; i++)
	{
		int mono;
		mono = palette[i*3+0] * 0.299 + palette[i*3+1] * 0.587 + palette[i*3+2] * 0.114;
		sys_palette[i*3+0] = mono;
		sys_palette[i*3+1] = mono;
		sys_palette[i*3+2] = mono;
	}
	memcpy(sys_colortable,colortable,sizeof(colortable));
}

/* Initialise the palette */
static void init_palette_color(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}



static struct DACinterface dac_interface =
{
    1,			/* number of DACs */
	{ 50 }	   /* volume */
};

static struct MachineDriver machine_driver_laser110 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3579500,	/* 3.57950 Mhz */
			readmem_laser110,writemem_laser110,
			readport,writeport,
			interrupt,1
		},
	},
	50, 0,	/* frames per second, vblank duration */
	1,
	laser110_init_machine,
	vtech1_shutdown_machine,

	/* video hardware */
	36*8,									/* screen width (inc. blank/sync) */
	38+192+25+1+6+13,						/* screen height (inc. blank/sync) */
	{ 0*8, 36*8-1, 0, 20+192+12-1}, 		/* visible_area */
	gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,	/* colors used for the characters */
	sizeof(colortable)/sizeof(colortable[0]),
	init_palette_monochrome,				/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	vtech1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_laser210 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3579500,	/* 3.57950 Mhz */
			readmem_laser210,writemem_laser210,
			readport,writeport,
			interrupt,1
		},
	},
	50, 0,	/* frames per second, vblank duration */
	1,
	laser210_init_machine,
	vtech1_shutdown_machine,

	/* video hardware */
	36*8,									/* screen width (inc. blank/sync) */
	38+192+25+1+6+13,						/* screen height (inc. blank/sync) */
	{ 0*8, 36*8-1, 0, 20+192+12-1}, 		/* visible_area */
	gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,	/* colors used for the characters */
	sizeof(colortable)/sizeof(colortable[0]),
	init_palette_color, 					/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	vtech1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_laser310 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			17734000/5, 					/* 17.734MHz / 5 = 3.54690 Mhz */
			readmem_laser310,writemem_laser310,
			readport,writeport,
			interrupt,1
		},
	},
	50, 0,									/* frames per second, vblank duration */
	1,
	laser310_init_machine,
	vtech1_shutdown_machine,

	/* video hardware */
	36*8,									/* screen width (inc. blank/sync) */
	38+192+25+1+6+13,						/* screen height (inc. blank/sync) */
    { 0*8, 36*8-1, 0, 20+192+12-1},         /* visible_area */
	gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette)/sizeof(palette[0])/3,	/* colors used for the characters */
    sizeof(colortable)/sizeof(colortable[0]),
	init_palette_color, 					/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	vtech1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START( laser110 )
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("vtechv20.lo",  0x0000, 0x2000, 0xcc854fe9)
	ROM_LOAD("vtectv20.hi",  0x2000, 0x2000, 0x7060f91a)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, 0xead006a1)
ROM_END


ROM_START( laser210 )
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("vtechv20.lo",  0x0000, 0x2000, 0xcc854fe9)
	ROM_LOAD("vtectv20.hi",  0x2000, 0x2000, 0x7060f91a)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, 0xead006a1)
ROM_END
#define rom_laser200	rom_laser210
#define rom_vz200		rom_laser210
#define rom_fellow		rom_laser210
#define rom_tx8000		rom_laser210

ROM_START( laser310 )
	ROM_REGIONX(0x10000,REGION_CPU1)
    ROM_LOAD("vtechv20.rom", 0x0000, 0x4000, 0x613de12c)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, 0xead006a1)
ROM_END
#define rom_vz300		rom_laser310

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const struct IODevice io_laser[] = {
    {
		IO_CASSETTE,			/* type */
		1,						/* count */
		"vz\0cas\0",            /* file extensions */
		NULL,					/* private */
		vtech1_cassette_id, 	/* id */
		vtech1_cassette_init,	/* init */
		vtech1_cassette_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
    },
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
		"dsk\0",                /* file extensions */
		NULL,					/* private */
		vtech1_floppy_id,		/* id */
		vtech1_floppy_init, 	/* init */
		vtech1_floppy_exit, 	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
    },
    { IO_END }
};

#define io_laser110 io_laser
#define io_laser210 io_laser
#define io_laser200 io_laser
#define io_vz200	io_laser
#define io_fellow	io_laser
#define io_tx8000	io_laser
#define io_laser310 io_laser
#define io_vz300	io_laser

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP ( 1983, laser110, 0,		 laser110, vtech1,	 vtech1,   "Video Technology", "Laser 110" )
COMP ( 1983, laser210, 0,		 laser210, vtech1,	 vtech1,   "Video Technology", "Laser 210" )
COMPX( 1983, laser200, laser210, laser210, vtech1,	 vtech1,   "Video Technology", "Laser 200", GAME_ALIAS )
COMPX( 1983, vz200,    laser210, laser210, vtech1,	 vtech1,   "Video Technology", "Sanyo / Dick Smith VZ200", GAME_ALIAS )
COMPX( 1983, fellow,   laser210, laser210, vtech1,	 vtech1,   "Video Technology", "Salora Fellow", GAME_ALIAS )
COMPX( 1983, tx8000,   laser210, laser210, vtech1,	 vtech1,   "Video Technology", "Texet TX8000", GAME_ALIAS )
COMP ( 1983, laser310, 0,		 laser310, vtech1,	 vtech1,   "Video Technology", "Laser 310" )
COMPX( 1983, vz300,    laser310, laser310, vtech1,	 vtech1,   "Video Technology", "Sanyo / Dick Smith VZ300", GAME_ALIAS )

