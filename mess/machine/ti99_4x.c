/*
	Machine code for TI99/4, TI-99/4A, TI99/4P.
	Raphael Nabet, 1999-2002.
	Some code derived from Ed Swartz's V9T9.

	References:
	* The TI-99/4A Tech Pages <http://www.nouspikel.com/ti99/titech.htm>.  Great site.
	* ftp.whtech.com has schematics and hardware documentations.  The documents of most general
	  interest are:
		<ftp://ftp.whtech.com//datasheets/Hardware manuals/99-4  console specification and schematic.pdf>
		<ftp://ftp.whtech.com//datasheets/Hardware manuals/99-4A Console and peripheral Expansion System Technical Data.pdf>
	  Schematics for various extension cards are available in the same directory.
	* V9T9 source code.
	* Harald Glaab's site has software and documentation for the TI99/4P (nicknamed "SGCPU").
		<http://home.t-online.de/home/harald.glaab/snug/>

Emulated:
	* All TI99 basic console hardware, except a few tricks in TMS9901 emulation.
	* Cartridge with ROM (either non-paged or paged) and GROM (GRAM or extra GPL ports are possible).
	* Speech Synthesizer, with standard speech ROM (no speech ROM expansion).
	* Disk emulation (only SSSD disk images, and timings are completely wrong).

	Compatibility looks quite good.

TODO:
	* DUMP THIS BLOODY TI99/4 ROM
	* Submit speech improvements to Nicola again
	* support for other peripherals and DSRs as documentation permits
	* find programs which use super AMS or any other extended memory card
	* finish 99/4p support: ROM6, HSGPL and EVPC cards

New (9911):
	* updated for 16 bit handlers.
	* added some timing (OK, it's a trick...).
New (991202):
	* fixed the CRU base for disk
New (991206):
	* "Juergenified" the timer code
New (991210):
	* "Juergenified" code to use region accessor
New (000125):
	* Many small improvements, and bug fixes
	* Moved tms9901 code to tms9901.c
New (000204):
	* separated tms9901 code from ti99/4a code further
	* created drivers for 50hz versions of ti99/4x
New (000305):
	* updated for new MESS devices interfaces
	* added tape support (??? Does not work on me...)
New (0004):
	* uses file extensions (Norberto Bensa)
New (0005):
	* fixed problems caused by the former patch
	* fixed some tape bugs.  The CS1 unit now works occasionally
New (000531):
	* various small bugfixes
New (001004):
	* updated for new mem handling
New (020327):
	This is a big rewrite.  The code should be more flexible and more readable.
	* updated tms9901 code
	* fdc, speech synthesizer, memory extension can be disabled
	* support for multiple extension cards
	* support for super AMS, foundation, and a myarc look-alike memory extension cards
	* support for multiple GROM ports
	* better GROM and speech timings
*/

#include "driver.h"
#include "includes/wd179x.h"
#include "tms9901.h"
#include "vidhrdw/tms9928a.h"
#include "vidhrdw/v9938.h"
#include "sndhrdw/spchroms.h"
#include "includes/basicdsk.h"
#include <math.h>
#include "cassette.h"

#include "ti99_4x.h"


/* prototypes */
static READ16_HANDLER ( ti99_rw_rspeech );
static WRITE16_HANDLER ( ti99_ww_wspeech );

static void tms9901_interrupt_callback(int intreq, int ic);
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

static void ti99_expansion_card_init(void);
static void ti99_4p_internal_dsr_init(void);
static void ti99_TIxram_init(void);
static void ti99_sAMSxram_init(void);
static void ti99_4p_mapper_init(void);
static void ti99_myarcxram_init(void);
static void ti99_fdc_init(void);
static void ti99_evpc_init(void);

/*
	pointers to RAM areas
*/
/* pointer to scratch RAM */
static UINT16 *sRAM_ptr;
/* pointer to extended RAM */
static UINT16 *xRAM_ptr;

/*
	Configuration
*/
/* model of ti99 in emulation */
static enum
{
	model_99_4,
	model_99_4a,
	model_99_4p
} ti99_model;
/* memory extension type */
static xRAM_kind_t xRAM_kind;
/* TRUE if speech synthesizer present */
static char has_speech;
/* TRUE if floppy disk controller present */
static char has_fdc;
/* TRUE if evpc card present */
static char has_evpc;


/* tms9901 setup */
static const tms9901reset_param tms9901reset_param_ti99 =
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
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};

/* keyboard interface */
static int KeyCol;
static int AlphaLockLine;

/*
	GROM support.

In short:

	TI99/4x hardware supports GROMs.  GROMs are slow, which are interfaced via a 8-bit data
	bus, and include an internal address pointer which is incremented after each read.
	This implies that accesses are faster when reading consecutive bytes, although
	the address pointer can be read and written at any time.

	They are generally used to store programs in GPL (a proprietary, interpreted language -
	the interpreter take most of a TI99/4(a) CPU ROMs).  They can used to store large pieces
	of data, too.

	Both TI99/4 and TI99/4a include three GROMs, with some start-up code, system routines and
	TI-Basic.  TI99/4 reportedly includes an additionnal Equation Editor.  Maybe a part of
	the Hand Held Unit DSR lurks there, too (this is only a supposition).  TI99/8 includes
	the three standard GROMs and 16 GROMs for the UCSD p-system.  TI99/2 does not include GROMs
	at all, and was not designed to support any, although would be relatively easy to create
	an expansion card with the GPL interpreter and a /4a cartridge port.

The simple way:

	Each GROM is logically 8kb long.

	Communication with GROM is done with 4 memory-mapped locations.  You can read or write
	a 16-bit address pointer, and read data from GROMs.  One register allows to write data, too,
	which would support some GRAMs, but AFAIK TI never built such GRAMs (although docs from TI
	do refer to this possibility).

	Since address are 16-bit long, you can have up to 8 GROMs.  So, a cartridge may
	include up to 5 GROMs.  (Actually, there is a way more GROMs can be used - see below...)

	The address pointer is incremented after each GROM operation, but it will always remain
	within the bounds of the currently selected GROM (e.g. after 0x3fff comes 0x2000).

Some details:

	Original TI-built GROM are 6kb long, but take 8kb in address space.  The extra 2kb can be
	read, and follow the following formula:
		GROM[0x1800+offset] = GROM[0x0800+offset] | GROM[0x1000+offset];
	(sounds like address decoding is incomplete - we are lucky we don't burn any silicon when
	doing so...)

	Needless to say, some hackers simulated 8kb GRAMs and GROMs with normal RAM/PROM chips and
	glue logic.

GPL ports:

	When accessing the GROMs registers, 8 address bits (cpu_addr & 0x03FC) may be used as a port
	number, which permits the use of up to 256 independant GROM ports, with 64kb of address
	space in each.  TI99/4(a) ROMs can take advantage of the first 16 ports: it will look for
	GPL programs in every ROM of the 16 first ports.  On the other hand, the other ports cannot
	contain standard GROMs, but they may still contain custom GROMs with data.

	Note, however, that the TI99/4(a) console does not decode the page number, so console GROMs
	occupy the first 24kb of EVERY port, and cartridge GROMs occupy the next 40kb of EVERY port.
	(Note that the TI99/8 console does have the required decoder.)  Fortunately, GROM drivers
	have a relatively high impedance, and therefore extension cards can use TTL drivers to impose
	another value on the data bus with no risk of burning the console GROMs.  This hack permits
	the addition of additionnal GROMs in other ports, with the only side effect that whenever
	the address pointer in port N is altered, the address pointer of console GROMs in port 0
	is altered, too.  Overriding the system GROMs with custom code is possible, too (some hackers
	have done so), but I would not recommended it, as connecting such a device to a TI99/8 might
	burn some drivers.

	The p-code card (-> UCSD Pascal system) contains 8 GROMs, all in port 16.  This port is not
	in the range recognized by the TI ROMs (0-15), and therefore it is used by the p-code DSR ROMs
	as custom data.  Additonnally, some hackers used the extra ports to implement "GRAM" devices.
*/
/* defines for GROM_flags */
enum
{
	gf_has_look_ahead = 0x01,	/* has a look-ahead buffer - true with original GROMs */
	gf_is_GRAM = 0x02			/* is a GRAM - false with original GROMs */
};

/* GROM_port_t: descriptor for a port of 8 GROMs */
typedef struct GROM_port_t
{
	/* pointer to GROM data */
	UINT8 *data_ptr;
	/* active GROM in port (3 bits: 0-7) */
	//int active_GROM;
	/* current address pointer for the active GROM in port (16 bits) */
	unsigned int addr;
	/* GROM data buffer */
	UINT8 buf;
	/* internal flip-flops which are set after the first access to GROM address, so that next access
	is mapped LSB, and cleared after each data access */
	char raddr_LSB, waddr_LSB;
	/* flags for active GROM */
	char cur_flags;
	/* flags for each GROM chip */
	char GROM_flags[8];
} GROM_port_t;

