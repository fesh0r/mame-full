/***************************************************************************

	commodore vic20 home computer

***************************************************************************/
#include "mess/machine/vc20.h"
#include "mess/machine/6522via.h"
#include "mess/machine/vc1541.h"
#include "driver.h"

static struct MemoryReadAddress vc20_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
//	{ 0x0400, 0x0fff, MRA_RAM }, // ram, rom or nothing; I think read 0xff!
	{ 0x1000, 0x1fff, MRA_RAM },
//	{ 0x2000, 0x3fff, MRA_RAM }, // ram, rom or nothing
//	{ 0x4000, 0x5fff, MRA_RAM }, // ram, rom or nothing
//	{ 0x6000, 0x7fff, MRA_RAM }, // ram, rom or nothing
	{ 0x8000, 0x8fff, MRA_ROM },
	{ 0x9000, 0x900f, vic6560_port_r },
	{ 0x9010, 0x910f, MRA_NOP },
	{ 0x9110, 0x911f, via_0_r },
	{ 0x9120, 0x912f, via_1_r },
	{ 0x9130, 0x93ff, MRA_NOP },
	{ 0x9400, 0x97ff, MRA_RAM }, //color ram 4 bit
	{ 0x9800, 0x9fff, MRA_NOP },
//	{ 0xa000, 0xbfff, MRA_RAM }, // or nothing
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress vc20_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM,&vc20_memory },
	{ 0x1000, 0x1fff, MWA_RAM },
	{ 0x8000, 0x8fff, MWA_ROM },
	{ 0x9000, 0x900f, vic6560_port_w },
	{ 0x9010, 0x910f, MWA_NOP },
	{ 0x9110, 0x911f, via_0_w },
	{ 0x9120, 0x912f, via_1_w },
	{ 0x9130, 0x93ff, MWA_NOP },
	{ 0x9400, 0x97ff, vc20_write_9400,&vc20_memory_9400 },
	{ 0x9800, 0x9fff, MWA_NOP },
	{ 0xc000, 0xffff, MWA_NOP }, // MWA_ROM }, but logfile
	{ -1 }  /* end of table */
};

