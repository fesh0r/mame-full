/*
 *	Beatmania DJ Main Board (GX753)
 *
 *	Product numbers:
 *	GQ753 beatmania (first release in 1997)
 *	Gx853 beatmania 2nd MIX (1998)
 *	Gx825 beatmania 3rd MIX
 *	Gx858 beatmania complete MIX (1999)
 *	Gx847 beatmania 4th MIX
 *	Gx981 beatmania 5th MIX
 *	Gx993 beatmania Club MIX (2000)
 *	Gx988 beatmania complete MIX 2
 *	GxA05 beatmania CORE REMIX
 *	GxA21 beatmania 6th MIX (2001)
 *	GxB07 beatmania 7th MIX
 *	GxC01 beatmania THE FINAL (2002)
 *
 *	Gx803 Pop'n Music 1 (1998)
 *	Gx831 Pop'n Music 2
 *	Gx980 Pop'n Music 3 (1999)
 *
 *	Gx970 Pop'n Stage EX (1999)
 *
 *	Chips:
 *	15a:	MC68EC020FG25
 *	25b:	001642
 *	18d:	055555 (priority encoder)
 *	 5f:	056766 (sprites)
 *	18f:	056832 (tiles)
 *	22f:	058143 = 054156 (tiles)
 *	12j:	058141 = 054539 (x2) (2 sound chips in one)
 *
 *	TODO:
 *	- correct FPS
 *	- volum control
 *	- bmcorerm: fix sprite position of game option menu.
 *	- popn3: fix that some songs are unplayable.
 *
 */

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "machine/idectrl.h"
#include "sound/k054539.h"
#include "vidhrdw/konamiic.h"

// light LED as neon instead of 1P/2P/Effect buttons.
#define LED_AS_NEON


extern data32_t *djmain_obj_ram;

VIDEO_UPDATE( djmain );
VIDEO_START( djmain );


static int sndram_bank;
static data8_t *sndram;

static int scratch_select;
static data8_t scratch_data[2];

static int pending_vb_int;
static data16_t v_ctrl;
static data32_t obj_regs[0xa0/4];

#define DISABLE_VB_INT	(!(v_ctrl & 0x8000))


/*************************************
 *
 *	68k CPU memory handlers
 *
 *************************************/

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

 	r = (data >>  0) & 0xff;
	g = (data >>  8) & 0xff;
	b = (data >> 16) & 0xff;

	palette_set_color(offset, r, g, b);
}


//---------

static void sndram_set_bank(void)
{
	sndram = memory_region(REGION_SOUND1) + 0x80000 * sndram_bank;
}

static WRITE32_HANDLER( sndram_bank_w )
{
	if (ACCESSING_MSW32)
	{
		sndram_bank = (data >> 16) & 0x1f;
		sndram_set_bank();
	}
}

static READ32_HANDLER( sndram_r )
{
	data32_t data = 0;

	if ((mem_mask & 0xff000000) == 0)
		data |= sndram[offset * 4] << 24;

	if ((mem_mask & 0x00ff0000) == 0)
		data |= sndram[offset * 4 + 1] << 16;

	if ((mem_mask & 0x0000ff00) == 0)
		data |= sndram[offset * 4 + 2] << 8;

	if ((mem_mask & 0x000000ff) == 0)
		data |= sndram[offset * 4 + 3];

	return data;
}

static WRITE32_HANDLER( sndram_w )
{
	if ((mem_mask & 0xff000000) == 0)
		sndram[offset * 4] = data >> 24;

	if ((mem_mask & 0x00ff0000) == 0)
		sndram[offset * 4 + 1] = data >> 16;

	if ((mem_mask & 0x0000ff00) == 0)
		sndram[offset * 4 + 2] = data >> 8;

	if ((mem_mask & 0x000000ff) == 0)
		sndram[offset * 4 + 3] = data;
}


//---------

static READ16_HANDLER( dual539_16_r )
{
	data16_t ret = 0;

	if (ACCESSING_LSB16)
		ret |= K054539_1_r(offset);
	if (ACCESSING_MSB16)
		ret |= K054539_0_r(offset)<<8;

	return ret;
}

static WRITE16_HANDLER( dual539_16_w )
{
	if (ACCESSING_LSB16)
		K054539_1_w(offset, data);
	if (ACCESSING_MSB16)
		K054539_0_w(offset, data>>8);
}

static READ32_HANDLER( dual539_r )
{
	data32_t data = 0;

	if (~mem_mask & 0xffff0000)
		data |= dual539_16_r(offset * 2, mem_mask >> 16) << 16;
	if (~mem_mask & 0x0000ffff)
		data |= dual539_16_r(offset * 2 + 1, mem_mask);

	return data;
}

static WRITE32_HANDLER( dual539_w )
{
	if (~mem_mask & 0xffff0000)
		dual539_16_w(offset * 2, data >> 16, mem_mask >> 16);
	if (~mem_mask & 0x0000ffff)
		dual539_16_w(offset * 2 + 1, data, mem_mask);
}


//---------

static READ32_HANDLER( obj_ctrl_r )
{
	// read obj_regs[0x0c/4]: unknown
	// read obj_regs[0x24/4]: unknown

	return obj_regs[offset];
}

static WRITE32_HANDLER( obj_ctrl_w )
{
	// write obj_regs[0x28/4]: bank for rom readthrough

	COMBINE_DATA(&obj_regs[offset]);
}

static READ32_HANDLER( obj_rom_r )
{
	data8_t *mem8 = memory_region(REGION_GFX1);
	int bank = obj_regs[0x28/4] >> 16;

	offset += bank * 0x200;
	offset *= 4;

	if (~mem_mask & 0x0000ffff)
		offset += 2;

	if (~mem_mask & 0xff00ff00)
		offset++;

	return mem8[offset] * 0x01010101;
}


//---------

static WRITE32_HANDLER( v_ctrl_w )
{
	if (ACCESSING_MSW32)
	{
		data >>= 16;
		mem_mask >>= 16;
		COMBINE_DATA(&v_ctrl);

		if (pending_vb_int && !DISABLE_VB_INT)
		{
			pending_vb_int = 0;
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
		}
	}
}

