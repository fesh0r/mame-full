/***************************************************************************
	mc10.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	MC-10 memory map (preliminary)

        0000-001F MC6803 internal registers
        0100-3FFF unused
        4000-41FF Video RAM
        4200-4FFF Internal RAM
        5000-BFFF External expansion RAM
        C000-DFFF External expansion ROM
        E000-FFFF BASIC ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6800/m6800.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

/* from machine/vz.c */

extern void mc10_init_machine(void);
extern void mc10_shutdown_machine(void);

extern int	mc10_rom_id(const char *name, const char * gamename);

extern int mc10_floppy_r(int offset);
extern void mc10_floppy_w(int offset, int data);

extern int mc10_joystick_r(int offset);
extern int mc10_keyboard_r(int offset);
extern void mc10_latch_w(int offset, int data);

/* from vidhrdw/vz.c */
extern int	mc10_vh_start(void);
extern void mc10_vh_stop(void);
extern void mc10_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x00ff, m6803_internal_registers_r },
	{ 0x0100, 0x3fff, MRA_NOP },
	{ 0x4000, 0x41ff, MRA_RAM },
	{ 0x4200, 0x4fff, MRA_RAM },
//	{ 0x5000, 0xbfff, MRA_RAM },	/* expansion RAM */
//	{ 0xc000, 0xdfff, MRA_ROM },	/* expansion ROM */
    { 0xe000, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x00ff, m6803_internal_registers_w },
	{ 0x0100, 0x3fff, MWA_NOP },
	{ 0x4000, 0x41ff, videoram_w, &videoram, &videoram_size },
	{ 0x4200, 0x4fff, MWA_RAM },
//	{ 0x5000, 0xbfff, MWA_RAM },	/* expansion RAM */
//	{ 0xc000, 0xdfff, MWA_ROM },	/* expansion ROM */
    { 0xe000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

INPUT_PORTS_START( vz )
	PORT_START /* IN0 */
	PORT_DIPNAME( 0x80, 0x00, "16K RAM module", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x80, "yes")
	PORT_DIPNAME( 0x40, 0x00, "DOS extension",  IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x40, "yes")
	PORT_BITX(	  0x20, 0x00, IPT_KEYBOARD | IPF_TOGGLE, "Joystick",  KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "no")
	PORT_DIPSETTING(	0x20, "yes")
	PORT_BITX(	  0x10, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset",   KEYCODE_F3, IP_JOY_NONE )
	PORT_BIT(	  0x0f, 0x0f, IPT_UNUSED )

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

static struct GfxLayout mc10_charlayout =
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

static struct GfxLayout mc10_gfxlayout =
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

static struct GfxDecodeInfo mc10_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mc10_charlayout, 0, 8 },
	{ 1, 0x0c00, &mc10_gfxlayout, 2*8, 2 },
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
	240,240,255,	/* 'buff' ?? */
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
static void mc10_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,vz200_palette,sizeof(vz200_palette));
	memcpy(sys_colortable,vz200_colortable,sizeof(vz200_colortable));
}


/* Kludge: patch a BASIC image or start a machine code image after some time */
static int mc10_image_load(void)
{
    static int framecount = 0;
    UINT8 *RAM = memory_region(REGION_CPU1);
	const char magic_basic[] = "VZF0";
    const char magic_mcode[] = "  \000\000";
	char name[17+1] = "";
	UINT16 start = 0, end = 0, addr, size;
	UINT8 type = 0;
    char buff[4];
    void *file;
    int i;

	if( ++framecount == 50 )
	{
		for( i = 0; i < MAX_ROM; i++ )
		{
			if( !rom_name[i] )
				continue;
			if( !rom_name[i][0] )
				continue;
			file = osd_fopen(Machine->gamedrv->name, rom_name[i], OSD_FILETYPE_IMAGE_RW, 0);
			if( file )
			{

				osd_fread(file, buff, sizeof(buff));
				if( memcmp(buff, magic_basic, sizeof(buff)) &&
					memcmp(buff, magic_mcode, sizeof(buff)) )
				{
					LOG((errorlog, "mc10_rom_load: magic not found\n"));
					return 1;
				}
				osd_fread(file, &name, 17);
				osd_fread(file, &type, 1);
				osd_fread_lsbfirst(file, &addr, 2);
				name[17] = '\0';
				LOG((errorlog, "mc10_rom_load: %s ($%02X) addr $%04X\n", name, type, addr));
				start = addr;
				size = osd_fread(file, &RAM[addr], 0xffff - addr);
				osd_fclose(file);
				addr += size;
                if( type == 0xf0 ) /* Basic image? */
					end = addr - 2;
                else
				if( type == 0xf1 ) /* mcode image? */
					end = addr;
			}
		}
		if( type == 0xf0 )
		{
			RAM[0x78a4] = start & 0xff;
			RAM[0x78a5] = start >> 8;
			RAM[0x78f9] = end & 0xff;
			RAM[0x78fa] = end >> 8;
			RAM[0x78fb] = end & 0xff;
			RAM[0x78fc] = end >> 8;
			RAM[0x78fd] = end & 0xff;
			RAM[0x78fe] = end >> 8;
        }
		else
		if( type == 0xf1 )
		{
			RAM[0x788e] = start & 0xff;
			RAM[0x788f] = start >> 8;
			cpu_set_pc(start);
		}
		return 1;
	}
	return 0;
}

static struct DACinterface mc10_DAC_interface =
{
    1,			/* number of DACs */
	{ 50 }		/* volume */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6803,
			890000, 	/* 0.89 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
	},
	60, 0,	/* frames per second, vblank duration */
	1,
	mc10_init_machine,
	mc10_shutdown_machine,

	/* video hardware */
	32*8,									/* screen width */
	312,									/* screen height */
    { 0*8, 32*8-1, 0*12, 16*12-1},          /* visible_area */
	mc10_gfxdecodeinfo, 					/* graphics decode info */
	9, 8*2 + 2*4,							/* colors used for the characters */
	mc10_init_palette,						/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_bitmapped_vh_start,
	generic_vh_stop,
	mc10_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&mc10_DAC_interface
		}
	}
};

ROM_START(mc10)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("mc10.rom", 0xe000, 0x2000, 0xffffffff)

	ROM_REGIONX(0x0d00,REGION_GFX1)
	ROM_LOAD("mc10.fnt", 0x0000, 0x0c00, 0xead006a1)
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
static const char *mc10_file_extensions[] = {
	"mc", NULL
};

static void mc10_rom_decode(void)
{
	int i;
	for( i = 0; i < 256; i++ )
		memory_region(REGION_GFX1)[0x0c00+i] = i;
}

struct GameDriver vz200_driver =
{
	__FILE__,
	0,
	"mc10",
	"MC-10",
	"1983",
	"Tandy Radio-Shack",
	NULL,
	0,
	&machine_driver,
	0,

	rom_mc10,
	NULL,					/* load rom_file images */
	mc10_rom_id,			/* identify rom images */
	mc10_file_extensions,	/* file extensions */
    1,                      /* number of ROM slots - in this case, a CMD binary */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	mc10_rom_decode,		/* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	input_ports_mc10,

	0,                      /* color_prom */
	0,						/* color palette */
	0,						/* color lookup table */

	GAME_COMPUTER | ORIENTATION_DEFAULT,	/* orientation */

	mc10_image_load,		/* hiscore load */
	0,                      /* hiscore save */
};

