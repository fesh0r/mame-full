/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

	TODO:
	clean up (maybe split) the different pieces of hardware
	PIC, PIT, DMA... add support for LPT, COM (almost done)
	emulation of a serial mouse on a COM port (almost done)
	support for Game Controller port at 0x0201
	support for XT harddisk (under the way, see machine/pc_hdc.c)
	whatever 'hardware' comes to mind,
	maybe SoundBlaster? EGA? VGA?

***************************************************************************/
#include <assert.h>
#include "driver.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"

#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/dma8237.h"
#include "includes/uart8250.h"
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_aga.h"

#include "devices/pc_flopp.h"
#include "includes/pc_mouse.h"
#include "includes/pckeybrd.h"
#include "includes/pc_fdc_h.h"

#include "includes/pclpt.h"
#include "includes/centroni.h"

#include "devices/pc_hdc.h"
#include "includes/nec765.h"

#include "includes/pcshare.h"
#include "mscommon.h"

#define VERBOSE_DBG 0       /* general debug messages */
#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

#define VERBOSE_JOY 0		/* JOY (joystick port) */

#if VERBOSE_JOY
#define LOG(LEVEL,N,M,A)  \
#define JOY_LOG(N,M,A) \
	if(VERBOSE_JOY>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define JOY_LOG(n,m,a)
#endif

// preliminary machines setup 
//#define MESS_MENU
#ifdef MESS_MENU
#include "menu.h"
#include "menuentr.h"
#endif

#define FDC_DMA 2

/* called when a interrupt is set/cleared from com hardware */
static void pc_com_interrupt(int nr, int state)
{
	static const int irq[4]={4,3,4,3};
	/* issue COM1/3 IRQ4, COM2/4 IRQ3 */
	if (state)
	{
		pic8259_0_issue_irq(irq[nr]);
	}
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected(int n, int data)
{
	/* mouse connected to this port? */
	if (readinputport(3) & (0x80>>n))
		pc_mouse_handshake_in(n,data);
}

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
static uart8250_interface com_interface[4]=
{
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	}
};

/*
 * timer0	heartbeat IRQ
 * timer1	DRAM refresh (ignored)
 * timer2	PIO port C pin 4 and speaker polling
 */
static PIT8253_CONFIG pc_pit8253_config={
	TYPE8253,
	{
		{
			4770000/4,				/* heartbeat IRQ */
			pic8259_0_issue_irq,
			NULL
		}, {
			4770000/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4770000/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};

static PC_LPT_CONFIG lpt_config[3]={
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	}
};

static CENTRONICS_CONFIG cent_config[3]={
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	}
};

#define SETUP_MEMORY_640 1
#define SETUP_SER_MOUSE 1
#define SETUP_FDC_SSDD 1
#define SETUP_FDC_DSDD 2
#define SETUP_FDC_DSDD80 3
#define SETUP_FDC_3DSDD 4
#define SETUP_FDC_DSHD 5
#define SETUP_FDC_3DSHD 6
#define SETUP_FDC_3DSED 7

typedef enum { 
	SETUP_END,
	SETUP_HEADER,
	SETUP_COMMENT,
	SETUP_MEMORY,
	SETUP_GRAPHIC0,
	SETUP_KEYB,
	SETUP_FDC0, 
	SETUP_FDC0D0, SETUP_FDC0D1, SETUP_FDC0D2, SETUP_FDC0D3,
	SETUP_HDC0, SETUP_HDC0D0, SETUP_HDC0D1,
	SETUP_RTC,
	SETUP_SER0, SETUP_SER0CHIP, SETUP_SER0DEV, 
	SETUP_SER1, SETUP_SER1CHIP, SETUP_SER1DEV, 
	SETUP_SER2, SETUP_SER2CHIP, SETUP_SER2DEV, 
	SETUP_SER3, SETUP_SER3CHIP, SETUP_SER3DEV,
	SETUP_SERIAL_MOUSE,
	SETUP_PAR0, SETUP_PAR0TYPE, SETUP_PAR0DEV, 
	SETUP_PAR1, SETUP_PAR1TYPE, SETUP_PAR1DEV, 
	SETUP_PAR2, SETUP_PAR2TYPE, SETUP_PAR2DEV,
	SETUP_GAME0, SETUP_GAME0C0, SETUP_GAME0C1,
	SETUP_MPU0, SETUP_MPU0D0,
	SETUP_FM, SETUP_FM_TYPE, SETUP_FM_PORT,
	SETUP_CMS, SETUP_CMS_TEXT,
	SETUP_PCJR_SOUND,
	SETUP_AMSTRAD_JOY, SETUP_AMSTRAD_MOUSE
} PC_ID;
typedef struct _PC_SETUP {
	PC_ID id;
	int def, mask; 
} PC_SETUP;

static PC_SETUP pc_setup_at_[]=
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
};