/* GPL_port_lookup[n]: points to specific GROM_port_t structure when console GROMs are overriden
	by another GROM in port n, NULL otherwise */
static GROM_port_t *GPL_port_lookup[256];

/* descriptor for console GROMs */
GROM_port_t console_GROMs;

/*
	Cartridge support
*/
/* pointer on two 8kb cartridge pages */
static UINT16 *cartridge_pages[2] = {NULL, NULL};
/* flag: TRUE if the cartridge is minimemory (4kb of battery-backed SRAM in 0x5000-0x5fff) */
static char cartridge_minimemory = FALSE;
/* flag: TRUE if the cartridge is paged */
static char cartridge_paged = FALSE;
/* flag on the data for the current page (cartridge_pages[0] if cartridge is not paged) */
static UINT16 *current_page_ptr;
/* keep track of cart file types - required for cleanup... */
typedef enum slot_type_t { SLOT_EMPTY = -1, SLOT_GROM = 0, SLOT_CROM = 1, SLOT_DROM = 2, SLOT_MINIMEM = 3 } slot_type_t;
static slot_type_t slot_type[3] = { SLOT_EMPTY, SLOT_EMPTY, SLOT_EMPTY};


/* tms9900_ICount: used to implement memory waitstates (hack) */
extern int tms9900_ICount;



/*===========================================================================*/
/*
	General purpose code:
	initialization, cart loading, etc.
*/

void init_ti99_4(void)
{
	ti99_model = model_99_4;
	has_evpc = FALSE;
}

void init_ti99_4a(void)
{
	ti99_model = model_99_4a;
	has_evpc = FALSE;
}

void init_ti99_4ev(void)
{
	ti99_model = model_99_4a;
	has_evpc = TRUE;
}

void init_ti99_4p(void)
{
	ti99_model = model_99_4p;
	has_evpc = TRUE;
}

int ti99_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 40, 1, 9, 256, 0, 0);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

int ti99_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void ti99_cassette_exit(int id)
{
	device_close(IO_CASSETTE,id);
}

/*
	Load ROM.  All files are in raw binary format.
	1st ROM: GROM (up to 40kb)
	2nd ROM: CPU ROM (8kb)
	3rd ROM: CPU ROM, 2nd page (8kb)
*/
int ti99_load_rom(int id)
{
	const char *name = device_filename(IO_CARTSLOT,id);
	void *cartfile = NULL;

	int slot_empty = ! (name && name[0]);

	if (ti99_model == model_99_4p)
	{
		cartridge_pages[0] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart_4p);
		cartridge_pages[1] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart_4p + 0x2000);
	}
	else
	{
		cartridge_pages[0] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart);
		cartridge_pages[1] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart + 0x2000);
	}


	if (slot_empty)
		slot_type[id] = SLOT_EMPTY;

	if (! slot_empty)
	{
		cartfile = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE, 0);
		if (cartfile == NULL)
		{
			logerror("TI99 - Unable to locate cartridge: %s\n", name);
			return INIT_FAIL;
		}

		/* Trick - we identify file types according to their extension */
		/* Note that if we do not recognize the extension, we revert to the slot location <-> type
		scheme.  I do this because the extension concept is quite unfamiliar to mac people
		(I am dead serious). */
		/* Original idea by Norberto Bensa */
		{

		const char *ch, *ch2;
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
			osd_fread(cartfile, memory_region(region_grom) + 0x6000, 0xA000);
			break;

		case SLOT_MINIMEM:
			cartridge_minimemory = TRUE;
		case SLOT_CROM:
			osd_fread_msbfirst(cartfile, cartridge_pages[0], 0x2000);
			current_page_ptr = cartridge_pages[0];
			break;

		case SLOT_DROM:
			cartridge_paged = TRUE;
			osd_fread_msbfirst(cartfile, cartridge_pages[1], 0x2000);
			current_page_ptr = cartridge_pages[0];
			break;
		}

		}

		osd_fclose(cartfile);
	}

	return INIT_PASS;
}

void ti99_rom_cleanup(int id)
{
	switch (slot_type[id])
	{
	case SLOT_EMPTY:
		break;

	case SLOT_GROM:
		memset(memory_region(region_grom) + 0x6000, 0, 0xA000);
		break;

	case SLOT_MINIMEM:
		cartridge_minimemory = FALSE;
		/* we should insert some code to save the minimem contents... */
	case SLOT_CROM:
		memset(cartridge_pages[0], 0, 0x2000);
		break;

	case SLOT_DROM:
		cartridge_paged = FALSE;
		current_page_ptr = cartridge_pages[0];
		break;
	}
}


/*
	ti99_init_machine() ; launched AFTER ti99_load_rom...
*/
void machine_init_ti99(void)
{
	int i;


	/* set up RAM pointers */
	if (ti99_model == model_99_4p)
	{
		sRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_sram_4p);
		xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram_4p);
	}
	else
	{
		sRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_sram);
		xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram);
	}

	/* erase GPL_port_lookup, so that only console_GROMs are installed by default */
	memset(GPL_port_lookup, 0, sizeof(GPL_port_lookup));

	for (i=0; i<8;i++)
		console_GROMs.GROM_flags[i] = gf_has_look_ahead;

	console_GROMs.data_ptr = memory_region(region_grom);
	console_GROMs.addr = 0;
	console_GROMs.cur_flags = console_GROMs.GROM_flags[0];

	if (ti99_model == model_99_4p)
	{
		/* set up system ROM and scratch pad pointers */
		cpu_setbank(1, memory_region(REGION_CPU1) + offset_rom0_4p);
		cpu_setbank(2, sRAM_ptr);
	}
	else
	{
		/* set up 4 mirrors of scratch pad to the same address */
		cpu_setbank(1, sRAM_ptr);
		cpu_setbank(2, sRAM_ptr);
		cpu_setbank(3, sRAM_ptr);
		cpu_setbank(4, sRAM_ptr);
	}

	/* reset cartridge mapper */
	current_page_ptr = cartridge_pages[0];

	/* init tms9901 */
	tms9901_init(& tms9901reset_param_ti99);

	if (! has_evpc)
		TMS9928A_reset();
	if (has_evpc)
		v9938_reset();

	/* clear keyboard interface state (probably overkill, but can't harm) */
	KeyCol = 0;
	AlphaLockLine = 0;

	/* read config */
	if (ti99_model == model_99_4p)
		xRAM_kind = xRAM_kind_99_4p_1Mb;	/* hack */
	else
		xRAM_kind = (readinputport(input_port_config) >> config_xRAM_bit) & config_xRAM_mask;
	has_speech = (readinputport(input_port_config) >> config_speech_bit) & config_speech_mask;
	has_fdc = (readinputport(input_port_config) >> config_fdc_bit) & config_fdc_mask;

	/* set up optional expansion hardware */
	ti99_expansion_card_init();

	if (ti99_model == model_99_4p)
		ti99_4p_internal_dsr_init();

	if (has_speech)
	{
		spchroms_interface speech_intf = { region_speech_rom };

		spchroms_config(& speech_intf);

		install_mem_read16_handler(0, 0x9000, 0x93ff, ti99_rw_rspeech);
		install_mem_write16_handler(0, 0x9400, 0x97ff, ti99_ww_wspeech);
	}
	else
	{
		install_mem_read16_handler(0, 0x9000, 0x93ff, ti99_rw_null8bits);
		install_mem_write16_handler(0, 0x9400, 0x97ff, ti99_ww_null8bits);
	}

	switch (xRAM_kind)
	{
	case xRAM_kind_none:
	default:
		/* reset mem handler to none */
		install_mem_read16_handler(0, 0x2000, 0x3fff, ti99_rw_null8bits);
		install_mem_write16_handler(0, 0x2000, 0x3fff, ti99_ww_null8bits);
		install_mem_read16_handler(0, 0xa000, 0xffff, ti99_rw_null8bits);
		install_mem_write16_handler(0, 0xa000, 0xffff, ti99_ww_null8bits);
		break;
	case xRAM_kind_TI:
		ti99_TIxram_init();
		break;
	case xRAM_kind_super_AMS:
		ti99_sAMSxram_init();
		break;
	case xRAM_kind_99_4p_1Mb:
		ti99_4p_mapper_init();
		break;
	case xRAM_kind_foundation_128k:
	case xRAM_kind_foundation_512k:
	case xRAM_kind_myarc_128k:
	case xRAM_kind_myarc_512k:
		ti99_myarcxram_init();
		break;
	}

	if (has_fdc)
	{
		ti99_fdc_init();
	}

	if (has_evpc)
	{
		ti99_evpc_init();
	}
}

void machine_stop_ti99(void)
{
	tms9901_cleanup();
}


