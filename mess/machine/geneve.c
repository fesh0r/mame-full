/*
	Machine code for Myarc Geneve.
	Raphael Nabet, 2003.
*/

#include <math.h>
#include "driver.h"
#include "tms9901.h"
#include "mm58274c.h"
#include "vidhrdw/v9938.h"
#include "sndhrdw/spchroms.h"
#include "ti99_4x.h"
#include "geneve.h"
#include "99_peb.h"
#include "994x_ser.h"
#include "99_dsk.h"
#include "99_ide.h"


/* prototypes */
static void inta_callback(int state);
static void intb_callback(int state);

/*static READ16_HANDLER ( geneve_rw_rspeech );
static WRITE16_HANDLER ( geneve_ww_wspeech );*/

static void poll_keyboard(int dummy);

static void tms9901_interrupt_callback(int intreq, int ic);
static int R9901_0(int offset);
static int R9901_1(int offset);
static int R9901_2(int offset);
static int R9901_3(int offset);

static void W9901_PE_bus_reset(int offset, int data);
static void W9901_VDP_reset(int offset, int data);
static void W9901_JoySel(int offset, int data);
static void W9901_KeyboardReset(int offset, int data);
static void W9901_ext_mem_wait_states(int offset, int data);
static void W9901_VDP_wait_states(int offset, int data);

/*
	pointers to memory areas
*/
/* pointer to boot ROM */
static UINT8 *ROM_ptr;
/* pointer to static RAM */
static UINT8 *SRAM_ptr;
/* pointer to dynamic RAM */
static UINT8 *DRAM_ptr;

/*
	Configuration
*/
/* TRUE if speech synthesizer present */
static char has_speech;
/* floppy disk controller type */
static fdc_kind_t fdc_kind;
/* TRUE if ide card present */
static char has_ide;
/* TRUE if rs232 card present */
static char has_rs232;


