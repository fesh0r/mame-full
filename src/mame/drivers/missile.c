/***************************************************************************

    Atari Missile Command hardware

    Games supported:
        * Missile Command

    Known issues:
        * none at this time

****************************************************************************

    Horizontal sync chain:

        A J/K flip flop @ D6 counts the 1H line, and cascades into a
        4-bit binary counter @ D5, which counts the 2H,4H,8H,16H lines.
        This counter cascades into a 4-bit BCD decade counter @ E5
        which counts the 32H,64H,128H,HBLANK lines. The counter system
        rolls over after counting to 320.

        Pixel clock = 5MHz
        HBLANK ends at H = 0
        HBLANK begins at H = 256
        HSYNC begins at H = 260
        HSYNC ends at H = 288
        HTOTAL = 320

    Vertical sync chain:

        The HSYNC signal clocks a 4-bit binary counter @ A4, which counts
        the 1V,2V,4V,8V lines. This counter cascades into a second 4-bit
        binary counter @ B4 which counts the 16V,32V,64V,128V lines. The
        counter system rolls over after counting to 256.

        if not flipped (V counts up):
            VBLANK ends at V = 24
            VBLANK begins at V = 0
            VSYNC begins at V = 4
            VSYNC ends at V = 8
            VTOTAL = 256

        if flipped (V counts down):
            VBLANK ends at V = 0
            VBLANK begins at V = 24
            VSYNC begins at V = 20
            VSYNC ends at V = 16
            VTOTAL = 256

    Interrupts:

        /IRQ connected to Q on flip-flop @ F7, clocked by SYNC which
        indicates an instruction opcode fetch. Input to the flip-flop
        (D) comes from a second flip-flop @ F7, which is clocked by
        /16V or 16V depending on whether or not we are flipped. Input
        to this second flip-flop is 32V.

        if not flipped (V counts up):
            clock @   0 -> 32V = 0 -> /IRQ = 0
            clock @  32 -> 32V = 1 -> /IRQ = 1
            clock @  64 -> 32V = 0 -> /IRQ = 0
            clock @  96 -> 32V = 1 -> /IRQ = 1
            clock @ 128 -> 32V = 0 -> /IRQ = 0
            clock @ 160 -> 32V = 1 -> /IRQ = 1
            clock @ 192 -> 32V = 0 -> /IRQ = 0
            clock @ 224 -> 32V = 1 -> /IRQ = 1

        if flipped (V counts down):
            clock @ 208 -> 32V = 0 -> /IRQ = 0
            clock @ 176 -> 32V = 1 -> /IRQ = 1
            clock @ 144 -> 32V = 0 -> /IRQ = 0
            clock @ 112 -> 32V = 1 -> /IRQ = 1
            clock @  80 -> 32V = 0 -> /IRQ = 0
            clock @  48 -> 32V = 1 -> /IRQ = 1
            clock @  16 -> 32V = 0 -> /IRQ = 0
            clock @ 240 -> 32V = 1 -> /IRQ = 1

****************************************************************************

    CPU Clock:
      _   _   _   _   _   _   _   _   _   _   _   _   _   _
    _| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |  1H
        ___     ___     ___     ___     ___     ___     ___
    ___|   |___|   |___|   |___|   |___|   |___|   |___|   |  2H
            _______         _______         _______
    _______|       |_______|       |_______|       |_______|  4H
    _______     ___________     ___________     ___________
           |___|           |___|           |___|              /(4H & /2H)
    _         ___             ___             ___
     |_______|   |___________|   |___________|   |_________   /Q on FF @ A7
                ___             ___             ___
    ___________|   |___________|   |___________|   |_______   Q on FF @ B8

    When V < 224,
        ___     ___     ___     ___     ___     ___     ___
    ___|   |___|   |___|   |___|   |___|   |___|   |___|   |  Sigma-X = 2H

    When V >= 224,
                ___             ___             ___
    ___________|   |___________|   |___________|   |_______   Sigma-X = Q on FF @ B8

****************************************************************************

    Modified from original schematics...

    MISSILE COMMAND
    ---------------
    HEX      R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
    ---------+-----+------------------------+------------------------
    0000-01FF  R/W   D  D  D    D  D  D  D  D   512 bytes working ram

    0200-05FF  R/W   D  D  D    D  D  D  D  D   3rd color bit region
                                                of screen ram.
                                                Each bit of every odd byte is the low color
                                                bit for the bottom scanlines
                                                The schematics say that its for the bottom
                                                32 scanlines, although the code only accesses
                                                $401-$5FF for the bottom 8 scanlines...
                                                Pretty wild, huh?

    0600-063F  R/W   D  D  D    D  D  D  D  D   More working ram.

    0640-3FFF  R/W   D  D  D    D  D  D  D  D   2-color bit region of
                                                screen ram.
                                                Writes to 4 bytes each to effectively
                                                address $1900-$ffff.

    1900-FFFF  R/W   D  D                       2-color bit region of
                                                screen ram
                                                  Only accessed with
                                                   LDA ($ZZ,X) and
                                                   STA ($ZZ,X)
                                                  Those instructions take longer
                                                  than 5 cycles.

    ---------+-----+------------------------+------------------------
    4000-400F  R/W   D  D  D    D  D  D  D  D   POKEY ports.
    -----------------------------------------------------------------
    4008         R     D  D  D  D  D  D  D  D   Game Option switches
    -----------------------------------------------------------------
    4800         R     D                        Right coin
    4800         R        D                     Center coin
    4800         R           D                  Left coin
    4800         R              D               1 player start
    4800         R                 D            2 player start
    4800         R                    D         2nd player left fire(cocktail)
    4800         R                       D      2nd player center fire  "
    4800         R                          D   2nd player right fire   "
    ---------+-----+------------------------+------------------------
    4800         R                 D  D  D  D   Horiz trackball displacement
                                                        if ctrld=high.
    4800         R     D  D  D  D               Vert trackball displacement
                                                        if ctrld=high.
    ---------+-----+------------------------+------------------------
    4800         W     D                        Unused ??
    4800         W        D                     screen flip
    4800         W           D                  left coin counter
    4800         W              D               center coin counter
    4800         W                 D            right coin counter
    4800         W                    D         2 player start LED.
    4800         W                       D      1 player start LED.
    4800         W                          D   CTRLD, 0=read switches,
                                                        1= read trackball.
    ---------+-----+------------------------+------------------------
    4900         R     D                        VBLANK read
    4900         R        D                     Self test switch input.
    4900         R           D                  SLAM switch input.
    4900         R              D               Horiz trackball direction input.
    4900         R                 D            Vert trackball direction input.
    4900         R                    D         1st player left fire.
    4900         R                       D      1st player center fire.
    4900         R                          D   1st player right fire.
    ---------+-----+------------------------+------------------------
    4A00         R     D  D  D  D  D  D  D  D   Pricing Option switches.
    4B00-4B07  W                   D  D  D  D   Color RAM.
    4C00         W                              Watchdog.
    4D00         W                              Interrupt acknowledge.
    ---------+-----+------------------------+------------------------
    5000-7FFF  R       D  D  D  D  D  D  D  D   Program.
    ---------+-----+------------------------+------------------------


    MISSILE COMMAND SWITCH SETTINGS (Atari, 1980)
    ---------------------------------------------


    GAME OPTIONS:
    (8-position switch at R8)

    1   2   3   4   5   6   7   8   Meaning
    -------------------------------------------------------------------------
    Off Off                         Game starts with 7 cities
    On  On                          Game starts with 6 cities
    On  Off                         Game starts with 5 cities
    Off On                          Game starts with 4 cities
            On                      No bonus credit
            Off                     1 bonus credit for 4 successive coins
                On                  Large trak-ball input
                Off                 Mini Trak-ball input
                    On  Off Off     Bonus city every  8000 pts
                    On  On  On      Bonus city every 10000 pts
                    Off On  On      Bonus city every 12000 pts
                    On  Off On      Bonus city every 14000 pts
                    Off Off On      Bonus city every 15000 pts
                    On  On  Off     Bonus city every 18000 pts
                    Off On  Off     Bonus city every 20000 pts
                    Off Off Off     No bonus cities
                                On  Upright
                                Off Cocktail



    PRICING OPTIONS:
    (8-position switch at R10)

    1   2   3   4   5   6   7   8   Meaning
    -------------------------------------------------------------------------
    On  On                          1 coin 1 play
    Off On                          Free play
    On Off                          2 coins 1 play
    Off Off                         1 coin 2 plays
            On  On                  Right coin mech * 1
            Off On                  Right coin mech * 4
            On  Off                 Right coin mech * 5
            Off Off                 Right coin mech * 6
                    On              Center coin mech * 1
                    Off             Center coin mech * 2
                        On  On      English
                        Off On      French
                        On  Off     German
                        Off Off     Spanish
                                On  ( Unused )
                                Off ( Unused )

    -there are 2 different versions of the Super Missile Attack board.  It's not known if
    the roms are different.  The SMA manual mentions a set 3(035822-03E) that will work
    as well as set 2. Missile Command set 1 will not work with the SMA board. It would
        appear set 1 and set 2 as labeled by mame are reversed.

******************************************************************************************/

