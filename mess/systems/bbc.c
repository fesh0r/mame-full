/******************************************************************************
	BBC Model A,B

	BBC Model B Plus is still very much WIP. Very little of the extra
	memory as been added correctly

	MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/bbc.h"
#include "machine/6522via.h"
#include "includes/upd7002.h"
#include "includes/basicdsk.h"



/******************************************************************************
FRED
&FC00
JIM
&FD00
SHEILA
&FE00
&00-&07 6845 CRTC		Video controller				8  ( 2 bytes x	4 )
&08-&0F 6850 ACIA		Serial controller				8  ( 2 bytes x	4 )
&10-&1F Serial ULA		Serial system chip				16 ( 1 byte  x 16 )
&20-&2F Video ULA		Video system chip				16 ( 2 bytes x	8 )
&30-&3F 74LS161 		Paged ROM selector				16 ( 1 byte  x 16 )
&40-&5F 6522 VIA		SYSTEM VIA						32 (16 bytes x	2 )
&60-&7F 6522 VIA		USER VIA						32 (16 bytes x	2 )
&80-&9F 8271 FDC		FDC Floppy disc controller		32 ( 8 bytes x	4 )
&A0-&BF 68B54 ADLC		ECONET controller				32 ( 4 bytes x	8 )
&C0-&DF uPD7002 		Analogue to digital converter	32 ( 4 bytes x	8 )
&E0-&FF Tube ULA		Tube system interface			32
******************************************************************************/

/* for the model A just address the 4 on board ROM sockets */
static WRITE_HANDLER ( page_selecta_w )
{
	cpu_setbank(3,memory_region(REGION_CPU2)+((data&0x03)<<14));
}

/* for the model B address all 16 of the ROM sockets */
static WRITE_HANDLER ( page_selectb_w )
{
	cpu_setbank(3,memory_region(REGION_CPU2)+((data&0x0f)<<14));
}


static WRITE_HANDLER ( memory_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}


/* functions to return reads to empty hardware addresses */

static READ_HANDLER ( BBC_NOP_00_r )
{
	return 0x00;
}

static READ_HANDLER ( BBC_NOP_FE_r )
{
	return 0xFE;
}

static READ_HANDLER ( BBC_NOP_FF_r )
{
	return 0xFF;
}



/****************************************/
/* BBC B Plus memory handling function */
/****************************************/

static int pagedRAM=0;
static int vdusel=0;
static int rombankselect=0;
/* the model B plus addresses all 16 of the ROM sockets plus the extra 12K of ram at 0x8000
   and 20K of shadow ram at 0x3000 */
static WRITE_HANDLER ( page_selectbp_w )
{
	if ((offset&0x04)==0)
	{
		logerror("write to rom pal $%04X  $%02X\n",offset,data);
		pagedRAM=(data&0x80)>>7;
		rombankselect=data&0x0f;

		if (pagedRAM)
		{
			/* if paged ram then set 8000 to afff to read from the ram 8000 to afff */
			cpu_setbank(3,memory_region(REGION_CPU1)+0x8000);
		} else {
			/* if paged rom then set the rom to be read from 8000 to afff */
			cpu_setbank(3,memory_region(REGION_CPU2)+(rombankselect<<14));
		};

		/* set the rom to be read from b000 to bfff */
		cpu_setbank(4,memory_region(REGION_CPU2)+(rombankselect<<14)+0x03000);
	}
	else
	{
		//the video display should now use this flag to display the shadow ram memory
		vdusel=(data&0x80)>>7;
		bbcbp_setvideoshadow(vdusel);
		//need to make the video display do a full screen refresh for the new memory area
	}
}


/* write to the normal memory from 0x0000 to 0x2fff
   the writes to this memory are just done the normal
   way */

static WRITE_HANDLER ( memorybp0_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}


/*  this function should return true if
	the instruction is in the VDU driver address ranged
	these are set when:
	PC is in the range c000 to dfff
	or if pagedRAM set and PC is in the range a000 to afff
*/
int vdudriverset(void)
{
	int PC;
	PC=m6502_get_pc(); // this needs to be set to the 6502 program counter
	return (((PC>=0xc000) && (PC<=0xdfff)) || ((pagedRAM) && ((PC>=0xa000) && (PC<=0xafff))));
}

