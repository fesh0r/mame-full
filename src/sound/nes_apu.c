/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** Permission granted for distribution under the MAME license by the
** author, Matthew Conte.  If you use these routines in your own
** projects, feel free to choose between the MAME license or the GNU
** LGPL, as outlined below.
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nes_apu.c
**
** NES APU emulation
** $Id: nes_apu.c,v 1.10 2000/09/12 20:39:47 hjb Exp $
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "driver.h"
#include "nes_apu.h"
#include "cpu/m6502/m6502.h"

#define log_printf logerror
#define ASSERT assert

#define APU_OVERSAMPLE
#define APU_VOLUME_DECAY(x)  x -= x/128


/* increase triangle volume relative to noise + dmc ? */
#if 1
#define APU_MIX_RECTANGLE	256
#define APU_MIX_TRIANGLE	384
#define APU_MIX_NOISE		192
#define APU_MIX_DMC 		192
#else
#define APU_MIX_RECTANGLE	256
#define APU_MIX_TRIANGLE	256
#define APU_MIX_NOISE		256
#define APU_MIX_DMC 		256
#endif

/* active APU */
static apu_t apu;

/* can be overwritten */
static double apu_baseclock = APU_BASECLOCK;

/* noise lookups for both modes */
static INT8 *noise_long_lut;
static INT8 *noise_short_lut;

/* vblank length table used for rectangles, triangle, noise */
static const UINT8 vbl_length[32] =
{
	 5,   127,
	10,   1,
	19,   2,
	40,   3,
	80,   4,
	30,   5,
	 7,   6,
	13,   7,
	 6,   8,
	12,   9,
	24,  10,
	48,  11,
	96,  12,
	36,  13,
	 8,  14,
	16,  15
};

/* noise frequency lookup table */
static const int noise_divisor[16] =
{
	  4,	8,	 16,   32,	 64,   96,	128,  160,
	202,  254,	380,  508,	762, 1016, 2034, 4068
};

/* DMC transfer freqs */
const int dmc_clocks[16] =
{
	428, 380, 340, 320, 286, 254, 226, 214,
	190, 160, 142, 128, 106,  85,  72,	54
};

/* ratios of pos/neg pulse for rectangle waves */
static const int duty_flip[4] = { 2, 4, 8, 12 };


void apu_setcontext(apu_t *src_apu)
{
	if (src_apu)
		apu = *src_apu;
}

int apu_getcontext(apu_t *dst_apu)
{
	if (dst_apu)
		*dst_apu = apu;
	return sizeof(apu_t);
}

void apu_setchan(int chan, BOOLEAN enabled)
{
	apu.mix_enable &= ~(1 << chan);
	if (enabled)
		apu.mix_enable |= 1 << chan;
}

/* emulation of the 15-bit shift register the
** NES uses to generate pseudo-random series
** for the white noise channel
*/
static void shift_register15(INT8 *buf, int count)
{
	static int sreg = 0x4000;
	int bit0, bit1, bit6, bit14;

	if (count == APU_NOISE_93)
	{
		while (count--)
		{
			bit0 = sreg & 1;
			bit6 = (sreg & 0x40) >> 6;
			bit14 = (bit0 ^ bit6);
			sreg >>= 1;
			sreg |= (bit14 << 14);
			*buf++ = bit0 ^ 1;
		}
	}
	else /* 32K noise */
	{
		while (count--)
		{
			bit0 = sreg & 1;
			bit1 = (sreg & 2) >> 1;
			bit14 = (bit0 ^ bit1);
			sreg >>= 1;
			sreg |= (bit14 << 14);
			*buf++ = bit0 ^ 1;
		}
	 }
}

/* RECTANGLE WAVE
** ==============
** reg0: 0-3=volume, 4=envelope, 5=hold, 6-7=duty cycle
** reg1: 0-2=sweep shifts, 3=sweep inc/dec, 4-6=sweep length, 7=sweep on
** reg2: 8 bits of freq
** reg3: 0-2=high freq, 7-3=vbl length counter
*/
#ifdef APU_OVERSAMPLE

