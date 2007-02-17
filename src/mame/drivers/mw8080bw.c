/***************************************************************************

    Midway 8080-based black and white hardware

    driver by Michael Strutts, Nicola Salmoria, Tormod Tjaberg, Mirko Buffoni,
    Lee Taylor, Valerio Verrando, Marco Cassili, Zsolt Vasvari and others

    Games supported:
        * Sea Wolf
        * Gun Fight
        * Tornado Baseball
        * Datsun 280 Zzzap
        * Amazing Maze
        * Boot Hill
        * Checkmate
        * Desert Gun
        * Double Play
        * Laguna Racer
        * Guided Missile
        * M-4
        * Clowns
        * Extra Inning
        * Shuffleboard
        * Dog Patch
        * Space Encounters
        * Phantom II
        * Bowling Alley
        * Space Invaders
        * Blue Shark
        * Space Invaders II (Midway, cocktail version)
        * Space Invaders Deluxe (cocktail version)

    Other games on this basic hardware:
        * Space Walk
        * Bowling Alley (cocktail version)

    Notes:
        * The Amazing Maze Game" on title screen, but manual, flyer,
          cabinet side art all call it just "Amazing Maze"
        * Desert Gun was originally named Road Runner. The name was changed
          when Midway merged with Bally who had a game by the same title

    Known issues/to-do's:
        * Space Encounters: verify trench colors
        * Space Encounters: verify strobe light frequency
        * Phantom II: cloud generator is implemented according to the schematics,
          but it doesn't look right.  Cloud color mixing to be verified as well
        * Dog Patch: find schematics and verify all assumptions


****************************************************************************

    Memory map

****************************************************************************

    ========================================================================
    MAIN CPU memory address space
    ========================================================================

    Address (15-bits) Dir Data     Description
    ----------------- --- -------- -----------------------
    x0xxxxxxxxxxxxx   R   xxxxxxxx Program ROM (various amounts populated)
    -1xxxxxxxxxxxxx   R/W xxxxxxxx Video RAM (256x256x1 bit display)
                                   Portion in VBLANK region used as work RAM
    Legend: (x)   bit significant
            (-)   bit ignored
            (0/1) bit must be given value

    The I/O address space is used differently from game to game.

****************************************************************************/

#include "driver.h"
#include "rendlay.h"
#include "rescap.h"
#include "timer.h"
#include "mw8080bw.h"

#include "clowns.lh"
#include "invaders.lh"
#include "invad2ct.lh"



/*************************************
 *
 *  Shifter circuit
 *
 *************************************/

static UINT16 shift_data;	/* 15 bits only */
static UINT8 shift_amount;	/* 3 bits */
static UINT8 rev_shift_res;


INLINE UINT8 do_shift(void)
{
	return shift_data >> shift_amount;
}


static READ8_HANDLER( mw8080bw_shift_res_r )
{
	return do_shift();
}


static READ8_HANDLER( mw8080bw_shift_res_rev_r )
{
	UINT8 ret = do_shift();

	return BITSWAP8(ret,0,1,2,3,4,5,6,7);
}


static READ8_HANDLER( mw8080bw_reversable_shift_res_r )
{
	UINT8 ret;

	if (rev_shift_res)
	{
		ret = mw8080bw_shift_res_rev_r(0);
	}
	else
	{
		ret = mw8080bw_shift_res_r(0);
	}

	return ret;
}

static WRITE8_HANDLER( mw8080bw_shift_amount_w )
{
	shift_amount = ~data & 0x07;
}


static WRITE8_HANDLER( mw8080bw_reversable_shift_amount_w)
{
	mw8080bw_shift_amount_w(offset, data);

	rev_shift_res = data & 0x08;
}


static WRITE8_HANDLER( mw8080bw_shift_data_w )
{
	shift_data = (shift_data >> 8) | ((UINT16)data << 7);
}



/*************************************
 *
 *  Interrupt generation
 *
 *************************************/

/* an interrupt is raised when the expression
   !(!(!VPIXCOUNT14 & VPIXCOUNT15) & !VBLANK) goes from LO to HI.
   This happens when the vertical pixel count is 0x80 and 0xda and
   VBLANK is 0 and 1, respectively.  These correspond to lines
   96 and 224 as displayed.  The interrupt vector is given by the
   expression: 0xc7 | (VPIXCOUNT14 << 4) | (!VPIXCOUNT14 << 3),
   giving 0xcf and 0xd7 for the vectors. */

static mame_timer *interrupt_timer;


static void mw8080bw_interrupt_callback(int param)
{
	/* determine vector and trigger interrupt based on the current vertical position */
	int vpos = video_screen_get_vpos(0);

	UINT8 vector = (vpos & 0x80) ? 0xd7 : 0xcf;  /* rst 10h/08h */
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, vector);

	/* set up for next interrupt */
	vpos = (vpos == 96) ? 224 : 96;
	mame_timer_adjust(interrupt_timer, video_screen_get_time_until_pos(0, vpos, 0), 0, time_zero);
}


static void mw8080bw_create_interrupt_timer(void)
{
	interrupt_timer = timer_alloc(mw8080bw_interrupt_callback);
}


static void mw8080bw_start_interrupt_timer(void)
{
	mame_timer_adjust(interrupt_timer, video_screen_get_time_until_pos(0, 96, 0), 0, time_zero);
}



/*************************************
 *
 *  Machine setup
 *
 *************************************/

static MACHINE_START( mw8080bw )
{
	mw8080bw_create_interrupt_timer();

	/* setup for save states */
	state_save_register_global(shift_data);
	state_save_register_global(shift_amount);
	state_save_register_global(rev_shift_res);

	return machine_start_mw8080bw_audio(machine);
}



/*************************************
 *
 *  Machine reset
 *
 *************************************/

static MACHINE_RESET( mw8080bw)
{
	mw8080bw_start_interrupt_timer();
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_MIRROR(0x4000) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x5fff) AM_ROM
ADDRESS_MAP_END


static MACHINE_DRIVER_START( root )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",8080,MW8080BW_CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_MACHINE_START(mw8080bw)
	MDRV_MACHINE_RESET(mw8080bw)

	/* video hardware
       +4 is added to HBSTART because the hardware displays that many pixels after
       setting HBLANK */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_RAW_PARAMS(MW8080BW_PIXEL_CLOCK, MW8080BW_HTOTAL, MW8080BW_HBEND, MW8080BW_HBSTART + 4, MW8080BW_VTOTAL, MW8080BW_VBEND, MW8080BW_VBSTART) \
	MDRV_VIDEO_UPDATE(mw8080bw)

MACHINE_DRIVER_END



/*************************************
 *
 *  Sea Wolf (PCB #596)
 *
 *************************************/

#define SEAWOLF_ERASE_SW_PORT_TAG	("ERASESW")
#define SEAWOLF_ERASE_DIP_PORT_TAG	("ERASEDIP")


static WRITE8_HANDLER( seawolf_explosion_lamp_w )
{
/*  D0-D3 are column drivers and D4-D7 are row drivers.
    The following table shows values that light up individual lamps.

    D7 D6 D5 D4 D3 D2 D1 D0   Function
    --------------------------------------------------------------------------------------
     0  0  0  1  1  0  0  0   Explosion Lamp 0
     0  0  0  1  0  1  0  0   Explosion Lamp 1
     0  0  0  1  0  0  1  0   Explosion Lamp 2
     0  0  0  1  0  0  0  1   Explosion Lamp 3
     0  0  1  0  1  0  0  0   Explosion Lamp 4
     0  0  1  0  0  1  0  0   Explosion Lamp 5
     0  0  1  0  0  0  1  0   Explosion Lamp 6
     0  0  1  0  0  0  0  1   Explosion Lamp 7
     0  1  0  0  1  0  0  0   Explosion Lamp 8
     0  1  0  0  0  1  0  0   Explosion Lamp 9
     0  1  0  0  0  0  1  0   Explosion Lamp A
     0  1  0  0  0  0  0  1   Explosion Lamp B
     1  0  0  0  1  0  0  0   Explosion Lamp C
     1  0  0  0  0  1  0  0   Explosion Lamp D
     1  0  0  0  0  0  1  0   Explosion Lamp E
     1  0  0  0  0  0  0  1   Explosion Lamp F
*/
	int i;

	static const char *lamp_names[] =
	{
		"EXP_LAMP_0", "EXP_LAMP_1", "EXP_LAMP_2", "EXP_LAMP_3",
		"EXP_LAMP_4", "EXP_LAMP_5", "EXP_LAMP_6", "EXP_LAMP_7",
		"EXP_LAMP_8", "EXP_LAMP_9", "EXP_LAMP_A", "EXP_LAMP_B",
		"EXP_LAMP_C", "EXP_LAMP_D", "EXP_LAMP_E", "EXP_LAMP_F"
	};

	static const UINT8 bits_for_lamps[] =
	{
		0x18, 0x14, 0x12, 0x11,
		0x28, 0x24, 0x22, 0x21,
		0x48, 0x44, 0x42, 0x41,
		0x88, 0x84, 0x82, 0x81
	};

	/* set each lamp */
	for (i = 0; i < 16; i++)
	{
		UINT8 bits_for_lamp = bits_for_lamps[i];

		output_set_value(lamp_names[i], (data & bits_for_lamp) == bits_for_lamp);
	}
}


static WRITE8_HANDLER( seawolf_periscope_lamp_w )
{
	/* the schematics and the connecting diagrams show the
       torpedo light order differently, but this order is
       confirmed by the software */
	output_set_value("TORP_LAMP_4", (data >> 0) & 0x01);
	output_set_value("TORP_LAMP_3", (data >> 1) & 0x01);
	output_set_value("TORP_LAMP_2", (data >> 2) & 0x01);
	output_set_value("TORP_LAMP_1", (data >> 3) & 0x01);

	output_set_value("READY_LAMP",  (data >> 4) & 0x01);

	output_set_value("RELOAD_LAMP", (data >> 5) & 0x01);
}


static UINT32 seawolf_erase_input_r(void *param)
{
	return readinputportbytag(SEAWOLF_ERASE_SW_PORT_TAG) &
		   readinputportbytag(SEAWOLF_ERASE_DIP_PORT_TAG);
}


