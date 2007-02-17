/*************************************************************************

    Cinematronics Dragon's Lair system

    Games supported:
        * Dragon's Lair
        * Dragon's Lair (European version)
        * Space Ace
        * Space Ace (European version)

**************************************************************************

    There are two revisions of the Cinematronics board used in the
    U.S. Rev A


    ROM Revisions
    -------------
    Revision A, B, and C EPROMs use the Pioneer PR-7820 only.
    Revision D EPROMs used the Pioneer LD-V1000 only.
    Revisions E, F, and F2 EPROMs used either player.

    Revisions A, B, C, and D are a five chip set.
    Revisions E, F, and F2 are a four chip set.


    Laserdisc Players Used
    ----------------------
    Pioneer PR-7820 (USA / Cinematronics)
    Pioneer LD-V1000 (USA / Cinematronics)
    Philips 22VP932 (Europe / Atari) and (Italian / Sidam)

*************************************************************************/

#include "driver.h"
#include "render.h"
#include "cpu/z80/z80daisy.h"
#include "machine/laserdsc.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "sound/ay8910.h"
#include "sound/custom.h"
#include "sound/beep.h"
#include "dlair.lh"



/*************************************
 *
 *  Constants
 *
 *************************************/

#define MASTER_CLOCK_US				16000000
#define MASTER_CLOCK_EURO			14318180

#define LASERDISC_TYPE_MASK			0x7f
#define LASERDISC_TYPE_VARIABLE		0x80



/*************************************
 *
 *  Globals
 *
 *************************************/

static laserdisc_info *discinfo;
static UINT8 last_misc;

static UINT8 laserdisc_type;
static UINT8 laserdisc_data;

static render_texture *video_texture;
static render_texture *overlay_texture;
static UINT32 last_seqid;

static mame_bitmap *overlay_bitmap;

static const UINT8 led_map[16] =
	{ 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7c,0x07,0x7f,0x67,0x77,0x7c,0x39,0x5e,0x79,0x00 };



/*************************************
 *
 *  Z80 peripheral interfaces
 *
 *************************************/

static void dleuro_interrupt(int state)
{
	cpunum_set_input_line(0, 0, state);
}


WRITE8_HANDLER( serial_transmit )
{
	laserdisc_data_w(discinfo, data);
}


int serial_receive(int ch)
{
	/* if we still have data to send, do it now */
	if (ch == 0 && laserdisc_line_r(discinfo, LASERDISC_LINE_DATA_AVAIL) == ASSERT_LINE)
		return laserdisc_data_r(discinfo);

	return -1;
}


static z80ctc_interface ctc_intf =
{
	0,              	/* clock (filled in from the CPU 0 clock) */
	0,              	/* timer disables */
	dleuro_interrupt,  	/* interrupt handler */
	0,					/* ZC/TO0 callback */
	0,              	/* ZC/TO1 callback */
	0               	/* ZC/TO2 callback */
};


static z80sio_interface sio_intf =
{
	0,                  /* clock (filled in from the CPU 3 clock) */
	dleuro_interrupt,	/* interrupt handler */
	0,					/* DTR changed handler */
	0,					/* RTS changed handler */
	0,					/* BREAK changed handler */
	serial_transmit,	/* transmit handler */
	serial_receive		/* receive handler */
};


struct z80_irq_daisy_chain dleuro_daisy_chain[] =
{
	{ z80sio_reset, z80sio_irq_state, z80sio_irq_ack, z80sio_irq_reti, 0 },
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }		/* end mark */
};



/*************************************
 *
 *  Video startup/shutdown
 *
 *************************************/

static void video_cleanup(running_machine *machine)
{
	/* free our textures */
	if (video_texture != NULL)
		render_texture_free(video_texture);
	if (overlay_texture != NULL)
		render_texture_free(overlay_texture);

	/* ensure all async laserdisc activity is complete */
	laserdisc_exit(discinfo);
}


VIDEO_START( dlair )
{
	mame_bitmap *vidbitmap;

	/* create textures */
	last_seqid = laserdisc_get_video(discinfo, &vidbitmap);
	video_texture = render_texture_alloc(vidbitmap, NULL, 0, TEXFORMAT_YUY16, NULL, NULL);
	overlay_bitmap = NULL;

	add_exit_callback(machine, video_cleanup);
	return 0;
}


PALETTE_INIT( dleuro )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		palette_set_color(machine, i, pal1bit(i >> 0), pal1bit(i >> 1), pal1bit(i >> 2));
		colortable[2 * i + 0] = 8;
		colortable[2 * i + 1] = i;
	}
	palette_set_color(machine, 8, 0, 0, 0);
}


