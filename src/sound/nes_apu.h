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
** nes_apu.h
**
** NES APU emulation header file
** $Id: nes_apu.h,v 1.7 2000/09/12 17:56:23 hjb Exp $
*/

#ifndef _NES_APU_H_
#define _NES_APU_H_

#ifndef BOOLEAN
typedef int BOOLEAN;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* wrapper for non-MAME use (backwards compatibility only) */
#ifndef PULSE_LINE
#define PULSE_LINE	1
#define cpu_set_irq_line(num,line,state) n2a03_irq()
#define cpunum_readmem(num,address) cpu_readmem16(address)
#endif

#ifndef INLINE
#ifdef __GNUC__
#define  INLINE 	 static inline
#elif defined(WIN32)
#define  INLINE 	 static __inline
#else
#define  INLINE 	 static
#endif
#endif

#define  APU_WRA0		0x4000
#define  APU_WRA1		0x4001
#define  APU_WRA2		0x4002
#define  APU_WRA3		0x4003
#define  APU_WRB0		0x4004
#define  APU_WRB1		0x4005
#define  APU_WRB2		0x4006
#define  APU_WRB3		0x4007
#define  APU_WRC0		0x4008
#define  APU_WRC2		0x400A
#define  APU_WRC3		0x400B
#define  APU_WRD0		0x400C
#define  APU_WRD2		0x400E
#define  APU_WRD3		0x400F
#define  APU_WRE0		0x4010
#define  APU_WRE1		0x4011
#define  APU_WRE2		0x4012
#define  APU_WRE3		0x4013

#define  APU_SMASK		0x4015

/* length of generated noise */
#define  APU_NOISE_32K	0x7FFF
#define  APU_NOISE_93	93

#define  APU_BASECLOCK	1789772.7272727272727272

/* channel structures */
/* As much data as possible is precalculated,
** to keep the sample processing as lean as possible
*/

typedef struct rectangle_s
{
	UINT8 regs[4];

	BOOLEAN enabled;

	double accu;
	double freq;
	INT32 divisor;
	INT32 output_vol;
	BOOLEAN fixed_envelope;
	BOOLEAN holdnote;
	UINT8 volume;

	INT32 sweep_phase;
	INT32 sweep_delay;
	BOOLEAN sweep_on;
	UINT8 sweep_shifts;
	UINT8 sweep_length;
	BOOLEAN sweep_inc;


	INT32 env_phase;
	INT32 env_delay;
	UINT8 env_vol;

	int vbl_length;
	UINT8 adder;
	int duty_flip;
} rectangle_t;

typedef struct triangle_s
{
	UINT8	regs[3];

	BOOLEAN enabled;

	double	accu;
	double	freq;
	INT32	divisor;
	INT32	output_vol;

	UINT8 adder;

	BOOLEAN holdnote;

	int 	vbl_length;
	int 	linear_length;
} triangle_t;


typedef struct noise_s
{
	UINT8 regs[3];

	BOOLEAN enabled;

	double accu;
	double freq;
	INT32 divisor;
	INT32 output_vol;

	INT32 env_phase;
	INT32 env_delay;
	UINT8 env_vol;
	BOOLEAN fixed_envelope;
	BOOLEAN holdnote;

	UINT8 volume;

	int vbl_length;

	BOOLEAN short_sample;
	int cur_pos;
} noise_t;

typedef struct dmc_s
{
	UINT8 regs[4];

	/* bodge for timestamp queue */
	BOOLEAN enabled;

	double accu;
	double freq;
	INT32 divisor;
	INT32 output_vol;

	UINT32 address;
	UINT32 cached_addr;
	int dmalength;
	int cached_dmalength;
	UINT8 cur_byte;

	BOOLEAN looping;
	BOOLEAN irq_gen;
	BOOLEAN irq_occurred;

} dmc_t;

enum
{
	APU_FILTER_NONE,
	APU_FILTER_LOWPASS,
	APU_FILTER_WEIGHTED
};

typedef struct
{
	UINT32 min_range, max_range;
	UINT8 (*read_func)(UINT32 address);
} apu_memread;

typedef struct
{
	UINT32 min_range, max_range;
	void (*write_func)(UINT32 address, UINT8 value);
} apu_memwrite;