#include "driver.h"
#include "sound/pokey.h"


#define MASTER_CLOCK	(10000000)

#define PIXEL_CLOCK		(MASTER_CLOCK/2)
#define HTOTAL			(320)
#define VTOTAL			(256)



/*************************************
 *
 *  Globals
 *
 *************************************/

static const UINT8 *writeprom;

static mame_timer *irq_timer;
static mame_timer *cpu_timer;

static UINT8 irq_state;
static UINT8 ctrld;
static UINT8 flipscreen;
static UINT8 madsel_delay;
static UINT16 madsel_lastpc;



/*************************************
 *
 *  VBLANK and IRQ generation
 *
 *************************************/

INLINE int scanline_to_v(int scanline)
{
	/* since the vertical sync counter counts backwards when flipped,
        this function returns the current effective V value, given
        that cpu_getscanline() only counts forward */
	return flipscreen ? (256 - scanline) : scanline;
}


INLINE int v_to_scanline(int v)
{
	/* same as a above, but the opposite transformation */
	return flipscreen ? (256 - v) : v;
}


INLINE void schedule_next_irq(int curv)
{
	/* IRQ = /32V, clocked by /16V ^ flip */
	/* When not flipped, clocks on 0, 64, 128, 192 */
	/* When flipped, clocks on 16, 80, 144, 208 */
	if (flipscreen)
		curv = ((curv - 32) & 0xff) | 0x10;
	else
		curv = ((curv + 32) & 0xff) & ~0x10;

	/* next one at the start of this scanline */
	mame_timer_adjust(irq_timer, video_screen_get_time_until_pos(0, v_to_scanline(curv), 0), curv, time_zero);
}