/*
	video initialization.
*/
int video_start_ti99_4ev(void)
{
	return v9938_init(MODEL_V9938, 0x20000, tms9901_set_int2);	/* v38 with 128 kb of video RAM */
}

/*
	VBL interrupt  (mmm... actually, it happens when the beam enters the lower border, so it is not
	a genuine VBI, but who cares ?)
*/
void ti99_vblank_interrupt(void)
{
	TMS9928A_interrupt();
}

void ti99_4ev_hblank_interrupt(void)
{
	v9938_interrupt();
}


/*===========================================================================*/
/*
	Memory handlers.

	TODO:
	* save minimem RAM when quitting
	* actually implement GRAM support and GPL port support
*/

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
	Cartridge read: same as MRA_ROM, but with an additionnal delay.
*/
READ16_HANDLER ( ti99_rw_cartmem )
{
	tms9900_ICount -= 4;

	return current_page_ptr[offset];
}

/*
	this handler handles ROM switching in cartridges
*/
WRITE16_HANDLER ( ti99_ww_cartmem )
{
	tms9900_ICount -= 4;

	if (cartridge_minimemory && offset >= 0x800)
		/* handle minimem RAM */
		COMBINE_DATA(current_page_ptr+offset);
	else if (cartridge_paged)
		/* handle pager */
		current_page_ptr = cartridge_pages[offset & 1];
}


/*---------------------------------------------------------------------------*/
/*
	memory-mapped register handlers.

Theory:

	These are registers to communicate with several peripherals.
	These registers are all 8-bit-wide, and are located at even adresses between
	0x8400 and 0x9FFE.
	The registers are identified by (addr & 1C00), and, for VDP and GPL access, by (addr & 2).
	These registers are either read-only or write-only.  (Or, more accurately,
	(addr & 4000) disables register read, whereas !(addr & 4000) disables register read.)

	Memory mapped registers list:
	- write sound chip. (8400-87FE)
	- read VDP memory. (8800-8BFC), (addr&2) == 0
	- read VDP status. (8802-8BFE), (addr&2) == 2
	- write VDP memory. (8C00-8FFC), (addr&2) == 0
	- write VDP address and VDP register. (8C02-8FFE), (addr&2) == 2
	- read speech synthesis chip. (9000-93FE)
	- write speech synthesis chip. (9400-97FE)
	- read GPL memory. (9800-9BFC), (addr&2) == 0 (1)
	- read GPL adress. (9802-9BFE), (addr&2) == 2 (1)
	- write GPL memory. (9C00-9FFC), (addr&2) == 0 (1)
	- write GPL adress. (9C02-9FFE), (addr&2) == 2 (1)

(1) on some hardware, (addr & 0x3FC) provides a GPL page number.
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
	TMS9918(A)/9928(A)/9929(A) VDP read
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
	TMS9918(A)/9928(A)/9929(A) vdp write
*/
WRITE16_HANDLER ( ti99_ww_wvdp )
{
	tms9900_ICount -= 4;

	if (offset & 1)
	{	/* write VDP address */
		TMS9928A_register_w(0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(0, (data >> 8) & 0xff);
	}
}

/*
	V38 VDP read
*/
READ16_HANDLER ( ti99_rw_rv38 )
{
	tms9900_ICount -= 4;

	if (offset & 1)
	{	/* read VDP status */
		return ((int) v9938_status_r(0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) v9938_vram_r(0)) << 8;
	}
}

/*
	V38 vdp write
*/
WRITE16_HANDLER ( ti99_ww_wv38 )
{
	tms9900_ICount -= 4;

	switch (offset & /*3*/1)
	{
	case 0:
		/* write VDP data */
		v9938_vram_w(0, (data >> 8) & 0xff);
		break;
	case 1:
		/* write VDP address */
		v9938_command_w(0, (data >> 8) & 0xff);
		break;
#if 0
	case 2:
		/* write VDP palette */
		v9938_palette_w(0, data);
		break;
	case 3:
		/* write VDP register */
		v9938_register_w(0, data);
		break;
#endif
	}
}

/*
	TMS5200 speech chip read
*/
static READ16_HANDLER ( ti99_rw_rspeech )
{
	tms9900_ICount -= 4;		/* this is just a minimum, it can be more */

	return ((int) tms5220_status_r(offset)) << 8;
}

/*
	TMS5200 speech chip write
*/
static WRITE16_HANDLER ( ti99_ww_wspeech )
{
	tms9900_ICount -= 30;		/* this is just an approx. minimum, it can be much more */

	tms5220_data_w(offset, (data >> 8) & 0xff);
}

/*
	GPL read
*/
READ16_HANDLER ( ti99_rw_rgpl )
{
	GROM_port_t *port = GPL_port_lookup[(offset & 0x1FE) >> 1];	/* GROM/GRAM can be paged */
	int reply;


	tms9900_ICount -= 4/*16*/;		/* from 4 to 16? */

	if (offset & 1)
	{	/* read GPL address */
		/* the console GROMs are always affected */
		console_GROMs.waddr_LSB = FALSE;	/* right??? */

		if (console_GROMs.raddr_LSB)
		{
			reply = (console_GROMs.addr << 8) & 0xff00;
			console_GROMs.raddr_LSB = FALSE;
		}
		else
		{
			reply = console_GROMs.addr & 0xff00;
			console_GROMs.raddr_LSB = TRUE;
		}

		if (port)
		{
			port->waddr_LSB = FALSE;

			if (port->raddr_LSB)
			{
				reply = (port->addr << 8) & 0xff00;
				port->raddr_LSB = FALSE;
			}
			else
			{
				reply = port->addr & 0xff00;
				port->raddr_LSB = TRUE;
			}
		}
	}
	else
	{	/* read GPL data */
		/* the console GROMs are always affected */
		reply = ((int) console_GROMs.buf) << 8;	/* retreive buffer */

		/* read ahead */
		console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
		console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;

		if (port)
		{
			reply = ((int) port->buf) << 8;	/* retreive buffer */

			/* read ahead */
			port->buf = port->data_ptr[port->addr];
			port->addr = ((port->addr + 1) & 0x1FFF) | (port->addr & 0xE000);
			port->raddr_LSB = port->waddr_LSB = FALSE;
		}
	}

	return reply;
}

/*
	GPL write
*/
WRITE16_HANDLER ( ti99_ww_wgpl )
{
	GROM_port_t *port = GPL_port_lookup[(offset & 0x1FE) >> 1];	/* GROM/GRAM can be paged */


	tms9900_ICount -= 4/*16*/;		/* from 4 to 16? */

	if (offset & 1)
	{	/* write GPL adress */
		/* the console GROMs are always affected */
		console_GROMs.raddr_LSB = FALSE;

		if (console_GROMs.waddr_LSB)
		{
			console_GROMs.addr = (console_GROMs.addr & 0xFF00) | ((data >> 8) & 0xFF);

			/* read ahead */
			console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
			console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);

			console_GROMs.waddr_LSB = FALSE;
		}
		else
		{
			console_GROMs.addr = (data & 0xFF00) | (console_GROMs.addr & 0xFF);
			console_GROMs.cur_flags = console_GROMs.GROM_flags[data >> 13];

			console_GROMs.waddr_LSB = TRUE;
		}

		if (port)
		{
			port->raddr_LSB = FALSE;

			if (port->waddr_LSB)
			{
				port->addr = (port->addr & 0xFF00) | ((data >> 8) & 0xFF);

				/* read ahead */
				port->buf = port->data_ptr[port->addr];
				port->addr = ((port->addr + 1) & 0x1FFF) | (port->addr & 0xE000);

				port->waddr_LSB = FALSE;
			}
			else
			{
				port->addr = (data & 0xFF00) | (port->addr & 0xFF);
				port->cur_flags = port->GROM_flags[data >> 13];

				port->waddr_LSB = TRUE;
			}
		}
	}
	else
	{	/* write GPL byte */
		/* the console GROMs are always affected */
		/* BTW, console GROMs are never GRAMs, therefore there is no need to check */
		/* read ahead */
		console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
		console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;

		if (port)
		{
			/* GRAM support is almost ready, but we need to support imitation "GROMs" with no read-ahead */
			if (port->GROM_flags[port->addr >> 13])
				port->data_ptr[port->addr] = ((data >> 8) & 0xFF);

			/* read ahead */
			port->buf = port->data_ptr[port->addr];
			port->addr = ((port->addr + 1) & 0x1FFF) | (port->addr & 0xE000);
			port->raddr_LSB = port->waddr_LSB = FALSE;
		}
	}
}