/* the next two function handle reads and write to the shadow video ram area
   between 0x3000 and 0x7fff

   when vdusel is set high the video display uses the shadow ram memory
   the processor only reads and write to the shadow ram when vdusel is set
   and when the instruction being executed is stored in a set range of memory
   addresses known as the VDU driver instructions.
*/


READ_HANDLER ( memorybp1_r )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just read normal ram
		return memory_region(REGION_CPU1)[offset+0x3000];
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then read from shadow ram
			return memory_region(REGION_CPU1)[offset+0xb000];
		} else {
			// else read from normal ram
			return memory_region(REGION_CPU1)[offset+0x3000];
		}
	}
}

static WRITE_HANDLER ( memorybp1_w )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just write to normal ram
		memory_region(REGION_CPU1)[offset+0x3000]=data;
		vidmem[offset+0x3000]=1;
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then write to shadow ram
			memory_region(REGION_CPU1)[offset+0xb000]=data;
			vidmem[offset+0xb000]=1;
		} else {
			// else write to normal ram
			memory_region(REGION_CPU1)[offset+0x3000]=data;
			vidmem[offset+0x3000]=1;
		}
	}
}


/* if the pagedRAM is set write to RAM between 0x8000 to 0xafff
otherwise this area contains ROM so no write is required */
static WRITE_HANDLER ( memorybp3_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
}


/* the BBC B plus 128K had extra ram mapped in replacing the
rom bank 0,1,c and d.
The function memorybp3_128_w handles memory writes from 0x8000 to 0xafff
which could either be sideways ROM, paged RAM, or sideways RAM.
The function memorybp4_128_w handles memory writes from 0xb000 to 0xbfff
which could either be sideways ROM or sideways RAM */


unsigned short bbc_b_plus_sideways_ram_banks[16]=
{
	1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0
};


static WRITE_HANDLER ( memorybp3_128_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
	else
	{
		if (bbc_b_plus_sideways_ram_banks[rombankselect])
		{
			memory_region(REGION_CPU2)[offset+(rombankselect<<14)]=data;
		}
	}
}

static WRITE_HANDLER ( memorybp4_128_w )
{
	if (bbc_b_plus_sideways_ram_banks[rombankselect])
	{
		memory_region(REGION_CPU2)[offset+(rombankselect<<14)+0x3000]=data;
	}
}


/* I need to check fe18 to fe1f I think this is mapped to the econet ID number DIPs */


static MEMORY_READ_START(readmem_bbca)
	{ 0x0000, 0x3fff, MRA_RAM		   },
	{ 0x4000, 0x7fff, MRA_BANK1 	   },  /* bank 1 is a repeat of the memory at 0x0000 to 0x3fff	 */
	{ 0x8000, 0xbfff, MRA_BANK3 	   },  /* Paged ROM                                              */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                     */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                       */
										   /* Shiela Address Page &fe00 - &feff 					 */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller				 */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller				 */
	{ 0xfe10, 0xfe1f, BBC_NOP_00_r	   },  /* &10-&1f  Serial ULA	 Serial system chip 			 */
	{ 0xfe20, 0xfe2f, videoULA_r	   },  /* &20-&2f  Video ULA	 Video system chip				 */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  84LS161		 Paged ROM selector 			 */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 					 */
	{ 0xfe60, 0xfe7f, BBC_NOP_00_r	   },  /* &60-&7f  6522 VIA 	 1 USER VIA 					 */
	{ 0xfe80, 0xfe9f, BBC_NOP_00_r	   },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller		 */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 1 ECONET controller			 */
	{ 0xfec0, 0xfedf, BBC_NOP_00_r	   },  /* &c0-&df  uPD7002		 1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 1 Tube system interface		 */
	{ 0xff00, 0xffff, MRA_BANK2		   },  /* Hardware marked with a 1 is not present in a Model A	 */
MEMORY_END

