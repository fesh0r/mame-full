/* This is a modification of the SystemVia.c code found in the xbeeb emulator.
 * In the Vectrex the Via is a central part which controls vector drawing,
 * the PSG, buttons, joystick and other things. In the first attempt I
 * stuffed many things in this file which souldn't be there, so this VIA
 * code is not of much use to other emulators.
 * 1998/6/10 Mathis Rosenhauer
 *
 * $Id: SystemVia.c,v 1.8 1996/10/01 00:33:03 james Exp $
 *
 * Copyright (c) James Fidell 1994, 1995, 1996.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of the copyright holder
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission. The
 * copyright holder makes no representations about the suitability of
 * this software for any purpose. It is provided "as is" without express
 * or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Modification History
 *
 * $Log: SystemVia.c,v $
 * Revision 1.8  1996/10/01 00:33:03  james
 * Created separate hardware reset code for each emulated unit and called
 * these from the main initialisation section of the code to do all of the
 * setup necessary.
 *
 * Revision 1.7  1996/09/25 19:19:57  james
 * Major overhaul of VIA emulation code :
 *
 *   Enabled toggling of PB7 in system VIA depending on ACR bit 6 and the
 *   one-shot/free-run mode of T1
 *
 *   Implemented User VIA T1 free-running mode.  Set the initial value of
 *   the User VIA ORA to 0x80.  Planetoid/Defender now works for the first
 *   time!
 *
 *   Corrected value returned by read from VIA T2CL and T2CH.  Frak! now
 *   works.
 *
 *   Set up dummy return for reads from the system VIA IRA and SR.
 *
 *   Implemented address wrap-around for memory-mapped registers in the VIA.
 *
 *   Set up dummy return for reads from the user VIA SR.
 *
 *   Implemented address wrap-around for memory-mapped registers in the VIA.
 *
 *   Updated 6522 VIA emulation to have correct initial values for VIA
 *   registers wherever possible.
 *
 *   Heavily modified 6522 VIA code to separate out the input/output
 *   registers and what is actually on the data pins.  This has the benefits
 *   of tidying up the whole VIA i/o emulation and not requiring any nasty
 *   configuration hacks to get software to work (apart from those that exist
 *   because of uncompleted emulation).
 *
 *   Tidied up 6522Via interrupt handling code.
 *
 * Revision 1.6  1996/09/24 23:05:44  james
 * Update copyright dates.
 *
 * Revision 1.5  1996/09/23 19:02:13  james
 * Major overhaul of the VIA emulation code.
 *
 * Revision 1.4  1996/09/22 21:00:54  james
 * Changed IRQ-handling code to be called only when I think an IRQ may have
 * happened.  Converted it all to #defines so that it can be compiled inline.
 *
 * Revision 1.3  1996/09/22 20:36:13  james
 * More conversions of exit(x) to FatalError()
 *
 * Revision 1.2  1996/09/21 22:13:51  james
 * Replaced "unsigned char" representation of 1 byte with "byteval".
 *
 * Revision 1.1  1996/09/21 17:20:41  james
 * Source files moved to src directory.
 *
 * Revision 1.1.1.1  1996/09/21 13:52:48  james
 * Xbeeb v0.1 initial release
 *
 *
 */

#include <stdio.h>
#include <unistd.h>

#include "vect_via.h"
#include "driver.h"
#include "vidhrdw/vectrex.h"
#include "cpuintrf.h"
#include "sndhrdw/psgintf.h"
#include "sndhrdw/dac.h"
#include "inptport.h"
#include "M6809/M6809.h"

static void				ClearPortAInterrupts ( void );
static void				ClearPortBInterrupts ( void );

ViaType Via;
int 	ViaTimer1;
int 	ViaTimer2;
byteval ViaPortAPinLevel;
byteval ViaPortBPinLevel;
unsigned char ViaTimer2InterruptEnable;
unsigned char ViaTimer1Continuous;
unsigned char ViaTimer1SetPB7;
unsigned char ViaTimer2PulseCount;
static unsigned char	PortALatchEnable;
static unsigned char	PortBLatchEnable;
static byteval		CA1Control, CA2Control, CB1Control, CB2Control;
static void *via_timer1_handle, *via_timer2_handle;

/* Vectrex specific vars */
static int cpu_cycles_t1, cpu_cycles, cpu_cycles_nt, select_dest, vectrex_dac;
static int vectrex_x_int_c, vectrex_y_int_c, vectrex_xy_int_offs, vectrex_brightness, vectrex_DAC_sound;
static int joy_state;
static int vectrex_x_int_c_save;

