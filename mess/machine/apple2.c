/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

  TODO:  Make a standard set of peripherals work.
  TODO:  Allow swappable peripherals in each slot.
  TODO:  Verify correctness of C08X switches.
			- need to do double-read before write-enable RAM

***************************************************************************/

/* common.h included for the RomModule definition */
#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define PROFILER_C00X	PROFILER_USER2
#define PROFILER_C01X	PROFILER_USER2
#define PROFILER_C08X	PROFILER_USER2
#define PROFILER_A2INT	PROFILER_USER2

/* softswitch */
UINT32 a2;

/* before the softswitch is changed, these are applied */
static UINT32 a2_mask;
static UINT32 a2_set;


/* local */
static int a2_speaker_state;

static void mockingboard_init (int slot);
static int mockingboard_r (int offset);
static void mockingboard_w (int offset, int data);
static WRITE_HANDLER ( apple2_mainram0200_w );
static WRITE_HANDLER ( apple2_mainram0400_w );
static WRITE_HANDLER ( apple2_mainram0800_w );
static WRITE_HANDLER ( apple2_mainram2000_w );
static WRITE_HANDLER ( apple2_mainram4000_w );
static WRITE_HANDLER ( apple2_auxram0200_w );
static WRITE_HANDLER ( apple2_auxram0400_w );
static WRITE_HANDLER ( apple2_auxram0800_w );
static WRITE_HANDLER ( apple2_auxram2000_w );
static WRITE_HANDLER ( apple2_auxram4000_w );
static WRITE_HANDLER ( apple2_LC_ram_w );
static WRITE_HANDLER ( apple2_LC_ram1_w );
static WRITE_HANDLER ( apple2_LC_ram2_w );

static double joystick_x1_time;
static double joystick_y1_time;
static double joystick_x2_time;
static double joystick_y2_time;

static UINT8 *apple_rom;

/***************************************************************************
  apple2_slotrom
  returns a pointer to a slot ROM
***************************************************************************/
static UINT8 *apple2_slotrom(int slot)
{
	UINT8 *rom;
	UINT8 *slotrom;
	size_t rom_size;
	size_t slot_rom_pos;
	size_t slot_rom_size = 0x100;
	size_t slot_count;
	
	rom = memory_region(REGION_CPU1);
	rom_size = memory_region_length(REGION_CPU1);
	slot_rom_pos = rom_size - (rom_size % 0x1000);
	slot_count = (rom_size - slot_rom_pos) / slot_rom_size;

	/* slots are one-counted */
	slot--;

	assert(slot >= 0);
	assert(slot < slot_count);

	slotrom = &rom[slot_rom_pos + (slot * slot_rom_size)];
	return slotrom;
}

/***************************************************************************
  apple2_hasslots
***************************************************************************/
static int apple2_hasslots(void)
{
	return (memory_region_length(REGION_CPU1) % 0x1000) != 0;
}