static void clock_irq(int curv)
{
	/* assert the IRQ if not already asserted */
	irq_state = (~curv >> 5) & 1;
	cpunum_set_input_line(0, 0, irq_state ? ASSERT_LINE : CLEAR_LINE);

	/* force an update while we're here */
	video_screen_update_partial(0, v_to_scanline(curv));

	/* find the next edge */
	schedule_next_irq(curv);
}


static UINT32 get_vblank(void *param)
{
	int v = scanline_to_v(video_screen_get_vpos(0));
	return v < 24;
}



/*************************************
 *
 *  Machine setup
 *
 *************************************/

static void adjust_cpu_speed(int curv)
{
	/* starting at scanline 224, the CPU runs at half speed */
	if (curv == 224)
		cpunum_set_clock(0, MASTER_CLOCK/16);
	else
		cpunum_set_clock(0, MASTER_CLOCK/8);

	/* scanline for the next run */
	curv ^= 224;
	mame_timer_adjust(cpu_timer, video_screen_get_time_until_pos(0, v_to_scanline(curv), 0), curv, time_zero);
}


static offs_t missile_opbase_handler(offs_t address)
{
	/* offset accounts for lack of A15 decoding */
	int offset = address & 0x8000;
	address &= 0x7fff;

	/* RAM? */
	if (address < 0x4000)
	{
		opcode_base = opcode_arg_base = videoram - offset;
		return ~0;
	}

	/* ROM? */
	else if (address >= 0x5000)
	{
		opcode_base = opcode_arg_base = memory_region(REGION_CPU1) - offset;
		return ~0;
	}

	/* anything else falls through */
	return address;
}


static MACHINE_START( missile )
{
	/* initialize globals */
	writeprom = memory_region(REGION_PROMS);
	flipscreen = 0;

	/* set up an opcode base handler since we use mapped handlers for RAM */
	memory_set_opbase_handler(0, missile_opbase_handler);
	opcode_base = opcode_arg_base = videoram;

	/* create a timer to speed/slow the CPU */
	cpu_timer = timer_alloc(adjust_cpu_speed);
	mame_timer_adjust(cpu_timer, video_screen_get_time_until_pos(0, v_to_scanline(0), 0), 0, time_zero);

	/* create a timer for IRQs and set up the first callback */
	irq_timer = timer_alloc(clock_irq);
	irq_state = 0;
	schedule_next_irq(-32);

	/* setup for save states */
	state_save_register_global(irq_state);
	state_save_register_global(ctrld);
	state_save_register_global(flipscreen);
	state_save_register_global(madsel_delay);
	state_save_register_global(madsel_lastpc);

	return 0;
}


static MACHINE_RESET( missile )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	irq_state = 0;
}



