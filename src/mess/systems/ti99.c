/*
  MESS Driver for TI 99/4A Home Computer.
  R Nabet, 1999.

  see machine/ti99.c for some details and references

  NOTE!!!!!!!!!!  for MAME 36b5, I commented out some members of this function..
  check for m36b5 that Nicola added the new TMS5220 drivers.......

  static struct TMS5220interface tms5220interface =
  {
    640000L,   640kHz -> 8kHz output
    10000,     Volume.  I don't know the best value.
  	NULL,      no IRQ callback
  	2,         memory region where the speech ROM is.  -1 means no speech ROM
  	0,         memory size of speech rom (0 -> take memory region length)
  };

*/

#include "driver.h"

#include "mess/vidhrdw/tms9928a.h"

/* protos for support code in machine/ti99.c */

void ti99_init_machine(void);
void ti99_stop_machine(void);

int ti99_load_rom (void);
int ti99_id_rom (const char *name, const char *gamename);
int ti99_vh_start(void);
int ti99_vblank_interrupt(void);

void ti99_wb_cartmem(int offset, int data);

int ti99_rb_scratchpad(int offset);
void ti99_wb_scratchpad(int offset, int data);

void ti99_wb_wsnd(int offset, int data);
int ti99_rb_rvdp(int offset);
void ti99_wb_wvdp(int offset, int data);
int ti99_rb_rspeech(int offset);
void ti99_wb_wspeech(int offset, int data);
int ti99_rb_rgpl(int addr);
void ti99_wb_wgpl(int offset, int data);

int ti99_R9901_0(int offset);
int ti99_R9901_1(int offset);
void ti99_W9901_0(int offset, int data);
void ti99_W9901_S(int offset, int data);
void ti99_W9901_F(int offset, int data);

int ti99_R9901_2(int offset);
int ti99_R9901_3(int offset);
void ti99_KeyC2(int offset, int data);
void ti99_KeyC1(int offset, int data);
void ti99_KeyC0(int offset, int data);
void ti99_AlphaW(int offset, int data);
void ti99_CS1_motor(int offset, int data);
void ti99_CS2_motor(int offset, int data);
void ti99_audio_gate(int offset, int data);
void ti99_CS_output(int offset, int data);

int ti99_rb_disk(int offset);
void ti99_wb_disk(int offset, int data);
int ti99_DSKget(int offset);
void ti99_DSKROM(int offset, int data);
void ti99_DSKhold(int offset, int data);
void ti99_DSKheads(int offset, int data);
void ti99_DSKsel(int offset, int data);
void ti99_DSKside(int offset, int data);

/*
  memory mapping.
*/

static struct MemoryReadAddress readmem[] =
{
  { 0x0000, 0x1fff, MRA_ROM },          /*system ROM*/
  { 0x2000, 0x3fff, MRA_RAM },          /*lower 8kb of RAM extension*/
  { 0x4000, 0x5fff, ti99_rb_disk },     /*DSR ROM... only disk is emulated */
  { 0x6000, 0x7fff, MRA_ROM },          /*cartidge memory... some RAM is actually possible*/
  { 0x8000, 0x82ff, ti99_rb_scratchpad },   /*RAM PAD, mapped to 0x8300-0x83ff*/
  { 0x8300, 0x83ff, MRA_RAM },          /*RAM PAD*/
  { 0x8400, 0x87ff, MRA_NOP },          /*soundchip write*/
  { 0x8800, 0x8Bff, ti99_rb_rvdp },     /*vdp read*/
  { 0x8C00, 0x8Fff, MRA_NOP },          /*vdp write*/
  { 0x9000, 0x93ff, ti99_rb_rspeech },  /*speech read*/
  { 0x9400, 0x97ff, MRA_NOP },          /*speech write*/
  { 0x9800, 0x9Bff, ti99_rb_rgpl },     /*GPL read*/
  { 0x9C00, 0x9fff, MRA_NOP },          /*GPL write*/
  { 0xA000, 0xffff, MRA_RAM },          /*upper 24kb of RAM extension*/
  { -1 }    /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
  { 0x0000, 0x1fff, MWA_ROM },          /*system ROM*/
  { 0x2000, 0x3fff, MWA_RAM },          /*lower 8kb of memory expansion card*/
  { 0x4000, 0x5fff, ti99_wb_disk },     /*DSR ROM... only disk is emulated ! */
  { 0x6000, 0x7fff, ti99_wb_cartmem },  /*cartidge memory... some RAM or paging system is possible*/
  { 0x8000, 0x82ff, ti99_wb_scratchpad },   /*RAM PAD, mapped to 0x8300-0x83ff*/
  { 0x8300, 0x83ff, MWA_RAM },          /*RAM PAD*/
  { 0x8400, 0x87ff, ti99_wb_wsnd },     /*soundchip write*/
  { 0x8800, 0x8Bff, MWA_NOP },          /*vdp read*/
  { 0x8C00, 0x8Fff, ti99_wb_wvdp },     /*vdp write*/
  { 0x9000, 0x93ff, MWA_NOP },          /*speech read*/
  { 0x9400, 0x97ff, ti99_wb_wspeech },  /*speech write*/
  { 0x9800, 0x9Bff, MWA_NOP },          /*GPL read*/
  { 0x9C00, 0x9fff, ti99_wb_wgpl },     /*GPL write*/
  { 0xA000, 0xffff, MWA_RAM },          /*upper 24kb of RAM extension*/
  { -1 }    /* end of table */
};


