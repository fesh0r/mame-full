/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"

#include "includes/vc4000.h"
#include "devices/cartslot.h"
#include "image.h"

static READ_HANDLER(vc4000_key_r)
{
	UINT8 data=0;
	switch(offset) {
	case 0:
		data = readinputport(1);
		break;
	case 1:
		data = readinputport(2);
		break;
	case 2:
		data = readinputport(3);
		break;
	case 3:
		data = readinputport(0);
		break;
	case 4:
		data = readinputport(4);
		break;
	case 5:
		data = readinputport(5);
		break;
	case 6:
		data = readinputport(6);
		break;
	}
	return data;
}


static MEMORY_READ_START( vc4000_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
{ 0x1e88, 0x1e8e, vc4000_key_r },
	{ 0x1f00, 0x1fff, vc4000_video_r },
MEMORY_END

static MEMORY_WRITE_START( vc4000_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x1f00, 0x1fff, vc4000_video_w },
MEMORY_END

static PORT_READ_START( vc4000_readport )
//{ S2650_CTRL_PORT,S2650_CTRL_PORT, },
//{ S2650_DATA_PORT,S2650_DATA_PORT, },
{ S2650_SENSE_PORT,S2650_SENSE_PORT, vc4000_vsync_r},
PORT_END

static PORT_WRITE_START( vc4000_writeport )
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( vc4000 )
	PORT_START
	DIPS_HELPER( 0x40, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x80, "Game Select", KEYCODE_F2, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x20, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x10, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_Z)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x40, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x20, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x10, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x20, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x10, "Player 1/Left Clear", KEYCODE_C, KEYCODE_ESC)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x40, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
#ifndef ANALOG_HACK
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_ANALOGX(0x1ff,0x70,IPT_AD_STICK_X|IPF_CENTER,100,1,20,225,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x70,IPT_AD_STICK_Y|IPF_CENTER,100,1,20,225,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)
    PORT_START
PORT_ANALOGX(0x1ff,0x70,IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2,100,1,20,225,KEYCODE_DEL,KEYCODE_PGDN,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x70,IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,100,1,20,225,KEYCODE_HOME,KEYCODE_END,JOYCODE_2_UP,JOYCODE_2_DOWN)
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

static struct GfxLayout vc4000_charlayout =
{
        8,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
	    0,
	    1,
	    2,
	    3,
	    4,
	    5,
	    6,
	    7,
        },
        /* y offsets */
        { 0 },
        1*8
};

static struct GfxDecodeInfo vc4000_gfxdecodeinfo[] = {
    { REGION_GFX1, 0x0000, &vc4000_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static INTERRUPT_GEN( vc4000 )
{
}

static unsigned char vc4000_palette[] =
{
	// background colors
	0, 0, 0, // black
	0, 0, 255, // blue
	0, 255, 0, // green
	0, 255, 255, // cyan
	255, 0, 0, // red
	255, 0, 255, // magenta
	255, 255, 0, // yellow
	255, 255, 255, // white
	// sprite colors
	// simplier to add another 8 colors else using colormapping
	// xor 7, bit 2 not green, bit 1 not blue, bit 0 not red
	255, 255, 255, // white
	0, 255, 255, // cyan
	255, 255, 0, // yellow
	0, 255, 0, // green
	255, 0, 255, // magenta
	0, 0, 255, // blue
	255, 0, 0, // red
	0, 0, 0 // black
};

static unsigned short vc4000_colortable[1][2] = {
	{ 0, 1 },
};

static PALETTE_INIT( vc4000 )
{
	palette_set_colors(0, vc4000_palette, sizeof(vc4000_palette) / 3);
	memcpy(colortable, vc4000_colortable,sizeof(vc4000_colortable));
}

static MACHINE_DRIVER_START( vc4000 )
	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 3000000/3)        /* 3580000/3, 4430000/3 */
	MDRV_CPU_MEMORY(vc4000_readmem,vc4000_writemem)
	MDRV_CPU_PORTS(vc4000_readport,vc4000_writeport)
	MDRV_CPU_PERIODIC_INT(vc4000_video_line,312*50)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
#ifndef DEBUG
	MDRV_SCREEN_SIZE(128+2*XPOS, 208+YPOS+YBOTTOM_SIZE)
	MDRV_VISIBLE_AREA(0, 2*XPOS+128-1, 0, YPOS+208+YBOTTOM_SIZE-1)
#else
	MDRV_SCREEN_SIZE(256+2*XPOS, 260+YPOS+YBOTTOM_SIZE)
	MDRV_VISIBLE_AREA(0, 2*XPOS+256-1, 0, YPOS+260+YBOTTOM_SIZE-1)
#endif
	MDRV_GFXDECODE( vc4000_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(vc4000_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (vc4000_colortable) / sizeof(vc4000_colortable[0][0]))
	MDRV_PALETTE_INIT( vc4000 )

	MDRV_VIDEO_START( vc4000 )
	MDRV_VIDEO_UPDATE( vc4000 )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, vc4000_sound_interface)
MACHINE_DRIVER_END

ROM_START(vc4000)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

static DEVICE_LOAD( vc4000_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0, 0x0001, memory_region_length(REGION_CPU1), 0);
}

SYSTEM_CONFIG_START(vc4000)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "bin\0", NULL, NULL, device_load_vc4000_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static DRIVER_INIT( vc4000 )
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
}

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
CONSX(1978,	vc4000,	0,		0,		vc4000,	vc4000,	vc4000,	vc4000,	"Interton",	"VC4000", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND )
