/***************************************************************************

  machine\dgn_beta.c (machine.c)

	Moved out of dragon.c, 2005-05-05, P.Harvey-Smith.
	
	I decided to move this out of the main Dragon/CoCo source files, as 
	the Beta is so radically different from the other Dragon machines that 
	this made more sense (to me at least).

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  References:
	Disassembly of Dragon Beta ROM, examination of only (known) surviving board.
	
  TODO:
	Pretty much everything !
	
	Display working with 6845 taking data from rom.
	
  2005-05-10
	Memory banking seems to be working, as documented in code comments below.

  2005-05-31
	CPU#2 now executes code correctly to do transfers from WD2797.

  2005-06-03
  
	When fed a standard OS-9 boot disk it reads in the boot file and attempts
	to start it, not being able to find init, it fails. Hopefully I will
	soon have an image of a Beta boot disk.
	
***************************************************************************/

#include <math.h>
#include <assert.h>

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/dragon.h"
#include "includes/dgn_beta.h"
#include "includes/6551.h"
#include "image.h"
#include "includes/wd179x.h"
#include "includes/crtc6845.h"

static UINT8 *system_rom;
static int pia2_irq_a;		
static int pia2_irq_b;
static int wd2797_intrq;
static int wd2797_drq;

/* These sets of defines control logging.  When MAME_DEBUG is off, all logging
 * is off.  There is a different set of defines for when MAME_DEBUG is on so I
 * don't have to worry abount accidently committing a version of the driver
 * with logging enabled after doing some debugging work
 *
 * Logging options marked as "Sparse" are logging options that happen rarely,
 * and should not get in the way.  As such, these options are always on
 * (assuming MAME_DEBUG is on).  "Frequent" options are options that happen
 * enough that they might get in the way.
 */

#ifdef MAME_DEBUG
#else /* !MAME_DEBUG */
#endif /* MAME_DEBUG */

#ifdef MAME_DEBUG
static unsigned dgnbeta_dasm_override(int cpunum, char *buffer, unsigned pc);
#endif /* MAME_DEBUG */


static READ8_HANDLER(d_pia0_pa_r);
static WRITE8_HANDLER(d_pia0_pa_w);
static READ8_HANDLER(d_pia0_pb_r);
static WRITE8_HANDLER(d_pia0_pb_w);
static void d_pia0_irq_a(int state);
static void d_pia0_irq_b(int state);
static READ8_HANDLER(d_pia1_pa_r);
static WRITE8_HANDLER(d_pia1_pa_w);
static READ8_HANDLER(d_pia1_pb_r);
static WRITE8_HANDLER(d_pia1_pb_w);
static void d_pia1_irq_a(int state);
static void d_pia1_irq_b(int state);
static READ8_HANDLER(d_pia2_pa_r);
static WRITE8_HANDLER(d_pia2_pa_w);
static READ8_HANDLER(d_pia2_pb_r);
static WRITE8_HANDLER(d_pia2_pb_w);
static void d_pia2_irq_a(int state);
static void d_pia2_irq_b(int state);

static void cpu0_recalc_irq(void);
static void cpu0_recalc_firq(void);
static void cpu0_recalc_nmi(void);

static void cpu1_recalc_irq(void);
static void cpu1_recalc_firq(void);
static void cpu1_recalc_nmi(void);


int dgnbeta_font=0;

static struct pia6821_interface dgnbeta_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ 	d_pia0_pa_r, d_pia1_pa_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ 	d_pia0_pa_w, d_pia0_pb_w, 0, 0,
		/*irqs	 : A/B		   */ 	d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, 0,0,
		/*irqs	 : A/B		   */ d_pia1_irq_a, d_pia1_irq_b
	},

	/* PIA 2 */
	/* This seems to control the RAM paging system, and have the DRQ */
	/* from the WD2797 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia2_pa_r,d_pia2_pb_r, 0, 0, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia2_pa_w,d_pia2_pb_w, 0,0,
		/*irqs	 : A/B	   	   */ d_pia2_irq_a, d_pia2_irq_b
	}
};

