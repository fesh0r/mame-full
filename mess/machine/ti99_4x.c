/*
	Machine code for TI-99/4A (TI99/4).
	Raphael Nabet, 1999, 2000.
	Some code derived from Ed Swartz's V9T9.

	References :
	* The TI-99/4A Tech Pages <http://www.nouspikel.com/ti99/titech.htm>.  Great site.
	* V9T9 source code.

Emulated :
	* All TI99 basic console hardware, except tape input, and a few tricks in TMS9901 emulation.
	* Cartidge with ROM (either non-paged or paged) and GROM (no GRAM or extra GPL ports).
	* Speech Synthesizer, with standard speech ROM (no speech ROM expansion).
	* Disk emulation (incomplete, but seems to work OK).

	Compatibility looks quite good.

TODO :
	* Tape support ?
	* Submit speech improvements to Nicola again
	* support for other peripherals and DSRs as documentation permits
	* support for TI99/4 as documentation permits
	* ...

New (9911) :
	* updated for 16 bit handlers.
	* added some timing (OK, it's a trick...).
New (991202) :
	* fixed the CRU base for disk
New (991206) :
	* "Juergenified" the timer code
New (991210) :
	* "Juergenified" code to use region accessor
New (000125) :
	* Many small improvements, and bug fixes
	* Moved tms9901 code to tms9901.c
New (000204) :
	* separated tms9901 code from ti99/4a code further
	* created drivers for 50hz versions of ti99/4x
New (000305) :
	* updated for new MESS devices interfaces
	* added tape support (??? Does not work on me...)
New (0004) :
	* uses file extensions (Norberto Bensa)
New (0005) :
	* fixed problems caused by the former patch
	* fixed some tape bugs.  The CS1 unit now works occasionally
New (000531) :
	* various small bugfixes
New (001004) :
	* updated for new mem handling
*/

#include "driver.h"
#include "includes/wd179x.h"
#include "tms9901.h"
#include "vidhrdw/tms9928a.h"
#include "sndhrdw/spchroms.h"
#include "includes/basicdsk.h"
#include <math.h>

#include "ti99_4x.h"

/*
	pointers in RAM areas
*/
UINT16 *ti99_scratch_RAM;
UINT16 *ti99_xRAM_low;
UINT16 *ti99_xRAM_high;

/*UINT16 *ti99_cart_mem;*/
UINT16 *ti99_DSR_mem;