static READ32_HANDLER( v_rom_r )
{
	data8_t *mem8 = memory_region(REGION_GFX2);
	int bank = K056832_word_r(0x34/2, 0xffff);

	offset *= 2;

	if (!ACCESSING_MSB32)
		offset += 1;

	offset += bank * 0x800 * 4;

	if (v_ctrl & 0x020)
		offset += 0x800 * 2;

	return mem8[offset] * 0x01010000;
}


//---------

static READ32_HANDLER( inp1_r )
{
	data32_t result = (input_port_5_r(0)<<24) | (input_port_2_r(0)<<16) | (input_port_1_r(0)<<8) | input_port_0_r(0);

	return result;
}

static READ32_HANDLER( inp2_r )
{
	return (input_port_3_r(0)<<24) | (input_port_4_r(0)<<16) | 0xffff;
}

static READ32_HANDLER( scratch_r )
{
	data32_t result = 0;

	if (!(mem_mask & 0x0000ff00))
	{
		if (input_port_6_r(0) & (1 << scratch_select))
			scratch_data[scratch_select]++;
		if (input_port_6_r(0) & (4 << scratch_select))
			scratch_data[scratch_select]--;

		result |= scratch_data[scratch_select] << 8;
	}

	return result;
}

static WRITE32_HANDLER( scratch_w )
{
	if (!(mem_mask & 0x00ff0000))
		scratch_select = (data >> 19) & 1;
}


//---------

#define IDE_STD_OFFSET	(0x1f0/2)
#define IDE_ALT_OFFSET	(0x3f6/2)

static READ32_HANDLER( ide_std_r )
{
	if (ACCESSING_LSB32)
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x00ff) >> 8;
	else
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x0000) << 16;
}

static WRITE32_HANDLER( ide_std_w )
{
	if (ACCESSING_LSB32)
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data << 8, 0x00ff);
	else
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data >> 16, 0x0000);
}


static READ32_HANDLER( ide_alt_r )
{
	if (offset == 0)
		return ide_controller16_0_r(IDE_ALT_OFFSET, 0xff00) << 24;

	return 0;
}

static WRITE32_HANDLER( ide_alt_w )
{
	if (offset == 0 && !(mem_mask & 0x00ff0000))
		ide_controller16_0_w(IDE_ALT_OFFSET, data >> 24, 0xff00);
}


//---------

// light/coin blocker control

// 0x5d0000 (MSW16):		active
// bit 0: 1P button 1 LED	high
//     1: 1P button 2 LED	high
//     2: 1P button 3 LED	high
//     3: 1P button 4 LED	high
//     4: 1P button 5 LED	high
//     5: R blue HIGHLIGHT	low
//     6: 2P button 1 LED	high
//     7: 2P button 2 LED	high
//     8: 2P button 3 LED	high
//     9: L blue HIGHLIGHT	low
//    10: L red HIGHLIGHT	low
//    11: R red HIGHLIGHT	low
// 12-15: not used? (always low)

// 0x5d2000 (MSW16):		active
//     0: 1P START button LED	high
//     1: 2P START button LED	high
//     2: EFFECT button LED	high
//  3-10: not used? (always low)
//    11: SSR			high
//    12: 2P button 4 LED	high
//    13: 2P button 5 LED	high
//    14: COIN BLOCKER		high
//    15: not used? (always low)

static WRITE32_HANDLER( light_ctrl_1_w )
{
	//logerror("%08X: light_ctrl_1 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);

#ifdef LED_AS_NEON
	if (ACCESSING_MSW32)
	{
		set_led_status(0, !(data & 0x02000000));	// L blue HIGHLIGHT
		set_led_status(2, !(data & 0x00200000));	// R blue HIGHLIGHT
	}
#endif /* LED_AS_NEON */
}

static WRITE32_HANDLER( light_ctrl_2_w )
{
	//logerror("%08X: light_ctrl_2 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);

	if (ACCESSING_MSW32)
	{
#ifdef LED_AS_NEON
		set_led_status(1, data & 0x08000000);		// SSR
#else /* LED_AS_NEON */
		set_led_status(0, data & 0x00010000);		// 1P START
		set_led_status(1, data & 0x00020000);		// 2P START
		set_led_status(2, data & 0x00040000);		// EFFECT
#endif /* LED_AS_NEON */
	}
}


//---------

// unknown ports :-(