/* tms9901 setup */
static const tms9901reset_param tms9901reset_param_ti99 =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INT8 | TMS9901_INTB | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		R9901_0,
		R9901_1,
		R9901_2,
		R9901_3
	},

	{	/* write handlers */
		W9901_PE_bus_reset,
		W9901_VDP_reset,
		W9901_JoySel,
		NULL,
		NULL,
		NULL,
		W9901_KeyboardReset,
		W9901_ext_mem_wait_states,
		W9901_VDP_wait_states,
		NULL,
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
static int JoySel;
enum
{
	KeyQueueSize = 256
};
static UINT8 KeyQueue[KeyQueueSize];
int KeyQueueHead;
int KeyQueueLen;
int KeyInBuf;

/*
	GROM support.

	The Geneve does not include any GROM, but it features a simple GROM emulator.
*/
static int GROM_Address;

/*
	Cartridge support

	The Geneve does not have a cartridge port, but it has cartridge emulation.
*/
static int cartridge_page;


static int mode_flags;

static UINT8 page_lookup[8];

enum
{
	mf_PAL			= 0x001,	/* bit 5 */
	mf_keyclock		= 0x008,	/* bit 8 */
	mf_keyclear		= 0x010,	/* bit 9 */
	mf_genevemode	= 0x020,	/* bit 10 */
	mf_mapmode		= 0x040,	/* bit 11 */
	mf_waitstate	= 0x400		/* bit 15 */
};

/* tms9995_ICount: used to implement memory waitstates (hack) */
extern int tms9995_ICount;



/*===========================================================================*/
/*
	General purpose code:
	initialization, cart loading, etc.
*/

void init_geneve(void)
{
	//ti99_model = model_geneve;
	//has_evpc = TRUE;
}


void machine_init_geneve(void)
{
	//int i;

	mode_flags = /*0*/mf_mapmode;
	/* initialize page lookup */
	memset(page_lookup, 0, sizeof(page_lookup));

	/* set up RAM pointers */
	ROM_ptr = memory_region(REGION_CPU1) + offset_rom_geneve;
	SRAM_ptr = memory_region(REGION_CPU1) + offset_sram_geneve;
	DRAM_ptr = memory_region(REGION_CPU1) + offset_dram_geneve;

	/* erase GPL_port_lookup, so that only console_GROMs are installed by default */
	GROM_Address = 0;

	/* reset cartridge mapper */
	cartridge_page = 0;

	/* init tms9901 */
	tms9901_init(0, & tms9901reset_param_ti99);

	v9938_reset();

	mm58274c_init();

	/* clear keyboard interface state (probably overkill, but can't harm) */
	/*KeyCol = 0;
	AlphaLockLine = 0;*/
	timer_pulse(TIME_IN_SEC(.05), 0, poll_keyboard);

	/* read config */
	has_speech = (readinputport(input_port_config) >> config_speech_bit) & config_speech_mask;
	fdc_kind = (readinputport(input_port_config) >> config_fdc_bit) & config_fdc_mask;
	has_ide = (readinputport(input_port_config) >> config_ide_bit) & config_ide_mask;
	has_rs232 = (readinputport(input_port_config) >> config_rs232_bit) & config_rs232_mask;

	/* set up optional expansion hardware */
	ti99_peb_init(0, inta_callback, intb_callback);

	if (has_speech)
	{
		spchroms_interface speech_intf = { region_speech_rom };

		spchroms_config(& speech_intf);

		/*install_mem_read16_handler(0, 0x9000, 0x93ff, ti99_rw_rspeech);
		install_mem_write16_handler(0, 0x9400, 0x97ff, ti99_ww_wspeech);*/
	}
	else
	{
		/*install_mem_read16_handler(0, 0x9000, 0x93ff, ti99_rw_null8bits);
		install_mem_write16_handler(0, 0x9400, 0x97ff, ti99_ww_null8bits);*/
	}

	switch (fdc_kind)
	{
	case fdc_kind_TI:
		ti99_fdc_init();
		break;
	case fdc_kind_BwG:
		ti99_bwg_init();
		break;
	case fdc_kind_hfdc:
		ti99_hfdc_init();
		break;
	case fdc_kind_none:
		break;
	}

	if (has_ide)
		ti99_ide_init();

	if (has_rs232)
		ti99_rs232_init();
}

void machine_stop_geneve(void)
{
	if (has_rs232)
		ti99_rs232_cleanup();

	tms9901_cleanup(0);
}


/*
	video initialization.
*/
int video_start_geneve(void)
{
	return v9938_init(MODEL_V9938, 0x20000, tms9901_set_int2);	/* v38 with 128 kb of video RAM */
}

/*
	scanline interrupt
*/
void geneve_hblank_interrupt(void)
{
	v9938_interrupt();
}

/*
	inta is connected to both tms9901 IRQ1 line and to tms9995 INT4/EC line.
*/
static void inta_callback(int state)
{
	tms9901_set_single_int(0, 1, state);
	cpu_set_irq_line(0, 1, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
	intb is connected to tms9901 IRQ12 line.
*/
static void intb_callback(int state)
{
	tms9901_set_single_int(0, 12, state);
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark MEMORY HANDLERS
#endif
/*
	Memory handlers.
*/

READ_HANDLER ( geneve_r )
{
	int page;

	if (mode_flags & mf_genevemode)
	{
		/* geneve mode */
		if ((offset >= 0xf100) && (offset < 0xf140))
		{
			/* memory-mapped registers */
			/* VDP ports 2 & 3 and sound port are write-only */
			switch (offset)
			{
			case 0xf100:
				return v9938_vram_r(0);

			case 0xf102:
				return v9938_status_r(0);

			case 0xf110:
			case 0xf111:
			case 0xf112:
			case 0xf113:
			case 0xf114:
			case 0xf115:
			case 0xf116:
			case 0xf117:
				return page_lookup[offset-0xf110];

			case 0xf118:
				return KeyQueueLen ? KeyQueue[KeyQueueHead] : 0;

			case 0xf130:
			case 0xf131:
			case 0xf132:
			case 0xf133:
			case 0xf134:
			case 0xf135:
			case 0xf136:
			case 0xf137:
			case 0xf138:
			case 0xf139:
			case 0xf13a:
			case 0xf13b:
			case 0xf13c:
			case 0xf13d:
			case 0xf13e:
			case 0xf13f:
				return mm58274c_r(offset-0xf130);

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				return 0;
			}
		}
	}
	else
	{
		/* ti99 mode */
		if ((offset >= 0x8000) && (offset < 0xa000))
		{
			switch ((offset - 0x8000) >> 10)
			{
			case 0:
				/* 0x8000 */
				if ((offset >= 0x8000) && (offset < 0x8020))
				{
					/* geneve memory-mapped registers */
					switch (offset)
					{
					case 0x8000:
					case 0x8001:
					case 0x8002:
					case 0x8003:
					case 0x8004:
					case 0x8005:
					case 0x8006:
					case 0x8007:
						return page_lookup[offset-0x8000];

					case 0x8008:	/* right??? */
						return /*key_buf*/0;

					/*case 0xf130:
					case 0xf131:
					case 0xf132:
					case 0xf133:
					case 0xf134:
					case 0xf135:
					case 0xf136:
					case 0xf137:
					case 0xf138:
					case 0xf139:
					case 0xf13a:
					case 0xf13b:
					case 0xf13c:
					case 0xf13d:
					case 0xf13e:
					case 0xf13f:
						return mm58274c_r(offset-0xf130);*/

					default:
						logerror("unmapped read offs=%d\n", (int) offset);
						return 0;
					}
				}
				else
					/* PAD RAM */
					return DRAM_ptr[0x35*0x2000 + (offset-0x8000)];	/* ??? */

			case 2:
				/* VDP read */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* read VDP status */
						return v9938_status_r(0);
					}
					else
					{	/* read VDP RAM */
						return v9938_vram_r(0);
					}
				}
				return 0;

			case 4:
				/* speech read */
				/* ... */
				return 0;

			case 6:
				/* GPL read */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* read GPL address */
						int reply;
						reply = GROM_Address >> 8;
						GROM_Address = (GROM_Address << 8) & 0xffff;
						GROM_Address = (GROM_Address & 0xe000) | ((GROM_Address+1) | 0x1fff);
						return reply;
					}
					else
					{	/* read GPL data */
						int reply;
						reply = DRAM_ptr[0x38*0x2000 + GROM_Address];
						GROM_Address = (GROM_Address & 0xe000) | ((GROM_Address+1) | 0x1fff);
						return reply;
					}
				}
				return 0;

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				return 0;
			}
		}
	}

	page = offset >> 13;

	if (! (mode_flags & mf_mapmode))
		page = page_lookup[page];
	else
		page = page + 0xf0;

	offset &= 0x1fff;

	if (page < 0x40)
	{
		/* DRAM */
		return DRAM_ptr[page*0x2000 + offset];
	}
	else if ((page >= 0xb8) && (page < 0xc0))
	{
		/* PE-bus */
		switch (page)
		{
		case 0xba:
			return geneve_peb_r(offset);
		}
	}
	else if ((page >= 0xe8) && (page < 0xf0))
	{
		/* SRAM */
		return SRAM_ptr[(page-0xe8)*0x2000 + offset];
	}
	else if ((page >= 0xf0) && (page < 0xf2))
	{
		/* Boot ROM */
		return ROM_ptr[(page-0xf0)*0x2000 + offset];
	}

	logerror("unmapped read offs=%d\n", (int) offset);
	return 0;
}

