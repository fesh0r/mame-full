/*********************************************************************
From Marat's SMS.doc

  Following is a memory map of SMS/GG: 

     -- 10000 ----------------------------------------------
              Mirror of RAM at C000-DFFF
     --- E000 ----------------------------------------------
              8kB of on-board RAM (mirrored at E000-FFFF)
     --- C000 ----------------------------------------------
              16kB ROM Page #2, Cartridge RAM
     --- 8000 ----------------------------------------------
              16kB ROM Page #1
     --- 4000 ----------------------------------------------
              16kB ROM Page #0
     --- 0000 ----------------------------------------------

  Locations FFFCh-FFFFh and DFFCh-DFFFh belong to the Memory Mapper (see 
"Memory Mapper").


****************************** I/O ports *******************************

  Following is an I/O map of SMS/GG:

-- DC -- JOYPAD1 ---------------------------------------------- READ-ONLY --
  This is one of two joystick ports. Each bit corresponds to a button: 1
for pressed, 0 for released. This port is also mirrored at address C0h
(see Pit-Pot cartridge) and probably at some other officially unused
addresses. Its bits are laid out as follows: 

  bit   MasterSystem                    GameGear
  7     Joypad #2 DOWN                  Not used
  6     Joypad #2 UP                    Not used
  5     Joypad #1 FIRE-B                Joypad FIRE-B
  4     Joypad #1 FIRE-A                Joypad FIRE-A
  3     Joypad #1 RIGHT                 Joypad RIGHT
  2     Joypad #1 LEFT                  Joypad LEFT
  1     Joypad #1 DOWN                  Joypad DOWN
  0     Joypad #1 UP                    Joypad UP
  
-- DD -- JOYPAD2 ---------------------------------- SMS-ONLY -- READ-ONLY --
  This is the second of two joystick ports. Each bit corresponds to a
button: 1 for pressed, 0 for released. This port is also mirrored at 
address C1h (see Pit-Pot cartridge) and probably at some other officially
unused addresses. Its bits are laid out as 

  bit   MasterSystem
  7     Lightgun #2
  6     Lightgun #1
  5     Not used
  4     RESET button
  3     Joypad #2 FIRE-B
  2     Joypad #2 FIRE-A
  1     Joypad #2 RIGHT
  0     Joypad #2 LEFT

  This port doesn't appear to be present in GG, as GG has no second
joystick and no lightgun. The upper two bits (7 and 6) are also used for
automatic nationalization (see "Nationalization"). 

-- 00 -- JOYPAD3 ----------------------------------- GG-ONLY -- READ-ONLY --
  This port is only present in the GG and contains the START button bit
and the nationalization bit: 

  bit   GameGear
  7     START button (1 when pressed, 0 when released)
  6     Nationalization (1 in European/American GG, 0 in Japanese GG)

-- 7E -- CURLINE/PSGPORT ------------------------------------- READ/WRITE --
  This port is performing a dual role. When read, it contains the current
scanline number (CURLINE). When written, it serves to output data into the
PSG chip (PSGPORT). The 7Eh port also appears to be mirrored at address
7Fh and probably at some other officially unused addresses. 

-- BE -- VDPDATA --------------------------------------------- READ/WRITE --
  This is a VDP chip port used to read and write data from/to VRAM. See 
"VDP" chapter for more information.

-- BF -- VDPSTAT/VDPCTRL ------------------------------------- READ/WRITE --
  When read, this port returns the VDP status word as follows:

  bit   Function
  7     VBlank Flag, sets to 1 at the beginning of each Vertical Blanking
        Impulse. This flag resets to 0 after the register is read.
  6
  5

  When written, this port is used to pass VRAM addresses, register
numbers, and color numbers to VDP. See "VDP" chapter for more information.
The BFh port is also mirrored at address BDh (see SailorMoon cartridge)
and probably at some other officially unused addresses. 

-- 3F -- GUNPORT --------------------------------- SMS-ONLY -- WRITE-ONLY --
  This port is only present in the SMS and serves both for lightgun
initialization and for determining the system nationalization in SMS. 

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/sn76496.h"
#include "vidhrdw/sms.h"
#include "vidhrdw/smsvdp.h"

/* from machine/sms.c */

int sms_load_rom (void);
int sms_id_rom (const char *name, const char *gamename);
int gamegear_id_rom (const char *name, const char *gamename);
int sms_ram_r (int offset);
void sms_ram_w (int offset, int data);
void sms_mapper_w (int offset, int data);
void sms_init_machine (void);
int sms_country_r (int offset);
void sms_country_w (int offset, int data);