/***************************************************************************
  apple2_setvar
  sets the 'a2' var, and adjusts banking accordingly
***************************************************************************/
static void apple2_setvar(UINT32 val, UINT32 mask)
{
	UINT32 offset;
	UINT32 othermask;

	LOG(("apple2_setvar(): val=0x%06x mask=0x%06x pc=0x%04x\n", val, mask, activecpu_get_pc()));

	assert((val & mask) == val);

	/* apply mask and set */
	val &= a2_mask;
	val |= a2_set;

	/* change the softswitch */
	a2 &= ~mask;
	a2 |= val;

	if (mask & VAR_ROMSWITCH)
	{
		apple_rom = &memory_region(REGION_CPU1)[(a2 & VAR_ROMSWITCH) ? 0x4000 : 0x0000];
	}

	if (mask & VAR_RAMRD)
	{
		cpu_setbank(5,  &mess_ram[(a2 & VAR_RAMRD) ? 0x10200 : 0x00200]);
		cpu_setbank(8,  &mess_ram[(a2 & VAR_RAMRD) ? 0x10800 : 0x00800]);
		cpu_setbank(10, &mess_ram[(a2 & VAR_RAMRD) ? 0x14000 : 0x04000]);
	}

	if (mask & (VAR_80STORE|VAR_PAGE2|VAR_HIRES|VAR_RAMRD))
	{
		othermask = (a2 & VAR_80STORE) ? VAR_PAGE2 : VAR_RAMRD;
		cpu_setbank(7,  &mess_ram[(a2 & othermask) ? 0x10400 : 0x00400]);

		othermask = ((a2 & (VAR_80STORE|VAR_HIRES)) == (VAR_80STORE|VAR_HIRES)) ? VAR_PAGE2 : VAR_RAMWRT;
		cpu_setbank(9,  &mess_ram[(a2 & VAR_RAMRD) ? 0x12000 : 0x02000]);
	}

	if (mask & (VAR_80STORE|VAR_PAGE2|VAR_HIRES|VAR_RAMWRT))
	{
		memory_set_bankhandler_w(5,  0, (a2 & VAR_RAMWRT) ? apple2_auxram0200_w : apple2_mainram0200_w);
		memory_set_bankhandler_w(8,  0, (a2 & VAR_RAMWRT) ? apple2_auxram0800_w : apple2_mainram0800_w);
		memory_set_bankhandler_w(10, 0, (a2 & VAR_RAMWRT) ? apple2_auxram4000_w : apple2_mainram4000_w);

		othermask = (a2 & VAR_80STORE) ? VAR_PAGE2 : VAR_RAMWRT;
		memory_set_bankhandler_w(7,  0, (a2 & othermask) ? apple2_auxram0400_w : apple2_mainram0400_w);

		othermask = ((a2 & (VAR_80STORE|VAR_HIRES)) == (VAR_80STORE|VAR_HIRES)) ? VAR_PAGE2 : VAR_RAMWRT;
		memory_set_bankhandler_w(9,  0, (a2 & VAR_RAMWRT) ? apple2_auxram2000_w : apple2_mainram2000_w);
	}

	if (mask & (VAR_INTCXROM|VAR_ROMSWITCH))
	{
		cpu_setbank(3,	(a2 & VAR_INTCXROM)		? &apple_rom[0x100] : apple2_slotrom(1));
		cpu_setbank(12,	(a2 & VAR_INTCXROM)		? &apple_rom[0x400] : apple2_slotrom(4));
	}

	if (mask & (VAR_INTCXROM|VAR_SLOTC3ROM|VAR_ROMSWITCH))
	{
		cpu_setbank(11,	((a2 & (VAR_INTCXROM|VAR_SLOTC3ROM)) == VAR_SLOTC3ROM) ? apple2_slotrom(3) : &apple_rom[0x300]);
	}

	if (mask & (VAR_ALTZP))
	{
		cpu_setbank(4, &mess_ram[(a2 & VAR_ALTZP) ? 0x10000 : 0x00000]);
	}

	if (mask & (VAR_ALTZP|VAR_LCRAM|VAR_LCRAM2|VAR_ROMSWITCH))
	{
		if (a2 & VAR_LCRAM)
		{
			cpu_setbank(2, &mess_ram[(a2 & VAR_ALTZP) ? 0x1e000 : 0xe000]);
			if (a2 & VAR_LCRAM2)
				offset = (a2 & VAR_ALTZP) ? 0x1d000 : 0xd000;
			else
				offset = (a2 & VAR_ALTZP) ? 0x1c000 : 0xc000;
			cpu_setbank(1, &mess_ram[offset]);
		}
		else
		{
			cpu_setbank(1, &apple_rom[0x1000]);
			cpu_setbank(2, &apple_rom[0x2000]);
		}
	}

	if (mask & (VAR_LCWRITE|VAR_LCRAM2))
	{
		if (a2 & VAR_LCWRITE)
		{
			memory_set_bankhandler_w(2, 0, apple2_LC_ram_w);
			if (a2 & VAR_LCRAM2)
				memory_set_bankhandler_w(1, 0, apple2_LC_ram2_w);
			else
				memory_set_bankhandler_w(1, 0, apple2_LC_ram1_w);
		}
		else
		{
			memory_set_bankhandler_w(1, 0, MWA_ROM);
			memory_set_bankhandler_w(2, 0, MWA_ROM);
		}
	}
}