WRITE_HANDLER ( geneve_w )
{
	int page;

	if (mode_flags & mf_genevemode)
	{
		/* geneve mode */
		if ((offset >= 0xf100) && (offset < 0xf140))
		{
			/* memory-mapped registers */
			/* VDP ports 2 & 3 and sound port are write-only */
			switch (offset)
			{
			case 0xf100:
				v9938_vram_w(0, data);
				return;

			case 0xf102:
				v9938_command_w(0, data);
				return;

			case 0xf104:
				v9938_palette_w(0, data);
				return;

			case 0xf106:
				v9938_register_w(0, data);
				return;

			case 0xf110:
			case 0xf111:
			case 0xf112:
			case 0xf113:
			case 0xf114:
			case 0xf115:
			case 0xf116:
			case 0xf117:
				page_lookup[offset-0xf110] = data;
				return;

			/*case 0xf118:	// right???
				key_buf = data;
				return*/

			case 0xf130:
			case 0xf131:
			case 0xf132:
			case 0xf133:
			case 0xf134:
			case 0xf135:
			case 0xf136:
			case 0xf137:
			case 0xf138:
			case 0xf139:
			case 0xf13a:
			case 0xf13b:
			case 0xf13c:
			case 0xf13d:
			case 0xf13e:
			case 0xf13f:
				mm58274c_w(offset-0xf130, data);
				return;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				return;
			}
		}
	}
	else
	{
		/* ti99 mode */
		if ((offset >= 0x8000) && (offset < 0xa000))
		{
			switch ((offset - 0x8000) >> 10)
			{
			case 0:
				/* 0x8000 */
				if ((offset >= 0x8000) && (offset < 0x8020))
				{
					/* geneve memory-mapped registers */
					switch (offset)
					{
					case 0x8000:
					case 0x8001:
					case 0x8002:
					case 0x8003:
					case 0x8004:
					case 0x8005:
					case 0x8006:
					case 0x8007:
						page_lookup[offset-0x8000] = data;
						return;

					/*case 0x8008:	// right???
						key_buf = data;
						return*/

					/*case 0xf130:
					case 0xf131:
					case 0xf132:
					case 0xf133:
					case 0xf134:
					case 0xf135:
					case 0xf136:
					case 0xf137:
					case 0xf138:
					case 0xf139:
					case 0xf13a:
					case 0xf13b:
					case 0xf13c:
					case 0xf13d:
					case 0xf13e:
					case 0xf13f:
						return mm58274c_w(offset-0xf130, data);*/

					default:
						logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
						return;
					}
				}
				else
				{
					/* PAD RAM */
					DRAM_ptr[0x35*0x2000 + (offset-0x8000)] = data;	/* ??? */
					return;
				}

			case 1:
				/* sound write */
				SN76496_0_w(0, data);
				return;

			case 3:
				/* VDP write */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* write VDP address */
						v9938_command_w(0, data);
					}
					else
					{	/* write VDP RAM */
						v9938_vram_w(0, data);
					}
				}
				return;

			case 5:
				/* speech write */
				/* ... */
				return;

			case 7:
				/* GPL write */
				if (! (offset & 1))
				{
					if (offset & 2)
					{	/* write GPL address */
						GROM_Address = ((int) data << 8) | (GROM_Address >> 8);
						GROM_Address = (GROM_Address & 0xe000) | ((GROM_Address+1) | 0x1fff);
					}
					else
					{	/* write GPL data */
						/* unimplemented */
					}
				}
				return;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				return;
			}
		}
	}

	page = offset >> 13;

	if (! (mode_flags & mf_mapmode))
		page = page_lookup[page];
	else
		page = page + 0xf0;

	offset &= 0x1fff;

	if (page < 0x40)
	{
		/* DRAM */
		DRAM_ptr[page*0x2000 + offset] = data;
		return;
	}
	else if ((page >= 0xb8) && (page < 0xc0))
	{
		/* PE-bus */
		switch (page)
		{
		case 0xba:
			geneve_peb_w(offset, data);
			return;
		}
	}
	else if ((page >= 0xe8) && (page < 0xf0))
	{
		/* SRAM */
		SRAM_ptr[(page-0xe8)*0x2000 + offset] = data;
		return;
	}
	else if ((page >= 0xf0) && (page < 0xf2))
	{
		/* Boot ROM */
		/*ROM_ptr[(page-0xf0)*0x2000 + offset] = data;*/
		return;
	}

	logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
	return;
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark CRU HANDLERS
#endif