static PC_SETUP pc_setup_t1000hx_[] =
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			4, 16 },
	{ SETUP_KEYB,				5, 32 },
	{ SETUP_MEMORY,				1, 3 }, //?
	{ SETUP_PCJR_SOUND,			1, 2 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				4, 16 },	
	{ SETUP_FDC0D2,				0, 31 },	
	{ SETUP_FDC0D3,				0, 31 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_GAME0,				1, 2 },
	{ SETUP_GAME0C0,			1, 3 },
	{ SETUP_GAME0C1,			1, 3 },
	{ SETUP_END }
};

static PC_SETUP pc_setup_pc1512_[]=
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			6, 64 },
	{ SETUP_KEYB,				6, 64 },
	{ SETUP_AMSTRAD_JOY,		1, 3 },
	{ SETUP_AMSTRAD_MOUSE,		1, 3 },
	{ SETUP_MEMORY,				1, 3 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				2, 5 },	
	{ SETUP_FDC0D1,				0, 5 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_RTC,				1, 2 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_END }
};

static PC_SETUP pc_setup_pc1640_[] = 
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			5, 32 },
	{ SETUP_KEYB,				6, 64 },
	{ SETUP_AMSTRAD_JOY,		1, 3 },
	{ SETUP_AMSTRAD_MOUSE,		1, 3 },
	{ SETUP_MEMORY,				1, 3 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				2, 5 },	
	{ SETUP_FDC0D1,				0, 5 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_RTC,				1, 2 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_END }
};

static PC_SETUP pc_setup_[] =
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
};

static PC_SETUP pc_setup_europc_[] = 
{
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
};

PC_SETUP *pc_setup_at		= pc_setup_at_;
PC_SETUP *pc_setup_t1000hx	= pc_setup_t1000hx_;
PC_SETUP *pc_setup_pc1512	= pc_setup_pc1512_;
PC_SETUP *pc_setup_pc1640	= pc_setup_pc1640_;
PC_SETUP *pc_setup			= pc_setup_;
PC_SETUP *pc_setup_europc	= pc_setup_europc_;