static void apple2_updatevar(void)
{
	apple2_setvar(a2, ~0);
}

/***************************************************************************
  apple2_getfloatingbusvalue
  preliminary floating bus video scanner code - look for comments with FIX:
***************************************************************************/
static data8_t apple2_getfloatingbusvalue(void)
{
	enum
	{
		// scanner types
		kScannerNone = 0, kScannerApple2, kScannerApple2e, 

		// scanner constants
		kHBurstClock      =    53, // clock when Color Burst starts
		kHBurstClocks     =     4, // clocks per Color Burst duration
		kHClock0State     =  0x18, // H[543210] = 011000
		kHClocks          =    65, // clocks per horizontal scan (including HBL)
		kHPEClock         =    40, // clock when HPE (horizontal preset enable) goes low
		kHPresetClock     =    41, // clock when H state presets
		kHSyncClock       =    49, // clock when HSync starts
		kHSyncClocks      =     4, // clocks per HSync duration
		kNTSCScanLines    =   262, // total scan lines including VBL (NTSC)
		kNTSCVSyncLine    =   224, // line when VSync starts (NTSC)
		kPALScanLines     =   312, // total scan lines including VBL (PAL)
		kPALVSyncLine     =   264, // line when VSync starts (PAL)
		kVLine0State      = 0x100, // V[543210CBA] = 100000000
		kVPresetLine      =   256, // line when V state presets
		kVSyncLines       =     4, // lines per VSync duration
		kClocksPerVSync   = kHClocks * kNTSCScanLines // FIX: NTSC only?
	};

	// vars
	//
	int i, Hires, Mixed, Page2, _80Store, ScanLines, VSyncLine, ScanCycles,
		h_clock, h_state, h_0, h_1, h_2, h_3, h_4, h_5,
		v_line, v_state, v_A, v_B, v_C, v_0, v_1, v_2, v_3, v_4, v_5,
		_hires, addend0, addend1, addend2, sum, address;

	// video scanner data
	//
	i = activecpu_gettotalcycles() % kClocksPerVSync; // cycles into this VSync

	// machine state switches
	//
	Hires    = (a2 & VAR_HIRES) ? 1 : 0;
	Mixed    = (a2 & VAR_MIXED) ? 1 : 0;
	Page2    = (a2 & VAR_PAGE2) ? 1 : 0;
	_80Store = (a2 & VAR_80STORE) ? 1 : 0;

	// calculate video parameters according to display standard
	//
	ScanLines  = 1 ? kNTSCScanLines : kPALScanLines; // FIX: NTSC only?
	VSyncLine  = 1 ? kNTSCVSyncLine : kPALVSyncLine; // FIX: NTSC only?
	ScanCycles = ScanLines * kHClocks;

	// calculate horizontal scanning state
	//
	h_clock = (i + kHPEClock) % kHClocks; // which horizontal scanning clock
	h_state = kHClock0State + h_clock; // H state bits
	if (h_clock >= kHPresetClock) // check for horizontal preset
	{
		h_state -= 1; // correct for state preset (two 0 states)
	}
	h_0 = (h_state >> 0) & 1; // get horizontal state bits
	h_1 = (h_state >> 1) & 1;
	h_2 = (h_state >> 2) & 1;
	h_3 = (h_state >> 3) & 1;
	h_4 = (h_state >> 4) & 1;
	h_5 = (h_state >> 5) & 1;

	// calculate vertical scanning state
	//
	v_line  = i / kHClocks; // which vertical scanning line
	v_state = kVLine0State + v_line; // V state bits
	if ((v_line >= kVPresetLine)) // check for previous vertical state preset
	{
		v_state -= ScanLines; // compensate for preset
	}
	v_A = (v_state >> 0) & 1; // get vertical state bits
	v_B = (v_state >> 1) & 1;
	v_C = (v_state >> 2) & 1;
	v_0 = (v_state >> 3) & 1;
	v_1 = (v_state >> 4) & 1;
	v_2 = (v_state >> 5) & 1;
	v_3 = (v_state >> 6) & 1;
	v_4 = (v_state >> 7) & 1;
	v_5 = (v_state >> 8) & 1;

	// calculate scanning memory address
	//
	_hires = Hires;
	if (Hires && Mixed && (v_4 & v_2))
	{
		_hires = 0; // (address is in text memory)
	}

	addend0 = 0x68; // 1            1            0            1
	addend1 =              (h_5 << 5) | (h_4 << 4) | (h_3 << 3);
	addend2 = (v_4 << 6) | (v_3 << 5) | (v_4 << 4) | (v_3 << 3);
	sum     = (addend0 + addend1 + addend2) & (0x0F << 3);

	address = 0;
	address |= h_0 << 0; // a0
	address |= h_1 << 1; // a1
	address |= h_2 << 2; // a2
	address |= sum;      // a3 - aa6
	address |= v_0 << 7; // a7
	address |= v_1 << 8; // a8
	address |= v_2 << 9; // a9
	address |= ((_hires) ? v_A : (1 ^ (Page2 & (1 ^ _80Store)))) << 10; // a10
	address |= ((_hires) ? v_B : (Page2 & (1 ^ _80Store))) << 11; // a11
	if (_hires) // hires?
	{
		// Y: insert hires only address bits
		//
		address |= v_C << 12; // a12
		address |= (1 ^ (Page2 & (1 ^ _80Store))) << 13; // a13
		address |= (Page2 & (1 ^ _80Store)) << 14; // a14
	}
	else
	{
		// N: text, so no higher address bits unless Apple ][, not Apple //e
		//
		if ((1) && // Apple ][? // FIX: check for Apple ][? (FB is most useful in old games)
			(kHPEClock <= h_clock) && // Y: HBL?
			(h_clock <= (kHClocks - 1)))
		{
			address |= 1 << 12; // Y: a12 (add $1000 to address!)
		}
	}

	// update VBL' state
	//
	if (v_4 & v_3) // VBL?
	{
		//CMemory::mState &= ~CMemory::kVBLBar; // Y: VBL' is false // FIX: MESS?
	}
	else
	{
		//CMemory::mState |= CMemory::kVBLBar; // N: VBL' is true // FIX: MESS?
	}

	return mess_ram[address]; // FIX: this seems to work, but is it right!?
}