VIDEO_START( dleuro )
{
	int result = video_start_dlair(machine);

	overlay_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	fillbitmap(overlay_bitmap, 8, NULL);
	overlay_texture = render_texture_alloc(overlay_bitmap, NULL, 0, TEXFORMAT_PALETTE16, NULL, NULL);

	return result;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( dlair )
{
	mame_bitmap *vidbitmap;
	UINT32 seqid;

	/* get the current video and update the bitmap if different */
	seqid = laserdisc_get_video(discinfo, &vidbitmap);
	if (seqid != last_seqid)
		render_texture_set_bitmap(video_texture, vidbitmap, NULL, 0, TEXFORMAT_YUY16);
	last_seqid = seqid;

	/* cover the whole screen with a quad */
	render_screen_add_quad(0, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), video_texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));

	if (discinfo != NULL)
		popmessage("%s", laserdisc_describe_state(discinfo));

	return 0;
}


VIDEO_UPDATE( dleuro )
{
	mame_bitmap *vidbitmap;
	UINT32 seqid;
	int x, y;

	/* redraw the overlay */
	for (y = 0; y < 32; y++)
		for (x = 0; x < 32; x++)
		{
			UINT8 *base = &videoram[y * 64 + x * 2 + 1];
			drawgfx(overlay_bitmap, Machine->gfx[0], base[0], base[1], 0, 0, 10 * x, 16 * y, cliprect, TRANSPARENCY_NONE, 0);
		}

	/* update the overlay */
	render_texture_set_bitmap(overlay_texture, overlay_bitmap, &machine->screen[0].visarea, 0, TEXFORMAT_PALETTE16);

	/* get the current video and update the bitmap if different */
	seqid = laserdisc_get_video(discinfo, &vidbitmap);
	if (seqid != last_seqid)
		render_texture_set_bitmap(video_texture, vidbitmap, NULL, 0, TEXFORMAT_YUY16);
	last_seqid = seqid;

	/* cover the whole screen with a quad */
	if (last_misc & 0x02)
		render_screen_add_quad(0, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), video_texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));
	else
		render_screen_add_quad(0, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), overlay_texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));

	if (discinfo != NULL)
		popmessage("%s", laserdisc_describe_state(discinfo));

	return 0;
}



/*************************************
 *
 *  Machine startup/reset
 *
 *************************************/

static MACHINE_START( dlair )
{
	discinfo = laserdisc_init(laserdisc_type & LASERDISC_TYPE_MASK, get_disk_handle(0), 0);
	return 0;
}


static MACHINE_START( dleuro )
{
	/* initialize the CTC and SIO peripherals */
	ctc_intf.baseclock = Machine->drv->cpu[0].cpu_clock;
	sio_intf.baseclock = Machine->drv->cpu[0].cpu_clock;
	z80ctc_init(0, &ctc_intf);
	z80sio_init(0, &sio_intf);

	discinfo = laserdisc_init(laserdisc_type & LASERDISC_TYPE_MASK, get_disk_handle(0), 0);
	return 0;
}


static MACHINE_RESET( dlair )
{
	/* determine the laserdisc player from the DIP switches */
	if (laserdisc_type & LASERDISC_TYPE_VARIABLE)
	{
		laserdisc_type &= ~LASERDISC_TYPE_MASK;
		laserdisc_type |= (readinputportbytag("DSW2") & 0x08) ? LASERDISC_TYPE_LDV1000 : LASERDISC_TYPE_PR7820;
	}
	laserdisc_reset(discinfo, laserdisc_type & LASERDISC_TYPE_MASK);
}



/*************************************
 *
 *  VBLANK callback
 *
 *************************************/

void vblank_callback(void)
{
	/* update the laserdisc */
	laserdisc_vsync(discinfo);

	/* also update the speaker on the European version */
	if (sndti_exists(SOUND_BEEP, 0))
	{
		beep_set_state(0, 1);
		beep_set_frequency(0, z80ctc_getperiod(0, 0));
	}
}



/*************************************
 *
 *  Outputs
 *
 *************************************/

