/***************************************************************************
	vz.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	Guy Thomason, Jason Oakley, Bushy Maunder and anybody else
	on the vzemu list :)

    VZ200 memory map (preliminary)

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

    VZ300 memory map (preliminary)

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
		Verify the colors and accuracy of the video modes
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

extern int vz_latch;

extern void vz_init_driver(void);
extern void vz200_init_machine(void);
extern void vz300_init_machine(void);
extern void vz_shutdown_machine(void);

extern int  vz_rom_id(const char *name, const char * gamename);

extern int vz_fdc_r(int offset);
extern void vz_fdc_w(int offset, int data);

extern int vz_joystick_r(int offset);
extern int vz_keyboard_r(int offset);
extern void vz_latch_w(int offset, int data);

/* from vidhrdw/vz.c */
extern int  vz_vh_start(void);
extern void vz_vh_stop(void);
extern void vz_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_ROM },
	{ 0x6800, 0x6fff, vz_keyboard_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0x8fff, MRA_RAM },
//	{ 0x9000, 0xcfff, MRA_RAM },	/* opt. installed */
    { 0xd000, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_ROM },
	{ 0x6800, 0x6fff, vz_latch_w },
	{ 0x7000, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0x8fff, MWA_RAM },
//	{ 0x9000, 0xcfff, MWA_RAM },	/* opt. installed */
	{ 0xd000, 0xffff, MWA_NOP },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_vz300[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_ROM },
	{ 0x6800, 0x6fff, vz_keyboard_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0xb7ff, MRA_RAM },
//	{ 0xb800, 0xf7ff, MRA_RAM },	/* opt. installed */
	{ 0xf800, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_vz300[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_ROM },
	{ 0x6800, 0x6fff, vz_latch_w },
	{ 0x7000, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7800, 0xb7ff, MWA_RAM },
//	{ 0xb800, 0xf7ff, MWA_RAM },	/* opt. installed */
    { 0xf800, 0xffff, MRA_NOP },
    { -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x10, 0x1f, vz_fdc_r },
    { 0x20, 0x2f, vz_joystick_r },
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{ 0x10, 0x1f, vz_fdc_w },
    { -1 }
};

INPUT_PORTS_START( vz )
	PORT_START /* IN0 */
	PORT_DIPNAME( 0x80, 0x80, "16K RAM module")
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x80, "yes")
	PORT_DIPNAME( 0x40, 0x40, "DOS extension")
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x40, "yes")
	PORT_BITX(	  0x20, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset",   KEYCODE_F3, IP_JOY_NONE )
	PORT_BIT(	  0x1f, 0x0f, IPT_UNUSED )

	PORT_START /* IN1 KEY ROW 0 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R       RETURN LEFT$",    KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q       FOR    CHR$",     KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E       NEXT   LEN(",     KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W       TO     VAL(",     KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "T       THEN   MID$",     KEYCODE_T,          IP_JOY_NONE )

	PORT_START /* IN2 KEY ROW 1 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F       GOSUB  RND(",     KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A       MODE(  ASC(",     KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D              RESTORE",  KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",                    KEYCODE_LCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",                    KEYCODE_RCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S       STEP   SINS(",    KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G       GOTO   STOP",     KEYCODE_G,          IP_JOY_NONE )

	PORT_START /* IN3 KEY ROW 2 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V       LPRINT  USR",     KEYCODE_V,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z       PEEK(   INP",     KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C       CONT    COPY",    KEYCODE_C,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",                   KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",                   KEYCODE_RSHIFT,     IP_JOY_NONE )
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
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M       Left",            KEYCODE_M,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE   Down",            KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ",       Right",           KEYCODE_COMMA,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".       Up",              KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N       COLOR   USING",   KEYCODE_N,          IP_JOY_NONE )

	PORT_START /* IN6 KEY ROW 5 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '    END     ???",     KEYCODE_7,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0  @    ????    INT(",    KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (    NEW     SQR(",    KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =    Break",           KEYCODE_MINUS,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )    READ    ABS(",    KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &    RUN     EXP(",    KEYCODE_6,          IP_JOY_NONE )

	PORT_START /* IN7 KEY ROW 6 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U       IF      INKEY$",  KEYCODE_U,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P       PRINT   NOT",     KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "I       INPUT   AND",     KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN  FUNCTION",        KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O       LET     OR",      KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y       ELSE    RIGHT$",  KEYCODE_Y,          IP_JOY_NONE )

	PORT_START /* IN8 KEY ROW 7 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "J       REM     RESET",   KEYCODE_J,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ";       RUBOUT",          KEYCODE_QUOTE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K       TAB(    PRINT",   KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":       INVERSE",         KEYCODE_COLON,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L       INSERT",          KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H       CLS     SET",     KEYCODE_H,          IP_JOY_NONE )

    PORT_START /* IN9 EXTRA KEYS */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Inverse",           KEYCODE_HOME,       IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Rubout",            KEYCODE_DEL,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Cursor left",       KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Cursor down",       KEYCODE_DOWN,       IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Cursor right",      KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Backspace",         KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Cursor up",         KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Extra Insert",            KEYCODE_INSERT,     IP_JOY_NONE )

    PORT_START /* JOYSTICK #1 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_UP )

	PORT_START /* JOYSTICK #1 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON2 )
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* JOYSTICK #2 */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP )

	PORT_START /* JOYSTICK #2 'Arm' */
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout vz_charlayout =
{
	8,12,					/* 8 x 12 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets triple height: use each line three times */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,
	   6*8,  7*8,  8*8,  9*8, 10*8, 11*8 },
	8*12					/* every char takes 12 bytes */
};

