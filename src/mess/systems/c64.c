/***************************************************************************

	commodore c64 home computer

***************************************************************************/
#include "driver.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"
#include "mess/machine/cia6526.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/sndhrdw/sid6581.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"

#include "mess/machine/c64.h"

static struct MemoryReadAddress ultimax_readmem[] =
{
	{ 0x0000, 0x0001, c64_m6510_port_r },
	{ 0x0002, 0x0fff, MRA_RAM },
        { 0x8000, 0x9fff, MRA_ROM },
	{ 0xd000, 0xd3ff, vic2_port_r },
	{ 0xd400, 0xd7ff, sid6581_port_r },
	{ 0xd800, 0xdbff, MRA_RAM }, /* colorram  */
	{ 0xdc00, 0xdcff, cia6526_0_port_r },
	{ 0xe000, 0xffff, MRA_ROM }, /* ram or kernel rom */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ultimax_writemem[] =
{
	{ 0x0000, 0x0001, c64_m6510_port_w, &c64_memory },
	{ 0x0002, 0x0fff, MWA_RAM },
	{ 0x8000, 0x9fff, MWA_ROM, &c64_roml },
	{ 0xd000, 0xd3ff, vic2_port_w },
	{ 0xd400, 0xd7ff, sid6581_port_w },
	{ 0xd800, 0xdbff, c64_colorram_write, &c64_colorram },
	{ 0xdc00, 0xdcff, cia6526_0_port_w },
	{ 0xe000, 0xffff, MWA_ROM, &c64_romh },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress c64_readmem[] =
{
	{ 0x0000, 0x0001, c64_m6510_port_r },
	{ 0x0002, 0x7fff, MRA_RAM },
	{ 0x8000, 0x9fff, MRA_BANK1 }, /* ram or external roml */
	{ 0xa000, 0xbfff, MRA_BANK3 }, /* ram or basic rom or external romh */
	{ 0xc000, 0xcfff, MRA_RAM },
#if 1
	{ 0xd000, 0xdfff, MRA_BANK5 },
#else
/* dram */
/* or character rom */
	{ 0xd000, 0xd3ff, vic2_port_r },
	{ 0xd400, 0xd7ff, sid6581_port_r },
	{ 0xd800, 0xdbff, MRA_RAM }, /* colorram  */
	{ 0xdc00, 0xdcff, cia6526_0_port_r },
	{ 0xdd00, 0xddff, cia6526_1_port_r },
        { 0xde00, 0xdeff, MRA_NOP }, /* csline expansion port */
        { 0xdf00, 0xdfff, MRA_NOP }, /* csline expansion port */
#endif
	{ 0xe000, 0xffff, MRA_BANK7 }, /* ram or kernel rom or external romh*/
	{ 0x10000, 0x11fff, MRA_ROM }, /* basic at 0xa000 */
	{ 0x12000, 0x13fff, MRA_ROM }, /* kernal at 0xe000 */
	{ 0x14000, 0x14fff, MRA_ROM }, /* charrom at 0xd000 */
        { 0x15000, 0x153ff, MRA_RAM }, /* colorram at 0xd800 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress c64_writemem[] =
{
	{ 0x0000, 0x0001, c64_m6510_port_w, &c64_memory },
	{ 0x0002, 0x7fff, MWA_RAM },
	{ 0x8000, 0x9fff, MWA_BANK2 },
	{ 0xa000, 0xcfff, MWA_RAM },
#if 1
	{ 0xd000, 0xdfff, MWA_BANK6 },
#else
	/* or dram memory */
	{ 0xd000, 0xd3ff, vic2_port_w },
	{ 0xd400, 0xd7ff, sid6581_port_w },
	{ 0xd800, 0xdbff, c64_colorram_write },
	{ 0xdc00, 0xdcff, cia6526_0_port_w },
	{ 0xdd00, 0xddff, cia6526_1_port_w },
        { 0xde00, 0xdeff, MWA_NOP }, /* csline expansion port */
        { 0xdf00, 0xdfff, MWA_NOP }, /* csline expansion port */
#endif
	{ 0xe000, 0xffff, MWA_BANK8 },
	{ 0x10000, 0x11fff, MWA_ROM, &c64_basic }, /* basic at 0xa000 */
	{ 0x12000, 0x13fff, MWA_ROM, &c64_kernal }, /* kernal at 0xe000 */
	{ 0x14000, 0x14fff, MWA_ROM, &c64_chargen }, /* charrom at 0xd000 */
        { 0x15000, 0x153ff, MWA_RAM, &c64_colorram }, /* colorram at 0xd800 */
	{ 0x16000, 0x17fff, MWA_RAM, &c64_roml }, /* roml at 0xa000 */
	{ 0x18000, 0x19fff, MWA_RAM, &c64_romh }, /* romh at 0xe000 */
	{ 0x1a000, 0x55fff, MWA_ROM }, /* additional space for roms */
	{ -1 }  /* end of table */
};

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define INPUTPORTS_BOTH \
     PORT_START \
     PORT_BIT( 0x800, IP_ACTIVE_HIGH, IPT_BUTTON1) \
     PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )\
     PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )\
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY ) \
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )\
	PORT_BITX( 8, IP_ACTIVE_HIGH, IPT_BUTTON1|IPF_PLAYER2, \
		   "P2 Button", KEYCODE_LALT,JOYCODE_2_BUTTON1 )\
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, \
		   IPT_JOYSTICK_LEFT|IPF_PLAYER2 | IPF_8WAY,\
		   "P2 Left",KEYCODE_DEL,JOYCODE_2_LEFT )\
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, \
		   IPT_JOYSTICK_RIGHT|IPF_PLAYER2 | IPF_8WAY,\
		   "P2 Right",KEYCODE_PGDN,JOYCODE_2_RIGHT )\
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, \
		   IPT_JOYSTICK_UP|IPF_PLAYER2 | IPF_8WAY,\
		   "P2 Up", KEYCODE_HOME, JOYCODE_2_UP)\
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, \
		   IPT_JOYSTICK_DOWN|IPF_PLAYER2 | IPF_8WAY,\
		   "P2 Down", KEYCODE_END, JOYCODE_2_DOWN)\
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )\
	PORT_START \
	PORT_BITX( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON1, \
		   "Paddle 1 Button", KEYCODE_LCONTROL, 0)\
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_REVERSE,\
		     30,20,0,0,255,KEYCODE_LEFT,KEYCODE_RIGHT,\
		     JOYCODE_1_LEFT,JOYCODE_1_RIGHT)\
	PORT_START \
	PORT_BITX( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON2, \
		   "Paddle 2 Button", KEYCODE_LALT, 0)\
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_PLAYER2|IPF_REVERSE,\
		     30,20,0,0,255,KEYCODE_DOWN,KEYCODE_UP,0,0)\
	PORT_START \
	PORT_BITX( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON3, \
		   "Paddle 3 Button", KEYCODE_INSERT, 0)\
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_PLAYER3|IPF_REVERSE,\
		     30,20,0,0,255,KEYCODE_HOME,KEYCODE_PGUP,0,0)\
     PORT_START \
	PORT_BITX( 0x100, IP_ACTIVE_HIGH, IPT_BUTTON4, \
		   "Paddle 4 Button", KEYCODE_DEL, 0)\
	PORT_ANALOGX(0xff,128,IPT_PADDLE|IPF_PLAYER4|IPF_REVERSE,\
		     30,20,0,0,255,KEYCODE_END,KEYCODE_PGDN,\
		     JOYCODE_1_UP,JOYCODE_1_DOWN)\
	PORT_START \
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON2, \
		   "Lightpen Signal", KEYCODE_LCONTROL, 0)\
     PORT_ANALOGX(0x1ff,0,IPT_PADDLE|IPF_PLAYER1,\
		  30,2,0,0,320-1,KEYCODE_LEFT,KEYCODE_RIGHT,\
		  JOYCODE_1_LEFT,JOYCODE_1_RIGHT)\
     PORT_START \
     PORT_ANALOGX(0xff,0,IPT_PADDLE|IPF_PLAYER2,\
		  30,2,0,0,200-1,KEYCODE_UP,KEYCODE_DOWN,\
		  JOYCODE_1_UP,JOYCODE_1_DOWN)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Arrow-Left", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "1 !   BLK   ORNG", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 \"   WHT   BRN", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #   RED   L RED", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $   CYN   D GREY", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %   PUR   GREY", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 &   GRN   L GRN", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 '   BLU   L BLU", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 (   YEL   L GREY", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 )   RVS-ON", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0     RVS-OFF", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "+", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0008, "-", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x0004, "Pound", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0002, "HOME CLR", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0001, "DEL INST", KEYCODE_BACKSPACE)\
	PORT_START \
	DIPS_HELPER( 0x8000, "CTRL", KEYCODE_RCONTROL)\
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0010, "At", KEYCODE_OPENBRACE)\
        DIPS_HELPER( 0x0008, "*", KEYCODE_ASTERISK)\
        DIPS_HELPER( 0x0004, "Arrow-Up Pi",KEYCODE_CLOSEBRACE)\
        DIPS_HELPER( 0x0002, "RESTORE", KEYCODE_PRTSCR)\
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_TAB)\
	PORT_START \
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPF_TOGGLE,\
		     "SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE)\
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)\
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)\
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)\
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)\
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0020, ": [", KEYCODE_COLON)\
	DIPS_HELPER( 0x0010, "; ]", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0008, "=", KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)\
	DIPS_HELPER( 0x0002, "CBM", KEYCODE_RALT)\
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)\
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)\
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)\
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)\
	DIPS_HELPER( 0x0100, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x0080, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0010, "CRSR-DOWN UP", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0008, "CRSR-RIGHT LEFT", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0002, "f1 f2", KEYCODE_F1)\
	DIPS_HELPER( 0x0001, "f3 f4", KEYCODE_F2)\
	PORT_START \
	DIPS_HELPER( 0x8000, "f5 f6", KEYCODE_F3)\
	DIPS_HELPER( 0x4000, "f7 f8", KEYCODE_F4)\
	DIPS_HELPER( 0x2000, "Special CRSR Up", KEYCODE_8_PAD)\
	DIPS_HELPER( 0x1000, "Special CRSR Left", KEYCODE_4_PAD)\
        PORT_BITX( 0x800, IP_ACTIVE_HIGH, IPF_TOGGLE,\
		   "Swap Gameport 1 and 2", KEYCODE_NUMLOCK, IP_JOY_NONE)\
	DIPS_HELPER( 0x08, "Quickload", KEYCODE_F8)\
	DIPS_HELPER( 0x04, "Tape Drive Play",       KEYCODE_F5)\
	DIPS_HELPER( 0x02, "Tape Drive Record",     KEYCODE_F6)\
	DIPS_HELPER( 0x01, "Tape Drive Stop",       KEYCODE_F7)\
	PORT_START \
	PORT_DIPNAME ( 0xc000, 0x4000, "Gameport A")\
	PORT_DIPSETTING(  0, "None" )\
	PORT_DIPSETTING(	0x4000, "Joystick 1" )\
	PORT_DIPSETTING(	0x8000, "Paddles 1, 2" )\
	PORT_DIPSETTING(	0xc000, "Lightpen" )\
	PORT_DIPNAME ( 0x2000, 0x2000, "Lightpen Draw Pointer")\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(	0x2000, DEF_STR( On ) )\
	PORT_DIPNAME ( 0x1800, 0x800, "Gameport B")\
	PORT_DIPSETTING(  0, "None" )\
	PORT_DIPSETTING(	0x0800, "Joystick 2" )\
	PORT_DIPSETTING(	0x1000, "Paddles 3, 4" )\
	PORT_DIPNAME   ( 0x200, 0x200, "Tape Drive/Device 1")\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(0x200, DEF_STR( On ) )\
	PORT_DIPNAME   ( 0x100, 0x00, " Noise")\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(0x100, DEF_STR( On ) )

