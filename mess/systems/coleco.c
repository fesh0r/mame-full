/***************************************************************************

  coleco.c

  Driver file to handle emulation of the Colecovision.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young

  NEWS:
    - Modified memory map, now it has only 1k of RAM mapped on 8k Slot
    - Modified I/O map, now it is handled as on a real ColecoVision:
        The I/O map is broken into 4 write and 4 read ports:
            80-9F (W) = Set both controllers to keypad mode
            80-9F (R) = Not Connected

            A0-BF (W) = Video Chip (TMS9928A), A0=0 -> Write Register 0 , A0=1 -> Write Register 1
            A0-BF (R) = Video Chip (TMS9928A), A0=0 -> Read Register 0 , A0=1 -> Read Register 1

            C0-DF (W) = Set both controllers to joystick mode 
            C0-DF (R) = Not Connected

            E0-FF (W) = Sound Chip (SN76496)
            E0-FF (R) = Read Controller data, A1=0 -> read controller 1, A1=1 -> read controller 2

    - Modified paddle handler, now it is handled as on a real ColecoVision
    - Added support for a Driving Controller (Expansion Module #2), enabled via configuration
    - Added support for a Roller Controller (Trackball), enabled via configuration
    - Added support for two Super Action Controller, enabled via configuracion

    EXTRA CONTROLLERS INFO:

        -Driving Controller (Expansion Module #2). It consist of a steering wheel and a gas pedal. Only one
         can be used on a real ColecoVision. The gas pedal is not analog, internally it is just a switch.
         On a real ColecoVision, when the Driving Controller is enabled, the controller 1 do not work because 
         have been replaced by the Driving Controller, and controller 2 have to be used to start game, gear 
         shift, etc.
         Driving Controller is just a spinner on controller 1 socket similar to the one on Roller Controller
         and Super Action Controllers so you can use Roller Controller or Super Action Controllers to play 
         games requiring Driving Controller.
         
        -Roller Controller. Basically a trackball with four buttons (the two fire buttons from player 1 and
         the two fire buttons from player 2). Only one Roller Controller can be used on a real ColecoVision.
	 Roller Controller is connected to both controller sockets and both controllers are conected to the Roller
	 Controller, it uses the spinner pins of both sockets to generate the X and Y signals (X from controller 1 
	 and the Y from controller 2)

        -Super Action Controllers. It is a hand controller with a keypad, four buttons (the two from
         the player pad and two more), and a spinner. This was made primarily for two player sport games, but
         will work for every other ColecoVision game.

***************************************************************************/


#include "driver.h"
#include "sound/sn76496.h"
#include "vidhrdw/tms9928a.h"
#include "includes/coleco.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( coleco_readmem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x1FFF) AM_READ( MRA8_ROM )  /* COLECO.ROM (ColecoVision OS7 Bios) */
    AM_RANGE( 0x2000, 0x5FFF) AM_READ( MRA8_NOP )  /* No memory here */
    AM_RANGE( 0x6000, 0x7fff) AM_READ( coleco_mem_r )  /* 1Kbyte RAM mapped on 8Kbyte Slot */
    AM_RANGE( 0x8000, 0xFFFF) AM_READ( MRA8_ROM )  /* Cartridge (32k max)*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( coleco_writemem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x1FFF) AM_WRITE( MWA8_ROM ) /* COLECO.ROM (ColecoVision OS7 Bios) */
    AM_RANGE( 0x2000, 0x5FFF) AM_WRITE( MWA8_NOP ) /* No memory here */
    AM_RANGE( 0x6000, 0x7fff) AM_WRITE( coleco_mem_w ) /* 1Kbyte RAM mapped on 8Kbyte Slot */
    AM_RANGE( 0x8000, 0xFFFF) AM_WRITE( MWA8_ROM ) /* Cartridge (32k max)*/
ADDRESS_MAP_END

READ_HANDLER(coleco_mem_r)
{
    return memory_region(REGION_CPU1)[0x6000+(offset&0x3ff)];
}

WRITE_HANDLER(coleco_mem_w)
{
    memory_region(REGION_CPU1)[0x6000+(offset&0x3ff)] = data;
}

static ADDRESS_MAP_START (coleco_readport, ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0xA0, 0xBF) AM_READ( coleco_video_r )
    AM_RANGE( 0xE0, 0xFF) AM_READ( coleco_paddle_r )
ADDRESS_MAP_END

static ADDRESS_MAP_START (coleco_writeport, ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x80, 0x9F) AM_WRITE( coleco_paddle_toggle_off )
    AM_RANGE( 0xA0, 0xBF) AM_WRITE( coleco_video_w )
    AM_RANGE( 0xC0, 0xDF) AM_WRITE( coleco_paddle_toggle_on )
    AM_RANGE( 0xE0, 0xFF) AM_WRITE( SN76496_0_w )
ADDRESS_MAP_END

READ_HANDLER(coleco_video_r)
{  
    return ((offset&0x01) ? TMS9928A_register_r(1) : TMS9928A_vram_r(0));
}