/*
	GROM support.

In short :

	TI99/4x hardware supports GROMs.  GROMs are special ROMs, which can be accessed serially
	(i.e. byte by byte), although you can edit the address pointer whenever you want.

	These ROMs are rather slow, and do not support 16-bit accesses.  They are generally used to
	store programs in GPL (a proprietary, interpreted language - the interpreter take most of
	a TI99/4x CPU ROMs).  They can used to store large pieces of data, too.

	TI99/4a includes three GROMs, with some start-up code and TI-Basic.
	TI99/4 reportedly includes an extra GROM, with Equation Editor.  Maybe a part of the Hand Held
	Unit DSR lurks there, too (this is only a supposition).

The simple way :

	Each GROM is logically 8kb long.

	Communication with GROM is done with 4 memory-mapped registers.  You can read or write
	a 16-bit address pointer, and read data from GROMs.  One register allows to write data, too,
	which would support some GRAMs, but AFAIK TI never built such GRAMs (although docs from TI
	do refer to this possibility).

	Since address are 16-bit long, you can have up to 8 ROMs.  So, on TI99/4a, a cartidge may
	include up to 5 GROMs.  (Actually, there is a way you can use more GROMs - see below...)

	The address pointer is incremented after each GROM operation, but it will always remain
	within the bounds of the currently selected GROM (e.g. after 0x3fff comes 0x2000).

Some details :

	Original TI-built GROM are 6kb long, but take 8kb in address space.  The extra 2kb can be
	read, and follow the following formula :
	GROM[0x1800+offset] = GROM[0x0800+offset] | GROM[0x1000+offset]
	(sounds like address decoding is incomplete - we are lucky we don't burn any silicon...)

	Needless to say, some hackers simulated 8kb GRAMs and GROMs with normal RAM/PROM chips and
	glue logic.

GPL ports :

	TI99/4x ROMs have been designed to allow the use (addr & 0x3C) as a GPL port number.  As a
	consequence, we can theorically have up to 16 independant GPL ports, with 64kb of address space
	in each.

	Note however, that, AFAIK, the console GROMs on TI99/4a do not decode the page number, so they
	occupy the first 24kb of EVERY port, and only 40kb of address space are really available (which
	actually makes 30kb with 6kb GROMs).  (However, some hackers found that the systems GROM
	drivers did not burn when another chip imposed another value on the data bus, so you could
	override the system GROMs and use custom code instead...)

	The question is : which pieces of hardware do use this ? I can only make guesses :
	* p-code card (-> UCSD Pascal system) contains 8 GROMs, so it must use two ports.
	* TI99/4 reportedly has 4 GROMs, whereas 1979's Statistics module has 5 GROMs.  So either
	  the console or the module uses an extra port.  I suspect Equation Editor is located in GPL
	  port 1.
	* Also, I know that some hackers did use the extra ports.
*/
/*
	Implementation :

	Port address decoding is rarely done, so most times all GPL ports n just map to
	port 0.  To emulate this, we use the GPL_lookup_table : we fill it with 0s when no decoding
	is done, with 0,1,2,... 15 when complete decoding is done, etc.

	Limits :

	We only keep one address port
*/
/*static int GPL_lookup_table[16][8];
static int GPL_reverse_lookup_table[16][8];
static unsigned int GPL_address_pointer;
static unsigned char *GPL_data[16][8];
static int is_GRAM[16][8];*/

static unsigned char *GPL_data;

/*
	DSR support

In short :
	DSR = Device Service Routine

	16 CRU address intervals are reserved for expansion.  I appended known TI peripherals which use
	each port (appended '???' when I don't know what the peripheral is :-) ).
	* 0x1000-0x10FE unassigned
	* 0x1100-0x11FE disk controller
	* 0x1200-0x12FE modems ???
	* 0x1300-0x13FE RS232 1&2, PIO 1
	* 0x1400-0x14FE unassigned
	* 0x1500-0x15FE RS232 3&4, PIO 2
	* 0x1600-0x16FE unassigned
	* 0x1700-0x17FE hex-bus (prototypes)
	* 0x1800-0x18FE thermal printer (early peripheral)
	* 0x1900-0x19FE TI99/7 Mezzanine board (prototypes) - or EPROM programmer ???
	* 0x1A00-0x1AFE unassigned
	* 0x1B00-0x1BFE TI GPL debugger card
	* 0x1C00-0x1CFE Video Controller Card ???
	* 0x1D00-0x1DFE IEEE 488 Controller Card ??? (I MUST have heard of this one, but I don't know where)
	* 0x1E00-0x1EFE unassigned
	* 0x1F00-0x1FFE P-code card

	Also, the cards can trigger level-1 interrupts.  (A LOAD interrupt is possible, but was never
	used, except maybe by debugger cards.)

	Of course, we will generally need some support routines, and we may want to use memory-mapped
	registers.  To do this, memory range 0x4000-5FFF is shared by all cards.  The system enables
	each card as needed by writing a 1 to the first CRU bit : when this happens the card can safely
	enable its ROM, RAM, memory-mapped registers, etc.  Provided the ROM uses the proper ROM header,
	the system will recognize and call peripheral I/O functions, interrupt service routines,
	startup routines, etc.

	Only disk DSR is supported for now.
*/

extern int tms9900_ICount;


/*================================================================
	General purpose code :
	initialization, cart loading, etc.
================================================================*/