/*************************************
 *
 *  VRAM access override
 *
 *************************************/

INLINE int get_madsel(void)
{
	UINT16 pc = activecpu_get_previouspc();

	/* if we're at a different instruction than last time, reset our delay counter */
	if (pc != madsel_lastpc)
		madsel_delay = 0;

	/* MADSEL signal disables standard address decoding and routes
        writes to video RAM; it is enabled if the IRQ signal is clear
        and the low 5 bits of the fetched opcode are 0x01 */
	if (!irq_state && (cpu_readop(pc) & 0x1f) == 0x01)
	{
		/* the MADSEL signal goes high 5 cycles after the opcode is identified;
            this effectively skips the indirect memory read. Since this is difficult
            to do in MAME, we just ignore the first two positive hits on MADSEL
            and only return TRUE on the third or later */
		madsel_lastpc = pc;
		return (++madsel_delay >= 3);
	}
	madsel_delay = 0;
	return 0;
}


INLINE offs_t get_bit3_addr(offs_t pixaddr)
{
	/* the 3rd bit of video RAM is scattered about various areas
        we take a 16-bit pixel address here and convert it into
        a video RAM address based on logic in the schematics */
	return 	(( pixaddr & 0x0800) >> 1) |
			((~pixaddr & 0x0800) >> 2) |
			(( pixaddr & 0x07f8) >> 2) |
			(( pixaddr & 0x1000) >> 12);
}


static void write_vram(offs_t address, UINT8 data)
{
	static const UINT8 data_lookup[4] = { 0x00, 0x0f, 0xf0, 0xff };
	offs_t vramaddr;
	UINT8 vramdata;
	UINT8 vrammask;

	/* basic 2 bit VRAM writes go to addr >> 2 */
	/* data comes from bits 6 and 7 */
	/* this should only be called if MADSEL == 1 */
	vramaddr = address >> 2;
	vramdata = data_lookup[data >> 6];
	vrammask = writeprom[(address & 7) | 0x10];
	videoram[vramaddr] = (videoram[vramaddr] & vrammask) | (vramdata & ~vrammask);

	/* 3-bit VRAM writes use an extra clock to write the 3rd bit elsewhere */
	/* on the schematics, this is the MUSHROOM == 1 case */
	if ((address & 0xe000) == 0xe000)
	{
		vramaddr = get_bit3_addr(address);
		vramdata = -((data >> 5) & 1);
		vrammask = writeprom[(address & 7) | 0x18];
		videoram[vramaddr] = (videoram[vramaddr] & vrammask) | (vramdata & ~vrammask);

		/* account for the extra clock cycle */
		activecpu_adjust_icount(-1);
	}
}


static UINT8 read_vram(offs_t address)
{
	offs_t vramaddr;
	UINT8 vramdata;
	UINT8 vrammask;
	UINT8 result = 0xff;

	/* basic 2 bit VRAM reads go to addr >> 2 */
	/* data goes to bits 6 and 7 */
	/* this should only be called if MADSEL == 1 */
	vramaddr = address >> 2;
	vrammask = 0x11 << (address & 3);
	vramdata = videoram[vramaddr] & vrammask;
	if ((vramdata & 0xf0) == 0)
		result &= ~0x80;
	if ((vramdata & 0x0f) == 0)
		result &= ~0x40;

	/* 3-bit VRAM reads use an extra clock to read the 3rd bit elsewhere */
	/* on the schematics, this is the MUSHROOM == 1 case */
	if ((address & 0xe000) == 0xe000)
	{
		vramaddr = get_bit3_addr(address);
		vrammask = 1 << (address & 7);
		vramdata = videoram[vramaddr] & vrammask;
		if (vramdata == 0)
			result &= ~0x20;

		/* account for the extra clock cycle */
		activecpu_adjust_icount(-1);
	}
	return result;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( missile )
{
	pen_t black = get_black_pen(machine);
	int x, y;

	/* draw the bitmap to the screen, looping over Y */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dst = (UINT16 *)bitmap->base + y * bitmap->rowpixels;

		/* if we're in the VBLANK region, just fill with black */
		if (y < 24)
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				dst[x] = black;
		}

		/* non-VBLANK region: draw the bitmap */
		else
		{
			int effy = flipscreen ? ((256+24 - y) & 0xff) : y;
			UINT8 *src = &videoram[effy * 64];
			UINT8 *src3 = NULL;

			/* compute the base of the 3rd pixel row */
			if (effy >= 224)
				src3 = &videoram[get_bit3_addr(effy << 8)];

			/* loop over X */
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				/* if we're in the HBLANK region, just store black */
				if (x >= 256)
					dst[x] = black;

				/* otherwise, process normally */
				else
				{
					UINT8 pix = src[x / 4] >> (x & 3);
					pix = ((pix >> 2) & 4) | ((pix << 1) & 2);

					/* if we're in the lower region, get the 3rd bit */
					if (src3 != NULL)
						pix |= (src3[(x / 8) * 2] >> (x & 7)) & 1;

					dst[x] = pix;
				}
			}
		}
	}
	return 0;
}