#define DIPS_BOTH \
	PORT_START\
	PORT_BIT( 0x80, IP_ACTIVE_LOW,	IPT_JOYSTICK_RIGHT | IPF_8WAY )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW,	IPT_UNUSED )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW,	IPT_BUTTON1)\
	PORT_BIT( 0x10, IP_ACTIVE_LOW,	IPT_JOYSTICK_LEFT | IPF_8WAY )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW,	IPT_JOYSTICK_DOWN | IPF_8WAY )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW,	IPT_JOYSTICK_UP | IPF_8WAY )\
	PORT_BIT( 0x03, IP_ACTIVE_LOW,	IPT_UNUSED )\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2, "Paddle 2 Button", KEYCODE_DEL, 0)\
	PORT_BIT ( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Paddle 1 Button", KEYCODE_INSERT, 0)\
	PORT_BIT ( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )\
	PORT_START \
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_REVERSE,30,20,0,0,255,KEYCODE_HOME,KEYCODE_PGUP,0,0)\
	PORT_START \
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_PLAYER2|IPF_REVERSE,30,20,0,0,255,KEYCODE_END,KEYCODE_PGDN,0,0)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "DEL INST",          KEYCODE_BACKSPACE,  IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Pound",             KEYCODE_MINUS,      IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, "+",                 KEYCODE_PLUS_PAD,   IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "9 )   RVS-ON",      KEYCODE_9,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "7 '   BLU",         KEYCODE_7,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "5 %   PUR",         KEYCODE_5,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "3 #   RED",         KEYCODE_3,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "1 !   BLK",         KEYCODE_1,          IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "RETURN",            KEYCODE_ENTER,      IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "*",                 KEYCODE_ASTERISK,   IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, "P",                 KEYCODE_P,          IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "I",                 KEYCODE_I,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Y",                 KEYCODE_Y,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "R",                 KEYCODE_R,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "W",                 KEYCODE_W,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Arrow-Left",        KEYCODE_TILDE,      IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "CRSR-RIGHT LEFT",   KEYCODE_6_PAD,      IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "; ]",               KEYCODE_QUOTE,      IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, "L",                 KEYCODE_L,          IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "J",                 KEYCODE_J,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "G",                 KEYCODE_G,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "D",                 KEYCODE_D,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "A",                 KEYCODE_A,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "CTRL",              KEYCODE_RCONTROL,   IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "CRSR-DOWN UP",      KEYCODE_2_PAD,      IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "/ ?",               KEYCODE_SLASH,      IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, ", <",               KEYCODE_COMMA,      IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "N",                 KEYCODE_N,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "V",                 KEYCODE_V,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "X",                 KEYCODE_X,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Left-Shift",        KEYCODE_LSHIFT,     IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "STOP RUN",          KEYCODE_TAB,        IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "f1 f2",             KEYCODE_F1,         IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Right-Shift",       KEYCODE_RSHIFT,     IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, ". >",               KEYCODE_STOP,       IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "M",                 KEYCODE_M,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "B",                 KEYCODE_B,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "C",                 KEYCODE_C,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Z",                 KEYCODE_Z,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Space",             KEYCODE_SPACE,      IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "f3 f4",             KEYCODE_F2,         IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "=",                 KEYCODE_BACKSLASH,  IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, ": [",               KEYCODE_COLON,      IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "K",                 KEYCODE_K,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "H",                 KEYCODE_H,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "F",                 KEYCODE_F,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "S",                 KEYCODE_S,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "CBM",               KEYCODE_RALT,       IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "f5 f6",             KEYCODE_F3,         IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Arrow-Up Pi",       KEYCODE_CLOSEBRACE, IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, "At",                KEYCODE_OPENBRACE,  IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "O",                 KEYCODE_O,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "U",                 KEYCODE_U,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "T",                 KEYCODE_T,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "E",                 KEYCODE_E,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "Q",                 KEYCODE_Q,          IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_KEYBOARD, "f7 f8",             KEYCODE_F4,         IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  IPT_KEYBOARD, "HOME CLR",          KEYCODE_EQUALS,     IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  IPT_KEYBOARD, "-",                 KEYCODE_MINUS_PAD,  IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  IPT_KEYBOARD, "0     RVS-OFF",     KEYCODE_0,          IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_KEYBOARD, "8 (   YEL",         KEYCODE_8,          IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  IPT_KEYBOARD, "6 &   GRN",         KEYCODE_6,          IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  IPT_KEYBOARD, "4 $   CYN",         KEYCODE_4,          IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD, "2 \"   WHT",        KEYCODE_2,          IP_JOY_NONE)\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPF_TOGGLE,   "SHIFT-LOCK (switch)",   KEYCODE_CAPSLOCK,   IP_JOY_NONE)\
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RESTORE",               KEYCODE_PRTSCR,     IP_JOY_NONE)\
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Special CRSR Up",       KEYCODE_8_PAD,      IP_JOY_NONE)\
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Special CRSR Left",     KEYCODE_4_PAD,      IP_JOY_NONE)\
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Quickload",       KEYCODE_F8,         IP_JOY_NONE)\
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape Drive Play",       KEYCODE_F5,         IP_JOY_NONE)\
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape Drive Record",     KEYCODE_F6,         IP_JOY_NONE)\
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape Drive Stop",       KEYCODE_F7,         IP_JOY_NONE)\
	PORT_START \
	PORT_DIPNAME ( 0x07, 0, "RAM Cartridge")\
	PORT_DIPSETTING(	0, "None" )\
	PORT_DIPSETTING(	1, "3k" )\
	PORT_DIPSETTING(	2, "8k" )\
	PORT_DIPSETTING(	3, "16k" )\
	PORT_DIPSETTING(	4, "32k" )\
	PORT_DIPSETTING(	5, "Custom" )\
	PORT_DIPNAME   ( 0x08, 0, " Ram at 0x0400 till 0x0fff")\
	PORT_DIPSETTING( 0x08, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x10, 0, " Ram at 0x2000 till 0x3fff")\
	PORT_DIPSETTING( 0x10, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x20, 0, " Ram at 0x4000 till 0x5fff")\
	PORT_DIPSETTING( 0x20, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x40, 0, " Ram at 0x6000 till 0x7fff")\
	PORT_DIPSETTING( 0x40, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x80, 0, " Ram at 0xa000 till 0xbfff")\
	PORT_DIPSETTING( 0x80, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_START \
	PORT_DIPNAME   ( 0x80, 0x80, "Joystick")\
	PORT_DIPSETTING( 0x80, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x40, 0x40, "Paddles")\
	PORT_DIPSETTING( 0x40, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x20, 0x00, "Lightpen")\
	PORT_DIPSETTING( 0x20, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x10, 0x10, " Draw Pointer")\
	PORT_DIPSETTING( 0x10, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x08, 0x08, "Tape Drive/Device 1")\
	PORT_DIPSETTING( 0x08, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME   ( 0x04, 0x00, " Noise")\
	PORT_DIPSETTING( 0x04, "Yes" )\
	PORT_DIPSETTING( 0x00, "No" )\
	PORT_DIPNAME ( 0x02, 0x02, "Serial/Dev 8/VC1541 Floppy")\
	PORT_DIPSETTING(0x02, "Yes" )\
	PORT_DIPSETTING(  0, "No" )\
	PORT_DIPNAME ( 0x01, 0x01, "Serial/Dev 9/VC1541 Floppy")\
	PORT_DIPSETTING(  1, "Yes" )\
	PORT_DIPSETTING(  0, "No" )\
	PORT_START \
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2, "Lightpen Signal", KEYCODE_LALT, 0)

