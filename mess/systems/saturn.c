/*
   Sega Saturn Driver
   Copyright James Forshaw (TyRaNiD@totalise.net) July 2001

   Almost total rewrite. Uses the basic memory model from
   old saturn driver by Juergen Buchmueller <pullmoll@t-online.de>,
   although its been fixed up to work with the new mame memory model
   and to correct a few errors. Little else has survived.

   Memory Map

   0x0000000 > 0x0080000		: Boot (IPL) ROM
   0x0100000 > 0x0100080		: SMPC register area
   0x0180000 > 0x018ffff		: Backup RAM
   0x0180001 > 0x019ffff		: Backup RAM   Shadow
   0x0200000 > 0x0300000		: 1 MB of DRAM (Lower Work RAM)
   0x1000000 > 0x1000004		: Slave SH2 communication register
   0x1800000 > 0x1800004		: Master SH2 communication register
   0x2000000 > 0x4000000		: Expansion area (RAM cart etc.)
   0x5890000 > 0x5900000		: CDROM interface region
   0x5A00000 > 0x5B00EE4		: SCSP region
   0x5C00000 > 0x5CC0000		: VDP1 VRAM and frame buffer
   0x5D00000 > 0x5D00018		: VDP1 registers
   0x5E00000 > 0x5E80000		: VDP2 VRAM
   0x5F00000 > 0x5F01000		: VDP2 CRAM
   0x5F80000 > 0x5F80120		: VDP2 registers
   0x5FE0000 > 0x5FE00D0		: SCU registers
   0x6000000 > 0x6100000		: 1 MB SDRAM (Upper Work RAM)

   SH2 specific areas (are local to each CPU)
   0xC0000000 > 0xC0000200		: Cache RAM
   0xFFFFFE00 > 0xFFFFFFFF		: CPU module registers

   Main Processors 2 x Hitachi SH2 7604 RISC chips
   Clock speeds	At 320 pixels per scanline
       NTSC - 26.8741 MHz
       PAL  - 26.6875 MHz
       At 352 pixels per scanline
       NTSC - 28.6364 MHz
       PAL  - 28.4375 MHz
   Connected as master - slave.

   Supplementary Maths processor: DSP.
   Main Memory: 1 Megabyte of SDRAM and 1 Megabyte of DRAM
   Backup RAM: 64Kb battery backed SRAM
   Boot ROM: 512Kb-boot ROM
   Video Hardware: 2 VDP (video display processors) chip sets, VDP1 Draw
   Sprites, lines, warped sprites (quads), VDP2 controls background graphics.

   VDP1 can display sprites in colour depths of 4,8,15 bits per pixel. Number of
   sprites limited only by sizes and VRAM limitations. Can perform on chip
   gouroud shading on sprites and rotate sprite frame buffer. Not much point
   indicating maximum polygon counts as its extremely misleading.
   VRAM: 512Kb

   VDP2 can display up to 5 play-fields (although generally not simultaneously),
   rotated background support. Colour depths of 4,8,15,24 bits per pixel and can
   do special effects such as transparency, sprite shadowing, half-toning, line
   scroll, line-colour mapping.
   VRAM: 512Kb

   Selection of NTSC display resolutions

   320x224 @ 60Hz
   352x224 @ 60Hz
   640x224 @ 60Hz
   640x448 @ 30Hz
   704x480 @ 30Hz

   SCU: System control unit. Control memory mapping, contains 3 DMA
   channels, interfaces SH2 processors to DSP co-processor, handles and
   masks hardware interrupts.

   SMPC: Interfaces to peripherals, performs system management tasks (built in
   clock, system reset).

   SCSP: Custom sound processing module including MC68000 microprocessor
   and effects DSP.

   CDROM: Maximum 2x speed (300 k/sec) CLV. Custom interface controlled by
   Hitachi SH1 processor. 512 Kb Buffer RAM.

   -= Current State Infomation =-
   Bios starts requesting pad data so maybe we are now in the time set area ????
   Need to emulate pad system, add more interrupts and get some grafix working so we
   can see exactly where we are at this current time.
   Currently locks up waiting on (i guess) a response from the 68k cpu. Probably need to
   emulate this shortly.

   -= UPDATES =-

   01/07/2001 - Added priliminary vdp2 drawing code.
   25/06/2001 - Added register names for vdp1/2. Not included yet. Added support for vdp2 reg
   read/write. Added all my bios images.
   23/06/2001 - Had to modify sh2.c to get interrupts working. Bios starts resquesting pad data
   till the sound system causes a lock.. Included a small hack to bypass this lock but then
   it blocks somewhere else. Waiting on another interrupt ?????
   20/06/2001 - Made smpc return timer not set. Starts writing stuff to vdp ram. Started adding
   prelim HBlank support. Using HBlank timing to control VBlank timing (makes scu timers easier)
   19/06/2001 - Added simple smpc and cd commands.
   18/06/2001 - Bug in the original driver memory map. SMPC allocated a few meg instead of
   0x80 bytes :)
   17/06/2001 - Added all main ram areas. Should work ok for now.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/sh2/sh2.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#define DISP_MEM 0 /* define to log memory access to all non work ram areas */
#define DISP_VDP1 0 /* define to log vdp1 command execution */

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

#define PAL 0       /* Set to 1 for PAL mode. Must be set to 1 for euro bios */

static UINT32 *mem; /* Base memory pointer */
static UINT16 *sound_base;
static UINT32 *fb1_ram_base;
static UINT32 *workl_ram_base;
static UINT32 *vdp1_ram_base;
static UINT32 *vdp2_ram_base;
static UINT32 *color_ram_base;
static UINT32 *workh_ram_base;
static UINT32 *back_ram_base;
//static int saturn_video_dirty = 1;

/*
   Define memory bases. Note these are byte locations and widths
   Divide by 4 to get location in mem array
*/

 #define SATURN_ROM_BASE         0x00000000
 #define SATURN_ROM_SIZE         0x00080000
 #define SATURN_WORKL_RAM_BASE   0x00080000
 #define SATURN_WORKL_RAM_SIZE   0x00100000
 #define SATURN_WORKH_RAM_BASE   0x00180000
 #define SATURN_WORKH_RAM_SIZE   0x00100000

 #define SATURN_VDP1_RAM_BASE    0x00280000
 #define SATURN_VDP1_RAM_SIZE    0x00080000
 #define SATURN_VDP2_RAM_BASE    0x00300000
 #define SATURN_VDP2_RAM_SIZE    0x00080000
 #define SATURN_FB1_RAM_BASE     0x00380000
 #define SATURN_FB1_RAM_SIZE     0x00040000
#define SATURN_FB2_RAM_BASE     0x003c0000
#define SATURN_FB2_RAM_SIZE     0x00040000
 #define SATURN_COLOR_RAM_BASE   0x00400000
 #define SATURN_COLOR_RAM_SIZE   0x00001000
 #define SATURN_BACK_RAM_BASE    0x00401000
 #define SATURN_BACK_RAM_SIZE    0x00020000

#define SATURN_SCR_WIDTH    704
#define SATURN_SCR_HEIGHT   512

#ifdef LSB_FIRST
#define SWAP_WORDS(x) (((x)<<16) | ((x)>>16)) /* Swaps over words so its easier to interface 32 and 16 bit datas */
#else
#define SWAP_WORDS(x) (x) /* On MSB systems no need to swap */
#endif

/* Memory handlers */
/* Read handler get offset (byte offset / 4) and mem_mask (all bits required are 0) */
/* Write handler get data, offset and mem_mask */

#ifdef UNNEEDED
READ32_HANDLER( saturn_workh_ram_r )
{
  /*offs_t ea;

  ea = (SATURN_WORKH_RAM_BASE / 4) + offset;*/
  return workh_ram_base[offset] & (~mem_mask);
}

WRITE32_HANDLER( saturn_workh_ram_w )
{

  workh_ram_base[offset] = (workh_ram_base[offset] & mem_mask) | data;
}
#endif

static READ32_HANDLER( saturn_sound_ram_r )
{

#if DISP_MEM
  logerror("soundram_r offset=%08lX mem_mask=%08lX PC=%08lX\n",offset,mem_mask,activecpu_get_reg(SH2_PC));
#endif

  return ((sound_base[(offset<<1)] << 16) | (sound_base[(offset<<1)+1])) & (~mem_mask);

}

static WRITE32_HANDLER( saturn_sound_ram_w )
{

  UINT16 *sb_temp = &sound_base[(offset<<1)];
#if DISP_MEM
  logerror("soundram_w offset=%08lX data=%08lX mem_mask=%08lX PC=%08lX\n",offset,data,mem_mask,activecpu_get_reg(SH2_PC));
#endif

  *sb_temp = (*sb_temp & (mem_mask >> 16)) | (data >> 16);
  sb_temp++;
  *sb_temp = (*sb_temp & mem_mask) | data;
}