extern int T2_running;

/*
 * This is a hack to cover bits of the emulation that doesn't exist yet.
 * See where it's used in the code for more detail.
 */

#define	DEFAULT_PORT_B	0xf0

/* There are some delays introduced by the analog part of the Vectrex.
 * There is also a delay between the timers reaching zero
 * and the flags in IFR and PB7 going up. The net result is that
 * RAMP (PB7) is asserted a little bit longer than the timer is running.
 * This T1_IFR delay should be 1.5 from the specs but I rounded it up
 * to fit my vector drawing function. Actually it could even be a bit
 * higher.
 */
#define RAMP_DELAY      10
#define BLANK_DELAY      0
#define DAC_DELAY        0
#define MUX_DELAY        0
#define SH_DELAY         0
#define VIA_T1_IFR_DELAY 2

void
ResetVia(void)
{
	/*
	 * The 6522 resets all registers to zero when it is powered up
	 * or reset, except the counters and latches, which are random.
	 * The SR should be random, too, but it always appears to be 0xff
	 * on my machine.
	 */
	
	ViaPortAPinLevel = DEF_LOGIC;
	ViaPortBPinLevel = DEF_LOGIC;
	
	/*
	 * FIX ME
	 *
	 * IRB has the Speech unit INTERRUPT and READY signals hard-wired
	 * high and the joystick button press lines hardwared as if the buttons
	 * are not pressed.
	 */

	Via [ IRB ] = DEFAULT_PORT_B;
	Via [ IRA ] = 0x00;
	Via [ ORB ] = 0x00;
	Via [ ORA ] = 0x00;
	Via [ DDRB ] = 0x00;
	Via [ DDRA ] = 0x00;

	/*
	 * FIX ME
	 *
	 * Set these to random values...
	 */

	/*
	 * This takes care of the counters...
	 */

	ViaTimer1 = 0x1234;
	ViaTimer2 = 0x9876;

	Via [ T1LL ] = 0xaa;
	Via [ T1LH ] = 0x55;
	Via [ T2LL ] = 0xa5;

	/*
	 * FIX ME
	 *
	 * I'm not sure that Timer2InterruptEnable is needed.  In my docs. it
	 * states that the T2 interrupt can only be re-enabled once triggered
	 * by writing to T2CH or reading from T2CL.  It doesn't say if this is
	 * because doing so resets the interrupt flag, or whether they act
	 * independently.  I've assumed that they act independently because it
	 * seems safer to do so.
	 */

	ViaTimer2InterruptEnable = 1;

	Via [ SR ] = 0;

	/*
	 * FIX ME
	 *
	 * Need to handle the initial value of the SR control bits here, too.
	 */

	Via [ ACR ] = 0x00;
	ViaTimer1SetPB7 = 0;
	ViaTimer1Continuous = 0;
	ViaTimer2PulseCount = 0;
	PortALatchEnable = 0;
	PortBLatchEnable = 0;

	Via [ PCR ] = 0x0;
	CA1Control = CA2Control = CB1Control = CB2Control = 0x0;

	Via [ IER ] = 0x0;
	Via [ IFR ] = 0x0;

	/*
	 * FIX ME
	 *
	 * Some values should be forced at the hardware now the register
	 * values have been set -- write to the sound hardware for instance...
	 */

	return;
}