/*===========================================================================*/
/*
	TI99/4x-specific tms9901 I/O handlers

	See mess/machine/tms9901.c for generic tms9901 CRU handlers.

	BTW, although TMS9900 is generally big-endian, it is little endian as far as CRU is
	concerned. (i.e. bit 0 is the least significant)

KNOWN PROBLEMS:
	* a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is:
	  on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
	  timer mode even though the program did not explicitely ask... and THIS is impossible
	  to emulate efficiently (we would have to check every memory operation).
*/
/*
	TMS9901 interrupt handling on a TI99/4(a).

	TI99/4(a) uses the following interrupts:
	INT1: external interrupt (used by RS232 controller, for instance)
	INT2: VDP interrupt
	TMS9901 timer interrupt (overrides INT3)

	Three interrupts are used by the system (INT1, INT2, and timer), out of 15/16 possible
	interrupt.  Keyboard pins can be used as interrupt pins, too, but this is not emulated
	(it's a trick, anyway, and I don't know of any program which uses it).

	When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
	a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
	the interrupt pin and the revelant bit in the interrupt mask are set.

	TIMER interrupts are kind of an exception, since they are not associated with an external
	interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
	the pending interrupt (or am I wrong once again ?).

nota:
	All interrupt routines notify (by software) the TMS9901 of interrupt recognition ("SBO n").
	However, AFAIK, this has strictly no consequence in the TMS9901, and interrupt routines
	would work fine without this (except probably TIMER interrupt).  All this is quite weird.
	Maybe the interrupt recognition notification is needed on TMS9985, or any other weird
	variant of TMS9900 (TI990/10 with TI99 development system?).
*/

/*
	set the state of int2 (called by tms9928 core)
*/
void tms9901_set_int2(int state)
{
	tms9901_set_single_int(2, state);
}

/*
	Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static void tms9901_interrupt_callback(int intreq, int ic)
{
	if (intreq)
	{
		/* On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		 * but hard-wired to force level 1 interrupts */
		cpu_set_irq_line_and_vector(0, 0, ASSERT_LINE, 1);	/* interrupt it, baby */
	}
	else
	{
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bit 3-7: keyboard status bits 0 to 4
*/
static int ti99_R9901_0(int offset)
{
	int answer;

	answer = (readinputport(input_port_keyboard + KeyCol) << 3) & 0xF8;

	if ((ti99_model == model_99_4a) || (ti99_model == model_99_4p))
	{
		if (! AlphaLockLine)
			answer &= ~ (readinputport(input_port_caps_lock) << 3);
	}

	return (answer);
}

/*
	Read pins INT8*-INT15* of TI99's 9901.

	signification:
	 bit 0-2: keyboard status bits 5 to 7
	 bit 3-7: weird, not emulated
*/
static int ti99_R9901_1(int offset)
{
	return (readinputport(input_port_keyboard + KeyCol) >> 5) & 0x07;
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
	/*only important bit: bit 27: tape input */

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	return device_input(IO_CASSETTE, 0) > 0 ? 8 : 0;
}


/*
	WRITE column number bit 2 (P2)
*/
static void ti99_KeyC2(int offset, int data)
{
	if (data)
		KeyCol |= 1;
	else
		KeyCol &= (~1);
}

/*
	WRITE column number bit 1 (P3)
*/
static void ti99_KeyC1(int offset, int data)
{
	if (data)
		KeyCol |= 2;
	else
		KeyCol &= (~2);
}

/*
	WRITE column number bit 0 (P4)
*/
static void ti99_KeyC0(int offset, int data)
{
	if (data)
		KeyCol |= 4;
	else
		KeyCol &= (~4);
}

/*
	WRITE alpha lock line - TI99/4a only (P5)
*/
static void ti99_AlphaW(int offset, int data)
{
	if ((ti99_model == model_99_4a) || (ti99_model == model_99_4p))
		AlphaLockLine = data;
}

/*
	command CS1 tape unit motor (P6)
*/
static void ti99_CS1_motor(int offset, int data)
{
	if (data)
		device_status(IO_CASSETTE, 0, device_status(IO_CASSETTE, 0, -1) & ~ WAVE_STATUS_MOTOR_INHIBIT);
	else
		device_status(IO_CASSETTE, 0, device_status(IO_CASSETTE, 0, -1) | WAVE_STATUS_MOTOR_INHIBIT);
}

/*
	command CS2 tape unit motor (P7)
*/
static void ti99_CS2_motor(int offset, int data)
{
	if (data)
		device_status(IO_CASSETTE, 1, device_status(IO_CASSETTE, 1, -1) & ~ WAVE_STATUS_MOTOR_INHIBIT);
	else
		device_status(IO_CASSETTE, 1, device_status(IO_CASSETTE, 1, -1) | WAVE_STATUS_MOTOR_INHIBIT);
}

/*
	audio gate (P8)

	connected to the AUDIO IN pin of TMS9919

	set to 1 before using tape (in order not to burn the TMS9901 ??)
	Must actually control the mixing of tape sound with computer sound.

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
	tape output (P9)
	I think polarity is correct, but don't take my word for it.
*/
static void ti99_CS_output(int offset, int data)
{
	device_output(IO_CASSETTE, 0, data ? 32767 : -32767);
	device_output(IO_CASSETTE, 1, data ? 32767 : -32767);
}



/*===========================================================================*/
/*
	Extension card support code

In short:
	16 CRU address intervals are reserved for expansion.  I appended known TI peripherals which
	use each port (appended '???' when I don't know what the peripheral is :-) ).
	* 0x1000-0x10FE "For test equipment use on production line"
	* 0x1100-0x11FE disk controller
	* 0x1200-0x12FE modem???
	* 0x1300-0x13FE RS232 1&2, PIO 1
	* 0x1400-0x14FE unassigned
	* 0x1500-0x15FE RS232 3&4, PIO 2
	* 0x1600-0x16FE unassigned
	* 0x1700-0x17FE hex-bus (prototypes)
	* 0x1800-0x18FE thermal printer (early peripheral)
	* 0x1900-0x19FE EPROM programmer??? (Mezzanine board in July 1981's TI99/7 prototype)
	* 0x1A00-0x1AFE unassigned
	* 0x1B00-0x1BFE TI GPL debugger card
	* 0x1C00-0x1CFE Video Controller Card (Possibly the weirdest device.  The card is connected to
		the computer video output, a VCR video output, and a monitor.  It can control the VCR,
		connect the display to either VCR output or computer video output, and it can read or save
		binary data to video tape.  I think it can act as a genlock interface (i.e. TMS9918
		transparent background shows the video signal), too, but I am not sure.)
	* 0x1D00-0x1DFE IEEE 488 Controller Card (parallel port, schematics on ftp.whtech.com)
	* 0x1E00-0x1EFE unassigned
	* 0x1F00-0x1FFE P-code card

	Known mappings for 3rd party cards:
	* Horizon RAMdisk: any ports from 0 to 7 (port 0 is most common).
	* Myarc 128k/512k (RAM and DSR ROM): port 0 (0x1000-0x11FE)
	* Corcomp 512k (RAM and DSR ROM): port unknown
	* Super AMS 128k/512k: port 14 (0x1E00-0x1EFE)
	* Foundation 128k/512k: port 14 (0x1E00-0x1EFE)
	* Gram Karte: any port (0-15)
	* EVPC (video card): 0x1400-0x14FE
	* HSGPL (GROM replacement): 0x1B00-0x1BFE

	Of course, these devices additionnally need some support routines, and possibly memory-mapped
	registers.  To do this, memory range 0x4000-5FFF is shared by all cards.  The system enables
	each card as needed by writing a 1 to the first CRU bit: when this happens the card can safely
	enable its ROM, RAM, memory-mapped registers, etc.  Provided the ROM uses the proper ROM header,
	the system will recognize and call peripheral I/O functions, interrupt service routines,
	and startup routines (all these routines are generally called DSR = Device Service Routine).

	Also, the cards can trigger level-1 ("INTA") interrupts.  (A LOAD interrupt is possible, too,
	but it was never used by TI, AFAIK.  Also, an interrupt line called "INTB" is found on several
	cards built by TI, but it is not connected: I am quite unsure whether it was intended to be
	connected to the LOAD line, to the INT2 line of TMS9985, or it was only to be used with the
	TI99/4(a) development system running on a TI990/10.  This is not the only unused feature:
	cards built by TI support sensing of an interrupt status register and 19-bit addressing,
	even though TI99/4(A) does not take advantage of these feature.  TI documentation tells that
	some of these features are used by the "Personal Computer PCC at Texas Instrument",
	which is not very explicit (What on earth does "PCC" stand for?).  Maybe it refers to
	the TI99/4(a) development system running on a TI990/10, or to the TI99/7 and TI99/8 prototypes.

	Note that I use 8-bit handlers.  Obviously, using 16-bit handlers would be equivalent and
	quite faster, but I wanted to be able to interface the extension cards to a future 99/8
	emulator (which was possible with several PEB boards).

	Only the floppy disk controller card is supported for now.
*/

typedef int (*cru_read_handler)(int offset);
typedef void (*cru_write_handler)(int offset, int data);

typedef struct expansion_port_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	mem_read_handler mem_read;		/* card mem read handler (8 bits) */
	mem_write_handler mem_write;	/* card mem write handler (8 bits) */
} expansion_port_t;

