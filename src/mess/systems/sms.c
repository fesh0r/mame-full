/******************************************************************************
	Marat Fayzullin (MG source)
	Charles Mac Donald
	Mathis Rosenhauer
	Brad Oliver
 ******************************************************************************/
#include "driver.h"
#include "sound/sn76496.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/smsvdp.h"

/* from machine/sms.c */

void sms_init_machine (void);
int sms_load_rom (int id, const char *rom_name);

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
    { 0xC000, 0xDFFF, sms_ram_r },
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

INPUT_PORTS_START( sms )
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


static struct MachineDriver machine_driver_sms =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
            3579545,
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

static struct MachineDriver machine_driver_gamegear =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
            3597545,
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

ROM_START(sms)
    ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_REGIONX(256*16384+512,REGION_CPU2)
ROM_END

ROM_START(gamegear)
    ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_REGIONX(256*16384+512,REGION_CPU2)
ROM_END

static const struct IODevice io_sms[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"sms\0",            /* file extensions */
		NULL,				/* private */
		sms_id_rom, 		/* id */
		sms_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
    },
	{ IO_END }
};

static const struct IODevice io_gamegear[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"gg\0",             /* file extensions */
		NULL,				/* private */
		gamegear_id_rom,	/* id */
		sms_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
    },
	{ IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
CONS( 1983, sms,	  0, 		sms,	  sms,		0,		  "Sega",   "Sega Master System" )
CONS( 19??, gamegear, 0, 		gamegear, sms,		0,		  "Sega",   "Sega Game Gear" )
