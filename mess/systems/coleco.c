/***************************************************************************

  coleco.c

  Driver file to handle emulation of the Colecovision.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young

  TODO:
    - Extra Controller Support

***************************************************************************/


#include "driver.h"
#include "sound/sn76496.h"
#include "vidhrdw/tms9928a.h"
#include "includes/coleco.h"

static MEMORY_READ_START( coleco_readmem )
    { 0x0000, 0x1FFF, MRA_ROM },  /* COLECO.ROM */
    { 0x6000, 0x63ff, MRA_RAM },
    { 0x6400, 0x67ff, MRA_RAM },
    { 0x6800, 0x6bff, MRA_RAM },
    { 0x6c00, 0x6fff, MRA_RAM },
    { 0x7000, 0x73ff, MRA_RAM },
    { 0x7400, 0x77ff, MRA_RAM },
    { 0x7800, 0x7bff, MRA_RAM },
    { 0x7c00, 0x7fff, MRA_RAM },
    { 0x8000, 0xFFFF, MRA_ROM },  /* Cartridge */
MEMORY_END

static MEMORY_WRITE_START( coleco_writemem )
    { 0x0000, 0x1FFF, MWA_ROM }, /* COLECO.ROM */
    { 0x6000, 0x63ff, MWA_RAM },
    { 0x6400, 0x67ff, MWA_RAM },
    { 0x6800, 0x6bff, MWA_RAM },
    { 0x6c00, 0x6fff, MWA_RAM },
    { 0x7000, 0x73ff, MWA_RAM },
    { 0x7400, 0x77ff, MWA_RAM },
    { 0x7800, 0x7bff, MWA_RAM },
    { 0x7c00, 0x7fff, MWA_RAM },
    { 0x8000, 0xFFFF, MWA_ROM }, /* Cartridge */
MEMORY_END


static PORT_READ_START (coleco_readport)
    { 0xA0, 0xA0, TMS9928A_vram_r },
    { 0xA1, 0xA1, TMS9928A_register_r },
    { 0xBE, 0xBE, TMS9928A_vram_r },
    { 0xBF, 0xBF, TMS9928A_register_r },
	{ 0xE0, 0xFF, coleco_paddle_r },
PORT_END

static PORT_WRITE_START (coleco_writeport)
    { 0x80, 0x80, coleco_paddle_toggle_off },
    { 0x9F, 0x9F, coleco_paddle_toggle_off }, /* Antarctic Adventure */
    { 0xA0, 0xA0, TMS9928A_vram_w },
    { 0xA1, 0xA1, TMS9928A_register_w },
    { 0xBE, 0xBE, TMS9928A_vram_w },
    { 0xBF, 0xBF, TMS9928A_register_w },
    { 0xC0, 0xC0, coleco_paddle_toggle_on },
    { 0xDF, 0xDF, coleco_paddle_toggle_on }, /* Antarctic Adventure */
    { 0xE0, 0xFF, SN76496_0_w },
PORT_END