INPUT_PORTS_START( ultimax )
     INPUTPORTS_BOTH
     PORT_BIT ( 0x1c, 0x0,	 IPT_UNUSED ) /* only ultimax cartridges */
     PORT_BIT ( 0x2, 0x0,	 IPT_UNUSED ) /* no serial bus */
     PORT_BIT ( 0x1, 0x0,	 IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( c64 )
     INPUTPORTS_BOTH
	PORT_DIPNAME ( 0x1c, 0x00, "Cartridge Type")
        PORT_DIPSETTING(  0, "Automatic" )
	PORT_DIPSETTING(  4, "Ultimax (GAME)" )
	PORT_DIPSETTING(  8, "C64 (EXROM)" )
	PORT_DIPSETTING( 0x10, "CBM Supergames" )
	PORT_DIPSETTING( 0x14, "Ocean Robocop2" )
	PORT_DIPNAME ( 0x02, 0x02, "Serial Bus/Device 8")
	PORT_DIPSETTING(  0, "None" )
	PORT_DIPSETTING(  2, "VC1541 Floppy Drive" )
	PORT_DIPNAME ( 0x01, 0x01, "Serial Bus/Device 9")
	PORT_DIPSETTING(  0, "None" )
	PORT_DIPSETTING(  1, "VC1541 Floppy Drive" )
INPUT_PORTS_END

static void c64_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
  memcpy(sys_palette,vic2_palette,sizeof(vic2_palette));
}