/*
  CRU mapping.
*/

static struct IOWritePort writeport[] =
{
  {0x0000, 0x0000, ti99_W9901_0},
  {0x0001, 0x000e, ti99_W9901_S},
  {0x000f, 0x000f, ti99_W9901_F},

  {0x0012, 0x0012, ti99_KeyC2},
  {0x0013, 0x0013, ti99_KeyC1},
  {0x0014, 0x0014, ti99_KeyC0},
  {0x0015, 0x0015, ti99_AlphaW},
  {0x0016, 0x0016, ti99_CS1_motor},
  {0x0017, 0x0017, ti99_CS2_motor},
  {0x0018, 0x0018, ti99_audio_gate},
  {0x0019, 0x0019, ti99_CS_output},

  /*{0x0F01, 0x0F0e, ti99_W9901_S},*/ /* this mirror is used by a system routine */

  {0x0800, 0x0800, ti99_DSKROM},
  /*{0x0801, 0x0801, ti99_DSKmotor},*/
  {0x0802, 0x0802, ti99_DSKhold},
  {0x0803, 0x0803, ti99_DSKheads},
  {0x0804, 0x0806, ti99_DSKsel},
  {0x0807, 0x0807, ti99_DSKside},
  { -1 }    /* end of table */
};

static struct IOReadPort readport[] =
{
  {0x0000, 0x0000, ti99_R9901_0},
  {0x0001, 0x0001, ti99_R9901_1},

  {0x0002, 0x0002, ti99_R9901_2},
  {0x0003, 0x0003, ti99_R9901_3},

  {0x0100, 0x0100, ti99_DSKget},

  { -1 }    /* end of table */
};


/*
  Input ports, used by machine code for TI keyboard and joystick emulation.
*/
INPUT_PORTS_START(ti99_input_ports)

  PORT_START    /* col 0 */
    PORT_BITX(0x88, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_O/*PTION*/, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

  PORT_START    /* col 1 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

  PORT_START    /* col 2 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 # ERASE", KEYCODE_3, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I", KEYCODE_I, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)

  PORT_START    /* col 3 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)

  PORT_START    /* col 4 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)

  PORT_START    /* col 5 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A (incomplete vertical bar)", KEYCODE_A, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

  PORT_START    /* col 6 : "wired handset 1" (= joystick 1) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

  PORT_START    /* col 7 : "wired handset 2" (= joystick 2) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

  PORT_START    /* one more port for Alpha line */
    PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }    /* end of array */
};

/*extern unsigned char TMS9928A_palette[];*/

