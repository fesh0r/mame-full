/* Oric 1 & Oric Atmos Machine Drivers ..... */

#include "driver.h"
#include "vidhrdw/generic.h"

/* from machine/oric.c */

void oric_init_machine (void);
void oric_shutdown_machine (void);
int oric_load_rom (int id);
int oric_interrupt(void);
READ_HANDLER ( oric_IO_r );
WRITE_HANDLER ( oric_IO_w );

/* from vidhrdw/oric.c */
void oric_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int oric_vh_start(void);
void oric_vh_stop(void);

extern unsigned char *oric_IO;

// int oric_ram_r (int offset);
// void oric_ram_w (int offset, int data);

static struct MemoryReadAddress oric_readmem[] =
{
    /* { 0x0000, 0x04ff, oric_io_r, &oric_ram }, */
    /* { 0x0000, 0xBFFF, oric_ram_r, &oric_ram }, */
    { 0x0000, 0x02FF, MRA_RAM },
    { 0x0300, 0x03ff, oric_IO_r },
    { 0x0400, 0xBFFF, MRA_RAM },
    { 0xc000, 0xFFFF, MRA_ROM },
    { -1 }
};

static struct MemoryWriteAddress oric_writemem[] =
{
    /* { 0x0000, 0x04ff, oric_IO_w, &oric_ram }, */
    /* { 0x0000, 0xbFFF, oric_ram_w, &oric_ram }, */
    { 0x0000, 0x02FF, MWA_RAM },
    { 0x0300, 0x03ff, oric_IO_w },
    { 0x0400, 0xbFFF, MWA_RAM },
    { 0xc000, 0xFFFF, MWA_ROM },
    { -1 }
};

INPUT_PORTS_START( oric )
	PORT_START /* IN0 */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "0.7: 3 #",      KEYCODE_3,          IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0.6: x X",      KEYCODE_X,          IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "0.5: 1 !",      KEYCODE_1,          IP_JOY_NONE)
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "0.3: v V",      KEYCODE_V,          IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0.2: 5 %",      KEYCODE_5,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "0.1: n N",      KEYCODE_N,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "0.0: 7 ^",      KEYCODE_7,          IP_JOY_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1.7: d D",      KEYCODE_D,          IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "1.6: q Q",      KEYCODE_Q,          IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "1.5: ESC",      KEYCODE_ESC,        IP_JOY_NONE)
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "1.3: f F",      KEYCODE_F,          IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "1.2: r R",      KEYCODE_R,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1.1: t T",      KEYCODE_T,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "1.0: j J",      KEYCODE_J,          IP_JOY_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "2.7: c C",      KEYCODE_C,          IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "2.6: 2 @",      KEYCODE_2,          IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "2.5: z Z",      KEYCODE_Z,          IP_JOY_NONE)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "2.4: CTL",      KEYCODE_LCONTROL,   IP_JOY_NONE)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "2.3: 4 $",      KEYCODE_4,          IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2.2: b B",      KEYCODE_B,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "2.1: 6 &",      KEYCODE_6,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2.0: m M",      KEYCODE_M,          IP_JOY_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "3.7: ' \"",     KEYCODE_QUOTE,      IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "3.6: \\ |",     KEYCODE_BACKSLASH,  IP_JOY_NONE)
	PORT_BIT (0x20, 0x00, IPT_UNUSED)
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3.3: - œ",      KEYCODE_MINUS,      IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "3.2: ; :",      KEYCODE_COLON,      IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3.1: 9 (",      KEYCODE_9,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "3.0: k K",      KEYCODE_K,          IP_JOY_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "4.7: RIGHT",    KEYCODE_RIGHT,      IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "4.6: DOWN",     KEYCODE_DOWN,       IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "4.5: LEFT",     KEYCODE_LEFT,       IP_JOY_NONE)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4.4: LSHIFT",   KEYCODE_LSHIFT,     IP_JOY_NONE)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "4.3: UP",       KEYCODE_UP,         IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4.2: . <",      KEYCODE_STOP,       IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "4.1: , >",      KEYCODE_COMMA,      IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "4.0: SPACE",    KEYCODE_SPACE,      IP_JOY_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "5.7: [ {",      KEYCODE_OPENBRACE,  IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "5.6: ] }",      KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5.5: DEL",      KEYCODE_BACKSPACE,  IP_JOY_NONE)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "5.4: FCT",      IP_KEY_NONE,        IP_JOY_NONE)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5.3: p P",      KEYCODE_P,          IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "5.2: o O",      KEYCODE_O,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "5.1: i I",      KEYCODE_I,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "5.0: u U",      KEYCODE_U,          IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "6.7: w W",      KEYCODE_W,          IP_JOY_NONE)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6.6: s S",      KEYCODE_S,          IP_JOY_NONE)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "6.5: a A",      KEYCODE_A,          IP_JOY_NONE)
	PORT_BIT (0x10, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "6.3: e E",      KEYCODE_E,          IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "6.2: g G",      KEYCODE_G,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "6.1: h H",      KEYCODE_H,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "6.0: y Y",      KEYCODE_Y,          IP_JOY_NONE)

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7.7: = +",      KEYCODE_EQUALS,     IP_JOY_NONE)
	PORT_BIT (0x40, 0x00, IPT_UNUSED)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7.5: RETURN",   KEYCODE_ENTER,      IP_JOY_NONE)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "7.4: RSHIFT",   KEYCODE_RSHIFT,     IP_JOY_NONE)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "7.3: / ?",      KEYCODE_SLASH,      IP_JOY_NONE)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "7.2: 0 )",      KEYCODE_0,          IP_JOY_NONE)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "7.1: l L",      KEYCODE_L,          IP_JOY_NONE)
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "7.0: 8 *",      KEYCODE_8,          IP_JOY_NONE)
INPUT_PORTS_END