/* handlers for each of 16 slots */
static expansion_port_t expansion_ports[16];

/* expansion port for the 99/4p system, which supports dynamical 16-bit accesses */
/* (TI did not design this!) */
typedef struct ti99_4p_expansion_port_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	enum {width_8bit = 0, width_16bit} width;
	union
	{
		struct
		{
			mem_read_handler mem_read;		/* card mem read handler (8 bits) */
			mem_write_handler mem_write;	/* card mem write handler (8 bits) */
		} width_8bit;
		struct
		{
			mem_read16_handler mem_read;		/* card mem read handler (8 bits) */
			mem_write16_handler mem_write;	/* card mem write handler (8 bits) */
		} width_16bit;
	} w;
} ti99_4p_expansion_port_t;

typedef struct ti99_4p_16bit_expansion_port_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	mem_read16_handler mem_read;	/* card mem read handler (16 bits) */
	mem_write16_handler mem_write;	/* card mem write handler (16 bits) */
} ti99_4p_16bit_expansion_port_t;

/* handlers for each of 28 slots */
static ti99_4p_expansion_port_t ti99_4p_expansion_ports[28];

/* index of the currently active card (-1 if none) */
static int active_card = -1;

/* ila: inta status register (not actually used on TI99/4(A), but still present) */
static int ila;
/* ilb: intb status register (completely pointless on TI99/4(A), as neither ilb nor senilb are
connected) */
static int ilb;

/* only supported by ti99/4p */
static int senila, senilb;

/* masks for ila and ilb (from actual ILA and ILB registers) */
enum
{
	inta_rs232_1_bit = 0,
	inta_rs232_2_bit = 1,
	inta_rs232_3_bit = 4,
	inta_rs232_4_bit = 5,

	/*inta_rs232_1_mask = (0x80 >> inta_rs232_1_bit),
	inta_rs232_2_mask = (0x80 >> inta_rs232_2_bit),
	inta_rs232_3_mask = (0x80 >> inta_rs232_3_bit),
	inta_rs232_4_mask = (0x80 >> inta_rs232_4_bit),*/

	intb_fdc_bit     = 0,
	intb_ieee488_bit = 1
};

/* hack to simulate TMS9900 byte write */
static int tmp_buffer;


/*
	Resets the expansion card handlers
*/
static void ti99_expansion_card_init(void)
{
	memset(expansion_ports, 0, sizeof(expansion_ports));

	active_card = -1;
	ila = 0;
	ilb = 0;
}

/*
	Sets the handlers for one expansion port

	cru_base: CRU base address (any of 0x1000, 0x1100, 0x1200, ..., 0x1F00)
*/
static void ti99_set_expansion_card_handlers(int cru_base, const expansion_port_t *handler)
{
	int port;

	if (cru_base & 0xff)
		return;

	if (ti99_model == model_99_4p)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			ti99_4p_expansion_ports[port].width = width_8bit;
			ti99_4p_expansion_ports[port].w.width_8bit.mem_read = handler->mem_read;
			ti99_4p_expansion_ports[port].w.width_8bit.mem_write = handler->mem_write;
		}
	}
	else
	{
		port = (cru_base - 0x1000) >> 8;

		if ((port>=0) && (port<16))
		{
			expansion_ports[port] = *handler;
		}
	}
}

/*
	Sets the handlers for one expansion port

	cru_base: CRU base address (any of 0x1000, 0x1100, 0x1200, ..., 0x1F00)
*/
static void ti99_4p_set_16bit_expansion_card_handlers(int cru_base, const ti99_4p_16bit_expansion_port_t *handler)
{
	int port;

	if (cru_base & 0xff)
		return;

	if (ti99_model == model_99_4p)
	{
		port = (cru_base - 0x0400) >> 8;

		if ((port>=0) && (port<28))
		{
			ti99_4p_expansion_ports[port].cru_read = handler->cru_read;
			ti99_4p_expansion_ports[port].cru_write = handler->cru_write;
			ti99_4p_expansion_ports[port].width = width_16bit;
			ti99_4p_expansion_ports[port].w.width_16bit.mem_read = handler->mem_read;
			ti99_4p_expansion_ports[port].w.width_16bit.mem_write = handler->mem_write;
		}
	}
}

/*
	Update ila status register and asserts or clear interrupt line accordingly.

	bit: bit number [0,7]
	state: 1 to assert bit, 0 to clear
*/
static void set_ila_bit(int bit, int state)
{
	if (state)
	{
		ila |= 0x80 >> bit;
		tms9901_set_single_int(1, 1);
	}
	else
	{
		ila &= ~(0x80 >> bit);
		if (! ila)
			tms9901_set_single_int(1, 0);
	}
}

/*
	Update ilb status register.

	bit: bit number [0,7]
	state: 1 to assert bit, 0 to clear
*/
static void set_ilb_bit(int bit, int state)
{
	if (state)
		ilb |= 0x80 >> bit;
	else
		ilb &= ~(0x80 >> bit);
}


/*
	Read CRU in range >1000->1ffe (>800->8ff)
*/
READ16_HANDLER ( ti99_expansion_CRU_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = (offset & 0x00f0) >> 4;
	handler = expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >1000->1ffe (>800->8ff)
*/
WRITE16_HANDLER ( ti99_expansion_CRU_w )
{
	int port;
	cru_write_handler handler;

	port = (offset & 0x0780) >> 7;
	handler = expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
		}
		else
		{
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
	Read mem in range >4000->5ffe
*/
READ16_HANDLER ( ti99_rw_expansion )
{
	int reply = 0;
	mem_read_handler handler;

	tms9900_ICount -= 4;

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_read;
		if (handler)
		{
			reply = (*handler)((offset << 1) + 1);
			reply |= ((unsigned) (*handler)(offset << 1)) << 8;
		}
	}

	return tmp_buffer = reply;
}

/*
	Write mem in range >4000->5ffe
*/
WRITE16_HANDLER ( ti99_ww_expansion )
{
	mem_write_handler handler;

	tms9900_ICount -= 4;

	/* simulate byte write */
	data = (tmp_buffer & mem_mask) | (data & ~mem_mask);

	if (active_card != -1)
	{
		handler = expansion_ports[active_card].mem_write;
		if (handler)
		{
			(*handler)((offset << 1) + 1, data & 0xff);
			(*handler)(offset << 1, (data >> 8) & 0xff);
		}
	}
}


/*
	Read CRU in range >0400->1ffe (>200->8ff)
*/
READ16_HANDLER ( ti99_4p_expansion_CRU_r )
{
	int port;
	cru_read_handler handler;
	int reply;

	port = offset >> 4;
	handler = ti99_4p_expansion_ports[port].cru_read;

	reply = (handler) ? (*handler)(offset & 0xf) : 0;

	return reply;
}

/*
	Write CRU in range >0400->1ffe (>200->8ff)
*/
WRITE16_HANDLER ( ti99_4p_expansion_CRU_w )
{
	int port;
	cru_write_handler handler;

	port = offset >> 7;
	handler = ti99_4p_expansion_ports[port].cru_write;

	if (handler)
		(*handler)(offset & 0x7f, data);

	/* expansion card enable? */
	if ((offset & 0x7f) == 0)
	{
		if (data & 1)
		{
			/* enable */
			active_card = port;
		}
		else
		{
			if (port == active_card)	/* geez... who cares? */
			{
				active_card = -1;			/* no port selected */
			}
		}
	}
}

/*
	Read mem in range >4000->5ffe
*/
READ16_HANDLER ( ti99_4p_rw_expansion )
{
	int reply = 0;
	mem_read_handler handler;
	mem_read16_handler handler16;


	if (active_card == -1)
		tms9900_ICount -= 4;	/* ??? */
	else
	{

		if (ti99_4p_expansion_ports[active_card].width == width_8bit)
		{
			tms9900_ICount -= 4;

			handler = ti99_4p_expansion_ports[active_card].w.width_8bit.mem_read;
			if (handler)
			{
				reply = (*handler)((offset << 1) + 1);
				reply |= ((unsigned) (*handler)(offset << 1)) << 8;
			}
		}
		else
		{
			tms9900_ICount -= 1;	/* ??? */

			handler16 = ti99_4p_expansion_ports[active_card].w.width_16bit.mem_read;
			if (handler16)
				reply = (*handler16)(offset, /*mem_mask*/0);
		}
	}

	if (senila || senilb)
	{
		if ((active_card != -1) || (senila && senilb))
			/* ah, the smell of burnt silicon... */
			logerror("<Scrrrrr>: your computer has just burnt (maybe).\n");

		if (senila)
			reply = ila | (ila << 8);
		else if (senilb)
			reply = ilb | (ilb << 8);
	}

	return tmp_buffer = reply;
}

/*
	Write mem in range >4000->5ffe
*/
WRITE16_HANDLER ( ti99_4p_ww_expansion )
{
	mem_write_handler handler;
	mem_write16_handler handler16;

	if (active_card == -1)
		tms9900_ICount -= 4;	/* ??? */
	else
	{
		/* simulate byte write */
		data = (tmp_buffer & mem_mask) | (data & ~mem_mask);

		if (ti99_4p_expansion_ports[active_card].width == width_8bit)
		{
			tms9900_ICount -= 4;

			handler = ti99_4p_expansion_ports[active_card].w.width_8bit.mem_write;
			if (handler)
			{
				(*handler)((offset << 1) + 1, data & 0xff);
				(*handler)(offset << 1, (data >> 8) & 0xff);
			}
		}
		else
		{
			tms9900_ICount -= 1;	/* ??? */

			handler16 = ti99_4p_expansion_ports[active_card].w.width_16bit.mem_write;
			if (handler16)
				(*handler16)(offset, data, /*mem_mask*/0);
		}
	}
}


/*===========================================================================*/
/*
	TI99/4p internal DSR support.

	Includes a few specific signals, and an extra ROM.
*/

/* prototypes */
static void ti99_4p_internal_dsr_cru_w(int offset, int data);
static READ16_HANDLER(ti99_4p_rw_internal_dsr);


static const ti99_4p_16bit_expansion_port_t ti99_4p_internal_dsr_handlers =
{
	NULL,
	ti99_4p_internal_dsr_cru_w,
	ti99_4p_rw_internal_dsr,
	NULL
};

/* pointer to the internal DSR ROM data */
static UINT16 *ti99_4p_internal_DSR;

static int internal_rom6_enable;


/* set up handlers, and set initial state */
static void ti99_4p_internal_dsr_init(void)
{
	ti99_4p_internal_DSR = (UINT16 *) (memory_region(REGION_CPU1) + offset_rom4_4p);

	ti99_4p_set_16bit_expansion_card_handlers(0x0f00, & ti99_4p_internal_dsr_handlers);

	internal_rom6_enable = 0;
	senila = 0;
	senilb = 0;
}

/* write CRU bit:
	bit0: enable/disable internal DSR ROM,
	bit1: enable/disable internal cartridge ROM
	bit2: set/clear senila
	bit3: set/clear senilb*/
static void ti99_4p_internal_dsr_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 1:
		internal_rom6_enable = data;
		/* ... */
		break;
	case 2:
		senila = data;
		break;
	case 3:
		senilb = data;
		break;
	case 4:
		/* 0: 16-bit (fast) memory timings */
		/* 1: 8-bit memory timings */
		break;
	case 5:
		/* if 0: "KBENA" mode (enable keyboard?) */
		/* if 1: "KBINH" mode (inhibit keyboard?) */
		break;
	}
}

