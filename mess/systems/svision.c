/******************************************************************************
 watara supervision handheld

 PeT mess@utanet.at in december 2000
******************************************************************************/

#include <assert.h>
#include "driver.h"
#include "image.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

#include "includes/svision.h"
#include "devices/cartslot.h"

/*
supervision
watara

cartridge code is m65c02 or something more (65ce02?)



4 mhz quartz

right dil28 ram? 8kb?
left dil28 ???

integrated speaker
stereo phone jack
40 pin connector for cartridges
com port (9 pol dsub) pc at rs232?
looked at
5 4 3 2 1
 9 8 7 6
2 black -->vlsi
3 brown -->vlsi
4 yellow -->vlsi
5 red vlsi
7 violett
9 white

port for 6V power supply

on/off switch
volume control analog
contrast control analog




cartridge connector (look at the cartridge)
 /oe or /ce	1  40 +5v (picture side)
		a0  2  39 nc
		a1  3  38 nc
		a2  4  37 nc
		a3  5  36 nc
		a4  6  35 nc in crystball
		a5  7  34 d0
		a6  8  33 d1
		a7  9  32 d2
		a8  10 31 d3
		a9  11 30 d4
		a10 12 29 d5
		a11 13 28 d6
		a12 14 27 d7
        a13 15 26 nc
        a14 16 25 nc
        a15 17 24 nc
        a16?18 23 nc
        a17?19 22 gnd connected with 21 in crystalball
        a18?20 21 (shorter pin in crystalball)

adapter for dumping as 27c4001

cryst ball:
a16,a17,a18 not connected

delta hero:
a16,a17,a18 not connected


ordering of pins in the cartridge!
21,22 connected
idea: it is a 27512, and pin are in this ordering

+5V 40
a15 17
a12 14
a7   9
a6   8
a5   7
a4   6
a3   5
a2   4
a1   3
a0   2
d0  34
d1  33
d2  32
gnd 21!
d3  31
d4  30
d5  29
d6  28
d7  27
ce  21 (gnd)
a10 12
oe  1
a11 13
a9  11
a8  10
a13 15
a14 16
*/

// in pixel
#define XPOS svision_reg[2]

static UINT8 *svision_reg;
/*
  0x2000 0xa0 something to do with video dma?
  0x2001 0xa0 something to do with video dma?
  0x2010,11,12 audio channel
   offset 0,1 frequency; offset 1 always zero?
   offset 2:
    0, 0x60-0x6f
    bit 0..3: volume??
    bit 5: on left??
    bit 6: on right??
  0x2014,15,16 audio channel
  0x2020 buttons and pad
  0x2022 0x0f ?
  0x2023 timer?
   next interrupt at 256*value?
   writing sets timer and clear interrupt request?
   fast irq in crystball needed for timing
   slower irq in deltahero with music?
  0x2026 bank switching
  0x2027
   bit 0: 0x2023 timer interrupt occured

  0x2041-0x2053
  0x3041-
 */

struct {
    void *timer1;
    int timer1_shot;
} svision;

static void svision_timer(int param)
{
    svision.timer1_shot=TRUE;
    svision.timer1=NULL;
    cpu_set_irq_line(0, M65C02_IRQ_LINE, ASSERT_LINE);
}

static READ_HANDLER(svision_r)
{
    int data=svision_reg[offset];
    switch (offset) {
    case 0x20:
	data=readinputport(0);
	break;
    case 0x27:
	if (svision.timer1_shot) data|=1; //crystball irq routine
	break;
    case 0x24: case 0x25://deltahero irq routine read
	break;
    default:
	logerror("%.6f svision read %04x %02x\n",timer_get_time(),offset,data);
	break;
    }

    return data;
}

static WRITE_HANDLER(svision_w)
{
	svision_reg[offset]=data;
	switch (offset) {
	case 0x26: // bits 5,6 memory management for a000?
		cpu_setbank(1,memory_region(REGION_CPU1)+0x10000+((data&0x60)<<9) );
		break;

	case 0x23: //delta hero irq routine write
		cpu_set_irq_line(0, M65C02_IRQ_LINE, CLEAR_LINE);
		svision.timer1_shot=FALSE;
		timer_reset(svision.timer1, TIME_IN_CYCLES(data*256, 0));
		break;

	case 0x10: case 0x11: case 0x12:
		svision_soundport_w(svision_channel+0, offset&3, data);
		break;

	case 0x14: case 0x15: case 0x16:
		svision_soundport_w(svision_channel+1, offset&3, data);
		break;

	default:
		logerror("%.6f svision write %04x %02x\n",timer_get_time(),offset,data);
		break;
	}
}

static MEMORY_READ_START( readmem )
    { 0x0000, 0x1fff, MRA_RAM },
    { 0x2000, 0x3fff, svision_r },
    { 0x4000, 0x5fff, MRA_RAM }, //?
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
    { 0x2000, 0x3fff, svision_w, &svision_reg },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( svision )
	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "B", CODE_DEFAULT, CODE_DEFAULT )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "A", CODE_DEFAULT, CODE_DEFAULT )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "select", KEYCODE_5, IP_JOY_DEFAULT )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "start/pause",  KEYCODE_1, IP_JOY_DEFAULT )