// Info for bank switcher
struct bank_info_entry
{
	write8_handler handler;	// Pointer to write handler
	offs_t start;		// Offset of start of block
	offs_t end;		// offset of end of block
};

static const struct bank_info_entry bank_info[] =
{
	{ dgnbeta_ram_b0_w, 0x0000, 0x0fff },
	{ dgnbeta_ram_b1_w, 0x1000, 0x1fff },
	{ dgnbeta_ram_b2_w, 0x2000, 0x2fff },
	{ dgnbeta_ram_b3_w, 0x3000, 0x3fff },
	{ dgnbeta_ram_b4_w, 0x4000, 0x4fff },
	{ dgnbeta_ram_b5_w, 0x5000, 0x5fff },
	{ dgnbeta_ram_b6_w, 0x6000, 0x6fff },
	{ dgnbeta_ram_b7_w, 0x7000, 0x7fff },
	{ dgnbeta_ram_b8_w, 0x8000, 0x8fff },
	{ dgnbeta_ram_b9_w, 0x9000, 0x9fff },
	{ dgnbeta_ram_bA_w, 0xA000, 0xAfff },
	{ dgnbeta_ram_bB_w, 0xB000, 0xBfff },
	{ dgnbeta_ram_bC_w, 0xC000, 0xCfff },
	{ dgnbeta_ram_bD_w, 0xD000, 0xDfff },
	{ dgnbeta_ram_bE_w, 0xE000, 0xEfff },
	{ dgnbeta_ram_bF_w, 0xF000, 0xFBff },
	{ dgnbeta_ram_bG_w, 0xFF00, 0xFfff }
};

struct PageReg 
{
	int	value;			/* Value of the page register */
	UINT8	*memory;		/* The memory it actually points to */
};

static int 	TaskReg;			/* Task register */
static int	EnableMapRegs;			/* Should we use map registers, or just map normal */
static struct PageReg	PageRegs[16][16];	/* 16 sets of 16 page regs, allowing for 16 seperate contexts */

//
// Memory pager, maps machine's 1Mb total address space into the 64K addressable by the 6809.
// This is in some way similar to the system used on the CoCo 3, except the beta uses 4K 
// pages (instead of 8K on the CoCo), and has 16 task registers instead of the 2 on the CoCo.
//
// Each block of 16 page registers (1 for each task), is paged in at $FE00-$FE0F, the bottom 
// 4 bits of the PIA register at $FCC0 seem to contain which task is active.
// bit 6 of the same port seems to enable the memory paging
//
// For the purpose of this driver any block that is not ram, and is not a known ROM block,
// is mapped to the first page of the boot rom, I do not know what happens in the real
// hardware, however this does allow the boot rom to correctly size the RAM. 
// this should probably be considdered a hack !
//

static void UpdateBanks(int first, int last)
{
	int		Page;
	UINT8 		*readbank;
	write8_handler 	writebank;
	int		bank_start;
	int		bank_end;
	int		MapPage;

	for(Page=first;Page<=last;Page++)
	{
		bank_start	= bank_info[Page].start;
		bank_end	= bank_info[Page].end;
		
		if (Page<16)
			MapPage	= PageRegs[TaskReg][Page].value;
		else
			MapPage = PageRegs[TaskReg][15].value;

		//
		// Map block, $00-$BF are ram, $FC-$FF are Boot ROM
		//

		if ((MapPage*4) < (RamSize-1))		// Block is ram
		{
			if (Page<16) 		
				readbank = &mess_ram[MapPage*RamPageSize];
			else
				readbank=&mess_ram[(MapPage*RamPageSize)-256];
			
			writebank=bank_info[Page].handler;
		}
		else					// Block is rom, or undefined
		{
			if (MapPage>0xfB)
			{
				if (Page<16)
					readbank=&system_rom[(MapPage-0xFC)*0x1000];		
				else
					readbank=&system_rom[0x3F00];
			}
			else
				readbank=system_rom;
			
			writebank=MWA8_ROM;
		}

		if(Page==6)
			videoram=readbank;

		PageRegs[TaskReg][Page].memory=readbank;
		memory_set_bankptr(Page+1,readbank);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);
		memory_install_write8_handler(1, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);
	}
}