#ifdef MESS_MENU
/* must be reentrant! */
static void pc_update_setup(int id, int value)
{
	int flag=0;
	int v;

	/* avoid conflicts */
	switch (id) {
	case SETUP_SER0DEV: case SETUP_SER1DEV: case SETUP_SER2DEV: case SETUP_SER3DEV:
		if (value==SETUP_SER_MOUSE) {
			v=menu_get_value(m_setup, SETUP_SER0DEV);
			if ((id!=SETUP_SER0DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER0DEV, 0);

			v=menu_get_value(m_setup,SETUP_SER1DEV);
			if ((id!=SETUP_SER1DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER1DEV, 0);

			v=menu_get_value(m_setup, SETUP_SER2DEV);
			if ((id!=SETUP_SER2DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER2DEV, 0);

			v=menu_get_value(m_setup, SETUP_SER3DEV);
			if ((id!=SETUP_SER3DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER3DEV, 0);
		}
		break;
	case SETUP_FDC0:
		menu_set_visibility(m_setup, SETUP_FDC0D0, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D1, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D2, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D3, value!=0);
		break;
	case SETUP_HDC0:
		menu_set_visibility(m_setup, SETUP_HDC0D0, value!=0);
		menu_set_visibility(m_setup, SETUP_HDC0D1, value!=0);
		break;
	case SETUP_SER0:
		menu_set_visibility(m_setup, SETUP_SER0CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER0DEV, value!=0);
		break;
	case SETUP_SER1:
		menu_set_visibility(m_setup, SETUP_SER1CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER1DEV, value!=0);
		break;
	case SETUP_SER2:
		menu_set_visibility(m_setup, SETUP_SER2CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER2DEV, value!=0);
		break;
	case SETUP_SER3:
		menu_set_visibility(m_setup, SETUP_SER3CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER3DEV, value!=0);
		break;
	case SETUP_PAR0:
		menu_set_visibility(m_setup, SETUP_PAR0TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR0DEV, value!=0);
		break;
	case SETUP_PAR1:
		menu_set_visibility(m_setup, SETUP_PAR1TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR1DEV, value!=0);
		break;
	case SETUP_PAR2:
		menu_set_visibility(m_setup, SETUP_PAR2TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR2DEV, value!=0);
		break;
	}

	switch (id) {
	case SETUP_MEMORY:
		switch (value) {
		case SETUP_MEMORY_640: flag=1; break;
		}
		install_mem_read_handler(0, 0x80000, 0x9ffff, flag?MRA_RAM:MRA_ROM );
		install_mem_write_handler(0, 0x80000, 0x9ffff, flag?MWA_RAM:MWA_NOP );
		break;
	case SETUP_SER0DEV:
	}
}

static struct {
	PC_ID id;
	MENU_TEXT text;
} text_entries[]= {
	{ SETUP_HEADER, { "Machine Setup", MENU_TEXT_CENTER } },
	{ SETUP_COMMENT, { "(Reset is neccessary for several options)", MENU_TEXT_CENTER } },
	{ SETUP_CMS_TEXT, { " (Were also on Soundblaster 1.x)", MENU_TEXT_LEFT } },
};

static struct {
	PC_ID id;
	MENU_SELECTION selection;
} selection_entries[]= {
	{ SETUP_MEMORY, { "Memory", 1, pc_update_setup,
	  { "512KB", "640KB" } } },
	{ SETUP_GRAPHIC0, { "Graphics Adapter", 0, pc_update_setup,
	  { "CGA", "MDA", "Hercules", 
		"Tseng Labs ET4000", "PC Junior/Tandy 1000", 
		"Paradise EGA", "PC1512" } } },
	{ SETUP_KEYB, { "Keyboard", 4, pc_update_setup,
	  { "None", "US", "International", "MF2", "MF2 International", 
		"Tandy 1000", "Amstrad PC1512/1640" } } },
	{ SETUP_FDC0, { "Floppy controller", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x3f0 IRQ:6 DMA:2" } } },
	{ SETUP_FDC0D0, { " Floppy Drive 0", 2, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D1, { " Floppy Drive 1", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D2, { " Floppy Drive 2", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D3, { " Floppy Drive 3", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_HDC0, { "Harddisk controller 0", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x320" } } },
	{ SETUP_HDC0D0, { " Harddisk 0", 0, pc_update_setup, 
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_HDC0D1, { " Harddisk 1", 0, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_RTC, { "MC146818 RTC with batterie bufferd CMOS RAM", 0, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER0, { "Serial Port at IO:0x3f8 IRQ:4 (COM1)", 1, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER0CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER0DEV, { " Device", 1,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER1, { "Serial Port at IO:0x2f8 IRQ:3 (COM2)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER1CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER1DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER2, { "Serial Port at IO:0x3e8 IRQ:4 (COM3)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER2CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER2DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER3, { "Serial Port at IO:0x2e8 IRQ:3 (COM4)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER3CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER3DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SERIAL_MOUSE, { "Serial Mouse Protocol", 0, pc_update_setup,
	  { "Microsoft", "Mouse Systems" } } },
	{ SETUP_PAR0, { "Parallel Port at IO:0x3bc (LPT1)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR0TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR0DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_PAR1, { "Parallel Port at IO:0x378 (LPT2)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR1TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR1DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_PAR2, { "Parallel Port at IO:0x278 (LPT3)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR2TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR2DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_GAME0, { "Game Port  at IO:0x200", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_GAME0C0, { " Controller 0", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_GAME0C1, { " Controller 1", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_MPU0, { "MPU 401 Midi Interface at IO:0x330", 0, pc_update_setup, 
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_MPU0D0, { " Device", 0,  pc_update_setup,
	  { "None", "MT32 Synthesizer" } } },
	{ SETUP_FM, { "FM Chip", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_FM_TYPE, { " Type", 0,  pc_update_setup,
	  { "YM3812/OPL II (Adlib)", 
		"2 YM3812/OPL II (Soundblaster Pro Variant 1)",
		"OPL III" } } },
	{ SETUP_FM_PORT, { " Port" , 2, pc_update_setup,
	  { "IO:0x378(Adlib)", "IO:0x378/0x210 (Soundblaster)", "IO:0x378/0x220 (Soundblaster)",
		"IO:0x378/0x230 (Soundblaster)",
		"IO:0x378/0x240 (Soundblaster)" } } },
	{ SETUP_CMS, { "2 SAA1099 (Creative Music System/Gameblaster)", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x220" } } },
	{ SETUP_PCJR_SOUND, { "PC Junior Sound", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_AMSTRAD_JOY, { "Amstrad Joystick", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_AMSTRAD_MOUSE, { "Amstrad Mouse", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
};

static MENU_ENTRY_TYPE pc_find_entry(int id, void **result)
{
	int i;
	for (i=0; i<ARRAY_LENGTH(text_entries); i++) {
		if (text_entries[i].id==id) {
			*result=&text_entries[i].text;
			return MENU_ENTRY_TEXT;
		}
	}
	for (i=0; i<ARRAY_LENGTH(selection_entries); i++) {
		if (selection_entries[i].id==id) {
			*result=&selection_entries[i].selection;
			return MENU_ENTRY_SELECTION;
		}
	}
	exit(1);
}
#endif

static void pc_add_entry(int id, int def, int mask)
{
#ifdef MESS_MENU
	void *entry;
	int type=pc_find_entry(id, &entry);
	int i,j;

	switch (type) {
	case MENU_ENTRY_SELECTION:
		((MENU_SELECTION*)entry)->value=def;
		mask=~mask;
		for (i=0, j=0; i<ARRAY_LENGTH(((MENU_SELECTION*)entry)->options);i++) {
			if ((1<<i)&mask) ((MENU_SELECTION*)entry)->options[i]=0;
			if (((MENU_SELECTION*)entry)->options[i]) j++;
		}
		((MENU_SELECTION*)entry)->editable=j>1;
		break;
	}
	menu_add_entry(m_setup, id, 1, type, entry);
#endif
}

void pc_init_setup(PC_SETUP *setup) 
{
	int i;

	for (i=0; setup[i].id!=SETUP_END; i++ ) {
		pc_add_entry(setup[i].id, setup[i].def, setup[i].mask);
	}
}

void init_pc_common(void)
{
	pit8253_config(0,&pc_pit8253_config);
	/* FDC hardware */
	pc_fdc_setup();

	/* com hardware */
	uart8250_init(0,com_interface);
	uart8250_reset(0);
	uart8250_init(1,com_interface+1);
	uart8250_reset(1);
	uart8250_init(2,com_interface+2);
	uart8250_reset(2);
	uart8250_init(3,com_interface+3);
	uart8250_reset(3);

	pc_lpt_config(0, lpt_config);
	centronics_config(0, cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(1, lpt_config+1);
	centronics_config(1, cent_config+1);
	pc_lpt_set_device(1, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(2, lpt_config+2);
	centronics_config(2, cent_config+2);
	pc_lpt_set_device(2, &CENTRONICS_PRINTER_DEVICE);

	/* serial mouse */
	pc_mouse_set_protocol(TYPE_MICROSOFT_MOUSE);
	pc_mouse_set_input_base(12);
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* PC-XT keyboard */
	at_keyboard_init();
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);

	statetext_add_function(pc_harddisk_state);
//	statetext_add_function(nec765_state);
}

void pc_mda_init(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb0000, 0xbffff, MRA_RAM );
	install_mem_write_handler(0, 0xb0000, 0xbffff, pc_mda_videoram_w );
	videoram=memory_region(REGION_CPU1)+0xb0000; videoram_size=0x10000;

	install_port_read_handler(0, 0x3b0, 0x3bf, pc_MDA_r );
	install_port_write_handler(0, 0x3b0, 0x3bf, pc_MDA_w );
}

void pc_cga_init(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb8000, 0xbbfff, MRA_RAM );
	install_mem_write_handler(0, 0xb8000, 0xbbfff, pc_cga_videoram_w );
	videoram=memory_region(REGION_CPU1)+0xb8000; videoram_size=0x4000;

	install_port_read_handler(0, 0x3d0, 0x3df, pc_CGA_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc_CGA_w );
}

void pc_vga_init(void)
{
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

#if 0
        for (chksum=0, i=0;i<0x8000;i++) {
                chksum+=memory[i];
        }
        printf("checksum %.2x\n",chksum);
#endif

	install_mem_read_handler(0, 0xa0000, 0xaffff, MRA_BANK1 );
	install_mem_read_handler(0, 0xb0000, 0xb7fff, MRA_BANK2 );
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA_BANK3 );
	install_mem_read_handler(0, 0xc0000, 0xc7fff, MRA_ROM );

	install_mem_write_handler(0, 0xa0000, 0xaffff, MWA_BANK1 );
	install_mem_write_handler(0, 0xb0000, 0xb7fff, MWA_BANK2 );
	install_mem_write_handler(0, 0xb8000, 0xbffff, MWA_BANK3 );
	install_mem_write_handler(0, 0xc0000, 0xc7fff, MWA_ROM );

	install_port_read_handler(0, 0x3b0, 0x3bf, vga_port_03b0_r );
	install_port_read_handler(0, 0x3c0, 0x3cf, vga_port_03c0_r );
	install_port_read_handler(0, 0x3d0, 0x3df, vga_port_03d0_r );

	install_port_write_handler(0, 0x3b0, 0x3bf, vga_port_03b0_w );
	install_port_write_handler(0, 0x3c0, 0x3cf, vga_port_03c0_w );
	install_port_write_handler(0, 0x3d0, 0x3df, vga_port_03d0_w );
}

/***********************************/
/* PC interface to PC COM hardware */
/* Done this way because PCW16 also has PC-com hardware but it
is connected in a different way */

static int pc_COM_r(int n, int offset)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_r",("COM%d $%02x: disabled\n", n+1, 0x0ff));
		return 0x0ff;
    }

	return uart8250_r(n, offset);
}

static void pc_COM_w(int n, int offset, int data)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_w",("COM%d $%02x: disabled\n", n+1, data));
		return;
    }

	uart8250_w(n,offset, data);
}

READ_HANDLER(pc_COM1_r)
{
	return pc_COM_r(0, offset);
}

READ_HANDLER(pc_COM2_r)
{
	return pc_COM_r(1, offset);
}

READ_HANDLER(pc_COM3_r)
{
	return pc_COM_r(2, offset);
}

READ_HANDLER(pc_COM4_r)
{
	return pc_COM_r(3, offset);
}


WRITE_HANDLER(pc_COM1_w)
{
	uart8250_w(0, offset,data);
}

WRITE_HANDLER(pc_COM2_w)
{
	uart8250_w(1, offset,data);
}

WRITE_HANDLER(pc_COM3_w)
{
	uart8250_w(2, offset,data);
}

WRITE_HANDLER(pc_COM4_w)
{
	uart8250_w(3, offset,data);
}

/*
   keyboard seams to permanently sent data clocked by the mainboard
   clock line low for longer means "resync", keyboard sends 0xaa as answer
   will become automatically 0x00 after a while
*/
static struct {
	UINT8 data;
	int on;
} pc_keyb= { 0 };

UINT8 pc_keyb_read(void)
{
	return pc_keyb.data;
}

static void pc_keyb_timer(int param)
{
	pc_keyboard_init();
	pc_keyboard();
}

void pc_keyb_set_clock(int on)
{
	if (!pc_keyb.on && on)
		timer_set(1/200.0, 0, pc_keyb_timer);

	pc_keyb.on = on;
	// NPW 2-Feb-2001 - Disabling this because it appears to cause problems
	//pc_keyboard();
}

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

//	if( !pic8259_0_irq_pending(1) && ((pc_port[0x61]&0xc0)==0xc0) ) // amstrad problems
	if( !pic8259_0_irq_pending(1) && pc_keyb.on)
	{
		if ( (data=at_keyboard_read())!=-1) {
			pc_keyb.data = data;
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_keyb.data));
			pic8259_0_issue_irq(1);
		}
	}
}

/*************************************************************************
 *
 *		JOY
 *		joystick port
 *
 *************************************************************************/

static double JOY_time = 0.0;

WRITE_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}

#if 0
#define JOY_VALUE_TO_TIME(v) (24.2e-6+11e-9*(100000.0/256)*v)
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
#if 0
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
#endif
	{
		delta=new_time-JOY_time;
		if ( delta>JOY_VALUE_TO_TIME(input_port_16_r(0)) ) data &= ~0x01;
		if ( delta>JOY_VALUE_TO_TIME(input_port_17_r(0)) ) data &= ~0x02;
		if ( delta>JOY_VALUE_TO_TIME(input_port_18_r(0)) ) data &= ~0x04;
		if ( delta>JOY_VALUE_TO_TIME(input_port_19_r(0)) ) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input_port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#else
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
	{
		delta = (int)( 256 * 1000 * (new_time - JOY_time) );
		if (input_port_16_r(0) < delta) data &= ~0x01;
		if (input_port_17_r(0) < delta) data &= ~0x02;
		if (input_port_18_r(0) < delta) data &= ~0x04;
		if (input_port_19_r(0) < delta) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input
						   _port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#endif