static WRITE8_HANDLER( misc_w )
{
	/*
        D0-D3 = B0-B3
           D4 = coin counter
           D5 = OUT DISC DATA
           D6 = ENTER
           D7 = INT/EXT
    */
	UINT8 diff = data ^ last_misc;
	last_misc = data;

	coin_counter_w(0, (~data >> 4) & 1);

	/* on bit 5 going low, push the data out to the laserdisc player */
	if ((diff & 0x20) && !(data & 0x20))
		laserdisc_data_w(discinfo, laserdisc_data);

	/* on bit 6 going low, we need to signal enter */
	laserdisc_line_w(discinfo, LASERDISC_LINE_ENTER, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
}


static WRITE8_HANDLER( dleuro_misc_w )
{
	/*
           D0 = CHAR GEN ON+
           D1 = KILL VIDEO+
           D2 = SEL CHAR GEN VIDEO+
           D3 = counter 2
           D4 = coin counter
           D5 = OUT DISC DATA
           D6 = ENTER
           D7 = INT/EXT
    */
	UINT8 diff = data ^ last_misc;
	last_misc = data;

	coin_counter_w(1, (~data >> 3) & 1);
	coin_counter_w(0, (~data >> 4) & 1);

	/* on bit 5 going low, push the data out to the laserdisc player */
	if ((diff & 0x20) && !(data & 0x20))
		laserdisc_data_w(discinfo, laserdisc_data);

	/* on bit 6 going low, we need to signal enter */
	laserdisc_line_w(discinfo, LASERDISC_LINE_ENTER, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
}


static WRITE8_HANDLER( led_den1_w )
{
	output_set_digit_value(0 + (offset & 7), led_map[data & 0x0f]);
}


static WRITE8_HANDLER( led_den2_w )
{
	output_set_digit_value(8 + (offset & 7), led_map[data & 0x0f]);
}



/*************************************
 *
 *  Laserdisc communication
 *
 *************************************/

static UINT32 laserdisc_status_r(void *param)
{
	if (discinfo == NULL)
		return 0;

	switch (laserdisc_type & LASERDISC_TYPE_MASK)
	{
		case LASERDISC_TYPE_PR7820:
			return 0;

		case LASERDISC_TYPE_LDV1000:
			return (laserdisc_line_r(discinfo, LASERDISC_LINE_STATUS) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_22VP932:
			return 0;
	}
	return 0;
}


static UINT32 laserdisc_command_r(void *param)
{
	if (discinfo == NULL)
		return 0;

	switch (laserdisc_type & LASERDISC_TYPE_MASK)
	{
		case LASERDISC_TYPE_PR7820:
			return (laserdisc_line_r(discinfo, LASERDISC_LINE_READY) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_LDV1000:
			return (laserdisc_line_r(discinfo, LASERDISC_LINE_COMMAND) == ASSERT_LINE) ? 0 : 1;

		case LASERDISC_TYPE_22VP932:
			return 0;
	}
	return 0;
}


static READ8_HANDLER( laserdisc_r )
{
	UINT8 result = laserdisc_data_r(discinfo);
	mame_printf_debug("laserdisc_r = %02X\n", result);
	return result;
}


static WRITE8_HANDLER( laserdisc_w )
{
	laserdisc_data = data;
}



/*************************************
 *
 *  Z80 SIO/DART handlers
 *
 *************************************/

READ8_HANDLER( sio_r )
{
	return (offset & 1) ? z80sio_c_r(0, (offset >> 1) & 1) : z80sio_d_r(0, (offset >> 1) & 1);
}


WRITE8_HANDLER( sio_w )
{
	if (offset & 1)
		z80sio_c_w(0, (offset >> 1) & 1, data);
	else
		z80sio_d_w(0, (offset >> 1) & 1, data);
}



/*************************************
 *
 *  U.S. version memory map
 *
 *************************************/

/* complete memory map derived from schematics */
static ADDRESS_MAP_START( dlus_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_MIRROR(0x1800) AM_RAM
	AM_RANGE(0xc000, 0xc000) AM_MIRROR(0x1fc7) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xc008, 0xc008) AM_MIRROR(0x1fc7) AM_READ(input_port_2_r)
	AM_RANGE(0xc010, 0xc010) AM_MIRROR(0x1fc7) AM_READ(input_port_3_r)
	AM_RANGE(0xc020, 0xc020) AM_MIRROR(0x1fc7) AM_READ(laserdisc_r)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xe008, 0xe008) AM_MIRROR(0x1fc7) AM_WRITE(misc_w)
	AM_RANGE(0xe010, 0xe010) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xe020, 0xe020) AM_MIRROR(0x1fc7) AM_WRITE(laserdisc_w)
	AM_RANGE(0xe030, 0xe037) AM_MIRROR(0x1fc0) AM_WRITE(led_den2_w)
	AM_RANGE(0xe038, 0xe03f) AM_MIRROR(0x1fc0) AM_WRITE(led_den1_w)
ADDRESS_MAP_END



/*************************************
 *
 *  European version memory map
 *
 *************************************/

/* complete memory map derived from schematics */
static ADDRESS_MAP_START( dleuro_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_MIRROR(0x1800) AM_RAM
	AM_RANGE(0xc000, 0xc7ff) AM_MIRROR(0x1800) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x1f47) // WT LED 1
	AM_RANGE(0xe008, 0xe008) AM_MIRROR(0x1f47) // WT LED 2
	AM_RANGE(0xe010, 0xe010) AM_MIRROR(0x1f47) AM_WRITE(led_den1_w) // WT EXT LED 1
	AM_RANGE(0xe018, 0xe018) AM_MIRROR(0x1f47) AM_WRITE(led_den2_w) // WT EXT LED 2
	AM_RANGE(0xe020, 0xe020) AM_MIRROR(0x1f47) AM_WRITE(laserdisc_w) // DISC WT
	AM_RANGE(0xe028, 0xe028) AM_MIRROR(0x1f47) AM_WRITE(dleuro_misc_w) // WT MISC
	AM_RANGE(0xe030, 0xe030) AM_MIRROR(0x1f47) AM_WRITE(watchdog_reset_w) // CLR WDOG
	AM_RANGE(0xe080, 0xe080) AM_MIRROR(0x1f47) AM_READ(input_port_0_r) // CP A
	AM_RANGE(0xe088, 0xe088) AM_MIRROR(0x1f47) AM_READ(input_port_1_r) // CP B
	AM_RANGE(0xe090, 0xe090) AM_MIRROR(0x1f47) AM_READ(input_port_2_r) // OPT SW A
	AM_RANGE(0xe098, 0xe098) AM_MIRROR(0x1f47) AM_READ(input_port_3_r) // OPT SW B
	AM_RANGE(0xe0a0, 0xe0a0) AM_MIRROR(0x1f47) AM_READ(laserdisc_r) // RD DISC DATA
ADDRESS_MAP_END


/* complete memory map derived from schematics */
static ADDRESS_MAP_START( dleuro_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x7c) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0x83) AM_MIRROR(0x7c) AM_READWRITE(sio_r, sio_w)
ADDRESS_MAP_END


/*
WT LED 1 -> LS273 -> DS1 = 7segment
WT LED 2 -> LS273 -> DS2 = 7segment

Character generator:
  decade counter LS90
  cleared on HSYNC
  clocked each pixel
  on 0 -> VOUTLD- -> load RGB color info and character bits (D0=red, D1=green, D2=blue)
  on 3 -> RADRA- -> advance RA0-RA5
  on 6 -> LCHAR- -> load next character into address lines on ROM
  on 7 -> RADRB- -> advance RA0-RA5
  on 9 -> 0

Serial in on the shift register is 0, so extra bits are 0

Video clock:
7.16MHz 0 1 0 1 0 1 0 1 0 1
ACLK+   0 0 1 1 0 0 1 1 0 0 = 3.58Mhz
ACLK-   1 1 0 0 1 1 0 0 1 1 = 3.58Mhz
BCLK+   0 0 0 1 1 0 0 1 1 0 = 3.58Mhz
BCLK-   1 1 1 0 0 1 1 0 0 1 = 3.58Mhz

Row clock:
Cleared on VSYNC
Clocked on HSYNC
ROWCLK- = Qa (i.e., row / 2)

Line clock:
Cleared on VSYNC
Clocked on ROWCLK-
LINECLK- = Qc (i.e., ROWCLK / 8 = row / 16)

Video RAM = 11-bit = 2048 bytes
If CHAR GEN ON+, RAM address = RA0-RA10
If not CHAR GEN ON+, RAM address = BA0-BA10

RA0-RA5 = counters (64 across)
  Cleared to 0 on HSYNC
  Clocked on !(RADRA- & RADRB-)

RA6-RA10 = counters (32 up & down)
  Cleared to 0 on VSYNC
  Clocked on LINECLK- (every 16 rows)
  RA6 = Qb (i.e., counts by 2, not 1)

MA0-MA3 = counters
  Cleared to 0 on VSYNC
  Clocked by ROWCLK- (every 2 rows)

Address in ROM:
  MA0-MA3     = A0-A3
  D0-D7       = A4-A11 (data from VRAM)
  KILL VIDEO+ = A12
*/



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( dlair )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("A:1,0")
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Unused ) )
	PORT_DIPNAME( 0x04, 0x00, "Difficulty Mode" ) PORT_DIPLOCATION("A:2")
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, "Engineering mode" ) PORT_DIPLOCATION("A:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "2 Credits/Free play" ) PORT_DIPLOCATION("A:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) ) PORT_DIPLOCATION("A:5")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Pay as you go" ) PORT_DIPLOCATION("A:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "A:7" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Sound every 8 attracts" ) PORT_DIPLOCATION("B:0")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("B:1")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unlimited Dirks" ) PORT_DIPLOCATION("B:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Joystick Feedback Sound" ) PORT_DIPLOCATION("B:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Pay as you go options" ) PORT_DIPLOCATION("B:6,5")
	PORT_DIPSETTING(    0x00, "PAYG1" )
	PORT_DIPSETTING(    0x20, "PAYG2" )
	PORT_DIPSETTING(    0x40, "PAYG3" )
	PORT_DIPSETTING(    0x60, "PAYG4" )
	PORT_DIPNAME( 0x90, 0x10, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("B:7,4")
	PORT_DIPSETTING(	0x00, "Increase after 5" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x10, "Increase after 9" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x00, DEF_STR( Hard ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x10, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_status_r, 0) 	/* status strobe */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_command_r, 0)	/* command strobe */
INPUT_PORTS_END


INPUT_PORTS_START( dlaire )
	PORT_INCLUDE(dlair)

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x08, 0x08, "LD Player" )		/* In Rev F, F2 and so on... before it was Joystick Sound Feedback */
	PORT_DIPSETTING(    0x00, "LD-PR7820" )
	PORT_DIPSETTING(    0x08, "LDV-1000" )