// Reset ram/rom banks to power on state.
static void ResetBanks(void)
{
	int		Page;
	UINT8 		*readbank;
	write8_handler 	writebank;
	int		bank_start;
	int		bank_end;
	
	for(Page=0;Page<=MaxPage;Page++)
	{
		bank_start	= bank_info[Page].start;
		bank_end	= bank_info[Page].end;

		if (Page<12)
		{
			readbank=&mess_ram[Page*RamPageSize];
			writebank=bank_info[Page].handler;
		}
		else
		{
			if(Page<16)
			{
				readbank=&system_rom[(Page-12)*0x1000];
			}
			else
			{
				readbank=&system_rom[0x3F00];
			}
			
			writebank=MWA8_ROM;
		}
		
		if(Page==6)
			videoram=readbank;
		
		PageRegs[TaskReg][Page].memory=readbank;
		memory_set_bankptr(Page+1,readbank);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);
		memory_install_write8_handler(1, ADDRESS_SPACE_PROGRAM, bank_start, bank_end,0,0,writebank);
	}
}


// Return the value of a page register
READ8_HANDLER( dgn_beta_page_r )
{
	return PageRegs[TaskReg][offset].value;
}

// Write to a page register, writes to the register, and then checks to see
// if memory banking is active, if it is, it calls UpdateBanks, to actually
// setup the mappings.

WRITE8_HANDLER( dgn_beta_page_w )
{	
	PageRegs[TaskReg][offset].value=data;

	if (EnableMapRegs) 
	{		
		UpdateBanks(offset,offset);
		if (offset==15)
			UpdateBanks(offset+1,offset+1);

	}
}

/*********************** Memory bank write handlers ************************/
static void dgn_beta_bank_memory(int offset, int data, int bank)
{
	/* I think writing $C0 to a bank register means do not change ! */
	if (data!=0xC0)
		PageRegs[TaskReg][bank].memory[offset]=data;
}

void dgnbeta_ram_b0_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,0);
}

void dgnbeta_ram_b1_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,1);
}

void dgnbeta_ram_b2_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,2);
}

void dgnbeta_ram_b3_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,3);
}

void dgnbeta_ram_b4_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,4);
}

void dgnbeta_ram_b5_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,5);
}

void dgnbeta_ram_b6_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,6);
}

void dgnbeta_ram_b7_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,7);
}

void dgnbeta_ram_b8_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,8);
}

void dgnbeta_ram_b9_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,9);
}

void dgnbeta_ram_bA_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,10);
}

void dgnbeta_ram_bB_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,11);
}

void dgnbeta_ram_bC_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,12);
}

void dgnbeta_ram_bD_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,13);
}

void dgnbeta_ram_bE_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,14);
}

void dgnbeta_ram_bF_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,15);
}

void dgnbeta_ram_bG_w(offs_t offset, data8_t data)
{
	dgn_beta_bank_memory(offset,data,16);
}


/*********************************** PIA Handlers ************************/
/* PIA #0 at $FC20-$FC23 */
static READ8_HANDLER(d_pia0_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia0_pa_w)
{
}

static READ8_HANDLER(d_pia0_pb_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia0_pb_w)
{
}

static void d_pia0_irq_a(int state)
{
}

static void d_pia0_irq_b(int state)
{
}

/* PIA #1 at $FC24-$FC27 */
static READ8_HANDLER(d_pia1_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia1_pa_w)
{
	int	HALT;

	/* Bit 7 of $FF24, seems to control HALT on second CPU */
	if(data & 0x80)
		HALT=ASSERT_LINE;
	else
		HALT=CLEAR_LINE;

	cpunum_set_input_line(1, INPUT_LINE_HALT, HALT);

	/* second CPU un-halted let it run ! */
	if (HALT==CLEAR_LINE)
		cpu_yield();
}

static READ8_HANDLER(d_pia1_pb_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia1_pb_w)
{
}

static void d_pia1_irq_a(int state)
{
}