/*
  My palette.  Nicer than the default TMS9928A_palette...
*/
static unsigned char palette[] =
{
  0, 0, 0,
  0, 0, 0,
  64, 179, 64,
  96, 192, 96,
  64, 64, 192,
  96, 96, 244,
  192, 64, 64,
  64, 244, 244,
  244, 64, 64,
  255, 128, 64,
  224, 192, 64,
  255, 224, 64,
  64, 128, 64,
  192, 64, 192,
  224, 224, 224,
  255, 255, 255,
};

/*extern unsigned short TMS9928A_colortable[];*/

static unsigned short colortable[] =
{
  0, 1,
  0, 2,
  0, 3,
  0, 4,
  0, 5,
  0, 6,
  0, 7,
  0, 8,
  0, 9,
  0,10,
  0,11,
  0,12,
  0,13,
  0,14,
  0,15,
};


/*
  TMS9919 sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,        /* one sound chip */
	{3579545},  /* clock speed. connected to the TMS9918A CPUCLK pin... */
	{ 10000 } /* Volume.  I don't know the best value. */
};

static struct TMS5220interface tms5220interface =
{
  640000L,  /* 640kHz -> 8kHz output */
  10000,    /* Volume.  I don't know the best value. */
	NULL,     /* no IRQ callback */
	//2,        /* memory region where the speech ROM is.  -1 means no speech ROM */
	//0,        /* memory size of speech rom (0 -> take memory region length) */
};

/*
  we use 2 DACs to emulate "audio gate and" tape output, even thought
  a) there was no DAC in a TI99
  b) these are 2-level output (a DAc is useless...)
*/
static struct DACinterface aux_sound_intf =
{
  2,	/* total number of DACs */
  {
    10000,  /* volume for tape output */
    10000   /* volume for audio gate*/
  }
};


/*
  machine description.
*/
static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,    /* 3.0 Mhz*/
			0,          /* Memory region #0 */
			readmem, writemem, readport, writeport,
			ti99_vblank_interrupt, 1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,							/* screen width */
	192,							/* screen height */
	{ 0, 256-1, 0, 192-1},			/* visible_area */
	gfxdecodeinfo,					/* graphics decode info (???)*/
	TMS9928A_PALETTE_SIZE,TMS9928A_COLORTABLE_SIZE/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ti99_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
		  SOUND_DAC,
			&aux_sound_intf
		}
	}
};


/*
  ROM loading
*/
ROM_START(ti99_rom)
	/*CPU memory space*/
	ROM_REGION(0x10000)
	ROM_LOAD("994arom.bin",  0x0000, 0x2000, 0xdb8f33e5)    /* system ROMs */
	ROM_LOAD("disk.bin",  0x4000, 0x2000, 0x8f7df93f)       /* disk DSR ROM */

	/*GPL memory space*/
	ROM_REGION(0x10000)
	ROM_LOAD("994agrom.bin",  0x0000, 0x6000, 0xaf5c2449)   /* system GROMs */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000)
	ROM_LOAD("spchrom.bin",  0x0000, 0x8000, 0x58b155f7)    /* system speech ROM */
ROM_END


struct GameDriver ti99_driver =
{
	__FILE__,
	0,
	"ti99",
	"TI99/4A Home Computer",
	"1999",
	"Texas Instrument",
	"R Nabet, based on Ed Swartz's V9T9.",
	GAME_COMPUTER,
	&machine_driver,
	0,

	ti99_rom,
	ti99_load_rom, 			/* load rom_file images */
	ti99_id_rom,			/* identify rom images */
	3,						/* number of ROM slots - in this case, a CMD binary */
	/* TI99 only had one slot on console, but cutting the ROMs in 3 files seems to be the anly way
	to handle cartidge until I use a header format.
	Note that there was sometime a speech ROM slot in the speech synthesizer, and you could plug
	up to 16 additonnal DSR roms and quite a lot of GROMs in the side port.  None of these is
	emulated. */
	3,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	0/*2*/,						/* number of cassette drives supported */
	0,						/* rom decoder */
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	ti99_input_ports,

	0,						/* color_prom */
	/*TMS9928A_palette*/palette,				/* color palette */
	/*TMS9928A_colortable*/colortable,				/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};


