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

UINT8 *apple2_slot_rom;
UINT8 *apple2_slot1;
UINT8 *apple2_slot2;
UINT8 *apple2_slot3;
UINT8 *apple2_slot4;
UINT8 *apple2_slot5;
UINT8 *apple2_slot6;
UINT8 *apple2_slot7;

/*UINT8 *apple2_rom;*/

APPLE2_STRUCT a2;

/* local */
static int a2_speaker_state;

static void mockingboard_init (int slot);
static int mockingboard_r (int offset);
static void mockingboard_w (int offset, int data);
static WRITE_HANDLER ( apple2_mainram_w );
static WRITE_HANDLER ( apple2_auxram_w );

static double joystick_x_time;
static double joystick_y_time;

/***************************************************************************
  apple2_getfloatingbusvalue
  preliminary floating bus video scanner code - look for comments with FIX:
***************************************************************************/
data8_t apple2_getfloatingbusvalue(void)
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
	Hires    = (a2.HIRES) ? 1 : 0;
	Mixed    = (a2.MIXED) ? 1 : 0;
	Page2    = (a2.PAGE2) ? 1 : 0;
	_80Store = (a2.STORE80) ? 1 : 0;

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
  apple2_init_machine
***************************************************************************/
MACHINE_INIT( apple2e )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	mess_ram = RAM;

	/* Init our language card banks to initially point to ROM */
	memory_set_bankhandler_w (1, 0, MWA_ROM);
	cpu_setbank (1, &RAM[0x21000]);
	memory_set_bankhandler_w (2, 0, MWA_ROM);
	cpu_setbank (2, &RAM[0x22000]);
	/* Use built-in slot ROM ($c100-$c7ff) */
	cpu_setbank (3, &RAM[0x20100]);
	/* Use main zp/stack */
	cpu_setbank (4, &RAM[0x0000]);
	/* Use main RAM */
	memory_set_bankhandler_w (5, 0, apple2_mainram_w);
	cpu_setbank (5, &RAM[0x0200]);
	/* Use built-in slot ROM ($c800) */
	cpu_setbank (6, &RAM[0x20800]);

	/* Slot 3 is funky - it isn't mapped like the other slot ROMs */
	memcpy (&RAM[0x24200], &RAM[0x20300], 0x100);

	AY3600_init();

	memset (&a2, 0, sizeof (APPLE2_STRUCT));
	a2_speaker_state = 0;

	/* TODO: add more initializers as we add more slots */
	mockingboard_init (4);
	apple2_slot6_init();

	joystick_x_time = joystick_y_time = 0;
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
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xc000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram2_w
***************************************************************************/
static WRITE_HANDLER ( apple2_LC_ram2_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xd000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram_w
***************************************************************************/
static WRITE_HANDLER ( apple2_LC_ram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xe000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_mainram_w
***************************************************************************/
WRITE_HANDLER ( apple2_mainram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	RAM[0x0200 + offset] = data;
	apple2_video_touch(0x0200 + offset);
}

/***************************************************************************
  apple2_auxram_w
***************************************************************************/
WRITE_HANDLER ( apple2_auxram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	RAM[0x10200 + offset] = data;
	apple2_video_touch(0x10200 + offset);
}

void apple2_slotrom_disable (int offset, int data)
{
//	a2_RDCXROM = 1;
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
***************************************************************************/
WRITE_HANDLER ( apple2_c00x_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	profiler_mark(PROFILER_C00X);

	switch (offset)
	{
		/* 80STOREOFF */
		case 0x00:		a2.STORE80 = 0x00;		break;
		/* 80STOREON - use 80-column memory mapping */
		case 0x01:		a2.STORE80 = 0x80;		break;
		/* RAMRDOFF */
		case 0x02:
			a2.RAMRD = 0x00;
			cpu_setbank (5, &RAM[0x0200]);
			break;
		/* RAMRDON - read from aux 48k */
		case 0x03:
			a2.RAMRD = 0x80;
			cpu_setbank (5, &RAM[0x10200]);
			break;
		/* RAMWRTOFF */
		case 0x04:
			a2.RAMWRT = 0x00;
			memory_set_bankhandler_w (5, 0, apple2_mainram_w);
			break;
		/* RAMWRTON - write to aux 48k */
		case 0x05:
			a2.RAMWRT = 0x80;
			memory_set_bankhandler_w (5, 0, apple2_auxram_w);
			break;
		/* INTCXROMOFF */
		case 0x06:
			a2.INTCXROM = 0x00;
			/* TODO: don't switch slot 3 */
			cpu_setbank (3, &RAM[0x24000]);
			break;
		/* INTCXROMON */
		case 0x07:
			a2.INTCXROM = 0x80;
			/* TODO: don't switch slot 3 */
			cpu_setbank (3, &RAM[0x20100]);
			break;
		/* ALTZPOFF */
		case 0x08:
			a2.ALTZP = 0x00;
			cpu_setbank (4, &RAM[0x00000]);
			if (a2.LC_RAM)
			{
				cpu_setbank (2, &RAM[0xe000]);
				if (a2.LC_RAM2)
				{
					cpu_setbank (1, &RAM[0xd000]);
				}
				else
				{
					cpu_setbank (1, &RAM[0xc000]);
				}
			}
			break;
		/* ALTZPON - use aux ZP, stack and language card area */
		case 0x09:
			a2.ALTZP = 0x80;
			cpu_setbank (4, &RAM[0x10000]);
			if (a2.LC_RAM)
			{
				cpu_setbank (2, &RAM[0x1e000]);
				if (a2.LC_RAM2)
				{
					cpu_setbank (1, &RAM[0x1d000]);
				}
				else
				{
					cpu_setbank (1, &RAM[0x1c000]);
				}
			}
			break;
		/* SLOTC3ROMOFF */
		case 0x0A:		a2.SLOTC3ROM = 0x00;	break;
		/* SLOTC3ROMON - use external slot 3 ROM */
		case 0x0B:		a2.SLOTC3ROM = 0x80;	break;
		/* 80COLOFF */
		case 0x0C:		a2.COL80 = 0x00;		break;
		/* 80COLON - use 80-column display mode */
		case 0x0D:		a2.COL80 = 0x80;		break;
		/* ALTCHARSETOFF */
		case 0x0E:		a2.ALTCHARSET = 0x00;	break;
		/* ALTCHARSETON - use alt character set */
		case 0x0F:		a2.ALTCHARSET = 0x80;	break;
	}

	LOG(("a2 softswitch_w: %04x\n", offset + 0xc000));
	profiler_mark(PROFILER_END);
}

/***************************************************************************
  apple2_c01x_r
***************************************************************************/
READ_HANDLER ( apple2_c01x_r )
{
	data8_t result = apple2_getfloatingbusvalue() & 0x7F;

	profiler_mark(PROFILER_C01X);

	LOG(("a2 softswitch_r: %04x\n", offset + 0xc010));
	switch (offset)
	{
		case 0x00:			result |= AY3600_anykey_clearstrobe_r();		break;
		case 0x01:			result |= a2.LC_RAM2;						break;
		case 0x02:			result |= a2.LC_RAM;							break;
		case 0x03:			result |= a2.RAMRD;							break;
		case 0x04:			result |= a2.RAMWRT;							break;
		case 0x05:			result |= a2.INTCXROM;						break;
		case 0x06:			result |= a2.ALTZP;							break;
		case 0x07:			result |= a2.SLOTC3ROM;						break;
		case 0x08:			result |= a2.STORE80;						break;
		case 0x09:			result |= input_port_0_r(0);	/* RDVBLBAR */	break;
		case 0x0A:			result |= a2.TEXT;							break;
		case 0x0B:			result |= a2.MIXED;							break;
		case 0x0C:			result |= a2.PAGE2;							break;
		case 0x0D:			result |= a2.HIRES;							break;
		case 0x0E:			result |= a2.ALTCHARSET;						break;
		case 0x0F:			result |= a2.COL80;							break;
	}

	profiler_mark(PROFILER_END);
	return 0;
}

/***************************************************************************
  apple2_c01x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c01x_w )
{
	/* Clear the keyboard strobe - ignore the returned results */
	profiler_mark(PROFILER_C01X);
	AY3600_anykey_clearstrobe_r();
	profiler_mark(PROFILER_END);
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
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
		/* Joystick/paddle pots */
		case 0x08:		a2.AN0   = 0x80; break;	/* AN0 has reverse SET logic */
		case 0x09:		a2.AN0   = 0x00; break;
		case 0x0A:		a2.AN1   = 0x80; break; /* AN1 has reverse SET logic */
		case 0x0B:		a2.AN1   = 0x00; break;
		case 0x0C:		a2.AN2   = 0x80; break; /* AN2 has reverse SET logic */
		case 0x0D:		a2.AN2   = 0x00; break;
		case 0x0E:		a2.AN3   = 0x80; break; /* AN3 has reverse SET logic */
		case 0x0F:		a2.AN3   = 0x00; break;
	}

	return apple2_getfloatingbusvalue();
}

/***************************************************************************
  apple2_c05x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c05x_w )
{
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
	}
}

/***************************************************************************
  apple2_c06x_r
***************************************************************************/
READ_HANDLER ( apple2_c06x_r )
{
	int result = 0;
	switch (offset) {
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
		/* X Joystick axis */
		result = timer_get_time() < joystick_x_time;
		break;
	case 0x05:
		/* Y Joystick axis */
		result = timer_get_time() < joystick_y_time;
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
		joystick_x_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(9);
		joystick_y_time = timer_get_time() + TIME_IN_USEC(12.0) * readinputport(10);
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
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	profiler_mark(PROFILER_C08X);
	LOG(("language card bankswitch read, offset: $c08%0x\n", offset));

	if ((offset & 0x01)==0x00)
	{
		memory_set_bankhandler_w (1, 0, MWA_ROM);
		memory_set_bankhandler_w (2, 0, MWA_ROM);
		a2.LC_WRITE = 0x00;
	}
	else
	{
		memory_set_bankhandler_w (2, 0, apple2_LC_ram_w);

		if ((offset & 0x08)==0x00)
			memory_set_bankhandler_w (1, 0, apple2_LC_ram2_w);
		else
			memory_set_bankhandler_w (1, 0, apple2_LC_ram1_w);
		a2.LC_WRITE = 0x80;
	}

	switch (offset & 0x03)
	{
		case 0x00:
		case 0x03:
			cpu_setbank (2, &RAM[0xe000 + aux_offset]);
			a2.LC_RAM = 0x80;
			if ((offset & 0x08)==0x00)
			{
				cpu_setbank (1, &RAM[0xd000 + aux_offset]);
				a2.LC_RAM2 = 0x80;
			}
			else
			{
				cpu_setbank (1, &RAM[0xc000 + aux_offset]);
				a2.LC_RAM2 = 0x00;
			}
			break;
		case 0x01:
		case 0x02:
			cpu_setbank (1, &RAM[0x21000]);
			cpu_setbank (2, &RAM[0x22000]);
			a2.LC_RAM = a2.LC_RAM2 = 0;
			break;
	}

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
	return;
}

READ_HANDLER ( apple2_slot4_r )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (a2.INTCXROM)
		/* Read the built-in ROM */
		return RAM[0x20400 + offset];
	else
		/* Read the slot ROM */
		return mockingboard_r (offset);
}

static void mockingboard_init (int slot)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* TODO: fix this */
	/* What follows is pure filth. It abuses the core like an angry pimp on a bad hair day. */

	/* Since we know that the Mockingboard has no code ROM, we'll copy into the slot ROM space
	   an image of the onboard ROM so that when an IRQ bankswitches to the onboard ROM, we read
	   the proper stuff. Without this, it will choke and try to use the memory handler above, and
	   fail miserably. That should really be fixed. I beg you -- if you are reading this comment,
	   fix this :) */
	memcpy (&RAM[0x24000 + (slot-1) * 0x100], &RAM[0x20000 + (slot * 0x100)], 0x100);
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