static WRITE32_HANDLER( unknown590000_w )
{
	//logerror("%08X: unknown 590000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknown802000_w )
{
	//logerror("%08X: unknown 802000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknownc02000_w )
{
	//logerror("%08X: unknown c02000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *	Interrupt handlers
 *
 *************************************/

static INTERRUPT_GEN( vb_interrupt )
{
	pending_vb_int = 0;

	if (DISABLE_VB_INT)
	{
		pending_vb_int = 1;
		return;
	}

	//logerror("V-Blank interrupt\n");
	cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
}


static void ide_interrupt(int state)
{
	if (state != CLEAR_LINE)
	{
		//logerror("IDE interrupt asserted\n");
		cpu_set_irq_line(0, MC68000_IRQ_1, HOLD_LINE);
	}
	else
	{
		//logerror("IDE interrupt cleared\n");
		cpu_set_irq_line(0, MC68000_IRQ_1, CLEAR_LINE);
	}
}




/*************************************
 *
 *	Memory definitions
 *
 *************************************/

static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },		// PRG ROM
	{ 0x400000, 0x40ffff, MRA32_RAM },		// WORK RAM
	{ 0x480000, 0x48443f, paletteram32_r },		// COLOR RAM (tilemap)
	{ 0x500000, 0x57ffff, sndram_r },		// SOUND RAM
	{ 0x580000, 0x58003f, K056832_long_r },		// VIDEO REG (tilemap)
	{ 0x5b0000, 0x5b04ff, dual539_r },		// SOUND regs
	{ 0x5c0000, 0x5c0003, inp1_r },			// input port
	{ 0x5c8000, 0x5c8003, inp2_r },			// input port
	{ 0x5e0000, 0x5e0003, scratch_r },		// scratch input port
	{ 0x600000, 0x601fff, v_rom_r },		// VIDEO ROM readthrough (for POST)
	{ 0x801000, 0x8017ff, MRA32_RAM },		// OBJECT RAM
	{ 0x803000, 0x80309f, obj_ctrl_r },		// OBJECT REGS
	{ 0x803800, 0x803fff, obj_rom_r },		// OBJECT ROM readthrough (for POST)
	{ 0xc00000, 0xc01fff, K056832_ram_long_r },	// VIDEO RAM (tilemap) (beatmania)
	{ 0xd00000, 0xd0000f, ide_std_r },		// IDE control regs (hiphopmania)
	{ 0xd4000c, 0xd4000f, ide_alt_r },		// IDE status control reg (hiphopmania)
	{ 0xe00000, 0xe01fff, K056832_ram_long_r },	// VIDEO RAM (tilemap) (hiphopmania)
	{ 0xf00000, 0xf0000f, ide_std_r },		// IDE control regs (beatmania)
	{ 0xf4000c, 0xf4000f, ide_alt_r },		// IDE status control reg (beatmania)
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },		// PRG ROM
	{ 0x400000, 0x40ffff, MWA32_RAM },		// WORK RAM
	{ 0x480000, 0x48443f, paletteram32_w, &paletteram32 },	// COLOR RAM
	{ 0x500000, 0x57ffff, sndram_w },		// SOUND RAM
	{ 0x580000, 0x58003f, K056832_long_w },		// VIDEO REG (tilemap)
	{ 0x590000, 0x590007, unknown590000_w },	// ??
	{ 0x5a0000, 0x5a005f, K055555_long_w },		// 055555: priority encoder
	{ 0x5b0000, 0x5b04ff, dual539_w },		// SOUND regs
	{ 0x5d0000, 0x5d0003, light_ctrl_1_w },		// light/coin blocker control 1
	{ 0x5d2000, 0x5d2003, light_ctrl_2_w },		// light/coin blocker control 2
	{ 0x5d4000, 0x5d4003, v_ctrl_w },		// VIDEO control
	{ 0x5d6000, 0x5d6003, sndram_bank_w },		// SOUND RAM bank
	{ 0x5e0000, 0x5e0003, scratch_w },		// scratch input port
	{ 0x801000, 0x8017ff, MWA32_RAM, &djmain_obj_ram },	// OBJECT RAM
	{ 0x802000, 0x802fff, unknown802000_w },	// ??
	{ 0x803000, 0x80309f, obj_ctrl_w },		// OBJECT REGS
	{ 0xc00000, 0xc01fff, K056832_ram_long_w },	// VIDEO RAM (tilemap) (beatmania)
	{ 0xc02000, 0xc02047, unknownc02000_w },	// ??
	{ 0xd00000, 0xd0000f, ide_std_w },		// IDE control regs (hiphopmania)
	{ 0xd4000c, 0xd4000f, ide_alt_w },		// IDE status control reg (hiphopmania)
	{ 0xe00000, 0xe01fff, K056832_ram_long_w },	// VIDEO RAM (tilemap) (hiphopmania)
	{ 0xf00000, 0xf0000f, ide_std_w },		// IDE control regs (beatmania)
	{ 0xf4000c, 0xf4000f, ide_alt_w },		// IDE status control reg (beatmania)
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

#define BEATMANIA_IN0 \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

#define BEATMANIA_IN1 \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )	/* EFFECT */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

#define BEATMANIA_IN2 \
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	/* TEST SW */ \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* SERVICE */ \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* RESET SW */ \
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define BEATMANIA_SCRATCH \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* +R */ \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2 )	/* +R */ \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )	/* -L */ \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER2 )	/* -L */

