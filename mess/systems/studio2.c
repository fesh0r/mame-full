/******************************************************************************
 PeT mess@utanet.at May 2000

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "image.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cartslot.h"
#include "includes/studio2.h"
#include "inputx.h"

extern READ_HANDLER( cdp1861_video_enable_r );

static UINT8 keylatch;

/* vidhrdw */

static unsigned char studio2_palette[] =
{
	0, 0, 0,
	255,255,255
};

static unsigned short studio2_colortable[1][2] = {
	{ 0, 1 },
};

static PALETTE_INIT( studio2 )
{
	palette_set_colors(0, studio2_palette, sizeof(studio2_palette) / 3);
	memcpy(colortable,studio2_colortable,sizeof(studio2_colortable));
}

/* Read/Write Handlers */

static WRITE_HANDLER( keylatch_w )
{
	keylatch = data & 0x0f;
}

static WRITE_HANDLER( bankswitch_w )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
}

/* Memory Maps */

static ADDRESS_MAP_START( studio2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x09ff) AM_RAM
	AM_RANGE(0x0a00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( studio2_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(cdp1861_video_enable_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(1) // rom mapped in at reset, switched to ram with out 4
	AM_RANGE(0x0400, 0x0fff) AM_RAM
	AM_RANGE(0x8000, 0x83ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(cdp1861_video_enable_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( studio2 )
	PORT_START
	PORT_KEY0( 0x001, IP_ACTIVE_HIGH, "Player 1/Left 0", KEYCODE_0, KEYCODE_X )
	PORT_KEY0( 0x002, IP_ACTIVE_HIGH, "Player 1/Left 1", KEYCODE_1, CODE_NONE )
	PORT_KEY0( 0x004, IP_ACTIVE_HIGH, "Player 1/Left 2", KEYCODE_2, CODE_NONE )
	PORT_KEY0( 0x008, IP_ACTIVE_HIGH, "Player 1/Left 3", KEYCODE_3, CODE_NONE )
	PORT_KEY0( 0x010, IP_ACTIVE_HIGH, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q )
	PORT_KEY0( 0x020, IP_ACTIVE_HIGH, "Player 1/Left 5", KEYCODE_5, KEYCODE_W )
	PORT_KEY0( 0x040, IP_ACTIVE_HIGH, "Player 1/Left 6", KEYCODE_6, KEYCODE_E )
	PORT_KEY0( 0x080, IP_ACTIVE_HIGH, "Player 1/Left 7", KEYCODE_7, KEYCODE_A )
	PORT_KEY0( 0x100, IP_ACTIVE_HIGH, "Player 1/Left 8", KEYCODE_8, KEYCODE_S )
	PORT_KEY0( 0x200, IP_ACTIVE_HIGH, "Player 1/Left 9", KEYCODE_9, KEYCODE_D )

	PORT_START
	PORT_KEY0( 0x001, IP_ACTIVE_HIGH, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE )
	PORT_KEY0( 0x002, IP_ACTIVE_HIGH, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE )
	PORT_KEY0( 0x004, IP_ACTIVE_HIGH, "Player 2/Right 2", KEYCODE_2_PAD, KEYCODE_UP )
	PORT_KEY0( 0x008, IP_ACTIVE_HIGH, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE )
	PORT_KEY0( 0x010, IP_ACTIVE_HIGH, "Player 2/Right 4", KEYCODE_4_PAD, KEYCODE_LEFT )
	PORT_KEY0( 0x020, IP_ACTIVE_HIGH, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE )
	PORT_KEY0( 0x040, IP_ACTIVE_HIGH, "Player 2/Right 6", KEYCODE_6_PAD, KEYCODE_RIGHT )
	PORT_KEY0( 0x080, IP_ACTIVE_HIGH, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE )
	PORT_KEY0( 0x100, IP_ACTIVE_HIGH, "Player 2/Right 8", KEYCODE_8_PAD, KEYCODE_DOWN )
	PORT_KEY0( 0x200, IP_ACTIVE_HIGH, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( vip )
	PORT_START
	PORT_KEY0( 0x0001, IP_ACTIVE_HIGH, "0 MW", KEYCODE_0, KEYCODE_0_PAD )
	PORT_KEY0( 0x0002, IP_ACTIVE_HIGH, "1", KEYCODE_1, KEYCODE_1_PAD )
	PORT_KEY0( 0x0004, IP_ACTIVE_HIGH, "2", KEYCODE_2, KEYCODE_2_PAD )
	PORT_KEY0( 0x0008, IP_ACTIVE_HIGH, "3", KEYCODE_3, KEYCODE_3_PAD )
	PORT_KEY0( 0x0010, IP_ACTIVE_HIGH, "4", KEYCODE_4, KEYCODE_4_PAD )
	PORT_KEY0( 0x0020, IP_ACTIVE_HIGH, "5", KEYCODE_5, KEYCODE_5_PAD )
	PORT_KEY0( 0x0040, IP_ACTIVE_HIGH, "6", KEYCODE_6, KEYCODE_6_PAD )
	PORT_KEY0( 0x0080, IP_ACTIVE_HIGH, "7", KEYCODE_7, KEYCODE_7_PAD )
	PORT_KEY0( 0x0100, IP_ACTIVE_HIGH, "8", KEYCODE_8, KEYCODE_8_PAD )
	PORT_KEY0( 0x0200, IP_ACTIVE_HIGH, "9", KEYCODE_9, KEYCODE_9_PAD )
	PORT_KEY0( 0x0400, IP_ACTIVE_HIGH, "A MR", KEYCODE_A, CODE_NONE )
	PORT_KEY0( 0x0800, IP_ACTIVE_HIGH, "B TR", KEYCODE_B, CODE_NONE )
	PORT_KEY0( 0x1000, IP_ACTIVE_HIGH, "C", KEYCODE_C, CODE_NONE )
	PORT_KEY0( 0x2000, IP_ACTIVE_HIGH, "D", KEYCODE_D, CODE_NONE )
	PORT_KEY0( 0x4000, IP_ACTIVE_HIGH, "E", KEYCODE_E, CODE_NONE )
	PORT_KEY0( 0x8000, IP_ACTIVE_HIGH, "F TW", KEYCODE_F, CODE_NONE )

	PORT_START
INPUT_PORTS_END

/* Graphics Layouts */

static struct GfxLayout studio2_charlayout =
{
        32,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
			0, 0, 0, 0,
			1, 1, 1, 1,
			2, 2, 2, 2,
			3, 3, 3, 3,
			4, 4, 4, 4,
			5, 5, 5, 5,
			6, 6, 6, 6,
			7, 7, 7, 7
        },
        /* y offsets */
        { 0 },
        1*8
};

/* Graphics Decode Information */

static struct GfxDecodeInfo studio2_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &studio2_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

/* Interrupt Generators */

static INTERRUPT_GEN( studio2_frame_int )
{
}

/* CDP1802 Configuration */

/* studio 2
   output q speaker (300 hz tone on/off)
   f1 dma_activ
   f3 on player 2 key pressed
   f4 on player 1 key pressed
   inp 1 video on
   out 2 read key value selects keys to put at f3/f4 */

/* vip
   out 1 turns video off
   out 2 set keyboard multiplexer (bit 0..3 selects key)
   out 4 switch ram at 0000
   inp 1 turn on video
   q sound
   f1 vertical blank line (low when displaying picture
   f2 tape input (high when tone read)
   f3 keyboard in
 */

static int studio2_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<keylatch)) a|=4;
	if (readinputport(1)&(1<<keylatch)) a|=8;

	return a;
}

static int vip_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<keylatch)) a|=4;

	return a;
}