INPUT_PORTS_END


INPUT_PORTS_START( dleuro )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_status_r, 0) 	/* status strobe */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(laserdisc_command_r, 0)	/* command strobe */

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("A:1,0")
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Unused ) )
	PORT_DIPNAME( 0x04, 0x00, "Difficulty Mode" ) PORT_DIPLOCATION("A:2")
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, "Engineering mode" ) PORT_DIPLOCATION("A:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "2 Credits/Free play" ) PORT_DIPLOCATION("A:4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) ) PORT_DIPLOCATION("A:5")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Pay as you go" ) PORT_DIPLOCATION("A:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_HIGH, "A:7" )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Sound every 8 attracts" ) PORT_DIPLOCATION("B:0")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("B:1")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unlimited Dirks" ) PORT_DIPLOCATION("B:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Joystick Feedback Sound" ) PORT_DIPLOCATION("B:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Pay as you go options" ) PORT_DIPLOCATION("B:6,5")
	PORT_DIPSETTING(    0x00, "PAYG1" )
	PORT_DIPSETTING(    0x20, "PAYG2" )
	PORT_DIPSETTING(    0x40, "PAYG3" )
	PORT_DIPSETTING(    0x60, "PAYG4" )
	PORT_DIPNAME( 0x90, 0x10, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("B:7,4")
	PORT_DIPSETTING(	0x00, "Increase after 5" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x10, "Increase after 9" ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(	0x00, DEF_STR( Hard ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x10, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x80, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
	PORT_DIPSETTING(	0x90, DEF_STR( Easy ) ) PORT_CONDITION("DSW1",0x04,PORTCOND_EQUALS,0x04)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(7,-1) },
	{ STEP16(0,8) },
	16*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout, 0, 8 },
	{ -1 }
};



/*************************************
 *
 *  Sound chip definitions
 *
 *************************************/

struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	input_port_1_r
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( dlair )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK_US/4)
	MDRV_CPU_PROGRAM_MAP(dlus_map,0)
	MDRV_CPU_VBLANK_INT(vblank_callback, 1)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(MASTER_CLOCK_US/8/16/16/16/16))

	MDRV_MACHINE_START(dlair)
	MDRV_MACHINE_RESET(dlair)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_REFRESH_RATE(59.94)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_VIDEO_START(dlair)
	MDRV_VIDEO_UPDATE(dlair)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK_US/8)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.33)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(laserdisc_custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dleuro )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK_EURO/4)
	MDRV_CPU_CONFIG(dleuro_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(dleuro_map,0)
	MDRV_CPU_IO_MAP(dleuro_io_map,0)
	MDRV_CPU_VBLANK_INT(vblank_callback, 1)

	MDRV_WATCHDOG_TIME_INIT(TIME_IN_HZ(MASTER_CLOCK_EURO/16/16/16/16/16/8))

	MDRV_MACHINE_START(dleuro)
	MDRV_MACHINE_RESET(dlair)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 199, 0, 239)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(9)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_PALETTE_INIT(dleuro)
	MDRV_VIDEO_START(dleuro)
	MDRV_VIDEO_UPDATE(dleuro)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.33)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.33)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(laserdisc_custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( dlair )		/* revision F2 */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_f2_u1.bin", 0x0000, 0x2000,  CRC(f5ea3b9d) SHA1(c0cafff8b2982125fd3314ffc66681e47f027fc9) )
	ROM_LOAD( "dl_f2_u2.bin", 0x2000, 0x2000,  CRC(dcc1dff2) SHA1(614ca8f6c5b6fa1d590f6b80d731377faa3a65a9) )
	ROM_LOAD( "dl_f2_u3.bin", 0x4000, 0x2000,  CRC(ab514e5b) SHA1(29d1015b951f0f2d4e5257497f3bf007c5e2262c) )
	ROM_LOAD( "dl_f2_u4.bin", 0x6000, 0x2000,  CRC(f5ec23d2) SHA1(71149e2d359cc5944fbbb53dd7d0c2b42fbc9bb4) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlaira )		/* revision A */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_a_u1.bin", 0x0000, 0x2000,  CRC(d76e83ec) SHA1(fc7ff5d883de9b38a9e0532c35990f4b319ba1d3) )
	ROM_LOAD( "dl_a_u2.bin", 0x2000, 0x2000,  CRC(a6a723d8) SHA1(5c71cb0b6be7331083adaf6fac6bdfc8445cb485) )
	ROM_LOAD( "dl_a_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_a_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_a_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlairb )		/* revision B */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_b_u1.bin", 0x0000, 0x2000,  CRC(d76e83ec) SHA1(fc7ff5d883de9b38a9e0532c35990f4b319ba1d3) )
	ROM_LOAD( "dl_b_u2.bin", 0x2000, 0x2000,  CRC(6751103d) SHA1(e94e19f738e0eb69700e56c6069c7f3c0911303f) )
	ROM_LOAD( "dl_b_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_b_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_b_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlairc )		/* revision C */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_c_u1.bin", 0x0000, 0x2000,  CRC(cebfe26a) SHA1(1c808de5c92fef67d8088621fbd743c1a0a3bb5e) )
	ROM_LOAD( "dl_c_u2.bin", 0x2000, 0x2000,  CRC(6751103d) SHA1(e94e19f738e0eb69700e56c6069c7f3c0911303f) )
	ROM_LOAD( "dl_c_u3.bin", 0x4000, 0x2000,  CRC(52c59014) SHA1(d4015046bf1c1f51c29d9d9f8e8d008519b61cd1) )
	ROM_LOAD( "dl_c_u4.bin", 0x6000, 0x2000,  CRC(924d12f2) SHA1(05b487e651a4817991dfc2308834b8f2fae918b4) )
	ROM_LOAD( "dl_c_u5.bin", 0x8000, 0x2000,  CRC(6ec2f9c1) SHA1(0b8026927697a99fe8fa0dd4bd643418779a1d45) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlaird )		/* revision D */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_d_u1.bin", 0x0000, 0x2000,  CRC(0b5ab120) SHA1(6ec59d6aaa27994d8de4f5635935fd6c1d42d2f6) )
	ROM_LOAD( "dl_d_u2.bin", 0x2000, 0x2000,  CRC(93ebfffb) SHA1(2a8f6d7ab18845e22a2ba238b44d7c636908a125) )
	ROM_LOAD( "dl_d_u3.bin", 0x4000, 0x2000,  CRC(22e6591f) SHA1(3176c07af6d942496c9ae338e3b93e28e2ce7982) )
	ROM_LOAD( "dl_d_u4.bin", 0x6000, 0x2000,  CRC(5f7212cb) SHA1(69c34de1bb44b6cd2adc2947d00d8823d3e87130) )
	ROM_LOAD( "dl_d_u5.bin", 0x8000, 0x2000,  CRC(2b469c89) SHA1(646394b51325ca9163221a43b5af64a8067eb80b) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlaire )		/* revision E */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_e_u1.bin", 0x0000, 0x2000,  CRC(02980426) SHA1(409de05045adbd054bc1fda24d4a9672832e2fae) )
	ROM_LOAD( "dl_e_u2.bin", 0x2000, 0x2000,  CRC(979d4c97) SHA1(5da6ceab5029ac5f5846bf52841675c5c70b17af) )
	ROM_LOAD( "dl_e_u3.bin", 0x4000, 0x2000,  CRC(897bf075) SHA1(d2ff9c2fec37544cfe8fb60273524c6610488502) )
	ROM_LOAD( "dl_e_u4.bin", 0x6000, 0x2000,  CRC(4ebffba5) SHA1(d04711247ffa88e371ec461465dd75a8158d90bc) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dlairf )		/* revision F */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_f_u1.bin", 0x0000, 0x2000,  CRC(06fc6941) SHA1(ea8cf6d370f89d60721ab00ec58ff24027b5252f) )
	ROM_LOAD( "dl_f_u2.bin", 0x2000, 0x2000,  CRC(dcc1dff2) SHA1(614ca8f6c5b6fa1d590f6b80d731377faa3a65a9) )
	ROM_LOAD( "dl_f_u3.bin", 0x4000, 0x2000,  CRC(ab514e5b) SHA1(29d1015b951f0f2d4e5257497f3bf007c5e2262c) )
	ROM_LOAD( "dl_f_u4.bin", 0x6000, 0x2000,  CRC(a817324e) SHA1(1299c83342fc70932f67bda8ae60bace91d66429) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, NO_DUMP )
ROM_END

ROM_START( dleuro )		/* European Atari version */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "elu45.bin", 0x0000, 0x2000, CRC(4d3a9eac) SHA1(e6cd274b4a0f92b1fb1f013f80f6fd2db3212431) )
	ROM_LOAD( "elu46.bin", 0x2000, 0x2000, CRC(8479612b) SHA1(b5543a06928274bde0e1bdda0747d936feaff177) )
	ROM_LOAD( "elu47.bin", 0x4000, 0x2000, CRC(6a66f6b4) SHA1(2bee981870e61977565439c34568952043656cfa) )
	ROM_LOAD( "elu48.bin", 0x6000, 0x2000, CRC(36575106) SHA1(178e26e7d5c7f879bc55c2fb170f3bb47a709610) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "elu33.bin", 0x0000, 0x2000, CRC(e7506d96) SHA1(610ae25bd8db13b18b9e681e855ffa978043255b) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dleuro", 0, NO_DUMP )
ROM_END

ROM_START( dlital )		/* Italian Sidam version */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dlita45.bin", 0x0000, 0x2000, CRC(4d3a9eac) SHA1(e6cd274b4a0f92b1fb1f013f80f6fd2db3212431) )
	ROM_LOAD( "dlita46.bin", 0x2000, 0x2000, CRC(8479612b) SHA1(b5543a06928274bde0e1bdda0747d936feaff177) )
	ROM_LOAD( "dlita47.bin", 0x4000, 0x2000, CRC(6a66f6b4) SHA1(2bee981870e61977565439c34568952043656cfa) )
	ROM_LOAD( "dlita48.bin", 0x6000, 0x2000, CRC(36575106) SHA1(178e26e7d5c7f879bc55c2fb170f3bb47a709610) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dlita33.bin", 0x0000, 0x2000, CRC(e7506d96) SHA1(610ae25bd8db13b18b9e681e855ffa978043255b) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dleuro", 0, NO_DUMP )
ROM_END


ROM_START( spaceace )		/* revision A3 */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "sa_a3_u1.bin", 0x0000, 0x2000,  CRC(427522d0) SHA1(de4d5353af0be3e60afe1ed13d1d531c425cdb4d) )
	ROM_LOAD( "sa_a3_u2.bin", 0x2000, 0x2000,  CRC(18d0262d) SHA1(c3920e3cabfe2b2add51881e262f090c5018e508) )
	ROM_LOAD( "sa_a3_u3.bin", 0x4000, 0x2000,  CRC(4646832d) SHA1(9f1370b13cca9857b0ed13f58641ef4ba3c7326d) )
	ROM_LOAD( "sa_a3_u4.bin", 0x6000, 0x2000,  CRC(57db2a79) SHA1(5286905d9bde697845a98bd77f31f2a96a8874fc) )
	ROM_LOAD( "sa_a3_u5.bin", 0x8000, 0x2000,  CRC(85cbcdc4) SHA1(97e01e96c885ab7af4c3a3b586eb40374d54f12f) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "spaceace", 0, NO_DUMP )