static MEMORY_WRITE_START(writemem_bbca)
	{ 0x0000, 0x3fff, memory_w		   },
	{ 0x4000, 0x7fff, memory_w		   },  /* this is a repeat of the memory at 0x0000 to 0x3fff	 */
	{ 0x8000, 0xdfff, MWA_ROM		   },  /* Paged ROM                                              */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                     */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                       */
										   /* Shiela Address Page &fe00 - &feff 					 */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller				 */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller				 */
	{ 0xfe10, 0xfe1f, MWA_NOP		   },  /* &10-&1f  Serial ULA	 Serial system chip 			 */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip				 */
	{ 0xfe30, 0xfe3f, page_selecta_w   },  /* &30-&3f  84LS161		 Paged ROM selector 			 */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 					 */
	{ 0xfe60, 0xfe7f, MWA_NOP		   },  /* &60-&7f  6522 VIA 	 1 USER VIA 					 */
	{ 0xfe80, 0xfe9f, MWA_NOP		   },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller		 */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 1 ECONET controller			 */
	{ 0xfec0, 0xfedf, MWA_NOP		   },  /* &c0-&df  uPD7002		 1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 1 Tube system interface		 */
	{ 0xff00, 0xffff, MWA_ROM		   },  /* Hardware marked with a 1 is not present in a Model A	 */
MEMORY_END


static MEMORY_READ_START(readmem_bbcb)
	{ 0x0000, 0x7fff, MRA_RAM		   },
	{ 0x8000, 0xbfff, MRA_BANK3 	   },
	{ 0xc000, 0xfbff, MRA_BANK2		   },
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, BBC_NOP_00_r	   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_r	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_i8271_read   },  /* &80-&9f  8271 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r	       },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcb)
	{ 0x0000, 0x7fff, memory_w		   },
	{ 0x8000, 0xdfff, MWA_ROM		   },
	{ 0xc000, 0xfdff, MWA_ROM		   },
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, MWA_NOP		   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_i8271_write  },  /* &80-&9f  8271 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w        },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


static MEMORY_READ_START(readmem_bbcb1770)
	{ 0x0000, 0x7fff, MRA_RAM		   },
	{ 0x8000, 0xbfff, MRA_BANK3 	   },
	{ 0xc000, 0xfbff, MRA_BANK2		   },
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, BBC_NOP_00_r	   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_r	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r  	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcb1770)
	{ 0x0000, 0x7fff, memory_w		   },
	{ 0x8000, 0xdfff, MWA_ROM		   },
	{ 0xc000, 0xfdff, MWA_ROM		   },
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, MWA_NOP		   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


static MEMORY_READ_START(readmem_bbcbp)
	{ 0x0000, 0x2fff, MRA_RAM		   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, MRA_BANK1        },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, MRA_BANK3 	   },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xbfff, MRA_BANK4        },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, BBC_NOP_00_r	   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, BBC_NOP_FE_r	   },  /* &20-&2f  INTON		 Does something with econet	   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC			 NC					 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r 	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcbp)
	{ 0x0000, 0x2fff, memorybp0_w	   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, memorybp1_w      },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, memorybp3_w      },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xdfff, MWA_ROM          },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, MWA_NOP		   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectbp_w  },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END



static MEMORY_READ_START(readmem_bbcbp128)
	{ 0x0000, 0x2fff, MRA_RAM		   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, MRA_BANK1        },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, MRA_BANK3 	   },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xbfff, MRA_BANK4        },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfbff, MRA_BANK2		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, BBC_NOP_FF_r	   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_r	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, BBC_NOP_00_r	   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, BBC_NOP_00_r	   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, BBC_NOP_FE_r	   },  /* &20-&2f  INTON		 Does something with econet	   */
	{ 0xfe30, 0xfe3f, BBC_NOP_FE_r	   },  /* &30-&3f  NC			 NC					 		   */
	{ 0xfe40, 0xfe5f, via_0_r		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_r		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, BBC_NOP_FE_r	   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_r 	   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, BBC_NOP_FE_r	   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MRA_BANK2		   },
MEMORY_END