#define make_apu_rectangle(ch)												  \
INLINE INT32 apu_rectangle_##ch(void)										  \
{																			  \
	INT32 output;															  \
	INT32 total = 0;														  \
	int num_times = 0;														  \
																			  \
	APU_VOLUME_DECAY(apu.rectangle[ch].output_vol); 						  \
																			  \
	if (FALSE == apu.rectangle[ch].enabled || 0 == apu.rectangle[ch].vbl_length) \
		return apu.rectangle[ch].output_vol;								  \
																			  \
	/* vbl length counter */												  \
	if (FALSE == apu.rectangle[ch].holdnote)								  \
		apu.rectangle[ch].vbl_length--; 									  \
																			  \
	if (apu.rectangle[ch].fixed_envelope)									  \
	{																		  \
		output = APU_MIX_RECTANGLE * apu.rectangle[ch].volume;				  \
	}																		  \
	else																	  \
	{																		  \
		/* envelope decay at a rate of (env_delay + 1) / 240 secs */		  \
		apu.rectangle[ch].env_phase -= apu.rectangle[ch].env_delay; 		  \
		while (apu.rectangle[ch].env_phase < 0) 							  \
		{																	  \
			apu.rectangle[ch].env_phase += apu.sample_rate; 				  \
			if (apu.rectangle[ch].holdnote) 								  \
				apu.rectangle[ch].env_vol = (apu.rectangle[ch].env_vol - 1) & 0x0F; \
			else if (apu.rectangle[ch].env_vol < 0x0F)						  \
				apu.rectangle[ch].env_vol++;								  \
		}																	  \
		output = APU_MIX_RECTANGLE * (apu.rectangle[ch].env_vol ^ 0x0F);	  \
	}																		  \
																			  \
	/* frequency sweeping at a rate of (sweep_delay + 1) / 120 secs */		  \
	if (apu.rectangle[ch].sweep_on) 										  \
	{																		  \
		apu.rectangle[ch].sweep_phase -= apu.rectangle[ch].sweep_delay; 	  \
		while (apu.rectangle[ch].sweep_phase < 0)							  \
		{																	  \
			apu.rectangle[ch].sweep_phase += apu.sample_rate;				  \
																			  \
			if (apu.rectangle[ch].sweep_inc) /* ramp up */					  \
			{																  \
				/* rectangle 0 uses a complement addition for sweep 		  \
				** increases, while rectangle 1 uses subtraction			  \
				*/															  \
				if (ch == 0)												  \
					apu.rectangle[ch].divisor += ~(apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
				else														  \
					apu.rectangle[ch].divisor -= (apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
			}																  \
			else /* ramp down */											  \
			{																  \
				apu.rectangle[ch].divisor += (apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
			}																  \
			if (apu.rectangle[ch].divisor > 0)								  \
				apu.rectangle[ch].freq = apu_baseclock / apu.rectangle[ch].divisor; \
		}																	  \
	}																		  \
																			  \
	apu.rectangle[ch].accu -= apu.rectangle[ch].freq;						  \
	if (apu.rectangle[ch].accu >= 0)										  \
		return apu.rectangle[ch].output_vol;								  \
																			  \
	while (apu.rectangle[ch].accu < 0)										  \
	{																		  \
		apu.rectangle[ch].accu += apu.sample_rate;							  \
		apu.rectangle[ch].adder = (apu.rectangle[ch].adder + 1) & 0x0F; 	  \
																			  \
		if (apu.rectangle[ch].adder < apu.rectangle[ch].duty_flip)			  \
			total += output;												  \
		else																  \
			total -= output;												  \
																			  \
		num_times++;														  \
	}																		  \
																			  \
	apu.rectangle[ch].output_vol = total / num_times;						  \
	return apu.rectangle[ch].output_vol;									  \
}

#else	/* !APU_OVERSAMPLE */

#define make_apu_rectangle(ch)												  \
INLINE INT32 apu_rectangle_##ch(void)										  \
{																			  \
	INT32 output;															  \
																			  \
	APU_VOLUME_DECAY(apu.rectangle[ch].output_vol); 						  \
																			  \
	if (FALSE == apu.rectangle[ch].enabled || 0 == apu.rectangle[ch].vbl_length) \
		return apu.rectangle[ch].output_vol;								  \
																			  \
	/* vbl length counter */												  \
	if (FALSE == apu.rectangle[ch].holdnote)								  \
		apu.rectangle[ch].vbl_length--; 									  \
																			  \
	if (apu.rectangle[ch].fixed_envelope)									  \
	{																		  \
		output = APU_MIX_RECTANGLE * apu.rectangle[ch].volume;				  \
	}																		  \
	else																	  \
	{																		  \
		/* envelope decay at a rate of (env_delay + 1) / 240 secs */		  \
		apu.rectangle[ch].env_phase -= apu.rectangle[ch].env_delay; 		  \
		while (apu.rectangle[ch].env_phase < 0) 							  \
		{																	  \
			apu.rectangle[ch].env_phase += apu.sample_rate; 				  \
			if (apu.rectangle[ch].holdnote) 								  \
				apu.rectangle[ch].env_vol = (apu.rectangle[ch].env_vol - 1) & 0x0F; \
			else if (apu.rectangle[ch].env_vol < 0x0F)						  \
				apu.rectangle[ch].env_vol++;								  \
		}																	  \
		output = APU_MIX_RECTANGLE * (apu.rectangle[ch].env_vol ^ 0x0F);	  \
	}																		  \
																			  \
	/* frequency sweeping at a rate of (sweep_delay + 1) / 120 secs */		  \
	if (apu.rectangle[ch].sweep_on) 										  \
	{																		  \
		apu.rectangle[ch].sweep_phase -= apu.rectangle[ch].sweep_delay; 	  \
		while (apu.rectangle[ch].sweep_phase < 0)							  \
		{																	  \
			apu.rectangle[ch].sweep_phase += apu.sample_rate;				  \
																			  \
			if (apu.rectangle[ch].sweep_inc) /* ramp up */					  \
			{																  \
				/* rectangle 0 uses a complement addition for sweep 		  \
				** increases, while rectangle 1 uses subtraction			  \
				*/															  \
				if (ch == 0)												  \
					apu.rectangle[ch].divisor += ~(apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
				else														  \
					apu.rectangle[ch].divisor -= (apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
			}																  \
			else /* ramp down */											  \
			{																  \
				apu.rectangle[ch].divisor += (apu.rectangle[ch].divisor >> apu.rectangle[ch].sweep_shifts); \
			}																  \
			if (apu.rectangle[ch].divisor > 0)								  \
				apu.rectangle[ch].freq = apu_baseclock / apu.rectangle[ch].divisor; \
		}																	  \
	}																		  \
																			  \
	if (apu.rectangle[ch].freq > apu.sample_rate)							  \
		return apu.rectangle[ch].output_vol;								  \
																			  \
	apu.rectangle[ch].accu -= apu.rectangle[ch].freq;						  \
	if (apu.rectangle[ch].accu >= 0)										  \
		return apu.rectangle[ch].output_vol;								  \
																			  \
	while (apu.rectangle[ch].accu < 0)										  \
	{																		  \
		apu.rectangle[ch].accu += apu.sample_rate;							  \
		apu.rectangle[ch].adder = (apu.rectangle[ch].adder + 1) & 0x0F; 	  \
		if (0 == apu.rectangle[ch].adder)									  \
			apu.rectangle[ch].output_vol = output;							  \
		else if (apu.rectangle[ch].adder == apu.rectangle[ch].duty_flip)	  \
			apu.rectangle[ch].output_vol = -output; 						  \
	}																		  \
																			  \
	return apu.rectangle[ch].output_vol;									  \
}

#endif

/* create two instances of the apu_rectangle function (for speed) */
make_apu_rectangle(0)
make_apu_rectangle(1)

/* TRIANGLE WAVE
** =============
** reg0: 7=holdnote, 6-0=linear length counter
** reg2: low 8 bits of frequency
** reg3: 7-3=vbl length counter, 2-0=high 3 bits of frequency
*/

static INT32 apu_triangle(void)
{
	APU_VOLUME_DECAY(apu.triangle.output_vol);

	if (FALSE == apu.triangle.enabled ||
		0 == apu.triangle.vbl_length ||
		0 == apu.triangle.linear_length)
		return apu.triangle.output_vol;

	if (FALSE == apu.triangle.holdnote)
	{
		if (apu.triangle.vbl_length > 0)
			apu.triangle.vbl_length--;
		if (apu.triangle.linear_length > 0)
			apu.triangle.linear_length--;
	}

#ifndef APU_OVERSAMPLE
    if (apu.triangle.freq > apu.sample_rate)    /* inaudible */
		return apu.triangle.output_vol;
#endif

	apu.triangle.accu -= apu.triangle.freq;
	while (apu.triangle.accu < 0)
	{
		apu.triangle.accu += apu.sample_rate;
		apu.triangle.adder = (apu.triangle.adder + 1) & 0x1F;
		if (apu.triangle.adder & 0x10)
			apu.triangle.output_vol += APU_MIX_TRIANGLE;
		else
			apu.triangle.output_vol -= APU_MIX_TRIANGLE;
	}

	return apu.triangle.output_vol;
}

/* WHITE NOISE CHANNEL
** ===================
** reg0: 0-3=volume, 4=envelope, 5=hold
** reg2: 7=small(93 byte) sample,3-0=freq lookup
** reg3: 7-4=vbl length counter
*/

#ifdef APU_OVERSAMPLE

static INT32 apu_noise(void)
{
	INT32 output;
	int num_times = 0;
	INT32 total = 0;


	APU_VOLUME_DECAY(apu.noise.output_vol);

	if (FALSE == apu.noise.enabled || 0 == apu.noise.vbl_length)
		return apu.noise.output_vol;

	/* vbl length counter */
	if (FALSE == apu.noise.holdnote)
		apu.noise.vbl_length--;

	if (apu.noise.fixed_envelope)
	{
		output = APU_MIX_NOISE * apu.noise.volume;	/* fixed volume */
	}
	else
	{
		/* envelope decay at a rate of (env_delay + 1) / 240 secs */
		apu.noise.env_phase -= apu.noise.env_delay;
		while (apu.noise.env_phase < 0)
		{
			apu.noise.env_phase += apu.sample_rate;
			if (apu.noise.holdnote)
				apu.noise.env_vol = (apu.noise.env_vol - 1) & 0x0F;
			else if (apu.noise.env_vol < 0x0F)
				apu.noise.env_vol++;
		}
		output = APU_MIX_NOISE * (apu.noise.env_vol ^ 0x0F);
	}

	/* for noise frequencies above the hablf sample rate reduce the output */
	if (apu.noise.freq > apu.sample_rate / 2)
		output /= (int)(apu.noise.freq * 8 / apu.sample_rate);

	apu.noise.accu -= apu.noise.freq;
	if (apu.noise.accu >= 0)
		return apu.noise.output_vol;

	while (apu.noise.accu < 0)
	{
		apu.noise.accu += apu.sample_rate;
		apu.noise.cur_pos++;

		if (apu.noise.short_sample)
		{
			if (APU_NOISE_93 == apu.noise.cur_pos)
				apu.noise.cur_pos = 0;
			if (noise_short_lut[apu.noise.cur_pos])
				total += output;
			else
				total -= output;
		}
		else
		{
			if (APU_NOISE_32K == apu.noise.cur_pos)
				apu.noise.cur_pos = 0;
			if (noise_long_lut[apu.noise.cur_pos])
				total += output;
			else
                total -= output;
        }
		num_times++;
	}

	apu.noise.output_vol = total / num_times;
	return apu.noise.output_vol;
}

#else	/* !APU_OVERSAMPLE */

static INT32 apu_noise(void)
{
	INT32 output;
	INT32 skip;

	APU_VOLUME_DECAY(apu.noise.output_vol);

	if (FALSE == apu.noise.enabled || 0 == apu.noise.vbl_length)
		return apu.noise.output_vol;

	/* vbl length counter */
	if (FALSE == apu.noise.holdnote)
		apu.noise.vbl_length--;

	if (apu.noise.fixed_envelope)
	{
		output = APU_MIX_NOISE * apu.noise.volume; /* fixed volume */
	}
	else
	{
		/* envelope decay at a rate of (env_delay + 1) / 240 secs */
		apu.noise.env_phase -= apu.noise.env_delay;
		while (apu.noise.env_phase < 0)
		{
			apu.noise.env_phase += apu.sample_rate;
			if (apu.noise.holdnote)
				apu.noise.env_vol = (apu.noise.env_vol - 1) & 0x0F;
			else if (apu.noise.env_vol < 0x0F)
				apu.noise.env_vol++;
		}
		output = APU_MIX_NOISE * (apu.noise.env_vol ^ 0x0F);
	}

	/* for noise frequencies above half the sample rate reduce the output */
	if (apu.noise.freq > apu.sample_rate / 2)
		output /= (int)(apu.noise.freq * 8 / apu.sample_rate);

	apu.noise.accu -= apu.noise.freq;
	if (apu.noise.accu >= 0)
		return apu.noise.output_vol;

	skip = -apu.noise.accu / apu.sample_rate;
	apu.noise.accu += skip * apu.sample_rate;
	apu.noise.cur_pos += skip;
	if (apu.noise.short_sample)
	{
		apu.noise.cur_pos %= APU_NOISE_93;
		apu.noise.output_vol = noise_short_lut[apu.noise.cur_pos] ? output : -output;
	}
	else
	{
		apu.noise.cur_pos %= APU_NOISE_32K;
		apu.noise.output_vol = noise_long_lut[apu.noise.cur_pos] ? output : -output;
	}

	return apu.noise.output_vol;
}

#endif

INLINE void apu_dmcreload(void)
{
	apu.dmc.address = apu.dmc.cached_addr;
	apu.dmc.dmalength = apu.dmc.cached_dmalength;
	apu.dmc.irq_occurred = FALSE;
}

/* DELTA MODULATION CHANNEL
** =========================
** reg0: 7=irq gen, 6=looping, 3-0=pointer to clock table
** reg1: output dc level, 6 bits unsigned
** reg2: 8 bits of 64-byte aligned address offset : $C000 + (value * 64)
** reg3: length, (value * 16) + 1
*/
static INT32 apu_dmc(void)
{
	int delta_bit;

	APU_VOLUME_DECAY(apu.dmc.output_vol);

	/* only process when channel is alive */
	if (FALSE == apu.dmc.enabled || apu.dmc.dmalength == 0)
		return apu.dmc.output_vol;

	apu.dmc.accu -= apu.dmc.freq;
	while (apu.dmc.accu < 0)
	{
		apu.dmc.accu += apu.sample_rate;

		delta_bit = (apu.dmc.dmalength & 7) ^ 7;

		if (7 == delta_bit)
		{
			apu.dmc.cur_byte = cpunum_readmem(apu.cpunum, apu.dmc.address);

			/* steal a cycle from CPU */
			/* nes6502_burn(1); */

			if (0xFFFF == apu.dmc.address)
				apu.dmc.address = 0x8000;
			else
				apu.dmc.address++;
		 }

		if (--apu.dmc.dmalength == 0)
		{
			/* if loop bit set, we're cool to retrigger sample */
			if (apu.dmc.looping)
				apu_dmcreload();
			else
			{
				/* check to see if we should generate an irq */
				if (apu.dmc.irq_gen)
				{
					apu.dmc.irq_occurred = TRUE;
					cpu_set_irq_line(apu.cpunum, 0, PULSE_LINE);
				}
				/* bodge for timestamp queue */
				apu.dmc.enabled = FALSE;
				break;
			}
		}

		/* positive delta */
		if (apu.dmc.cur_byte & (1 << delta_bit))
		{
			if (apu.dmc.regs[1] < 0x7D)
			{
				apu.dmc.regs[1] += 2;
				apu.dmc.output_vol += APU_MIX_DMC;
			}
		}
		/* negative delta */
		else
		{
			if (apu.dmc.regs[1] > 1)
			{
				apu.dmc.regs[1] -= 2;
				apu.dmc.output_vol -= APU_MIX_DMC;
			}
		}
	}

	return apu.dmc.output_vol;
}


void apu_write(UINT32 address, UINT8 value)
{
	int chan;

	switch (address)
	{
	/* rectangles */
	case APU_WRA0:
	case APU_WRB0:
        chan = (address & 4) ? 1 : 0;
		log_printf("APU #%d WR%c0 $%02x (PC=$%04x)\n", apu.cpunum, 'A'+chan, value, cpu_get_pc());
        apu.rectangle[chan].regs[0] = value;
		apu.rectangle[chan].volume = value & 0x0F;
		apu.rectangle[chan].env_delay = apu.sample_rate / 240;
		apu.rectangle[chan].holdnote = (value & 0x20) ? TRUE : FALSE;
		apu.rectangle[chan].fixed_envelope = (value & 0x10) ? TRUE : FALSE;
		apu.rectangle[chan].duty_flip = duty_flip[value >> 6];
		log_printf("   volume         %d\n", apu.rectangle[chan].volume);
		log_printf("   env_delay      %d\n", apu.rectangle[chan].env_delay);
		log_printf("   holdnote       %d\n", apu.rectangle[chan].holdnote);
		log_printf("   fixed_env      %d\n", apu.rectangle[chan].fixed_envelope);
		log_printf("   duty_flip      %d\n", apu.rectangle[chan].duty_flip);
        break;

	case APU_WRA1:
	case APU_WRB1:
		chan = (address & 4) ? 1 : 0;
		log_printf("APU #%d WR%c1 $%02x (PC=$%04x)\n", apu.cpunum, 'A'+chan, value, cpu_get_pc());
        apu.rectangle[chan].regs[1] = value;
		apu.rectangle[chan].sweep_shifts = value & 7;
		apu.rectangle[chan].sweep_on = (value & 0x80) ? TRUE : FALSE;
		apu.rectangle[chan].sweep_delay = apu.sample_rate * (((value >> 4) & 7) + 1) / 120;
		apu.rectangle[chan].sweep_inc = (value & 0x08) ? TRUE : FALSE;
		log_printf("   sweep_shift    %d\n", apu.rectangle[chan].sweep_shifts);
		log_printf("   sweep_on       %d\n", apu.rectangle[chan].sweep_on);
		log_printf("   sweep_delay    %d\n", apu.rectangle[chan].sweep_delay);
		log_printf("   sweep_inc      %d\n", apu.rectangle[chan].sweep_inc);
        break;

	case APU_WRA2:
	case APU_WRB2:
		chan = (address & 4) ? 1 : 0;
		log_printf("APU #%d WR%c2 $%02x (PC=$%04x)\n", apu.cpunum, 'A'+chan, value, cpu_get_pc());
        apu.rectangle[chan].regs[2] = value;
		apu.rectangle[chan].divisor = 256 * (apu.rectangle[chan].regs[3] & 7) + value + 1;
		apu.rectangle[chan].freq = apu_baseclock / apu.rectangle[chan].divisor;
		log_printf("   divisor        %d\n", apu.rectangle[chan].divisor);
		log_printf("   freq           %f\n", apu.rectangle[chan].freq);
        break;

	case APU_WRA3:
	case APU_WRB3:
		chan = (address & 4) ? 1 : 0;
		log_printf("APU #%d WR%c3 $%02x (PC=$%04x)\n", apu.cpunum, 'A'+chan, value, cpu_get_pc());
        apu.rectangle[chan].regs[3] = value;
		apu.rectangle[chan].divisor = 256 * (value & 7) + apu.rectangle[chan].regs[2] + 1;
		apu.rectangle[chan].freq = apu_baseclock / apu.rectangle[chan].divisor;
		apu.rectangle[chan].vbl_length = apu.sample_rate * apu.refresh_rate / vbl_length[value >> 3];
		apu.rectangle[chan].env_vol = 0;	/* reset envelope */
		apu.rectangle[chan].env_phase = 0;	/* reset envelope accu */
		apu.rectangle[chan].adder = 0;
		log_printf("   divisor        %d\n", apu.rectangle[chan].divisor);
		log_printf("   freq           %f\n", apu.rectangle[chan].freq);
		log_printf("   vbl_length     %d\n", apu.rectangle[chan].vbl_length);
        break;

	/* triangle */
	case APU_WRC0:
		log_printf("APU #%d WRC0 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.triangle.regs[0] = value;
		apu.triangle.linear_length = apu.sample_rate * ((value & 0x7f) + 1) / 128 / 4;
		apu.triangle.holdnote = (value & 0x80) ? TRUE : FALSE;
		log_printf("   linear_length  %d\n", apu.triangle.linear_length);
		log_printf("   holdnote       %d\n", apu.triangle.holdnote);
        break;

	case APU_WRC2:
		log_printf("APU #%d WRC2 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.triangle.regs[1] = value;
		apu.triangle.divisor = 256 * (apu.triangle.regs[2] & 7) + value + 1;
		apu.triangle.freq = apu_baseclock / apu.triangle.divisor;
		log_printf("   divisor        %d\n", apu.triangle.divisor);
		log_printf("   freq           %f\n", apu.triangle.freq);
        break;

	case APU_WRC3:
		log_printf("APU #%d WRC3 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.triangle.regs[2] = value;
		apu.triangle.divisor = 256 * (value & 7) + apu.triangle.regs[1] + 1;
		apu.triangle.freq = apu_baseclock / apu.triangle.divisor;
		apu.triangle.vbl_length = apu.sample_rate * apu.refresh_rate / vbl_length[value >> 3];
		log_printf("   divisor        %d\n", apu.triangle.divisor);
        log_printf("   freq           %f\n", apu.triangle.freq);
		log_printf("   vbl_length     %d\n", apu.triangle.vbl_length);
        break;

	/* noise */
	case APU_WRD0:
		log_printf("APU #%d WRD0 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.noise.regs[0] = value;
		apu.noise.volume = value & 0x0F;
		apu.noise.env_delay = apu.sample_rate / 240;
		apu.noise.fixed_envelope = (value & 0x10) ? TRUE : FALSE;
        apu.noise.holdnote = (value & 0x20) ? TRUE : FALSE;
		log_printf("   volume         %d\n", apu.noise.volume);
		log_printf("   env_delay      %d\n", apu.noise.env_delay);
		log_printf("   holdnote       %d\n", apu.noise.holdnote);
		log_printf("   fixed_env      %d\n", apu.noise.fixed_envelope);
        break;

	case APU_WRD2:
		log_printf("APU #%d WRD2 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.noise.regs[1] = value;
		apu.noise.divisor = noise_divisor[value & 0x0F];
		apu.noise.freq =  apu_baseclock / apu.noise.divisor;
		if ((value & 0x80) && FALSE == apu.noise.short_sample)
			apu.noise.cur_pos %= APU_NOISE_93;
		apu.noise.short_sample = (value & 0x80) ? TRUE : FALSE;
		log_printf("   divisor        %d\n", apu.noise.divisor);
		log_printf("   freq           %f\n", apu.noise.freq);
		log_printf("   short_sample   %f\n", apu.noise.short_sample);
        break;

	case APU_WRD3:
		log_printf("APU #%d WRD3 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.noise.regs[2] = value;
		apu.noise.vbl_length = apu.sample_rate * apu.refresh_rate / vbl_length[value >> 3];
		apu.noise.env_vol = 0;		/* reset envelope */
		apu.noise.env_phase = 0;	/* reset envelope accu */
        log_printf("   vbl_length     %d\n", apu.noise.vbl_length);
        break;

	/* DMC */
	case APU_WRE0:
		log_printf("APU #%d WRE0 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.dmc.regs[0] = value;

		apu.dmc.divisor = dmc_clocks[value & 0x0F];
		apu.dmc.freq = apu_baseclock / apu.dmc.divisor;
		apu.dmc.looping = (value & 0x40) ? TRUE : FALSE;

		if (value & 0x80)
			apu.dmc.irq_gen = TRUE;
		else
		{
			apu.dmc.irq_gen = FALSE;
			apu.dmc.irq_occurred = FALSE;
		}
		break;

	case APU_WRE1: /* 7-bit DAC */
		log_printf("APU #%d WRE1 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        /* add the _delta_ between written value and
		** current output level of the volume reg
		*/
		value &= 0x7F; /* bit 7 ignored */
		apu.dmc.output_vol += 256 * (value - apu.dmc.regs[1]);
		apu.dmc.regs[1] = value;
		break;

	case APU_WRE2:
		log_printf("APU #%d WRE2 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.dmc.regs[2] = value & 0xff;
		apu.dmc.cached_addr = 0xC000 + 64 * (value & 0xff);
		break;

	case APU_WRE3:
		log_printf("APU #%d WRE3 $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.dmc.regs[3] = value;
		apu.dmc.cached_dmalength = 8 * ((16 * value) + 1);
		break;

	case APU_SMASK:
		log_printf("APU #%d SMASK $%02x (PC=$%04x)\n", apu.cpunum, value, cpu_get_pc());
        apu.enable_reg = value;

		for (chan = 0; chan < 2; chan++)
		{
			if (value & (1 << chan))
				apu.rectangle[chan].enabled = TRUE;
			else
			{
				apu.rectangle[chan].enabled = FALSE;
				apu.rectangle[chan].vbl_length = 0;
			}
		}

		if (value & 0x04)
			apu.triangle.enabled = TRUE;
		else
		{
			apu.triangle.enabled = FALSE;
			apu.triangle.vbl_length = 0;
		}

		if (value & 0x08)
			apu.noise.enabled = TRUE;
		else
		{
			apu.noise.enabled = FALSE;
			apu.noise.vbl_length = 0;
		}

		/* bodge for timestamp queue */
        apu.dmc.enabled = (value & 0x10) ? TRUE : FALSE;

        if (value & 0x10)
		{
			if (0 == apu.dmc.dmalength)
				apu_dmcreload();
		}
		else
			apu.dmc.dmalength = 0;

		apu.dmc.irq_occurred = FALSE;
		break;

	   /* unused, but they get hit in some mem-clear loops */
	case 0x4009:
	case 0x400D:
		break;

	default:
		break;
	}
}

/* Read from $4000-$4017 */
UINT8 apu_read(UINT32 address)
{
	UINT8 value;

	switch (address)
	{
	case APU_SMASK:
		value = 0;
		/* Return 1 in 0-5 bit pos if a channel is playing */
		if (apu.rectangle[0].enabled && apu.rectangle[0].vbl_length)
			value |= 0x01;
		if (apu.rectangle[1].enabled && apu.rectangle[1].vbl_length)
			value |= 0x02;
		if (apu.triangle.enabled && apu.triangle.vbl_length)
			value |= 0x04;
		if (apu.noise.enabled && apu.noise.vbl_length)
			value |= 0x08;

		/* bodge for timestamp queue */
		if (apu.dmc.enabled)
			value |= 0x10;

		if (apu.dmc.irq_occurred)
			value |= 0x80;

		break;

	default:
        value = (address >> 8); /* heavy capacitance on data bus */
        break;
	}

	return value;
}


void apu_getpcmdata(void **data, int *num_samples, int *sample_bits)
{
	*data = apu.buffer;
	*num_samples = apu.num_samples;
	*sample_bits = apu.sample_bits;
}

void apu_process(void *buffer, int num_samples)
{
	static INT32 prev_sample = 0;
	INT16 *buf16;
	UINT8 *buf8;

	if (NULL == buffer)
		return;

	buf16 = (INT16 *) buffer;
	buf8 = (UINT8 *) buffer;

	/* bleh */
	apu.buffer = buffer;

	while (num_samples--)
	{
		INT32 next_sample, accum = 0;

		if (apu.mix_enable & 0x01)
			accum += apu_rectangle_0();
		if (apu.mix_enable & 0x02)
			accum += apu_rectangle_1();
		if (apu.mix_enable & 0x04)
			accum += apu_triangle();
		if (apu.mix_enable & 0x08)
			accum += apu_noise();
		if (apu.mix_enable & 0x10)
			accum += apu_dmc();

		if (apu.ext && (apu.mix_enable & 0x20))
			accum += apu.ext->process();

		/* do any filtering */
		if (APU_FILTER_NONE != apu.filter_type)
		{
			next_sample = accum;

			if (APU_FILTER_LOWPASS == apu.filter_type)
			{
				accum += prev_sample;
				accum >>= 1;
			}
			else
				accum = (accum + accum + accum + prev_sample) >> 2;

			prev_sample = next_sample;
		}

		/* little extra kick for the kids */
		accum <<= 1;

		/* prevent clipping */
		if (accum > 0x7FFF)
			accum = 0x7FFF;
		else if (accum < -0x8000)
			accum = -0x8000;

		/* signed 16-bit output, unsigned 8-bit */
		if (16 == apu.sample_bits)
			*buf16++ = (INT16) accum;
		else
			*buf8++ = (accum >> 8) ^ 0x80;
	}
}

/* set the filter type */
void apu_setfilter(int filter_type)
{
	apu.filter_type = filter_type;
}

void apu_reset(void)
{
	UINT32 address;

	/* use to avoid bugs =) */
	for (address = 0x4000; address <= 0x4013; address++)
		apu_write(address, 0);

#ifdef NSF_PLAYER
	apu_write(0x400C, 0x10); /* silence noise channel on NSF start */
	apu_write(0x4015, 0x0F);
#else /* !NSF_PLAYER */
	apu_write(0x4015, 0x00);
#endif /* !NSF_PLAYER */

	if (apu.ext)
		apu.ext->reset();
}

void apu_setparams(int sample_rate, int refresh_rate, int sample_bits)
{
	apu.sample_rate = sample_rate;
	apu.refresh_rate = refresh_rate;
	apu.sample_bits = sample_bits;

	apu.num_samples = sample_rate / refresh_rate;

	/* generate noise samples */
	shift_register15(noise_long_lut, APU_NOISE_32K);
	shift_register15(noise_short_lut, APU_NOISE_93);

	apu_reset();
}

/* Initializes emulated sound hardware, creates waveforms/voices */
apu_t *apu_create(int cpunum, double baseclock, int sample_rate, int refresh_rate, int sample_bits)
{
	apu_t *temp_apu;
	int channel;

	/* if a baseclock is specified use that, else (0) use the default */
	apu_baseclock = (baseclock) ? baseclock : APU_BASECLOCK;

	if (!noise_short_lut)
		noise_short_lut = (INT8 *)malloc(APU_NOISE_93);
	if (!noise_short_lut)
		return NULL;

	if (!noise_long_lut)
		noise_long_lut = (INT8 *)malloc(APU_NOISE_32K);
	if (!noise_long_lut)
		return NULL;

	temp_apu = malloc(sizeof(apu_t));
	if (NULL == temp_apu)
	   return NULL;
	memset(temp_apu, 0, sizeof(apu_t));

	/* store pointer to this apu */
	temp_apu->apu_p = temp_apu;

	/* set the CPU number for interrupts */
    temp_apu->cpunum = cpunum;

    /* set the update routine */
	temp_apu->process = apu_process;
	temp_apu->ext = NULL;

	apu = *temp_apu;

	apu_setparams(sample_rate, refresh_rate, sample_bits);

	for (channel = 0; channel < 6; channel++)
	   apu_setchan(channel, TRUE);

    apu_setfilter(APU_FILTER_LOWPASS);

	return &apu;
}

void apu_destroy(apu_t *src_apu)
{
	if (src_apu)
	{
		if (src_apu->ext)
			src_apu->ext->shutdown();
		free(src_apu->apu_p);
	}
}

void apu_setext(apu_t *src_apu, apuext_t *ext)
{
	ASSERT(src_apu);

	src_apu->ext = ext;

	/* initialize it */
	if (src_apu->ext)
		src_apu->ext->init();
}

/*
** $Log: nes_apu.c,v $
** Revision 1.10  2000/09/12 20:39:47  hjb
** Reset env_phase with env_val when the registers are written to.
**
** Revision 1.9  2000/09/12 19:01:07  hjb
** Some more bugfixes - enabled the low-pass filter again.
**
** Revision 1.8  2000/09/12 17:56:23  hjb
** - Added functions to read/write memory from a specific cpu to cpuintrf.c/h
**   data_t cpunum_readmem(int cpunum, offs_t offset);
**   void cpunum_writemem(int cpunum, offs_t offset, data_t data);
** Changed the nesintf.c/h to support a per chip cpunum which is then used
** to generate IRQs and to fetch memory with the new function inside DMC.
** Added the VSNES drivers from Ernesto again.
**
** Revision 1.7  2000/09/12 13:23:37  hjb
** Removed the incorrect handling of triangle linear_length and replaced it
** with what was in Matthew's submission (shut off a triangle wave after
** 0 - 0.25 seconds depending on the linear_length value)
**
** Revision 1.6  2000/09/12 04:17:51  hjb
** - Several lookup tables are gone, all is done in-line (simple math)
** - The code now uses a static struct apu_t (not a pointer to that struct),
**   so one pointer dereference in the update loops is gone and that makes them
**   quite a bit faster. I therefore had to change how getcontext/setcontext work:
**   They now copy the data and do not just set or return a reference.
** - The driver didn't really use the baseclock from the interface. I added
**   a 'double baseclock' parameter to apu_create() which is used now.
** - The 'cycle_rate' is gone. All accumulators are synchronized solely on
**   the sample_rate - also the accus are double, so low frequencies will work
**   without error.
** - The triangle wave "linear length counter" was not decoded or used.
**   I have no idea if what I'm doing with it is right, but it sounds ok to me
**   and also seems to make sense. The value (0-0x7f) is used to modify the
**   attack/decay ramp times of the triangle:
**     0x00 = attack in one step, decay in 127
**     0x3f/0x40 = attack/decay are symmetric (well, almost)
**     0x7f = attack in 127 steps, decay in one
**   So you shift the waveform from a decaying sawtooth to an attacking sawtooth,
**   where 0x3f/0x40 is the real triangle waveform in the middle (hope that makes it clear :)
** - Ahh.. last not least I changed the indentation. It was all with 3 spaces, yuck ;)
**
** I don't know if or how Matthew want's to use that all. For dkong3 and punchout
** it sounds fine IMO. Also SMB3 works good (really nice music theme :).. However
** there are _some_ NES games that sound strange.. a noise is sometimes there
** were it should probably be off :-P
**
** I also doubt that the oversampling really adjusts the output for the impossibly
** high frequencies, but it is better than just chopping off inaudible tones.
**
** Last not least I think we should by default disable the crude low-pass filter?
** Also is there confirmation for the the default volume fall-off (APU_VOLUME_DECAY)?
**
** Revision 1.5  2000/09/11 13:17:41  ben
** Added Matt Conte's new NES sound core - requires a clean (or delete the sound object files ;)
**
** Revision 1.35  2000/09/07 21:57:14  matt
** api change
**
** Revision 1.34  2000/08/16 05:01:01  matt
** small buglet fixed
**
** Revision 1.33  2000/08/15 12:38:04  matt
** removed debug output
**
** Revision 1.32  2000/08/15 12:36:51  matt
** calling apu_process with buffer=NULL causes silent emulation of APU
**
** Revision 1.31  2000/08/11 02:27:21  matt
** general cleanups, plus apu_setparams routine
**
** Revision 1.30  2000/07/31 04:32:52  matt
** fragsize problem fixed, perhaps
**
** Revision 1.29  2000/07/30 04:32:59  matt
** no more apu_getcyclerate hack
**
** Revision 1.28  2000/07/28 03:15:46  matt
** accuracy changes for rectangle frequency sweeps
**
** Revision 1.27  2000/07/27 02:49:50  matt
** eccentricity in sweeping hardware now emulated correctly
**
** Revision 1.26  2000/07/25 02:25:14  matt
** safer apu_destroy
**
** Revision 1.25  2000/07/23 15:10:54  matt
** hacks for win32
**
** Revision 1.24  2000/07/17 01:52:31  matt
** made sure last line of all source files is a newline
**
** Revision 1.23  2000/07/10 19:24:55  matt
** irqs are not supported in NSF playing mode
**
** Revision 1.22  2000/07/10 13:54:32  matt
** using generic nes_irq() now
**
** Revision 1.21  2000/07/10 05:29:34  matt
** moved joypad/oam dma from apu to ppu
**
** Revision 1.20  2000/07/09 03:49:31  matt
** apu irqs now draw an irq line (bleh)
**
** Revision 1.19  2000/07/04 04:53:26  matt
** minor changes, sound amplification
**
** Revision 1.18  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.17  2000/06/26 11:01:55  matt
** made triangle a tad quieter
**
** Revision 1.16  2000/06/26 05:10:33  matt
** fixed cycle rate generation accuracy
**
** Revision 1.15  2000/06/26 05:00:37  matt
** cleanups
**
** Revision 1.14  2000/06/23 11:06:24  matt
** more faithful mixing of channels
**
** Revision 1.13  2000/06/23 03:29:27  matt
** cleaned up external sound inteface
**
** Revision 1.12  2000/06/20 00:08:39  matt
** bugfix to rectangle wave
**
** Revision 1.11  2000/06/13 13:48:58  matt
** fixed triangle write latency for fixed point apu cycle rate
**
** Revision 1.10  2000/06/12 01:14:36  matt
** minor change to clipping extents
**
** Revision 1.9  2000/06/09 20:00:56  matt
** fixed noise hiccup in NSF player mode
**
** Revision 1.8  2000/06/09 16:49:02  matt
** removed all floating point from sound generation
**
** Revision 1.7  2000/06/09 15:12:28  matt
** initial revision
**
*/