static void d_pia1_irq_b(int state)
{
}

/* PIA #2 */
static READ8_HANDLER(d_pia2_pa_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia2_pa_w)
{
	int OldTask;
	int NMI;

	/* Bit 7 of $FFC0, seems to control NMI on second CPU */
	if(data & 0x80)
		NMI=CLEAR_LINE;
	else
		NMI=ASSERT_LINE;
	
	cpunum_set_input_line(1,INPUT_LINE_NMI,NMI);
	
	if(NMI==ASSERT_LINE) 
		cpu_yield();
		
	
	/* Bit 6 seems to enable memory paging */
	if(data & 0x40)
		EnableMapRegs=0;
	else
		EnableMapRegs=1;
	
	/* Bits 0..3 seem to control which task register is selected */
	OldTask=TaskReg;
	TaskReg=data & 0x0F;

	if (TaskReg!=OldTask)		// Update ram banks only if task reg changed.
	{
		if (EnableMapRegs)
			UpdateBanks(0,16);
		else
			ResetBanks();
	}
}

static READ8_HANDLER(d_pia2_pb_r)
{
	return 0;
}

static WRITE8_HANDLER(d_pia2_pb_w)
{
}

static void d_pia2_irq_a(int state)
{
	pia2_irq_a=state;
	cpu0_recalc_irq();
}

static void d_pia2_irq_b(int state)
{
}

/************************************ Recalculate CPU inturrupts ****************************/
/* CPU 0 */
static void cpu0_recalc_irq(void)
{
	if (pia2_irq_a == ASSERT_LINE)
		cpunum_set_input_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		cpunum_set_input_line(0, M6809_IRQ_LINE, CLEAR_LINE);

}

static void cpu0_recalc_firq(void)
{
	if (wd2797_drq == ASSERT_LINE)
		cpunum_set_input_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpunum_set_input_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
	
}

static void cpu0_recalc_nmi(void)
{
}

/* CPU 1 */

static void cpu1_recalc_irq(void)
{
}

static void cpu1_recalc_firq(void)
{
	if (wd2797_drq == ASSERT_LINE)
		cpunum_set_input_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpunum_set_input_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
}

static void cpu1_recalc_nmi(void)
{
}


/********************************************************************************************/
/* Dragon Beta onboard FDC */
/********************************************************************************************/

static void dgnbeta_fdc_callback(int event)
{
	/* The INTRQ line goes through pia3 ca1, in exactly the same way as DRQ from DragonDos does */
	/* DRQ is routed through various logic to the FIRQ inturrupt line on *BOTH* CPUs */
	
	switch(event) 
	{
		case WD179X_IRQ_CLR:
			pia_2_ca1_w(0,CLEAR_LINE);
			break;
		case WD179X_IRQ_SET:
			pia_2_ca1_w(0,ASSERT_LINE);
			break;
		case WD179X_DRQ_CLR:
			wd2797_drq=CLEAR_LINE;
			cpu0_recalc_firq();
			cpu1_recalc_firq();
			break;
		case WD179X_DRQ_SET:
			wd2797_drq=ASSERT_LINE;
			cpu0_recalc_firq();
			cpu1_recalc_firq();
			break;		
	}
}

 READ8_HANDLER(dgnbeta_wd2797_r)
{
	int result = 0;

	switch(offset & 0x03) 
	{
		case 0:
			result = wd179x_status_r(0);
			break;
		case 1:
			result = wd179x_track_r(0);
			break;
		case 2:
			result = wd179x_sector_r(0);
			break;
		case 3:
			result = wd179x_data_r(0);
			break;
		default:
			break;
	}
		
	return result;
}

WRITE8_HANDLER(dgnbeta_wd2797_w)
{
    switch(offset & 0x3) 
	{
		case 0:
			/* disk head is encoded in the command byte */
			wd179x_set_side((data & 0x02) ? 1 : 0);
			wd179x_command_w(0, data);
			break;
		case 1:
			wd179x_track_w(0, data);
			break;
		case 2:
			wd179x_sector_w(0, data);
			break;
		case 3:
			wd179x_data_w(0, data);
			break;
	};
}

