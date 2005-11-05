/***************************************************************************

	systems\dgn_beta.c

	Dragon Beta prototype, based on two 68B09E processors, WD2797, 6845.

Project Beta was the second machine that Dragon Data had in development at 
the time they ceased trading, the first being Project Alpha (also know as the 
Dragon Professional). 

The machine uses dual 68B09 cpus which both sit on the same bus and access 
the same memory and IO chips ! The first is the main processor, used to run 
user code, the second is uses as a DMA controler, to amongst other things 
disk data transfers. The first processor controled the second by having the 
halt and nmi lines from the second cpu connected to PIA output lines so 
that the could be changed under OS control. The first CPU just passed 
instructions for the block to be transfered in 5 low ram addresses and 
generated an NMI on the second CPU.

Project Beta like the other Dragons used a WD2797 floppy disk controler
which is memory mapped, and controled by the second CPU.

Unlike the other Dragon machines, project Beta used a 68b45 to generate video, 
and totally did away with the SAM. 

The machine has a 6551 ACIA chip, but I have not yet found where this is 
memory mapped.

Project Beta, had a custom MMU bilt from a combination of LSTTL logic, and 
PAL programable logic. This MMU could address 256 blocks of 4K, giving a 
total addressable range of 1 megabyte, of this the first 768KB could be RAM,
however tha machine by default, came with 256K or ram, and a 16K boot rom,
which contained an OS-9 Level 2 bootstrap.

A lot of the infomrmation required to start work on this driver has been
infered from disassembly of the boot rom, and from what little hardware 
documentation still exists. 

***************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/crtc6845.h"
#include "includes/dragon.h"
#include "includes/dgn_beta.h"
#include "devices/basicdsk.h"
#include "includes/6551.h"
#include "formats/coco_dsk.h"
#include "devices/mflopimg.h"
#include "devices/coco_vhd.h"


/* This is just a guess, until we resurect the real machine */
unsigned char dgnbeta_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static unsigned short dgnbeta_colortable[][2] =
{
	{ 0, 1 },
	/* reverse */
	{ 1, 0 }
};

static gfx_layout dgnbeta_charlayout =
{
	8,9,
	256,                    /* 512 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,8,9 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8,},
	16*8			/* Each char is 16 bytes apart */
};

static gfx_decode dgnbeta_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &dgnbeta_charlayout,                     0, 1 },
	{ -1 } /* end of array */
};

/*
	2005-05-10
	
	I *THINK* I know how the memory paging works, the 64K memory map is devided
	into 16x 4K pages, what is mapped into each page is controled by the IO at
	FE00-FE0F like so :-

	Location	Memory page 	Initialised to 
	$FE00		$0000-$0FFF		$00
	$FE01		$1000-$1FFF		$00	(used as ram test page, when sizing memory)
	$FE02		$2000-$2FFF		$00
	$FE03		$3000-$3FFF		$00
	$FE04		$4000-$4FFF		$00
	$FE05		$5000-$5FFF		$00
	$FE06		$6000-$6FFF		$1F	($1F000)
	$FE07		$7000-$7FFF		$00		
	$FE08		$8000-$8FFF		$00
	$FE09		$9000-$9FFF		$00
	$FE0A		$A000-$AFFF		$00
	$FE0B		$B000-$BFFF		$00
	$FE0C		$C000-$CFFF		$00
	$FE0D		$D000-$DFFF		$00
	$FE0E		$E000-$EFFF		$FE
	$FE0F		$F000-$FFFF		$FF

	The value stored at each location maps it's page to a 4K page within a 1M address 
	space. Acording to the Beta product descriptions released by Dragon Data, the 
	machine could have up to 768K of RAM, if this where true then pages $00-$BF could 
	potentially be RAM, and pages $C0-$FF would be ROM. The initialisation code maps in
	the memory as described above.

	At reset time the Paging would of course be disabled, as the boot rom needs to be 
	mapped in at $C000, the initalisation code would set up the mappings above and then
	enable the paging hardware.

	It appears to be more complicated than this, whilst the above is true, there appear to 
	be 16 sets of banking registers, the active set is controled by the bottom 4 bits of
	FCC0, bit 6 has something to do with enabling and disabling banking.

*/