/* read internal DSR ROM */
static READ16_HANDLER(ti99_4p_rw_internal_dsr)
{
	return ti99_4p_internal_DSR[offset];
}


/*===========================================================================*/
/*
	TI memory extension support.

	Simple 8-bit DRAM: 32kb in two chunks of 8kb and 24kb repectively.
	Since the RAM is on the 8-bit bus, there is an additionnal delay.
*/

static READ16_HANDLER ( ti99_rw_TIxramlow );
static WRITE16_HANDLER ( ti99_ww_TIxramlow );
static READ16_HANDLER ( ti99_rw_TIxramhigh );
static WRITE16_HANDLER ( ti99_ww_TIxramhigh );

static void ti99_TIxram_init(void)
{
	install_mem_read16_handler(0, 0x2000, 0x3fff, ti99_rw_TIxramlow);
	install_mem_write16_handler(0, 0x2000, 0x3fff, ti99_ww_TIxramlow);
	install_mem_read16_handler(0, 0xa000, 0xffff, ti99_rw_TIxramhigh);
	install_mem_write16_handler(0, 0xa000, 0xffff, ti99_ww_TIxramhigh);
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_rw_TIxramlow )
{
	tms9900_ICount -= 4;

	return xRAM_ptr[offset];
}

static WRITE16_HANDLER ( ti99_ww_TIxramlow )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(xRAM_ptr + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_rw_TIxramhigh )
{
	tms9900_ICount -= 4;

	return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_ww_TIxramhigh )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(xRAM_ptr + offset+0x1000);
}


/*===========================================================================*/
/*
	Super AMS memory extension support.

	Up to 1Mb of SRAM.  Straightforward mapper, works with 4kb chunks.  The mapper was designed
	to be extendable with other RAM areas.
*/

/* prototypes */
static void sAMS_cru_w(int offset, int data);
static READ_HANDLER(sAMS_mapper_r);
static WRITE_HANDLER(sAMS_mapper_w);

static READ16_HANDLER ( ti99_rw_sAMSxramlow );
static WRITE16_HANDLER ( ti99_ww_sAMSxramlow );
static READ16_HANDLER ( ti99_rw_sAMSxramhigh );
static WRITE16_HANDLER ( ti99_ww_sAMSxramhigh );


static const expansion_port_t sAMS_expansion_handlers =
{
	NULL,
	sAMS_cru_w,
	sAMS_mapper_r,
	sAMS_mapper_w
};


static int sAMS_mapper_on;
static int sAMSlookup[16];


/* set up super AMS handlers, and set initial state */
static void ti99_sAMSxram_init(void)
{
	int i;


	install_mem_read16_handler(0, 0x2000, 0x3fff, ti99_rw_sAMSxramlow);
	install_mem_write16_handler(0, 0x2000, 0x3fff, ti99_ww_sAMSxramlow);
	install_mem_read16_handler(0, 0xa000, 0xffff, ti99_rw_sAMSxramhigh);
	install_mem_write16_handler(0, 0xa000, 0xffff, ti99_ww_sAMSxramhigh);

	ti99_set_expansion_card_handlers(0x1e00, & sAMS_expansion_handlers);

	sAMS_mapper_on = 0;

	for (i=0; i<16; i++)
		sAMSlookup[i] = i << 1;
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void sAMS_cru_w(int offset, int data)
{
	if (offset == 1)
		sAMS_mapper_on = data;
}

/* read a mapper register */
static READ_HANDLER(sAMS_mapper_r)
{
	return (sAMSlookup[(offset >> 1) & 0xf] >> 11);
}

/* write a mapper register */
static WRITE_HANDLER(sAMS_mapper_w)
{
	sAMSlookup[(offset >> 1) & 0xf] = ((int) data) << 11;
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_rw_sAMSxramlow )
{
	tms9900_ICount -= 4;

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]];
	else
		return xRAM_ptr[offset];
}

static WRITE16_HANDLER ( ti99_ww_sAMSxramlow )
{
	tms9900_ICount -= 4;

	if (sAMS_mapper_on)
		COMBINE_DATA(xRAM_ptr + (offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]);
	else
		COMBINE_DATA(xRAM_ptr + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_rw_sAMSxramhigh )
{
	tms9900_ICount -= 4;

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x5000+offset)>>11]];
	else
		return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_ww_sAMSxramhigh )
{
	tms9900_ICount -= 4;

	if (sAMS_mapper_on)
		COMBINE_DATA(xRAM_ptr + (offset&0x7ff)+sAMSlookup[(0x5000+offset)>>11]);
	else
		COMBINE_DATA(xRAM_ptr + offset+0x1000);
}


/*===========================================================================*/
/*
	TI99/4p Super AMS clone support.  Compatible with Super AMS, but uses a 16-bit bus.

	Up to 1Mb of SRAM.  Straightforward mapper, works with 4kb chunks.
*/

/* prototypes */
static void ti99_4p_mapper_cru_w(int offset, int data);
static READ16_HANDLER(ti99_4p_rw_mapper);
static WRITE16_HANDLER(ti99_4p_ww_mapper);


static const ti99_4p_16bit_expansion_port_t ti99_4p_mapper_handlers =
{
	NULL,
	ti99_4p_mapper_cru_w,
	ti99_4p_rw_mapper,
	ti99_4p_ww_mapper
};


static int ti99_4p_mapper_on;
static int ti99_4p_mapper_lookup[16];