static struct GfxLayout vz_gfxlayout =
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

static struct GfxDecodeInfo vz_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &vz_charlayout, 0, 8 },
	{ 1, 0x0c00, &vz_gfxlayout, 2*8, 2 },
	{ -1 } /* end of array */
};


/* some colors for the DOS frontend and selectable fore-/background */
static unsigned char vz200_palette[9*3] =
{
	  0,  0,  0,	/* black */
	  0,240,  0,	/* green */
	240,240,  0,	/* yellow */
	  0,  0,240,	/* blue */
	240,  0,  0,	/* red */
	120,120,120,	/* 'buff' ?? */
	  0,240,240,	/* cyan */
	240,  0,240,	/* magenta */
	240,120,  0 	/* orange */
};

static unsigned short vz200_colortable[8*2+2*4] =
{
/* text mode */
    0,1,        /* green */
	0,2,		/* yellow */
	0,3,		/* blue */
	0,4,		/* red */
	0,5,		/* white */
	0,6,		/* cyan */
	0,7,		/* magenta */
	0,8,		/* orange */
/* graphics mode */
    1,2,3,4,    /* green,yellow,blue,red */
	5,6,8,7 	/* buff,cyan,orange,magenta */
};


/* Initialise the palette */
static void vz_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,vz200_palette,sizeof(vz200_palette));
	memcpy(sys_colortable,vz200_colortable,sizeof(vz200_colortable));
}



static struct DACinterface vz200_DAC_interface =
{
    1,			/* number of DACs */
	{ 50 }	   /* volume */
};

static struct MachineDriver machine_driver_vz200 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3579500,	/* 3.57950 Mhz */
			readmem,writemem,
			readport,writeport,
			interrupt,1
		},
	},
	50, 0,	/* frames per second, vblank duration */
	1,
	vz200_init_machine,
	vz_shutdown_machine,

	/* video hardware */
	36*8,									/* screen width (inc. blank/sync) */
	38+192+25+1+6+13,						/* screen height (inc. blank/sync) */
	{ 0*8, 36*8-1, 0, 20+192+12-1}, 		/* visible_area */
    vz_gfxdecodeinfo,                       /* graphics decode info */
	9, 8*2 + 2*4,							/* colors used for the characters */
	vz_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	vz_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&vz200_DAC_interface
		}
	}
};

static struct MachineDriver machine_driver_vz300 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			17734000/5, 					/* 17.734MHz / 5 = 3.54690 Mhz */
			readmem_vz300,writemem_vz300,
			readport,writeport,
			interrupt,1
		},
	},
	50, 0,									/* frames per second, vblank duration */
	1,
	vz300_init_machine,
	vz_shutdown_machine,

	/* video hardware */
	36*8,									/* screen width (inc. blank/sync) */
	38+192+25+1+6+13,						/* screen height (inc. blank/sync) */
    { 0*8, 36*8-1, 0, 20+192+12-1},         /* visible_area */
    vz_gfxdecodeinfo,                       /* graphics decode info */
	9, 8*2 + 2*4,							/* colors used for the characters */
    vz_init_palette,                        /* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	vz_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&vz200_DAC_interface
		}
	}
};

ROM_START(vz200)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("vzrom.lo",  0x0000, 0x2000, 0xcc854fe9)
    ROM_LOAD("vzrom.hi",  0x2000, 0x2000, 0x7060f91a)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("vz200.fnt", 0x0000, 0x0c00, 0xead006a1)
ROM_END


ROM_START(vz300)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("vzrom.v20", 0x0000, 0x4000, 0x613de12c)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("vz200.fnt", 0x0000, 0x0c00, 0xead006a1)
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
static const char *vz_file_extensions[] = {
	"vz", "dvz", NULL
};


struct GameDriver vz200_driver =
{
	__FILE__,
	0,
	"vz200",
	"VZ200",
	"1983",
	"Video Technology",
	NULL,
	0,
	&machine_driver_vz200,
	vz_init_driver,

	rom_vz200,
	NULL,					/* load rom_file images */
	vz_rom_id,				/* identify rom images */
	vz_file_extensions, 	/* file extensions */
	1,						/* number of ROM slots */
	2,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_vz,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};

struct GameDriver vz300_driver =
{
	__FILE__,
	&vz200_driver,
	"vz300",
	"VZ300",
	"1983",
	"Video Technology",
	NULL,
	0,
	&machine_driver_vz300,
	vz_init_driver,

	rom_vz300,
	NULL,					/* load rom_file images */
	vz_rom_id,				/* identify rom images */
	vz_file_extensions, 	/* file extensions */
	1,						/* number of ROM slots */
	2,						/* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,						/* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_vz,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,                      /* hiscore save */
};