extern unsigned char *sms_user_ram;

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1 },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xbfff, MRA_BANK3 },
	{ 0xc000, 0xdfff, sms_ram_r, &sms_user_ram },
	{ 0xe000, 0xffff, sms_ram_r },                 /* mirror of $c000-$dfff */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0xbfff, MWA_BANK3 },
	{ 0xc000, 0xdfff, sms_ram_w, &sms_user_ram },
	{ 0xe000, 0xfff0, sms_ram_w },
	{ 0xfff0, 0xffff, sms_mapper_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_2_r }, /* GameGear only ?? */
	{ 0x7e, 0x7f, sms_vdp_curline_r },
#ifdef OLD_VIDEO
	{ 0xbd, 0xbd, sms_vdp_register_r },
	{ 0xbe, 0xbe, sms_vdp_vram_r },
	{ 0xbf, 0xbf, sms_vdp_register_r },
#else
	{ 0xbd, 0xbd, SMSVDP_register_r },
	{ 0xbe, 0xbe, SMSVDP_vram_r },
	{ 0xbf, 0xbf, SMSVDP_register_r },
#endif
	{ 0xdc, 0xdc, input_port_0_r },
	{ 0xc0, 0xc0, input_port_0_r },
	{ 0xdd, 0xdd, input_port_1_r },
	{ 0xc1, 0xc1, input_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x3f, 0x3f, sms_country_w },
	{ 0x7e, 0x7f, SN76496_0_w },
#ifdef OLD_VIDEO
	{ 0xbd, 0xbd, sms_vdp_register_w },
	{ 0xbe, 0xbe, sms_vdp_vram_w },
	{ 0xbf, 0xbf, sms_vdp_register_w },
#else
	{ 0xbd, 0xbd, SMSVDP_register_w },
	{ 0xbe, 0xbe, SMSVDP_vram_w },
	{ 0xbf, 0xbf, SMSVDP_register_w },
#endif
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )

	PORT_START	/* IN1 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
    PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 )
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 )

	PORT_START	/* IN2 - GameGear only */
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Nationalization bit */
    PORT_BIT ( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	448,	/* 448 characters */
	4,	/* 4 bits per pixel */
	{ 0*8, 1*8, 2*8, 3*8 },	/* the four bitplanes are in separate, consecutive bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 32 },
	{ -1 } /* end of array */
};

static struct SN76496interface sn76496_interface =
{
	1,              /* 1 chip */
	4194304,        /* 4.194304 MHz */
	{ 255 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3325187,	/* 3.325187 Mhz */
			0,
			readmem,writemem,readport,writeport,
#ifdef OLD_VIDEO
			sms_interrupt, SCANLINES_PER_FRAME,
#else
			SMSVDP_interrupt, 1,
#endif
			0, 0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	sms_init_machine, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	32*8, 24*8, { 1*8, 31*8-1, 0*8, 24*8-1 },
	gfxdecodeinfo,
	SMS_PALETTE_SIZE, SMS_COLORTABLE_SIZE,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sms_vdp_start,
	sms_vdp_stop,
	sms_vdp_refresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver gamegear_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3325187,	/* 3.325187 Mhz */
			0,
			readmem,writemem,readport,writeport,
#ifdef OLD_VIDEO
			sms_interrupt, SCANLINES_PER_FRAME,
#else
			SMSVDP_interrupt, 1,
#endif
			0, 0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	sms_init_machine, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	32*8, 24*8, { 1*8, 31*8-1, 3*8, 21*8-1 },
	gfxdecodeinfo,
	SMS_PALETTE_SIZE, SMS_COLORTABLE_SIZE,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gamegear_vdp_start,
	sms_vdp_stop,
	sms_vdp_refresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/
struct GameDriver sms_driver =
{
	__FILE__,
	0,	
	"sms",
	"Sega Master System",
	"19??",
	"Sega",
	"Marat Fayzullin (MG source)\nMathis Rosenhauer\nBrad Oliver",
	GAME_WRONG_COLORS,
	&machine_driver,

	0,
	sms_load_rom,
	sms_id_rom,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0,
	0, 0
};

struct GameDriver gamegear_driver =
{
	__FILE__,
	0,	
	"gamegear",
	"Sega Game Gear",
	"19??",
	"Sega",
	"Marat Fayzullin (MG source)\nMathis Rosenhauer\nBrad Oliver",
	0,
	&gamegear_machine_driver,

	0,
	sms_load_rom,
	gamegear_id_rom,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0,
	0, 0
};