ROM_END

ROM_START( spaceaa2 )		/* revision A2 */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "sa_a2_u1.bin", 0x0000, 0x2000,  CRC(71b39e27) SHA1(15a34eee9d541b186761a78b5c97449c7b496e4f) )
	ROM_LOAD( "sa_a2_u2.bin", 0x2000, 0x2000,  CRC(18d0262d) SHA1(c3920e3cabfe2b2add51881e262f090c5018e508) )
	ROM_LOAD( "sa_a2_u3.bin", 0x4000, 0x2000,  CRC(4646832d) SHA1(9f1370b13cca9857b0ed13f58641ef4ba3c7326d) )
	ROM_LOAD( "sa_a2_u4.bin", 0x6000, 0x2000,  CRC(57db2a79) SHA1(5286905d9bde697845a98bd77f31f2a96a8874fc) )
	ROM_LOAD( "sa_a2_u5.bin", 0x8000, 0x2000,  CRC(85cbcdc4) SHA1(97e01e96c885ab7af4c3a3b586eb40374d54f12f) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "spaceace", 0, NO_DUMP )
ROM_END

ROM_START( spaceaa )		/* revision A */
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "sa_a_u1.bin", 0x0000, 0x2000,  CRC(8eb1889e) SHA1(bfa2c5fc139c448b7b6b5c5757d4f2f74e610b85) )
	ROM_LOAD( "sa_a_u2.bin", 0x2000, 0x2000,  CRC(18d0262d) SHA1(c3920e3cabfe2b2add51881e262f090c5018e508) )
	ROM_LOAD( "sa_a_u3.bin", 0x4000, 0x2000,  CRC(4646832d) SHA1(9f1370b13cca9857b0ed13f58641ef4ba3c7326d) )
	ROM_LOAD( "sa_a_u4.bin", 0x6000, 0x2000,  CRC(57db2a79) SHA1(5286905d9bde697845a98bd77f31f2a96a8874fc) )
	ROM_LOAD( "sa_a_u5.bin", 0x8000, 0x2000,  CRC(85cbcdc4) SHA1(97e01e96c885ab7af4c3a3b586eb40374d54f12f) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "spaceace", 0, NO_DUMP )