int ReadVia ( int addr )
{
	/*
	 * Have to handle the address wrap-around -- the VIA only has 16
	 * registers, but 32 memory locations are assigned to it.
	 */

	addr &= 0x0f;
	switch ( addr )
	{
	case ORB :
		/*
		 * Returns the bit in the ORB when the corresponding DDRB bit
		 * is set and either the pin level or the IRB bit when the
		 * corresponding DDRB bit is clear, for non-latching/latching
		 * modes respectively.
		 *
		 * In the default configuration, PB0-PB3 are outputs and
		 * PB4-PB7 are inputs.
		 */
		
		/*
		 * Call ClearPortBInterrupts to clear the relevant flags
		 * in the interrupt flag register.
		 */
		
		ClearPortBInterrupts();
		
		if ( PortBLatchEnable )
		{
#ifdef	INFO
			printf ( "read system VIA IRB = %02x\n", ( Via [ ORB ] &
								   Via [ DDRB ] ) | ( Via [ IRB ] &
										      ~Via [ DDRB ] ));
#endif
			return ((( Via [ ORB ] & Via [ DDRB ] ) |
				 ( Via [ IRB ] & ~Via [ DDRB ] )));
		}
		else
		{
#ifdef	INFO
			printf ( "read system VIA IRB = %02x\n", ( Via [ ORB ] &
								   Via [ DDRB ] ) | ( ViaPortBPinLevel &
										      ~Via [ DDRB ] ));
#endif
			/* Vectrex */
			/* Joystick handling. */
			
			vectrex_dac = (signed char)ViaPortAPinLevel;  /* this is meant as a signed char */
			joy_state = input_port_1_r (0);
			  
			select_dest = (ViaPortBPinLevel >> 1) & 0x3;
			switch (select_dest)
			{
			case 0:
			{
				if (vectrex_dac > 0)
				{
					if ( joy_state & 0x01 )
						ViaPortBPinLevel |= 0x20;
					else
						ViaPortBPinLevel &= ~0x20;
				}
				else
				{
					if ( joy_state & 0x02 )
						ViaPortBPinLevel &= ~0x20;
					else
						ViaPortBPinLevel |= 0x20;
				}
				break;
			}
			case 1:
			{
				if (vectrex_dac > 0)
				{
					if ( joy_state & 0x04 )
						ViaPortBPinLevel |= 0x20;
					else
						ViaPortBPinLevel &= ~0x20;
				}
				else
				{
					if ( joy_state & 0x08 )
						ViaPortBPinLevel &= ~0x20;
					else
						ViaPortBPinLevel |= 0x20;
				}
				break;
			}
			case 2:
			{
				if (vectrex_dac > 0)
				{
					if ( joy_state & 0x10 )
						ViaPortBPinLevel |= 0x20;
					else
						ViaPortBPinLevel &= ~0x20;
				}
				else
				{
					if ( joy_state & 0x20 )
						ViaPortBPinLevel &= ~0x20;
					else
						ViaPortBPinLevel |= 0x20;
				}
				break;
			}
			case 3:
			{
				if (vectrex_dac > 0)
				{
					if ( joy_state & 0x40 )
						ViaPortBPinLevel |= 0x20;
					else
						ViaPortBPinLevel &= ~0x20;
				}
				else
				{
					if ( joy_state & 0x80 )
						ViaPortBPinLevel &= ~0x20;
					else
						ViaPortBPinLevel |= 0x20;
				}
				break;
			}
			}

			/* Not sure about this from the specs. But it's needed by Spike.
			 * PB7 should be reflected by ORB if it's under T1 control.
			 * FIX ME */

			if (ViaTimer1SetPB7)
			{
				Via [ ORB ] &= ~0x80;
				Via [ ORB ] |= ViaPortBPinLevel & 0x80;
			}

			return ( Via [ ORB ] & Via [ DDRB ] ) | ( ViaPortBPinLevel & ~Via [ DDRB ] );
		}
		break;

	case ORA :
	case ORA_nh :
		/*
		 * FIX ME
		 *
		 * That AUG says that reading this should return the level on
		 * the PA pin if latching is disabled or the IRA bit if latching
		 * is enabled.  Seems strange that you can't read the ORA.  Can
		 * that be correct ?
		 */

		/*
		 * FIX ME
		 *
		 * Furthermore, I assume that reading the non-handshaking version
		 * of the register always returns the pin levels.
		 */


		ClearPortAInterrupts();

		if (( !PortBLatchEnable ) || ( addr == ORA_nh ))
		{
#ifdef  INFO
			printf ( "read system VIA IRA(nh) = %02x\n",
				 ViaPortAPinLevel );
#endif
			/* Vectrex */
			if ((!(ViaPortBPinLevel & 0x10)) && (ViaPortBPinLevel & 0x08)) 
				/* BDIR inactive, we can read the PSG. BC1 has to be active. */
				return AY8910_read_port_0_r (0);
				
			return ViaPortAPinLevel;
		}
		else
		{

			return Via [ IRA ];
		}
		break;

		/*
		 * FIX ME
		 *
		 * I assume that reading DDRA and DDRB just returns the current
		 * settings...
		 *
		 */

	case DDRB :
		return ( Via [ DDRB ] );
		break;

	case DDRA :
		return ( Via [ DDRA ] );
		break;

	case T1CL :
		ViaClearInterrupt ( INT_T1 );
		return ViaTimer1 & 0xff;
		break;

	case T1CH :
		return ViaTimer1 >> 8;
		break;

	case T1LL :
		return Via [ T1LL ];
		break;

	case T1LH :
		return Via [ T1LH ];
		break;

	case T2CL :
		ViaTimer2InterruptEnable = 1;
		ViaClearInterrupt ( INT_T2 );
		return ViaTimer2 & 0xff;
		break;

	case T2CH :
		return ViaTimer2 >> 8;
		break;

	case SR :
		return Via [ SR ];
		break;

	case PCR :
		/*
		 * FIX ME
		 *
		 * I assume that reading from this register just returns
		 * the value written.
		 */

		return Via [ PCR ];
		break;

	case ACR :
		/*
		 * FIX ME
		 *
		 * I assume that reading from this register just returns
		 * the value written.
		 */

		return Via [ ACR ];
		break;

	case IER :
		/*
		 * On reads bit 7 of the IER is always read as high.
		 */
#ifdef	INFO
		printf ( "read system VIA IER = %02x\n", Via [ IER ] | 0x80);
#endif
		return ( Via [ IER ] | 0x80 );
		break;

	case IFR :
#ifdef	INFO
		printf ( "read system VIA IFR = %02x\n", Via [ IFR ] );
#endif

		return ( Via [ IFR ] );
		break;
	}

	/* NOTREACHED */

	/*
	 * FIX ME
	 *
	 * Should give a fatal error here ?
	 */

	return 0xff;
}