static UINT16 *cartidge_pages[2] = {NULL, NULL};
static int cartidge_minimemory = FALSE;
static int cartidge_paged = FALSE;
static UINT16 *current_page_ptr;
/* tells the cart file types - needed for cleanup... */
typedef enum slot_type_t { SLOT_EMPTY = -1, SLOT_GROM = 0, SLOT_CROM = 1, SLOT_DROM = 2, SLOT_MINIMEM = 3 } slot_type_t;
static slot_type_t slot_type[3] = { SLOT_EMPTY, SLOT_EMPTY, SLOT_EMPTY};


int ti99_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_OK)
	{
		basicdsk_set_geometry(id, 40, 1, 9, 256,0);

		return INIT_OK;
	}

	return INIT_FAILED;
}

int ti99_cassette_init(int id)
{
	void *file;

#if 1
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAILED;

		return INIT_OK;
	}
#endif

	/* HJB 02/18: no file, create a new file instead */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 22050; /* maybe 11025 Hz would be sufficient? */
		/* open in write mode */
		if (device_open(IO_CASSETTE, id, 1, &wa))
			return INIT_FAILED;
		return INIT_OK;
	}

	return INIT_OK;
}

void ti99_cassette_exit(int id)
{
	device_close(IO_CASSETTE,id);
}

/*
	Load ROM.  All files are in raw binary format.
	1st ROM : GROM (up to 40kb)
	2nd ROM : CPU ROM (8kb)
	3rd ROM : CPU ROM, 2nd page (8kb)
*/
int ti99_load_rom(int id)
{
	const char *name = device_filename(IO_CARTSLOT,id);
	void *cartfile = NULL;

	int slot_empty = ! (name && name[0]);

	cartidge_pages[0] = (UINT16 *) (memory_region(REGION_CPU1)+0x06000);
	cartidge_pages[1] = (UINT16 *) (memory_region(REGION_USER2));

	if (slot_empty)
		slot_type[id] = SLOT_EMPTY;

	if (! slot_empty)
	{
		cartfile = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
		if (cartfile == NULL)
		{
			logerror("TI99 - Unable to locate cartridge: %s\n", name);
			return INIT_FAILED;
		}

		/* Trick - we identify file types according to their extension */
		/* Note that if we do not recognize the extension, we revert to the slot location <-> type
		scheme.  I do this because the extension concept is quite unfamiliar to mac people
		(I am dead serious). */
		/* Original idea by Norberto Bensa <nbensa@hotmail.com> */
		{

		char *ch, *ch2;
		slot_type_t type = (slot_type_t) id;

		ch = strrchr(name, '.');
		ch2 = (ch-1 >= name) ? ch-1 : "";

		if (ch)
		{
			if ((! stricmp(ch2, "g.bin")) || (! stricmp(ch, ".grom")) || (! stricmp(ch, ".g")))
			{
				/* grom */
				type = SLOT_GROM;
			}
			else if ((! stricmp(ch2, "c.bin")) || (! stricmp(ch, ".crom")) || (! stricmp(ch, ".c")))
			{
				/* rom first page */
				type = SLOT_CROM;
			}
			else if ((! stricmp(ch2, "d.bin")) || (! stricmp(ch, ".drom")) || (! stricmp(ch, ".d")))
			{
				/* rom second page */
				type = SLOT_DROM;
			}
			else if ((! stricmp(ch2, "m.bin")) || (! stricmp(ch, ".mrom")) || (! stricmp(ch, ".m")))
			{
				/* rom minimemory  */
				type = SLOT_MINIMEM;
			}
		}

		slot_type[id] = type;

		switch (type)
		{
		case SLOT_EMPTY:
			break;

		case SLOT_GROM:
			osd_fread(cartfile, memory_region(REGION_USER1) + 0x6000, 0xA000);
			break;

		case SLOT_MINIMEM:
			cartidge_minimemory = TRUE;
		case SLOT_CROM:
			osd_fread_msbfirst(cartfile, cartidge_pages[0], 0x2000);
			break;

		case SLOT_DROM:
			cartidge_paged = TRUE;
			osd_fread_msbfirst(cartfile, cartidge_pages[1], 0x2000);
			break;
		}

		}

		osd_fclose(cartfile);
	}

	return INIT_OK;
}

