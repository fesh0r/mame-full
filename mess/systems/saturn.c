/* 
   Sega Saturn Driver - Copyright James Forshaw (TyRaNiD) July 2001

   Almost total rewrite. Uses the basic memory model from 
   old saturn driver by Juergen Buchmueller <pullmoll@t-online.de>,
   although its been fixed up to work with the new mame memory model
   and to correct a few errors. Little else has survived.

   Memory Map

   0x0000000 > 0x0080000		: Boot (IPL) ROM
   0x0100000 > 0x0100080		: SMPC register area
   0x0180000 > 0x0190000		: Backup RAM 
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
   Can run for a reasonable period, and most of the main bios title screen is setup.
   No drawing occurs and the cpu locks in an infinite loop (Probably waiting on VBlank handling) 

   -= UPDATES =-
   
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
#define VERBOSE 1
#endif

#define DISP_MEM 1 /* define to log memory access to all non work ram areas */

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

static UINT32 *mem; /* Base memory pointer */

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
#define SATURN_SOUND_RAM_BASE   0x00280000
#define SATURN_SOUND_RAM_SIZE   0x00080000
#define SATURN_VDP1_RAM_BASE    0x00300000
#define SATURN_VDP1_RAM_SIZE    0x00080000
#define SATURN_VDP2_RAM_BASE    0x00380000
#define SATURN_VDP2_RAM_SIZE    0x00080000
#define SATURN_FB1_RAM_BASE     0x00400000
#define SATURN_FB1_RAM_SIZE     0x00040000
#define SATURN_FB2_RAM_BASE     0x00440000
#define SATURN_FB2_RAM_SIZE     0x00040000
#define SATURN_COLOR_RAM_BASE   0x00480000
#define SATURN_COLOR_RAM_SIZE   0x00001000
#define SATURN_BACK_RAM_BASE    0x00481000
#define SATURN_BACK_RAM_SIZE    0x00010000

#define SATURN_SCR_WIDTH    704
#define SATURN_SCR_HEIGHT   512

#ifdef LSB_FIRST
#define SWAP_WORDS(x) (((x)<<16) | ((x)>>16)) /* Swaps over words so its easier to interface 32 and 16 bit datas */
#else
#define SWAP_WORDS(x) (x) /* On MSB systems no need to swap */
#endif

/* Memory handlers */
/* Read handler get offset (byte offset / 4) and mem_mask (all bytes required are 0) */
/* Write handler get data, offset and mem_mask */

READ32_HANDLER( saturn_rom_r )	  /* ROM UNUSED */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_rom_w )   /* ROM UNUSED */
{
}

READ32_HANDLER( saturn_workl_ram_r )
{
  offs_t ea;

  ea = (SATURN_WORKL_RAM_BASE / 4) + offset; /* Calculate effective address */
  return mem[ea] & (~mem_mask); /* Return dword masked with NOT mem_mask */
}