ROM_START(c64)
     ROM_REGIONX(0x56000,REGION_CPU1)
     ROM_LOAD("basic.a0", 0x10000,0x2000,0xf833d117)
     ROM_LOAD("kernel.e0",0x12000,0x2000,0xdbe3e7c7)
     ROM_LOAD("char.do",0x14000,0x1000,0xec4272ee)
#ifdef VC1541
       VC1541_ROM(REGION_CPU2)
#endif
ROM_END

ROM_START(c64pal)
     ROM_REGIONX(0x56000,REGION_CPU1)
     ROM_LOAD("basic.a0", 0x10000,0x2000,0xf833d117)
     ROM_LOAD("kernel.e0",0x12000,0x2000,0xdbe3e7c7)
     ROM_LOAD("char.do",0x14000,0x1000,0xec4272ee)
#ifdef VC1541
       VC1541_ROM(REGION_CPU2)
#endif
ROM_END

ROM_START(ultimax)
     ROM_REGIONX(0x10000,REGION_CPU1)
ROM_END


static struct MachineDriver machine_driver_c64 =
{
  /* basic machine hardware */
  {
    {
      CPU_M6510,
      VIC6567_CLOCK,
      c64_readmem,c64_writemem,
      0,0,
      c64_frame_interrupt,1,
      vic2_raster_irq, VIC2_HRETRACERATE,
    },
#ifdef VC1541
    VC1541_CPU(REGION_CPU2)
#endif
  },
  VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
  0,
  c64_init_machine,
  c64_shutdown_machine,