static void studio2_out_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG studio2_config={
	studio2_video_dma,
	studio2_out_q,
	studio2_in_ef
};

static CDP1802_CONFIG vip_config={
	studio2_video_dma,
	studio2_out_q,
	vip_in_ef
};

/* Machine Initialization */

static MACHINE_INIT( studio2 )
{
}

static MACHINE_INIT( vip )
{
	cpu_setbank(1,memory_region(REGION_CPU1)+0x8000);
}

/* Sound Interfaces */

static struct beep_interface studio2_sound=
{
	1,
	{100}
};

/* Machine Drivers */

static MACHINE_DRIVER_START( studio2 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", CDP1802, 1780000/8)
	MDRV_CPU_PROGRAM_MAP(studio2_map, 0)
	MDRV_CPU_IO_MAP(studio2_io_map, 0)
	MDRV_CPU_VBLANK_INT(studio2_frame_int, 1)
	MDRV_CPU_CONFIG(studio2_config)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( studio2 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*4, 128)
	MDRV_VISIBLE_AREA(0, 64*4-1, 0, 128-1)
	MDRV_GFXDECODE( studio2_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (studio2_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (studio2_colortable) / sizeof(studio2_colortable[0][0]))
	MDRV_PALETTE_INIT( studio2 )

	MDRV_VIDEO_START( studio2 )
	MDRV_VIDEO_UPDATE( studio2 )

	/* sound hardware */
	MDRV_SOUND_ADD(BEEP, studio2_sound)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vip )
	MDRV_IMPORT_FROM( studio2 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(vip_map, 0)
	MDRV_CPU_IO_MAP(vip_io_map, 0)
	MDRV_CPU_CONFIG( vip_config )
	MDRV_MACHINE_INIT( vip )
MACHINE_DRIVER_END

/* ROMs */

ROM_START(studio2)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("studio2.rom", 0x0000, 0x800, CRC(a494b339))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vip)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("monitor.rom", 0x8000, 0x200, CRC(5be0a51f))
	ROM_LOAD("chip8.rom", 0x8200, 0x200, CRC(3e0f50f0))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

/* System Configuration */

static DEVICE_LOAD( studio2_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0x0400, 0x0001, 0xfc00, 0);
}

SYSTEM_CONFIG_START(studio2)
	/* maybe quickloader */
	/* tape */
	/* cartridges at 0x400-0x7ff ? */
	CONFIG_DEVICE_CARTSLOT_OPT(1, "bin\0", NULL, NULL, device_load_studio2_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/* Driver Initialization */

static DRIVER_INIT( studio2 )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	for (i=0; i<256; i++)
		gfx[i]=i;
	beep_set_frequency(0, 300);
}

static DRIVER_INIT( vip )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);
	for (i=0; i<256; i++)
		gfx[i]=i;
	beep_set_frequency(0, 300);
	memory_region(REGION_CPU1)[0x8022] = 0x3e; //bn3, default monitor
}

/* Game Drivers */

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG      COMPANY   FULLNAME */
// rca cosmac elf development board (2 7segment leds, some switches/keys)
// rca cosmac elf2 16 key keyblock
CONSX(1977,	vip,		0,		0,		vip,		vip,		vip,		studio2,	"RCA",		"COSMAC VIP", GAME_NOT_WORKING )
CONSX(1976,	studio2,	0,		0,		studio2,	studio2,	studio2,	studio2,	"RCA",		"Studio II", GAME_NOT_WORKING )
// hanimex mpt-02
// colour studio 2 (m1200) with little color capability