void ti99_rom_cleanup(int id)
{
	switch (slot_type[id])
	{
	case SLOT_EMPTY:
		break;

	case SLOT_GROM:
		memset(memory_region(REGION_USER1) + 0x6000, 0, 0xA000);
		break;

	case SLOT_MINIMEM:
		cartidge_minimemory = FALSE;
		/* we should insert some code to save the minimem contents... */
	case SLOT_CROM:
		memset(cartidge_pages[0], 0, 0x2000);
		break;

	case SLOT_DROM:
		cartidge_paged = FALSE;
		break;
	}
}

/*
	ti99_init_machine() ; launched AFTER ti99_load_rom...
*/

static int ti99_R9901_0(int offset);
static int ti99_R9901_1(int offset);
static int ti99_R9901_3(int offset);

static void ti99_KeyC2(int offset, int data);
static void ti99_KeyC1(int offset, int data);
static void ti99_KeyC0(int offset, int data);
static void ti99_AlphaW(int offset, int data);
static void ti99_CS1_motor(int offset, int data);
static void ti99_CS2_motor(int offset, int data);
static void ti99_audio_gate(int offset, int data);
static void ti99_CS_output(int offset, int data);

static tms9901reset_param tms9901reset_param_ti99 =
{
	TMS9901_INT1 | TMS9901_INT2,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_R9901_0,
		ti99_R9901_1,
		NULL,
		ti99_R9901_3
	},

	{	/* write handlers */
		NULL,
		NULL,
		ti99_KeyC2,
		ti99_KeyC1,
		ti99_KeyC0,
		ti99_AlphaW,
		ti99_CS1_motor,
		ti99_CS2_motor,
		ti99_audio_gate,
		ti99_CS_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	}
};

static void tms9901_set_int2(int state)
{
	tms9901_set_single_int(2, state);
}

static void ti99_fdc_callback(int);

void ti99_init_machine(void)
{
	spchroms_interface speech_intf = { REGION_SOUND1 };

	spchroms_config(& speech_intf);

	GPL_data = memory_region(REGION_USER1);

	/* callback for the TMS9901 to be notified of changes to the
	 * TMS9928A INT* line (connected to the TMS9901 INT2* line)
	 */
	TMS9928A_int_callback(tms9901_set_int2);

	wd179x_init(ti99_fdc_callback);				/* initialize the floppy disk controller */
	wd179x_set_density(DEN_FM_LO);

	tms9901_init(& tms9901reset_param_ti99);

	current_page_ptr = cartidge_pages[0];
}

void ti99_stop_machine(void)
{
	wd179x_exit();
	tms9901_cleanup();
}


/*
	ROM identification : currently does nothing.

	Actually, TI ROM header starts with a >AA.  Unfortunately, header-less ROMs do exist.
*/
int ti99_id_rom(int id)
{
	return 1;
}

/*
	video initialization.
*/
int ti99_4_vh_start(void)
{
	return TMS9928A_start(TMS99x8, 0x4000);		/* tms9918/28/29 with 16 kb of video RAM */
}

int ti99_4a_vh_start(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);	/* tms9918a/28a/29a with 16 kb of video RAM */
}

/*
	VBL interrupt  (mmm... actually, it happens when the beam enters the lower border, so it is not
	a genuine VBI, but who cares ?)
*/
int ti99_vblank_interrupt(void)
{
	TMS9928A_interrupt();

	return ignore_interrupt();
}



/*================================================================
	Memory handlers.

	TODO :
	* GRAM support
	* GPL paging support
	* DSR support
	* better minimem support ?

================================================================*/

/*
	Same as MRA_NOP, but with an additionnal delay.
*/
READ16_HANDLER ( ti99_rw_null8bits )
{
	tms9900_ICount -= 4;

	return (0);
}