/* set up handlers, and set initial state */
static void ti99_4p_mapper_init(void)
{
	int i;

	/* Not required at run-time */
	/*install_mem_read16_handler(0, 0x2000, 0x2fff, MRA16_BANK3);
	install_mem_write16_handler(0, 0x2000, 0x2fff, MWA16_BANK3);
	install_mem_read16_handler(0, 0x3000, 0x3fff, MRA16_BANK4);
	install_mem_write16_handler(0, 0x3000, 0x3fff, MWA16_BANK4);
	install_mem_read16_handler(0, 0xa000, 0xafff, MRA16_BANK5);
	install_mem_write16_handler(0, 0xa000, 0xafff, MWA16_BANK5);
	install_mem_read16_handler(0, 0xb000, 0xbfff, MRA16_BANK6);
	install_mem_write16_handler(0, 0xb000, 0xbfff, MWA16_BANK6);
	install_mem_read16_handler(0, 0xc000, 0xcfff, MRA16_BANK7);
	install_mem_write16_handler(0, 0xc000, 0xcfff, MWA16_BANK7);
	install_mem_read16_handler(0, 0xd000, 0xdfff, MRA16_BANK8);
	install_mem_write16_handler(0, 0xd000, 0xdfff, MWA16_BANK8);
	install_mem_read16_handler(0, 0xe000, 0xefff, MRA16_BANK9);
	install_mem_write16_handler(0, 0xe000, 0xefff, MWA16_BANK9);
	install_mem_read16_handler(0, 0xf000, 0xffff, MRA16_BANK10);
	install_mem_write16_handler(0, 0xf000, 0xffff, MWA16_BANK10);*/

	ti99_4p_set_16bit_expansion_card_handlers(0x1e00, & ti99_4p_mapper_handlers);

	ti99_4p_mapper_on = 0;

	for (i=0; i<16; i++)
	{
		ti99_4p_mapper_lookup[i] = i << 11;

		/* update bank base */
		switch (i)
		{
		case 2:
		case 3:
			cpu_setbank(3+(i-2), xRAM_ptr + (i<<11));
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			cpu_setbank(5+(i-10), xRAM_ptr + (i<<11));
			break;
		}
	}
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void ti99_4p_mapper_cru_w(int offset, int data)
{
	int i;

	if (offset == 1)
	{
		if (ti99_4p_mapper_on != data)
		{
			ti99_4p_mapper_on = data;

			for (i=0; i<16; i++)
			{
				/* update bank base */
				switch (i)
				{
				case 2:
				case 3:
					cpu_setbank(3+(i-2), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
					break;

				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
					cpu_setbank(5+(i-10), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
					break;
				}
			}
		}
	}

}

/* read a mapper register */
static READ16_HANDLER(ti99_4p_rw_mapper)
{
	return (ti99_4p_mapper_lookup[offset & 0xf] >> 3);
}

/* write a mapper register */
static WRITE16_HANDLER(ti99_4p_ww_mapper)
{
	int page = offset & 0xf;

	ti99_4p_mapper_lookup[page] = (data & 0xff00) << 3;

	if (ti99_4p_mapper_on)
	{
		/* update bank base */
		switch (page)
		{
		case 2:
		case 3:
			cpu_setbank(3+(page-2), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			cpu_setbank(5+(page-10), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;
		}
	}
}


/*===========================================================================*/
/*
	myarc-ish memory extension support (foundation, and myarc clones).

	Up to 512kb of RAM.  Straightforward mapper, works with 32kb chunks.
*/

static int myarc_cru_r(int offset);
static void myarc_cru_w(int offset, int data);

static READ16_HANDLER ( ti99_rw_myarcxramlow );
static WRITE16_HANDLER ( ti99_ww_myarcxramlow );
static READ16_HANDLER ( ti99_rw_myarcxramhigh );
static WRITE16_HANDLER ( ti99_ww_myarcxramhigh );


static const expansion_port_t myarc_expansion_handlers =
{
	myarc_cru_r,
	myarc_cru_w,
	NULL,
	NULL
};


static int myarc_cur_page_offset;
static int myarc_page_offset_mask;


/* set up myarc handlers, and set initial state */
static void ti99_myarcxram_init(void)
{
	install_mem_read16_handler(0, 0x2000, 0x3fff, ti99_rw_myarcxramlow);
	install_mem_write16_handler(0, 0x2000, 0x3fff, ti99_ww_myarcxramlow);
	install_mem_read16_handler(0, 0xa000, 0xffff, ti99_rw_myarcxramhigh);
	install_mem_write16_handler(0, 0xa000, 0xffff, ti99_ww_myarcxramhigh);

	switch (xRAM_kind)
	{
	case xRAM_kind_foundation_128k:	/* 128kb foundation */
	case xRAM_kind_myarc_128k:		/* 128kb myarc clone */
		myarc_page_offset_mask = 0x0c000;
		break;
	case xRAM_kind_foundation_512k:	/* 512kb foundation */
	case xRAM_kind_myarc_512k:		/* 512kb myarc clone */
		myarc_page_offset_mask = 0x3c000;
		break;
	default:
		break;	/* let's just keep GCC's big mouth shut */
	}

	switch (xRAM_kind)
	{
	case xRAM_kind_foundation_128k:	/* 128kb foundation */
	case xRAM_kind_foundation_512k:	/* 512kb foundation */
		ti99_set_expansion_card_handlers(0x1e00, & myarc_expansion_handlers);
		break;
	case xRAM_kind_myarc_128k:		/* 128kb myarc clone */
	case xRAM_kind_myarc_512k:		/* 512kb myarc clone */
		ti99_set_expansion_card_handlers(0x1000, & myarc_expansion_handlers);
		ti99_set_expansion_card_handlers(0x1900, & myarc_expansion_handlers);
		break;
	default:
		break;	/* let's just keep GCC's big mouth shut */
	}

	myarc_cur_page_offset = 0;
}

/* read CRU bit:
	bit 1-2 (128kb) or 1-4 (512kb): read current map offset */
static int myarc_cru_r(int offset)
{
	/*if (offset == 0)*/	/* right??? */
	{
		return (myarc_cur_page_offset >> 14);
	}
}

/* write CRU bit:
	bit 1-2 (128kb) or 1-4 (512kb): write map offset */
static void myarc_cru_w(int offset, int data)
{
	offset &= 0x7;	/* right??? */
	if (offset >= 1)
	{
		int mask = 1 << (offset-1+14);

		if (data)
			myarc_cur_page_offset |= mask;
		else
			myarc_cur_page_offset &= ~mask;

		myarc_cur_page_offset &= myarc_page_offset_mask;
	}
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_rw_myarcxramlow )
{
	tms9900_ICount -= 4;

	return xRAM_ptr[myarc_cur_page_offset + offset];
}

static WRITE16_HANDLER ( ti99_ww_myarcxramlow )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(xRAM_ptr + myarc_cur_page_offset + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_rw_myarcxramhigh )
{
	tms9900_ICount -= 4;

	return xRAM_ptr[myarc_cur_page_offset + offset+0x1000];
}

static WRITE16_HANDLER ( ti99_ww_myarcxramhigh )
{
	tms9900_ICount -= 4;

	COMBINE_DATA(xRAM_ptr + myarc_cur_page_offset + offset+0x1000);
}


/*===========================================================================*/
/*
	TI99/4(a) Floppy disk controller card emulation
*/

/* prototypes */
static void fdc_callback(int);
static void motor_on_timer_callback(int dummy);
static int fdc_cru_r(int offset);
static void fdc_cru_w(int offset, int data);
static READ_HANDLER(fdc_mem_r);
static WRITE_HANDLER(fdc_mem_w);

/* pointer to the fdc ROM data */
static UINT8 *ti99_disk_DSR;

/* when TRUE the CPU is halted while DRQ/IRQ are true */
static int DSKhold;

/* defines for DRQ_IRQ_status */
enum
{
	fdc_IRQ = 1,
	fdc_DRQ = 2
};

/* current state of the DRQ and IRQ lines */
static int DRQ_IRQ_status;

/* disk selection bits */
static int DSEL;
/* currently selected disk unit */
static int DSKnum;
/* current side */
static int DSKside;

/* 1 if motor is on */
static int DVENA;
/* on rising edge, sets DVENA for 4.23 seconds on rising edge */
static int motor_on;
/* count 4.23s from rising edge of motor_on */
void *motor_on_timer;

static const expansion_port_t fdc_handlers =
{
	fdc_cru_r,
	fdc_cru_w,
	fdc_mem_r,
	fdc_mem_w
};


/*
	Reset fdc card, set up handlers
*/
static void ti99_fdc_init(void)
{
	ti99_disk_DSR = memory_region(region_dsr) + offset_fdc_dsr;

	DSEL = 0;
	DSKnum = -1;
	DSKside = 0;

	DVENA = 0;
	motor_on = 0;
	//motor_on_timer = NULL;
	motor_on_timer = timer_alloc(motor_on_timer_callback);

	ti99_set_expansion_card_handlers(0x1100, & fdc_handlers);

	wd179x_init(WD_TYPE_179X, fdc_callback);		/* initialize the floppy disk controller */
	wd179x_set_density(DEN_FM_LO);
}


/*
	call this when the state of DSKhold or DRQ/IRQ or DVENA change

	Emulation is faulty because the CPU is actually stopped in the midst of instruction, at
	the end of the memory access
*/
static void fdc_handle_hold(void)
{
	if (DSKhold && (!DRQ_IRQ_status) && DVENA)
		cpu_set_halt_line(0, ASSERT_LINE);
	else
		cpu_set_halt_line(0, CLEAR_LINE);
}

/*
	callback called whenever DRQ/IRQ state change
*/
static void fdc_callback(int event)
{
	switch (event)
	{
	case WD179X_IRQ_CLR:
		DRQ_IRQ_status &= ~fdc_IRQ;
		set_ilb_bit(intb_fdc_bit, 0);
		break;
	case WD179X_IRQ_SET:
		DRQ_IRQ_status |= fdc_IRQ;
		set_ilb_bit(intb_fdc_bit, 1);
		break;
	case WD179X_DRQ_CLR:
		DRQ_IRQ_status &= ~fdc_DRQ;
		break;
	case WD179X_DRQ_SET:
		DRQ_IRQ_status |= fdc_DRQ;
		break;
	}

	fdc_handle_hold();
}

/*
	callback called at the end of DVENA pulse
*/
static void motor_on_timer_callback(int dummy)
{
	DVENA = 0;
	fdc_handle_hold();
}

/*
	Read disk CRU interface

	bit 0: HLD pin
	bit 1-3: drive n active
	bit 4: 0: motor strobe on
	bit 5: always 0
	bit 6: always 1
	bit 7: selected side
*/
static int fdc_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	case 0:
		reply = 0x40;
		if (DSKside)
			reply |= 0x80;
		if (DVENA)
			reply |= (DSEL << 1);
		if (! DVENA)
			reply |= 0x10;
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}

/*
	Write disk CRU interface
*/
static void fdc_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		/* WRITE to DISK DSR ROM bit (bit 0) */
		/* handled in ti99_expansion_CRU_w() */
		break;

	case 1:
		/* Strobe motor (motor is on for 4.23 sec) */
		if (data && !motor_on)
		{	/* on rising edge, set DVENA for 4.23s */
			DVENA = 1;
			fdc_handle_hold();
			timer_adjust(motor_on_timer, 4.23, 0, 0.);
		}
		motor_on = data;
		break;

	case 2:
		/* Set disk ready/hold (bit 2)
			0: ignore IRQ and DRQ
			1: TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates
			  for 4.23s after write to revelant CRU bit, this is not emulated and could cause
			  the TI99 to lock...) */
		DSKhold = data;
		fdc_handle_hold();
		break;

	case 3:
		/* Load disk heads (HLT pin) (bit 3) */
		/* ... */
		break;

	case 4:
	case 5:
	case 6:
		/* Select drive X (bits 4-6) */
		{
			int drive = offset-4;					/* drive # (0-2) */

			if (data)
			{
				DSEL |= 1 << drive;

				if (drive != DSKnum)			/* turn on drive... already on ? */
				{
					DSKnum = drive;

					wd179x_set_drive(DSKnum);
					wd179x_set_side(DSKside);
				}
			}
			else
			{
				DSEL &= ~ (1 << drive);

				if (drive == DSKnum)			/* geez... who cares? */
				{
					DSKnum = -1;				/* no drive selected */
				}
			}
		}
		break;

	case 7:
		/* Select side of disk (bit 7) */
		DSKside = data;
		wd179x_set_side(DSKside);
		break;
	}
}


/*
	read a byte in disk DSR space
*/
static READ_HANDLER(fdc_mem_r)
{
	switch (offset)
	{
	case 0x1FF0:					/* Status register */
		return (wd179x_status_r(offset) ^ 0xFF);
		break;
	case 0x1FF2:					/* Track register */
		return wd179x_track_r(offset) ^ 0xFF;
		break;
	case 0x1FF4:					/* Sector register */
		return wd179x_sector_r(offset) ^ 0xFF;
		break;
	case 0x1FF6:					/* Data register */
		return wd179x_data_r(offset) ^ 0xFF;
		break;
	default:						/* DSR ROM */
		return ti99_disk_DSR[offset];
		break;
	}
}

/*
	write a byte in disk DSR space
*/
static WRITE_HANDLER(fdc_mem_w)
{
	data ^= 0xFF;	/* inverted data bus */

	switch (offset)
	{
	case 0x1FF8:					/* Command register */
		wd179x_command_w(offset, data);
		break;
	case 0x1FFA:					/* Track register */
		wd179x_track_w(offset, data);
		break;
	case 0x1FFC:					/* Sector register */
		wd179x_sector_w(offset, data);
		break;
	case 0x1FFE:					/* Data register */
		wd179x_data_w(offset, data);
		break;
	}
}


/*===========================================================================*/
/*
	Thierry Nouspikel's IDE card emulation
*/

/* prototypes */
static int ide_cru_r(int offset);
static void ide_cru_w(int offset, int data);
static READ_HANDLER(ide_mem_r);
static WRITE_HANDLER(ide_mem_w);

/* pointer to the IDE SRAM area */
/* static UINT8 *ti99_ide_SRAM; */

static const expansion_port_t ide_handlers =
{
	ide_cru_r,
	ide_cru_w,
	ide_mem_r,
	ide_mem_w
};


/*
	Reset ide card, set up handlers
*/
static void ti99_ide_init(void)
{
	/*ti99_ide_SRAM = memory_region(region_dsr) + offset_ide_sram;*/

	/*ti99_set_expansion_card_handlers(0x1100, & ide_handlers);*/
}

/*
	Read ide CRU interface
*/
static int ide_cru_r(int offset)
{
	int reply;

	switch (offset)
	{
	default:
		reply = 0;
		break;
	}

	return reply;
}

/*
	Write ide CRU interface
*/
static void ide_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		break;

	case 1:
		break;

	case 2:
		break;

	case 3:
		break;

	case 4:
		break;

	case 5:
		break;

	case 6:
		break;

	case 7:
		break;
	}
}