static READ32_HANDLER( saturn_vdp1_ram_r )
{
  /*offs_t ea;*/

#if DISP_MEM
  logerror("vdp1ram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  /*ea = (SATURN_VDP1_RAM_BASE / 4) + offset;*/
  return vdp1_ram_base[offset] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_vdp1_ram_w )
{

#if DISP_MEM
  logerror("vdp1ram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  vdp1_ram_base[offset] = (vdp1_ram_base[offset] & mem_mask) | data;
}

static READ32_HANDLER( saturn_vdp2_ram_r )
{
  /*offs_t ea;*/

#if DISP_MEM
  logerror("vdp2ram_r offset=%08lX mem_mask=%08lX PC=%08lX\n",offset,mem_mask,activecpu_get_reg(SH2_PC));
#endif

  /*ea = (SATURN_VDP2_RAM_BASE / 4) + offset;*/
  return vdp2_ram_base[offset] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_vdp2_ram_w )
{
  /* offs_t ea;*/

#if DISP_MEM
  logerror("vdp2ram_w offset=%08lX data=%08lX mem_mask=%08lX PC=%08lX\n",offset,data,mem_mask,activecpu_get_reg(SH2_PC));
#endif

  /*ea = (SATURN_VDP2_RAM_BASE / 4) + offset;*/
  vdp2_ram_base[offset] = (vdp2_ram_base[offset] & mem_mask) | data;
}

static READ32_HANDLER( saturn_fb1_ram_r )
{
  /* offs_t ea;*/

#if DISP_MEM
  logerror("fb1_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  /* ea = (SATURN_FB1_RAM_BASE / 4) + offset;*/
  return fb1_ram_base[offset] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_fb1_ram_w )
{
	logerror("fb1_w offset=%08lX data=%08lX mem_mask=%08lX\n", (long) offset, (long) data, (long) mem_mask);
	fb1_ram_base[offset] = (fb1_ram_base[offset] & mem_mask) | data;
}

/* FB2 not mapped directly */
#if 0
static READ32_HANDLER( saturn_fb2_ram_r )
{
	offs_t ea;

	logerror("fb2_r offset=%08lX mem_mask=%08lX\n", (long) offset, (long) mem_mask);

	ea = (SATURN_FB2_RAM_BASE / 4) + offset;
	return mem[ea] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_fb2_ram_w )
{
	offs_t ea;

	logerror("fb2_w offset=%08lX data=%08lX mem_mask=%08lX\n", (long) offset, (long) data, (long) mem_mask);

	ea = (SATURN_FB2_RAM_BASE / 4) + offset;
	mem[ea] = (mem[ea] & mem_mask) | data;
}
#endif
/* END FB2 */

static READ32_HANDLER( saturn_color_ram_r )

{
  logerror("colorram_r offset=%08lX mem_mask=%08lX PC=%08lX\n", (long) offset, (long) mem_mask, (long) activecpu_get_reg(SH2_PC));

  return color_ram_base[offset] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_color_ram_w )
{
  /* offs_t ea;*/

#if DISP_MEM
  logerror("colorram_w offset=%08lX data=%08lX mem_mask=%08lX PC=%08lX\n",offset,data,mem_mask,activecpu_get_reg(SH2_PC));
#endif

  /*ea = (SATURN_COLOR_RAM_BASE / 4) + offset;*/
  color_ram_base[offset] = (color_ram_base[offset] & mem_mask) | data;
}

static READ32_HANDLER( saturn_back_ram_r )
{

#if DISP_MEM
  logerror("backram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif
if (offset >= 0x4000) offset -= 0x4000; /* do the shadow RAM offsets */
  return back_ram_base[offset] & (~mem_mask);
}

static WRITE32_HANDLER( saturn_back_ram_w )
{

#if DISP_MEM
  logerror("backram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

if (offset >= 0x4000) offset -= 0x4000; /* do the shadow RAM offsets */
back_ram_base[offset] = (back_ram_base[offset] & mem_mask) | data;
}

/****************************************************************
 *  SMPC Handler                                                *
 ****************************************************************/

struct _smpc_state /* Holds infomation on the current state of the smpc */

{
  UINT8 smem[4];      /* Internal SRAM memory */
  int reset_disable;  /* Is reset disabled? */
  int set_timer;      /* Should the timer be set on boot ? */
  UINT8 smpc_regs[0x80];
} smpc_state;

/* Defines to access regsiters at known locations */

#define COMMREG (0x1F)
#define STATUSR (0x61)
#define STATUSF (0x63)
#define IREG(x) (((x)<<1) + 1)
#define OREG(x) (((x)<<1) + 0x21)
#define PDR1    (0x75)
#define DDR1    (0x79)
#define PDR2    (0x77)
#define DDR2    (0x7B)
#define IOSEL   (0x7D)
#define EXEL    (0x7F)

#define SET_TIMER 1    /* Set to 1 to indicate timer must be set */

static void reset_smpc(void)

     /* Reset SMPC system */

{
  memset(smpc_state.smpc_regs,0,0x80); /* Clear SMPC regs */
  smpc_state.smpc_regs[STATUSF] = 1;   /* Initially set to 1 ?? */
  memset(smpc_state.smem,0,4);
  smpc_state.reset_disable = 1;  /* Initially reset is disabled */
  smpc_state.set_timer = SET_TIMER; /* Setup set timer state */
}

static void smpc_execcomm(int commcode)

     /* Function to execute a smpc command when COMMREG is written to */

{
  /*  if(smpc_state.smpc_regs[STATUSF]) If status flag set (can only execute here, not) */

    {
      switch(commcode)
	{
	case 0x1A :
	  smpc_state.reset_disable = 1; /* Disable reset */
	  logerror("smpc - Reset Disable (0x1A)\n");
	  break;
	case 0x19 :
	  smpc_state.reset_disable = 0; /* Enable reset */
	  logerror("smpc - Reset Enable (0x19)\n");
	  break;
	case 0x10 :
	  smpc_state.smpc_regs[STATUSR] = 0x40; /* No data remaining, reset off */
	  smpc_state.smpc_regs[OREG(0)] = smpc_state.set_timer << 7 | smpc_state.reset_disable << 6;
	  smpc_state.smpc_regs[OREG(1)] = 0x20;
	  smpc_state.smpc_regs[OREG(2)] = 0x01;
	  smpc_state.smpc_regs[OREG(3)] = 0x01;
	  smpc_state.smpc_regs[OREG(4)] = 0x11;
	  smpc_state.smpc_regs[OREG(5)] = 0x11;
	  smpc_state.smpc_regs[OREG(6)] = 0x11;
	  smpc_state.smpc_regs[OREG(7)] = 0x11;
	  smpc_state.smpc_regs[OREG(8)] = 0;
	  smpc_state.smpc_regs[OREG(9)] = 0x1;
	  smpc_state.smpc_regs[OREG(10)] = 0;
	  smpc_state.smpc_regs[OREG(11)] = 0;
	  smpc_state.smpc_regs[OREG(12)] = 0;
	  smpc_state.smpc_regs[OREG(13)] = 0;
	  smpc_state.smpc_regs[OREG(14)] = 0;
	  smpc_state.smpc_regs[OREG(15)] = 0;
	  logerror("smpc - Int Back (0x10)\n");
	  break;
	case 0x2:
	  printf /*logerror*/ ("smpc - Slave SH-2 ON (0x2)\n");
	  break;
	case 0x0:
	  printf /*logerror*/ ("smpc - Master SH-2 ON (0x0)\n");
	  break;
	case 0x3:
	  printf /*logerror*/ ("smpc - Slave SH-2 OFF (0x3)\n");
	  break;
	case 0x7 :
	  printf /*logerror*/ ("smpc - Sound OFF (0x7)\n");
	  cpu_set_halt_line(2, ASSERT_LINE);
	  break;
	case 0x6 :
	  printf /*logerror*/("smpc - Sound ON (0x6)\n");
	  cpu_set_halt_line(2, ASSERT_LINE);
	  cpu_set_reset_line(2,PULSE_LINE);
	  cpu_set_halt_line(2, CLEAR_LINE);
	  break;
	case 0xD :
	  logerror("smpc - Reset System (0xD)\n");
	  cpu_set_halt_line(2, ASSERT_LINE);
	  cpu_set_reset_line(0,PULSE_LINE);
	  cpu_set_reset_line(1,PULSE_LINE);
	  break;
	}
      smpc_state.smpc_regs[OREG(31)] = commcode;
      smpc_state.smpc_regs[STATUSF]  = 0;
    }
}

static READ32_HANDLER( saturn_smpc_r )   /* SMPC */
{
  UINT32 ret_val = 0;
  int loop;
  UINT32 temp_mask;

  ret_val  = smpc_state.smpc_regs[offset<<2] << 24;      /* Hopefully will reconstruct long word */
  ret_val |= smpc_state.smpc_regs[(offset<<2)+1] << 16;  /* independant of endian from bytes     */
  ret_val |= smpc_state.smpc_regs[(offset<<2)+2] << 8;
  ret_val |= smpc_state.smpc_regs[(offset<<2)+3];

  /* SMPC regs only on odd address values so could remove two statements */

  /* Log data accesses */

  temp_mask = 0xFF000000; /* Setup to check MSB of mask */
  for(loop = 0;loop < 4;loop++)
    {
      if(!(mem_mask & temp_mask)) /* If not masked we can write a byte */
	{
	  UINT32 ea;
	  UINT32 d;

	  ea = (offset*4) + loop;
	  d = (ret_val >> (8*(3-loop))) & 0xFF;

	  /* Log smpc read - should be turned off when not needed */

	  if((ea >= IREG(0)) && (ea <= IREG(7)))
	    {
	      logerror("smpc_r IREG%01d -> %02lX - PC=%08lX\n", (int) (ea-IREG(0))/2, (long) d, (long) activecpu_get_reg(SH2_PC));
	    }
	  else
	    if((ea >= OREG(0)) && (ea <= OREG(31)))
		{
		  logerror("smpc_r OREG%01d -> %02lX - PC=%08lX\n", (int) (ea-OREG(0))/2, (long) d, (long) activecpu_get_reg(SH2_PC));
		}
	    else
	      {
		switch(ea) /* See if the write is significant */
		  {
		  case COMMREG : logerror("smpc_r COMMREG - command = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case STATUSR : logerror("smpc_r SR      - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case STATUSF : logerror("smpc_r SF      - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case PDR1    : logerror("smpc_r PDR1    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case DDR1    : logerror("smpc_r DDR1    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case PDR2    : logerror("smpc_r PDR2    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case DDR2    : logerror("smpc_r DDR2    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case IOSEL   : logerror("smpc_r IOSEL   - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case EXEL    : logerror("smpc_r EXEL    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  default      : logerror("smpc_r offset=%08lX data=%02lX - PC=%08lX\n", (long) ea, (long) d, (long) activecpu_get_reg(SH2_PC));
		  }
	      }
	}
      temp_mask >>= 8;
    }

  return ret_val & ~mem_mask;
}

static WRITE32_HANDLER( saturn_smpc_w )  /* SMPC */
{
  int loop;
  UINT32 temp_mask;

  //  logerror("smpc_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);
  temp_mask = 0xFF000000; /* Setup to check MSB of mask */
  for(loop = 0;loop < 4;loop++)
    {
      if(!(mem_mask & temp_mask)) /* If not masked we can write a byte */
	{
	  UINT32 ea;
	  UINT32 d;

	  ea = (offset*4) + loop;
	  d = (data >> (8*(3-loop))) & 0xFF;

	  /* work out what register has been set and execute command etc */

	  smpc_state.smpc_regs[ea] = d;
	  switch(ea)
	    {
	    case COMMREG :
	      smpc_execcomm(d); /* If a write to commreg, execute command */
	      break;
	    }

	  /* Log smpc write - should be turned off when not needed */

	  if((ea >= IREG(0)) && (ea <= IREG(7)))
	    {
	      logerror("smpc_w IREG%01d <- %02lX - PC=%08lX\n", (int) (ea-IREG(0))/2, (long) d, (long) activecpu_get_reg(SH2_PC));
	    }
	  else
	    if((ea >= OREG(0)) && (ea <= OREG(31)))
		{
		  logerror("smpc_w OREG%01d <- %02lX - PC=%08lX\n", (int) (ea-OREG(0))/2, (long) d, (long) activecpu_get_reg(SH2_PC));
		}
	    else
	      {
		switch(ea) /* See if the write is significant */
		  {
		  case COMMREG : logerror("smpc_w COMMREG - command = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case STATUSR : logerror("smpc_w SR      - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case STATUSF : logerror("smpc_w SF      - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case PDR1    : logerror("smpc_w PDR1    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case DDR1    : logerror("smpc_w DDR1    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case PDR2    : logerror("smpc_w PDR2    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case DDR2    : logerror("smpc_w DDR2    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case IOSEL   : logerror("smpc_w IOSEL   - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  case EXEL    : logerror("smpc_w EXEL    - data = %02lX - PC=%08lX\n", (long) d, (long) activecpu_get_reg(SH2_PC));
		    break;
		  default      : logerror("smpc_w offset=%08X data=%02X - Pc=%08X\n",ea,d,activecpu_get_reg(SH2_PC));
		  }
	      }
	}
      temp_mask >>= 8;
    }
}

static READ32_HANDLER( saturn_cs0_r )	  /* CS0 */
{
	logerror("cs0_r offset=%08lX mem_mask=%08lX\n", (long) offset*4, (long) mem_mask);

	return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_cs0_w )   /* CS0 */
{
}

static READ32_HANDLER( saturn_cs1_r )	  /* CS1 */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_cs1_w )   /* CS1 */
{
}

static READ32_HANDLER( saturn_cs2_r )	  /* CS2 */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_cs2_w )   /* CS2 */
{
}

/* SCU Handler */

static const char *scu_regnames[0x34] = {"DMA0 Read     ",     /* 0x00 */
					 "DMA0 Write    ",     /* 0x04 */
					 "DMA0 Count    ",     /* 0x08 */
					 "DMA0 Addr add ",     /* 0x0C */
					 "DMA0 Enable   ",     /* 0x10 */
					 "DMA0 Mode     ",     /* 0x14 */
					 "X18           ",     /* 0x18 */
					 "X1C           ",     /* 0x1C */
					 "DMA1 Read     ",     /* 0x20 */
					 "DMA1 Write    ",     /* 0x24 */
					 "DMA1 Count    ",     /* 0x28 */
					 "DMA1 Addr add ",     /* 0x2C */
					 "DMA1 Enable   ",     /* 0x30 */
					 "DMA1 Mode     ",     /* 0x34 */
					 "X38           ",     /* 0x38 */
					 "X3C           ",     /* 0x3C */
					 "DMA2 Read     ",     /* 0x40 */
					 "DMA2 Write    ",     /* 0x44 */
					 "DMA2 Count    ",     /* 0x48 */
					 "DMA2 Addr add ",     /* 0x4C */
					 "DMA2 Enable   ",     /* 0x50 */
					 "DMA2 Mode     ",     /* 0x54 */
					 "X58           ",     /* 0x58 */
					 "X5C           ",     /* 0x5C */
					 "X60           ",     /* DMA force stop. Doesn't exist */
					 "X64           ",
					 "X68           ",
					 "X6C           ",
					 "X70           ",
					 "X74           ",
					 "X78           ",
					 "X7C           ",
					 "DSP Ctrl Port ",
					 "DSP Prog RAM  ",
					 "DSP Data Addr ",
					 "DSP Data Data ",
					 "Timer0 Compare",
					 "Timer1 Set    ",
					 "Timer1 Mode   ",
					 "X9C           ",
					 "Int Mask      ",
					 "Int Stat      ",
					 "A-Bus IntAck  ",
					 "XAC           ",
					 "A-Bus Set 0   ",
					 "A-Bus Set 1   ",
					 "A-Bus Refresh ",
					 "XBC           ",
					 "XC0           ",
					 "SDRAM select  ",
					 "SCU Version   ",
					 "XCC           "};


UINT32 scu_regs[0x34]; /* SCU register block */
static const char *int_names[16] = {
  "VBlank-IN", "VBlank-OUT", "HBlank-IN", "Timer 0",
  "Timer 1", "DSP", "Sound", "SMPC", "PAD",
  "DMA Level 2", "DMA Level 1", "DMA Level 0",
  "DMA Illegal", "Sprite END", "Illegal", "A-Bus" };

enum

{
  VBLANK_IN_INT,
  VBLANK_OUT_INT,
  HBLANK_IN_INT,
  TIMER_0_INT,
  TIMER_1_INT,
  DSP_INT,
  SOUND_INT,
  SMPC_INT,
  PAD_INT,
  DMA2_INT,
  DMA1_INT,
  DMA0_INT,
  DMA_ILL_INT,
  SPRITE_INT,
  ILLEGAL_INT,
  ABUS_INT
};

static void reset_scu(void)

{
  memset(scu_regs,0,0x34*4);
  scu_regs[0x28] = 0xffffffff;
}

static READ32_HANDLER( saturn_scu_r )	  /* SCU, DMA/DSP */

{
  // logerror("scu_r %s - data = %08lX - PC=%08lX\n",scu_regnames[offset],scu_regs[offset],activecpu_get_reg(SH2_PC));
  return scu_regs[offset] & ~mem_mask;
}
 
static int scu_irq_line[16] = /* Indicates what irq pin is to be used for each int */
{
    0, 1, 2, 3, 4, 5, 6, 7,
    7, 6, 6, 6, 6, 5, 5, 5
};

static int scu_irq_levels[16] =
{
    15, 14, 13, 12, 11, 10,  9,  8,
     8,  6,  6,  5,  3,  2,  0,  0
};

static void scu_set_imask(void)
{
    int irq;
    LOG(("saturn_scu_w    interrupt mask change:"));
    for (irq = 0; irq < 16; irq++)
    {
        if ((scu_regs[0x28] & (1 <<irq)) == 0)
            logerror(" %s,", int_names[irq]);
        else
	  cpu_set_irq_line(0, scu_irq_line[irq], CLEAR_LINE);
    }
    LOG(("\n"));
}

static void scu_pulse_interrupt(int irq)
{
    if (irq >= ABUS_INT)
    {
        LOG(("scu    pulsed abus irq\n"));
    }
    else
    {
        LOG(("scu    IRQ #%d", irq));
        if ((scu_regs[0x28] & (1 << irq)) == 0)
        {
            LOG((" - pulsed"));
            cpu_irq_line_vector_w(0, scu_irq_line[irq], 0x40 + irq + (scu_irq_levels[irq] << 8)); 
            cpu_set_irq_line(0, scu_irq_line[irq], HOLD_LINE);
        }
        else
        {
            LOG((" - masked"));
        }
        LOG(("\n"));
    }
}

static WRITE32_HANDLER( saturn_scu_w )   /* SCU, DMA/DSP */
{
	logerror("scu_w %s - data = %08lX - PC=%08lX\n", scu_regnames[offset], (long) data, (long) activecpu_get_reg(SH2_PC));
	scu_regs[offset] = (scu_regs[offset] & mem_mask) | data;
	if (offset == 0x28) scu_set_imask();
}

static const char *cd_regnames[0xA] = {"X0",
				       "X4",
				       "HIRQ",
				       "HIRQ Mask",
				       "X10",
				       "X14",
				       "CR1",
				       "CR2",
				       "CR3",
				       "CR4"};

UINT32 cd_regs[0xA];
UINT32 periodic; /* Currently a hack to bypass bios area */

#define CD_HIRQ     0x2
#define CD_HIRQMASK 0x3
#define CD_CR1      0x6
#define CD_CR2      0x7
#define CD_CR3      0x8
#define CD_CR4      0x9

static void reset_cd(void)

{
  cd_regs[CD_CR1]  =  'C' << 16;
  cd_regs[CD_CR2]  = ('D' << 24) | ('B' << 16);
  cd_regs[CD_CR3]  = ('L' << 24) | ('O' << 16);
  cd_regs[CD_CR4]  = ('C' << 24) | ('K' << 16);
  cd_regs[CD_HIRQ] = 1;
  cd_regs[CD_HIRQMASK] = 0xBE1;
  periodic = 0; /* This will toggle every other read so that bios will execute */
}

static void cd_execcomm(void)

{
  int command;

  command = cd_regs[CD_CR1] >> 24; /* Shift down command code to low byte */

  switch(command)
    {
    case 0x0 : /* Command 0x00 - Get Status */
      cd_regs[CD_CR1] = 0x07FF0000; /* Return no disc status for now */
      cd_regs[CD_CR2] = 0xFFFF0000;
      cd_regs[CD_CR3] = 0xFFFF0000;
      cd_regs[CD_CR4] = 0xFFFF0000;
      logerror("cd - Get Status (0x00)\n");
      break;
    case 0x1 : /* Command 0x01 - Get Hardware Info */
      cd_regs[CD_CR1] = 0x07000000;
      cd_regs[CD_CR2] = 0x00000000;
      cd_regs[CD_CR3] = 0x00000000;
      cd_regs[CD_CR4] = 0x00000000;
      logerror("cd - Get Hardware Info\n");
      break;
    }
}

static READ32_HANDLER( saturn_cd_r )	 /* CD */
{
	if(offset < 0xA)
	{
		logerror("cd_r %s - data = %04lX - PC=%08lX\n",
			cd_regnames[offset],
			(long) cd_regs[offset] >> 16,
			(long) activecpu_get_reg(SH2_PC));
	}
	else
	{
		logerror("cd_r offset=%08lX mem_mask=%08lX - PC=%08lX\n",
			(long) offset*4,
			(long) mem_mask,
			(long) activecpu_get_reg(SH2_PC));
	}

	if(offset < 0xA)
	{
		if(offset == CD_CR1)
		{
			if(periodic) /* Hack to indicate periodic response while cd system not finished */
			{
				periodic = 0;
				return (cd_regs[offset] & ~mem_mask) | 0x10000000;
			}
			else
			{
				periodic = 1;
			}
		}
		return cd_regs[offset] & ~mem_mask;
	}
	else
		return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_cd_w )	 /* CD */
{
  if(offset < 0xA)
    {
      logerror("cd_w %s - data = %04lX - PC=%08lX\n",cd_regnames[offset], (long) data >> 16, (long) activecpu_get_reg(SH2_PC));
    }
  else
    {
      logerror("cd_w offset=%08lX data=%08lX mem_mask=%08lX - PC=%08lX\n", (long) offset*4, (long) data, (long) mem_mask, (long) activecpu_get_reg(SH2_PC));
    }

  if(offset < 0xA)
    {
      switch(offset)
	{
	case CD_HIRQ :
	  cd_regs[offset] = ((cd_regs[offset] & mem_mask) & ~data) | 0x00010000;
	  break; /* Case 2 HIRQ register - writing a 1 clears state*/
	case CD_CR4  :
	  cd_regs[offset] = (cd_regs[offset] & mem_mask) | data;
	  cd_execcomm(); /* When CR4 written execute command */
	  break;
	default:
	  cd_regs[offset] = (cd_regs[offset] & mem_mask) | data;
	}
    }
}

/********************************************************
 *  FRT master                                          *
 ********************************************************/

static READ32_HANDLER( saturn_minit_r )  /* MINIT */
{
	return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_minit_w )  /* MINIT */
{
	logerror("minit_w offset=%08lX data=%08lX mem_mask=%08lX\n", (long) offset, (long) data, (long) mem_mask);
}

/********************************************************
 *  FRT slave
 ********************************************************/
static READ32_HANDLER( saturn_sinit_r )  /* SINIT */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_sinit_w )  /* SINIT */
{
	logerror("sinit_w offset=%08lX data=%08lX mem_mask=%08lX\n",
		(long) offset,
		(long) data,
		(long) mem_mask);
}

/********************************************************
 *  DSP
 ********************************************************/
static READ32_HANDLER( saturn_dsp_r )	 /* DSP */
{
	logerror("dsp_r offset=%08lX mem_mask=%08lX\n",
		(long) offset*4,
		(long) mem_mask);

	return 0xa5a5a5a5 & ~mem_mask;
}

static WRITE32_HANDLER( saturn_dsp_w )  /* DSP */
{
	logerror("dsp_w offset=%08lX data=%08lX mem_mask=%08lX\n", (long) offset*4, (long) data, (long) mem_mask);
}

/********************************************************
 *  VDP1                                                *
 ********************************************************/

static struct mame_bitmap *saturn_bitmap[2];
int video_w; /* indicates which bitmap is currently displayed and which is drawn */

struct _vdp1_state

{
  UINT16 vdp1_regs[0xC];
  UINT32 localx,localy;  /* Local x and y coordinates */
} vdp1_state;

static const char *vdp1_regnames[] =

{
  "TV Mode Selection            ",
  "Frame Buffer Switch          ",
  "Plot Trigger                 ",
  "Erase/Write Data             ",
  "Erase/Write Upper Left coord ",
  "Erase/Write Lower Right coord",
  "Plot Abnormal End            ",
  "Reserved                     ",
  "Transfer End Status          ",
  "Last Operation Addr          ",
  "Current Operation Addr       ",
  "Mode Status                  "
};

static void reset_vdp1(void)

{
  memset(vdp1_state.vdp1_regs,0,0xC<<1);
}

static void cmd0(UINT32 comm, unsigned short *fb)
{
	UINT32 *vram;
	short x,y;
	UINT32 color_mode;
	UINT32 color_bank;
	UINT32 char_addr;
	UINT32 width,height;
	int loopx,loopy;

	vram = vdp1_ram_base;
	comm = comm * 8;

	color_mode = (vram[comm + 1] >> 19) & 0x7; /* Pull out parameter infomation */
	color_bank = (vram[comm + 1] & 0xFFFF);
	char_addr  = (vram[comm + 2] >> 16) * 8;
	width      = ((vram[comm + 2] & 0xFFFF) >> 8) * 8;
	height     = ((vram[comm + 2] & 0xFFFF) & 0xFF);
	x = (short) (vram[comm + 3] >> 16); /* Cast as short to preserve sign */
	y = (short) (vram[comm + 3] & 0xFFFF);

	logerror("Colour Mode  = %d\n", color_mode);
	logerror("Colour Bank  = %08lX\n", (long) color_bank);
	logerror("Char Addr    = %08lX\n", (long) char_addr);
	logerror("Width,Height = %ld,%ld\n", (long) width, (long) height);
	logerror("X,Y = %d,%d\n",x,y);

	vram = vram + (char_addr / 4);
	x = x + vdp1_state.localx;
	y = y + vdp1_state.localy;

	height += y;
	width += x;

	if (color_mode == 5) {
		for(loopy = y;loopy < height;loopy++)	{
    		for(loopx = x;loopx < width;loopx+=2) {

			UINT32 colour;

			colour = *vram++;
			if(colour >> 16) {
 		   		plot_pixel(saturn_bitmap[video_w],loopx,
					loopy,Machine->pens[(colour>>16) & 0x7FFF]);
			}
			if(colour & 0xFFFF) {
				plot_pixel(saturn_bitmap[video_w],loopx+1,
					loopy,Machine->pens[colour&0x7FFF]);
			}
			}
		}
	}
}

static void execute_vdp1(void)
{
	/* Execute the vdp1 command set */
	UINT32 *base;
	UINT32 command;
	UINT32 temp;
	unsigned short fb[512*256];

	logerror("vdp1 execute command\n");

	base = vdp1_ram_base;
	command = 0;

	while(!(*base & 0x80000000))
	{
		switch((*base >> 16) & 0xF) /* Select command code */
	{
	case 0 : logerror("%08lX - Normal Sprite Draw\n", (long) command);
		cmd0(command,fb);
		break;
	case 1 : logerror("%08lX - Scaled Sprite Draw\n", (long) command);
		break;
	case 2 : logerror("%08lX - Distorted Sprite Draw\n", (long) command);
		break;
	case 4 : logerror("%08lX - Polygon Draw\n", (long) command);
		break;
	case 5 : logerror("%08lX - Polyline Draw\n", (long) command);
		break;
	case 6 : logerror("%08lX - Line Draw\n", (long) command);
		break;
	case 8 : logerror("%08lX - Set User Clip\n", (long) command);
		break;
	case 9 :
		temp = *(base + 5);
		logerror("%08lX - Set System Clip (%ld,%ld)\n", (long) command, (long) temp>>16, (long) temp&0xFFFF);
		break;
	case 10:
		temp = *(base + 3);
		logerror("%08lX - Local Coordinates (%ld,%ld)\n", (long) command, (long) temp>>16, (long) temp&0xFFFF);
		vdp1_state.localx = temp >> 16;
		vdp1_state.localy = temp & 0xFFFF;
		break;
	}
		base += (0x20/4);
		command++;
	}
	logerror("vdp1 execute end\n");
}

static READ32_HANDLER( saturn_vdp1_r )   /* VDP1 registers */
{
  UINT32 ret_val;

  ret_val = *(((UINT32 *) vdp1_state.vdp1_regs) + offset);
  ret_val = SWAP_WORDS(ret_val) & ~SWAP_WORDS(mem_mask);

  /* logerror("vdp1_r offset=%08lX mem_mask=%08lX ret_val=%08lX\n",offset*4,mem_mask,ret_val);*/
  if((mem_mask & 0xFFFF0000) == 0) /* If we are reading from first word in dword */
    {
      logerror("vdp1_r %s data=%04lX : PC=%08lX\n", vdp1_regnames[offset<<1], (long) ret_val & 0xFFFF, (long) activecpu_get_reg(SH2_PC));
    }
  if((mem_mask & 0xFFFF) == 0) /* If we are reading from 2nd word in dword */
    {
      logerror("vdp1_r %s data=%04lX : PC=%08lX\n", vdp1_regnames[(offset<<1)+1], (long) ret_val >> 16, (long) activecpu_get_reg(SH2_PC));
    }

  return SWAP_WORDS(ret_val);
}

static WRITE32_HANDLER( saturn_vdp1_w )  /* VDP1 registers */
{
	UINT32 olddata;

	/*  logerror("vdp1_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);*/
	if((mem_mask & 0xFFFF0000) == 0) /* If we are writing to first word in dword */
	{
		logerror("vdp1_w %s data=%04lX : PC=%08lX\n",
			vdp1_regnames[offset<<1],
			(long) data >> 16,
			(long) activecpu_get_reg(SH2_PC));
	}
	if((mem_mask & 0xFFFF) == 0) /* If we are writing to 2nd word in dword */
	{
		logerror("vdp1_w %s data=%04lX : PC=%08lX\n",
			vdp1_regnames[(offset<<1)+1],
			(long) data & 0xFFFF,
			(long) activecpu_get_reg(SH2_PC));
	}

	olddata = *(((UINT32 *) vdp1_state.vdp1_regs) + offset);
	olddata &= SWAP_WORDS(mem_mask);
	olddata |= SWAP_WORDS(data);

	*(((UINT32 *) vdp1_state.vdp1_regs) + offset) = olddata;
}

/********************************************************
 *  VDP2                                                *
 ********************************************************/

#define SCREEN_LINES 224 /* How many lines are actually displayed */
#define MAX_LINES 256 /* Max lines including blanked - NTSC res - PAL higher*/
#define FRAME_TIME 477273 /* Clock cycles per frame (~60Hz) */
#define LINE_TIME  (FRAME_TIME/MAX_LINES)   /* Approx cycles per line (~FRAME_TIME / 512) */

struct _vdp2_state

{
  UINT16 vdp2_regs[0x90];
} vdp2_state;

UINT32 HBlankCount;
UINT32 InVBlank;   /* Are we in vertical blanking ? */
void timer_hblank(int param);

static const char *vdp2_regnames[] =

{
  "TV Screen Mode                                  ",
  "Ext Signal Enable                               ",
  "Screen Status                                   ",
  "VRAM Size                                       ",
  "H-Counter                                       ",
  "V-Counter                                       ",
  "Reserved                                        ",
  "RAM Control                                     ",
  "VRAM Cycle (BANK A0) L                          ",
  "VRAM Cycle (BANK A0) U                          ",
  "VRAM Cycle (BANK A1) L                          ",
  "VRAM Cycle (BANK A1) U                          ",
  "VRAM Cycle (BANK B0) L                          ",
  "VRAM Cycle (BANK B0) U                          ",
  "VRAM Cycle (BANK B1) L                          ",
  "VRAM Cycle (BANK B1) U                          ",
  "Screen Display Enable                           ",
  "Mosaic Control                                  ",
  "Special Func Code Sel                           ",
  "Special Func Code                               ",
  "Char Control (NBG0, NBG1)                       ",
  "Char Control (NBG2, NBG3, RBG0)                 ",
  "Bitmap Pal No (NBG0, NBG1)                      ",
  "Bitmap Pal No (RBG0)                            ",
  "Pattern Name Ctrl (NBG0)                        ",
  "Pattern Name Ctrl (NBG1)                        ",
  "Pattern Name Ctrl (NBG2)                        ",
  "Pattern Name Ctrl (NBG3)                        ",
  "Pattern Name Ctrl (RGB0)                        ",
  "Plane Size                                      ",
  "Map Offs (NBG0-NBG3)                            ",
  "Map Offs (Rotation Param A,B)                   ",
  "Map (NBG0, Plane A,B)                           ",
  "Map (NBG0, Plane C,D)                           ",
  "Map (NBG1, Plane A,B)                           ",
  "Map (NBG1, Plane C,D)                           ",
  "Map (NBG2, Plane A,B)                           ",
  "Map (NBG2, Plane C,D)                           ",
  "Map (NBG3, Plane A,B)                           ",
  "Map (NBG3, Plane C,D)                           ",
  "Map (Rotation Param A, Plane A,B)               ",
  "Map (Rotation Param A, Plane C,D)               ",
  "Map (Rotation Param A, Plane E,F)               ",
  "Map (Rotation Param A, Plane G,H)               ",
  "Map (Rotation Param A, Plane I,J)               ",
  "Map (Rotation Param A, Plane K,L)               ",
  "Map (Rotation Param A, Plane M,N)               ",
  "Map (Rotation Param A, Plane O,P)               ",
  "Map (Rotation Param B, Plane A,B)               ",
  "Map (Rotation Param B, Plane C,D)               ",
  "Map (Rotation Param B, Plane E,F)               ",
  "Map (Rotation Param B, Plane G,H)               ",
  "Map (Rotation Param B, Plane I,J)               ",
  "Map (Rotation Param B, Plane K,L)               ",
  "Map (Rotation Param B, Plane M,N)               ",
  "Map (Rotation Param B, Plane O,P)               ",
  "Scr Scrl Val (NBG0, Horiz Integer Part)         ",
  "Scr Scrl Val (NBG0, Horiz Fraction Part)        ",
  "Scr Scrl Val (NBG0, Vert Integer Part)          ",
  "Scr Scrl Val (NBG0, Vert Fraction Part)         ",
  "Coord Inc (NBG0, Horiz Integer Part)            ",
  "Coord Inc (NBG0, Horiz Fraction Part)           ",
  "Coord Inc (NBG0, Vert Integer Part)             ",
  "Coord Inc (NBG0, Vert Fraction Part)            ",
  "Scr Scrl Val (NBG1, Horiz Integer Part)         ",
  "Scr Scrl Val (NBG1, Horiz Fraction Part)        ",
  "Scr Scrl Val (NBG1, Vert Integer Part)          ",
  "Scr Scrl Val (NBG1, Vert Fraction Part)         ",
  "Coord Inc (NBG1, Horiz Integer Part)            ",
  "Coord Inc (NBG1, Horiz Fraction Part)           ",
  "Coord Inc (NBG1, Vert Integer Part)             ",
  "Coord Inc (NBG1, Vert Fraction Part)            ",
  "Scr Scrl Val (NBG2, Horizontal)                 ",
  "Scr Scrl Val (NBG2, Vertical)                   ",
  "Scr Scrl Val (NBG3, Horizontal)                 ",
  "Scr Scrl Val (NBG3, Vertical)                   ",
  "Reduction Enable                                ",
  "Line, Vert Cell Scroll (NBG0, NBG1)             ",
  "Vert Cell Scrol Tbl Addt (NBG0, NBG1) U         ",
  "Vert Cell Scrol Tbl Addt (NBG0, NBG1) L         ",
  "Line Scrl Tbl Addr (NBG0) U                     ",
  "Line Scrl Tbl Addr (NBG0) L                     ",
  "Line Scrl Tbl Addr (NBG1) U                     ",
  "Line Scrl Tbl Addr (NBG1) L                     ",
  "Line Colour Scr Table Addr U                    ",
  "Line Colour Scr Table Addr L                    ",
  "Back Scr Tbl Addr U                             ",
  "Back Scr Tbl Addr L                             ",
  "Rotation Param Mode                             ",
  "Rotation Param Read Ctrl                        ",
  "Co-efficient Tbl Ctrl                           ",
  "Co-efficient Tbl Addr Offs (Rot Param A,B)      ",
  "Screen Over Pattern Name (Rot Param A)          ",
  "Screen Over Pattern Name (Rot Param B)          ",
  "Rot Param Tbl Addr (Rot Param A,B) U            ",
  "Rot Param Tbl Addr (Rot Param A,B) L            ",
  "Window Pos (W0, Horiz Start)                    ",
  "Window Pos (W0, Vert Start)                     ",
  "Window Pos (W0, Horiz End)                      ",
  "Window Pos (W0, Vert End)                       ",
  "Window Pos (W1, Horiz Start)                    ",
  "Window Pos (W1, Vert Start)                     ",
  "Window Pos (W1, Horiz End)                      ",
  "Window Pos (W1, Vert End)                       ",
  "Window Ctrl (NBG0, NBG1)                        ",
  "Window Ctrl (NBG2, NBG3)                        ",
  "Window Ctrl (RBG0, SPRITE)                      ",
  "Window Ctrl (Param Win, Colour Calc Win)        ",
  "Line Win Tbl Addr (W0) U                        ",
  "Line Win Tbl Addr (W0) L                        ",
  "Line Win Tbl Addr (W1) U                        ",
  "Line Win Tbl Addr (W1) L                        ",
  "Sprite Ctrl                                     ",
  "Shadow Ctrl                                     ",
  "Colour RAM Addr Offs (NBG0-NBG3)                ",
  "Colour RAM Addr Offs (RBG0, SPRITE)             ",
  "Line Colour Scr Enable                          ",
  "Special Priority Mode                           ",
  "Colour Calc Ctrl                                ",
  "Special Colour Calc Mode                        ",
  "Priority No (SPRITE 0,1)                        ",
  "Priority No (SPRITE 2,3)                        ",
  "Priority No (SPRITE 4,5)                        ",
  "Priority No (SPRITE 6,7)                        ",
  "Priority No (NBG0, NBG1)                        ",
  "Priority No (NBG2, NBG3)                        ",
  "Priority No (RGB0)                              ",
  "Reserved                                        ",
  "Colour Calc Ratio (SPRITE 0,1)                  ",
  "Colour Calc Ratio (SPRITE 2,3)                  ",
  "Colour Calc Ratio (SPRITE 4,5)                  ",
  "Colour Calc Ratio (SPRITE 6,7)                  ",
  "Colour Calc Ratio (NBG0, NBG1)                  ",
  "Colour Calc Ratio (NBG2, NBG3)                  ",
  "Colour Calc Ratio (RBG0)                        ",
  "Colour Calc Ratio (Line Colour Scr, Back Screen)",
  "Colour Offs Enable                              ",
  "Colour Offs Select                              ",
  "Colour Offs A (RED)                             ",
  "Colour Offs A (GREEN)                           ",
  "Colour Offs A (BLUE)                            ",
  "Colour Offs B (RED)                             ",
  "Colour Offs B (GREEN)                           ",
  "Colour Offs B (BLUE)                            "
};

static void reset_vdp2(void)
{
	HBlankCount = 0;
	timer_set(TIME_IN_CYCLES(LINE_TIME,0),0,timer_hblank);
	InVBlank = 0;
	memset(vdp2_state.vdp2_regs,0,0x90*2);
}

static void draw_1s8(UINT32 *vram_base,unsigned char *display,UINT32 pitch)

     /* Draws a 1Hx1V cell in 8bit colour */

{
  unsigned int loop;
  UINT32 vrtmp;

  for(loop = 0;loop < 16;loop+=2)
    {
	  vrtmp = *vram_base++;
      *display++ = (vrtmp >> 24); //& 0xFF;
      *display++ = (vrtmp >> 16); //& 0xFF;
      *display++ = (vrtmp >> 8); //& 0xFF;
      *display++ = (vrtmp); //& 0xFF;

	  vrtmp = *vram_base++;
      *display++ = (vrtmp >> 24); //& 0xFF;
      *display++ = (vrtmp >> 16); //& 0xFF;
      *display++ = (vrtmp >> 8); //& 0xFF;
      *display++ = (vrtmp); //& 0xFF;

      display += (pitch-8);
    }
}

static void render_plane(unsigned char *buffer,int pal,int trans)

{
  struct mame_bitmap *bitmap = saturn_bitmap[video_w];
  int loopx,loopy;
  int col;
  UINT32 *memt;

  pal = ((pal * 0x200) / 4);
  memt = &color_ram_base[pal]; /* &mem[SATURN_COLOR_RAM_BASE/4 + pal]; */

  if(!trans)
    {
      for(loopy = 0;loopy < 512;loopy++)
	{
	  for(loopx = 0;loopx < 512;loopx++)
	    {
	      col = *buffer++;
	      if(col & 1) {
			  col =  memt[col/2] & 0x7FFF;
		  } else {
			  col = (memt[col/2] >> 16) & 0x7FFF;
		  }
	      plot_pixel(bitmap,loopx,loopy,Machine->pens[col]);
	    }
	}
    }
  else
    {
      for(loopy = 0;loopy < 512;loopy++)
	{
	  for(loopx = 0;loopx < 512;loopx++)
	    {
	      col = *buffer++;
	      if(col)
		{
		  if(col & 1)
		    {
		      col = memt[col/2] & 0x7FFF;
		    }
		  else
		    {
		      col = (memt[col/2] >> 16) & 0x7FFF;
		    }
		  plot_pixel(bitmap,loopx,loopy,Machine->pens[col]);
		}
	    }
	}
    }
}

static void draw_nbg3(void)

{
  UINT32 planea_addr,planeb_addr,planec_addr,planed_addr;
  UINT16 *regs;
  unsigned char frame[512*512]; /* Temporary frame display */
  int loopx,loopy;
  UINT32 *pattern;
  static UINT32 pathi,patlo = 0xffffffff;

  regs = vdp2_state.vdp2_regs;

  planea_addr = (regs[0x4C>>1] & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0); /* Get plane start addresses */
  planeb_addr = ((regs[0x4C>>1] >> 8) & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);
  planec_addr = (regs[0x4E>>1] & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);
  planed_addr = ((regs[0x4E>>1] >> 8) & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);

  planea_addr <<= 14; /* *= 0x4000 */
  planeb_addr <<= 14;
  planec_addr <<= 14;
  planed_addr <<= 14;

  logerror("NBG3 Draw - PA=%08lX PB=%08lX PC=%08lX PD=%08lX\n",
	  (long) planea_addr,
	  (long) planeb_addr,
	  (long) planec_addr,
	  (long) planed_addr);

  pattern = &vdp2_ram_base[planea_addr/4];

  for(loopy = 0;loopy < 64;loopy++)
    {
      for(loopx = 0;loopx < 64;loopx++)
	{
	  UINT32 pat_no;

	  pat_no = *pattern++;
		if (patlo > pat_no) { patlo = pat_no; printf("new Pattern Low bound: [%d] PlaneA,C=%08x %08x Regs %x %x\n",pat_no,planea_addr,planec_addr,(UINT32)regs[0x4c>>1],(UINT32)regs[0x3c>>1]);}
		if (pathi < pat_no) { pathi = pat_no; /*printf("new Pattern  Hi bound: [%d] PlaneA,C=%08x %08x Regs %x %x\n",pat_no,planea_addr,planec_addr,(UINT32)regs[0x4c>>1],(UINT32)regs[0x3c>>1]);*/}

	  if (pat_no+1 < 4000) {
		  draw_1s8(&vdp2_ram_base[pat_no*8],&frame[loopy*4096 + loopx*8],512);
	   } else {
		   printf("bye bye\n");
	   }
	   /* note to Tyra: on crashing, loopx is 32, and planea_addr is 0, so is the reg[4c>>1], */
	   /* suspect the BIOS is switching resolutions to 1/2 what this function expects */
	   /* also, why do you compute unused plane addrs in this function? */
	}
    }
  render_plane(frame,3,0);
  /*  {
    FILE *fp;
    fp = fopen("nbg3.bin","wb");
    fwrite(frame,512,512,fp);
    fclose(fp);
    }*/
}

static void draw_nbg2(void)

{
  UINT32 planea_addr,planeb_addr,planec_addr,planed_addr;
  UINT16 *regs;
  unsigned char frame[512*512];
  int loopx,loopy;
  UINT32 *pattern;

  regs = vdp2_state.vdp2_regs;

  planea_addr = (regs[0x48>>1] & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0); /* Get plane start addresses */
  planeb_addr = ((regs[0x48>>1] >> 8) & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);
  planec_addr = (regs[0x4A>>1] & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);
  planed_addr = ((regs[0x4A>>1] >> 8) & 0x3F) | ((regs[0x3C>>1]>>6) & 0x1C0);

  planea_addr <<= 14; /* *= 0x4000 */
  planeb_addr <<= 14;
  planec_addr <<= 14;
  planed_addr <<= 14;

  logerror("NBG2 Draw - PA=%08lX PB=%08lX PC=%08lX PD=%08lX\n",
	  (long) planea_addr,
	  (long) planeb_addr,
	  (long) planec_addr,
	  (long) planed_addr);

  pattern = &vdp2_ram_base[planea_addr/4];

  for(loopy = 0;loopy < 64;loopy++)
    {
      for(loopx = 0;loopx < 64;loopx++)
	{
	  UINT32 pat_no;

	  pat_no = *pattern++;
	  draw_1s8(&vdp2_ram_base[pat_no*8],&frame[loopy*4096 + loopx*8],512);
	}
    }
  render_plane(frame,2,1);
  /*  {
    FILE *fp;
    fp = fopen("nbg2.bin","wb");
    fwrite(frame,512,512,fp);
    fclose(fp);
    }*/
}

void timer_hblank(int param)

     /*
	Called everytime we need a HBlank. Easier to count this and call VBlanks than do vblanks
	as SCU timers require timings based of this.
     */

{
  //  logerror("HBlank Interrupt %ld\n",HBlankCount); /* Logging hblanks is abit of a waste :) */
  timer_set(TIME_IN_CYCLES(LINE_TIME,0),0,timer_hblank); /* Reset timer */
  HBlankCount++;
  if((HBlankCount > SCREEN_LINES) && (!InVBlank))
    {
      /* We are going into vertical blanking area */
      /* Execute VBlank-IN interrupt */
      InVBlank = 1;
      scu_pulse_interrupt(VBLANK_IN_INT);
      //logerror("VBlankIN\n");
    }
  else
    {
      if(HBlankCount >= MAX_LINES)
	{
	  /* We are coming out of Vblank */
	  /* Setup up VBlank-OUT Interrupt */
	  InVBlank = 0;
	  HBlankCount = 0; /* Reset hblank counter */
	  scu_pulse_interrupt(VBLANK_OUT_INT);

	  /* Draw display */
	  if(vdp2_state.vdp2_regs[0] & 0x8000)
	    {
	      draw_nbg3();
	      draw_nbg2();
	      execute_vdp1();
	      //  logerror("Drawing screen\n");
	    }
	  else
	    {
	      fillbitmap(saturn_bitmap[video_w & 1],Machine->pens[0],NULL);
	      //	      logerror("Clearing bitmap\n");
	    }
	  video_w ^= 1; /* Flip write buffers over */
	  //	  logerror("VBlankOUT %08lX %01X\n",vdp2_state.vdp2_regs[0],video_w);
	}
      else
	{
	  /* Issue H-Blank (bit of a hack :P) */
	}
    }
}

/*
static void dump_vdp2(void)

{
  FILE *fp;
  int loop;

  fp = fopen("vdp2.bin","wb");
  if(fp != NULL)
    {
      for(loop = 0;loop < 0x20000;loop++)
	{
	  putc((mem[loop + (SATURN_VDP2_RAM_BASE/4)] >> 24) & 0xFF,fp);
	  putc((mem[loop + (SATURN_VDP2_RAM_BASE/4)] >> 16) & 0xFF,fp);
	  putc((mem[loop + (SATURN_VDP2_RAM_BASE/4)] >> 8) & 0xFF,fp);
	  putc(mem[loop + (SATURN_VDP2_RAM_BASE/4)] & 0xFF,fp);
	}
      fclose(fp);
    }

  fp = fopen("colorram.bin","wb");
  if(fp != NULL)
    {
      for(loop = 0;loop < (0x1000/4);loop++)
	{
	  putc((mem[loop + (SATURN_COLOR_RAM_BASE/4)] >> 24) & 0xFF,fp);
	  putc((mem[loop + (SATURN_COLOR_RAM_BASE/4)] >> 16) & 0xFF,fp);
	  putc((mem[loop + (SATURN_COLOR_RAM_BASE/4)] >> 8) & 0xFF,fp);
	  putc(mem[loop + (SATURN_COLOR_RAM_BASE/4)] & 0xFF,fp);
	}
      fclose(fp);
    }
  fp = fopen("vdp1.bin","wb");
  if(fp != NULL)
    {
      for(loop = 0;loop < 0x20000;loop++)
	{
	  putc((vdp1_ram_base[loop] >> 24) & 0xFF,fp);
	  putc((vdp1_ram_base[loop] >> 16) & 0xFF,fp);
	  putc((vdp1_ram_base[loop] >> 8)  & 0xFF,fp);
	  putc((vdp1_ram_base[loop]     )  & 0xFF,fp);
	}
      fclose(fp);
    }
}

static void draw_pal(void)

{
  UINT32 loopx,loopy,col;
  int rectx,recty;

  for(loopy = 0;loopy < 32;loopy++)
    for(loopx = 0;loopx < 32;loopx++)
      {
	col = loopy * 32 + loopx;
	if(col & 1)
	  {
	    col = color_ram_base[col/2] & 0x7FFF;
	  }
	else
	  {
	    col = (color_ram_base[col/2] >> 16) & 0x7FFF;
	  }

	logerror("pal %08lX = %08lX\n",loopy*32 + loopx,col);
	for(recty = 0;recty < 16;recty++)
	  for(rectx = 0;rectx < 16;rectx++)
	    plot_pixel(saturn_bitmap[video_w],loopx*16 + rectx,loopy*16 + recty,Machine->pens[col]);
      }
}
*/

static READ32_HANDLER( saturn_vdp2_r )   /* VDP2 registers */
{
  UINT32 ret_val;

  ret_val = *(((UINT32 *) vdp2_state.vdp2_regs) + offset);

#if PAL
  if(offset == 0x1)
    {
      ret_val |= 0x00010000;
    }
#endif

	ret_val = SWAP_WORDS(ret_val) & ~SWAP_WORDS(mem_mask);

	/*  logerror("vdp2_r offset=%08lX mem_mask=%08lX ret_val=%08lX\n",offset*4,mem_mask,ret_val);*/
	if((mem_mask & 0xFFFF0000) == 0) /* If we are reading from first word in dword */
	{
		logerror("vdp2_r %s data=%04lX : PC=%08lX\n",
			vdp2_regnames[offset<<1],
			(long) ret_val & 0xFFFF,
			(long) activecpu_get_reg(SH2_PC));
	}
	if((mem_mask & 0xFFFF) == 0) /* If we are reading from 2nd word in dword */
	{
		logerror("vdp2_r %s data=%04lX : PC=%08lX\n",
			vdp2_regnames[(offset<<1)+1],
			(long) ret_val >> 16,
			(long) activecpu_get_reg(SH2_PC));
	}
	return SWAP_WORDS(ret_val);
}

static WRITE32_HANDLER( saturn_vdp2_w )  /* VDP2 registers */
{
	UINT32 olddata;

	/* logerror("vdp2_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);*/
	if((mem_mask & 0xFFFF0000) == 0) /* If we are writing to first word in dword */
	{
		logerror("vdp2_w %s data=%04lX : PC=%08lX\n",
			vdp2_regnames[offset<<1],
			(long) data >> 16,
			(long) activecpu_get_reg(SH2_PC));
	}
	if((mem_mask & 0xFFFF) == 0) /* If we are writing to 2nd word in dword */
	{
		logerror("vdp2_w %s data=%04lX : PC=%08lX\n",
			vdp2_regnames[(offset<<1)+1],
			(long) data & 0xFFFF,
			(long) activecpu_get_reg(SH2_PC));
	}

	olddata = *(((UINT32 *) vdp2_state.vdp2_regs) + offset);
	olddata &= SWAP_WORDS(mem_mask);
	olddata |= SWAP_WORDS(data);

	*(((UINT32 *) vdp2_state.vdp2_regs) + offset) = olddata;

	if(offset == 0)
	{
		if(data & 0x80000000)
		{
			logerror("vdp_w Screen Enabled\n");
			/*	  dump_vdp2();
			video_w = 0;
			draw_nbg3();
			draw_nbg2();
			execute_vdp1();
			video_w = 1;
			draw_nbg3();
			draw_nbg2();
			execute_vdp1();*/
			//draw_pal();
		}
	}
}

/* 68k handlers */

static READ16_HANDLER( dsp_68k_r )

{
	logerror("16 bit DSP READ offset %08x mask %08x\n",(UINT32)offset,(UINT32)mem_mask);
  return 0xdeed;
}

static WRITE16_HANDLER( dsp_68k_w )

{
	logerror("16 bit DSP WRITE offset %08x mask %08x data %08x\n",(UINT32)offset,(UINT32)mem_mask,(UINT32)data);
}

/********************************************************
 *  Main Machine Code                                   *
 ********************************************************/

static MACHINE_INIT( saturn )
{
	int i;
	UINT32 *mem2;
	int mem_length;

	mem = (UINT32 *) memory_region(REGION_CPU1);
	mem2 = (UINT32 *) memory_region(REGION_CPU2);
	fb1_ram_base = (UINT32 *) &mem[SATURN_FB1_RAM_BASE/4];
	workl_ram_base = (UINT32 *) &mem[SATURN_WORKL_RAM_BASE/4];
	vdp1_ram_base = (UINT32 *) &mem[SATURN_VDP1_RAM_BASE/4];
	color_ram_base = (UINT32 *) &mem[SATURN_COLOR_RAM_BASE/4];
	workh_ram_base = (UINT32 *) &mem[SATURN_WORKH_RAM_BASE/4];
	vdp2_ram_base = (UINT32 *) &mem[SATURN_VDP2_RAM_BASE/4];
	back_ram_base = (UINT32 *) &mem[SATURN_BACK_RAM_BASE/4];

	/* Copy bios rom into second cpu area */
	memcpy(mem2,mem,SATURN_ROM_SIZE);

	mem_length = (memory_region_length(REGION_CPU1) - SATURN_ROM_SIZE) / 4;

	for (i = (SATURN_ROM_SIZE/4);i < mem_length; i++)
	{
		mem[i] = 0; /* Clear RAM */
	}

	sound_base = (UINT16 *) memory_region(REGION_CPU3); /*
	Setup reset vector
	for 68k stupidity */

	*(sound_base + 0) = 0;
	*(sound_base + 1) = 0x1000;
	*(sound_base + 2) = 0;
	*(sound_base + 3) = 8;
	*(sound_base + 4) = 0x60fe;

	/* Install memory handlers. Must be done dynamically to avoid allocating too much ram */

	for (i = 0; i < 2; i++)
	{
		install_mem_read32_handler (i, 0x00000000, 0x0007ffff, MRA32_ROM );
		install_mem_write32_handler(i, 0x00000000, 0x0007ffff, MWA32_ROM );

		install_mem_read32_handler (i, 0x00100000, 0x0010007f, saturn_smpc_r );
		install_mem_write32_handler(i, 0x00100000, 0x0010007f, saturn_smpc_w );

		install_mem_read32_handler (i, 0x00180000, 0x0019ffff, saturn_back_ram_r );
		install_mem_write32_handler(i, 0x00180000, 0x0019ffff, saturn_back_ram_w );

		install_mem_read32_handler (i, 0x00200000, 0x002fffff, MRA32_BANK1 /*saturn_workl_ram_r*/ );
		install_mem_write32_handler(i, 0x00200000, 0x002fffff, MWA32_BANK1 /*saturn_workl_ram_w*/ );

		install_mem_read32_handler (i, 0x01000000, 0x01000003, saturn_minit_r );
		install_mem_write32_handler(i, 0x01000000, 0x01000003, saturn_minit_w );

		install_mem_read32_handler (i, 0x01800000, 0x01800003, saturn_sinit_r );
		install_mem_write32_handler(i, 0x01800000, 0x01800003, saturn_sinit_w );

		install_mem_read32_handler (i, 0x02000000, 0x03ffffff, saturn_cs0_r );
		install_mem_write32_handler(i, 0x02000000, 0x03ffffff, saturn_cs0_w );

		install_mem_read32_handler (i, 0x04000000, 0x04ffffff, saturn_cs1_r );
		install_mem_write32_handler(i, 0x04000000, 0x04ffffff, saturn_cs1_w );

		install_mem_read32_handler (i, 0x05000000, 0x057fffff, saturn_cs2_r );
		install_mem_write32_handler(i, 0x05000000, 0x057fffff, saturn_cs2_w );

		install_mem_read32_handler (i, 0x05890000, 0x0589ffff, saturn_cd_r );
		install_mem_write32_handler(i, 0x05890000, 0x0589ffff, saturn_cd_w );

		install_mem_read32_handler (i, 0x05a00000, 0x05a7ffff, saturn_sound_ram_r );
		install_mem_write32_handler(i, 0x05a00000, 0x05a7ffff, saturn_sound_ram_w );
		install_mem_read32_handler (i, 0x05a80000, 0x05afffff, MRA32_NOP );
		install_mem_write32_handler(i, 0x05a80000, 0x05afffff, MWA32_NOP );

		install_mem_read32_handler (i, 0x05b00000, 0x05b00ee3, saturn_dsp_r );
		install_mem_write32_handler(i, 0x05b00000, 0x05b00ee3, saturn_dsp_w );

		install_mem_read32_handler (i, 0x05c00000, 0x05c7ffff, saturn_vdp1_ram_r );
		install_mem_write32_handler(i, 0x05c00000, 0x05c7ffff, saturn_vdp1_ram_w );

		install_mem_read32_handler (i, 0x05c80000, 0x05cbffff, saturn_fb1_ram_r );
		install_mem_write32_handler(i, 0x05c80000, 0x05cbffff, saturn_fb1_ram_w );

		install_mem_read32_handler (i, 0x05d00000, 0x05d00017, saturn_vdp1_r );
		install_mem_write32_handler(i, 0x05d00000, 0x05d00017, saturn_vdp1_w );

		install_mem_read32_handler (i, 0x05e00000, 0x05e7ffff, saturn_vdp2_ram_r );
		install_mem_write32_handler(i, 0x05e00000, 0x05e7ffff, saturn_vdp2_ram_w );

		install_mem_read32_handler (i, 0x05f00000, 0x05f00fff, saturn_color_ram_r );
		install_mem_write32_handler(i, 0x05f00000, 0x05f00fff, saturn_color_ram_w );

		install_mem_read32_handler (i, 0x05f80000, 0x05f8011f, saturn_vdp2_r );
		install_mem_write32_handler(i, 0x05f80000, 0x05f8011f, saturn_vdp2_w );

		install_mem_read32_handler (i, 0x05fe0000, 0x05fe00cf, saturn_scu_r );
		install_mem_write32_handler(i, 0x05fe0000, 0x05fe00cf, saturn_scu_w );

		install_mem_read32_handler (i, 0x06000000, 0x060fffff, MRA32_BANK2 );
		install_mem_write32_handler(i, 0x06000000, 0x060fffff, MWA32_BANK2 );
	}

	install_mem_read16_handler(2, 0x000000, 0x07ffff, MRA16_BANK3);
	install_mem_write16_handler(2, 0x000000, 0x07ffff, MWA16_BANK3);
	install_mem_read16_handler(2, 0x100000, 0x100ee3, dsp_68k_r);
	install_mem_write16_handler(2, 0x100000, 0x100ee3, dsp_68k_w);

	cpu_setbank(1, (UINT8 *) workl_ram_base); /* Setup banking (for???) */
	cpu_setbank(2, (UINT8 *) workh_ram_base);
	cpu_setbank(3, (UINT8 *) sound_base);
/*  cpu_setbank(4, (UINT8 *) &mem[SATURN_VDP1_RAM_BASE/4]);
  cpu_setbank(5, (UINT8 *) &mem[SATURN_VDP2_RAM_BASE/4]);
  cpu_setbank(6, (UINT8 *) &mem[SATURN_FB1_RAM_BASE/4]);
  cpu_setbank(7, (UINT8 *) &mem[SATURN_FB2_RAM_BASE/4]);
  cpu_setbank(8, (UINT8 *) &mem[SATURN_COLOR_RAM_BASE/4]);
  cpu_setbank(9, (UINT8 *) &mem[SATURN_BACK_RAM_BASE/4]);*/
}

static DRIVER_INIT( saturn )
{
	reset_smpc();
	reset_cd();
	reset_scu();
	reset_vdp1();
	reset_vdp2();
}

static VIDEO_START( saturn )
{
	logerror("saturn_vh_start\n");
	saturn_bitmap[0] = auto_bitmap_alloc_depth(SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT, 16);
	saturn_bitmap[1] = auto_bitmap_alloc_depth(SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT, 16);

	if((!saturn_bitmap[0]) || (!saturn_bitmap[1]))
		return 1;

	fillbitmap(saturn_bitmap[0],Machine->pens[0x7FFF],NULL);
	fillbitmap(saturn_bitmap[1],Machine->pens[0],NULL);
	video_w = 0;

	return 0;
}


static VIDEO_UPDATE( saturn )
{
	//  logerror("saturn_vh_screenrefresh\n");
	if(saturn_bitmap[video_w])
	{
		copybitmap(bitmap, saturn_bitmap[video_w], 0, 0, 0, 0, NULL, TRANSPARENCY_NONE, 0);
	}
}

/*
   all read/write mem's are setup in init functions
   These are stubs so that mame doesn't segfault on NULL pointer access. doh :)
*/

static MEMORY_READ32_START( saturn_readmem )
MEMORY_END

static MEMORY_WRITE32_START( saturn_writemem )
MEMORY_END

INPUT_PORTS_START( saturn )

     PORT_START /* DIP switches */
     PORT_BIT(0xff, 0xff, IPT_UNUSED)

INPUT_PORTS_END

static MEMORY_READ16_START( readmem_68k )
MEMORY_END

static MEMORY_WRITE16_START( writemem_68k )
MEMORY_END

static PALETTE_INIT( saturn )
{
	/*Setup the internal palette to 15bit colour */
	int i;

	for ( i = 0; i < 0x8000; i++ )
	{
		int r, g, b;

		r = (( i >> 10 ) & 0x1f) << 3;
		g = (( i >> 5 ) & 0x1f) << 3;
		b = (i & 0x1f) << 3;

		palette_set_color(i, b, g, r);

		colortable[i] = i;
	}

	logerror("saturn_init_palette\n");
}


static MACHINE_DRIVER_START( saturn )
	/* basic machine hardware */
	MDRV_CPU_ADD(SH2, 28636400)			/* NTSC Clock speed at 352/704 Pixel/line dot clock */
	MDRV_CPU_MEMORY(saturn_readmem, saturn_writemem)
	MDRV_CPU_ADD(SH2, 28636400)			/* NTSC Clock speed at 352/704 Pixel/line dot clock */
	MDRV_CPU_MEMORY(saturn_readmem, saturn_writemem)
	MDRV_CPU_ADD(M68000, 11300000)		/* Sound CPU; 11.3mhz (MC68000-12)*/
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_68k,writemem_68k)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( saturn )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT)
	MDRV_VISIBLE_AREA(0, SATURN_SCR_WIDTH-1, 0, SATURN_SCR_HEIGHT-1)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(32768)
	MDRV_PALETTE_INIT( saturn )

	MDRV_VIDEO_START( saturn )
	MDRV_VIDEO_UPDATE( saturn )
MACHINE_DRIVER_END


ROM_START(saturn)
     ROM_REGION(0x00421000, REGION_CPU1,0)
     /*ROM_LOAD("sega_100.bin", 0x00000000, 0x00080000,CRC( 0x2ABA43C2)) */
     ROM_LOAD("sega_101.bin", 0x00000000, 0x00080000,CRC( 0x224b752c))
     /*ROM_LOAD("sega_eur.bin", 0x00000000, 0x00080000,CRC( 0x4AFCF0FA)) */
     /*Make sure you set the PAL define to 1 otherwise euro bios will lock badly */

     /* STV Bios Note these are in correct endian order. not byte swapped versions */
     /* ROM_LOAD("mp17951a.s", 0x00000000, 0x00080000,CRC( 0x574FD2C3))*/
     /*ROM_LOAD("mp17952a.s", 0x00000000, 0x00080000,CRC( 0xBF7DBDD7)) */
     ROM_REGION(0x00080000, REGION_CPU2,0)
     ROM_REGION(0x00080000, REGION_CPU3,0)
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY	FULLNAME */
CONSX(1994, saturn,	0,		0,		saturn,	saturn,	saturn,	NULL,	"Sega",	"Saturn", GAME_NOT_WORKING)