INPUT_PORTS_START( vic20 )
	DIPS_BOTH
	PORT_START /* in 16	lightpen X */
	PORT_ANALOGX(0xff,0,IPT_PADDLE|IPF_PLAYER3,30,2,0,0,(VIC6560_MAME_XSIZE-1),KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
	PORT_START /* in 17	lightpen Y */
	PORT_ANALOGX(0xff,0,IPT_PADDLE|IPF_PLAYER4,30,2,0,0,(VIC6560_MAME_YSIZE-1),KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)

INPUT_PORTS_END

INPUT_PORTS_START( vc20 )
	DIPS_BOTH
	PORT_START /* in 16	lightpen X */
	PORT_ANALOGX(0xff,0,IPT_PADDLE|IPF_PLAYER3,30,2,0,0,(VIC6561_MAME_XSIZE-1),KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
	PORT_START /* in 17	lightpen Y */
	PORT_ANALOGX(0x1ff,0,IPT_PADDLE|IPF_PLAYER4,30,2,0,0,(VIC6561_MAME_YSIZE-1),KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)

INPUT_PORTS_END


/* Initialise the vc20 palette */
static void vc20_init_palette(unsigned char *sys_palette,
			      unsigned short *sys_colortable,
			      const unsigned char *color_prom)
{
	memcpy(sys_palette,vic6560_palette,sizeof(vic6560_palette));
//	memcpy(sys_colortable,colortable,sizeof(colortable));
}

// ntsc version
static struct MachineDriver vic20_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			VIC6560_CLOCK,
			vc20_readmem,vc20_writemem,
			0,0,
			vc20_frame_interrupt,1,
		  },
#ifdef VC1541
		VC1541_CPU(REGION_CPU2)
#endif
	},
	VIC6560_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	vc20_init_machine,
	vc20_shutdown_machine,

	/* video hardware */
	(VIC6560_XSIZE+7)&~7,							/* screen width */
	VIC6560_YSIZE,							/* screen height */
	{ VIC6560_MAME_XPOS,VIC6560_MAME_XPOS+VIC6560_MAME_XSIZE-1,
	  VIC6560_MAME_YPOS,VIC6560_MAME_YPOS+VIC6560_MAME_YSIZE-1 },
	0,	/* graphics decode info */
	sizeof(vic6560_palette) / sizeof(vic6560_palette[0]) / 3,
	0,
	vc20_init_palette,											/* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic6560_vh_start,
	vic6560_vh_stop,
	vic6560_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &vic6560_sound_interface },
		{ SOUND_DAC, &vc20tape_sound_interface }
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START(vic20)
     ROM_REGIONX(0x10000, REGION_CPU1)
     ROM_LOAD("basic",  0xc000, 0x2000, 0xdb4c43c1)
     ROM_LOAD("kernal",    0xe000, 0x2000, 0xe5e7c174)
     ROM_LOAD("chargen",     0x8000, 0x1000, 0x83e032a6)

#ifdef VC1541
     VC1541_ROM(REGION_CPU2)
#endif
ROM_END

struct GameDriver vic20_driver =
{
	__FILE__,
	0,
	"vic20",
	"Commodore VIC20 (ntsc)",
	"198?",
	"CBM",
	"Peter Trauner\n"
	"Marko Makela (MOS6560 Docu)",
	0,
	&vic20_machine_driver,
	vic20_driver_init,

	rom_vic20, 			/* rom module */
	0, //vc20_rom_load, // called from program, needs allocated memory
	vc20_rom_id,				/* identify rom images */
	0,	// file extensions
	2,						/* number of ROM slots */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	1,						/* number of cassette drives supported */
	0, 	/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports_vic20, 	/* input ports */

	0,						/* color_prom */
	0, // vic6560_palette,				/* color palette */
	0, 		/* color lookup table */

	GAME_COMPUTER|GAME_IMPERFECT_SOUND|ORIENTATION_DEFAULT,	/* flags */

	0,						/* hiscore load */
	0,						/* hiscore save */
};