WRITE32_HANDLER( saturn_workl_ram_w )
{
  offs_t ea;

  ea = (SATURN_WORKL_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_workh_ram_r )
{
  offs_t ea;

  ea = (SATURN_WORKH_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_workh_ram_w )
{
  offs_t ea;

  ea = (SATURN_WORKH_RAM_BASE / 4) + offset;
  /*  if(offset == (0x22C / 4))
    logerror("workram - at 0x22c offset=%08lX data=%08lX mem_mask=%08lX PC=%08lX\n",
    offset,data,mem_mask,cpu_get_reg(SH2_PC));*/

  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_sound_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("soundram_r offset=%08lX mem_mask=%08lX PC=%08lX\n",offset,mem_mask,cpu_get_reg(SH2_PC));
#endif

  ea = (SATURN_SOUND_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_sound_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("soundram_w offset=%08lX data=%08lX mem_mask=%08lX PC=%08lX\n",offset,data,mem_mask,cpu_get_reg(SH2_PC));
#endif

  ea = (SATURN_SOUND_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_vdp1_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("vdp1ram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_VDP1_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_vdp1_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("vdp1ram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_VDP1_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_vdp2_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("vdp2ram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_VDP2_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_vdp2_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("vdp2ram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_VDP2_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_fb1_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("fb1_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_FB1_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_fb1_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("fb1_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_FB1_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

/* FB2 not mapped directly */
READ32_HANDLER( saturn_fb2_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("fb2_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_FB2_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_fb2_ram_w )

{
  offs_t ea;

#ifdef DISP_MEM
  logerror("fb2_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_FB2_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}
/* END FB2 */

READ32_HANDLER( saturn_color_ram_r )

{
  offs_t ea;

#ifdef DISP_MEM
  logerror("colorram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_COLOR_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_color_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("colorram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_COLOR_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
}

READ32_HANDLER( saturn_back_ram_r )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("backram_r offset=%08lX mem_mask=%08lX\n",offset,mem_mask);
#endif

  ea = (SATURN_BACK_RAM_BASE / 4) + offset;
  return mem[ea] & (~mem_mask);
}

WRITE32_HANDLER( saturn_back_ram_w )
{
  offs_t ea;

#ifdef DISP_MEM
  logerror("backram_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
#endif

  ea = (SATURN_BACK_RAM_BASE / 4) + offset;
  mem[ea] = (mem[ea] & mem_mask) | data;
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

void reset_smpc(void)

     /* Reset SMPC system */

{
  memset(smpc_state.smpc_regs,0,0x80); /* Clear SMPC regs */
  smpc_state.smpc_regs[STATUSF] = 1;   /* Initially set to 1 ?? */
  memset(smpc_state.smem,0,4);
  smpc_state.reset_disable = 1;  /* Initially reset is disabled */
  smpc_state.set_timer = SET_TIMER; /* Setup set timer state */
}

void smpc_execcomm(int commcode)

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
	case 0x7 : 
	  logerror("smpc - Sound OFF (0x7)\n");
	  break;
	case 0x6 :
	  logerror("smpc - Sound ON (0x6)\n");
	  break;
	}
      smpc_state.smpc_regs[OREG(31)] = commcode; 
      smpc_state.smpc_regs[STATUSF]  = 0;
    }
} 

READ32_HANDLER( saturn_smpc_r )   /* SMPC */
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
	      logerror("smpc_r IREG%01d -> %02lX - PC=%08lX\n",(ea-IREG(0))/2,d,cpu_get_reg(SH2_PC));
	    }
	  else
	    if((ea >= OREG(0)) && (ea <= OREG(31)))
		{
		  logerror("smpc_r OREG%01d -> %02lX - PC=%08lX\n",(ea-OREG(0))/2,d,cpu_get_reg(SH2_PC));
		}
	    else
	      {
		switch(ea) /* See if the write is significant */
		  {
		  case COMMREG : logerror("smpc_r COMMREG - command = %02lX - PC=%08lX\n",d
					  ,cpu_get_reg(SH2_PC));
		    break;
		  case STATUSR : logerror("smpc_r SR - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case STATUSF : logerror("smpc_r SF - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case PDR1    : logerror("smpc_r PDR1 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case DDR1    : logerror("smpc_r DDR1 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case PDR2    : logerror("smpc_r PDR2 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case DDR2    : logerror("smpc_r DDR2 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case IOSEL   : logerror("smpc_r IOSEL - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case EXEL    : logerror("smpc_r EXEL - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  default      : logerror("smpc_r offset=%08lX data=%02lX - PC=%08lX\n",ea,d,cpu_get_reg(SH2_PC));
		  }
	      }
	}
      temp_mask >>= 8;
    }

  return ret_val & ~mem_mask;
}

WRITE32_HANDLER( saturn_smpc_w )  /* SMPC */
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
	      logerror("smpc_w IREG%01d <- %02lX - PC=%08lX\n",(ea-IREG(0))/2,d,cpu_get_reg(SH2_PC));
	    }
	  else
	    if((ea >= OREG(0)) && (ea <= OREG(31)))
		{
		  logerror("smpc_w OREG%01d <- %02lX - PC=%08lX\n",(ea-OREG(0))/2,d,cpu_get_reg(SH2_PC));
		}
	    else
	      {
		switch(ea) /* See if the write is significant */
		  {
		  case COMMREG : logerror("smpc_w COMMREG - command = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case STATUSR : logerror("smpc_w SR - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case STATUSF : logerror("smpc_w SF - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case PDR1    : logerror("smpc_w PDR1 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case DDR1    : logerror("smpc_w DDR1 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case PDR2    : logerror("smpc_w PDR2 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case DDR2    : logerror("smpc_w DDR2 - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case IOSEL   : logerror("smpc_w IOSEL - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  case EXEL    : logerror("smpc_w EXEL - data = %02lX - PC=%08lX\n",d,cpu_get_reg(SH2_PC));
		    break;
		  default      : logerror("smpc_w offset=%08lX data=%02lX - Pc=%08lX\n",ea,d,cpu_get_reg(SH2_PC));
		  }
	      }
	}
      temp_mask >>= 8;
    }  
}

READ32_HANDLER( saturn_cs0_r )	  /* CS0 */
{
  logerror("cs0_r offset=%08lX mem_mask=%08lX\n",offset*4,mem_mask);

  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_cs0_w )   /* CS0 */
{
}

READ32_HANDLER( saturn_cs1_r )	  /* CS1 */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_cs1_w )   /* CS1 */
{
}

READ32_HANDLER( saturn_cs2_r )	  /* CS2 */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_cs2_w )   /* CS2 */
{
}

/* SCU Handler */

const char scu_regnames[0x34][15] = {{"DMA0 Read"},     /* 0x00 */
				     {"DMA0 Write"},    /* 0x04 */
				     {"DMA0 Count"},    /* 0x08 */
				     {"DMA0 Addr add"}, /* 0x0C */
				     {"DMA0 Enable"},   /* 0x10 */
				     {"DMA0 Mode"},     /* 0x14 */
				     {"X18"},           /* 0x18 */
				     {"X1C"},           /* 0x1C */
				     {"DMA1 Read"},     /* 0x20 */
				     {"DMA1 Write"},    /* 0x24 */
				     {"DMA1 Count"},    /* 0x28 */
				     {"DMA1 Addr add"}, /* 0x2C */
				     {"DMA1 Enable"},   /* 0x30 */
				     {"DMA1 Mode"},     /* 0x34 */
				     {"X38"},           /* 0x38 */
				     {"X3C"},           /* 0x3C */
				     {"DMA2 Read"},     /* 0x40 */
				     {"DMA2 Write"},    /* 0x44 */
				     {"DMA2 Count"},    /* 0x48 */
				     {"DMA2 Addr add"}, /* 0x4C */
				     {"DMA2 Enable"},   /* 0x50 */
				     {"DMA2 Mode"},     /* 0x54 */
				     {"X58"},           /* 0x58 */
				     {"X5C"},           /* 0x5C */
				     {"X60"},           /* DMA force stop. Doesn't exist */
				     {"X64"},
				     {"X68"},
				     {"X6C"},
				     {"X70"},
				     {"X74"},
				     {"X78"},
				     {"X7C"},
				     {"DSP Ctrl Port"},
				     {"DSP Prog RAM"},
				     {"DSP Data Addr"},
				     {"DSP Data Data"},
				     {"Timer0 Compare"},
				     {"Timer1 Set"},
				     {"Timer1 Mode"},
				     {"X9C"},
				     {"Int Mask"},
				     {"Int Stat"},
				     {"A-Bus IntAck"},
				     {"XAC"},
				     {"A-Bus Set 0"},
				     {"A-Bus Set 1"},
				     {"A-Bus Refresh"},
				     {"XBC"},
				     {"XC0"},
				     {"SDRAM select"},
				     {"SCU Version"},
				     {"XCC"}};
				   
		      

UINT32 scu_regs[0x34]; /* SCU register block temporary structure*/ 

void reset_scu(void)

{
  memset(scu_regs,0,0x34*4);
}

READ32_HANDLER( saturn_scu_r )	  /* SCU, DMA/DSP */

{
  logerror("scu_r %s - data = %08lX - PC=%08lX\n",scu_regnames[offset],scu_regs[offset],cpu_get_reg(SH2_PC));
  return scu_regs[offset] & ~mem_mask;
}

WRITE32_HANDLER( saturn_scu_w )   /* SCU, DMA/DSP */
{
  logerror("scu_w %s - data = %08lX - PC=%08lX\n",scu_regnames[offset],data,cpu_get_reg(SH2_PC));
  scu_regs[offset] = (scu_regs[offset] & mem_mask) | data;
}

const char cd_regnames[0xA][10] = {{"X0"},
				   {"X4"},
				   {"HIRQ"},
				   {"HIRQ Mask"},
				   {"X10"},
				   {"X14"},
				   {"CR1"},
				   {"CR2"},
				   {"CR3"},
				   {"CR4"}};
				  
UINT32 cd_regs[0xA];
UINT32 periodic; /* Currently a hack to bypass bios area */

#define CD_HIRQ     0x2
#define CD_HIRQMASK 0x3
#define CD_CR1      0x6
#define CD_CR2      0x7
#define CD_CR3      0x8
#define CD_CR4      0x9

void reset_cd(void)
     
{
  cd_regs[CD_CR1]  = 'C' << 16;
  cd_regs[CD_CR2]  = ('D' << 24) | ('B' << 16);
  cd_regs[CD_CR3]  = ('L' << 24) | ('O' << 16);
  cd_regs[CD_CR4]  = ('C' << 24) | ('K' << 16);
  cd_regs[CD_HIRQ] = 1;
  cd_regs[CD_HIRQMASK] = 0xBE1;
  periodic = 0; /* This will toggle every other read so that bios will execute */
}

void cd_execcomm(void)

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

READ32_HANDLER( saturn_cd_r )	 /* CD */

{
  if(offset < 0xA)
    {
      logerror("cd_r %s - data = %04lX - PC=%08lX\n",cd_regnames[offset],cd_regs[offset] >> 16,cpu_get_reg(SH2_PC));
    }
  else
    {
      logerror("cd_r offset=%08lX mem_mask=%08lX - PC=%08lX\n",offset*4,mem_mask,cpu_get_reg(SH2_PC));
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

WRITE32_HANDLER( saturn_cd_w )	 /* CD */
{
  if(offset < 0xA)
    {
      logerror("cd_w %s - data = %04lX - PC=%08lX\n",cd_regnames[offset],data >> 16,cpu_get_reg(SH2_PC));
    }
  else
    {
      logerror("cd_w offset=%08lX data=%08lX mem_mask=%08lX - PC=%08lX\n",offset*4,data,mem_mask,cpu_get_reg(SH2_PC));
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

READ32_HANDLER( saturn_minit_r )  /* MINIT */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_minit_w )  /* MINIT */
{
  logerror("minit_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
}

/********************************************************
 *  FRT slave
 ********************************************************/
READ32_HANDLER( saturn_sinit_r )  /* SINIT */
{
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_sinit_w )  /* SINIT */
{
  logerror("sinit_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset,data,mem_mask);
}

/********************************************************
 *  DSP
 ********************************************************/
READ32_HANDLER( saturn_dsp_r )	 /* DSP */
{
  logerror("dsp_r offset=%08lX mem_mask=%08lX\n",offset*4,mem_mask);

  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_dsp_w )  /* DSP */
{
  logerror("dsp_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);
}

UINT16 vdp1_regs[0xC];

void reset_vdp1(void)

{
  memset(vdp1_regs,0,0xC<<1);
}

READ32_HANDLER( saturn_vdp1_r )   /* VDP1 registers */
{
  UINT32 ret_val;

  ret_val = *(((UINT32 *) vdp1_regs) + offset);
  ret_val = SWAP_WORDS(ret_val) & ~SWAP_WORDS(mem_mask);
  
  logerror("vdp1_r offset=%08lX mem_mask=%08lX ret_val=%08lX\n",offset*4,mem_mask,ret_val);

  return SWAP_WORDS(ret_val);
}

WRITE32_HANDLER( saturn_vdp1_w )  /* VDP1 registers */
{
  UINT32 olddata;

  logerror("vdp1_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);

  olddata = *(((UINT32 *) vdp1_regs) + offset);
  olddata &= SWAP_WORDS(mem_mask);
  olddata |= SWAP_WORDS(data);

  *(((UINT32 *) vdp1_regs) + offset) = olddata;
}

#define SCREEN_LINES 224 /* How many lines are actually displayed */
#define MAX_LINES 256 /* Max lines including blanked - NTSC res - PAL higher*/
#define FRAME_TIME 477273 /* Clock cycles per frame (~60Hz) */
#define LINE_TIME  (FRAME_TIME/MAX_LINES)   /* Approx cycles per line (~FRAME_TIME / 512) */

void *HBlankTimer;
UINT32 HBlankCount;
UINT32 InVBlank;   /* Are we in vertical blanking ? */
void timer_hblank(int param);

void reset_vdp2(void)

{
  HBlankCount = 0;
  HBlankTimer = timer_set(TIME_IN_CYCLES(LINE_TIME,0),0,timer_hblank);
  InVBlank = 0;
}

void timer_hblank(int param)

     /* 
	Called everytime we need a HBlank. Easier to count this and call VBlanks than do vblanks
	as SCU timers require timings based of this.
     */

{
  //  logerror("HBlank Interrupt %ld\n",HBlankCount); /* Logging hblanks is abit of a waste :) */
  HBlankTimer = timer_set(TIME_IN_CYCLES(LINE_TIME,0),0,timer_hblank); /* Reset timer */
  HBlankCount++;
  if((HBlankCount > SCREEN_LINES) && (!InVBlank))
    { 
      /* We are going into vertical blanking area */
      /* Execute VBlank-IN interrupt */
      InVBlank = 1;
      logerror("VBlankIN\n");
    }
  else
    {
      if(HBlankCount >= MAX_LINES)
	{
	  /* We are coming out of Vblank */
	  /* Setup up VBlank-OUT Interrupt */
	  InVBlank = 0;
	  HBlankCount = 0; /* Reset hblank counter */
	  logerror("VBlankOUT\n");
	}
      else
	{
	  /* Issue H-Blank (bit of a hack :P) */
	}
    }
}

READ32_HANDLER( saturn_vdp2_r )   /* VDP2 registers */
{
  logerror("vdp2_r offset=%08lX mem_mask=%08lX\n",offset*4,mem_mask);
  return 0xa5a5a5a5 & ~mem_mask;
}

WRITE32_HANDLER( saturn_vdp2_w )  /* VDP2 registers */
{
  logerror("vdp2_w offset=%08lX data=%08lX mem_mask=%08lX\n",offset*4,data,mem_mask);
}

void saturn_init_machine(void)
{
  int i;
  UINT32 *mem2;
  int mem_length;

  mem = (UINT32 *) memory_region(REGION_CPU1);
  mem2 = (UINT32 *) memory_region(REGION_CPU2);
  
  for (i = 0; i < (SATURN_ROM_SIZE/4); i++)
    {
      mem2[i] = mem[i]; /* Copy bios rom into second cpu area */
    }
  
  mem_length = (memory_region_length(REGION_CPU1) - SATURN_ROM_SIZE) / 4;
  
  for (i = (SATURN_ROM_SIZE/4);i < mem_length; i++)
    {
      mem[i] = 0; /* Clear RAM */
    }
 
  cpu_setbank(1, (UINT8 *) &mem[SATURN_WORKL_RAM_BASE/4]); /* Setup banking (for???) */
  cpu_setbank(2, (UINT8 *) &mem[SATURN_WORKH_RAM_BASE/4]);
  cpu_setbank(3, (UINT8 *) &mem[SATURN_SOUND_RAM_BASE/4]);
  cpu_setbank(4, (UINT8 *) &mem[SATURN_VDP1_RAM_BASE/4]);
  cpu_setbank(5, (UINT8 *) &mem[SATURN_VDP2_RAM_BASE/4]);
  cpu_setbank(6, (UINT8 *) &mem[SATURN_FB1_RAM_BASE/4]);
  cpu_setbank(7, (UINT8 *) &mem[SATURN_FB2_RAM_BASE/4]);
  cpu_setbank(8, (UINT8 *) &mem[SATURN_COLOR_RAM_BASE/4]);
  cpu_setbank(9, (UINT8 *) &mem[SATURN_BACK_RAM_BASE/4]);
 
  /* Install memory handlers. Must be done dynamically to avoid allocating too much ram */

  for (i = 0; i < 2; i++)
    {
      install_mem_read32_handler (i, 0x00000000, 0x0007ffff, MRA32_ROM );
      install_mem_write32_handler(i, 0x00000000, 0x0007ffff, MWA32_ROM );
      
      install_mem_read32_handler (i, 0x00100000, 0x0010007f, saturn_smpc_r );
      install_mem_write32_handler(i, 0x00100000, 0x0010007f, saturn_smpc_w );
      
      install_mem_read32_handler (i, 0x00180000, 0x0018ffff, saturn_back_ram_r );
      install_mem_write32_handler(i, 0x00180000, 0x0018ffff, saturn_back_ram_w );
      
      install_mem_read32_handler (i, 0x00200000, 0x002fffff, saturn_workl_ram_r );
      install_mem_write32_handler(i, 0x00200000, 0x002fffff, saturn_workl_ram_w );
      
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
      
      install_mem_read32_handler (i, 0x06000000, 0x060fffff, saturn_workh_ram_r );
      install_mem_write32_handler(i, 0x06000000, 0x060fffff, saturn_workh_ram_w );
    }
}

void init_saturn(void)
{
  reset_smpc();
  reset_cd();
  reset_scu();
  reset_vdp1();
  reset_vdp2();
}

int saturn_vh_start(void)
{
  logerror("saturn_vh_start\n");
  return 0;
}

void saturn_vh_stop(void)
{
  logerror("saturn_vh_stop\n");
}

void saturn_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
  //  logerror("saturn_vh_screenrefresh\n");
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

void saturn_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
  logerror("saturn_init_palette\n");
}

static struct MachineDriver machine_driver_saturn =
{
  /* basic machine hardware */
  {
    {
      CPU_SH2,
      28636400,   /* NTSC Clock speed at 352/704 Pixel/line dot clock */
      saturn_readmem, saturn_writemem,
      0,0,
      ignore_interrupt, 1
    },
    {
      CPU_SH2,
      28636400, 
      saturn_readmem,saturn_writemem,
      0,0,
      ignore_interrupt, 1
    }
  },
  /* frames per second, VBL duration */
  60, DEFAULT_60HZ_VBLANK_DURATION,
  1,                      /* dual CPU */
  saturn_init_machine,
  NULL,                   /* stop machine */

  /* video hardware */
  SATURN_SCR_WIDTH, SATURN_SCR_HEIGHT,
  { 0, SATURN_SCR_WIDTH - 1, 0, SATURN_SCR_HEIGHT - 1},
  NULL,
  32768, 32768,
  saturn_init_palette,    /* convert color prom */

  VIDEO_TYPE_RASTER,      /* video flags */
  0,                      /* obsolete */
  saturn_vh_start,
  saturn_vh_stop,
  saturn_vh_screenrefresh,

  /* sound hardware */
  0,0,0,0,
};

ROM_START(saturn)
     ROM_REGION(0x00491000, REGION_CPU1,0)
     ROM_LOAD("sega_101.bin", 0x00000000, 0x00080000, 0x224b752c)
     ROM_REGION(0x00080000, REGION_CPU2,0)
ROM_END

static const struct IODevice io_saturn[] = {
  { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
CONSX(1992, saturn, 0, saturn, saturn, saturn, "Sega", "Saturn", GAME_NOT_WORKING | GAME_REQUIRES_16BIT )