static unsigned char oric_palette[16*3] = {
	0x00, 0x00, 0x00, 0xcf, 0x00, 0x00,
	0x00, 0xcf, 0x00, 0xcf, 0xcf, 0x00,
	0x00, 0x00, 0xcf, 0xcf, 0x00, 0xcf,
	0x00, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf,

	0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
	0x00, 0xff, 0x00, 0xff, 0xff, 0x00,
	0x00, 0x00, 0xff, 0xff, 0x00, 0xff,
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static unsigned short oric_colortable[128*2] = {
	 0,0,  0,1,  0, 2,	0, 3,  0, 4,  0, 5,  0, 6,	0, 7,
	 1,0,  2,1,  2, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 2,0,  4,1,  4, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 3,0,  6,1,  6, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 4,0,  1,1,  1, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 5,0,  3,1,  3, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 6,0,  6,1,  6, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,
	 7,0,  7,1,  7, 2,	1, 3,  1, 4,  1, 5,  1, 6,	1, 7,

	 8,8,  8,9,  8,10,	8,11,  8,12,  8,13,  8,14,	8,15,
	 9,8,  9,9,  9,10,	9,11,  9,12,  9,13,  9,14,	9,15,
	10,8, 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8, 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8, 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8, 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8, 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8, 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};

/* Initialise the palette */
static void oric_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,oric_palette,sizeof(oric_palette));
	memcpy(sys_colortable,oric_colortable,sizeof(oric_colortable));
}

/* read PSG port A */
READ_HANDLER ( oric_psg_porta_read )
{
	return 0;
}


static struct AY8910interface oric_ay_interface =
{
	1,	/* 1 chips */
	1000000,	/* 1.0 MHz  */
	{ 25,25},
	{ oric_psg_porta_read },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver_oric =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
            1000000,
			oric_readmem,oric_writemem,0,0,
			oric_interrupt, 10,
			0, 0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	oric_init_machine, /* init_machine */
	oric_shutdown_machine, /* stop_machine */

	/* video hardware */
	40*6,								/* screen width */
	28*8,								/* screen height */
	{ 0, 40*6-1, 0, 28*8-1},			/* visible_area */
	NULL,								/* graphics decode info */
	16, 256,							/* colors used for the characters */
	oric_init_palette,					/* convert color prom */

	VIDEO_TYPE_RASTER,


	0,
	oric_vh_start,
	oric_vh_stop,
	oric_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&oric_ay_interface
		}
	}
};


ROM_START(oric1)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD ("oric1.rom", 0xc000, 0x4000, 0xf18710b4)
ROM_END

ROM_START(orica)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD ("orica.rom", 0xc000, 0x4000, 0xc3a92bef)
ROM_END

static const struct IODevice io_oric1[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"tap\0",            /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		oric_load_rom,		/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

static const struct IODevice io_orica[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"tap\0",            /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		oric_load_rom,		/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*    YEAR   NAME      PARENT    MACHINE   INPUT     INIT      COMPANY      FULLNAME */
COMP( 1983,  oric1,    0,		 oric,	   oric,	 0,		   "Tangerine", "Oric 1" )
COMP( 1984,  orica,    0,		 oric,	   oric,	 0,		   "Tangerine", "Oric Atmos" )