/********************************* Machine/Driver Initialization ****************************************/

MACHINE_INIT( dgnbeta )
{
	system_rom = memory_region(REGION_CPU1);

	/* Make sure CPU 1 is started out halted ! */
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	
	/* Reset to task 0, and map banks disabled, so standard memory map */
	/* with ram at $0000-$BFFF, ROM at $C000-FBFF, IO at $FC00-$FEFF */
	/* and ROM at $FF00-$FFFF */
	TaskReg=0;
	EnableMapRegs=0;
	memset(PageRegs,0,sizeof(PageRegs));	/* Reset page registers to 0 */
	ResetBanks();
	
	wd2797_intrq=CLEAR_LINE;
	wd2797_drq=CLEAR_LINE;

	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &dgnbeta_pia_intf[0]);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &dgnbeta_pia_intf[1]);
	pia_config(2, PIA_STANDARD_ORDERING | PIA_8BIT, &dgnbeta_pia_intf[2]); 
	pia_reset();

	pia2_irq_a=CLEAR_LINE;
	pia2_irq_b=CLEAR_LINE;
	
	wd179x_reset();
	wd179x_set_density(DEN_MFM_LO);
	wd179x_set_drive(0);
}

MACHINE_STOP( dgnbeta )
{
}

DRIVER_INIT( dgnbeta )
{
	init_video();
	
	wd179x_init(WD_TYPE_179X,dgnbeta_fdc_callback);
#ifdef MAME_DEBUG
	cpuintrf_set_dasm_override(dgnbeta_dasm_override);
#endif
}

/***************************************************************************
  OS9 Syscalls for disassembly
****************************************************************************/

#ifdef MAME_DEBUG

