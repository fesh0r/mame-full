/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/bbc.h"
#include "mess/vidhrdw/bbc.h"
#include "mess/machine/6522via.h"

/******************************************************************************
FRED
&FC00
JIM
&FD00
SHEILA
&FE00
&00-&07 6845 CRTC       Video controller                8  ( 2 bytes x  4 )
&08-&0F 6850 ACIA       Serial controller               8  ( 2 bytes x  4 )
&10-&1F Serial ULA      Serial system chip              16 ( 1 byte  x 16 )
&20-&2F Video ULA       Video system chip               16 ( 2 bytes x  8 )
&30-&3F 74LS161         Paged ROM selector              16 ( 1 byte  x 16 )
&40-&5F 6522 VIA        SYSTEM VIA                      32 (16 bytes x  2 )
&60-&7F 6522 VIA        USER VIA                        32 (16 bytes x  2 )
&80-&9F 8271 FDC        FDC Floppy disc controller      32 ( 8 bytes x  4 )
&A0-&BF 68B54 ADLC      ECONET controller               32 ( 4 bytes x  8 )
&C0-&DF uPD7002         Analogue to digital converter   32 ( 4 bytes x  8 )
&E0-&FF Tube ULA        Tube system interface           32

to get a working bbc micro you need, an initial screen display
a working SYSTEM VIA with the keyboard circuit connected, and
a line counter pulse back from the video circuit.
(Working meaning key presses will come up on the screen and a typed in
BBC basic program will run.)

Video controller is not initially needed fudged a screen display just to get thing working.
Serial controller is not needed.
Serial system chip is not needed.
Video system chip is not initially needed fudged a screen display just to get thing working.
Paged ROM selector initially there is only one ROM (Basic) so no paged rom selection is required.
SYSTEM VIA is needed with a connected keyboard.
USER VIA is not needed.
FDC Floppy disc controller is not needed.
ECONET controller is not needed.
Analogue to digital convert is not needed.

order of progress:
Add the system VIA (6522) use 6522 emulation as used already by the vic20.
Add the keyboard to the 6522.
Add the user VIA (many games use its counters) Controls the user and printer port which may be added later.
Add the video pules from the video system.
Add lots more video support
Add sound BBC sound chip is emulated in mame and is connected to the 6522, so "should" be easy.
Add teletext display (MODE 7)
Add floppy disc support, the bbc uses an 8271 disc controller,
and then started using WD1770 which may be like the WD1790 in mess (May need a bit of work)



******************************************************************************/

void page_select_w(int offset, int data)
{
	cpu_setbank(1,memory_region(REGION_CPU1)+0x10000+((data&0x0f)<<14));
}

void memory_w(int offset, int data)
{
	memory_region(REGION_CPU1)[offset]=data;
	vidmem[offset]=1;
}

void via_0_write(int offset, int data)
{
	/*if (errorlog) { fprintf(errorlog, "system VIA write %d = %d\n",offset,data); }; */
	via_0_w(offset,data);
}

int via_0_read(int offset)
{
	int data;
	data=via_0_r(offset);
	/*if (errorlog) { fprintf(errorlog, "system VIA read %d = %d\n",offset,data); }; */
	return data;
}

void via_1_write(int offset, int data)
{
	/*if (errorlog) { fprintf(errorlog, "user VIA write %d = %d\n",offset,data); }; */
	via_1_w(offset,data);
}

int via_1_read(int offset)
{
	int data;
	data=via_1_r(offset);
	/*if (errorlog) { fprintf(errorlog, "user VIA read %d = %d\n",offset,data); }; */
	return data;
}
static struct MemoryReadAddress readmem_bbc[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xfbff, MRA_ROM },
	{ 0xfc00, 0xfdff, MRA_NOP },  /* FRED & JIM Pages */
				      /* Shiela Address Page &fe00 - &feff                    */
	{ 0xfe00, 0xfe07, crtc6845_r },  /* &00-&07  6845 CRTC     Video controller              */
	{ 0xfe08, 0xfe0f, MRA_NOP },  /* &08-&0f  6850 ACIA     Serial Controller             */
	{ 0xfe10, 0xfe1f, MRA_NOP },  /* &10-&1f  Serial ULA    Serial system chip            */
	{ 0xfe20, 0xfe2f, videoULA_r },  /* &20-&2f  Video ULA     Video system chip             */
	{ 0xfe30, 0xfe3f, MRA_NOP },  /* &30-&3f  84LS161       Paged ROM selector            */
	{ 0xfe40, 0xfe5f, via_0_read },  /* &40-&5f  6522 VIA      SYSTEM VIA                    */
	{ 0xfe60, 0xfe7f, via_1_read },  /* &60-&7f  6522 VIA      USER VIA                      */
	{ 0xfe80, 0xfe9f, MRA_NOP },  /* &80-&9f  8271 FDC      Floppy disc controller        */
	{ 0xfea0, 0xfebf, MRA_NOP },  /* &a0-&bf  68B54 ADLC    ECONET controller             */
	{ 0xfec0, 0xfedf, MRA_NOP },  /* &c0-&df  uPD7002       Analogue to digital converter */
	{ 0xfee0, 0xfeff, MRA_NOP },  /* &e0-&ff  Tube ULA      Tube system interface         */
	{ 0xff00, 0xffff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress writemem_bbc[] =
{
	{ 0x0000, 0x7fff, memory_w },
	{ 0x8000, 0xdfff, MWA_NOP },
	{ 0xc000, 0xfdff, MWA_NOP },
	{ 0xfc00, 0xfdff, MWA_NOP },  /* FRED & JIM Pages */
				      /* Shiela Address Page &fe00 - &feff                    */
	{ 0xfe00, 0xfe07, crtc6845_w },  /* &00-&07  6845 CRTC     Video controller              */
	{ 0xfe08, 0xfe0f, MWA_NOP },  /* &08-&0f  6850 ACIA     Serial Controller             */
	{ 0xfe10, 0xfe1f, MWA_NOP },  /* &10-&1f  Serial ULA    Serial system chip            */
	{ 0xfe20, 0xfe2f, videoULA_w },  /* &20-&2f  Video ULA     Video system chip             */
	{ 0xfe30, 0xfe3f, page_select_w },  /* &30-&3f  84LS161 Paged ROM selector            */
	{ 0xfe40, 0xfe5f, via_0_write },  /* &40-&5f  6522 VIA      SYSTEM VIA                    */
	{ 0xfe60, 0xfe7f, via_1_write },  /* &60-&7f  6522 VIA      USER VIA                      */
	{ 0xfe80, 0xfe9f, MWA_NOP },  /* &80-&9f  8271 FDC      Floppy disc controller        */
	{ 0xfea0, 0xfebf, MWA_NOP },  /* &a0-&bf  68B54 ADLC    ECONET controller             */
	{ 0xfec0, 0xfedf, MWA_NOP },  /* &c0-&df  uPD7002       Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP },  /* &e0-&ff  Tube ULA      Tube system interface         */
	{ 0xff00, 0xffff, MWA_ROM },
    {-1}
};

unsigned short bbc_colour_table[9]=
{
	0,1,2,3,4,5,6,7,8
};

unsigned char   bbc_palette[9*3]=
{

	0x0ff,0x0ff,0x0ff,
	0x000,0x0ff,0x0ff,
	0x0ff,0x000,0x0ff,
	0x000,0x000,0x0ff,
	0x0ff,0x0ff,0x000,
	0x000,0x0ff,0x000,
	0x0ff,0x000,0x000,
	0x000,0x000,0x000,
	0x000,0x000,0x000
};

static void init_palette_bbc(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,bbc_palette,sizeof(bbc_palette));
	memcpy(sys_colortable,bbc_colour_table,sizeof(bbc_colour_table));
}