  /* video hardware */
  336,										/* screen width */
  216,										/* screen height */
  { 0,336-1, 0,216-1 },					/* visible_area */
  0,						/* graphics decode info */
  sizeof(vic2_palette) / sizeof(vic2_palette[0]) / 3,
  0,
  c64_init_palette,											/* convert color prom */
  VIDEO_TYPE_RASTER,
  0,
  vic2_vh_start,
  vic2_vh_stop,
  vic2_vh_screenrefresh,

  /* sound hardware */
  0,0,0,0,
  {
/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
    { SOUND_DAC, &vc20tape_sound_interface }
  }
};

static struct MachineDriver machine_driver_c64pal =
{
  /* basic machine hardware */
  {
    {
      CPU_M6510,
      VIC6569_CLOCK,
      c64_readmem,c64_writemem,
      0,0,
      c64_frame_interrupt,1,
      vic2_raster_irq, VIC2_HRETRACERATE,
    },
#ifdef VC1541
    VC1541_CPU(REGION_CPU2)
#endif
  },
  VIC6569_VRETRACERATE,
  DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
  0,
  c64_init_machine,
  c64_shutdown_machine,

  /* video hardware */
  336,	/* screen width */
  216,	/* screen height */
  { 0,336-1, 0,216-1 },	/* visible_area */
  0,	/* graphics decode info */
  sizeof(vic2_palette) / sizeof(vic2_palette[0]) / 3,
  0,
  c64_init_palette,		/* convert color prom */
  VIDEO_TYPE_RASTER,
  0,
  vic2_vh_start,
  vic2_vh_stop,
  vic2_vh_screenrefresh,