void WriteVia ( int addr, int val )
{
	/*
	 * Remember to handle the address wrap-around...
	 */

	switch ( addr & 0xf )
	{
	case ORB :
	{

		/* Vectrex */
		if (!ViaTimer1SetPB7)    /* PB7 not under Timer 1 control */
		{
			if ((ViaPortBPinLevel & 0x80) && !(val & 0x80))  /* RAMP goes active (low) ? */
			{
				/* This is really dirty: We assume that we will be doing something like
				   text now. Therefore we clear the SR because it probably has stopped shifting
				   anyway. Otherwise we would draw crap before the first character */

				Via [ SR ] = 0;
				cpu_cycles_nt = cpu_gettotalcycles()+RAMP_DELAY;
			}

			if (!(ViaPortBPinLevel & 0x80) && (val & 0x80))  /* RAMP goes inactive (high) ? */
			{
				cpu_cycles = cpu_gettotalcycles()+RAMP_DELAY;
				vectrex_add_point(cpu_cycles-cpu_cycles_nt, vectrex_x_int_c, vectrex_y_int_c,
						  vectrex_xy_int_offs, Via [ SR ], vectrex_brightness);
			}
		}

		/*
		 * The values in ORB always change, but the pin levels only
		 * change as and when DDRB configures the pin as an output.
		 */


		if (ViaTimer1SetPB7)
		{
			Via [ ORB ] = (val & ~0x80) | (ViaPortBPinLevel & 0x80);
			ViaPortBPinLevel &= ((~Via [ DDRB ]) | 0x80);
			val &= Via [ DDRB ] & ~0x80;
		}
		else
		{
			Via [ ORB ] = val;
			ViaPortBPinLevel &= ~Via [ DDRB ];
			val &= Via [ DDRB ];
		}

		ViaPortBPinLevel |= val;

		/*
		 * FIX ME
		 *
		 * OK, so what do we do with the new values ?
		 */

		/*
		 * Clear the interrupt flags
		 */

		ClearPortBInterrupts();
#ifdef	INFO
		printf ( " VIA ORB = %02x, ", Via [ ORB ] );
		printf ( "Pin Levels = %02x\n", ViaPortBPinLevel );
#endif
			
		/* Vectrex */
		if (ViaPortBPinLevel & 0x10) /* BDIR active, PSG latches */
		{
			if (ViaPortBPinLevel & 0x08) /* BC1 (do we select a reg or write it ?) */
				AY8910_control_port_0_w (0, ViaPortAPinLevel);
			else
				AY8910_write_port_0_w (0, ViaPortAPinLevel);
		}
			      
			      
		/* The MUX could have been enabled, so check MUX where to route the DAC output.  */

		if ( !(ViaPortBPinLevel & 0x1) ) /* MUX enable */
		{
			vectrex_dac = (signed char)ViaPortAPinLevel;  /* this is meant as a signed value */
			select_dest = (ViaPortBPinLevel >> 1) & 0x3;
			switch (select_dest)
			{
			case 0:
			{
				vectrex_y_int_c = vectrex_dac;  /* DAC output goes into Y integrator */
				break;
			}
				
			case 1:
			{
				vectrex_xy_int_offs = vectrex_dac;
				break;
			}

			case 2:
			{
				vectrex_brightness = vectrex_dac;
				break;
			}

			case 3:
			{
				/* FIX ME. This is ulaw encoded or something like that so we need to
				   decode it first. */
				DAC_data_w(0,vectrex_dac+0x80); /* the dac driver expects unsigned samples */
				vectrex_DAC_sound=vectrex_dac;
				break;
			}
			}
		}
			
		break;
	}
	case ORA :
	case ORA_nh :
	{

		/*
		 * The values in ORA always change, but the pin levels only
		 * change as and when DDRA configures the pin as an output.
		 */

		Via [ ORA ] = val;

		val &= Via [ DDRA ];
		ViaPortAPinLevel &= ~Via [ DDRA ];
		ViaPortAPinLevel |= val;

#ifdef	INFO
		if ( addr == ORA )
			printf ( " VIA ORA = %02x, ", Via [ ORA ] );
		else
			printf ( " VIA ORA(nh) = %02x, ", Via [ ORA ] );
		printf ( "Pin Levels = %02x\n", ViaPortAPinLevel );
#endif

		/* Vectrex */

		vectrex_dac = (signed char)ViaPortAPinLevel;  /* this is meant as a signed char */

		if (!ViaTimer1SetPB7)    /* PB7 not under Timer 1 control */
		{
			if (!(ViaPortBPinLevel & 0x80))  /* RAMP active (low) ? */
			{
				cpu_cycles = cpu_gettotalcycles()+DAC_DELAY+SH_DELAY;
				vectrex_add_point(cpu_cycles-cpu_cycles_nt, vectrex_x_int_c, vectrex_y_int_c,
						  vectrex_xy_int_offs, Via [ SR ], vectrex_brightness);
				cpu_cycles_nt = cpu_cycles;
			}
		}

		vectrex_x_int_c = vectrex_dac;  /* DAC output always goes into X integrator */

		/* Check MUX where to put  ViaPortAPinLevel.
		   I think the Vectrex bios goes the other way round: 
		   first write to ORA and then enable the
		   MUX. But this is needed for joystick handling in some games */

		if ( !(ViaPortBPinLevel & 0x1) ) /* MUX is low active */
		{
			select_dest = (ViaPortBPinLevel >> 1) & 0x3;
			switch (select_dest)
			{
			case 0:
			{
				vectrex_y_int_c = vectrex_dac;  /* DAC output goes into Y integrator */
				break;
			}
				
			case 1:
			{
				vectrex_xy_int_offs = vectrex_dac;
				break;
			}

			case 2:
			{
				vectrex_brightness = vectrex_dac;
				break;
			}

			case 3:
			{
				/* FIX ME. This is ulaw encoded or something like that so we need to
				   decode it first. */
				DAC_data_w(0,vectrex_dac+0x80); /* the dac driver expects unsigned samples */
				vectrex_DAC_sound=vectrex_dac;
				break;
			}
			}
		}


		/*
		 * Clear the interrupt flags
		 */

		ClearPortAInterrupts();

		/*
		 * The OS *always* uses ORA_nh as it's output port, so
		 * if we're writing to ORA, barf here.  Otherwise, as I
		 * understand it, there's no need to do anything with the
		 * newly calculated values because the addressable latch
		 * and it's PB3 input will need to be tweaked before anything
		 * happens here.
		 *
		 * There's always one, though, isn't there ?  So we take
		 * account of the situation anyway...
		 */

		break;
			
	}
	case DDRB :
		/*
		 * FIX ME
		 *
		 * DEFAULT_PORT_B should at some stage be replaced by whatever
		 * values are put on the input lines by the hardware we're
		 * emulating as attached to user VIA port B.
		 */

		Via [ DDRB ] = val;

#ifdef  INFO
		printf ( " VIA DDRB = %02x\n", val );
#endif
		return;
		break;

	case DDRA :
		/*
		 * FIX ME
		 *
		 * DEF_LOGIC should at some stage be replaced by whatever
		 * values are put on the input lines by the hardware we're
		 * emulating as attached to user VIA port A.
		 */

		Via [ DDRA ] = val;
		ViaPortAPinLevel = ~val & DEF_LOGIC;
		ViaPortAPinLevel |= ( val & Via [ ORA ] );
#ifdef  INFO
		printf ( " VIA DDRA = %02x\n", val );
#endif
		return;
		break;

	case T1CL :
	case T1LL :
		/*
		 * The contents of this register will be written to T1CL
		 * when T1CH is written.
		 *
		 */

#ifdef  INFO
		printf ( " VIA T1CL/LL = %02x\n", val );
#endif

		Via [ T1LL ] = val;
		break;

	case T1LH :
		Via [ T1LH ] = val;
		break;

	case T1CH :
		/*
		 * When this register is written, the value is also loaded into
		 * T1LH, T1LL is copied to T1CL and the T1 interrupt flag in
		 * the IFR is cleared.  Also, PB7 goes low if it is an
		 * output
		 */

		Via [ T1CH ] = Via [ T1LH ] = val;
		Via [ T1CL ] = Via [ T1LL ];
		ViaTimer1 = Via [ T1CH ] * 256 + Via [ T1CL ];

		if (ViaTimer1)
		{
			ViaClearInterrupt ( INT_T1 );
			if ( ViaTimer1SetPB7 )
			{
				ViaPortBPinLevel &= ~0x80;
				Via [ORB] &= ~0x80;
				ViaTimer1SetPB7 = 1;
			}

			/* Vectrex */


			if (via_timer1_handle)
				timer_reset (via_timer1_handle, CYCLES_TO_TIME(0,ViaTimer1+VIA_T1_IFR_DELAY));
			else
				/* Attention: this screws up the CPU cycle counter. I think this is a bug */
				via_timer1_handle=timer_set (CYCLES_TO_TIME(0,ViaTimer1+VIA_T1_IFR_DELAY), 0, ViaTimer1Expire);
			
			cpu_cycles_t1 = cpu_gettotalcycles();    /* Get cpu cycles when timer 1 starts. Should
								  * be safe to do this here (after timer_set) */

			vectrex_x_int_c_save = vectrex_x_int_c;  /* Save the X integrator because it could
								  * be overwritten before the line was drawn.
								  * This happens in spike during speech
								  * output. */
		}
#ifdef  INFO
		printf ( " VIA T1CH = %02x\n", val );
#endif

		break;

	case T2CL :
		/*
		 * A write to here actually writes the T2 low-order latch.
		 */

		Via [ T2LL ] = val;
#ifdef  INFO
		printf ( " VIA T2CL = %02x\n", val );
#endif
		break;

	case T2CH :
		/*
		 * Writing this also copies T2LL to T2CL and clears the T2
		 * interrupt flag in the IFR.
		 */

		Via [ T2CH ] = val;
		Via [ T2CL ] = Via [ T2LL ];
		ViaTimer2 = Via [ T2CH ] * 256 + Via [ T2CL ];
		ViaTimer2InterruptEnable = 1;
		ViaClearInterrupt ( INT_T2 );

		/* Vectrex (not really)*/
		via_timer2_handle = timer_set (CYCLES_TO_TIME(0,ViaTimer2), ViaTimer2, ViaTimer2Update);

#ifdef  INFO
		printf ( " VIA T2CH = %02x\n", val );
#endif

		break;

	case SR :
		/* Vectrex */
		if (!ViaTimer1SetPB7)    /* PB7 not under Timer 1 control */
		{
			if (!(ViaPortBPinLevel & 0x80))  /* RAMP active (low) ? */
			{
				cpu_cycles = cpu_gettotalcycles()+BLANK_DELAY;
				if (cpu_cycles > cpu_cycles_nt)
				{
					vectrex_add_point(cpu_cycles-cpu_cycles_nt, vectrex_x_int_c, vectrex_y_int_c,
							  vectrex_xy_int_offs, Via [ SR ], vectrex_brightness);
					cpu_cycles_nt = cpu_cycles;
				}
			}
		}
		else
		{
			if ( !(Via [ IFR ] & 0x40) ) /* Timer 1 is running ? */
			{
				/* The program is updating the pattern while a line is drawn */
				cpu_cycles = cpu_gettotalcycles();


				if (ViaTimer1 > (cpu_cycles-cpu_cycles_t1)) /* The last update should be done by
									     * the timer */
				{
					ViaTimer1Update (cpu_cycles-cpu_cycles_t1);
					vectrex_add_point(cpu_cycles-cpu_cycles_t1, vectrex_x_int_c_save, vectrex_y_int_c,
							  vectrex_xy_int_offs, Via [ SR ], vectrex_brightness);
					cpu_cycles_t1=cpu_cycles;
				}
			}
			else
				if (val & 0x80)  
					/* We want dots. Do some optimisations. Dunno if it's worth */
					vectrex_add_point_dot(vectrex_brightness);
		}


		Via [ SR ] = val;
			  
#ifdef  INFO
		printf ( " VIA SR = %02x\n", val );
#endif

		break;

	case ACR :
		/*
		 * Controls what happens with the timers, shift register, PB7
		 * and latching.
		 *
		 */

		/*
		 * FIX ME
		 *
		 * Haven't dealt with the other bits of the register yet...
		 */

		if (!ViaTimer1SetPB7 && (val & 0x80)) /* If PB7 comes back under T1 control, we pull up PB7 */
			ViaPortBPinLevel |= 0x80;

		Via [ ACR ] = val;
		ViaTimer1SetPB7 = Via [ ACR ] & 0x80;
		ViaTimer1Continuous = Via [ ACR ] & 0x40;
		ViaTimer2PulseCount = Via [ ACR ] & 0x20;
		PortBLatchEnable = Via [ ACR ] & 0x02;
		PortALatchEnable = Via [ ACR ] & 0x01;
#ifdef	INFO
		printf ( " VIA ACR = %02x\n", Via [ ACR ] );
#endif
		break;

	case PCR :
		Via [ PCR ] = val;
		CA1Control = val & 0x01;
		CA2Control = ( val >> 1 ) & 0x07;
		CB1Control = ( val >> 4 ) & 0x01;
		CB2Control = ( val >> 5 ) & 0x07;

		if  (!(CA2Control & 0x1))    /* ~ZERO low ? Then zero integrators*/
			vectrex_zero_integrators(vectrex_xy_int_offs);

#ifdef  INFO
		printf ( " VIA PCR = %02x\n", val );
#endif

			  

#ifdef	WARNINGS
		fprintf(stderr, "WARNING:  VIA PCR = %02x\n",Via[PCR]);
#endif
		break;

	case IER :
		/*
		 * If bit 7 of the value written is zero, then each set bit
		 * clears the corresponding bit in the IER.
		 *
		 * If bit 7 is set, then each set bit in sets the corresponding
		 * bit in the IER.
		 *
		 */

		if ( val & 0x80 )
			Via [ IER ] |= ( val & 0x7f );
		else
			Via [ IER ] &= ( val ^ 0x7f );

#ifdef INFO
		printf ( " VIA IER = %02x\n", Via [ IER ] );
#endif

		/*
		 * Now check to see if we need to cause an IRQ.
		 */
		
		ViaSetInterrupt ( Via [ IFR ] );
		break;

	case IFR :
		ViaClearInterrupt ( val );
#ifdef	INFO
		printf ( " VIA IFR = %02x\n", Via [ IFR ] );
#endif
		break;

	}
	return;
}