/*
	read a byte in ide DSR space
*/
static READ_HANDLER(ide_mem_r)
{
	switch (offset)
	{
	default:
		return 0;
	}
}

/*
	write a byte in ide DSR space
*/
static WRITE_HANDLER(ide_mem_w)
{
	switch (offset)
	{
	}
}

/*===========================================================================*/
/*
	SNUG's EVPC emulation
*/

/* prototypes */
static int evpc_cru_r(int offset);
static void evpc_cru_w(int offset, int data);
static READ_HANDLER(evpc_mem_r);
static WRITE_HANDLER(evpc_mem_w);

/* pointer to the evpc DSR area */
static UINT8 *ti99_evpc_DSR;

/* pointer to the evpc novram area */
/*static UINT8 *ti99_evpc_novram;*/

static int RAMEN;

static int evpc_dsr_page;

static const expansion_port_t evpc_handlers =
{
	evpc_cru_r,
	evpc_cru_w,
	evpc_mem_r,
	evpc_mem_w
};


/*
	Reset evpc card, set up handlers
*/
static void ti99_evpc_init(void)
{
	ti99_evpc_DSR = memory_region(region_dsr) + offset_evpc_dsr;

	RAMEN = 0;
	evpc_dsr_page = 0;

	ti99_set_expansion_card_handlers(0x1400, & evpc_handlers);
}

/*
	Read evpc CRU interface
*/
static int evpc_cru_r(int offset)
{
	return 0;	/* dip-switch value */
}

/*
	Write evpc CRU interface
*/
static void evpc_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		break;

	case 1:
		if (data)
			evpc_dsr_page |= 1;
		else
			evpc_dsr_page &= ~1;
		break;

	case 2:
		break;

	case 3:
		RAMEN = data;
		break;

	case 4:
		if (data)
			evpc_dsr_page |= 4;
		else
			evpc_dsr_page &= ~4;
		break;

	case 5:
		if (data)
			evpc_dsr_page |= 2;
		else
			evpc_dsr_page &= ~2;
		break;

	case 6:
		break;

	case 7:
		break;
	}
}


/*
	read a byte in evpc DSR space
*/
static READ_HANDLER(evpc_mem_r)
{
	if (offset < 0x1f00)
	{
		return ti99_evpc_DSR[offset+evpc_dsr_page*0x2000];
	}
	else if (offset < 0x1ff0)
	{
		if (RAMEN)
		{	/* NOVRAM */
			return 0;
		}
		else
		{
			return ti99_evpc_DSR[offset+evpc_dsr_page*0x2000];
		}
	}
	else
	{	/* PALETTE */
		logerror("palette read, offset=%d\n", offset-0x1ff0);
		return 0;
	}
}

/*
	write a byte in evpc DSR space
*/
static WRITE_HANDLER(evpc_mem_w)
{
	if ((offset >= 0x1f00) && (offset < 0x1ff0) && RAMEN)
	{	/* NOVRAM */
	}
	else if (offset >= 0x1ff0)
	{	/* PALETTE */
		logerror("palette write, offset=%d\n, data=%d", offset-0x1ff0, data);
	}
}