static const char *os9syscalls[] =
{
	"F$Link",          /* Link to Module */
	"F$Load",          /* Load Module from File */
	"F$UnLink",        /* Unlink Module */
	"F$Fork",          /* Start New Process */
	"F$Wait",          /* Wait for Child Process to Die */
	"F$Chain",         /* Chain Process to New Module */
	"F$Exit",          /* Terminate Process */
	"F$Mem",           /* Set Memory Size */
	"F$Send",          /* Send Signal to Process */
	"F$Icpt",          /* Set Signal Intercept */
	"F$Sleep",         /* Suspend Process */
	"F$SSpd",          /* Suspend Process */
	"F$ID",            /* Return Process ID */
	"F$SPrior",        /* Set Process Priority */
	"F$SSWI",          /* Set Software Interrupt */
	"F$PErr",          /* Print Error */
	"F$PrsNam",        /* Parse Pathlist Name */
	"F$CmpNam",        /* Compare Two Names */
	"F$SchBit",        /* Search Bit Map */
	"F$AllBit",        /* Allocate in Bit Map */
	"F$DelBit",        /* Deallocate in Bit Map */
	"F$Time",          /* Get Current Time */
	"F$STime",         /* Set Current Time */
	"F$CRC",           /* Generate CRC */
	"F$GPrDsc",        /* get Process Descriptor copy */
	"F$GBlkMp",        /* get System Block Map copy */
	"F$GModDr",        /* get Module Directory copy */
	"F$CpyMem",        /* Copy External Memory */
	"F$SUser",         /* Set User ID number */
	"F$UnLoad",        /* Unlink Module by name */
	"F$Alarm",         /* Color Computer Alarm Call (system wide) */
	NULL,
	NULL,
	"F$NMLink",        /* Color Computer NonMapping Link */
	"F$NMLoad",        /* Color Computer NonMapping Load */
	NULL,
	NULL,
	"F$TPS",           /* Return System's Ticks Per Second */
	"F$TimAlm",        /* COCO individual process alarm call */
	"F$VIRQ",          /* Install/Delete Virtual IRQ */
	"F$SRqMem",        /* System Memory Request */
	"F$SRtMem",        /* System Memory Return */
	"F$IRQ",           /* Enter IRQ Polling Table */
	"F$IOQu",          /* Enter I/O Queue */
	"F$AProc",         /* Enter Active Process Queue */
	"F$NProc",         /* Start Next Process */
	"F$VModul",        /* Validate Module */
	"F$Find64",        /* Find Process/Path Descriptor */
	"F$All64",         /* Allocate Process/Path Descriptor */
	"F$Ret64",         /* Return Process/Path Descriptor */
	"F$SSvc",          /* Service Request Table Initialization */
	"F$IODel",         /* Delete I/O Module */
	"F$SLink",         /* System Link */
	"F$Boot",          /* Bootstrap System */
	"F$BtMem",         /* Bootstrap Memory Request */
	"F$GProcP",        /* Get Process ptr */
	"F$Move",          /* Move Data (low bound first) */
	"F$AllRAM",        /* Allocate RAM blocks */
	"F$AllImg",        /* Allocate Image RAM blocks */
	"F$DelImg",        /* Deallocate Image RAM blocks */
	"F$SetImg",        /* Set Process DAT Image */
	"F$FreeLB",        /* Get Free Low Block */
	"F$FreeHB",        /* Get Free High Block */
	"F$AllTsk",        /* Allocate Process Task number */
	"F$DelTsk",        /* Deallocate Process Task number */
	"F$SetTsk",        /* Set Process Task DAT registers */
	"F$ResTsk",        /* Reserve Task number */
	"F$RelTsk",        /* Release Task number */
	"F$DATLog",        /* Convert DAT Block/Offset to Logical */
	"F$DATTmp",        /* Make temporary DAT image (Obsolete) */
	"F$LDAXY",         /* Load A [X,[Y]] */
	"F$LDAXYP",        /* Load A [X+,[Y]] */
	"F$LDDDXY",        /* Load D [D+X,[Y]] */
	"F$LDABX",         /* Load A from 0,X in task B */
	"F$STABX",         /* Store A at 0,X in task B */
	"F$AllPrc",        /* Allocate Process Descriptor */
	"F$DelPrc",        /* Deallocate Process Descriptor */
	"F$ELink",         /* Link using Module Directory Entry */
	"F$FModul",        /* Find Module Directory Entry */
	"F$MapBlk",        /* Map Specific Block */
	"F$ClrBlk",        /* Clear Specific Block */
	"F$DelRAM",        /* Deallocate RAM blocks */
	"F$GCMDir",        /* Pack module directory */
	"F$AlHRam",        /* Allocate HIGH RAM Blocks */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F$RegDmp",        /* Ron Lammardo's debugging register dump call */
	"F$NVRAM",         /* Non Volatile RAM (RTC battery backed static) read/write */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"I$Attach",        /* Attach I/O Device */
	"I$Detach",        /* Detach I/O Device */
	"I$Dup",           /* Duplicate Path */
	"I$Create",        /* Create New File */
	"I$Open",          /* Open Existing File */
	"I$MakDir",        /* Make Directory File */
	"I$ChgDir",        /* Change Default Directory */
	"I$Delete",        /* Delete File */
	"I$Seek",          /* Change Current Position */
	"I$Read",          /* Read Data */
	"I$Write",         /* Write Data */
	"I$ReadLn",        /* Read Line of ASCII Data */
	"I$WritLn",        /* Write Line of ASCII Data */
	"I$GetStt",        /* Get Path Status */
	"I$SetStt",        /* Set Path Status */
	"I$Close",         /* Close Path */
	"I$DeletX"         /* Delete from current exec dir */
};


static unsigned dgnbeta_dasm_override(int cpunum, char *buffer, unsigned pc)
{
	unsigned call;
	unsigned result = 0;

	if ((cpu_readop(pc + 0) == 0x10) && (cpu_readop(pc + 1) == 0x3F))
	{
		call = cpu_readop(pc + 2);
		if ((call >= 0) && (call < sizeof(os9syscalls) / sizeof(os9syscalls[0])) && os9syscalls[call])
		{
			sprintf(buffer, "OS9   %s", os9syscalls[call]);
			result = 3;
		}
	}
	return result;
}



#endif /* MAME_DEBUG */