void ViaClearInterrupt ( byteval val )
{
	byteval		new;

	/*
	 * If bits 0 to 6 are clear, then bit 7 is clear
	 * otherwise bit 7 is set.
	 *
	 * It is not possible to explicitly write to bit 7.
	 * Writing to a bit clears it.
	 */

	new = ( Via [ IFR ] & ~val ) & 0x7f;
	if ( new & Via [ IER ] )
		new |= INT_ANY;
	Via [ IFR ] = new;
	return;
}

void ViaSetInterrupt( byteval IFR_flag )
{
	m6809_Regs regs;

	/*
	 * It's not entirely clear how this works from the documentation
	 * but I suspect that flags will be set in the IFR even if the IER
	 * doesn't allow an interrupt.  All that the IER does is appear to
	 * control whether an IRQ condition occurs, at which point bit 7
	 * of the IFR will also be set.
	 */

	/*
	 * Set the flag for this type of interrupt.
	 */

	Via [ IFR ] |= IFR_flag;

	/*
	 * Cause an interrupt if we are allowed to do so.
	 */

	if ( Via [ IER ] & IFR_flag )
	{
		Via [ IFR ] |= INT_ANY;

		m6809_GetRegs(&regs);         /* There has to be a better way ... */

		if ((regs.cc & 0x10)==0)      /* Only do an interrupt when the IRQ flag is clear .
					       * I think there should be function in cpuintrf.c that
					       * causes an IRQ only if the corresponding flag in CC
					       * is clear and doesn't queue it as a pending IRQ */
			cpu_cause_interrupt(0, interrupt());
	}
	return;
}