/*************************************
 *
 *  Global read/write handlers
 *
 *************************************/

static WRITE8_HANDLER( missile_w )
{
	/* if we're in MADSEL mode, write to video RAM */
	if (get_madsel())
	{
		write_vram(offset, data);
		return;
	}

	/* otherwise, strip A15 and handle manually */
	offset &= 0x7fff;

	/* RAM */
	if (offset < 0x4000)
		videoram[offset] = data;

	/* POKEY */
	else if (offset < 0x4800)
		pokey1_w(offset & 0x0f, data);

	/* OUT0 */
	else if (offset < 0x4900)
	{
		flipscreen = ~data & 0x40;
		coin_counter_w(0, data & 0x20);
		coin_counter_w(1, data & 0x10);
		coin_counter_w(2, data & 0x08);
		set_led_status(1, ~data & 0x04);
		set_led_status(0, ~data & 0x02);
		ctrld = data & 1;
	}

	/* color RAM */
	else if (offset >= 0x4b00 && offset < 0x4c00)
		palette_set_color(Machine, offset & 7, pal1bit(~data >> 3), pal1bit(~data >> 2), pal1bit(~data >> 1));

	/* watchdog */
	else if (offset >= 0x4c00 && offset < 0x4d00)
		watchdog_reset();

	/* interrupt ack */
	else if (offset >= 0x4d00 && offset < 0x4e00)
	{
		if (irq_state)
		{
			cpunum_set_input_line(0, 0, CLEAR_LINE);
			irq_state = 0;
		}
	}

	/* anything else */
	else
		logerror("%04X:Unknown write to %04X = %02X\n", activecpu_get_pc(), offset, data);
}