static ADDRESS_MAP_START( seawolf_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(seawolf_explosion_lamp_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(seawolf_periscope_lamp_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(seawolf_sh_port_w)
ADDRESS_MAP_END


static const UINT32 graybit5_controller_table[32] =
{
	0x0f, 0x0e, 0x0c, 0x0d, 0x09, 0x08, 0x0a, 0x0b,
	0x03, 0x02, 0x00, 0x01, 0x05, 0x04, 0x06, 0x07,
	0x17, 0x16, 0x14, 0x15, 0x11, 0x10, 0x12, 0x13,
	0x1b, 0x1a, 0x18, 0x19, 0x1d, 0x1c, 0x1e, 0x1f
};


static INPUT_PORTS_START( seawolf )
	PORT_START_TAG("IN0")
	/* The grey code is inverted by buffers and the controls are mirrored */
	PORT_BIT( 0x1f, 0x10, IPT_POSITIONAL ) PORT_POSITIONS(32) PORT_REMAP_TABLE(graybit5_controller_table) PORT_INVERT PORT_REVERSE PORT_SENSITIVITY(20) PORT_KEYDELTA(8) PORT_CENTERDELTA(0) PORT_NAME("Periscope axis") PORT_CROSSHAIR(X, -((float)MW8080BW_HBSTART - 8 -4)/MW8080BW_HBSTART, 8.0/MW8080BW_HBSTART, 32.0/MW8080BW_VBSTART)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Game_Time ) ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0xe0) PORT_DIPLOCATION("G4:1,2")
	PORT_DIPSETTING(    0x00, "60 seconds + 20 extended" ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(    0x40, "70 seconds + 20 extended" ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(    0x80, "80 seconds + 20 extended" ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(    0xc0, "90 seconds + 20 extended" ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0x00)
	PORT_DIPSETTING(    0x00, "60 seconds" ) PORT_CONDITION("IN1",0xe0,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x40, "70 seconds" ) PORT_CONDITION("IN1",0xe0,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x80, "80 seconds" ) PORT_CONDITION("IN1",0xe0,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0xc0, "90 seconds" ) PORT_CONDITION("IN1",0xe0,PORTCOND_EQUALS,0x00)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN1",0xe0,PORTCOND_NOTEQUALS,0xe0) PORT_DIPLOCATION("G4:3,4")
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(seawolf_erase_input_r, 0)
	PORT_DIPNAME( 0xe0, 0x60, "Extended Time At" ) PORT_DIPLOCATION("G4:6,7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x20, "2000" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x60, "4000" )
	PORT_DIPSETTING(    0x80, "5000" )
	PORT_DIPSETTING(    0xa0, "6000" )
	PORT_DIPSETTING(    0xc0, "7000" )
	PORT_DIPSETTING(    0xe0, "Test Mode" )

	/* 2 fake ports for the 'Reset High Score' input, which has a DIP to enable it */
	PORT_START_TAG(SEAWOLF_ERASE_SW_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Reset High Score") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(SEAWOLF_ERASE_DIP_PORT_TAG)
	PORT_DIPNAME( 0x01, 0x01, "Enable Reset High Score Button" ) PORT_DIPLOCATION("G4:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( seawolf )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(seawolf_io_map,0)
	/* there is no watchdog */

	/* sound hardware */
	MDRV_IMPORT_FROM(seawolf_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Gun Fight (PCB #597)
 *
 *************************************/

static WRITE8_HANDLER( gunfight_io_w )
{
	if (offset & 0x01)  gunfight_sh_port_w(0, data);

	if (offset & 0x02)  mw8080bw_shift_amount_w(0, data);

	if (offset & 0x04)  mw8080bw_shift_data_w(0, data);
}


static ADDRESS_MAP_START( gunfight_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	/* no decoder, just 3 AND gates */
	AM_RANGE(0x00, 0x07) AM_WRITE(gunfight_io_w)
ADDRESS_MAP_END


/* the schematic shows the table different, but these are correct */
static const UINT32 gunfight_controller_table[7] =
{
	0x01, 0x05, 0x07, 0x03, 0x02, 0x06, 0x04
};


static INPUT_PORTS_START( gunfight )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x70, 0x03, IPT_POSITIONAL ) PORT_POSITIONS(7) PORT_REMAP_TABLE(gunfight_controller_table) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_H) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x70, 0x03, IPT_POSITIONAL ) PORT_POSITIONS(7) PORT_REMAP_TABLE(gunfight_controller_table) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(KEYCODE_J) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("C1:1,2,3,4")
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Game_Time ) ) PORT_DIPLOCATION("C1:5,6")
	PORT_DIPSETTING(    0x00, "60 seconds" )
	PORT_DIPSETTING(    0x10, "70 seconds" )
	PORT_DIPSETTING(    0x20, "80 seconds" )
	PORT_DIPSETTING(    0x30, "90 seconds" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( gunfight )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gunfight_io_map,0)
	/* there is no watchdog */

	/* sound hardware */
	MDRV_IMPORT_FROM(gunfight_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Tornado Baseball (PCB #605)
 *
 *  Notes:
 *  -----
 *
 *  In baseball, the Visitor team always hits first and the Home team pitches (throws the ball).
 *  This rule gives an advantage to the Home team because they get to score last in any baseball game.
 *  It is also the team that pitches that controls the player on the field, which, in this game,
 *  is limited to moving the 3 outfielders left and right.
 *
 *  There are 3 types of cabinets using the same software:
 *
 *  Old Upright: One of everything
 *
 *  New Upright: One fielding/pitching controls, but two (Left/Right) hitting buttons
 *
 *  Cocktail:    Two of everything, but the pitching/fielding controls are swapped
 *
 *  Interestingly, the "Whistle" sound effect is controlled by a different
 *  bit on the Old Upright cabinet than the other two types.
 *
 *************************************/

#define TORNBASE_L_HIT_PORT_TAG			("LHIT")
#define TORNBASE_R_HIT_PORT_TAG			("RHIT")
#define TORNBASE_L_PITCH_PORT_TAG		("LPITCH")
#define TORNBASE_R_PITCH_PORT_TAG		("RPITCH")
#define TORNBASE_SCORE_SW_PORT_TAG		("SCORESW")
#define TORNBASE_SCORE_DIP_PORT_TAG		("ERASEDIP")
#define TORNBASE_CAB_TYPE_PORT_TAG		("CAB")


UINT8 tornbase_get_cabinet_type()
{
	return readinputportbytag(TORNBASE_CAB_TYPE_PORT_TAG);
}


static UINT32 tornbase_hit_left_input_r(void *param)
{
	return readinputportbytag(TORNBASE_L_HIT_PORT_TAG);
}


static UINT32 tornbase_hit_right_input_r(void *param)
{
	UINT32 ret;

	switch (tornbase_get_cabinet_type())
	{
	case TORNBASE_CAB_TYPE_UPRIGHT_OLD:
		ret = readinputportbytag(TORNBASE_L_HIT_PORT_TAG);
		break;

	case TORNBASE_CAB_TYPE_UPRIGHT_NEW:
	case TORNBASE_CAB_TYPE_COCKTAIL:
	default:
		ret = readinputportbytag(TORNBASE_R_HIT_PORT_TAG);
		break;
	}

	return ret;
}


static UINT32 tornbase_pitch_left_input_r(void *param)
{
	UINT32 ret;

	switch (tornbase_get_cabinet_type())
	{
	case TORNBASE_CAB_TYPE_UPRIGHT_OLD:
	case TORNBASE_CAB_TYPE_UPRIGHT_NEW:
		ret = readinputportbytag(TORNBASE_L_PITCH_PORT_TAG);
		break;

	case TORNBASE_CAB_TYPE_COCKTAIL:
	default:
		ret = readinputportbytag(TORNBASE_R_PITCH_PORT_TAG);
		break;
	}

	return ret;
}


static UINT32 tornbase_pitch_right_input_r(void *param)
{
	return readinputportbytag(TORNBASE_L_PITCH_PORT_TAG);
}


static UINT32 tornbase_score_input_r(void *param)
{
	return readinputportbytag(TORNBASE_SCORE_SW_PORT_TAG) &
		   readinputportbytag(TORNBASE_SCORE_DIP_PORT_TAG);
}


static WRITE8_HANDLER( tornbase_io_w )
{
	if (offset & 0x01)  tornbase_sh_port_w(0, data);

	if (offset & 0x02)  mw8080bw_shift_amount_w(0, data);

	if (offset & 0x04)  mw8080bw_shift_data_w(0, data);
}


static ADDRESS_MAP_START( tornbase_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	/* no decoder, just 3 AND gates */
	AM_RANGE(0x00, 0x07) AM_WRITE(tornbase_io_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( tornbase )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(tornbase_hit_left_input_r, 0)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(tornbase_pitch_left_input_r, 0)
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(tornbase_hit_right_input_r, 0)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(tornbase_pitch_right_input_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)  /* not connected */

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* schematics shows it as "START", but not used by the software */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(tornbase_score_input_r, 0)
	PORT_DIPNAME ( 0x78, 0x40, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B1:2,3,4,5")
	PORT_DIPSETTING (    0x18, "4 Coins/1 Inning" )
	PORT_DIPSETTING (    0x10, "3 Coins/1 Inning" )
	PORT_DIPSETTING (    0x38, "4 Coins/2 Innings" )
	PORT_DIPSETTING (    0x08, "2 Coins/1 Inning" )
	PORT_DIPSETTING (    0x30, "3 Coins/2 Innings" )
	PORT_DIPSETTING (    0x28, "2 Coins/2 Innings" )
	PORT_DIPSETTING (    0x00, "1 Coin/1 Inning" )
	PORT_DIPSETTING (    0x58, "4 Coins/4 Innings" )
	PORT_DIPSETTING (    0x50, "3 Coins/4 Innings" )
	PORT_DIPSETTING (    0x48, "2 Coins/4 Innings" )
	PORT_DIPSETTING (    0x20, "1 Coin/2 Innings" )
	PORT_DIPSETTING (    0x40, "1 Coin/4 Innings" )
	PORT_DIPSETTING (    0x78, "4 Coins/9 Innings" )
	PORT_DIPSETTING (    0x70, "3 Coins/9 Innings" )
	PORT_DIPSETTING (    0x68, "2 Coins/9 Innings" )
	PORT_DIPSETTING (    0x60, "1 Coin/9 Innings" )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "B1:6" )

	/* fake ports to handle the various input configurations based on cabinet type */
	PORT_START_TAG(TORNBASE_L_HIT_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Hit") PORT_PLAYER(1)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(TORNBASE_R_HIT_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Hit") PORT_PLAYER(2)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(TORNBASE_L_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Move Outfield Left") PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P1 Move Outfield Right") PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_NAME("P1 Pitch Left") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_NAME("P1 Pitch Right") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_NAME("P1 Pitch Slow") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_NAME("P1 Pitch Fast") PORT_PLAYER(1)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(TORNBASE_R_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 Move Outfield Left") PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P2 Move Outfield Right") PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_NAME("P2 Pitch Left") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_NAME("P2 Pitch Right") PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_NAME("P2 Pitch Slow") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_NAME("P2 Pitch Fast") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* 2 fakes port for the 'ERASE' input, which has a DIP to enable it.
       This switch is not actually used by the software */
	PORT_START_TAG(TORNBASE_SCORE_SW_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("SCORE Input (Not Used)") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(TORNBASE_SCORE_DIP_PORT_TAG)
	PORT_DIPNAME( 0x01, 0x01, "Enable SCORE Input" ) PORT_DIPLOCATION("B1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* fake port for cabinet type */
	PORT_START_TAG(TORNBASE_CAB_TYPE_PORT_TAG)
	PORT_CONFNAME( 0x03, TORNBASE_CAB_TYPE_UPRIGHT_NEW, DEF_STR( Cabinet ) )
	PORT_CONFSETTING( TORNBASE_CAB_TYPE_UPRIGHT_OLD, "Upright/w One Hit Button" )
	PORT_CONFSETTING( TORNBASE_CAB_TYPE_UPRIGHT_NEW, "Upright/w P1/P2 Hit Buttons" )
	PORT_CONFSETTING( TORNBASE_CAB_TYPE_COCKTAIL, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( tornbase)

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(tornbase_io_map,0)
	/* there is no watchdog */

	/* sound hardware */
	MDRV_IMPORT_FROM(tornbase_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  280 ZZZAP (PCB #610) / Laguna Racer (PCB #622)
 *
 *************************************/

static ADDRESS_MAP_START( zzzap_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x02, 0x02) AM_WRITE(zzzap_sh_port_1_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(zzzap_sh_port_2_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( zzzap )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0x0f) PORT_SENSITIVITY(100) PORT_KEYDELTA(64) PORT_PLAYER(1)   /* accelerator */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_TOGGLE PORT_NAME("P1 Shift") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )	/* not connected */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START_TAG("IN1")	/* steering wheel */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x0c,PORTCOND_NOTEQUALS,0x04) PORT_DIPLOCATION("E3:1,2")
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Game_Time ) ) PORT_DIPLOCATION("E3:3,4")
	PORT_DIPSETTING(    0x0c, "60 seconds + 30 extended" ) PORT_CONDITION("IN2",0x30,PORTCOND_NOTEQUALS,0x20)
	PORT_DIPSETTING(    0x00, "80 seconds + 40 extended" ) PORT_CONDITION("IN2",0x30,PORTCOND_NOTEQUALS,0x20)
	PORT_DIPSETTING(    0x08, "99 seconds + 50 extended" ) PORT_CONDITION("IN2",0x30,PORTCOND_NOTEQUALS,0x20)
	PORT_DIPSETTING(    0x0c, "60 seconds" ) PORT_CONDITION("IN2",0x30,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(    0x00, "80 seconds" ) PORT_CONDITION("IN2",0x30,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(    0x08, "99 seconds" ) PORT_CONDITION("IN2",0x30,PORTCOND_EQUALS,0x20)
	PORT_DIPSETTING(    0x04, "Test Mode" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time At" ) PORT_CONDITION("IN2",0x0c,PORTCOND_NOTEQUALS,0x04) PORT_DIPLOCATION("E3:5,6")
	PORT_DIPSETTING(    0x10, "2.00" )
	PORT_DIPSETTING(    0x00, "2.50" )
	PORT_DIPSETTING(    0x20, DEF_STR( None ) )
	/* PORT_DIPSETTING( 0x30, DEF_STR( None ) ) */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Language )) PORT_CONDITION("IN2",0x0c,PORTCOND_NOTEQUALS,0x04) PORT_DIPLOCATION("E3:7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( German ) )
	PORT_DIPSETTING(    0x80, DEF_STR( French ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Spanish ) )
INPUT_PORTS_END


static INPUT_PORTS_START( lagunar )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0x0f) PORT_SENSITIVITY(100) PORT_KEYDELTA(64) PORT_PLAYER(1)   /* accelerator */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_TOGGLE PORT_NAME("P1 Shift") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )	/* not connected */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )	/* start button, but never used */

	PORT_START_TAG("IN1")	/* steering wheel */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("E3:1,2")
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Game_Time ) ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0x04) PORT_DIPLOCATION("E3:3,4")
	PORT_DIPSETTING(    0x00, "45 seconds + 22 extended" ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0xc0)
	PORT_DIPSETTING(    0x04, "60 seconds + 30 extended" ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0xc0)
	PORT_DIPSETTING(    0x08, "75 seconds + 37 extended" ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0xc0)
	PORT_DIPSETTING(    0x0c, "90 seconds + 45 extended" ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0xc0)
	PORT_DIPSETTING(    0x00, "45 seconds" ) PORT_CONDITION("IN2",0xc0,PORTCOND_EQUALS,0xc0)
	PORT_DIPSETTING(    0x04, "60 seconds" ) PORT_CONDITION("IN2",0xc0,PORTCOND_EQUALS,0xc0)
	PORT_DIPSETTING(    0x08, "75 seconds" ) PORT_CONDITION("IN2",0xc0,PORTCOND_EQUALS,0xc0)
	PORT_DIPSETTING(    0x0c, "90 seconds" ) PORT_CONDITION("IN2",0xc0,PORTCOND_EQUALS,0xc0)
	PORT_DIPNAME( 0x30, 0x20, "Extended Time At" ) PORT_CONDITION("IN2",0xc0,PORTCOND_NOTEQUALS,0xc0) PORT_DIPLOCATION("E3:5,6")
	PORT_DIPSETTING(    0x00, "350" )
	PORT_DIPSETTING(    0x10, "400" )
	PORT_DIPSETTING(    0x20, "450" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0xc0, 0x00, "Test Modes/Extended Time") PORT_DIPLOCATION("E3:7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, "RAM/ROM Test" )
	PORT_DIPSETTING(    0x80, "Input Test" )
	PORT_DIPSETTING(    0xc0, "No Extended Time" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( zzzap )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(zzzap_io_map,0)
	MDRV_WATCHDOG_TIME_INIT( TIME_OF_555(RES_M(1), CAP_U(1)) ) /* 1.1s */

	/* sound hardware */
	/* MDRV_IMPORT_FROM(zzzap_sound) */

MACHINE_DRIVER_END



/*************************************
 *
 *  Amazing Maze (PCB #611)
 *
 *************************************/

static WRITE8_HANDLER( maze_coin_counter_w )
{
	/* the data is not used, just pulse the counter */
	coin_counter_w(0, 0);
	coin_counter_w(0, 1);
}


static WRITE8_HANDLER( maze_io_w )
{
	if (offset & 0x01)  maze_coin_counter_w(0, data);

	if (offset & 0x02)  watchdog_reset_w(0, data);
}


static ADDRESS_MAP_START( maze_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(2) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)

	/* no decoder, just a couple of AND gates */
	AM_RANGE(0x00, 0x03) AM_WRITE(maze_io_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( maze )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* labeled 'Not Used' */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_DIPNAME ( 0x30, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN1",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING (    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING (    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME ( 0x40, 0x40, "2 Player Game Time" ) PORT_CONDITION("IN1",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING (    0x40, "4 minutes" )
	PORT_DIPSETTING (    0x00, "6 minutes" )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "SW:4" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( maze )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(maze_io_map,0)
	MDRV_WATCHDOG_TIME_INIT( TIME_OF_555(RES_K(270), CAP_U(10)) ) /* 2.97s */

	/* sound hardware */
	/* MDRV_IMPORT_FROM(maze_sound) */

MACHINE_DRIVER_END



/*************************************
 *
 *  Boot Hill (PCB #612)
 *
 *************************************/

static ADDRESS_MAP_START( boothill_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_reversable_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_reversable_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(boothill_sh_port_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(midway_tone_generator_lo_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(midway_tone_generator_hi_w)
ADDRESS_MAP_END


/* the schematic shows the table different, but there is an table addendum confirming these values */
static const UINT32 boothill_controller_table[7] =
{
	0x00 , 0x04 , 0x06, 0x07, 0x03, 0x01, 0x05
};


static INPUT_PORTS_START( boothill )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x70, 0x03, IPT_POSITIONAL ) PORT_POSITIONS(7) PORT_REMAP_TABLE(boothill_controller_table) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_M) PORT_CODE_INC(KEYCODE_J) PORT_CENTERDELTA(0) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x70, 0x03, IPT_POSITIONAL ) PORT_POSITIONS(7) PORT_REMAP_TABLE(boothill_controller_table) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_N) PORT_CODE_INC(KEYCODE_H) PORT_CENTERDELTA(0) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x10,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING (    0x02, "2 Coins per Player" )
	PORT_DIPSETTING (    0x03, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING (    0x00, "1 Coin per Player" )
	PORT_DIPSETTING (    0x01, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME ( 0x0c, 0x04, DEF_STR( Game_Time ) ) PORT_CONDITION("IN2",0x10,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING (    0x00, "60 seconds" )
	PORT_DIPSETTING (    0x04, "70 seconds" )
	PORT_DIPSETTING (    0x08, "80 seconds" )
	PORT_DIPSETTING (    0x0c, "90 seconds" )
	PORT_SERVICE_DIPLOC (0x10, IP_ACTIVE_HIGH, "SW:5" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 35, "Music Volume" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( boothill )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(boothill_io_map,0)
	MDRV_WATCHDOG_TIME_INIT( TIME_OF_555(RES_K(270), CAP_U(10)) ) /* 2.97s */

	/* sound hardware */
	MDRV_IMPORT_FROM(boothill_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Checkmate (PCB #615)
 *
 *************************************/

static WRITE8_HANDLER( checkmat_io_w )
{
	if (offset & 0x01)  checkmat_sh_port_w(0, data);

	if (offset & 0x02)  watchdog_reset_w(0, data);
}


static ADDRESS_MAP_START( checkmat_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(2) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)

	/* no decoder, just a couple of AND gates */
	AM_RANGE(0x00, 0x03) AM_WRITE(checkmat_io_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( checkmat )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x01, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("A4:1")
	PORT_DIPSETTING (    0x00, "1 Coin/1 or 2 Players" )
	PORT_DIPSETTING (    0x01, "1 Coin/1 or 2 Players, 2 Coins/3 or 4 Players" )
	PORT_DIPNAME ( 0x02, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("A4:2")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x02, DEF_STR( On ) )
	PORT_DIPNAME ( 0x0c, 0x04, "Rounds" ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("A4:3,4")
	PORT_DIPSETTING (    0x00, "2" )
	PORT_DIPSETTING (    0x04, "3" )
	PORT_DIPSETTING (    0x08, "4" )
	PORT_DIPSETTING (    0x0c, "5" )
	PORT_DIPNAME ( 0x10, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("A4:5")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x10, DEF_STR( On ) )
	PORT_DIPNAME ( 0x60, 0x00, DEF_STR( Language ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("A4:6,7")
	PORT_DIPSETTING (    0x00, DEF_STR( English ) )
	PORT_DIPSETTING (    0x20, "Language 2" )
	PORT_DIPSETTING (    0x40, "Language 3" )
	PORT_DIPSETTING (    0x60, "Language 4" )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "A4:8" )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( checkmat )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(checkmat_io_map,0)
	MDRV_WATCHDOG_TIME_INIT( TIME_OF_555(RES_K(270), CAP_U(10)) ) /* 2.97s */

	/* sound hardware */
	/* MDRV_IMPORT_FROM(checkmat_sound) */

MACHINE_DRIVER_END



/*************************************
 *
 *  Desert Gun (PCB #618)
 *
 *************************************/

#define DESERTGU_DIP_SW_0_1_SET_1_TAG	("DIPSW01SET1")
#define DESERTGU_DIP_SW_0_1_SET_2_TAG	("DIPSW01SET2")


static UINT8 desertgu_controller_select;


static MACHINE_START( desertgu )
{
	/* setup for save states */
	state_save_register_global(desertgu_controller_select);

	return machine_start_mw8080bw(machine);
}


void desertgun_set_controller_select(UINT8 data)
{
	desertgu_controller_select = data;
}


static UINT32 desertgu_gun_input_r(void *param)
{
	UINT32 ret;

	if (desertgu_controller_select)
	{
		ret = readinputportbytag(DESERTGU_GUN_X_PORT_TAG);
	}
	else
	{
		ret = readinputportbytag(DESERTGU_GUN_Y_PORT_TAG);
	}

	return ret;
}


static UINT32 desertgu_dip_sw_0_1_r(void *param)
{
	UINT32 ret;

	if (desertgu_controller_select)
	{
		ret = readinputportbytag(DESERTGU_DIP_SW_0_1_SET_2_TAG);
	}
	else
	{
		ret = readinputportbytag(DESERTGU_DIP_SW_0_1_SET_1_TAG);
	}

	return ret;
}


static ADDRESS_MAP_START( desertgu_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(desertgu_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(midway_tone_generator_lo_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(midway_tone_generator_hi_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(desertgu_sh_port_2_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( desertgu )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(desertgu_gun_input_r, 0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(desertgu_dip_sw_0_1_r, 0)
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Language ) ) PORT_CONDITION("IN1",0x30,PORTCOND_NOTEQUALS,0x30) PORT_DIPLOCATION("C2:5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x04, DEF_STR( German ) )
	PORT_DIPSETTING(    0x08, DEF_STR( French ) )
	PORT_DIPSETTING(    0x0c, "Danish" )
	PORT_DIPNAME( 0x30, 0x10, "Extended Time At" ) PORT_DIPLOCATION("C2:7,8")
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "9000" )
	PORT_DIPSETTING(    0x30, "Test Mode" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* fake ports for reading the gun's X and Y axis */
	PORT_START_TAG(DESERTGU_GUN_X_PORT_TAG)
	PORT_BIT( 0xff, 0x4d, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX(0x10,0x8e) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START_TAG(DESERTGU_GUN_Y_PORT_TAG)
	PORT_BIT( 0xff, 0x48, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX(0x10,0x7f) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	/* bits 0 and 1 in the DIP SW input port can reflect two sets of switches depending on the controller
       select bit.  These two ports are fakes to handle this case */
	PORT_START_TAG(DESERTGU_DIP_SW_0_1_SET_1_TAG)
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN1",0x30,PORTCOND_NOTEQUALS,0x30) PORT_DIPLOCATION("C2:1,2")
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(DESERTGU_DIP_SW_0_1_SET_2_TAG)
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Game_Time ) ) PORT_CONDITION("IN1",0x30,PORTCOND_NOTEQUALS,0x30) PORT_DIPLOCATION("C2:3,4")
	PORT_DIPSETTING(    0x00, "40 seconds + 30 extended" )
	PORT_DIPSETTING(    0x01, "50 seconds + 30 extended" )
	PORT_DIPSETTING(    0x02, "60 seconds + 30 extended" )
	PORT_DIPSETTING(    0x03, "70 seconds + 30 extended" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 60, "Music Volume" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( desertgu )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(desertgu_io_map,0)
	MDRV_MACHINE_START(desertgu)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(desertgu_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Double Play (PCB #619) / Extra Inning (PCB #642)
 *
 *  This game comes in an upright and a cocktail cabinet.
 *  The upright one had a shared joystick and a hitting button for
 *  each player, while in the cocktail version each player
 *  had their own set of controls.  The display is never flipped,
 *  as the two players sit diagonally across from each other.
 *
 *************************************/

#define DPLAY_L_PITCH_PORT_TAG		("LPITCH")
#define DPLAY_R_PITCH_PORT_TAG		("RPITCH")
#define DPLAY_CAB_TYPE_PORT_TAG		("CAB")
#define DPLAY_CAB_TYPE_UPRIGHT		(0)
#define DPLAY_CAB_TYPE_COCKTAIL		(1)


static UINT32 dplay_pitch_left_input_r(void *param)
{
	UINT32 ret;

	if (readinputportbytag(DPLAY_CAB_TYPE_PORT_TAG) == DPLAY_CAB_TYPE_UPRIGHT)
	{
		ret = readinputportbytag(DPLAY_L_PITCH_PORT_TAG);
	}
	else
	{
		ret = readinputportbytag(DPLAY_R_PITCH_PORT_TAG);
	}

	return ret;
}


static UINT32 dplay_pitch_right_input_r(void *param)
{
	return readinputportbytag(DPLAY_L_PITCH_PORT_TAG);
}


static ADDRESS_MAP_START( dplay_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(dplay_sh_port_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(midway_tone_generator_lo_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(midway_tone_generator_hi_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( dplay )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Hit") PORT_PLAYER(1)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dplay_pitch_left_input_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Hit") PORT_PLAYER(2)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dplay_pitch_right_input_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x07, 0x00, DEF_STR( Coinage )) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:1,2,3")
	PORT_DIPSETTING (    0x05, "2 Coins/1 Inning/1 Player, 4 Coins/1 Inning/2 Players, 8 Coins/3 Innings/2 Players" )
	PORT_DIPSETTING (    0x04, "1 Coin/1 Inning/1 Player, 2 Coins/1 Inning/2 Players, 4 Coins/3 Innings/2 Players" )
	PORT_DIPSETTING (    0x02, "2 Coins per Inning" )
	PORT_DIPSETTING (    0x03, "2 Coins/1 Inning, 4 Coins/3 Innings" )
	PORT_DIPSETTING (    0x00, "1 Coin per Inning" )
	/* PORT_DIPSETTING ( 0x06, "1 Coin per Inning" ) */
	/* PORT_DIPSETTING ( 0x07, "1 Coin per Inning" ) */
	PORT_DIPSETTING (    0x01, "1 Coin/1 Inning, 2 Coins/3 Innings" )
	PORT_DIPNAME ( 0x08, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:4")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x08, DEF_STR( On ) )
	PORT_DIPNAME ( 0x10, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:5")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x10, DEF_STR( On ) )
	PORT_DIPNAME ( 0x20, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:6")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x20, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x40, IP_ACTIVE_LOW, "C1:7" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	/* fake ports to handle the various input configurations based on cabinet type */
	PORT_START_TAG(DPLAY_L_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Move Outfield Left") PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Move Outfield Right") PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("P1 Pitch Left") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("P1 Pitch Right") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("P1 Pitch Slow") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("P1 Pitch Fast") PORT_PLAYER(1)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG(DPLAY_R_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Move Outfield Left") PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P2 Move Outfield Right") PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("P2 Pitch Left") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("P2 Pitch Right") PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("P2 Pitch Slow") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("P2 Pitch Fast") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	/* fake port for cabinet type */
	PORT_START_TAG(DPLAY_CAB_TYPE_PORT_TAG)
	PORT_CONFNAME( 0x01, DPLAY_CAB_TYPE_UPRIGHT, DEF_STR( Cabinet ) )
	PORT_CONFSETTING( DPLAY_CAB_TYPE_UPRIGHT, DEF_STR( Upright ) )
	PORT_CONFSETTING( DPLAY_CAB_TYPE_COCKTAIL, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 60, "Music Volume" )
INPUT_PORTS_END


static INPUT_PORTS_START( einning )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Hit") PORT_PLAYER(1)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dplay_pitch_left_input_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Hit") PORT_PLAYER(2)
	PORT_BIT( 0x7e, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dplay_pitch_right_input_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x07, 0x00, DEF_STR( Coinage )) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:1,2,3")
	PORT_DIPSETTING (    0x05, "2 Coins/1 Inning/1 Player, 4 Coins/1 Inning/2 Players, 8 Coins/3 Innings/2 Players" )
	PORT_DIPSETTING (    0x04, "1 Coin/1 Inning/1 Player, 2 Coins/1 Inning/2 Players, 4 Coins/3 Innings/2 Players" )
	PORT_DIPSETTING (    0x02, "2 Coins per Inning" )
	PORT_DIPSETTING (    0x03, "2 Coins/1 Inning, 4 Coins/3 Innings" )
	PORT_DIPSETTING (    0x00, "1 Coin per Inning" )
	/* PORT_DIPSETTING ( 0x06, "1 Coin per Inning" ) */
	/* PORT_DIPSETTING ( 0x07, "1 Coin per Inning" ) */
	PORT_DIPSETTING (    0x01, "1 Coin/1 Inning, 2 Coins/3 Innings" )
	PORT_DIPNAME ( 0x08, 0x00, "Wall Knock Out Behavior" ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:4")
	PORT_DIPSETTING (    0x00, "Individually" )
	PORT_DIPSETTING (    0x08, "In Pairs" )
	PORT_DIPNAME ( 0x10, 0x00, "Double Score when Special Lit" ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:5")
	PORT_DIPSETTING (    0x00, "Home Run Only" )
	PORT_DIPSETTING (    0x10, "Any Hit" )
	PORT_DIPNAME ( 0x20, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x40,PORTCOND_EQUALS,0x40) PORT_DIPLOCATION("C1:6")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x20, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x40, IP_ACTIVE_LOW, "C1:7" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	/* fake ports to handle the various input configurations based on cabinet type */
	PORT_START_TAG(DPLAY_L_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Move Outfield Left") PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Move Outfield Right") PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("P1 Pitch Left") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("P1 Pitch Right") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("P1 Pitch Slow") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("P1 Pitch Fast") PORT_PLAYER(1)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG(DPLAY_R_PITCH_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Move Outfield Left") PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P2 Move Outfield Right") PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("P2 Pitch Left") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("P2 Pitch Right") PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("P2 Pitch Slow") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("P2 Pitch Fast") PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	/* fake port for cabinet type */
	PORT_START_TAG(DPLAY_CAB_TYPE_PORT_TAG)
	PORT_CONFNAME( 0x01, DPLAY_CAB_TYPE_UPRIGHT, DEF_STR( Cabinet ) )
	PORT_CONFSETTING( DPLAY_CAB_TYPE_UPRIGHT, DEF_STR( Upright ) )
	PORT_CONFSETTING( DPLAY_CAB_TYPE_COCKTAIL, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 60, "Music Volume" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( dplay )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(dplay_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(dplay_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Guided Missile (PCB #623)
 *
 *************************************/

static ADDRESS_MAP_START( gmissile_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_reversable_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_reversable_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(gmissile_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(gmissile_sh_port_2_w)
	/* also writes 0x00 to 0x06, but it is not connected */
	AM_RANGE(0x07, 0x07) AM_WRITE(gmissile_sh_port_3_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( gmissile )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x03, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("D1:1,2")
	PORT_DIPSETTING (    0x01, "2 Coins per Player" )
	PORT_DIPSETTING (    0x00, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING (    0x03, "1 Coin per Player" )
	PORT_DIPSETTING (    0x02, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME ( 0x0c, 0x08, DEF_STR( Game_Time ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("D1:3,4")
	PORT_DIPSETTING (    0x00, "60 seconds + 30 extended" )
	PORT_DIPSETTING (    0x08, "70 seconds + 35 extended" )
	PORT_DIPSETTING (    0x04, "80 seconds + 40 extended" )
	PORT_DIPSETTING (    0x0c, "90 seconds + 45 extended" )
	PORT_DIPNAME ( 0x30, 0x10, "Extended Time At" ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("D1:5,6")
	PORT_DIPSETTING (    0x00, "500" )
	PORT_DIPSETTING (    0x20, "700" )
	PORT_DIPSETTING (    0x10, "1000" )
	PORT_DIPSETTING (    0x30, "1300" )
	PORT_DIPNAME ( 0x40, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("D1:7")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x40, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_LOW, "D1:8" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( gmissile )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(gmissile_io_map,0)
	MDRV_WATCHDOG_VBLANK_INIT(255) /* really based on a 60Hz clock source */

	/* sound hardware */
	MDRV_IMPORT_FROM(gmissile_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  M-4 (PCB #626)
 *
 *************************************/

static ADDRESS_MAP_START( m4_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_reversable_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_reversable_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(m4_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(m4_sh_port_2_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( m4 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P2 Trigger") PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P2 Reload") PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_2WAY PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_2WAY PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Trigger") PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Reload") PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x10,PORTCOND_EQUALS,0x10) PORT_DIPLOCATION("C1:1,2")
	PORT_DIPSETTING (    0x02, "2 Coins per Player" )
	PORT_DIPSETTING (    0x03, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING (    0x00, "1 Coin per Player" )
	PORT_DIPSETTING (    0x01, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME ( 0x0c, 0x04, DEF_STR( Game_Time ) ) PORT_CONDITION("IN2",0x10,PORTCOND_EQUALS,0x10) PORT_DIPLOCATION("C1:3,4")
	PORT_DIPSETTING (    0x00, "60 seconds" )
	PORT_DIPSETTING (    0x04, "70 seconds" )
	PORT_DIPSETTING (    0x08, "80 seconds" )
	PORT_DIPSETTING (    0x0c, "90 seconds" )
	PORT_SERVICE_DIPLOC( 0x10, IP_ACTIVE_LOW, "C1:5" )
	PORT_DIPNAME ( 0x20, 0x00, "Extended Play" ) PORT_DIPLOCATION("C1:6")
	PORT_DIPSETTING (    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x00, DEF_STR( On ) )
	PORT_DIPNAME ( 0xC0, 0x00, "Extended Play At" ) PORT_DIPLOCATION("C1:8,7")
	PORT_DIPSETTING (    0x00, "110" )
	PORT_DIPSETTING (    0x80, "100" )
	PORT_DIPSETTING (    0x40, "80" )
	PORT_DIPSETTING (    0xc0, "70" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( m4 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(m4_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(m4_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Clowns (PCB #630)
 *
 *************************************/

#define CLOWNS_CONTROLLER_P1_TAG		("CONTP1")
#define CLOWNS_CONTROLLER_P2_TAG		("CONTP2")


static UINT8 clowns_controller_select;


static MACHINE_START( clowns )
{
	/* setup for save states */
	state_save_register_global(clowns_controller_select);

	return machine_start_mw8080bw(machine);
}


void clowns_set_controller_select(UINT8 data)
{
	clowns_controller_select = data;
}


static UINT32 clowns_controller_r(void *param)
{
	UINT32 ret;

	if (clowns_controller_select)
	{
		ret = readinputportbytag(CLOWNS_CONTROLLER_P2_TAG);
	}
	else
	{
		ret = readinputportbytag(CLOWNS_CONTROLLER_P1_TAG);
	}

	return ret;
}


static ADDRESS_MAP_START( clowns_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(clowns_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(midway_tone_generator_lo_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(midway_tone_generator_hi_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(clowns_sh_port_2_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( clowns )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(clowns_controller_r, 0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING (    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (    0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING (    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME ( 0x0c, 0x00, "Bonus Game" ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING (    0x00, "No Bonus" )
	PORT_DIPSETTING (    0x04, "9000" )
	PORT_DIPSETTING (    0x08, "11000" )
	PORT_DIPSETTING (    0x0c, "13000" )
	PORT_DIPNAME ( 0x10, 0x00, "Balloon Resets" ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING (    0x00, "Each Row" )
	PORT_DIPSETTING (    0x10, "All Rows" )
	PORT_DIPNAME ( 0x20, 0x00, DEF_STR( Bonus_Life ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:6")
	PORT_DIPSETTING (    0x00, "3000" )
	PORT_DIPSETTING (    0x20, "4000" )
	PORT_DIPNAME ( 0x40, 0x00, DEF_STR( Lives ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING (    0x00, "3" )
	PORT_DIPSETTING (    0x40, "4" )
	/* test mode - press coin button for input test */
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "SW:8" )

	/* fake ports for two analog controls multiplexed */
	PORT_START_TAG(CLOWNS_CONTROLLER_P1_TAG)
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(1)

	PORT_START_TAG(CLOWNS_CONTROLLER_P2_TAG)
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 60, "Music Volume" )
INPUT_PORTS_END


static INPUT_PORTS_START( clowns1 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(clowns_controller_r, 0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING (    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (    0x02, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING (    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME ( 0x0c, 0x04, DEF_STR( Lives ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING (    0x00, "2" )
	PORT_DIPSETTING (    0x04, "3" )
	PORT_DIPSETTING (    0x08, "4" )
	PORT_DIPSETTING (    0x0c, "5" )
	PORT_DIPNAME ( 0x10, 0x00, "Balloon Resets" ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING (    0x00, "Each Row" )
	PORT_DIPSETTING (    0x10, "All Rows" )
	PORT_DIPNAME ( 0x20, 0x00, DEF_STR( Bonus_Life ) ) PORT_CONDITION("IN2",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("SW:6")
	PORT_DIPSETTING (    0x00, "3000" )
	PORT_DIPSETTING (    0x20, "4000" )
	PORT_DIPNAME ( 0x40, 0x00, "Input Test"  ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x40, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "SW:8" )

	PORT_START_TAG(CLOWNS_CONTROLLER_P1_TAG)
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(1)

	PORT_START_TAG(CLOWNS_CONTROLLER_P2_TAG)
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x01,0xfe) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG("MUSIC_ADJ")  /* 3 */
	PORT_ADJUSTER( 60, "Music Volume" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( clowns )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(clowns_io_map,0)
	MDRV_MACHINE_START(clowns)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(clowns_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Shuffleboard (PCB #643)
 *
 *************************************/

static ADDRESS_MAP_START( shuffle_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(4) )	/* yes, 4, and no mirroring on the read handlers */
	AM_RANGE(0x01, 0x01) AM_READ(mw8080bw_shift_res_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)
	AM_RANGE(0x03, 0x03) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x05, 0x05) AM_READ(input_port_2_r)
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)

	AM_RANGE(0x01, 0x01) AM_MIRROR(0x08) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x08) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0x08) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0x08) AM_WRITE(shuffle_sh_port_1_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x08) AM_WRITE(shuffle_sh_port_2_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( shuffle )
	PORT_START_TAG("IN0")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Language ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("B3:1,2")
	PORT_DIPSETTING (    0x00, DEF_STR( English ) )
	PORT_DIPSETTING (    0x01, DEF_STR( French ) )
	PORT_DIPSETTING (    0x02, DEF_STR( German ) )
	/* PORT_DIPSETTING ( 0x03, DEF_STR( German ) ) */
	PORT_DIPNAME ( 0x0c, 0x04, "Points to Win" ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("B3:3,4")
	PORT_DIPSETTING (    0x00, "Game 1 = 25, Game 2 = 11" )
	PORT_DIPSETTING (    0x04, "Game 1 = 35, Game 2 = 15" )
	PORT_DIPSETTING (    0x08, "Game 1 = 40, Game 2 = 18" )
	PORT_DIPSETTING (    0x0c, "Game 1 = 50, Game 2 = 21" )
	PORT_DIPNAME ( 0x30, 0x10, DEF_STR( Coinage ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("B3:5,6")
	PORT_DIPSETTING (    0x30, "2 Coins per Player" )
	PORT_DIPSETTING (    0x20, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING (    0x10, "1 Coin per Player" )
	PORT_DIPSETTING (    0x00, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME ( 0x40, 0x40, "Time Limit" ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("B3:7")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x40, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_LOW, "B3:8" )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Game Select")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(50) PORT_PLAYER(1)

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)
INPUT_PORTS_END


static MACHINE_DRIVER_START( shuffle )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(shuffle_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	/* MDRV_IMPORT_FROM(shuffle_sound) */

MACHINE_DRIVER_END



/*************************************
 *
 *  Dog Patch (PCB #644)
 *
 *************************************/

#define DOGPATCH_GUN_P1_PORT_TAG	("GUNP1")
#define DOGPATCH_GUN_P2_PORT_TAG	("GUNP2")


static UINT32 dogpatch_gun_input_r(void *param)
{
	static const UINT32 remapped[] =
	{
		7, 6, 4, 5, 1, 0, 2, 2
	};

	const char *tag = (const char *)param;

	UINT32 normal_binary = (readinputportbytag(tag) >> 5) & 0x07;

	return remapped[normal_binary];
}


static ADDRESS_MAP_START( dogpatch_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(dogpatch_sh_port_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(midway_tone_generator_lo_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(midway_tone_generator_hi_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( dogpatch )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dogpatch_gun_input_r, DOGPATCH_GUN_P2_PORT_TAG)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dogpatch_gun_input_r, DOGPATCH_GUN_P1_PORT_TAG)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_DIPNAME ( 0x03, 0x02, "Number of Cans" ) PORT_CONDITION("IN2",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:1,2")
	PORT_DIPSETTING (    0x03, "10" )
	PORT_DIPSETTING (    0x02, "15" )
	PORT_DIPSETTING (    0x01, "20" )
	PORT_DIPSETTING (    0x00, "25" )
	PORT_DIPNAME ( 0x0c, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING (    0x08, "2 Coins per Player" )
	PORT_DIPSETTING (    0x0c, "2 Coins/1 or 2 Players" )
	PORT_DIPSETTING (    0x00, "1 Coin per Player" )
	PORT_DIPSETTING (    0x04, "1 Coin/1 or 2 Players" )
	PORT_DIPNAME ( 0x10, 0x10, "Extended Time Reward" ) PORT_CONDITION("IN2",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING (    0x10, "3 extra cans" )
	PORT_DIPSETTING (    0x00, "5 extra cans" )
	PORT_SERVICE_DIPLOC( 0x20, IP_ACTIVE_LOW, "SW:6" )
	PORT_DIPNAME ( 0xc0, 0x40, "Extended Time At" ) PORT_CONDITION("IN2",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:7,8")
	PORT_DIPSETTING (    0xc0, "150" )
	PORT_DIPSETTING (    0x80, "175" )
	PORT_DIPSETTING (    0x40, "225" )
	PORT_DIPSETTING (    0x00, "275" )

	/* fake ports for aiming the guns */
	PORT_START_TAG(DOGPATCH_GUN_P2_PORT_TAG)
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0x01,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(2)

	PORT_START_TAG(DOGPATCH_GUN_P1_PORT_TAG)
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0x01,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_PLAYER(1)
INPUT_PORTS_END


static MACHINE_DRIVER_START( dogpatch )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(dogpatch_io_map,0)
	/* the watch dog time is unknown, but all other */
	/* Midway boards of the era used the same circuit */
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(dogpatch_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Space Encounters (PCB #645)
 *
 *************************************/

#define SPCENCTR_STROBE_FREQ		(9.00)  /* Hz */
#define SPCENCTR_STROBE_PERIOD		(1.0 / SPCENCTR_STROBE_FREQ)  /* sec */
#define SPCENCTR_DUTY_CYCLE			(0.95)


static mame_timer *spcenctr_strobe_on_timer;
static mame_timer *spcenctr_strobe_off_timer;
static UINT8 spcenctr_strobe_state;
static UINT8 spcenctr_trench_width;
static UINT8 spcenctr_trench_center;
static UINT8 spcenctr_trench_slope[16];  /* 16x4 bit RAM */


static void adjust_strobe_timers(void)
{
	/* the strobe light is controlled by a 555 timer, which appears to have a
       frequency of 9Hz and a duty cycle of 95% */
	if (spcenctr_strobe_state)
	{
		mame_timer_adjust(spcenctr_strobe_on_timer, time_zero, 1, double_to_mame_time(TIME_IN_SEC(SPCENCTR_STROBE_PERIOD)));
		mame_timer_adjust(spcenctr_strobe_off_timer, double_to_mame_time(TIME_IN_SEC(SPCENCTR_STROBE_PERIOD) * SPCENCTR_DUTY_CYCLE), 0, double_to_mame_time(TIME_IN_SEC(SPCENCTR_STROBE_PERIOD)));
	}
	else
	{
		mame_timer_adjust(spcenctr_strobe_on_timer, time_never, 0, time_zero);
		mame_timer_adjust(spcenctr_strobe_off_timer, time_never, 0, time_zero);
	}
}


static void spcenctr_strobe_timer_callback(int param)
{
	output_set_value("STROBE", param);
}


static MACHINE_START( spcenctr )
{
	/* create timers */
	spcenctr_strobe_on_timer = timer_alloc(spcenctr_strobe_timer_callback);
	spcenctr_strobe_off_timer = timer_alloc(spcenctr_strobe_timer_callback);

	/* setup for save states */
	state_save_register_global(spcenctr_strobe_state);
	state_save_register_global(spcenctr_trench_width);
	state_save_register_global(spcenctr_trench_center);
	state_save_register_global_array(spcenctr_trench_slope);
	state_save_register_func_postload(adjust_strobe_timers);

	return machine_start_mw8080bw(machine);
}


void spcenctr_set_strobe_state(UINT8 data)
{
	if (data != spcenctr_strobe_state)
	{
		spcenctr_strobe_state = data;

		adjust_strobe_timers();
	}
}


UINT8 spcenctr_get_trench_width(void)
{
	return spcenctr_trench_width;
}


UINT8 spcenctr_get_trench_center(void)
{
	return spcenctr_trench_center;
}


UINT8 spcenctr_get_trench_slope(UINT8 addr)
{
	return spcenctr_trench_slope[addr & 0x0f];
}


static WRITE8_HANDLER( spcenctr_io_w )
{										/* A7 A6 A5 A4 A3 A2 A1 A0 */
	if ((offset & 0x07) == 0x02)
	{
		watchdog_reset_w(0, data);		/* -  -  -  -  -  0  1  0 */
	}
	else if ((offset & 0x5f) == 0x01)
	{
		spcenctr_sh_port_1_w(0, data);	/* -  0  -  0  0  0  0  1 */
	}
	else if ((offset & 0x5f) == 0x09)
	{
		spcenctr_sh_port_2_w(0, data);	/* -  0  -  0  1  0  0  1 */
	}
	else if ((offset & 0x5f) == 0x11)
	{
		spcenctr_sh_port_3_w(0, data);	/* -  0  -  1  0  0  0  1 */
	}
	else if ((offset & 0x07) == 0x03)
	{									/* -  -  -  -  -  0  1  1 */
		UINT8 addr = ((offset & 0xc0) >> 4) | ((offset & 0x18) >> 3);
		spcenctr_trench_slope[addr] = data;
	}
	else if ((offset & 0x07) == 0x04)
	{
		spcenctr_trench_center = data;	/* -  -  -  -  -  1  0  0 */
	}
	else if ((offset & 0x07) == 0x07)
	{
		spcenctr_trench_width = data;	/* -  -  -  -  -  1  1  1 */
	}
	else
	{
		logerror("%04x:  Unmapped I/O port write to %02x = %02x\n",
				 activecpu_get_pc(), offset, data);
	}
}


static ADDRESS_MAP_START( spcenctr_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xfc) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0xfc) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xfc) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xfc) AM_READNOP

	/* complicated addressing logic */
	AM_RANGE(0x00, 0xff) AM_WRITE(spcenctr_io_w)
ADDRESS_MAP_END


static const UINT32 gray_code_6_bit[] =
{
	0x3f, 0x3e, 0x3c, 0x3d, 0x39, 0x38, 0x3a, 0x3b,
	0x33, 0x32, 0x30, 0x31, 0x35, 0x34, 0x36, 0x37,
	0x27, 0x26, 0x24, 0x25, 0x21, 0x20, 0x22, 0x23,
	0x2b, 0x2a, 0x28, 0x29, 0x2d, 0x2c, 0x2e, 0x2f,
	0x0f, 0x0e, 0x0c, 0x0d, 0x09, 0x08, 0x0a, 0x0b,
	0x03, 0x02, 0x00, 0x01, 0x05, 0x04, 0x06, 0x07,
	0x17, 0x16, 0x14, 0x15, 0x11, 0x10, 0x12, 0x13,
	0x1b, 0x1a, 0x18, 0x19, 0x1d, 0x1c, 0x1e, 0x1f
};


static INPUT_PORTS_START( spcenctr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x3f, 0x20, IPT_POSITIONAL ) PORT_POSITIONS(64) PORT_REMAP_TABLE(gray_code_6_bit) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE /* 6 bit horiz encoder */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x3f, 0x20, IPT_POSITIONAL_V ) PORT_POSITIONS(64) PORT_REMAP_TABLE(gray_code_6_bit) PORT_SENSITIVITY(5) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) /* 6 bit vert encoder */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* marked as COIN #2, but the software never reads it */

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Bonus_Life ) ) PORT_CONDITION("IN2",0x30,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("F3:1,2")
	PORT_DIPSETTING(    0x00, "2000 4000 8000" )
	PORT_DIPSETTING(    0x01, "3000 6000 12000" )
	PORT_DIPSETTING(    0x02, "4000 8000 16000" )
	PORT_DIPSETTING(    0x03, "5000 10000 20000" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN2",0x30,PORTCOND_NOTEQUALS,0x10) PORT_DIPLOCATION("F3:3,4")
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x30, 0x00, "Bonus/Test Mode" ) PORT_DIPLOCATION("F3:5,6")
	PORT_DIPSETTING(    0x00, "Bonus On" )
	PORT_DIPSETTING(    0x30, "Bonus Off" )
	PORT_DIPSETTING(    0x20, "Cross Hatch" )
	PORT_DIPSETTING(    0x10, "Test Mode" )
	PORT_DIPNAME( 0xc0, 0x40, "Time" ) PORT_CONDITION("IN2",0x30,PORTCOND_NOTEQUALS,0x10) PORT_DIPLOCATION("F3:7,8")
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPSETTING(    0x40, "60" )
	PORT_DIPSETTING(    0x80, "75" )
	PORT_DIPSETTING(    0xc0, "90" )
INPUT_PORTS_END


static MACHINE_DRIVER_START( spcenctr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(spcenctr_io_map,0)
	MDRV_MACHINE_START(spcenctr)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* video hardware */
	MDRV_VIDEO_UPDATE(spcenctr)

	/* sound hardware */
	MDRV_IMPORT_FROM(spcenctr_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Phantom II (PCB #652)
 *
 *************************************/

static UINT16 phantom2_cloud_counter = 0;


static MACHINE_START( phantom2 )
{
	/* setup for save states */
	state_save_register_global(phantom2_cloud_counter);

	return machine_start_mw8080bw(machine);
}


UINT16 phantom2_get_cloud_counter(void)
{
	return phantom2_cloud_counter;
}


void phantom2_set_cloud_counter(UINT16 data)
{
	phantom2_cloud_counter = data;
}


static ADDRESS_MAP_START( phantom2_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(phantom2_sh_port_1_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(phantom2_sh_port_2_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( phantom2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )  /* not connected */

	PORT_START_TAG("IN1")
	PORT_DIPNAME ( 0x01, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING (    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME ( 0x06, 0x06, DEF_STR( Game_Time ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:2,3")
	PORT_DIPSETTING (    0x00, "45 seconds + 20 extended (at 20 points)" )
	PORT_DIPSETTING (    0x02, "60 seconds + 25 extended (at 25 points)" )
	PORT_DIPSETTING (    0x04, "75 seconds + 30 extended (at 30 points)" )
	PORT_DIPSETTING (    0x06, "90 seconds + 35 extended (at 35 points)" )
	PORT_DIPNAME ( 0x08, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x08, DEF_STR( On ) )
	PORT_DIPNAME ( 0x10, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x10, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x20, IP_ACTIVE_LOW, "SW:6" )
	PORT_DIPNAME ( 0x40, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x40, DEF_STR( On ) )
	PORT_DIPNAME ( 0x80, 0x00, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x20,PORTCOND_EQUALS,0x20) PORT_DIPLOCATION("SW:8")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( phantom2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(phantom2_io_map,0)
	MDRV_MACHINE_START(phantom2)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* video hardware */
	MDRV_VIDEO_UPDATE(phantom2)
	MDRV_VIDEO_EOF(phantom2)

	/* sound hardware */
	MDRV_IMPORT_FROM(phantom2_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Bowling Alley (PCB #730)
 *
 *************************************/

static READ8_HANDLER( bowler_shift_res_r )
{
	/* ZV - not too sure why this is needed, I don't see
       anything unusual on the schematics that would cause
       the bits to flip */

	return ~do_shift();
}


static WRITE8_HANDLER( bowler_lights_1_w )
{
	output_set_value("200_LEFT_LIGHT",  (data >> 0) & 0x01);

	output_set_value("400_LEFT_LIGHT",  (data >> 1) & 0x01);

	output_set_value("500_LEFT_LIGHT",  (data >> 2) & 0x01);

	output_set_value("700_LIGHT",       (data >> 3) & 0x01);

	output_set_value("500_RIGHT_LIGHT", (data >> 4) & 0x01);

	output_set_value("400_RIGHT_LIGHT", (data >> 5) & 0x01);

	output_set_value("200_RIGHT_LIGHT", (data >> 6) & 0x01);

	output_set_value("X_LEFT_LIGHT",    (data >> 7) & 0x01);
	output_set_value("X_RIGHT_LIGHT",   (data >> 7) & 0x01);
}


static WRITE8_HANDLER( bowler_lights_2_w )
{
	output_set_value("REGULATION_GAME_LIGHT", ( data >> 0) & 0x01);
	output_set_value("FLASH_GAME_LIGHT",      (~data >> 0) & 0x01);

	output_set_value("STRAIGHT_BALL_LIGHT",   ( data >> 1) & 0x01);

	output_set_value("HOOK_BALL_LIGHT",       ( data >> 2) & 0x01);

	output_set_value("SELECT_GAME_LIGHT",     ( data >> 3) & 0x01);

	/* bits 4-7 are not connected */
}


static ADDRESS_MAP_START( bowler_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(4) )  /* no masking on the reads, all 4 bits are decoded */
	AM_RANGE(0x01, 0x01) AM_READ(bowler_shift_res_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)
	AM_RANGE(0x03, 0x03) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x05, 0x05) AM_READ(input_port_2_r)
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(bowler_sh_port_1_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(bowler_sh_port_2_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(bowler_lights_1_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(bowler_sh_port_3_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(bowler_sh_port_4_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(bowler_sh_port_5_w)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(bowler_lights_2_w)
	AM_RANGE(0x0f, 0x0f) AM_WRITE(bowler_sh_port_6_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( bowler )
	PORT_START_TAG("IN0")
	PORT_DIPNAME ( 0x03, 0x00, DEF_STR( Language ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:1,2")
	PORT_DIPSETTING (    0x00, DEF_STR( English ) )
	PORT_DIPSETTING (    0x01, DEF_STR( French ) )
	PORT_DIPSETTING (    0x02, DEF_STR( German ) )
	/*PORT_DIPSETTING (  0x03, DEF_STR( German ) ) */
	PORT_DIPNAME ( 0x04, 0x04, DEF_STR( Demo_Sounds ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:3")
	PORT_DIPSETTING (    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x04, DEF_STR( On ) )  /* every 17 minutes */
	PORT_DIPNAME ( 0x08, 0x08, DEF_STR( Game_Time ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:4")
	PORT_DIPSETTING (    0x00, "No Limit" )
	PORT_DIPSETTING (    0x08, "5 Minutes" )
	PORT_DIPNAME ( 0x10, 0x00, DEF_STR( Coinage ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:5")
	PORT_DIPSETTING (    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME ( 0x20, 0x20, DEF_STR( Difficulty ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:6")
	PORT_DIPSETTING (    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING (    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME ( 0x40, 0x00, DEF_STR( Cabinet ) ) PORT_CONDITION("IN0",0x80,PORTCOND_EQUALS,0x00) PORT_DIPLOCATION("B3:7")
	PORT_DIPSETTING (    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING (    0x40, "Cocktail (not functional)" )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "B3:8" )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Hook/Straight") PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Game Select") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(50) PORT_REVERSE PORT_PLAYER(1)

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END


static MACHINE_DRIVER_START( bowler )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(bowler_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	/* MDRV_IMPORT_FROM(bowler_sound) */

MACHINE_DRIVER_END



/*************************************
 *
 *  Space Invaders (PCB #739)
 *
 *************************************/

#define INVADERS_COIN_INPUT_PORT_TAG	("COIN")
#define INVADERS_SW6_SW7_PORT_TAG		("SW6SW7")
#define INVADERS_SW5_PORT_TAG			("SW5")
#define INVADERS_CAB_TYPE_PORT_TAG		("CAB")
#define INVADERS_P1_CONTROL_PORT_TAG	("CONTP1")
#define INVADERS_P2_CONTROL_PORT_TAG	("CONTP2")


static UINT8 invaders_flip_screen = 0;


static MACHINE_START( invaders )
{
	/* setup for save states */
	state_save_register_global(invaders_flip_screen);

	return machine_start_mw8080bw(machine);
}


UINT8 invaders_is_flip_screen(void)
{
	return invaders_flip_screen;
}


void invaders_set_flip_screen(UINT8 data)
{
	invaders_flip_screen = data;
}


static UINT32 invaders_coin_input_r(void *param)
{
	UINT32 ret = readinputportbytag(INVADERS_COIN_INPUT_PORT_TAG);

	coin_counter_w(0, !ret);

	return ret;
}


static UINT32 invaders_sw6_sw7_r(void *param)
{
	UINT32 ret;

	/* upright PCB : switches visible
       cocktail PCB: HI */

	if (invaders_is_cabinet_cocktail())
	{
		ret = 0x03;
	}
	else
	{
		ret = readinputportbytag(INVADERS_SW6_SW7_PORT_TAG);
	}


	return ret;
}


static UINT32 invaders_sw5_r(void *param)
{
	UINT32 ret;

	/* upright PCB : switch visible
       cocktail PCB: HI */

	if (invaders_is_cabinet_cocktail())
	{
		ret = 0x01;
	}
	else
	{
		ret = readinputportbytag(INVADERS_SW5_PORT_TAG);
	}


	return ret;
}


static UINT32 invaders_in0_control_r(void *param)
{
	UINT32 ret;

	/* upright PCB : P1 controls
       cocktail PCB: HI */

	if (invaders_is_cabinet_cocktail())
	{
		ret = 0x07;
	}
	else
	{
		ret = readinputportbytag(INVADERS_P1_CONTROL_PORT_TAG);
	}


	return ret;
}


static UINT32 invaders_in2_control_r(void *param)
{
	UINT32 ret;

	/* upright PCB : P1 controls
       cocktail PCB: P2 controls */

	if (invaders_is_cabinet_cocktail())
	{
		ret = readinputportbytag(INVADERS_P2_CONTROL_PORT_TAG);
	}
	else
	{
		ret = readinputportbytag(INVADERS_P1_CONTROL_PORT_TAG);
	}


	return ret;
}


int invaders_is_cabinet_cocktail()
{
	return readinputportbytag(INVADERS_CAB_TYPE_PORT_TAG);
}


static ADDRESS_MAP_START( invaders_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(invaders_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(invaders_sh_port_2_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( invaders )
	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_sw6_sw7_r, 0)
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_in0_control_r, 0)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_sw5_r, 0)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_coin_input_r, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) /* in the software, this is TILI, but not connected on the Midway PCB */
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_in2_control_r, 0)
	PORT_DIPNAME( 0x80, 0x00, "Display Coinage" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* fake port for reading the coin input */
	PORT_START_TAG(INVADERS_COIN_INPUT_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* fake port for cabinet type */
	PORT_START_TAG(INVADERS_CAB_TYPE_PORT_TAG)
	PORT_CONFNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_CONFSETTING(    0x00, DEF_STR( Upright ) )
	PORT_CONFSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* fake ports for handling the various input ports based on cabinet type */
	PORT_START_TAG(INVADERS_SW6_SW7_PORT_TAG)
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(INVADERS_SW5_PORT_TAG)
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(INVADERS_P1_CONTROL_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG(INVADERS_P2_CONTROL_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


MACHINE_DRIVER_START( invaders )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(invaders_io_map,0)
	MDRV_MACHINE_START(invaders)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* video hardware */
	MDRV_VIDEO_UPDATE(invaders)

	/* sound hardware */
	MDRV_IMPORT_FROM(invaders_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Blue Shark (PCB #742)
 *
 *************************************/

#define BLUESHRK_COIN_INPUT_PORT_TAG	("COIN")


static UINT32 blueshrk_coin_input_r(void *param)
{
	UINT32 ret = readinputportbytag(BLUESHRK_COIN_INPUT_PORT_TAG);

	coin_counter_w(0, !ret);

	return ret;
}


static ADDRESS_MAP_START( blueshrk_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_rev_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(blueshrk_sh_port_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(watchdog_reset_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( blueshrk )
	PORT_START_TAG(BLUESHRK_SPEAR_PORT_TAG)
	PORT_BIT( 0xff, 0x45, IPT_PADDLE ) PORT_CROSSHAIR(X, 1.0, 0.0, 0.139) PORT_MINMAX(0x08,0x82) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(blueshrk_coin_input_r, 0)
	PORT_DIPNAME ( 0x04, 0x04, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING (    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )  /* not shown on the schematics, instead DIP SW4 is connected here */
	PORT_DIPNAME ( 0x10, 0x10, DEF_STR( Unused ) ) PORT_CONDITION("IN1",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("SW:5")
	PORT_DIPSETTING (    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING (    0x00, DEF_STR( On ) )
	PORT_DIPNAME ( 0x60, 0x40, "Replay" ) PORT_CONDITION("IN1",0x80,PORTCOND_EQUALS,0x80) PORT_DIPLOCATION("SW:6,7")
	PORT_DIPSETTING (    0x20, "14000" )
	PORT_DIPSETTING (    0x40, "18000" )
	PORT_DIPSETTING (    0x60, "22000" )
	PORT_DIPSETTING (    0x00, DEF_STR( None ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_LOW, "SW:8" )

	/* fake port for reading the coin input */
	PORT_START_TAG(BLUESHRK_COIN_INPUT_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( blueshrk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(blueshrk_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(blueshrk_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  Space Invaders II (cocktail) (PCB #851)
 *
 *************************************/

#define INVAD2CT_COIN_INPUT_PORT_TAG	("COIN")


static UINT32 invad2ct_coin_input_r(void *param)
{
	UINT32 ret = readinputportbytag(INVAD2CT_COIN_INPUT_PORT_TAG);

	coin_counter_w(0, !ret);

	return ret;
}


static ADDRESS_MAP_START( invad2ct_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(3) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x04) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x04) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x04) AM_READ(mw8080bw_shift_res_r)

	AM_RANGE(0x01, 0x01) AM_WRITE(invad2ct_sh_port_3_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(mw8080bw_shift_amount_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(invad2ct_sh_port_1_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(mw8080bw_shift_data_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(invad2ct_sh_port_2_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(invad2ct_sh_port_4_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( invad2ct )
	PORT_START_TAG("IN0")
	PORT_SERVICE_DIPLOC( 0x01, IP_ACTIVE_LOW, "SW:8" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )  /* labeled NAMED RESET, but not read by the software */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(invaders_coin_input_r, 0)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW:3,4")
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW:2") /* this switch only changes the orientation of the score */
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY  PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING(    0x80, "1500" )
	PORT_DIPSETTING(    0x00, "2000" )

	/* fake port for reading the coin input */
	PORT_START_TAG(INVAD2CT_COIN_INPUT_PORT_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static MACHINE_DRIVER_START( invad2ct )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(root)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(invad2ct_io_map,0)
	MDRV_WATCHDOG_TIME_INIT(255 / (MW8080BW_PIXEL_CLOCK / MW8080BW_HTOTAL / MW8080BW_VTOTAL))

	/* sound hardware */
	MDRV_IMPORT_FROM(invad2ct_sound)

MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( seawolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sw0041.h",   0x0000, 0x0400, CRC(8f597323) SHA1(b538277d3a633dd8a3179cff202f18d322e6fe17) )
	ROM_LOAD( "sw0042.g",   0x0400, 0x0400, CRC(db980974) SHA1(cc2a99b18695f61e0540c9f6bf8fe3b391dde4a0) )
	ROM_LOAD( "sw0043.f",   0x0800, 0x0400, CRC(e6ffa008) SHA1(385198434b08fe4651ad2c920d44fb49cfe0bc33) )
	ROM_LOAD( "sw0044.e",   0x0c00, 0x0400, CRC(c3557d6a) SHA1(bd345dd72fed8ce15da76c381782b025f71b006f) )
ROM_END

ROM_START( gunfight )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "7609h.bin",  0x0000, 0x0400, CRC(0b117d73) SHA1(99d01313e251818d336281700e206d9003c71dae) )
	ROM_LOAD( "7609g.bin",  0x0400, 0x0400, CRC(57bc3159) SHA1(c177e3f72db9af17ab99b2481448ca26318184b9) )
	ROM_LOAD( "7609f.bin",  0x0800, 0x0400, CRC(8049a6bd) SHA1(215b068663e431582591001cbe028929fa96d49f) )
	ROM_LOAD( "7609e.bin",  0x0c00, 0x0400, CRC(773264e2) SHA1(de3f2e6841122bbe6e2fda5b87d37842c072289a) )
ROM_END

ROM_START( tornbase )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tb.h",       0x0000, 0x0800, CRC(653f4797) SHA1(feb4c802aa3e0c2a66823cd032496cca5742c883) )
	ROM_LOAD( "tb.g",       0x0800, 0x0800, CRC(b63dcdb3) SHA1(bdaa0985bcb5257204ee10faa11a4e02a38b9ac5) )
	ROM_LOAD( "tb.f",       0x1000, 0x0800, CRC(215e070c) SHA1(425915b37e5315f9216707de0850290145f69a30) )
ROM_END

ROM_START( 280zzzap )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "zzzaph",     0x0000, 0x0400, CRC(1fa86e1c) SHA1(b9cf16eb037ada73631ed24297e9e3b3bf6ab3cd) )
	ROM_LOAD( "zzzapg",     0x0400, 0x0400, CRC(9639bc6b) SHA1(b2e2497e421e79a411d07ebf2eed2bb8dc227003) )
	ROM_LOAD( "zzzapf",     0x0800, 0x0400, CRC(adc6ede1) SHA1(206bf2575696c4b14437f3db37a215ba33211943) )
	ROM_LOAD( "zzzape",     0x0c00, 0x0400, CRC(472493d6) SHA1(ae5cf4481ee4b78ca0d2f4d560d295e922aa04a7) )
	ROM_LOAD( "zzzapd",     0x1000, 0x0400, CRC(4c240ee1) SHA1(972475f80253bb0d24773a10aec26a12f28e7c23) )
	ROM_LOAD( "zzzapc",     0x1400, 0x0400, CRC(6e85aeaf) SHA1(ffa6bb84ef1f7c2d72fd26c24bd33aa014aeab7e) )
ROM_END

ROM_START( maze )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "maze.h",     0x0000, 0x0800, CRC(f2860cff) SHA1(62b3fd3d04bf9c5dd9b50964374fb884dc0ab79c) )
	ROM_LOAD( "maze.g",     0x0800, 0x0800, CRC(65fad839) SHA1(893f0a7621e7df19f777be991faff0db4a9ad571) )
ROM_END

ROM_START( boothill )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "romh.cpu",   0x0000, 0x0800, CRC(1615d077) SHA1(e59a26c2f2fc67ab24301e22d2e3f33043acdf72) )
	ROM_LOAD( "romg.cpu",   0x0800, 0x0800, CRC(65a90420) SHA1(9f36c44b5ae5b912cdbbeb9ff11a42221b8362d2) )
	ROM_LOAD( "romf.cpu",   0x1000, 0x0800, CRC(3fdafd79) SHA1(b18e8ac9df40c4687ac1acd5174eb99f2ef60081) )
	ROM_LOAD( "rome.cpu",   0x1800, 0x0800, CRC(374529f4) SHA1(18c57b79df0c66052eef40a694779a5ade15d0e0) )
ROM_END

ROM_START( checkmat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "checkmat.h", 0x0000, 0x0400, CRC(3481a6d1) SHA1(f758599d6393398a6a8e6e7399dc1a3862604f65) )
	ROM_LOAD( "checkmat.g", 0x0400, 0x0400, CRC(df5fa551) SHA1(484ff9bfb95166ba09f34c753a7908a73de3cc7d) )
	ROM_LOAD( "checkmat.f", 0x0800, 0x0400, CRC(25586406) SHA1(39e0cf502735819a7e1d933e3686945fcfae21af) )
	ROM_LOAD( "checkmat.e", 0x0c00, 0x0400, CRC(59330d84) SHA1(453f95dd31968d439339c41e625481170437eb0f) )
	ROM_LOAD( "checkmat.d", 0x1000, 0x0400, NO_DUMP )	/* language ROM */
ROM_END

ROM_START( desertgu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "desertgu.h", 0x0000, 0x0800, CRC(c0030d7c) SHA1(4d0a3a59d4f8181c6e30966a6b1d19ba5b29c398) )
	ROM_LOAD( "desertgu.g", 0x0800, 0x0800, CRC(1ddde10b) SHA1(8fb8e85844a8ec6c0722883013ecdd4eeaeb08c1) )
	ROM_LOAD( "desertgu.f", 0x1000, 0x0800, CRC(808e46f1) SHA1(1cc4e9b0aa7e9546c133bd40d40ede6f2fbe93ba) )
	ROM_LOAD( "desertgu.e", 0x1800, 0x0800, CRC(ac64dc62) SHA1(202433dfb174901bd3b91e843d9d697a8333ef9e) )
ROM_END

ROM_START( dplay )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dplay619.h", 0x0000, 0x0800, CRC(6680669b) SHA1(49ad2333f81613c2f27231de60b415cbc254546a) )
	ROM_LOAD( "dplay619.g", 0x0800, 0x0800, CRC(0eec7e01) SHA1(2661e77061119d7d95d498807bd29d2630c6b6ab) )
	ROM_LOAD( "dplay619.f", 0x1000, 0x0800, CRC(3af4b719) SHA1(3122138ac36b1a129226836ddf1916d763d73e10) )
	ROM_LOAD( "dplay619.e", 0x1800, 0x0800, CRC(65cab4fc) SHA1(1ce7cb832e95e4a6d0005bf730eec39225b2e960) )
ROM_END

ROM_START( lagunar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "lagunar.h",  0x0000, 0x0800, CRC(0cd5a280) SHA1(89a744c912070f11b0b90b0cc92061e238b00b64) )
	ROM_LOAD( "lagunar.g",  0x0800, 0x0800, CRC(824cd6f5) SHA1(a74f6983787cf040eab6f19de2669c019962b9cb) )
	ROM_LOAD( "lagunar.f",  0x1000, 0x0800, CRC(62692ca7) SHA1(d62051bd1b45ca6e60df83942ff26a64ae25a97b) )
	ROM_LOAD( "lagunar.e",  0x1800, 0x0800, CRC(20e098ed) SHA1(e0c52c013f5e93794b363d7762ce0f34ba98c660) )
ROM_END

ROM_START( gmissile )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gm_623.h",   0x0000, 0x0800, CRC(a3ebb792) SHA1(30d9613de849c1a868056c5e28cf2a8608b63e88) )
	ROM_LOAD( "gm_623.g",   0x0800, 0x0800, CRC(a5e740bb) SHA1(963c0984953eb58fe7eab84fabb724ec6e29e706) )
	ROM_LOAD( "gm_623.f",   0x1000, 0x0800, CRC(da381025) SHA1(c9d0511567ed571b424459896ce7de0326850388) )
	ROM_LOAD( "gm_623.e",   0x1800, 0x0800, CRC(f350146b) SHA1(a07000a979b1a735754eca623cc880988924877f) )
ROM_END

ROM_START( m4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "m4.h",       0x0000, 0x0800, CRC(9ee2a0b5) SHA1(b81b4001c90ac6db25edd838652c42913022d9a9) )
	ROM_LOAD( "m4.g",       0x0800, 0x0800, CRC(0e84b9cb) SHA1(a7b74851979aaaa16496e506c487a18df14ab6dc) )
	ROM_LOAD( "m4.f",       0x1000, 0x0800, CRC(9ded9956) SHA1(449204a50efd3345cde815ca5f1fb596843a30ac) )
	ROM_LOAD( "m4.e",       0x1800, 0x0800, CRC(b6983238) SHA1(3f3b99b33135e144c111d2ebaac8f9433c269bc5) )
ROM_END

ROM_START( clowns )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "h2.cpu",     0x0000, 0x0400, CRC(ff4432eb) SHA1(997aee1e3669daa1d8169b4e103d04baaab8ea8d) )
	ROM_LOAD( "g2.cpu",     0x0400, 0x0400, CRC(676c934b) SHA1(72b681ca9ef23d820fdd297cc417932aecc9677b) )
	ROM_LOAD( "f2.cpu",     0x0800, 0x0400, CRC(00757962) SHA1(ef39211493393e97284a08eea63be0757643ac88) )
	ROM_LOAD( "e2.cpu",     0x0c00, 0x0400, CRC(9e506a36) SHA1(8aad486a72d148d8b03e7bec4c12abd14e425c5f) )
	ROM_LOAD( "d2.cpu",     0x1000, 0x0400, CRC(d61b5b47) SHA1(6051c0a2e81d6e975e82c2d48d0e52dc0d4723e3) )
	ROM_LOAD( "c2.cpu",     0x1400, 0x0400, CRC(154d129a) SHA1(61eebb319ee3a6be598b764b295c18a93a953c1e) )
ROM_END

ROM_START( clowns1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "clownsv1.h", 0x0000, 0x0400, CRC(5560c951) SHA1(b6972e1918604263579de577ec58fa6a91e8ff3e) )
	ROM_LOAD( "clownsv1.g", 0x0400, 0x0400, CRC(6a571d66) SHA1(e825f95863e901a1b648c74bb47098c8e74f179b) )
	ROM_LOAD( "clownsv1.f", 0x0800, 0x0400, CRC(a2d56cea) SHA1(61bc07e6a24a1980216453b4dd2688695193a4ae) )
	ROM_LOAD( "clownsv1.e", 0x0c00, 0x0400, CRC(bbd606f6) SHA1(1cbaa21d9834c8d76cf335fd118851591e815c86) )
	ROM_LOAD( "clownsv1.d", 0x1000, 0x0400, CRC(37b6ff0e) SHA1(bf83bebb6c14b3663ca86a180f9ae3cddb84e571) )
	ROM_LOAD( "clownsv1.c", 0x1400, 0x0400, CRC(12968e52) SHA1(71e4f09d30b992a4ac44b0e88e83b4f8a0f63caa) )
ROM_END

ROM_START( einning )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ei.h",       0x0000, 0x0800, CRC(eff9c7af) SHA1(316fffc972bd9935ead5ee4fd629bddc8a8ed5ce) )
	ROM_LOAD( "ei.g",       0x0800, 0x0800, CRC(5d1e66cb) SHA1(a5475362e12b7c251a05d67c2fd070cf7d333ad0) )
	ROM_LOAD( "ei.f",       0x1000, 0x0800, CRC(ed96785d) SHA1(d5557620227fcf6f30dcf6c8f5edd760d77d30ae) )
	ROM_LOAD( "ei.e",       0x1800, 0x0800, CRC(ad096a5d) SHA1(81d48302a0e039b8601a6aed7276e966592af693) )
	ROM_LOAD( "ei.b",       0x5000, 0x0800, CRC(56b407d4) SHA1(95e4be5b2f28192df85c6118079de2e68838b67c) )
ROM_END

ROM_START( shuffle )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "shuffle.h",  0x0000, 0x0800, CRC(0d422a18) SHA1(909c5b9e3c1194abd101cbf993a2ed7c8fbeb5d0) )
	ROM_LOAD( "shuffle.g",  0x0800, 0x0800, CRC(7db7fcf9) SHA1(f41b568f2340e5307a7a45658946cfd4cf4056bf) )
	ROM_LOAD( "shuffle.f",  0x1000, 0x0800, CRC(cd04d848) SHA1(f0f7e9bc483f08934d5c29568b4a7fe084623031) )
	ROM_LOAD( "shuffle.e",  0x1800, 0x0800, CRC(2c118357) SHA1(178db02aaa70963dd8dbcb9b8651209913c539af) )
ROM_END

ROM_START( dogpatch )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dogpatch.h", 0x0000, 0x0800, CRC(74ebdf4d) SHA1(6b31f9563b0f79fe9128ee83e85a3e2f90d7985b) )
	ROM_LOAD( "dogpatch.g", 0x0800, 0x0800, CRC(ac246f70) SHA1(7ee356c3218558a78ee0ff495f9f51ef88cac951) )
	ROM_LOAD( "dogpatch.f", 0x1000, 0x0800, CRC(a975b011) SHA1(fb807d9eefde7177d7fd7ab06fc2dbdc58ae6fcb) )
	ROM_LOAD( "dogpatch.e", 0x1800, 0x0800, CRC(c12b1f60) SHA1(f0504e16d2ce60a0fb3fc2af8c323bfca0143818) )
ROM_END

ROM_START( spcenctr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "4m33.h",     0x0000, 0x0800, CRC(7458b2db) SHA1(c4f41efb8a35fd8bebc75bff0111476affe2b34d) )
	ROM_LOAD( "4m32.g",     0x0800, 0x0800, CRC(1b873788) SHA1(6cdf0d602a65c7efcf8abe149c6172b4c7ab87a1) )
	ROM_LOAD( "4m31.f",     0x1000, 0x0800, CRC(d4319c91) SHA1(30830595c220f490fe150ad018fbf4671bb71e02) )
	ROM_LOAD( "4m30.e",     0x1800, 0x0800, CRC(9b9a1a45) SHA1(8023a05c13e8b541f9e2fe4d389e6a2dcd4766ea) )
	ROM_LOAD( "4m29.d",     0x4000, 0x0800, CRC(294d52ce) SHA1(0ee63413c5caf60d45ae8bef08f6c07099d30f79) )
	ROM_LOAD( "4m28.c",     0x4800, 0x0800, CRC(ce44c923) SHA1(9d35908de3194c5fe6fc8495ae413fa722018744) )
	ROM_LOAD( "4m27.b",     0x5000, 0x0800, CRC(098070ab) SHA1(72ae344591df0174353dc2e3d22daf5a70e2261f) )
	ROM_LOAD( "4m26.a",     0x5800, 0x0800, CRC(7f1d1f44) SHA1(2f4951171a55e7ac072742fa24eceeee6aca7e39) )
ROM_END

ROM_START( phantom2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "phantom2.h", 0x0000, 0x0800, CRC(0e3c2439) SHA1(450182e590845c651530b2c84e1f11fe2451dcf6) )
	ROM_LOAD( "phantom2.g", 0x0800, 0x0800, CRC(e8df3e52) SHA1(833925e44e686df4d4056bce4c0ffae3269d57df) )
	ROM_LOAD( "phantom2.f", 0x1000, 0x0800, CRC(30e83c6d) SHA1(fe34a3e4519a7e5ffe66e76fe974049988656b71) )
	ROM_LOAD( "phantom2.e", 0x1800, 0x0800, CRC(8c641cac) SHA1(c4986daacb7ed9efed59b022c6101240b0eddcdc) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )      /* cloud graphics */
	ROM_LOAD( "p2clouds.f2",0x0000, 0x0800, CRC(dcdd2927) SHA1(d8d42c6594e36c12b40ee6342a9ad01a8bbdef75) )
ROM_END

ROM_START( bowler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "h.cpu",      0x0000, 0x0800, CRC(74c29b93) SHA1(9cbd5b7b8a4c889406b6bc065360f74c036320b2) )
	ROM_LOAD( "g.cpu",      0x0800, 0x0800, CRC(ca26d8b4) SHA1(cf18991cde8044a961cf556f18c6eb60a7ade595) )
	ROM_LOAD( "f.cpu",      0x1000, 0x0800, CRC(ba8a0bfa) SHA1(bb017ddac58d031b249596b70ab1068cd1bad499) )
	ROM_LOAD( "e.cpu",      0x1800, 0x0800, CRC(4da65a40) SHA1(7795d59870fa722da89888e72152145662554080) )
	ROM_LOAD( "d.cpu",      0x4000, 0x0800, CRC(e7dbc9d9) SHA1(05049a69ee588de85db86df188e7670778b77e90) )
ROM_END

ROM_START( invaders )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "invaders.h", 0x0000, 0x0800, CRC(734f5ad8) SHA1(ff6200af4c9110d8181249cbcef1a8a40fa40b7f) )
	ROM_LOAD( "invaders.g", 0x0800, 0x0800, CRC(6bfaca4a) SHA1(16f48649b531bdef8c2d1446c429b5f414524350) )
	ROM_LOAD( "invaders.f", 0x1000, 0x0800, CRC(0ccead96) SHA1(537aef03468f63c5b9e11dd61e253f7ae17d9743) )
	ROM_LOAD( "invaders.e", 0x1800, 0x0800, CRC(14e538b0) SHA1(1d6ca0c99f9df71e2990b610deb9d7da0125e2d8) )
ROM_END

ROM_START( blueshrk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "blueshrk.h", 0x0000, 0x0800, CRC(4ff94187) SHA1(7cb80e2ccc34983bfd688c549ffc032d6dacf880) )
	ROM_LOAD( "blueshrk.g", 0x0800, 0x0800, CRC(e49368fd) SHA1(2495ba48532bb714361e4f0e94c9317161c6c77f) )
	ROM_LOAD( "blueshrk.f", 0x1000, 0x0800, CRC(86cca79d) SHA1(7b4633fb8033ee2c0e692135c383ebf57deef0e5) )
ROM_END

ROM_START( invad2ct )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "invad2ct.h", 0x0000, 0x0800, CRC(51d02a71) SHA1(2fa82ddc2702a72de0a9559ec244b70ab3db3f18) )
	ROM_LOAD( "invad2ct.g", 0x0800, 0x0800, CRC(533ac770) SHA1(edb65c289027432dad7861a7d6abbda9223c13b1) )
	ROM_LOAD( "invad2ct.f", 0x1000, 0x0800, CRC(d1799f39) SHA1(f7f1ba34d57f9883241ba3ef90e34ed20dfb8003) )
	ROM_LOAD( "invad2ct.e", 0x1800, 0x0800, CRC(291c1418) SHA1(0d9f7973ed81d28c43ef8b96f1180d6629871785) )
	ROM_LOAD( "invad2ct.b", 0x5000, 0x0800, CRC(8d9a07c4) SHA1(4acbe15185d958b5589508dc0ea3a615fbe3bcca) )
	ROM_LOAD( "invad2ct.a", 0x5800, 0x0800, CRC(efdabb03) SHA1(33f4cf249e88e2b7154350e54c479eb4fa86f26f) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/* PCB #              rom       parent  machine   inp */

/* 596 */ GAME( 1976, seawolf,  0,      seawolf,  seawolf,  0, ROT0,   "Midway", "Sea Wolf", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
/* 597 */ GAMEL(1975, gunfight, 0,      gunfight, gunfight, 0, ROT0,   "Midway", "Gun Fight", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE , layout_hoffff20 )
/* 605 */ GAME( 1976, tornbase, 0,      tornbase, tornbase, 0, ROT0,   "Midway", "Tornado Baseball", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 610 */ GAME( 1976, 280zzzap, 0,      zzzap,    zzzap,    0, ROT0,   "Midway", "Datsun 280 Zzzap", GAME_NO_SOUND | GAME_SUPPORTS_SAVE  )
/* 611 */ GAMEL(1976, maze,     0,      maze,     maze,     0, ROT0,   "Midway", "Amazing Maze", GAME_NO_SOUND | GAME_SUPPORTS_SAVE , layout_ho4f893d )
/* 612 */ GAME( 1977, boothill, 0,      boothill, boothill, 0, ROT0,   "Midway", "Boot Hill" , GAME_SUPPORTS_SAVE  )
/* 615 */ GAME( 1977, checkmat, 0,      checkmat, checkmat, 0, ROT0,   "Midway", "Checkmate", GAME_NO_SOUND | GAME_SUPPORTS_SAVE  )
/* 618 */ GAME( 1977, desertgu, 0,      desertgu, desertgu, 0, ROT0,   "Midway", "Desert Gun", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 619 */ GAME( 1977, dplay,    0,      dplay,    dplay,    0, ROT0,   "Midway", "Double Play", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 622 */ GAME( 1977, lagunar,  0,      zzzap,    lagunar,  0, ROT90,  "Midway", "Laguna Racer", GAME_NO_SOUND | GAME_SUPPORTS_SAVE  )
/* 623 */ GAME( 1977, gmissile, 0,      gmissile, gmissile, 0, ROT0,   "Midway", "Guided Missile", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 626 */ GAME( 1977, m4,       0,      m4,       m4,       0, ROT0,   "Midway", "M-4", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 630 */ GAMEL(1978, clowns,   0,      clowns,   clowns,   0, ROT0,   "Midway", "Clowns (rev. 2)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE , layout_clowns )
/* 630 */ GAMEL(1978, clowns1,  clowns, clowns,   clowns1,  0, ROT0,   "Midway", "Clowns (rev. 1)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE , layout_clowns )
/* 640 Space Walk (dump does not exist) */
/* 642 */ GAME( 1978, einning,  0,      dplay,    einning,  0, ROT0,   "Midway", "Extra Inning", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 643 */ GAME( 1978, shuffle,  0,      shuffle,  shuffle,  0, ROT90,  "Midway", "Shuffleboard", GAME_NO_SOUND | GAME_SUPPORTS_SAVE  )
/* 644 */ GAME( 1977, dogpatch, 0,      dogpatch, dogpatch, 0, ROT0,   "Midway", "Dog Patch", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 645 */ GAME( 1980, spcenctr, 0,      spcenctr, spcenctr, 0, ROT0,   "Midway", "Space Encounters", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 652 */ GAMEL(1979, phantom2, 0,      phantom2, phantom2, 0, ROT0,   "Midway", "Phantom II", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE , layout_hoa0a0ff )
/* 730 */ GAME( 1978, bowler,   0,      bowler,   bowler,   0, ROT90,  "Midway", "Bowling Alley", GAME_NO_SOUND | GAME_SUPPORTS_SAVE  )
/* 739 */ GAMEL(1978, invaders, 0,      invaders, invaders, 0, ROT270, "Midway", "Space Invaders", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE , layout_invaders )
/* 742 */ GAME( 1978, blueshrk, 0,      blueshrk, blueshrk, 0, ROT0,   "Midway", "Blue Shark", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE  )
/* 749 4 Player Bowling Alley (cocktail, dump does not exist) */
/* 851 */ GAMEL(1980, invad2ct, 0,      invad2ct, invad2ct, 0, ROT90,  "Midway", "Space Invaders II (Midway, cocktail)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE, layout_invad2ct )
/* 852 Space Invaders Deluxe (color hardware, not in this driver) */
/* 870 Space Invaders Deluxe (cocktail, dump does not exist) */