INPUT_PORTS_END

/* most games contain their graphics in roms, and have hardware to
   draw complete rectangular objects */
/* look into src/drawgfx.h for more info */
/* this is for a console with monochrom hires graphics in ram
   1 byte/ 8 pixels are enlarged */
static struct GfxLayout svision_charlayout =
{
	8,	/* width of object */
	1,	/* height of object */
	256,/* 256 characters */
	2,	/* bits per pixel */
	{ 0,1 }, /* no bitplanes */
	/* x offsets */
	{ 6,4,2,0 },
	/* y offsets */
	{ 0 },
	8*1 /* size of 1 object in bits */
};

static struct GfxDecodeInfo svision_gfxdecodeinfo[] = {
	{
		REGION_GFX1, /* memory region */
		0x0000, /* offset in memory region */
		&svision_charlayout,
		0, /* index in the color lookup table where color codes start */
		1  /* total number of color codes */
	},
    { -1 } /* end of array */
};

/* palette in red, green, blue tribles */
static unsigned char svision_palette[] =
{
#if 0
    // greens grabbed from a scan of a handheld
    // in its best adjustment for contrast
	53, 73, 42,
	42, 64, 47,
	22, 42, 51,
	22, 25, 32
#else
	// grabbed from chris covell's black white pics
	0xe0, 0xe0, 0xe0,
	0xb9, 0xb9, 0xb9,
	0x54, 0x54, 0x54,
	0x12, 0x12, 0x12
#endif
};

/* color table for 1 2 color objects */
static unsigned short svision_colortable[1][4] = {
	{ 0, 1, 2, 3 }
};

static PALETTE_INIT( svision )
{
	palette_set_colors(0, svision_palette, sizeof(svision_palette) / 3);
	memcpy(colortable, svision_colortable, sizeof(svision_colortable));
}

static VIDEO_UPDATE( svision )
{
	int x, y, i, j;
	UINT8 *vram=memory_region(REGION_CPU1)+0x4000+XPOS/4;

	for (y=0,i=0; y<160; y++,i+=0x30) {
		for (x=0,j=i; x<160; x+=4,j++) {
			drawgfx(bitmap, Machine->gfx[0], vram[j],0,0,0,
				x,y, 0, TRANSPARENCY_NONE,0);
		}
	}
}

static INTERRUPT_GEN( svision_frame_int )
{
	cpu_set_nmi_line(0, PULSE_LINE);
}

static DRIVER_INIT( svision )
{
	UINT8 *gfx=memory_region(REGION_GFX1);
	int i;
	for (i=0; i<256;i++)
		gfx[i]=i;
}

static MACHINE_INIT( svision )
{
	svision.timer1 = timer_alloc(svision_timer);
    svision.timer1_shot = FALSE;
}

struct CustomSound_interface svision_sound_interface =
{
	svision_custom_start,
	svision_custom_stop,
	svision_custom_update
};


static MACHINE_DRIVER_START( svision )
	/* basic machine hardware */
	MDRV_CPU_ADD(M65C02, 4000000)        /* ? stz used! speed? */
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_VBLANK_INT(svision_frame_int, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( svision )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)	/* lcd */
	MDRV_SCREEN_SIZE(160, 160)
	MDRV_VISIBLE_AREA(0, 160-1, 0, 160-1)
	MDRV_GFXDECODE( svision_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(svision_palette) / sizeof(svision_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof (svision_colortable) / sizeof(svision_colortable[0][0]))
	MDRV_PALETTE_INIT( svision )

	MDRV_VIDEO_UPDATE( svision )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, svision_sound_interface)
MACHINE_DRIVER_END

ROM_START(svision)
	ROM_REGION(0x20000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

/* deltahero
 c000
  dd6a clear 0x2000 at ($57/58) (0x4000)
  deb6 clear hardware regs
   e35d clear hardware reg
   e361 clear hardware reg
  e3a4
 c200

 nmi c053 ?
 irq c109
      e3f7
      def4
 routines:
 dd6a clear 0x2000 at ($57/58) (0x4000)
 */

static DEVICE_LOAD( svision_cart )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;

	size = mame_fsize(file);
	if (size>0x10000)
	{
	    logerror("%s: size %d not yet supported\n",image_filename(image), size);
	    return 1;
	}

	if (mame_fread(file, rom+0x20000-size, size)!=size)
	{
		logerror("%s load error\n",image_filename(image));
		return 1;
	}
	if (size==0x8000)
	{
	    memcpy(rom+0x10000, rom+0x20000-size, size);
	}
	memcpy(rom+0xc000, rom+0x1c000, 0x10000-0xc000);
	return 0;
}

SYSTEM_CONFIG_START(svision)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "bin\0", NULL, NULL, device_load_svision_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT	MACHINE		INPUT		INIT		CONFIG		COMPANY		FULLNAME */
CONSX(1992,	svision,	0,		svision,	svision,	svision,	svision,	"Watara",	"Super Vision", GAME_IMPERFECT_SOUND)
// marketed under a ton of firms and names