WRITE_HANDLER ( geneve_peb_mode_CRU_w )
{
	if ((offset >= /*0x770*/0x775) && (offset < 0x780))
	{
		if ((offset == 0x778) && data && (! (mode_flags & mf_keyclock)))
		{
			/* set mf_keyclock */
			if (KeyQueueLen)
			{
				tms9901_set_single_int(0, 8, 1);
			}
		}
		if ((offset == 0x779)  && data && (! (mode_flags & mf_keyclear)))
		{
			/* set mf_keyclear */
			if (KeyQueueLen)
			{
				KeyQueueHead = (KeyQueueHead + 1) % KeyQueueSize;
				KeyQueueLen--;
			}
			/* shift in new key immediately if possible, otherwise clear interrupt */
			tms9901_set_single_int(0, 8, (KeyQueueLen && (mode_flags & mf_keyclock)) ? 1 : 0);
		}

		/* tms9995 user flags */
		if (data)
			mode_flags |= 1 << (offset - 0x775);
		else
			mode_flags &= ~(1 << (offset - 0x775));
	}

	geneve_peb_CRU_w(offset, data);
}

/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark KEYBOARD INTERFACE
#endif

static void poll_keyboard(int dummy)
{
	static UINT32 old_keystate[4];
	UINT32 keystate;
	UINT32 key_transitions;
	int i, j;
	int keycode;


	for (i=0; (i<4) && (KeyQueueLen < KeyQueueSize); i++)
	{
		keystate = readinputport(input_port_keyboard_geneve + i*2)
					| (readinputport(input_port_keyboard_geneve + i*2 + 1) << 16);
		key_transitions = keystate ^ old_keystate[i];
		if (key_transitions)
		{
			for (j=0; (j<32) && (KeyQueueLen < KeyQueueSize); j++)
			{
				if ((key_transitions >> j) & 1)
				{
					keycode = (i << 5) | j;
					if ((keystate >> j) & 1)
						old_keystate[i] |= (1 << j);
					else
					{
						old_keystate[i] &= ~ (1 << j);
						keycode |= 0x80;
					}
					KeyQueue[(KeyQueueHead+KeyQueueLen) % KeyQueueSize] = keycode;
					KeyQueueLen++;
					if (mode_flags & mf_keyclock)
					{
						tms9901_set_single_int(0, 8, 1);
					}
				}
			}
		}
	}
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark TMS9901 INTERFACE
#endif
/*
	Geneve-specific tms9901 I/O handlers

	See mess/machine/tms9901.c for generic tms9901 CRU handlers.
*/
/*
	TMS9901 interrupt handling on a Geneve.

	Geneve uses the following interrupts:
	INT1: external interrupt (used by RS232 controller, for instance) -
	  connected to tms9995 int4, too.
	INT2: VDP interrupt
	TMS9901 timer interrupt? (overrides INT3)
	INT8: keyboard interrupt???
	INT10: mouse interrupt???
	INT11: clock interrupt???
	INT12: INTB interrupt from PE-bus
*/

/*
	set the state of int2 (called by tms9928 core)
*/
/*void tms9901_set_int2(int state)
{
	tms9901_set_single_int(0, 2, state);
}*/

/*
	Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static void tms9901_interrupt_callback(int intreq, int ic)
{
	/* INTREQ is connected to INT1 (IC0-3 are not connected) */
	cpu_set_irq_line(0, 0, intreq ? ASSERT_LINE : CLEAR_LINE);
}

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bit 3-7: joystick status
*/
static int R9901_0(int offset)
{
	int answer;

	answer = readinputport(input_port_joysticks_geneve) >> (JoySel * 8);

	return (answer);
}

