/******************************************************************************
 PeT mess@utanet.at May 2000

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "image.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cartslot.h"
#include "includes/studio2.h"

static MEMORY_READ_START( studio2_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x09ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( studio2_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x09ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( vip_readmem )
    { 0x0000, 0x03ff, MRA_BANK1 }, // rom mapped in at reset, switched to ram with out 4
	{ 0x0400, 0x0fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( vip_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x0fff, MWA_RAM },
	{ 0x8000, 0x83ff, MWA_ROM },
MEMORY_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( studio2 )
	PORT_START
	DIPS_HELPER( 0x002, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 1/Left 2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x008, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x020, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x040, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x080, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x100, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x200, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x001, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	DIPS_HELPER( 0x002, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 2/Right 2", KEYCODE_2_PAD, KEYCODE_UP)
	DIPS_HELPER( 0x008, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 2/Right 4", KEYCODE_4_PAD, KEYCODE_LEFT)
	DIPS_HELPER( 0x020, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "Player 2/Right 6", KEYCODE_6_PAD, KEYCODE_RIGHT)
	DIPS_HELPER( 0x080, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x100, "Player 2/Right 8", KEYCODE_8_PAD, KEYCODE_DOWN)
	DIPS_HELPER( 0x200, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x001, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
INPUT_PORTS_END

INPUT_PORTS_START( vip )
	PORT_START
	DIPS_HELPER( 0x0002, "1", KEYCODE_1, KEYCODE_1_PAD)
	DIPS_HELPER( 0x0004, "2", KEYCODE_2, KEYCODE_2_PAD)
	DIPS_HELPER( 0x0008, "3", KEYCODE_3, KEYCODE_3_PAD)
	DIPS_HELPER( 0x1000, "C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x0010, "4", KEYCODE_4, KEYCODE_4_PAD)
	DIPS_HELPER( 0x0020, "5", KEYCODE_5, KEYCODE_5_PAD)
	DIPS_HELPER( 0x0040, "6", KEYCODE_6, KEYCODE_6_PAD)
	DIPS_HELPER( 0x2000, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x0080, "7", KEYCODE_7, KEYCODE_7_PAD)
	DIPS_HELPER( 0x0100, "8", KEYCODE_8, KEYCODE_8_PAD)
	DIPS_HELPER( 0x0200, "9", KEYCODE_9, KEYCODE_9_PAD)
	DIPS_HELPER( 0x4000, "E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x0400, "A    MR", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x0001, "0    MW", KEYCODE_0, KEYCODE_0_PAD)
	DIPS_HELPER( 0x0800, "B    TR", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x8000, "F    TW", KEYCODE_F, CODE_NONE)
	PORT_START
INPUT_PORTS_END

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

static struct GfxDecodeInfo studio2_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &studio2_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static INTERRUPT_GEN( studio2_frame_int )
{
}

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

static UINT8 studio2_keyboard_select;

static void studio2_out_n(int data, int n)
{
	if (n==2) studio2_keyboard_select=data;
}

static void vip_out_n(int data, int n)
{
	if (n==2) studio2_keyboard_select=data;
	if (n==4) {
		cpu_setbank(1,memory_region(REGION_CPU1)+0);
	}
}

static int studio2_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<studio2_keyboard_select)) a|=4;
	if (readinputport(1)&(1<<studio2_keyboard_select)) a|=8;

	return a;
}

static int vip_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<studio2_keyboard_select)) a|=4;

	return a;
}

static void studio2_out_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG studio2_config={
	studio2_video_dma,
	studio2_out_n,
	studio2_in_n,
	studio2_out_q,
	studio2_in_ef
};

static CDP1802_CONFIG vip_config={
	studio2_video_dma,
	vip_out_n,
	studio2_in_n,
	studio2_out_q,
	vip_in_ef
};

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

static MACHINE_INIT( studio2 )
{
}

static MACHINE_INIT( vip )
{
	cpu_setbank(1,memory_region(REGION_CPU1)+0x8000);
}

static struct beep_interface studio2_sound=
{
	1,
	{100}
};


static MACHINE_DRIVER_START( studio2 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", CDP1802, 1780000/8)
	MDRV_CPU_MEMORY(studio2_readmem,studio2_writemem)
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
	MDRV_CPU_MEMORY( vip_readmem,vip_writemem )
	MDRV_CPU_CONFIG( vip_config )
	MDRV_MACHINE_INIT( vip )
MACHINE_DRIVER_END


ROM_START(studio2)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("studio2.rom", 0x0000, 0x800,CRC( 0xa494b339))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vip)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("monitor.rom", 0x8000, 0x200,CRC( 0x5be0a51f))
	ROM_LOAD("chip8.rom", 0x8200, 0x200,CRC( 0x3e0f50f0))
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

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

/***************************************************************************

  Game driver(s)

***************************************************************************/

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


/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG      COMPANY   FULLNAME */
// rca cosmac elf development board (2 7segment leds, some switches/keys)
// rca cosmac elf2 16 key keyblock
CONSX(1977,	vip,		0,		0,		vip,		vip,		vip,		studio2,	"RCA",		"COSMAC VIP", GAME_NOT_WORKING )
CONSX(1976,	studio2,	0,		0,		studio2,	studio2,	studio2,	studio2,	"RCA",		"Studio II", GAME_NOT_WORKING )
// hanimex mpt-02
// colour studio 2 (m1200) with little color capability