#define BEATMANIA_DSW1 \
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0xf9, "1P 1C / 2P 1C / Continue 1C" ) \
	PORT_DIPSETTING(    0xfe, "1P 1C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0xfd, "1P 2C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0xfc, "1P 2C / 2P 2C / Continue 2C" ) \
	PORT_DIPSETTING(    0xfb, "1P 2C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0xfa, "1P 2C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0xff, "1P 2C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0xf8, "1P 2C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0xf7, "1P 3C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0xf6, "1P 3C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0xf5, "1P 3C / 2P 3C / Continue 3C" ) \
	PORT_DIPSETTING(    0xf4, "1P 3C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0xf3, "1P 3C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0xf2, "1P 3C / 2P 4C / Continue 3C" ) \
	PORT_DIPSETTING(    0xf1, "1P 3C / 2P 6C / Continue 1C" ) \
	PORT_DIPSETTING(    0xf0, "1P 3C / 2P 6C / Continue 2C" ) \
	PORT_DIPSETTING(    0xef, "1P 3C / 2P 6C / Continue 3C" ) \
	PORT_DIPSETTING(    0xee, "1P 4C / 2P 8C / Continue 2C" ) \
	PORT_DIPSETTING(    0xed, "1P 4C / 2P 8C / Continue 3C" ) \
	PORT_DIPSETTING(    0xec, "1P 4C / 2P 8C / Continue 4C" ) \
	PORT_DIPSETTING(    0xeb, "1P 5C / 2P 10C / Continue 3C" ) \
	PORT_DIPSETTING(    0xea, "1P 5C / 2P 10C / Continue 4C" ) \
	PORT_DIPSETTING(    0xe9, "1P 5C / 2P 10C / Continue 5C" ) \
	PORT_DIPSETTING(    0xe8, "1P 6C / 2P 12C / Continue 4C" ) \
	PORT_DIPSETTING(    0xe7, "1P 6C / 2P 12C / Continue 5C" ) \
	PORT_DIPSETTING(    0xe6, "1P 6C / 2P 12C / Continue 6C" ) \
	PORT_DIPSETTING(    0xe5, "1P 7C / 2P 14C / Continue 5C" ) \
	PORT_DIPSETTING(    0xe4, "1P 7C / 2P 14C / Continue 6C" ) \
	PORT_DIPSETTING(    0xe3, "1P 7C / 2P 14C / Continue 7C" ) \
	PORT_DIPSETTING(    0xe2, "1P 8C / 2P 16C / Continue 6C" ) \
	PORT_DIPSETTING(    0xe1, "1P 8C / 2P 16C / Continue 7C" ) \
	PORT_DIPSETTING(    0xe0, "1P 8C / 2P 16C / Continue 8C" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

#define BMCOMPMX_DSW1 \
	PORT_DIPNAME( 0xff, 0xbf, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0xb9, "1P 1C / 2P 1C / Continue 1C" ) \
	PORT_DIPSETTING(    0xbe, "1P 1C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0xbd, "1P 2C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0xbc, "1P 2C / 2P 2C / Continue 2C" ) \
	PORT_DIPSETTING(    0xbb, "1P 2C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0xba, "1P 2C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0xbf, "1P 2C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0xb8, "1P 2C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0xb7, "1P 3C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0xb6, "1P 3C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0xb5, "1P 3C / 2P 3C / Continue 3C" ) \
	PORT_DIPSETTING(    0xb4, "1P 3C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0xb3, "1P 3C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0xb2, "1P 3C / 2P 4C / Continue 3C" ) \
	PORT_DIPSETTING(    0xb1, "1P 3C / 2P 6C / Continue 1C" ) \
	PORT_DIPSETTING(    0xb0, "1P 3C / 2P 6C / Continue 2C" ) \
	PORT_DIPSETTING(    0xaf, "1P 3C / 2P 6C / Continue 3C" ) \
	PORT_DIPSETTING(    0xae, "1P 4C / 2P 8C / Continue 2C" ) \
	PORT_DIPSETTING(    0xad, "1P 4C / 2P 8C / Continue 3C" ) \
	PORT_DIPSETTING(    0xac, "1P 4C / 2P 8C / Continue 4C" ) \
	PORT_DIPSETTING(    0xab, "1P 5C / 2P 10C / Continue 3C" ) \
	PORT_DIPSETTING(    0xaa, "1P 5C / 2P 10C / Continue 4C" ) \
	PORT_DIPSETTING(    0xa9, "1P 5C / 2P 10C / Continue 5C" ) \
	PORT_DIPSETTING(    0xa8, "1P 6C / 2P 12C / Continue 4C" ) \
	PORT_DIPSETTING(    0xa7, "1P 6C / 2P 12C / Continue 5C" ) \
	PORT_DIPSETTING(    0xa6, "1P 6C / 2P 12C / Continue 6C" ) \
	PORT_DIPSETTING(    0xa5, "1P 7C / 2P 14C / Continue 5C" ) \
	PORT_DIPSETTING(    0xa4, "1P 7C / 2P 14C / Continue 6C" ) \
	PORT_DIPSETTING(    0xa3, "1P 7C / 2P 14C / Continue 7C" ) \
	PORT_DIPSETTING(    0xa2, "1P 8C / 2P 16C / Continue 6C" ) \
	PORT_DIPSETTING(    0xa1, "1P 8C / 2P 16C / Continue 7C" ) \
	PORT_DIPSETTING(    0xa0, "1P 8C / 2P 16C / Continue 8C" ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Free_Play ) )