void ViaSetCA1 ( byteval level )
{
	if ( CA1Control == level )
		ViaSetInterrupt ( INT_CA1 );
	return;
}

void ViaSetCA2 ( byteval level )
{
	if ( !level && ( CA2Control == HS2_NEGATIVE || CA2Control ==
			 HS2_NEGATIVE_IND ))
		ViaSetInterrupt ( INT_CA2 );
	else
		if ( level && ( CA2Control == HS2_POSITIVE || CA2Control ==
				HS2_POSITIVE_IND ))
			ViaSetInterrupt ( INT_CA2 );

	/*
	 * We don't have to worry about the other values of CA2Control -- they're
	 * outputs.
	 */

	return;
}


static void ClearPortBInterrupts( void )
{
	/*
	 * Always clear the CB1 active edge flag
	 */

	byteval		flags = INT_CB1;

	/*
	 * Clear the CB2 active edge flag unless the PCR indicates that
	 * CB2 is in ``independent'' mode.
	 */

	if ( CB2Control == HS2_POSITIVE_IND || CB2Control == HS2_NEGATIVE_IND )
		flags = INT_CB1 | INT_CB2;

	ViaClearInterrupt ( flags );
	return;
}

static void ClearPortAInterrupts( void )
{
	/*
	 * Always clear the CA1 active edge flag
	 */

	byteval		flags = INT_CA1;

	/*
	 * Clear the CA2 active edge flag unless the PCR indicates that
	 * CA2 is in ``independent'' mode.
	 */

	if ( CA2Control == HS2_POSITIVE_IND || CA2Control == HS2_NEGATIVE_IND )
		flags = INT_CA1 | INT_CA2;

	ViaClearInterrupt ( flags );
	return;
}