WRITE16_HANDLER ( ti99_ww_null8bits )
{
	tms9900_ICount -= 4;
}

/*
	Memory extension : same as MRA_RAM, but with an additionnal delay.
*/
/* low 8 kb : 0x2000-0x3fff */
READ16_HANDLER ( ti99_rw_xramlow )
{
	tms9900_ICount -= 4;

	return ti99_xRAM_low[offset];
}

WRITE16_HANDLER ( ti99_ww_xramlow )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(ti99_xRAM_low + offset);
}

/* high 24 kb : 0xa000-0xffff */
READ16_HANDLER ( ti99_rw_xramhigh )
{
	tms9900_ICount -= 4;

	return ti99_xRAM_high[offset];
}

WRITE16_HANDLER ( ti99_ww_xramhigh )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(ti99_xRAM_high + offset);
}

/*
	Cartidge read : same as MRA_ROM, but with an additionnal delay.
*/
READ16_HANDLER ( ti99_rw_cartmem )
{
	tms9900_ICount -= 4;

	return current_page_ptr[offset];
}

/*
	this handler handles ROM switching in cartidges
*/
WRITE16_HANDLER ( ti99_ww_cartmem )
{
	tms9900_ICount -= 4;

	if (cartidge_minimemory && offset >= 0x800)
		COMBINE_DATA(current_page_ptr+offset);
	else if (cartidge_paged)
		current_page_ptr = cartidge_pages[offset & 1];
}

/*----------------------------------------------------------------
	Scratch RAM PAD.
	0x8000-8300 are the same as 0x8300-8400. (only 256 bytes installed)
----------------------------------------------------------------*/

/*
	PAD read
*/
READ16_HANDLER ( ti99_rw_scratchpad )
{
	return ti99_scratch_RAM[offset & 0x7f];
}

/*
	PAD write
*/
WRITE16_HANDLER ( ti99_ww_scratchpad )
{
	COMBINE_DATA(ti99_scratch_RAM + (offset & 0x7f));
}

/*----------------------------------------------------------------
	memory-mapped registers handlers.
----------------------------------------------------------------*/

/*
	About memory-mapped registers.

	These are registers to communicate with several peripherals.
	These registers are all 8 bit wide, and are located at even adresses between
	0x8400 and 0x9FFE.
	The registers are identified by (addr & 1C00), and, for VDP and GPL access, by (addr & 2).
	These registers are either read-only or write-only.  (Actually, (addr & 4000) can
	be regarded as some sort of a R/W line.)

	Memory mapped registers list:
	- write sound chip. (8400-87FE)
	- read VDP memory. (8800-8BFE), (addr&2) == 0
	- read VDP status. (8800-8BFE), (addr&2) == 2
	- write VDP memory. (8C00-8FFE), (addr&2) == 0
	- write VDP address and VDP register. (8C00-8FFE), (addr&2) == 2
	- read speech synthesis chip. (9000-93FE)
	- write speech synthesis chip. (9400-97FE)
	- read GPL memory. (9800-9BFE), (addr&2) == 0 (1)
	- read GPL adress. (9800-9BFE), (addr&2) == 2 (1)
	- write GPL memory. (9C00-9FFE), (addr&2) == 0 (1)
	- write GPL adress. (9C00-9FFE), (addr&2) == 2 (1)

(1) on some hardware, (addr & 0x3C) provides a GPL page number.
*/

/*
	TMS9919 sound chip write
*/
WRITE16_HANDLER ( ti99_ww_wsnd )
{
	tms9900_ICount -= 4;

	SN76496_0_w(offset, (data >> 8) & 0xff);
}

/*
	TMS9918A VDP read
*/
READ16_HANDLER ( ti99_rw_rvdp )
{
	tms9900_ICount -= 4;

	if (offset & 1)
	{	/* read VDP status */
		return ((int) TMS9928A_register_r(0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) TMS9928A_vram_r(0)) << 8;
	}
}