static ADDRESS_MAP_START( dgnbeta_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0FFF) 	AM_READWRITE(MRA8_BANK1		,MWA8_BANK1)
	AM_RANGE(0x1000, 0x1FFF) 	AM_READWRITE(MRA8_BANK2		,MWA8_BANK2)
	AM_RANGE(0x2000, 0x2FFF) 	AM_READWRITE(MRA8_BANK3		,MWA8_BANK3)
	AM_RANGE(0x3000, 0x3FFF) 	AM_READWRITE(MRA8_BANK4		,MWA8_BANK4)
	AM_RANGE(0x4000, 0x4FFF) 	AM_READWRITE(MRA8_BANK5		,MWA8_BANK5)
	AM_RANGE(0x5000, 0x5FFF) 	AM_READWRITE(MRA8_BANK6		,MWA8_BANK6)
	AM_RANGE(0x6000, 0x6FFF) 	AM_READWRITE(MRA8_BANK7		,MWA8_BANK7) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x7000, 0x7FFF) 	AM_READWRITE(MRA8_BANK8		,MWA8_BANK8)
	AM_RANGE(0x8000, 0x8FFF) 	AM_READWRITE(MRA8_BANK9		,MWA8_BANK9)
	AM_RANGE(0x9000, 0x9FFF) 	AM_READWRITE(MRA8_BANK10	,MWA8_BANK10)
	AM_RANGE(0xA000, 0xAFFF) 	AM_READWRITE(MRA8_BANK11	,MWA8_BANK11)
	AM_RANGE(0xB000, 0xBFFF) 	AM_READWRITE(MRA8_BANK12	,MWA8_BANK12)
	AM_RANGE(0xC000, 0xCFFF) 	AM_READWRITE(MRA8_BANK13	,MWA8_BANK13)
	AM_RANGE(0xD000, 0xDFFF) 	AM_READWRITE(MRA8_BANK14	,MWA8_BANK14)
	AM_RANGE(0xE000, 0xEFFF) 	AM_READWRITE(MRA8_BANK15	,MWA8_BANK15)
	AM_RANGE(0xF000, 0xFBFF) 	AM_READWRITE(MRA8_BANK16	,MWA8_BANK16)
	AM_RANGE(0xFC20, 0xFC23)	AM_READWRITE(pia_0_r		,pia_0_w)
	AM_RANGE(0xFC24, 0xFC27)	AM_READWRITE(pia_1_r		,pia_1_w)
	AM_RANGE(0xfc80, 0xfc81)	AM_READWRITE(crtc6845_0_port_r	,crtc6845_0_port_w)	
	AM_RANGE(0xfce0, 0xfce3)	AM_READWRITE(dgnbeta_wd2797_r	,dgnbeta_wd2797_w)	/* Onboard disk interface */
	AM_RANGE(0xFCC0, 0xFCC3)	AM_READWRITE(pia_2_r		,pia_2_w)
	AM_RANGE(0xFE00, 0xFE0F)	AM_READWRITE(dgn_beta_page_r	,dgn_beta_page_w)
	AM_RANGE(0xFF00, 0xFFFF) 	AM_READWRITE(MRA8_BANK17	,MWA8_BANK17)
ADDRESS_MAP_END


INPUT_PORTS_START( dgnbeta )
	PORT_START
	PORT_START
INPUT_PORTS_END


static const char *dgnbeta_floppy_getname(const struct IODevice *dev, int id, char *buf, size_t bufsize)
{
	/* CoCo people like their floppy drives zero counted */
	snprintf(buf, bufsize, "Floppy #%d", id);
	return buf;
}

static void dgnbeta_floppy_getinfo(struct IODevice *dev)
{
	/* floppy */
	floppy_device_getinfo(dev, floppyoptions_coco);
	dev->count = 4;
	dev->name = dgnbeta_floppy_getname;
}

static PALETTE_INIT( dgnbeta )
{
	palette_set_colors(0, dgnbeta_palette, sizeof(dgnbeta_palette) / 3);
	memcpy(colortable, dgnbeta_colortable, sizeof(colortable));
}

static MACHINE_DRIVER_START( dgnbeta )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809E, DGNBETA_CPU_SPEED_HZ)        /* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(dgnbeta_map,0)
	
	/* both cpus in the beta share the same address/data busses */
	MDRV_CPU_ADD_TAG("dma", M6809E, DGNBETA_CPU_SPEED_HZ)        /* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(dgnbeta_map,0)

	MDRV_FRAMES_PER_SECOND(DGNBETA_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( dgnbeta )
	MDRV_MACHINE_STOP( dgnbeta )

	/* video hardware */
	
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0, 320 - 1, 0, 200 - 1)
	MDRV_GFXDECODE( dgnbeta_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (dgnbeta_palette) / sizeof (dgnbeta_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (dgnbeta_colortable) / sizeof(dgnbeta_colortable[0][0]))
	MDRV_PALETTE_INIT( dgnbeta )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( dgnbeta )
MACHINE_DRIVER_END

ROM_START(dgnbeta)
	ROM_REGION(0x4000,REGION_CPU1,0)
	ROM_LOAD("beta_bt.rom"	,0x0000	,0x4000	,CRC(4c54c1de) SHA1(141d9fcd2d187c305dff83fce2902a30072aed76))

	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD("betachar.rom"	,0x0000	,0x2000	,CRC(ca79d66c) SHA1(8e2090d471dd97a53785a7f44a49d3c8c85b41f2)) 	
ROM_END

SYSTEM_CONFIG_START(dgnbeta)
	CONFIG_RAM(RamSize * 1024)
	CONFIG_DEVICE( dgnbeta_floppy_getinfo )
SYSTEM_CONFIG_END

/*     YEAR	NAME		PARENT	COMPAT		MACHINE    	INPUT		INIT    	CONFIG		COMPANY			FULLNAME */
COMP(  1984,	dgnbeta,	0,	0,		dgnbeta,	dgnbeta,	dgnbeta,	dgnbeta,	"Dragon Data Ltd",    "Dragon Beta Prototype" , 0)