/***************/
/* pal version */
/***************/
ROM_START(vc20)
	ROM_REGIONX(0x10000, REGION_CPU1)
	ROM_LOAD("basic",  0xc000, 0x2000, 0xdb4c43c1)
	ROM_LOAD("kernal.pal",    0xe000, 0x2000, 0x4be07cb4)
	ROM_LOAD("chargen",     0x8000, 0x1000, 0x83e032a6)
#ifdef VC1541
     VC1541_ROM(REGION_CPU2)
#endif
ROM_END

static struct MachineDriver vc20_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			VIC6561_CLOCK,
			vc20_readmem,vc20_writemem,
			0,0,
			vc20_frame_interrupt,1,
		  },
#ifdef VC1541
		VC1541_CPU(REGION_CPU2)
#endif
	},
	VIC6561_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	vc20_init_machine,
	vc20_shutdown_machine,

	/* video hardware */
	(VIC6561_XSIZE+7)&~7,					/* screen width */
	VIC6561_YSIZE,										/* screen height */
	{ VIC6561_MAME_XPOS,VIC6561_MAME_XPOS+VIC6561_MAME_XSIZE-1,
	  VIC6561_MAME_YPOS,VIC6561_MAME_YPOS+VIC6561_MAME_YSIZE-1 },					/* visible_area */
	0,						/* graphics decode info */
	sizeof(vic6560_palette) / sizeof(vic6560_palette[0]) / 3,
	0,
	vc20_init_palette,		/* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic6560_vh_start,
	vic6560_vh_stop,
	vic6560_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &vic6560_sound_interface },
		{ SOUND_DAC, &vc20tape_sound_interface }
	}
};

struct GameDriver vc20_driver =
{
	__FILE__,
	&vic20_driver,
	"vc20",
	"Commodore VC20 (pal)",
	"198?",
	"CBM",
	"Peter Trauner\n"
	"Marko Makela (MOS6560 Docu)",
	0,
	&vc20_machine_driver,
	vc20_driver_init,

	rom_vc20, 			/* rom module */
	0,//vc20_rom_load, // called from program, needs allocated memory
	vc20_rom_id,				/* identify rom images */
	0,	// file extensions
	2,						/* number of ROM slots */
	2,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	1,						/* number of cassette drives supported */
	0, 	/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	input_ports_vc20, 	/* input ports */

	0,						/* color_prom */
	0, //vic6560_palette,				/* color palette */
	0, 		/* color lookup table */

	GAME_COMPUTER|GAME_IMPERFECT_SOUND|ORIENTATION_DEFAULT,	/* flags */

	0,						/* hiscore load */
	0,						/* hiscore save */
};