  /* sound hardware */
  0,0,0,0,
  {
/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
    { SOUND_DAC, &vc20tape_sound_interface }
  }
};

static struct MachineDriver machine_driver_ultimax =
{
  /* basic machine hardware */
  {
    {
      CPU_M6510,
      VIC6567_CLOCK,
      ultimax_readmem,ultimax_writemem,
      0,0,
      c64_frame_interrupt,1,
      vic2_raster_irq, VIC2_HRETRACERATE,
    }
  },
  VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
  0,
  c64_init_machine,
  c64_shutdown_machine,

  /* video hardware */
  336,										/* screen width */
  216,										/* screen height */
  { 0,336-1, 0,216-1 },					/* visible_area */
  0,						/* graphics decode info */
  sizeof(vic2_palette) / sizeof(vic2_palette[0]) / 3,
  0,
  c64_init_palette,											/* convert color prom */
  VIDEO_TYPE_RASTER,
  0,
  vic2_vh_start,
  vic2_vh_stop,
  vic2_vh_screenrefresh,

  /* sound hardware */
  0,0,0,0,
  {
/*    { SOUND_CUSTOM, &sid6581_sound_interface }, */
    { SOUND_DAC, &vc20tape_sound_interface }
  }
};

static const struct IODevice io_c64[] = {
  IODEVICE_CBM_QUICK,
        {
                IO_CARTSLOT,            /* type */
                2, /* normal 1 */       /* count */
                NULL,                           /* file extensions */
        NULL,               /* private */
                c64_rom_id,             /* id */
                c64_rom_load,           /* init */
                NULL,                           /* exit */
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
    IODEVICE_VC20TAPE,
    IODEVICE_CBM_DRIVE,
    { IO_END }
};

static const struct IODevice io_ultimax[] = {
  IODEVICE_CBM_QUICK,
        {
                IO_CARTSLOT,            /* type */
                2, /* normal 1 */       /* count */
                "CRT\0",      /* file extensions */
        NULL,               /* private */
                c64_rom_id,             /* id */
                c64_rom_init,           /* init */
                NULL,                           /* exit */
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
    IODEVICE_VC20TAPE,
    { IO_END }
};

#define init_c64 c64_driver_init
#define init_c64pal c64pal_driver_init
#define init_ultimax ultimax_driver_init

#define io_c64pal io_c64
#define io_max io_ultimax
#define rom_max rom_ultimax

/*     YEAR  NAME    PARENT MACHINE INPUT INIT
       COMPANY   FULLNAME */
COMPX( 198?, c64,    0,     c64,    c64,  c64,
       "Commodore Business Machines Co.",  "Commodore C64 (NTSC)",
       GAME_NOT_WORKING|GAME_NO_SOUND )
COMPX( 198?, c64pal, c64,   c64pal, c64,  c64pal,
       "Commodore Business Machines Co.",  "Commodore C64 (PAL)",
       GAME_NOT_WORKING|GAME_NO_SOUND )
COMPX( 198?, max,    c64,   ultimax, ultimax,  ultimax,
       "Commodore Business Machines Co.",
       "Commodore Max (VIC10/Ultimax/Vickie)",
       GAME_NOT_WORKING|GAME_NO_SOUND )