INPUT_PORTS_START(bbc)

	/* KEYBOARD COLUMN 0 */
	PORT_START
	PORT_BITX(0x01,  IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",      KEYCODE_RSHIFT,   IP_JOY_NONE)
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


INPUT_PORTS_END

ROM_START(bbc)
	ROM_REGION(0x50000,REGION_CPU1) /* ROM MEMORY */

	ROM_LOAD("os12.rom",    0x0C000, 0x4000, 0x3c14fc70 )

	ROM_LOAD("basic2.rom",  0x10000, 0x4000, 0x79434781 ) /* rom page 0 */
	ROM_LOAD("basic2.rom",  0x14000, 0x4000, 0x79434781 ) /* rom page 1 */
	ROM_LOAD("basic2.rom",  0x18000, 0x4000, 0x79434781 ) /* rom page 2 */
	ROM_LOAD("basic2.rom",  0x1c000, 0x4000, 0x79434781 ) /* rom page 3 */
	ROM_LOAD("basic2.rom",  0x20000, 0x4000, 0x79434781 ) /* rom page 4 */
	ROM_LOAD("basic2.rom",  0x24000, 0x4000, 0x79434781 ) /* rom page 5 */
	ROM_LOAD("basic2.rom",  0x28000, 0x4000, 0x79434781 ) /* rom page 6 */
	ROM_LOAD("basic2.rom",  0x2c000, 0x4000, 0x79434781 ) /* rom page 7 */
	ROM_LOAD("basic2.rom",  0x30000, 0x4000, 0x79434781 ) /* rom page 8 */
	ROM_LOAD("basic2.rom",  0x34000, 0x4000, 0x79434781 ) /* rom page 9 */
	ROM_LOAD("basic2.rom",  0x38000, 0x4000, 0x79434781 ) /* rom page 10 */
	ROM_LOAD("basic2.rom",  0x3c000, 0x4000, 0x79434781 ) /* rom page 11 */
	ROM_LOAD("basic2.rom",  0x40000, 0x4000, 0x79434781 ) /* rom page 12 */
	ROM_LOAD("basic2.rom",  0x44000, 0x4000, 0x79434781 ) /* rom page 13 */
	ROM_LOAD("basic2.rom",  0x48000, 0x4000, 0x79434781 ) /* rom page 14 */
	ROM_LOAD("basic2.rom",  0x4c000, 0x4000, 0x79434781 ) /* rom page 15 */

ROM_END

static int bbcb_vsync(void)
{
	via_0_ca1_w(0,0);
	via_0_ca1_w(0,1);

	return 0;
}


static struct SN76496interface sn76496_interface =
{
	1,      /* 1 chip */
	{ 4000000 },    /* 4Mhz */
	{ 100 }
};


static struct MachineDriver machine_driver_bbc =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,        /* 2.00Mhz */
			readmem_bbc,
			writemem_bbc,
			0,
			0,

			bbcb_vsync,     1,              /* screen refresh interrupts */
			bbcb_keyscan, 1000              /* scan keyboard */
		}
	},
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_bbc, /* init_machine */
	0, /* stop_machine */

	/* video hardware */
	800,300, {0,800-1,0,300-1},
	0,
	16, /*total colours*/
	16, /*colour_table_length*/
	init_palette_bbc, /*init palette */

	VIDEO_TYPE_RASTER,
	0,
	bbc_vh_start,
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


static const struct IODevice io_bbc[] =
{
	{IO_END}
};


COMP (1980,bbc,0,bbc,bbc,0,"Acorn","BBC Micro Model B")