/***************************************************************************
  driver init
***************************************************************************/
DRIVER_INIT( apple2 )
{
	state_save_register_UINT32("apple2", 0, "softswitch", &a2, 1);
	state_save_register_func_postload(apple2_updatevar);

	/* apple2 behaves much better when the default memory is zero */
	memset(mess_ram, 0, mess_ram_size);
}

/***************************************************************************
  machine init
***************************************************************************/
MACHINE_INIT( apple2 )
{
	/* --------------------------------------------- *
	 * set up the softswitch mask/set                *
	 * --------------------------------------------- */
	a2_mask = ~0;
	a2_set = 0;

	/* disable VAR_ROMSWITCH if the ROM is only 16k */
	if (memory_region_length(REGION_CPU1) < 0x8000)
		a2_mask &= ~VAR_ROMSWITCH;

	/* always internal ROM if no slots exist */
	if (!apple2_hasslots())
	{
		a2_mask &= ~VAR_SLOTC3ROM;
		a2_set |= VAR_INTCXROM;
	}

	if (mess_ram_size <= 64*1024)
		a2_mask &= ~(VAR_RAMRD | VAR_RAMWRT | VAR_80STORE | VAR_ALTZP | VAR_80COL);

	/* --------------------------------------------- */

	apple2_setvar(0, ~0);

	if (apple2_hasslots())
	{
		/* Slot 3 is funky - it isn't mapped like the other slot ROMs */
		cpu_setbank(3, &apple_rom[0x0100]);
		memcpy (apple2_slotrom(3), &apple_rom[0x0300], 0x100);
	}

	/* Use built-in slot ROM ($c800) */
	cpu_setbank(6, &apple_rom[0x0800]);

	AY3600_init();

	a2_speaker_state = 0;

	/* TODO: add more initializers as we add more slots */
	if (apple2_hasslots())
		mockingboard_init(4);

	apple2_slot6_init();

	joystick_x1_time = joystick_y1_time = 0;
	joystick_x2_time = joystick_y2_time = 0;
}