static READ8_HANDLER( missile_r )
{
	UINT8 result = 0xff;

	/* if we're in MADSEL mode, read from video RAM */
	if (get_madsel())
		return read_vram(offset);

	/* otherwise, strip A15 and handle manually */
	offset &= 0x7fff;

	/* RAM */
	if (offset < 0x4000)
		result = videoram[offset];

	/* ROM */
	else if (offset >= 0x5000)
		result = memory_region(REGION_CPU1)[offset];

	/* POKEY */
	else if (offset < 0x4800)
		result = pokey1_r(offset & 0x0f);

	/* IN0 */
	else if (offset < 0x4900)
	{
		if (ctrld)	/* trackball */
		{
			if (!flipscreen)
		  	    result = ((readinputport(5) << 4) & 0xf0) | (readinputport(4) & 0x0f);
			else
		  	    result = ((readinputport(7) << 4) & 0xf0) | (readinputport(6) & 0x0f);
		}
		else	/* buttons */
			result = readinputport(0);
	}

	/* IN1 */
	else if (offset < 0x4a00)
		result = readinputport(1);

	/* IN2 */
	else if (offset < 0x4b00)
		result = readinputport(2);

	/* anything else */
	else
		logerror("%04X:Unknown read from %04X\n", activecpu_get_pc(), offset);
	return result;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

/* complete memory map derived from schematics (implemented above) */
static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(missile_r, missile_w) AM_BASE(&videoram)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( missile )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x18, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_vblank, 0)

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("R10:1,2")
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, "Right Coin" ) PORT_DIPLOCATION("R10:3,4")
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x04, "*4" )
	PORT_DIPSETTING(    0x08, "*5" )
	PORT_DIPSETTING(    0x0c, "*6" )
	PORT_DIPNAME( 0x10, 0x00, "Center Coin" ) PORT_DIPLOCATION("R10:5")
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x10, "*2" )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Language ) ) PORT_DIPLOCATION("R10:6,7")
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x20, DEF_STR( French ) )
	PORT_DIPSETTING(    0x40, DEF_STR( German ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Spanish ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("R10:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x00, "Cities" ) PORT_DIPLOCATION("R8:1,2")
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Credit for 4 Coins" ) PORT_DIPLOCATION("R8:3")
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Trackball Size" ) PORT_DIPLOCATION("R8:4")
	PORT_DIPSETTING(    0x00, "Large" )
	PORT_DIPSETTING(    0x08, "Mini" )
	PORT_DIPNAME( 0x70, 0x70, "Bonus City" ) PORT_DIPLOCATION("R8:5,6,7")
	PORT_DIPSETTING(    0x10, "8000" )
	PORT_DIPSETTING(    0x70, "10000" )
	PORT_DIPSETTING(    0x60, "12000" )
	PORT_DIPSETTING(    0x50, "14000" )
	PORT_DIPSETTING(    0x40, "15000" )
	PORT_DIPSETTING(    0x30, "18000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("R8:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10)

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END


INPUT_PORTS_START( suprmatk )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x18, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_vblank, 0)

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("R10:1,2")
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, "Right Coin" ) PORT_DIPLOCATION("R10:3,4")
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x04, "*4" )
	PORT_DIPSETTING(    0x08, "*5" )
	PORT_DIPSETTING(    0x0c, "*6" )
	PORT_DIPNAME( 0x10, 0x00, "Center Coin" ) PORT_DIPLOCATION("R10:5")
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x10, "*2" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("R10:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x40, "Game" ) PORT_DIPLOCATION("R10:7,8")
	PORT_DIPSETTING(    0x00, "Missile Command" )
	PORT_DIPSETTING(    0x40, "Easy Super Missile Attack" )
	PORT_DIPSETTING(    0x80, "Reg. Super Missile Attack" )
	PORT_DIPSETTING(    0xc0, "Hard Super Missile Attack" )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x00, "Cities" ) PORT_DIPLOCATION("R8:1,2")
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Credit for 4 Coins" ) PORT_DIPLOCATION("R8:3")
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Trackball Size" ) PORT_DIPLOCATION("R8:4")
	PORT_DIPSETTING(    0x00, "Large" )
	PORT_DIPSETTING(    0x08, "Mini" )
	PORT_DIPNAME( 0x70, 0x70, "Bonus City" ) PORT_DIPLOCATION("R8:5,6,7")
	PORT_DIPSETTING(    0x10, "8000" )
	PORT_DIPSETTING(    0x70, "10000" )
	PORT_DIPSETTING(    0x60, "12000" )
	PORT_DIPSETTING(    0x50, "14000" )
	PORT_DIPSETTING(    0x40, "15000" )
	PORT_DIPSETTING(    0x30, "18000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("R8:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10)

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL

	PORT_START	/* FAKE */
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL
INPUT_PORTS_END



/*************************************
 *
 *  Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	{ 0 },
	input_port_3_r
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( missile )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, MASTER_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(main_map,0)

	MDRV_MACHINE_START(missile)
	MDRV_MACHINE_RESET(missile)
	MDRV_WATCHDOG_VBLANK_INIT(8)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_PALETTE_LENGTH(8)

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE((float)PIXEL_CLOCK / (float)VTOTAL / (float)HTOTAL)
	MDRV_SCREEN_SIZE(HTOTAL, VTOTAL)
	MDRV_SCREEN_VBLANK_TIME(0)			/* VBLANK is handled manually */
	MDRV_SCREEN_VISIBLE_AREA(0, 255, 25, 255)

	MDRV_VIDEO_UPDATE(missile)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(POKEY, MASTER_CLOCK/8)
	MDRV_SOUND_CONFIG(pokey_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( missile )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "035820.02",    0x5000, 0x0800, CRC(7a62ce6a) SHA1(9a39978138dc28fdefe193bfae1b226391e471db) )
	ROM_LOAD( "035821.02",    0x5800, 0x0800, CRC(df3bd57f) SHA1(0916925d3c94d766d33f0e4badf6b0add835d748) )
	ROM_LOAD( "035822.02",    0x6000, 0x0800, CRC(a1cd384a) SHA1(a1dd0953423750a0fbc6e3dccbf2ca64ef5a1f54) )
	ROM_LOAD( "035823.02",    0x6800, 0x0800, CRC(82e552bb) SHA1(d0f22894f779c74ceef644c9f03d840d9545efea) )
	ROM_LOAD( "035824.02",    0x7000, 0x0800, CRC(606e42e0) SHA1(9718f84a73c66b4e8ef7805a7ab638a7380624e1) )
	ROM_LOAD( "035825.02",    0x7800, 0x0800, CRC(f752eaeb) SHA1(0339a6ce6744d2091cc7e07675e509b202b0f380) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "035826.01",   0x0000, 0x0020, CRC(86a22140) SHA1(2beebf7855e29849ada1823eae031fc98220bc43) )