static MEMORY_WRITE_START(writemem_bbcbp128)
	{ 0x0000, 0x2fff, memorybp0_w	   },  /* Normal Ram                                           */
	{ 0x3000, 0x7fff, memorybp1_w      },  /* Video/Shadow Ram                                     */
	{ 0x8000, 0xafff, memorybp3_128_w  },  /* Paged ROM or 12K of RAM                              */
	{ 0xb000, 0xdfff, memorybp4_128_w  },  /* Rest of paged ROM area                               */
	{ 0xc000, 0xfdff, MWA_ROM		   },  /* OS                                                   */
	{ 0xfc00, 0xfdff, MWA_NOP		   },  /* FRED & JIM Pages                                     */
										   /* Shiela Address Page &fe00 - &feff 				   */
	{ 0xfe00, 0xfe07, BBC_6845_w	   },  /* &00-&07  6845 CRTC	 Video controller			   */
	{ 0xfe08, 0xfe0f, MWA_NOP		   },  /* &08-&0f  6850 ACIA	 Serial Controller			   */
	{ 0xfe10, 0xfe1f, MWA_NOP		   },  /* &10-&1f  Serial ULA	 Serial system chip 		   */
	{ 0xfe20, 0xfe2f, videoULA_w	   },  /* &20-&2f  Video ULA	 Video system chip			   */
	{ 0xfe30, 0xfe3f, page_selectbp_w  },  /* &30-&3f  84LS161		 Paged ROM selector 		   */
	{ 0xfe40, 0xfe5f, via_0_w		   },  /* &40-&5f  6522 VIA 	 SYSTEM VIA 				   */
	{ 0xfe60, 0xfe7f, via_1_w		   },  /* &60-&7f  6522 VIA 	 USER VIA					   */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  1770 FDC      Floppy disc controller 	   */
	{ 0xfea0, 0xfebf, MWA_NOP		   },  /* &a0-&bf  68B54 ADLC	 ECONET controller			   */
	{ 0xfec0, 0xfedf, uPD7002_w		   },  /* &c0-&df  uPD7002		 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP		   },  /* &e0-&ff  Tube ULA 	 Tube system interface		   */
	{ 0xff00, 0xffff, MWA_ROM		   },
MEMORY_END


unsigned short bbc_colour_table[8]=
{
	0,1,2,3,4,5,6,7
};

unsigned char	bbc_palette[8*3]=
{

	0x0ff,0x0ff,0x0ff,
	0x000,0x0ff,0x0ff,
	0x0ff,0x000,0x0ff,
	0x000,0x000,0x0ff,
	0x0ff,0x0ff,0x000,
	0x000,0x0ff,0x000,
	0x0ff,0x000,0x000,
	0x000,0x000,0x000
};

static void init_palette_bbc(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,bbc_palette,sizeof(bbc_palette));
	memcpy(sys_colortable,bbc_colour_table,sizeof(bbc_colour_table));
}