/***************************************************************************
  apple2_interrupt
***************************************************************************/
void apple2_interrupt(void)
{
	int irq_freq = 1;
	int scanline;

	profiler_mark(PROFILER_A2INT);

	scanline = cpu_getscanline();

	if (scanline > 190)
	{
		irq_freq --;
		if (irq_freq < 0)
			irq_freq = 1;

		/* We poll the keyboard periodically to scan the keys.  This is
		actually consistent with how the AY-3600 keyboard controller works. */
		AY3600_interrupt();

		if (irq_freq)
			cpu_set_irq_line(0, M6502_IRQ_LINE, PULSE_LINE);
	}

	force_partial_update(scanline);

	profiler_mark(PROFILER_END);
}

/***************************************************************************
  apple2_LC_ram1_w
***************************************************************************/
static WRITE_HANDLER ( apple2_LC_ram1_w )
{
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = (a2 & VAR_ALTZP) ? 0x10000 : 0x0000;
	mess_ram[0xc000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram2_w
***************************************************************************/
static WRITE_HANDLER ( apple2_LC_ram2_w )
{
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = (a2 & VAR_ALTZP) ? 0x10000 : 0x0000;
	mess_ram[0xd000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram_w
***************************************************************************/
static WRITE_HANDLER ( apple2_LC_ram_w )
{
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = (a2 & VAR_ALTZP) ? 0x10000 : 0x0000;
	mess_ram[0xe000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_mainram0200_w
  apple2_mainram0400_w
  apple2_mainram0800_w
  apple2_mainram2000_w
  apple2_mainram4000_w
***************************************************************************/
static WRITE_HANDLER ( apple2_mainram0200_w )
{
	offset += 0x200;
	mess_ram[offset] = data;
}

static WRITE_HANDLER ( apple2_mainram0400_w )
{
	offset += 0x400;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE_HANDLER ( apple2_mainram0800_w )
{
	offset += 0x800;
	mess_ram[offset] = data;
}

static WRITE_HANDLER ( apple2_mainram2000_w )
{
	offset += 0x2000;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE_HANDLER ( apple2_mainram4000_w )
{
	offset += 0x4000;
	mess_ram[offset] = data;
}

/***************************************************************************
  apple2_auxram0200_w
  apple2_auxram0400_w
  apple2_auxram0800_w
  apple2_auxram2000_w
  apple2_auxram4000_w
***************************************************************************/
static WRITE_HANDLER ( apple2_auxram0200_w )
{
	offset += 0x10200;
	mess_ram[offset] = data;
}

static WRITE_HANDLER ( apple2_auxram0400_w )
{
	offset += 0x10400;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE_HANDLER ( apple2_auxram0800_w )
{
	offset += 0x10800;
	mess_ram[offset] = data;
}

static WRITE_HANDLER ( apple2_auxram2000_w )
{
	offset += 0x12000;
	mess_ram[offset] = data;
	apple2_video_touch(offset);
}

static WRITE_HANDLER ( apple2_auxram4000_w )
{
	offset += 0x14000;
	mess_ram[offset] = data;
}

/***************************************************************************
  apple2_c00x_r
***************************************************************************/
READ_HANDLER ( apple2_c00x_r )
{
	data8_t result;

	/* Read the keyboard data and strobe */
	profiler_mark(PROFILER_C00X);
	result = AY3600_keydata_strobe_r();
	profiler_mark(PROFILER_END);

	return result;
}

/***************************************************************************
  apple2_c00x_w

  C000	80STOREOFF
  C001	80STOREON - use 80-column memory mapping
  C002	RAMRDOFF
  C003	RAMRDON - read from aux 48k
  C004	RAMWRTOFF
  C005	RAMWRTON - write to aux 48k
  C006	INTCXROMOFF
  C007	INTCXROMON
  C008	ALTZPOFF
  C009	ALTZPON - use aux ZP, stack and language card area
  C00A	SLOTC3ROMOFF
  C00B	SLOTC3ROMON - use external slot 3 ROM
  C00C	80COLOFF
  C00D	80COLON - use 80-column display mode
  C00E	ALTCHARSETOFF
  C00F	ALTCHARSETON - use alt character set
***************************************************************************/
WRITE_HANDLER ( apple2_c00x_w )
{
	UINT32 mask;
	mask = 1 << (offset / 2);
	apple2_setvar((offset & 1) ? mask : 0, mask);
}

/***************************************************************************
  apple2_c01x_r
***************************************************************************/
READ_HANDLER ( apple2_c01x_r )
{
	data8_t result = apple2_getfloatingbusvalue() & 0x7F;

	profiler_mark(PROFILER_C01X);

	LOG(("a2 softswitch_r: %04x\n", offset + 0xc010));
	switch (offset) {
	case 0x00:			result |= AY3600_anykey_clearstrobe_r();		break;
	case 0x01:			result |= (a2 & VAR_LCRAM2)		? 0x80 : 0x00;	break;
	case 0x02:			result |= (a2 & VAR_LCRAM)		? 0x80 : 0x00;	break;
	case 0x03:			result |= (a2 & VAR_RAMRD)		? 0x80 : 0x00;	break;
	case 0x04:			result |= (a2 & VAR_RAMWRT)		? 0x80 : 0x00;	break;
	case 0x05:			result |= (a2 & VAR_INTCXROM)	? 0x80 : 0x00;	break;
	case 0x06:			result |= (a2 & VAR_ALTZP)		? 0x80 : 0x00;	break;
	case 0x07:			result |= (a2 & VAR_SLOTC3ROM)	? 0x80 : 0x00;	break;
	case 0x08:			result |= (a2 & VAR_80STORE)	? 0x80 : 0x00;	break;
	case 0x09:			result |= input_port_0_r(0);	/* RDVBLBAR */	break;
	case 0x0A:			result |= (a2 & VAR_TEXT)		? 0x80 : 0x00;	break;
	case 0x0B:			result |= (a2 & VAR_MIXED)		? 0x80 : 0x00;	break;
	case 0x0C:			result |= (a2 & VAR_PAGE2)		? 0x80 : 0x00;	break;
	case 0x0D:			result |= (a2 & VAR_HIRES)		? 0x80 : 0x00;	break;
	case 0x0E:			result |= (a2 & VAR_ALTCHARSET)	? 0x80 : 0x00;	break;
	case 0x0F:			result |= (a2 & VAR_80COL)		? 0x80 : 0x00;	break;
	}

	profiler_mark(PROFILER_END);
	return result;
}

/***************************************************************************
  apple2_c01x_w
***************************************************************************/
WRITE_HANDLER( apple2_c01x_w )
{
	/* Clear the keyboard strobe - ignore the returned results */
	profiler_mark(PROFILER_C01X);
	AY3600_anykey_clearstrobe_r();
	profiler_mark(PROFILER_END);
}

/***************************************************************************
  apple2_c02x_r
***************************************************************************/
READ_HANDLER( apple2_c02x_r )
{
	apple2_c02x_w(offset, 0);
	return apple2_getfloatingbusvalue();
}

/***************************************************************************
  apple2_c02x_w
***************************************************************************/
WRITE_HANDLER( apple2_c02x_w )
{
	switch(offset) {
	case 0x08:
		apple2_setvar((a2 & VAR_ROMSWITCH) ^ VAR_ROMSWITCH, VAR_ROMSWITCH);
		break;
	}
}

/***************************************************************************
  apple2_c03x_r
***************************************************************************/
READ_HANDLER ( apple2_c03x_r )
{
	if (a2_speaker_state==0xFF)
		a2_speaker_state=0;
	else
		a2_speaker_state=0xFF;
	DAC_data_w(0,a2_speaker_state);
	return apple2_getfloatingbusvalue();
}

/***************************************************************************
  apple2_c03x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c03x_w )
{
	apple2_c03x_r(offset);
}

/***************************************************************************
  apple2_c05x_r
***************************************************************************/
READ_HANDLER ( apple2_c05x_r )
{
	UINT32 mask;

	/* ANx has reverse SET logic */
	if (offset >= 8)
		offset ^= 1;

	mask = 0x100 << (offset / 2);
	apple2_setvar((offset & 1) ? mask : 0, mask);
	return apple2_getfloatingbusvalue();
}

/***************************************************************************
  apple2_c05x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c05x_w )
{
	apple2_c05x_r(offset);
}

/***************************************************************************
  apple2_c06x_r
***************************************************************************/
READ_HANDLER ( apple2_c06x_r )
{
	int result = 0;
	switch (offset & 0x07) {
	case 0x01:
		/* Open-Apple/Joystick button 0 */
		result = pressed_specialkey(SPECIALKEY_BUTTON0);
		break;
	case 0x02:
		/* Closed-Apple/Joystick button 1 */
		result = pressed_specialkey(SPECIALKEY_BUTTON1);
		break;
	case 0x03:
		/* Joystick button 2. Later revision motherboards connected this to SHIFT also */
		result = pressed_specialkey(SPECIALKEY_BUTTON2);
		break;
	case 0x04:
		/* X Joystick 1 axis */
		result = timer_get_time() < joystick_x1_time;
		break;
	case 0x05:
		/* Y Joystick 1 axis */
		result = timer_get_time() < joystick_y1_time;
		break;
	case 0x06:
		/* X Joystick 2 axis */
		result = timer_get_time() < joystick_x2_time;
		break;
	case 0x07:
		/* Y Joystick 2 axis */
		result = timer_get_time() < joystick_y2_time;
		break;
	}
	return result ? 0x80 : 0x00;
}

/***************************************************************************
  apple2_c07x_r
***************************************************************************/
READ_HANDLER ( apple2_c07x_r )
{
	if (offset == 0)
	{
		joystick_x1_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(9);
		joystick_y1_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(10);
		joystick_x2_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(11);
		joystick_y2_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(12);
	}
	return 0;
}

/***************************************************************************
  apple2_c07x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c07x_w )
{
	apple2_c07x_r(offset);
}

/***************************************************************************
  apple2_c08x_r
***************************************************************************/
READ_HANDLER ( apple2_c08x_r )
{
	UINT32 val, mask;

	profiler_mark(PROFILER_C08X);
	LOG(("language card bankswitch read, offset: $c08%0x\n", offset));

	mask = VAR_LCWRITE | VAR_LCRAM | VAR_LCRAM2;
	val = 0;

	if (offset & 0x01)
		val |= VAR_LCWRITE;

	switch(offset & 0x03) {
	case 0x00:
	case 0x03:
		val |= VAR_LCRAM;
		break;
	}

	if (offset & 0x08)
		val |= VAR_LCRAM2;

	apple2_setvar(val, mask);

	profiler_mark(PROFILER_END);
	return 0;
}

/***************************************************************************
  apple2_c08x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c08x_w )
{
	apple2_c08x_r(offset);
}

/***************************************************************************
  apple2_c0xx_slot1_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot1_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot2_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot2_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot3_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot3_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot4_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot4_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot5_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot5_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot7_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot7_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot1_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot1_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot2_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot2_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot3_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot3_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot4_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot4_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot5_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot5_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot7_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot7_w )
{
	return;
}

/***************************************************************************
  apple2_slot1_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot1_w )
{
	return;
}

/***************************************************************************
  apple2_slot2_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot2_w )
{
	return;
}

/***************************************************************************
  apple2_slot3_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot3_w )
{
	return;
}

/***************************************************************************
  apple2_slot4_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot4_w )
{
	mockingboard_w (offset, data);
}

/***************************************************************************
  apple2_slot5_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot5_w )
{
	return;
}

/***************************************************************************
  apple2_slot7_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot7_w )
{
}

READ_HANDLER ( apple2_slot4_r )
{
	if (a2 & VAR_INTCXROM)
		/* Read the built-in ROM */
		return apple_rom[0x0400 + offset];
	else
		/* Read the slot ROM */
		return mockingboard_r (offset);
}

static void mockingboard_init (int slot)
{
	/* TODO: fix this */
	/* What follows is pure filth. It abuses the core like an angry pimp on a bad hair day. */

	/* Since we know that the Mockingboard has no code ROM, we'll copy into the slot ROM space
	   an image of the onboard ROM so that when an IRQ bankswitches to the onboard ROM, we read
	   the proper stuff. Without this, it will choke and try to use the memory handler above, and
	   fail miserably. That should really be fixed. I beg you -- if you are reading this comment,
	   fix this :) */
	memcpy (apple2_slotrom(slot), &apple_rom[0x0000 + (slot * 0x100)], 0x100);
}

static int mockingboard_r (int offset)
{
	static int flip1 = 0, flip2 = 0;

	switch (offset)
	{
		/* This is used to ID the board */
		case 0x04:
			flip1 ^= 0x08;
			return flip1;
			break;
		case 0x84:
			flip2 ^= 0x08;
			return flip2;
			break;
		default:
			LOG(("mockingboard_r unmapped, offset: %02x, pc: %04x\n", offset, activecpu_get_pc()));
			break;
	}
	return 0x00;
}

static void mockingboard_w (int offset, int data)
{
	static int latch0, latch1;

	LOG(("mockingboard_w, $%02x:%02x\n", offset, data));

	/* There is a 6522 in here which interfaces to the 8910s */
	switch (offset)
	{
		case 0x00: /* ORB1 */
			switch (data)
			{
				case 0x00: /* reset */
					AY8910_reset (0);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_0_w (0, latch0);
					break;
				case 0x07: /* set register */
					AY8910_control_port_0_w (0, latch0);
					break;
			}
			break;

		case 0x01: /* ORA1 */
			latch0 = data;
			break;

		case 0x02: /* DDRB1 */
		case 0x03: /* DDRA1 */
			break;

		case 0x80: /* ORB2 */
			switch (data)
			{
				case 0x00: /* reset */
					AY8910_reset (1);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_1_w (0, latch1);
					break;
				case 0x07: /* set register */
					AY8910_control_port_1_w (0, latch1);
					break;
			}
			break;

		case 0x81: /* ORA2 */
			latch1 = data;
			break;

		case 0x82: /* DDRB2 */
		case 0x83: /* DDRA2 */
			break;
	}
}

