#include "driver.h"
#include "sound/sn76496.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/smsvdp.h"

/* from machine/sms.c */

void sms_init_machine (void);
int sms_load_rom (void);

int sms_id_rom (const char *name, const char *gamename);
int gamegear_id_rom (const char *name, const char *gamename);

int sms_ram_r (int offset);
void sms_ram_w (int offset, int data);

void sms_mapper_w (int offset, int data);
int sms_mapper_r (int offset);

int sms_country_r (int offset);
void sms_country_w (int offset, int data);

extern unsigned char *sms_user_ram;


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x3FFF, MRA_BANK1 },
    { 0x4000, 0x7FFF, MRA_BANK2 },
    { 0x8000, 0xBFFF, MRA_BANK3 },
    { 0xC000, 0xDFFF, sms_ram_r, &sms_user_ram },
    { 0xE000, 0xFFFF, sms_ram_r },
    { -1 }
};

/* todo: add support for ram at bank 3 */
static struct MemoryWriteAddress writemem[] =
{
    { 0xC000, 0xDFFF, sms_ram_w, &sms_user_ram },
    { 0xE000, 0xFFFB, sms_ram_w },
    { 0xFFFC, 0xFFFF, sms_mapper_w },
    { -1 }
};

static struct IOReadPort readport[] =
{
    { 0xbe, 0xbe, sms_vdp_data_r },
    { 0xbd, 0xbd, sms_vdp_ctrl_r },
    { 0xbf, 0xbf, sms_vdp_ctrl_r },
    { 0x7e, 0x7f, sms_vdp_curline_r },
    { 0xdc, 0xdc, input_port_0_r },
    { 0xc0, 0xc0, input_port_0_r },
    { 0xdd, 0xdd, input_port_1_r },
    { 0xc1, 0xc1, input_port_1_r },
    { 0x00, 0x00, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
    { 0xbe, 0xbe, sms_vdp_data_w },
    { 0xbd, 0xbd, sms_vdp_ctrl_w },
    { 0xbf, 0xbf, sms_vdp_ctrl_w },
	{ 0x7e, 0x7f, SN76496_0_w },
	{ 0x3f, 0x3f, sms_country_w },
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
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - GameGear only */
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Nationalization bit */
    PORT_BIT ( 0x3f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct SN76496interface sn76496_interface =
{
	1,              /* 1 chip */
    {4194304},        /* 4.194304 MHz */
	{ 100 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
            3579545,
			0,
			readmem,writemem,readport,writeport,
            sms_vdp_interrupt, 262,
			0, 0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	sms_init_machine, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
    32*8, 28*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
    0,

    32, 32,
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
            3597545,
			0,
			readmem,writemem,readport,writeport,
            sms_vdp_interrupt, 262,
			0, 0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	sms_init_machine, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
    32*8, 28*8, { 6*8, 26*8-1, 3*8, 21*8-1 }, /* 256x224 -> 160x144 */
	0, /*gfxdecodeinfo,*/
    32, 32,
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

struct GameDriver sms_driver =
{
	__FILE__,
	0,
	"sms",
	"Sega Master System",
	"19??",
	"Sega",
    "Marat Fayzullin (MG source)\nCharles Mac Donald\nMathis Rosenhauer\nBrad Oliver",
    0,
	&machine_driver,
	0,

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
};

struct GameDriver gamegear_driver =
{
	__FILE__,
	0,
	"gamegear",
	"Sega Game Gear",
	"19??",
	"Sega",
    "Marat Fayzullin (MG source)\nCharles Mac Donald\nMathis Rosenhauer\nBrad Oliver",
	0,
	&gamegear_machine_driver,
	0,

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
};