/*
	Read pins INT8*-INT15* of TI99's 9901.

	signification:
	 (bit 0: keyboard interrupt)
	 bit 1: unused
	 (bit 2: mouse interrupt)
	 (bit 3: clock interrupt)
	 (bit 4: INTB from PE-bus)
	 bit 5 & 7: used as output
	 bit 6: unused
*/
static int R9901_1(int offset)
{
	int answer;

	answer = 0;//((readinputport(input_port_keyboard + (KeyCol >> 1)) >> ((KeyCol & 1) * 8)) >> 5) & 0x07;

	return answer;
}

/*
	Read pins P0-P7 of TI99's 9901.
*/
static int R9901_2(int offset)
{
	return 0;
}

/*
	Read pins P8-P15 of TI99's 9901.
*/
static int R9901_3(int offset)
{
	return 0;
}


/*
	Write PE bus reset line
*/
static void W9901_PE_bus_reset(int offset, int data)
{
}

/*
	Write VDP reset line
*/
static void W9901_VDP_reset(int offset, int data)
{
}

/*
	Write joystick select line
*/
static void W9901_JoySel(int offset, int data)
{
	JoySel = data;
}

static void W9901_KeyboardReset(int offset, int data)
{
}

/*
	Write external mem cycles (0=long, 1=short)
*/
static void W9901_ext_mem_wait_states(int offset, int data)
{
}

/*
	Write vdp wait cycles (1=add 15 cycles, 0=add none)
*/
static void W9901_VDP_wait_states(int offset, int data)
{
}



