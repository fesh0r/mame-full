/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at May 2000

******************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"

#include "includes/studio2.h"

static UINT8 *studio2_mem;

static struct MemoryReadAddress studio2_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x09ff, MRA_RAM },
	MEMORY_TABLE_END
};

static struct MemoryWriteAddress studio2_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_ROM, &studio2_mem },
	{ 0x0800, 0x09ff, MWA_RAM },
	MEMORY_TABLE_END
};

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( studio2 )
	PORT_START
	DIPS_HELPER( 0x002, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 1/Left 2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x008, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 1/Left 4", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x020, "Player 1/Left 5", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x040, "Player 1/Left 6", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x080, "Player 1/Left 7", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x100, "Player 1/Left 8", KEYCODE_8, CODE_NONE)
	DIPS_HELPER( 0x200, "Player 1/Left 9", KEYCODE_9, CODE_NONE)
	DIPS_HELPER( 0x001, "Player 1/Left 0", KEYCODE_0, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x002, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 2/Right 2", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x008, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x020, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x080, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x100, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x200, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x001, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
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

static int studio2_frame_int(void)
{
	return 0;
}

/* output q speaker (300 hz tone on/off)
   f1 dma_activ
   f3 on player 2 key pressed
   f4 on player 1 key pressed
   inp 1 video on
   out 2 read key value selects keys to put at f3/f4 */

static int studio2_in_n(int n)
{
	if (n==1) studio2_video_start();
	return 0; //?
}

static UINT8 studio2_keyboard_select;

static void studio2_out_n(int data, int n)
{
	if (n==2) studio2_keyboard_select=data;
}

static int studio2_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (input_port_0_r(0)&(1<<studio2_keyboard_select)) a|=4;
	if (input_port_1_r(0)&(1<<studio2_keyboard_select)) a|=8;
	
	return a;
}

static void studio2_out_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG config={
	studio2_video_dma,
	studio2_out_n,
	studio2_in_n,
	studio2_out_q,
	studio2_in_ef
};



static unsigned char studio2_palette[2][3] =
{
	{ 0, 0, 0 },
	{ 255,255,255 }
};

static unsigned short studio2_colortable[1][2] = {
	{ 0, 1 },
};

static void studio2_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	memcpy (sys_palette, studio2_palette, sizeof (studio2_palette));
	memcpy(sys_colortable,studio2_colortable,sizeof(studio2_colortable));
}

static void studio2_machine_init(void)
{
	studio2_video_stop();
}

static struct beep_interface studio2_sound= { 1 };
static struct MachineDriver machine_driver_studio2 =
{
	/* basic machine hardware */
	{
		{
			CPU_CDP1802,
			1780000/8,
			studio2_readmem,studio2_writemem,0,0,
			studio2_frame_int, 1,
			0,0,
			&config
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	studio2_machine_init,
	0,//pc1401_machine_stop,

	// ntsc tv display with 15720 line frequency!
	// good width, but only 128 lines height
	64*4, 128, { 0, 64*4 - 1, 0, 128 - 1},
	studio2_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (studio2_palette) / sizeof (studio2_palette[0]) ,
	sizeof (studio2_colortable) / sizeof(studio2_colortable[0][0]),
	studio2_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    studio2_vh_start,
	studio2_vh_stop,
	studio2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		// 300 hertz beeper
        { SOUND_BEEP, &studio2_sound }
    }
};

ROM_START(studio2)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("studio2.rom", 0x0000, 0x800, 0xa494b339)
	ROM_REGION(0x100,REGION_GFX1)
ROM_END


static const struct IODevice io_studio2[] = {
	// cartridges at 0x400-0x7ff ?
    { IO_END }
};

void init_studio2(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;

	beep_set_frequency(0, 300);
}

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */
// rca cosmac elf development board (2 7segment leds, some switches/keys)
// rca cosmac vip ditto
CONSX( 1976, studio2,	  0, 		studio2,  studio2, 	studio2,	  "RCA",  "Studio II", GAME_NOT_WORKING)
// colour studio 2 (m1200) with little color capability