INPUT_PORTS_START(bbca)

	/* KEYBOARD COLUMN 0 */
	PORT_START
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",      KEYCODE_RSHIFT,   IP_JOY_NONE)
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",      KEYCODE_LSHIFT,   IP_JOY_NONE)
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",          KEYCODE_Q,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F0",         KEYCODE_0_PAD,    IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "1",          KEYCODE_1,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK",  KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT LOCK", KEYCODE_LALT,     IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB",        KEYCODE_TAB,      IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "ESCAPE",     KEYCODE_ESC,      IP_JOY_NONE)

	/* KEYBOARD COLUMN 1 */
	PORT_START
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",       KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "3",          KEYCODE_3,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "W",          KEYCODE_W,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "2",          KEYCODE_2,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "A",          KEYCODE_A,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "S",          KEYCODE_S,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",          KEYCODE_Z,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",         KEYCODE_1_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 2 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 8 (Not Used)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "4",          KEYCODE_4,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "E",          KEYCODE_E,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "D",          KEYCODE_D,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "X",          KEYCODE_X,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "C",          KEYCODE_C,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",      KEYCODE_SPACE,    IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",         KEYCODE_2_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 3 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 7 (Not Used)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "5",          KEYCODE_5,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "T",          KEYCODE_T,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "R",          KEYCODE_R,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F",          KEYCODE_F,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "G",          KEYCODE_G,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "V",          KEYCODE_V,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",         KEYCODE_3_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 4 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 6 (Disc Speed 1)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",         KEYCODE_4_PAD,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "7",          KEYCODE_7,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "6",          KEYCODE_6,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",          KEYCODE_Y,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "H",          KEYCODE_H,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "B",          KEYCODE_B,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",         KEYCODE_5_PAD,    IP_JOY_NONE)

	/* KEYBOARD COLUMN 5 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 5 (Disc Speed 0)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "8",          KEYCODE_8,        IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "I",          KEYCODE_I,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "U",          KEYCODE_U,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "J",          KEYCODE_J,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "N",          KEYCODE_N,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "M",          KEYCODE_M,        IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F6",         KEYCODE_6_PAD,    IP_JOY_NONE)

/* KEYBOARD COLUMN 6 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 4 (Shift Break)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F7",         KEYCODE_7_PAD,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "9",          KEYCODE_9,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "O",          KEYCODE_O,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "K",          KEYCODE_K,        IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "L",          KEYCODE_L,        IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, ",",          KEYCODE_COMMA,    IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F8",         KEYCODE_3_PAD,    IP_JOY_NONE)

/* KEYBOARD COLUMN 7 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 3 (Mode bit 2)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "-",          KEYCODE_MINUS,    IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "0",          KEYCODE_0,        IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "P",          KEYCODE_P,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "@",          KEYCODE_BACKSLASH,IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, ";",          KEYCODE_COLON,    IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, ".",          KEYCODE_STOP,     IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "F9",         KEYCODE_9_PAD,    IP_JOY_NONE)


	/* KEYBOARD COLUMN 8 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 2 (Mode bit 1)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "^",          KEYCODE_EQUALS,     IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "_",          KEYCODE_TILDE,      IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "[",          KEYCODE_OPENBRACE,  IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, ":",          KEYCODE_QUOTE,      IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "]",          KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "/",          KEYCODE_SLASH,      IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "\\",         KEYCODE_BACKSLASH2, IP_JOY_NONE)

	/* KEYBOARD COLUMN 9 */
	PORT_START
	PORT_BITX(0x01,0x01,IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DIP 1 (Mode bit 0)", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x00,DEF_STR( Off ))
	PORT_DIPSETTING(0x01,DEF_STR( On ))
	PORT_BITX(0x02,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR LEFT", KEYCODE_LEFT,      IP_JOY_NONE)
	PORT_BITX(0x04,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR DOWN", KEYCODE_DOWN,      IP_JOY_NONE)
	PORT_BITX(0x08,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR UP",   KEYCODE_UP,        IP_JOY_NONE)
	PORT_BITX(0x10,  IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN",      KEYCODE_ENTER,     IP_JOY_NONE)
	PORT_BITX(0x20,  IP_ACTIVE_LOW, IPT_KEYBOARD, "DELETE",      KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x40,  IP_ACTIVE_LOW, IPT_KEYBOARD, "COPY",        KEYCODE_END,       IP_JOY_NONE)
	PORT_BITX(0x80,  IP_ACTIVE_LOW, IPT_KEYBOARD, "CURSOR RIGHT",KEYCODE_RIGHT,     IP_JOY_NONE)


	PORT_START  // KEYBOARD COLUMN 10 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 11 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 12 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 13 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 14 RESERVED FOR BBC MASTER
	PORT_START  // KEYBOARD COLUMN 15 RESERVED FOR BBC MASTER


	PORT_START
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff )

INPUT_PORTS_END

/* the BBC came with 4 rom sockets on the mother board as shown in the model A driver */
/* you could get a number of rom upgrade boards that took this up to 16 roms as in the */
/* model B driver */

ROM_START(bbca)
	ROM_REGION(0x04000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x14000,REGION_CPU2,0) /* ROM */
	ROM_LOAD("os12.rom",    0x10000,  0x4000, 0x3c14fc70)

						  /* rom page 0  00000 */
														  /* rom page 1  04000 */
														  /* rom page 2  08000 */
	ROM_LOAD("basic2.rom",  0x0c000, 0x4000, 0x79434781 ) /* rom page 3  0c000 */
ROM_END


/*	0000- 7fff	ram */
/*	8000- bfff	not used, this area is mapped over with one of the roms at 10000 and above */
/*	c000- ffff	OS rom and memory mapped hardware at fc00-feff */
/* 10000-4ffff	16 paged rom banks mapped back into 8000-bfff by the page rom select */

/* I have only plugged a few roms in */
ROM_START(bbcb)
	ROM_REGION(0x08000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x44000,REGION_CPU2,0) /* ROM */
	ROM_LOAD("os12.rom", 0x40000,0x4000, 0x3c14fc70)

#ifdef MAME_DEBUG
	ROM_LOAD("advromm.rom", 0x00000, 0x4000, 0xe7e2a294 ) /* rom page 0  00000 */
	ROM_LOAD("exmon.rom",   0x04000, 0x4000, 0x9a50231f ) /* rom page 1  04000 */
	ROM_LOAD("help.rom",    0x08000, 0x4000, 0xc1505821 ) /* rom page 2  08000 */
	ROM_LOAD("toolkit.rom", 0x0c000, 0x4000, 0x557ce483 ) /* rom page 3  0c000 */
	ROM_LOAD("view.rom",    0x10000, 0x4000, 0x4345359f ) /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */
#endif

	/* just use one of the following DFS roms */

	/* dnfs is acorns disc and network filing system rom it replaced dfs 0.9  */
//	ROM_LOAD("dnfs.rom",    0x38000, 0x4000, 0x8ccd2157 ) /* rom page 14 38000 */

	/* dfs 0.9 was the standard acorn dfs rom before it was replaced with the dnfs rom */
//	ROM_LOAD("dfs09.rom",   0x38000, 0x2000, 0x3ce609cf ) /* rom page 14 38000 */
//	ROM_RELOAD(             0x3a000, 0x2000             )

	/* dfs 1.44 from watford electronics, this is the best of the non-acorn dfs roms */
	ROM_LOAD("dfs144.rom",  0x38000, 0x4000, 0x9fb8d13f ) /* rom page 14 38000 */

	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 15 3c000 */


ROM_END



ROM_START(bbcb1770)
	ROM_REGION(0x08000,REGION_CPU1,0) /* RAM */

	ROM_REGION(0x44000,REGION_CPU2,0) /* ROM */
	ROM_LOAD("os12.rom", 0x40000,0x4000, 0x3c14fc70)

#ifdef MAME_DEBUG
	ROM_LOAD("advromm.rom", 0x00000, 0x4000, 0xe7e2a294 ) /* rom page 0  00000 */
	ROM_LOAD("exmon.rom",   0x04000, 0x4000, 0x9a50231f ) /* rom page 1  04000 */
	ROM_LOAD("help.rom",    0x08000, 0x4000, 0xc1505821 ) /* rom page 2  08000 */
	ROM_LOAD("toolkit.rom", 0x0c000, 0x4000, 0x557ce483 ) /* rom page 3  0c000 */
	ROM_LOAD("view.rom",    0x10000, 0x4000, 0x4345359f ) /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */
#endif
/* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 15 3c000 */

ROM_END




ROM_START(bbcbp)
	ROM_REGION(0x10000,REGION_CPU1,0) /* ROM MEMORY */

	ROM_REGION(0x44000,REGION_CPU2,0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, 0x9f356396 )  /* basic rom */
	ROM_CONTINUE(           0x40000, 0x4000)  /* OS */

#ifdef MAME_DEBUG
	ROM_LOAD("advromm.rom", 0x00000, 0x4000, 0xe7e2a294 ) /* rom page 0  00000 */
	ROM_LOAD("exmon.rom",   0x04000, 0x4000, 0x9a50231f ) /* rom page 1  04000 */
	ROM_LOAD("help.rom",    0x08000, 0x4000, 0xc1505821 ) /* rom page 2  08000 */
	ROM_LOAD("toolkit.rom", 0x0c000, 0x4000, 0x557ce483 ) /* rom page 3  0c000 */
	ROM_LOAD("view.rom",    0x10000, 0x4000, 0x4345359f ) /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */
#endif

    /* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

ROM_END


ROM_START(bbcbp128)
	ROM_REGION(0x10000,REGION_CPU1,0) /* ROM MEMORY */

	ROM_REGION(0x44000,REGION_CPU2,0) /* ROM */
	ROM_LOAD("bpos2.rom",   0x3c000, 0x4000, 0x9f356396 )  /* basic rom */
	ROM_CONTINUE(           0x40000, 0x4000)  /* OS */

#ifdef MAME_DEBUG
	ROM_LOAD("advromm.rom", 0x00000, 0x4000, 0xe7e2a294 ) /* rom page 0  00000 */
	ROM_LOAD("exmon.rom",   0x04000, 0x4000, 0x9a50231f ) /* rom page 1  04000 */
	ROM_LOAD("help.rom",    0x08000, 0x4000, 0xc1505821 ) /* rom page 2  08000 */
	ROM_LOAD("toolkit.rom", 0x0c000, 0x4000, 0x557ce483 ) /* rom page 3  0c000 */
	ROM_LOAD("view.rom",    0x10000, 0x4000, 0x4345359f ) /* rom page 4  10000 */
	                                                      /* rom page 5  14000 */
			  											  /* rom page 6  18000 */
	                                                      /* rom page 7  1c000 */
														  /* rom page 8  20000 */
														  /* rom page 9  24000 */
														  /* rom page 10 28000 */
														  /* rom page 11 2c000 */
														  /* rom page 12 30000 */
														  /* rom page 13 34000 */
#endif

    /* ddfs 2.23 this is acorns 1770 disc controller Double density disc filing system */
    ROM_LOAD("ddfs223.rom", 0x38000, 0x4000, 0x7891f9b7 ) /* rom page 14 38000 */

ROM_END


static int bbcb_vsync(void)
{
	via_0_ca1_w(0,0);
	via_0_ca1_w(0,1);
    //check_disc_status();
	return 0;
}


static struct SN76496interface sn76496_interface =
{
	1,		/* 1 chip */
	{ 4000000 },	/* 4Mhz */
	{ 100 }
};

static struct MachineDriver machine_driver_bbca =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbca,
			writemem_bbca,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbca, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_starta,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcb =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcb,
			writemem_bbcb,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbcb, /* init_machine */
	stop_machine_bbcb, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startb,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcb1770 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcb1770,
			writemem_bbcb1770,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbcb1770, /* init_machine */
	stop_machine_bbcb1770, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startb,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcbp =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcbp,
			writemem_bbcbp,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbcbp, /* init_machine */
	stop_machine_bbcbp, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startbp,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static struct MachineDriver machine_driver_bbcbp128 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,		/* 2.00Mhz */
			readmem_bbcbp128,
			writemem_bbcbp128,
			0,
			0,

			bbcb_vsync, 	1,				/* screen refresh interrupts */
			bbcb_keyscan, 1000				/* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbcbp, /* init_machine */
	stop_machine_bbcbp, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_startbp,
	bbc_vh_stop,
	bbc_vh_screenrefresh,
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static const struct IODevice io_bbca[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

static const struct IODevice io_bbcb[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		basicdsk_floppy_id, 	/* id */
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};


static const struct IODevice io_bbcb1770[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		basicdsk_floppy_id, 	/* id */
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};


static const struct IODevice io_bbcbp[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		basicdsk_floppy_id, 	/* id */
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

static const struct IODevice io_bbcbp128[] = {
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"wav\0",                /* File extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		NULL,					/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},	{
		IO_FLOPPY,				/* type */
		2,						/* count */
        "ssd\0bbc\0img\0",      /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		basicdsk_floppy_id, 	/* id */
		bbc_floppy_init,		/* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

/*	   year name	parent	machine  input	init	company */
COMP (1981,bbca,	0,		bbca,	 bbca,	0,	"Acorn","BBC Micro Model A" )
COMP (1981,bbcb,	bbca,	bbcb,	 bbca,	0,	"Acorn","BBC Micro Model B" )
COMP (1981,bbcb1770,bbca,	bbcb1770,bbca,	0,	"Acorn","BBC Micro Model B with WD1770 disc controller" )
COMP (1985,bbcbp,   bbca,	bbcbp   ,bbca,  0,  "Acorn","BBC Micro Model B+ 64K" )
COMP (1985,bbcbp128,bbca,   bbcbp128,bbca,  0,  "Acorn","BBC Micro Model B+ 128k" )
/*
COMP (198?,bbcm,    0,      bbcm    ,bbcm,  0,  "Acorn","BBC Master" )
*/

