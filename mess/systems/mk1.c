/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at September 2000
******************************************************************************/

#include "driver.h"

#include "includes/mk1.h"

/*
chess champion mk i


Signetics 7916E C48091 82S210-1 COPYRIGHT
2 KB rom? 2716 kompatible?

4 11 segment displays (2. point up left, verticals in the mid )

2 x 2111 256x4 SRAM

MOSTEK MK 3853N 7915 Philippines ( static memory interface for f8)

MOSTEK MK 3850N-3 7917 Philipines (fairchild f8 cpu)

16 keys (4x4 matrix)

switch s/l
(power on switch)
 */

static MEMORY_READ_START( mk1_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x08ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( mk1_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x08ff, MWA_RAM },
MEMORY_END


static PORT_READ_START( mk1_readport )
	// 0, 1 integrated io
	// c,d,e,f timer in 3853? (position mask programmed)
PORT_END

static PORT_WRITE_START( mk1_writeport )
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( mk1 )
	PORT_START
	PORT_DIPNAME ( 0x01, 0x01, "Switch")
	PORT_DIPSETTING(  0, "L" )
	PORT_DIPSETTING(  1, "S" )
	PORT_START
	DIPS_HELPER( 0x001, "White A    King", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x002, "White B    Queen", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x004, "White C    Bishop", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x008, "White D    PLAY", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x010, "White E    Knight", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x020, "White F    Castle", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x040, "White G    Pawn", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x080, "White H    md", KEYCODE_H, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "Black 1    King", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x002, "Black 2    Queen", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x004, "Black 3    Bishop", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x008, "Black 4    fp", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x010, "Black 5    Knight", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x020, "Black 6    Castle", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x040, "Black 7    Pawn", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x080, "Black 8    ep", KEYCODE_8, CODE_NONE)
#if 0
	PORT_START
	DIPS_HELPER( 0x001, "White 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x002, "White 2", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "White 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x008, "White 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "White 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x020, "White 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "White 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x080, "White 8", KEYCODE_8_PAD, CODE_NONE)
#endif
INPUT_PORTS_END

static int mk1_frame_int(void)
{
	return 0;
}

#if 0
static void mk1_machine_init(void)
{
}
#endif

static struct MachineDriver machine_driver_mk1 =
{
	/* basic machine hardware */
	{
		{
			CPU_F8, //MK3850
			1000000,
			mk1_readmem,mk1_writemem,
			mk1_readport,mk1_writeport,
			mk1_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION, // lcd!
	1,				/* single CPU */
	0,//mk2_machine_init,
	0,//pc1401_machine_stop,

	626, 323, { 0, 626 - 1, 0, 323 - 1},
	0,			   /* graphics decode info */
	sizeof (mk1_palette) / sizeof (mk1_palette[0]) ,
	sizeof (mk1_colortable) / sizeof(mk1_colortable[0][0]),
	mk1_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    mk1_vh_start,
	mk1_vh_stop,
	mk1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ 0 }
    }
};

ROM_START(mk1)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("82c210-1", 0x0000, 0x800, 0x278f7bf3)
ROM_END

static const struct IODevice io_mk1[] = {
    { IO_END }
};

// seams to be developed by mostek (MK) 
/*    YEAR  NAME    PARENT  MACHINE INPUT   INIT      COMPANY   FULLNAME */
CONS( 1979,	mk1,	0, 		mk1,	mk1,	0,	"Computer Electronic",  "Chess Champion MK I")

#ifdef RUNTIME_LOADER
extern void mk1_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"mk1")==0) drivers[i]=&driver_mk1;
	}
}
#endif