void ViaTimer1Expire ( int val )
{
	via_timer1_handle = NULL;
	ViaTimer1Update (ViaTimer1);
}

void ViaTimer1Update ( int val )
{
	/*
	 *  VIA Timer 1
	 *
	 * This can be in one of two modes -- free running or one shot, depending
	 * on the setting of bit 6 of the ACR.
	 *
	 */

	ViaTimer1 -= val;
	if ( ViaTimer1 <= 0 )
	{
		vectrex_add_point(val+ViaTimer1+VIA_T1_IFR_DELAY, vectrex_x_int_c_save, vectrex_y_int_c,
				  vectrex_xy_int_offs, Via [ SR ], vectrex_brightness);

		if ( ViaTimer1SetPB7 == 1 )
		{
			/*
			 * Need to set/invert the logic level of PB7, depending on
			 * whether we're in one-shot or free-running mode.
			 */

			if ( ViaTimer1Continuous )
				ViaPortBPinLevel ^= 0x80;
			else
			{
				ViaPortBPinLevel |= 0x80;
				Via [ ORB ] |= 0x80;
				/*
				 * Prevent further writes until the timer is reset.
				 */
				ViaTimer1SetPB7 = 2;
			}
		}

		/*
		 * In continuous mode, we also need to re-load the counter
		 * latches, otherwise just roll the counter over
		 */

		if ( ViaTimer1Continuous )
			ViaTimer1 += (Via[T1LL] + 256 * Via[T1LH]);
		else
			ViaTimer1 &= 0xffff;

		/*
		 * And generate the interrupt if we can.
		 */

		if ( !ViaCheckInterrupt ( INT_T1 ))
			ViaSetInterrupt ( INT_T1 );
	}
}

void ViaTimer2Update ( int val )
{
	/*
	 *  VIA Timer 2
	 *
	 * This is either a single timed interrupt, or a countdown on
	 * pulses on PB6, depending on bit 5 of the ACR.  If the timer is
	 * in pulse-counting mode, then that should be handled elsewhere.
	 *
	 */
	
	T2_running = 3;

	if ( !ViaTimer2PulseCount )
	{
		ViaTimer2 -= val;
		if ( ViaTimer2 <= 0 )
		{
			vectrex_screen_update();

			if ( ViaTimer2InterruptEnable )
			{
				/*
				 * FIX ME
				 *
				 * David Stacey commented this next line out to get
				 * Firetrack working.  I really need to look into that
				 * a little more.
				 */

				ViaSetInterrupt ( INT_T2 );
				ViaTimer2InterruptEnable = 0;
			}

			/*
			 * roll the counter over
			 */

			ViaTimer2 &= 0xffff;
		}
	}
}