ROM_END

ROM_START( saeuro )		/* Italian Sidam version */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sa_u45a.bin", 0x0000, 0x2000, CRC(41264d46) SHA1(3e0ecfb3249f857a29fe58a3853a55d31cbd63d6) )
	ROM_LOAD( "sa_u46a.bin", 0x2000, 0x2000, CRC(bc1c70cf) SHA1(cd6d2456ac2fbbfb86e1f31bd7cbd0cec0d31b45) )
	ROM_LOAD( "sa_u47a.bin", 0x4000, 0x2000, CRC(ff3f77c7) SHA1(d10ffd14ab9853cef8085c70aedfabea4059657e) )
	ROM_LOAD( "sa_u48a.bin", 0x6000, 0x2000, CRC(8c83ac81) SHA1(12818ee51ae8028a84bbbf3e43904b62942c76e3) )
	ROM_LOAD( "sa_u49a.bin", 0x6000, 0x2000, CRC(03b58fc3) SHA1(25e4c1df74e2d7cbb9e252e34f007bc1c9f015b2) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sa_u33a.bin", 0x0000, 0x2000, CRC(a8c14612) SHA1(dbcf90b929e714f328bdcb0d8cd7c9e7d08a8be7) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "saeuro", 0, NO_DUMP )
ROM_END




/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( pr7820 )
{
	laserdisc_type = LASERDISC_TYPE_PR7820;
}


static DRIVER_INIT( ldv1000 )
{
	laserdisc_type = LASERDISC_TYPE_LDV1000;
}


static DRIVER_INIT( variable )
{
	laserdisc_type = LASERDISC_TYPE_VARIABLE | LASERDISC_TYPE_LDV1000;
}


static DRIVER_INIT( 22vp932 )
{
	laserdisc_type = LASERDISC_TYPE_22VP932;
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAMEL( 1983, dlair,    0,        dlair,   dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. F2)", GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlairf,   dlair,    dlair,   dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. F)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlaire,   dlair,    dlair,   dlaire, variable, ROT0, "Cinematronics", "Dragon's Lair (US Rev. E)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlaird,   dlair,    dlair,   dlair,  ldv1000,  ROT0, "Cinematronics", "Dragon's Lair (US Rev. D, Pioneer LD-V1000)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlairc,   dlair,    dlair,   dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. C, Pioneer PR-7820)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlairb,   dlair,    dlair,   dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. B, Pioneer PR-7820)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlaira,   dlair,    dlair,   dlair,  pr7820,   ROT0, "Cinematronics", "Dragon's Lair (US Rev. A, Pioneer PR-7820)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dleuro,   dlair,    dleuro,  dlair,  22vp932,  ROT0, "Atari",         "Dragon's Lair (European)",  GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, dlital,   dlair,    dleuro,  dlair,  22vp932,  ROT0, "Sidam",         "Dragon's Lair (Italian)",  GAME_NOT_WORKING, layout_dlair )

GAMEL( 1983, spaceace, 0,        dlair,   dlaire, variable, ROT0, "Cinematronics", "Space Ace (US Rev. A3)", GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, spaceaa2, spaceace, dlair,   dlaire, variable, ROT0, "Cinematronics", "Space Ace (US Rev. A2)", GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, spaceaa,  spaceace, dlair,   dlaire, variable, ROT0, "Cinematronics", "Space Ace (US Rev. A)", GAME_NOT_WORKING, layout_dlair )
GAMEL( 1983, saeuro,   spaceace, dleuro,  dlair,  22vp932,  ROT0, "Atari",         "Space Ace (European)",  GAME_NOT_WORKING, layout_dlair )