#define BM4THMIX_DSW1 \
	PORT_DIPNAME( 0xff, 0x7f, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x79, "1P 1C / 2P 1C / Continue 1C" ) \
	PORT_DIPSETTING(    0x7e, "1P 1C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0x7d, "1P 2C / 2P 2C / Continue 1C" ) \
	PORT_DIPSETTING(    0x7c, "1P 2C / 2P 2C / Continue 2C" ) \
	PORT_DIPSETTING(    0x7b, "1P 2C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0x7a, "1P 2C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0x7f, "1P 2C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0x78, "1P 2C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0x77, "1P 3C / 2P 3C / Continue 1C" ) \
	PORT_DIPSETTING(    0x76, "1P 3C / 2P 3C / Continue 2C" ) \
	PORT_DIPSETTING(    0x75, "1P 3C / 2P 3C / Continue 3C" ) \
	PORT_DIPSETTING(    0x74, "1P 3C / 2P 4C / Continue 1C" ) \
	PORT_DIPSETTING(    0x73, "1P 3C / 2P 4C / Continue 2C" ) \
	PORT_DIPSETTING(    0x72, "1P 3C / 2P 4C / Continue 3C" ) \
	PORT_DIPSETTING(    0x71, "1P 3C / 2P 6C / Continue 1C" ) \
	PORT_DIPSETTING(    0x70, "1P 3C / 2P 6C / Continue 2C" ) \
	PORT_DIPSETTING(    0x6f, "1P 3C / 2P 6C / Continue 3C" ) \
	PORT_DIPSETTING(    0x6e, "1P 4C / 2P 8C / Continue 2C" ) \
	PORT_DIPSETTING(    0x6d, "1P 4C / 2P 8C / Continue 3C" ) \
	PORT_DIPSETTING(    0x6c, "1P 4C / 2P 8C / Continue 4C" ) \
	PORT_DIPSETTING(    0x6b, "1P 5C / 2P 10C / Continue 3C" ) \
	PORT_DIPSETTING(    0x6a, "1P 5C / 2P 10C / Continue 4C" ) \
	PORT_DIPSETTING(    0x69, "1P 5C / 2P 10C / Continue 5C" ) \
	PORT_DIPSETTING(    0x68, "1P 6C / 2P 12C / Continue 4C" ) \
	PORT_DIPSETTING(    0x67, "1P 6C / 2P 12C / Continue 5C" ) \
	PORT_DIPSETTING(    0x66, "1P 6C / 2P 12C / Continue 6C" ) \
	PORT_DIPSETTING(    0x65, "1P 7C / 2P 14C / Continue 5C" ) \
	PORT_DIPSETTING(    0x64, "1P 7C / 2P 14C / Continue 6C" ) \
	PORT_DIPSETTING(    0x63, "1P 7C / 2P 14C / Continue 7C" ) \
	PORT_DIPSETTING(    0x62, "1P 8C / 2P 16C / Continue 6C" ) \
	PORT_DIPSETTING(    0x61, "1P 8C / 2P 16C / Continue 7C" ) \
	PORT_DIPSETTING(    0x60, "1P 8C / 2P 16C / Continue 8C" ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Free_Play ) )

#define BEATMANIA_DSW2 \
	PORT_DIPNAME( 0x80, 0x80, "Score Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x40, "Low" ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, "Medium" ) \
	PORT_DIPSETTING(    0x00, "Silence" ) \
	PORT_DIPNAME( 0x10, 0x10, "Level Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x0c, 0x0c, "Normal Difficulty" ) \
	PORT_DIPSETTING(    0x08, "Level 0" ) \
	PORT_DIPSETTING(    0x0c, "Level 1" ) \
	PORT_DIPSETTING(    0x04, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" ) \
	PORT_DIPNAME( 0x03, 0x03, "Expert Difficulty" ) \
	PORT_DIPSETTING(    0x02, "Level 0" ) \
	PORT_DIPSETTING(    0x03, "Level 1" ) \
	PORT_DIPSETTING(    0x01, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" )

#define BMCOMPMX_DSW2 \
	PORT_DIPNAME( 0x80, 0x80, "Score Display" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x40, "Low" ) \
	PORT_DIPSETTING(    0x60, "Loud" ) \
	PORT_DIPSETTING(    0x20, "Medium" ) \
	PORT_DIPSETTING(    0x00, "Silence" ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* DSW 2 - 4 */ \
	PORT_DIPNAME( 0x0c, 0x0c, "Normal Difficulty" ) \
	PORT_DIPSETTING(    0x08, "Level 0" ) \
	PORT_DIPSETTING(    0x0c, "Level 1" ) \
	PORT_DIPSETTING(    0x04, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" ) \
	PORT_DIPNAME( 0x03, 0x03, "Expert Difficulty" ) \
	PORT_DIPSETTING(    0x02, "Level 0" ) \
	PORT_DIPSETTING(    0x03, "Level 1" ) \
	PORT_DIPSETTING(    0x01, "Level 2" ) \
	PORT_DIPSETTING(    0x00, "Level 3" )

#define BEATMANIA_DSW3 \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Stages" ) \
	PORT_DIPSETTING(    0x1c, "Normal 4 / Free 3" ) \
	PORT_DIPSETTING(    0x10, "Normal 3 / Free 2" ) \
	PORT_DIPSETTING(    0x08, "Normal 5 / Free 3" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 5 */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 6 */

#define BM1STMIX_DSW3 \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x14, "Stages" ) \
	PORT_DIPSETTING(    0x14, "Normal 4 / Free 3" ) \
	PORT_DIPSETTING(    0x10, "Normal 3 / Free 2" ) \
	PORT_DIPSETTING(    0x00, "Normal 5 / Free 3" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 5 */ \
	PORT_DIPNAME( 0x01, 0x01, "Free Hidden Songs" )	/* DSW 3 - 6 */ \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define BMCOMPMX_DSW3 \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Stages" ) \
	PORT_DIPSETTING(    0x1c, "Normal 4 / Free 3" ) \
	PORT_DIPSETTING(    0x10, "Normal 3 / Free 2" ) \
	PORT_DIPSETTING(    0x08, "Normal 5 / Free 3" ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 5 */ \
	PORT_DIPNAME( 0x01, 0x01, "Secret Expert Course" )  /* DSW 3 - 6 */ \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define BM4THMIX_DSW3 \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Stages" ) \
	PORT_DIPSETTING(    0x1c, "Normal 4 / Free 3" ) \
	PORT_DIPSETTING(    0x10, "Normal 3 / Free 2" ) \
	PORT_DIPSETTING(    0x08, "Normal 5 / Free 3" ) \
	PORT_DIPNAME( 0x02, 0x02, "Secret Expert Course" )  /* DSW 3 - 5 */ \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 6 */

#define HMCOMPM2_DSW3 \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_DIPNAME( 0x20, 0x20, "Event Mode" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x1c, 0x1c, "Stages" ) \
	PORT_DIPSETTING(    0x1c, "Normal 4 / Free 3" ) \
	PORT_DIPSETTING(    0x10, "Normal 3 / Free 2" ) \
	PORT_DIPSETTING(    0x08, "Normal 5 / Free 3" ) \
	PORT_DIPNAME( 0x02, 0x02, "Game Over Mode" ) \
	PORT_DIPSETTING(    0x02, "On Stage Middle" ) \
	PORT_DIPSETTING(    0x00, "On Stage Last" ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* DSW 3 - 6 */

INPUT_PORTS_START( beatmania )
	PORT_START      /* IN0 */
	BEATMANIA_IN0

	PORT_START      /* IN1 */
	BEATMANIA_IN1

	PORT_START      /* IN2 */
	BEATMANIA_IN2

	PORT_START      /* IN3: DSW 1 */
	BEATMANIA_DSW1

	PORT_START      /* IN4: DSW 2 */
	BEATMANIA_DSW2

	PORT_START      /* IN5: DSW 3 */
	BEATMANIA_DSW3

	PORT_START      /* IN6: fake port for scratch */
	BEATMANIA_SCRATCH
INPUT_PORTS_END

INPUT_PORTS_START( bmcompmx )
	PORT_START      /* IN0 */
	BEATMANIA_IN0

	PORT_START      /* IN1 */
	BEATMANIA_IN1

	PORT_START      /* IN2 */
	BEATMANIA_IN2

	PORT_START      /* IN3: DSW 1 */
	BMCOMPMX_DSW1

	PORT_START      /* IN4: DSW 2 */
	BMCOMPMX_DSW2

	PORT_START      /* IN5: DSW 3 */
	BMCOMPMX_DSW3

	PORT_START      /* IN6: fake port for scratch */
	BEATMANIA_SCRATCH
INPUT_PORTS_END

INPUT_PORTS_START( bm4thmix )
	PORT_START      /* IN0 */
	BEATMANIA_IN0

	PORT_START      /* IN1 */
	BEATMANIA_IN1

	PORT_START      /* IN2 */
	BEATMANIA_IN2

	PORT_START      /* IN3: DSW 1 */
	BM4THMIX_DSW1

	PORT_START      /* IN4: DSW 2 */
	BEATMANIA_DSW2

	PORT_START      /* IN5: DSW 3 */
	BM4THMIX_DSW3

	PORT_START      /* IN6: fake port for scratch */
	BEATMANIA_SCRATCH
INPUT_PORTS_END

INPUT_PORTS_START( hmcompm2 )
	PORT_START      /* IN0 */
	BEATMANIA_IN0

	PORT_START      /* IN1 */
	BEATMANIA_IN1

	PORT_START      /* IN2 */
	BEATMANIA_IN2

	PORT_START      /* IN3: DSW 1 */
	BEATMANIA_DSW1

	PORT_START      /* IN4: DSW 2 */
	BEATMANIA_DSW2

	PORT_START      /* IN5: DSW 3 */
	HMCOMPM2_DSW3

	PORT_START      /* IN6: fake port for scratch */
	BEATMANIA_SCRATCH

#ifdef AUTOPLAY_CHEAT
	PORT_START      /* IN7: autoplay cheat */
	PORT_BITX(0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Auto Play", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(0x01, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )
#endif /* AUTOPLAY_CHEAT */
INPUT_PORTS_END


/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout spritelayout =
{
	16, 16,	/* 16x16 characters */
	0x200000 / 128,	/* 16384 characters */
	4,	/* bit planes */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
	  4+256, 0+256, 12+256, 8+256, 20+256, 16+256, 28+256, 24+256 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  0*32+512, 1*32+512, 2*32+512, 3*32+512, 4*32+512, 5*32+512, 6*32+512, 7*32+512 },
	16*16*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,  (0x4440/4)/16 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	IDE interfaces
 *
 *************************************/

static struct ide_interface ide_intf =
{
	ide_interrupt,
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};



/*************************************
 *
 *	Machine-specific init
 *
 *************************************/

static MACHINE_INIT( djmain )
{
	/* reset sound ram bank */
	sndram_bank = 0;
	sndram_set_bank();

	/* reset the IDE controller */
	ide_controller_reset(0);

	/* reset LEDs */
	set_led_status(0, 1);
	set_led_status(1, 1);
	set_led_status(2, 1);
}



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( djmain )

	/* basic machine hardware */
	//MDRV_CPU_ADD(M68EC020, 32000000*3/4)	/* 24.000 MHz ? */
	//MDRV_CPU_ADD(M68EC020, 18432000)	/* 18.432 MHz ? */
	MDRV_CPU_ADD(M68EC020, 32000000/2)	/* 16.000 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vb_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(djmain)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(12, 512-12-1, 0, 384-1)
	MDRV_PALETTE_LENGTH(0x4440/4)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_START(djmain)
	MDRV_VIDEO_UPDATE(djmain)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( bmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858jab01.6a", 0x000000, 0x80000, CRC(92841EB5) SHA1(3a9d90a9c4b16cb7118aed2cadd3ab32919efa96) )
	ROM_LOAD16_BYTE( "858jab02.8a", 0x000001, 0x80000, CRC(7B19969C) SHA1(3545acabbf53bacc5afa72a3c5af3cd648bc2ae1) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "858jaa03.19a", 0x000000, 0x80000, CRC(8559F457) SHA1(133092994087864a6c29e9d51dcdbef2e2c2a123) )
	ROM_LOAD16_BYTE( "858jaa04.20a", 0x000001, 0x80000, CRC(770824D3) SHA1(5c21bc39f8128957d76be85bc178c96976987f5f) )
	ROM_LOAD16_BYTE( "858jaa05.22a", 0x100000, 0x80000, CRC(9CE769DA) SHA1(1fe2999f786effdd5e3e74475e8431393eb9403d) )
	ROM_LOAD16_BYTE( "858jaa06.24a", 0x100001, 0x80000, CRC(0CDE6584) SHA1(fb58d2b4f58144b71703431740c0381bb583f581) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858jaa07.22d", 0x000000, 0x80000, CRC(7D183F46) SHA1(7a1b0ccb0407b787af709bdf038d886727199e4e) )
	ROM_LOAD16_BYTE( "858jaa08.23d", 0x000001, 0x80000, CRC(C731DC8F) SHA1(1a937d76c02711b7f73743c9999456d4408ad284) )
	ROM_LOAD16_BYTE( "858jaa09.25d", 0x100000, 0x80000, CRC(0B4AD843) SHA1(c01e15053dd1975dc68db9f4e6da47062d8f9b54) )
	ROM_LOAD16_BYTE( "858jaa10.27d", 0x100001, 0x80000, CRC(00B124EE) SHA1(435d28a327c2707833a8ddfe841104df65ffa3f8) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11.chd", 0, MD5(e7b26f6f03f807a32b2e5e291324d582) )
ROM_END

ROM_START( hmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858uab01.6a", 0x000000, 0x80000, CRC(F9C16675) SHA1(f2b50a3544f43af6fd987256a8bd4125b95749ef) )
	ROM_LOAD16_BYTE( "858uab02.8a", 0x000001, 0x80000, CRC(4E8F1E78) SHA1(88d654de4377b584ff8a5e1f8bc81ffb293ec8a5) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "858uaa03.19a", 0x000000, 0x80000, CRC(52B51A5E) SHA1(9f01e2fcbe5a9d7f80b377c5e10f18da2c9dcc8e) )
	ROM_LOAD16_BYTE( "858uaa04.20a", 0x000001, 0x80000, CRC(A336CEE9) SHA1(0e62c0c38d86868c909b4c1790fbb7ecb2de137d) )
	ROM_LOAD16_BYTE( "858uaa05.22a", 0x100000, 0x80000, CRC(2E14CF83) SHA1(799b2162f7b11678d1d260f7e1eb841abda55a60) )
	ROM_LOAD16_BYTE( "858uaa06.24a", 0x100001, 0x80000, CRC(2BE07788) SHA1(5cc2408f907ca6156efdcbb2c10a30e9b81797f8) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858uaa07.22d", 0x000000, 0x80000, CRC(9D7C8EA0) SHA1(5ef773ade7ab12a5dc10484e8b7711c9d76fe2a1) )
	ROM_LOAD16_BYTE( "858uaa08.23d", 0x000001, 0x80000, CRC(F21C3F45) SHA1(1d7ff2c4161605b382d07900142093192aa93a48) )
	ROM_LOAD16_BYTE( "858uaa09.25d", 0x100000, 0x80000, CRC(99519886) SHA1(664f6bd953201a6e2fc123cb8b3facf72766107d) )
	ROM_LOAD16_BYTE( "858uaa10.27d", 0x100001, 0x80000, CRC(20AA7145) SHA1(eeff87eb9a9864985d751f45e843ee6e73db8cfd) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11.chd", 0, MD5(e7b26f6f03f807a32b2e5e291324d582) )
ROM_END

ROM_START( bm4thmix )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "847jaa01.6a", 0x000000, 0x80000, CRC(81138A1B) SHA1(ebe211126f871e541881e1670f56d50b058dead3) )
	ROM_LOAD16_BYTE( "847jaa02.8a", 0x000001, 0x80000, CRC(4EEB0010) SHA1(942303dfb19a4a78dd74ad24576031760553a661) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "847jaa03.19a", 0x000000, 0x80000, CRC(F447D140) SHA1(cc15b80419940d127a77765508f877421ed86ee2) )
	ROM_LOAD16_BYTE( "847jaa04.20a", 0x000001, 0x80000, CRC(EDC3E286) SHA1(341b1dc6ee1562b1ddf235a66ac96b94c482b67c) )
	ROM_LOAD16_BYTE( "847jaa05.22a", 0x100000, 0x80000, CRC(DA165B5E) SHA1(e46110590e6ab89b55f6abfbf6c53c99d28a75a9) )
	ROM_LOAD16_BYTE( "847jaa06.24a", 0x100001, 0x80000, CRC(8BFC2F28) SHA1(f8869867945d63d9f34b6228d95c5a61b193eed2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "847jab07.22d", 0x000000, 0x80000, CRC(C159E7C4) SHA1(96af0c29b2f1fef494b2223179862d16f26bb33f) )
	ROM_LOAD16_BYTE( "847jab08.23d", 0x000001, 0x80000, CRC(8FF084D6) SHA1(50cff8c701e33f2630925c1a9ae4351076912acd) )
	ROM_LOAD16_BYTE( "847jab09.25d", 0x100000, 0x80000, CRC(2E4AC9FE) SHA1(bbd4c6e0c82fc0be88f851e901e5853b6bcf775f) )
	ROM_LOAD16_BYTE( "847jab10.27d", 0x100001, 0x80000, CRC(C78516F5) SHA1(1adf5805c808dc55de14a9a9b20c3d2cf7bf414d) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "847jaa11.chd", 0, MD5(47cb5c1b856aa11cf38f0c7ea4a7d1c3) )
ROM_END

ROM_START( hmcompm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "988uaa01.6a", 0x000000, 0x80000, CRC(5E5CC6C0) SHA1(0e7cd601d4543715cbc9f65e6fd48837179c962a) )
	ROM_LOAD16_BYTE( "988uaa02.8a", 0x000001, 0x80000, CRC(E262984A) SHA1(f47662e40f91f2addb1a4b649923c1d0ee017341) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "988uaa03.19a", 0x000000, 0x80000, CRC(D0F204C8) SHA1(866baac5a6d301d5b9cf0c14e9937ee5f435db77) )
	ROM_LOAD16_BYTE( "988uaa04.20a", 0x000001, 0x80000, CRC(74C6B3ED) SHA1(7d9b064bab3f29fc6435f6430c71208abbf9d861) )
	ROM_LOAD16_BYTE( "988uaa05.22a", 0x100000, 0x80000, CRC(6B9321CB) SHA1(449e5f85288a8c6724658050fa9521c7454a1e46) )
	ROM_LOAD16_BYTE( "988uaa06.24a", 0x100001, 0x80000, CRC(DA6E0C1E) SHA1(4ef37db6c872bccff8c27fc53cccc0b269c7aee4) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "988uaa07.22d", 0x000000, 0x80000, CRC(9217870D) SHA1(d0536a8a929c41b49cdd053205165bfb8150e0c5) )
	ROM_LOAD16_BYTE( "988uaa08.23d", 0x000001, 0x80000, CRC(77777E59) SHA1(33b5508b961a04b82c9967a3326af6bbd838b85e) )
	ROM_LOAD16_BYTE( "988uaa09.25d", 0x100000, 0x80000, CRC(C2AD6810) SHA1(706388c5acf6718297fd90e10f8a673463a0893b) )
	ROM_LOAD16_BYTE( "988uaa10.27d", 0x100001, 0x80000, CRC(DAB0F3C9) SHA1(6fd899e753e32f60262c54ab8553c686c7ef28de) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "988jaa11.chd", 0, MD5(cc21d58d6bee58f1c4baf08f345fe2c5) )
ROM_END

ROM_START( bmcorerm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "a05jaa01.6a", 0x000000, 0x80000, CRC(CD6F1FC5) SHA1(237cbc17a693efb6bffffd6afb24f0944c29330c) )
	ROM_LOAD16_BYTE( "a05jaa02.8a", 0x000001, 0x80000, CRC(FE07785E) SHA1(14c652008cb509b5206fb515aad7dfe36a6fe6f4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPRITE */
	ROM_LOAD16_BYTE( "a05jaa03.19a", 0x000000, 0x80000, CRC(8B88932A) SHA1(df20f8323adb02d07b835da98f4a29b3142175c9) )
	ROM_LOAD16_BYTE( "a05jaa04.20a", 0x000001, 0x80000, CRC(CC72629F) SHA1(f95d06f409c7d6422d66a55c0452eb3feafc6ef0) )
	ROM_LOAD16_BYTE( "a05jaa05.22a", 0x100000, 0x80000, CRC(E241B22B) SHA1(941a76f6ac821e0984057ec7df7862b12fa657b8) )
	ROM_LOAD16_BYTE( "a05jaa06.24a", 0x100001, 0x80000, CRC(77EB08A3) SHA1(fd339aaec06916abfc928e850e33480707b5450d) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "a05jaa07.22d", 0x000000, 0x80000, CRC(4D79646D) SHA1(5f1237bbd3cb09b27babf1c5359ef6c0d80ae3a9) )
	ROM_LOAD16_BYTE( "a05jaa08.23d", 0x000001, 0x80000, CRC(F067494F) SHA1(ef031b5501556c1aa047a51604a44551b35a8b99) )
	ROM_LOAD16_BYTE( "a05jaa09.25d", 0x100000, 0x80000, CRC(1504D62C) SHA1(3c31c6625bc089235a96fe21021239f2d0c0f6e1) )
	ROM_LOAD16_BYTE( "a05jaa10.27d", 0x100001, 0x80000, CRC(99D75C36) SHA1(9599420863aa0a9492d3caeb03f8ac5fd4c3cdb2) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "a05jaa11.chd", 0, MD5(180f7b1b2145fab2d2ba717780f2ca26) )
ROM_END

/*************************************
 *
 *	Driver-specific init
 *
 *************************************/

static DRIVER_INIT( djmain )
{
	if (new_memory_region(REGION_SOUND1, 0x80000 * 32, 0))
		return;

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);

	state_save_register_int   ("djmain", 0, "sndram_bank",    &sndram_bank);
	state_save_register_UINT8 ("djmain", 0, "sndram",         memory_region(REGION_SOUND1), 0x80000 * 32);
	state_save_register_int   ("djmain", 0, "pending_vb_int", &pending_vb_int);
	state_save_register_UINT16("djmain", 0, "v_ctrl",         &v_ctrl,  1);
	state_save_register_UINT32("djmain", 0, "obj_regs",       obj_regs, sizeof (obj_regs) / sizeof (UINT32));

	state_save_register_func_postload(sndram_set_bank);
}

static UINT8 beatmania_master_password[2 + 32] =
{
	0x01, 0x00,
	0x4d, 0x47, 0x43, 0x28, 0x4b, 0x29, 0x4e, 0x4f,
	0x4d, 0x41, 0x20, 0x49, 0x4c, 0x41, 0x20, 0x4c,
	0x49, 0x52, 0x48, 0x47, 0x53, 0x54, 0x52, 0x20,
	0x53, 0x45, 0x52, 0x45, 0x45, 0x56, 0x2e, 0x44
};

static DRIVER_INIT( hmcompmx )
{
	static UINT8 hmcompmx_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3a, 0x34, 0x38, 0x2a,
		0x5a, 0x4d, 0x78, 0x3e, 0x74, 0x61, 0x6c, 0x0a,
		0x7a, 0x63, 0x19, 0x77, 0x73, 0x7d, 0x0d, 0x12,
		0x6b, 0x09, 0x02, 0x0f, 0x05, 0x00, 0x7d, 0x1b
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompmx_user_password);
}

static DRIVER_INIT( bm4thmix )
{
	static UINT8 bm4thmix_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x29, 0x4b, 0x2f, 0x2c, 0x4c, 0x32,
		0x48, 0x5d, 0x0c, 0x3e, 0x62, 0x6f, 0x7e, 0x73,
		0x67, 0x10, 0x19, 0x79, 0x6c, 0x7d, 0x00, 0x01,
		0x18, 0x06, 0x1e, 0x07, 0x77, 0x1a, 0x7d, 0x77
	};

	init_djmain();

	ide_set_user_password(0, bm4thmix_user_password);
}

static DRIVER_INIT( hmcompm2 )
{
	static UINT8 hmcompm2_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x3b, 0x39, 0x24, 0x3e, 0x4e, 0x59, 0x5c, 0x32,
		0x3b, 0x4c, 0x72, 0x57, 0x69, 0x04, 0x79, 0x65,
		0x76, 0x10, 0x6a, 0x77, 0x1f, 0x65, 0x0a, 0x16,
		0x09, 0x68, 0x71, 0x0b, 0x77, 0x15, 0x17, 0x1e
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompm2_user_password);
}

static DRIVER_INIT( bmcorerm )
{
	static UINT8 bmcorerm_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3f, 0x4d, 0x4a, 0x27,
		0x5a, 0x52, 0x0c, 0x3e, 0x6a, 0x04, 0x63, 0x6f,
		0x72, 0x64, 0x72, 0x7f, 0x1f, 0x73, 0x17, 0x04,
		0x05, 0x09, 0x14, 0x0d, 0x7a, 0x74, 0x7d, 0x7a
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmcorerm_user_password);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1999, bmcompmx, 0,        djmain,   bmcompmx,  djmain,   ROT0, "Konami", "Beatmania Complete MIX (ver JA-B)" )
GAME( 1999, hmcompmx, bmcompmx, djmain,   bmcompmx,  hmcompmx, ROT0, "Konami", "Hiphopmania Complete MIX (ver UA-B)" )
GAME( 1999, bm4thmix, 0,        djmain,   bm4thmix,  bm4thmix, ROT0, "Konami", "Beatmania 4th MIX (ver JA-A)" )
GAME( 2000, hmcompm2, 0,        djmain,   hmcompm2,  hmcompm2, ROT0, "Konami", "Hiphopmania Complete MIX 2 (ver UA-A)" )
GAME( 2000, bmcorerm, 0,        djmain,   beatmania, bmcorerm, ROT0, "Konami", "Beatmania CORE REMIX (ver JA-A)" )