INPUT_PORTS_START( coleco )
    PORT_START  /* IN0 */

    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "0 (pad 1)", KEYCODE_0, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "1 (pad 1)", KEYCODE_1, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "2 (pad 1)", KEYCODE_2, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, "3 (pad 1)", KEYCODE_3, IP_JOY_DEFAULT)
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_TILT, "4 (pad 1)", KEYCODE_4, IP_JOY_DEFAULT)
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_TILT, "5 (pad 1)", KEYCODE_5, IP_JOY_DEFAULT)
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_TILT, "6 (pad 1)", KEYCODE_6, IP_JOY_DEFAULT)
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_TILT, "7 (pad 1)", KEYCODE_7, IP_JOY_DEFAULT)

    PORT_START  /* IN1 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "8 (pad 1)", KEYCODE_8, IP_JOY_DEFAULT)
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "9 (pad 1)", KEYCODE_9, IP_JOY_DEFAULT)
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "# (pad 1)", KEYCODE_MINUS, IP_JOY_DEFAULT)
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, ". (pad 1)", KEYCODE_EQUALS, IP_JOY_DEFAULT)
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN2 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN3 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "0 (pad 2)", KEYCODE_0_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "1 (pad 2)", KEYCODE_1_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "2 (pad 2)", KEYCODE_2_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, "3 (pad 2)", KEYCODE_3_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_TILT, "4 (pad 2)", KEYCODE_4_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_TILT, "5 (pad 2)", KEYCODE_5_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_TILT, "6 (pad 2)", KEYCODE_6_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_TILT, "7 (pad 2)", KEYCODE_7_PAD, IP_JOY_DEFAULT )

    PORT_START  /* IN4 */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_TILT, "8 (pad 2)", KEYCODE_8_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_TILT, "9 (pad 2)", KEYCODE_9_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_TILT, "# (pad 2)", KEYCODE_MINUS_PAD, IP_JOY_DEFAULT )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_TILT, ". (pad 2)", KEYCODE_PLUS_PAD, IP_JOY_DEFAULT )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* IN5 */
    PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
    PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
    PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
    PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT ( 0xB0, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

static struct SN76496interface sn76496_interface =
{
    1,  		/* 1 chip 		*/
    {3579545},  /* 3.579545 MHz */
    { 100 }
};

/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static int coleco_interrupt(void)
{
    TMS9928A_interrupt();
    return ignore_interrupt ();
}

static void coleco_vdp_interrupt (int state)
{
	static int last_state = 0;

    /* only if it goes up */
	if (state && !last_state) cpu_set_nmi_line(0, PULSE_LINE);
	last_state = state;
}

static int coleco_vh_start(void)
{
	if (TMS9928A_start(TMS99x8A, 0x4000)) return 1;

	TMS9928A_int_callback(coleco_vdp_interrupt);
	return 0;
}

static struct MachineDriver machine_driver_coleco =
{
    /* basic machine hardware */
    {
        {
            CPU_Z80,
            3579545,    /* 3.579545 Mhz */
            coleco_readmem,coleco_writemem,coleco_readport,coleco_writeport,
            coleco_interrupt,1
        }
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
    1,
    0, /* init_machine */
	0, /* stop_machine */

    32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
    0, /* gfxdecodeinfo */
    TMS9928A_PALETTE_SIZE,
    TMS9928A_COLORTABLE_SIZE,
    tms9928A_init_palette,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
    0,
    coleco_vh_start,
    TMS9928A_stop,
    TMS9928A_refresh,

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

ROM_START (coleco)
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD ("coleco.rom", 0x0000, 0x2000, 0x3aa93ef3)
ROM_END

ROM_START (colecoa)
    ROM_REGION(0x10000,REGION_CPU1, 0)
    ROM_LOAD ("colecoa.rom", 0x0000, 0x2000, 0x39bb16fc)

	/* differences to 0x3aa93ef3
	   modified characters, added a pad 2 related fix */
ROM_END

//ROM_START (colecofb_rom)
//  ROM_REGIONX(0x10000,REGION_CPU1, 0)
//  ROM_LOAD ("colecofb.rom", 0x0000, 0x2000, 0x640cf85b) /* no pause after title screen */
//ROM_END

//ROM_START (coleconb_rom)
//  ROM_REGIONX(0x10000,REGION_CPU1, 0)
//  ROM_LOAD ("coleconb.rom", 0x0000, 0x2000, 0x66cda476) /* no title screen */
//ROM_END

static const struct IODevice io_coleco[] = {
    {
        IO_CARTSLOT,        /* type */
        1,                  /* count */
        "rom\0",            /* file extensions */
        IO_RESET_CPU,       /* reset if file changed */
        0,
        coleco_init_cart,	/* init */
        NULL,               /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

#define io_colecoa io_coleco

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
CONS( 1982, coleco,   0,        coleco,   coleco,   0,        "Coleco", "Colecovision" )
CONS( 1982, colecoa,  coleco,   coleco,   coleco,   0,        "Coleco", "Colecovision (Thick Characters)" )

#ifdef COLECO_HACKS
CONSX( 1982, colecofb, coleco,   coleco,   coleco,   0,        "Coleco", "Colecovision (Fast BIOS Hack)"
, GAME_COMPUTER_MODIFIED )
CONSX( 1982, coleconb, coleco,   coleco,   coleco,   0,        "Coleco", "Colecovision (NO BIOS Hack)"
, GAME_COMPUTER_MODIFIED )
#endif