/* external sound chip stuff */
typedef struct apuext_s
{
	void  (*init)(void);
	void  (*shutdown)(void);
	void  (*reset)(void);
	INT32 (*process)(void);
	apu_memread *mem_read;
	apu_memwrite *mem_write;
} apuext_t;

typedef struct apu_s
{
	rectangle_t rectangle[2];
	triangle_t triangle;
	noise_t noise;
	dmc_t dmc;
	UINT8 enable_reg;

	int cpunum; 		/* HJB: added the cpu number for DMC reads */

    void *buffer; /* pointer to output buffer */
	int num_samples;

	UINT8 mix_enable;	/* HJB: turned into a bit flag */
	int filter_type;

	int sample_rate;
	int sample_bits;
	int refresh_rate;

	void (*process)(void *buffer, int num_samples);

	struct apu_s *apu_p;
	/* external sound chip */
	apuext_t *ext;
} apu_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Function prototypes */
extern int apu_getcontext(apu_t *dst_apu);
extern void apu_setcontext(apu_t *src_apu);

extern apu_t *apu_create(int cpunum, double baseclock, int sample_rate, int refresh_rate, int sample_bits);
extern void apu_destroy(apu_t *apu);
extern void apu_setparams(int sample_rate, int refresh_rate, int sample_bits);

extern void apu_process(void *buffer, int num_samples);
extern void apu_reset(void);

extern void apu_setext(apu_t *apu, apuext_t *ext);
extern void apu_setfilter(int filter_type);
extern void apu_setchan(int chan, BOOLEAN enabled);

extern UINT8 apu_read(UINT32 address);
extern void apu_write(UINT32 address, UINT8 value);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NES_APU_H_ */

/*
** $Log: nes_apu.h,v $
** Revision 1.7  2000/09/12 17:56:23  hjb
** - Added functions to read/write memory from a specific cpu to cpuintrf.c/h
**   data_t cpunum_readmem(int cpunum, offs_t offset);
**   void cpunum_writemem(int cpunum, offs_t offset, data_t data);
** Changed the nesintf.c/h to support a per chip cpunum which is then used
** to generate IRQs and to fetch memory with the new function inside DMC.
** Added the VSNES drivers from Ernesto again.
**
** Revision 1.6  2000/09/12 13:23:37  hjb
** Removed the incorrect handling of triangle linear_length and replaced it
** with what was in Matthew's submission (shut off a triangle wave after
** 0 - 0.25 seconds depending on the linear_length value)
**
** Revision 1.5  2000/09/12 04:17:51  hjb
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
** Revision 1.4  2000/09/11 13:17:41  ben
** Added Matt Conte's new NES sound core - requires a clean (or delete the sound object files ;)
**
** Revision 1.21  2000/08/11 02:27:21  matt
** general cleanups, plus apu_setparams routine
**
** Revision 1.20  2000/07/30 04:32:59  matt
** no more apu_getcyclerate hack
**
** Revision 1.19  2000/07/27 02:49:50  matt
** eccentricity in sweeping hardware now emulated correctly
**
** Revision 1.18  2000/07/25 02:25:15  matt
** safer apu_destroy
**
** Revision 1.17  2000/07/23 15:10:54  matt
** hacks for win32
**
** Revision 1.16  2000/07/23 00:48:15  neil
** Win32 SDL
**
** Revision 1.15  2000/07/17 01:52:31  matt
** made sure last line of all source files is a newline
**
** Revision 1.14  2000/07/11 02:39:26  matt
** added setcontext() routine
**
** Revision 1.13  2000/07/10 05:29:34  matt
** moved joypad/oam dma from apu to ppu
**
** Revision 1.12  2000/07/04 04:54:48  matt
** minor changes that helped with MAME
**
** Revision 1.11  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.10  2000/06/26 05:00:37  matt
** cleanups
**
** Revision 1.9  2000/06/23 03:29:28  matt
** cleaned up external sound inteface
**
** Revision 1.8  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.7  2000/06/20 00:07:35  matt
** added convenience members to apu_t struct
**
** Revision 1.6  2000/06/09 16:49:02  matt
** removed all floating point from sound generation
**
** Revision 1.5  2000/06/09 15:12:28  matt
** initial revision
**
*/