/*
	TMS9918A vdp write
*/
WRITE16_HANDLER ( ti99_ww_wvdp )
{
	tms9900_ICount -= 4;

	if (offset & 1)
	{	/* write VDP adress */
		TMS9928A_register_w(0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(0, (data >> 8) & 0xff);
	}
}

/*
	TMS5200 speech chip read
*/
READ16_HANDLER ( ti99_rw_rspeech )
{
	tms9900_ICount -= 4;				/* much more, actually */

	return ((int) tms5220_status_r(offset)) << 8;
}

/*
	TMS5200 speech chip write
*/
WRITE16_HANDLER ( ti99_ww_wspeech )
{
	tms9900_ICount -= 4;				/* much more, actually */

	tms5220_data_w(offset, (data >> 8) & 0xff);
}

/* current position in the gpl memory space */
static int gpl_addr = 0;

/*
	GPL read
*/
READ16_HANDLER ( ti99_rw_rgpl )
{
	tms9900_ICount -= 4;				/* much more, actually */

/*int page = (offset & 0x1E) >> 1; *//* GROM/GRAM can be paged */

	if (offset & 1)
	{	/* read GPL adress */
		int value;

		/* increment gpl_addr (GPL wraps in 8k segments!) : */
		value = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
		gpl_addr = (value & 0xFF) << 8;	/* gpl_addr is shifted left by 8 bits */
		return value & 0xFF00;			/* and we retreive the MSB */
		/* to get full GPL address, we make two byte reads! */
	}
	else
	{	/* read GPL data */
		int value = GPL_data[gpl_addr /*+ ((long) page << 16) */ ];	/* retreive byte */

		/* increment gpl_addr (GPL wraps in 8k segments!) : */
		gpl_addr = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
		return (value << 8);
	}
}

/*
	GPL write
*/
WRITE16_HANDLER ( ti99_ww_wgpl )
{
	tms9900_ICount -= 4;				/* much more, actually */

	data = (data >> 8) & 0xff;

/*int page = (offset & 0x1E) >> 1; *//* GROM/GRAM can be paged */

	if (offset & 1)
	{	/* write GPL adress */
		gpl_addr = ((gpl_addr & 0xFF) << 8) | (data & 0xFF);
	}
	else
	{	/* write GPL byte */
#if 0
		/* Disabled because we don't know whether we have RAM or ROM. */
		/* GRAMs are quite uncommon, anyway. */
		if ((gpl_addr >= gram_start) && (gpl_addr <= gram_end))
			GPL_data[gpl_addr /*+ ((long) page << 16) */ ] = value;

		/* increment gpl_addr (GPL wraps in 8k segments!) : */
		gpl_addr = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
#endif
	}
}




/*----------------------------------------------------------------
	disk DSR handlers
----------------------------------------------------------------*/

/* TRUE when the disk DSR is active */
int diskromon = 0;

/*
	read a byte in disk DSR.
*/
READ16_HANDLER ( ti99_rw_disk )
{
	tms9900_ICount -= 4;

	if (!diskromon)
		return 0;

	switch (offset)
	{
	case 0xFF8:						/* Status register */
		return (wd179x_status_r(offset) ^ 0xFF) << 8;
		break;
	case 0xFF9:						/* Track register */
		return (wd179x_track_r(offset) ^ 0xFF) << 8;
		break;
	case 0xFFA:						/* Sector register */
		return (wd179x_sector_r(offset) ^ 0xFF) << 8;
		break;
	case 0xFFB:						/* Data register */
		return (wd179x_data_r(offset) ^ 0xFF) << 8;
		break;
	default:
		return ti99_DSR_mem[offset];
		break;
	}
}

/*
	write a byte in disk DSR.
*/
WRITE16_HANDLER ( ti99_ww_disk )
{
	tms9900_ICount -= 4;

	if (!diskromon)
		return;

	data = ((data >> 8) & 0xFF) ^ 0xFF;	/* inverted data bus */

	switch (offset)
	{
	case 0xFFC:						/* Command register */
		wd179x_command_w(offset, data);
		break;
	case 0xFFD:						/* Track register */
		wd179x_track_w(offset, data);
		break;
	case 0xFFE:						/* Sector register */
		wd179x_sector_w(offset, data);
		break;
	case 0xFFF:						/* Data register */
		wd179x_data_w(offset, data);
		break;
	}
}

/*
	TI99/4x-specific tms9901 I/O handlers

TODO:
	* support for tape input, possibly improved tape output.

	KNOWN PROBLEMS :
	* a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is :
	  on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
	  timer mode even though the program did not explicitely ask... and THIS is impossible
	  to emulate efficiently (we'd have to check each memory operation).
*/

/*
	TMS9901 interrupt handling on a TI99/4x.

	TI99 uses the following interrupts :
	INT1 : external interrupt (used by RS232 controller, for instance)
	INT2 : VDP interrupt
	timer interrupt (overrides INT3)

	Three interrupts are used by the system (INT1, INT2, and timer), out of 15/16 possible
	interrupt.  Keyboard pins can be used as interrupt pins, too, but this is not emulated
	(it's a trick, anyway, and I don't know of any program which uses it).

	When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
	a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
	the interrupt pin and the revelant bit in the interrupt mask are set.

	TIMER interrupts are kind of an exception, since they are not associated with an external
	interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
	the pending interrupt (or am I wrong once again ?).

nota :
	All interrupt routines notify (by software) the TMS9901 of interrupt recognition ("SBO n").
	However, AFAIK, this has strictly no consequence in the TMS9901, and interrupt routines
	would work fine without this (except probably TIMER interrupt).  All this is quite weird.
	Maybe the interrupt recognition notification is needed on TMS9985, or any other weird
	variant of TMS9900 (does any TI990/10 owner wants to test this :-) ?).
*/

/* keyboard interface */
static int KeyCol = 0;
static int AlphaLockLine = 0;

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification :
	 (bit 1 : INT1 status)
	 (bit 2 : INT2 status)
	 bit 3-7 : keyboard status bits 0 to 4
*/
static int ti99_R9901_0(int offset)
{
	int answer;

	answer = (readinputport(KeyCol) << 3) & 0xF8;

	if (! AlphaLockLine)
		answer &= ~ (input_port_8_r(0) << 3);

	return (answer);
}

/*
	Read pins INT8*-INT15* of TI99's 9901.

	signification :
	 bit 0-2 : keyboard status bits 5 to 7
	 bit 3-7 : weird, not emulated
*/
static int ti99_R9901_1(int offset)
{
	return (readinputport(KeyCol) >> 5) & 0x07;
}

/*
	Read pins P0-P7 of TI99's 9901.
	Nothing is connected !
*/
/*static int ti99_R9901_2(int offset)
{
	return 0;
}*/

/*
	Read pins P8-P15 of TI99's 9901.
*/
static int ti99_R9901_3(int offset)
{
	/*only important bit : bit 27 : tape input */

	/* we don't take CS2 into account */
	return device_input(IO_CASSETTE, 0) > 0 ? 8 : 0;

	/*return 8;*/
	/*return 0;*/
}





/*
	WRITE column number bit 2
*/
static void ti99_KeyC2(int offset, int data)
{
	if (data)
		KeyCol |= 1;
	else
		KeyCol &= (~1);
}

/*
	WRITE column number bit 1
*/
static void ti99_KeyC1(int offset, int data)
{
	if (data)
		KeyCol |= 2;
	else
		KeyCol &= (~2);
}

/*
	WRITE column number bit 0
*/
static void ti99_KeyC0(int offset, int data)
{
	if (data)
		KeyCol |= 4;
	else
		KeyCol &= (~4);
}

/*
	WRITE alpha lock line
*/
static void ti99_AlphaW(int offset, int data)
{
	AlphaLockLine = data;
}

/*
	command CS1 tape unit motor
*/
static void ti99_CS1_motor(int offset, int data)
{
	/*device_status(IO_CASSETTE, 0, data);*/
}

/*
	command CS2 tape unit motor
*/
static void ti99_CS2_motor(int offset, int data)
{
	/*device_status(IO_CASSETTE, 1, ! data);*/
}

/*
	audio gate

	connected to the AUDIO IN pin of TMS9919

	set to 1 before using tape (in order not to burn the TMS9901 ??)

	I am not sure about polarity.
*/
static void ti99_audio_gate(int offset, int data)
{
	if (data)
		DAC_data_w(0, 0xFF);
	else
		DAC_data_w(0, 0);
}

/*
	tape output
	I am not sure about polarity.
*/
static void ti99_CS_output(int offset, int data)
{
	device_output(IO_CASSETTE, 0, data ? -32768 : 32767);
}


/*================================================================
	CRU handlers.

	See also /machine/tms9901.c.

	BTW, although TMS9900 is generally big-endian, it is little endian as far as CRU is
	concerned. (i.e. bit 0 is the least significant)
================================================================*/


/*----------------------------------------------------------------
	disk CRU interface.
----------------------------------------------------------------*/

/*
	Read disk CRU interface

	bit 0 : HLD pin
	bit 1-3 : drive n selected (setting the bit means it is absent ???)
	bit 4 : 0: motor strobe on
	bit 5 : always 0
	bit 6 : always 1
	bit 7 : selected side
*/
READ16_HANDLER ( ti99_DSKget )
{
	return (0x40);
}

/*
	WRITE to DISK DSR ROM bit (bit 0)
*/
WRITE16_HANDLER ( ti99_DSKROM )
{
	if (data & 1)
	{
		diskromon = 1;
	}
	else
	{
		diskromon = 0;
	}
}



static int DSKhold;

static int DRQ_IRQ_status;

#define fdc_IRQ 1
#define fdc_DRQ 2

#define HALT   ASSERT_LINE
#define RESUME CLEAR_LINE

static void handle_hold(void)
{
	if (DSKhold && (!DRQ_IRQ_status))
		cpu_set_halt_line(1, ASSERT_LINE);
	else
		cpu_set_halt_line(1, CLEAR_LINE);
}

static void ti99_fdc_callback(int event)
{
	switch (event)
	{
	case WD179X_IRQ_CLR:
		DRQ_IRQ_status &= ~fdc_IRQ;
		break;
	case WD179X_IRQ_SET:
		DRQ_IRQ_status |= fdc_IRQ;
		break;
	case WD179X_DRQ_CLR:
		DRQ_IRQ_status &= ~fdc_DRQ;
		break;
	case WD179X_DRQ_SET:
		DRQ_IRQ_status |= fdc_DRQ;
		break;
	}

	handle_hold();
}

/*
	Set disk ready/hold (bit 2)
	0 : ignore IRQ and DRQ
	1 : TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates for
	    4.23s after write to revelant CRU bit, this is not emulated and could cause the TI99
	    to lock...)
*/
WRITE16_HANDLER ( ti99_DSKhold )
{
	DSKhold = data & 1;

	handle_hold();
}


/*
	Load disk heads (HLT pin) (bit 3)
*/
WRITE16_HANDLER ( ti99_DSKheads )
{
}




static int DSKnum = -1;
static int DSKside = 0;

/*
	Select drive X (bits 4-6)
*/
WRITE16_HANDLER ( ti99_DSKsel )
{
	int drive = offset;					/* drive # (0-2) */

	if (data & 1)
	{
		if (drive != DSKnum)			/* turn on drive... already on ? */
		{
			DSKnum = drive;

			wd179x_set_drive(DSKnum);
			wd179x_set_side(DSKside);
		}
	}
	else
	{
		if (drive == DSKnum)			/* geez... who cares? */
		{
			DSKnum = -1;				/* no drive selected */
		}
	}
}

/*
	Select side of disk (bit 7)
*/
WRITE16_HANDLER ( ti99_DSKside )
{
	DSKside = data & 1;
	wd179x_set_side(DSKside);
}