ROM_END


ROM_START( missile2 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "35820-01.h1",  0x5000, 0x0800, CRC(41cbb8f2) SHA1(5dcb58276c08d75d36baadb6cefe30d4916de9b0) )
	ROM_LOAD( "35821-01.jk1", 0x5800, 0x0800, CRC(728702c8) SHA1(6f25af7133d3ec79029117162649f94e93f36e0e) )
	ROM_LOAD( "35822-01.kl1", 0x6000, 0x0800, CRC(28f0999f) SHA1(eb52b11c6757c8dc3be88b276ea4dc7dfebf7cf7) )
	ROM_LOAD( "35823-01.mn1", 0x6800, 0x0800, CRC(bcc93c94) SHA1(f0daa5d2835a856e2038612e755dc7ded28fc923) )
	ROM_LOAD( "35824-01.np1", 0x7000, 0x0800, CRC(0ca089c8) SHA1(7f69ee990fd4fa1f2fceca7fc66fcaa02e4d2314) )
	ROM_LOAD( "35825-01.r1",  0x7800, 0x0800, CRC(428cf0d5) SHA1(03cabbef50c33852fbbf38dd3eecaf70a82df82f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "035826.01",   0x0000, 0x0020, CRC(86a22140) SHA1(2beebf7855e29849ada1823eae031fc98220bc43) )
ROM_END


ROM_START( suprmatk )
	ROM_REGION( 0x9000, REGION_CPU1, 0 )
	ROM_LOAD( "035820.02",    0x5000, 0x0800, CRC(7a62ce6a) SHA1(9a39978138dc28fdefe193bfae1b226391e471db) )
	ROM_LOAD( "035821.02",    0x5800, 0x0800, CRC(df3bd57f) SHA1(0916925d3c94d766d33f0e4badf6b0add835d748) )
	ROM_LOAD( "035822.02",    0x6000, 0x0800, CRC(a1cd384a) SHA1(a1dd0953423750a0fbc6e3dccbf2ca64ef5a1f54) )
	ROM_LOAD( "035823.02",    0x6800, 0x0800, CRC(82e552bb) SHA1(d0f22894f779c74ceef644c9f03d840d9545efea) )
	ROM_LOAD( "035824.02",    0x7000, 0x0800, CRC(606e42e0) SHA1(9718f84a73c66b4e8ef7805a7ab638a7380624e1) )
	ROM_LOAD( "035825.02",    0x7800, 0x0800, CRC(f752eaeb) SHA1(0339a6ce6744d2091cc7e07675e509b202b0f380) )
	ROM_LOAD( "e0.rom",       0x8000, 0x0800, CRC(d0b20179) SHA1(e2a9855899b6ff96b8dba169e0ab83f00a95919f) )
	ROM_LOAD( "e1.rom",       0x8800, 0x0800, CRC(c6c818a3) SHA1(b9c92a85c07dd343d990e196d37b92d92a85a5e0) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "035826.01",   0x0000, 0x0020, CRC(86a22140) SHA1(2beebf7855e29849ada1823eae031fc98220bc43) )
ROM_END


ROM_START( sprmatkd )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "035820.sma",   0x5000, 0x0800, CRC(75f01b87) SHA1(32ed71b6a869d7b361f244c384bbe6f407f6c6d7) )
	ROM_LOAD( "035821.sma",   0x5800, 0x0800, CRC(3320d67e) SHA1(5bb04b985421af6309818b94676298f4b90495cf) )
	ROM_LOAD( "035822.sma",   0x6000, 0x0800, CRC(e6be5055) SHA1(43912cc565cb43256a9193594cf36abab1c85d6f) )
	ROM_LOAD( "035823.sma",   0x6800, 0x0800, CRC(a6069185) SHA1(899cd8b378802eb6253d4bca7432797168595d53) )
	ROM_LOAD( "035824.sma",   0x7000, 0x0800, CRC(90a06be8) SHA1(f46fd6847bc9836d11ea0042df19fbf33ddab0db) )
	ROM_LOAD( "035825.sma",   0x7800, 0x0800, CRC(1298213d) SHA1(c8e4301704e3700c339557f2a833e70f6a068d5e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "035826.01",   0x0000, 0x0020, CRC(86a22140) SHA1(2beebf7855e29849ada1823eae031fc98220bc43) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( suprmatk )
{
	int i;
	UINT8 *rom = memory_region(REGION_CPU1);

	for (i = 0; i < 0x40; i++)
	{
        rom[0x7CC0+i] = rom[0x8000+i];
        rom[0x5440+i] = rom[0x8040+i];
        rom[0x5B00+i] = rom[0x8080+i];
        rom[0x5740+i] = rom[0x80C0+i];
        rom[0x6000+i] = rom[0x8100+i];
        rom[0x6540+i] = rom[0x8140+i];
        rom[0x7500+i] = rom[0x8180+i];
        rom[0x7100+i] = rom[0x81C0+i];
        rom[0x7800+i] = rom[0x8200+i];
        rom[0x5580+i] = rom[0x8240+i];
        rom[0x5380+i] = rom[0x8280+i];
        rom[0x6900+i] = rom[0x82C0+i];
        rom[0x6E00+i] = rom[0x8300+i];
        rom[0x6CC0+i] = rom[0x8340+i];
        rom[0x7DC0+i] = rom[0x8380+i];
        rom[0x5B80+i] = rom[0x83C0+i];
        rom[0x5000+i] = rom[0x8400+i];
        rom[0x7240+i] = rom[0x8440+i];
        rom[0x7040+i] = rom[0x8480+i];
        rom[0x62C0+i] = rom[0x84C0+i];
        rom[0x6840+i] = rom[0x8500+i];
        rom[0x7EC0+i] = rom[0x8540+i];
        rom[0x7D40+i] = rom[0x8580+i];
        rom[0x66C0+i] = rom[0x85C0+i];
        rom[0x72C0+i] = rom[0x8600+i];
        rom[0x7080+i] = rom[0x8640+i];
        rom[0x7D00+i] = rom[0x8680+i];
        rom[0x5F00+i] = rom[0x86C0+i];
        rom[0x55C0+i] = rom[0x8700+i];
        rom[0x5A80+i] = rom[0x8740+i];
        rom[0x6080+i] = rom[0x8780+i];
        rom[0x7140+i] = rom[0x87C0+i];
        rom[0x7000+i] = rom[0x8800+i];
        rom[0x6100+i] = rom[0x8840+i];
        rom[0x5400+i] = rom[0x8880+i];
        rom[0x5BC0+i] = rom[0x88C0+i];
        rom[0x7E00+i] = rom[0x8900+i];
        rom[0x71C0+i] = rom[0x8940+i];
        rom[0x6040+i] = rom[0x8980+i];
        rom[0x6E40+i] = rom[0x89C0+i];
        rom[0x5800+i] = rom[0x8A00+i];
        rom[0x7D80+i] = rom[0x8A40+i];
        rom[0x7A80+i] = rom[0x8A80+i];
        rom[0x53C0+i] = rom[0x8AC0+i];
        rom[0x6140+i] = rom[0x8B00+i];
        rom[0x6700+i] = rom[0x8B40+i];
        rom[0x7280+i] = rom[0x8B80+i];
        rom[0x7F00+i] = rom[0x8BC0+i];
        rom[0x5480+i] = rom[0x8C00+i];
        rom[0x70C0+i] = rom[0x8C40+i];
        rom[0x7F80+i] = rom[0x8C80+i];
        rom[0x5780+i] = rom[0x8CC0+i];
        rom[0x6680+i] = rom[0x8D00+i];
        rom[0x7200+i] = rom[0x8D40+i];
        rom[0x7E40+i] = rom[0x8D80+i];
        rom[0x7AC0+i] = rom[0x8DC0+i];
        rom[0x6300+i] = rom[0x8E00+i];
        rom[0x7180+i] = rom[0x8E40+i];
        rom[0x7E80+i] = rom[0x8E80+i];
        rom[0x6280+i] = rom[0x8EC0+i];
        rom[0x7F40+i] = rom[0x8F00+i];
        rom[0x6740+i] = rom[0x8F40+i];
        rom[0x74C0+i] = rom[0x8F80+i];
        rom[0x7FC0+i] = rom[0x8FC0+i];
	}
}


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1980, missile,  0,       missile, missile,         0, ROT0, "Atari", "Missile Command (set 1)", GAME_SUPPORTS_SAVE )
GAME( 1980, missile2, missile, missile, missile,         0, ROT0, "Atari", "Missile Command (set 2)", GAME_SUPPORTS_SAVE )
GAME( 1981, suprmatk, missile, missile, suprmatk, suprmatk, ROT0, "Atari + Gencomp", "Super Missile Attack (for set 2)", GAME_SUPPORTS_SAVE )
GAME( 1981, sprmatkd, missile, missile, suprmatk,        0, ROT0, "Atari + Gencomp", "Super Missile Attack (not encrypted)", GAME_SUPPORTS_SAVE )