WRITE_HANDLER(coleco_video_w)
{
    (offset&0x01) ? TMS9928A_register_w(1, data) : TMS9928A_vram_w(0, data);
}

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

    PORT_START /* IN6 */
    PORT_DIPNAME(0x07, 0x00, "Extra Controllers")
    PORT_DIPSETTING(0x00, "None" )
    PORT_DIPSETTING(0x01, "Driving Controller" )
    PORT_DIPSETTING(0x02, "Roller Controller" )
    PORT_DIPSETTING(0x04, "Super Action Controllers" )

    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3, "SAC Blue Button P1", KEYCODE_Z, IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4, "SAC Purple Button P1", KEYCODE_X, IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "SAC Blue Button P2", KEYCODE_Q, IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "SAC Purple Button P2", KEYCODE_W, IP_JOY_DEFAULT )        

    PORT_START	/* IN7, to emulate Extra Controls (Driving Controller, SAC P1 slider, Roller Controller X Axis) */
    PORT_ANALOGX( 0x0f, 0x00, IPT_TRACKBALL_X | IPF_CENTER, 20, 10, 0, 0, KEYCODE_L, KEYCODE_J, IP_JOY_NONE, IP_JOY_NONE )

    PORT_START	/* IN8, to emulate Extra Controls (SAC P2 slider, Roller Controller Y Axis) */
    PORT_ANALOGX( 0x0f, 0x00, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER2, 20, 10, 0, 0, KEYCODE_I, KEYCODE_K, IP_JOY_NONE, IP_JOY_NONE )

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

extern int JoyStat[2];

static INTERRUPT_GEN( coleco_interrupt )
{
    TMS9928A_interrupt();
}

static void coleco_vdp_interrupt (int state)
{
	static int last_state = 0;

    /* only if it goes up */
	if (state && !last_state) cpu_set_nmi_line(0, PULSE_LINE);
	last_state = state;
}

void paddle_callback (int param)
{
    int port7 = input_port_7_r (0);
    int port8 = input_port_8_r (0);

    if (port7==0) JoyStat[0] = 0;
    else if (port7&0x08) JoyStat[0] = -1;
    else JoyStat[0] = 1;

    if (port8==0) JoyStat[1] = 0;
    else if (port8&0x08) JoyStat[1] = -1;
    else JoyStat[1] = 1;

/*printf("%0d\n",port7);*/
    if (JoyStat[0] || JoyStat[1]) cpu_set_irq_line (0, 0, HOLD_LINE);
}

static MACHINE_INIT(coleco)
{
    cpu_irq_line_vector_w(0,0,0xff);
	memset(&memory_region(REGION_CPU1)[0x6000], 0xFF, 0x2000); /* Initializing RAM */
    timer_pulse(TIME_IN_MSEC(20), 0, paddle_callback);
} 

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	coleco_vdp_interrupt
};

static MACHINE_DRIVER_START( coleco )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3579545)       /* 3.579545 Mhz */
	MDRV_CPU_PROGRAM_MAP(coleco_readmem,coleco_writemem)
	MDRV_CPU_IO_MAP(coleco_readport,coleco_writeport)
	MDRV_CPU_VBLANK_INT(coleco_interrupt,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( coleco )

    /* video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (coleco)
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD ("coleco.rom", 0x0000, 0x2000, CRC(3aa93ef3))
ROM_END

ROM_START (colecoa)
    ROM_REGION(0x10000,REGION_CPU1, 0)
    ROM_LOAD ("colecoa.rom", 0x0000, 0x2000, CRC(39bb16fc))

	/* differences to 0x3aa93ef3
	   modified characters, added a pad 2 related fix */
ROM_END

#ifdef COLECO_HACKS
ROM_START (colecofb_rom)
  ROM_REGIONX(0x10000,REGION_CPU1, 0)
  ROM_LOAD ("colecofb.rom", 0x0000, 0x2000, CRC(640cf85b)) /* no pause after title screen */
ROM_END

ROM_START (coleconb_rom)
  ROM_REGIONX(0x10000,REGION_CPU1, 0)
  ROM_LOAD ("coleconb.rom", 0x0000, 0x2000, CRC(66cda476)) /* no title screen */
ROM_END
#endif

SYSTEM_CONFIG_START(coleco)
	CONFIG_DEVICE_CARTSLOT_OPT( 1, "rom\0col\0bin\0", NULL, NULL, device_load_coleco_cart, NULL, coleco_cart_verify, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY   FULLNAME */
CONS( 1982, coleco,   0,		0,		coleco,   coleco,   0,		coleco,	"Coleco", "Colecovision" )
CONS( 1982, colecoa,  coleco,	0,		coleco,   coleco,   0,		coleco,	"Coleco", "Colecovision (Thick Characters)" )

#ifdef COLECO_HACKS
CONSX(1982, colecofb, coleco,	0,		coleco,   coleco,   0,		coleco,	"Coleco", "Colecovision (Fast BIOS Hack)"
, GAME_COMPUTER_MODIFIED )
CONSX(1982, coleconb, coleco,	0,		coleco,   coleco,   0,		coleco,	"Coleco", "Colecovision (NO BIOS Hack)"
, GAME_COMPUTER_MODIFIED )
#endif

