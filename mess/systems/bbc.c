/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/bbc.h"
#include "vidhrdw/bbc.h"
#include "machine/6522via.h"


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
******************************************************************************/

/* for the model A just address the 4 on board rom sockets */
WRITE_HANDLER ( page_selecta_w )
{
	cpu_setbank(2,memory_region(REGION_CPU1)+0x10000+((data&0x03)<<14));
}

/* for the model B address all 16 of the rom sockets */
WRITE_HANDLER ( page_selectb_w )
{
	cpu_setbank(2,memory_region(REGION_CPU1)+0x10000+((data&0x0f)<<14));
}


WRITE_HANDLER ( memory_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}

static struct MemoryReadAddress readmem_bbca[] =
{
	{ 0x0000, 0x3fff, MRA_RAM          },
	{ 0x4000, 0x7fff, MRA_BANK1        },  /* bank 1 is a repeat of the memory at 0x0000 to 0x3fff   */
	{ 0x8000, 0xbfff, MRA_BANK2        },
	{ 0xc000, 0xfbff, MRA_ROM          },
	{ 0xfc00, 0xfdff, MRA_NOP          },  /* FRED & JIM Pages */
				                           /* Shiela Address Page &fe00 - &feff                      */
	{ 0xfe00, 0xfe07, BBC_6845_r       },  /* &00-&07  6845 CRTC     Video controller                */
	{ 0xfe08, 0xfe0f, MRA_NOP          },  /* &08-&0f  6850 ACIA     Serial Controller               */
	{ 0xfe10, 0xfe1f, MRA_NOP          },  /* &10-&1f  Serial ULA    Serial system chip              */
	{ 0xfe20, 0xfe2f, videoULA_r       },  /* &20-&2f  Video ULA     Video system chip               */
	{ 0xfe30, 0xfe3f, MRA_NOP          },  /* &30-&3f  84LS161       Paged ROM selector              */
	{ 0xfe40, 0xfe5f, via_0_r          },  /* &40-&5f  6522 VIA      SYSTEM VIA                      */
	{ 0xfe60, 0xfe7f, MRA_NOP          },  /* &60-&7f  6522 VIA      1 USER VIA                      */
	{ 0xfe80, 0xfe9f, MRA_NOP          },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller        */
	{ 0xfea0, 0xfebf, MRA_NOP          },  /* &a0-&bf  68B54 ADLC    1 ECONET controller             */
	{ 0xfec0, 0xfedf, MRA_NOP          },  /* &c0-&df  uPD7002       1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MRA_NOP          },  /* &e0-&ff  Tube ULA      1 Tube system interface         */
	{ 0xff00, 0xffff, MRA_ROM          },  /* Hardware marked with a 1 is not present in a Model A   */
    {-1}
};

static struct MemoryWriteAddress writemem_bbca[] =
{
	{ 0x0000, 0x3fff, memory_w         },
	{ 0x4000, 0x7fff, memory_w         },  /* this is a repeat of the memory at 0x0000 to 0x3fff     */
	{ 0x8000, 0xdfff, MWA_NOP          },
	{ 0xc000, 0xfdff, MWA_NOP          },
	{ 0xfc00, 0xfdff, MWA_NOP          },  /* FRED & JIM Pages */
				                           /* Shiela Address Page &fe00 - &feff                      */
	{ 0xfe00, 0xfe07, BBC_6845_w       },  /* &00-&07  6845 CRTC     Video controller                */
	{ 0xfe08, 0xfe0f, MWA_NOP          },  /* &08-&0f  6850 ACIA     Serial Controller               */
	{ 0xfe10, 0xfe1f, MWA_NOP          },  /* &10-&1f  Serial ULA    Serial system chip              */
	{ 0xfe20, 0xfe2f, videoULA_w       },  /* &20-&2f  Video ULA     Video system chip               */
	{ 0xfe30, 0xfe3f, page_selecta_w   },  /* &30-&3f  84LS161       Paged ROM selector              */
	{ 0xfe40, 0xfe5f, via_0_w          },  /* &40-&5f  6522 VIA      SYSTEM VIA                      */
	{ 0xfe60, 0xfe7f, MWA_NOP          },  /* &60-&7f  6522 VIA      1 USER VIA                      */
	{ 0xfe80, 0xfe9f, MWA_NOP          },  /* &80-&9f  8271/1770 FDC 1 Floppy disc controller        */
	{ 0xfea0, 0xfebf, MWA_NOP          },  /* &a0-&bf  68B54 ADLC    1 ECONET controller             */
	{ 0xfec0, 0xfedf, MWA_NOP          },  /* &c0-&df  uPD7002       1 Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP          },  /* &e0-&ff  Tube ULA      1 Tube system interface         */
	{ 0xff00, 0xffff, MWA_ROM          },  /* Hardware marked with a 1 is not present in a Model A   */
    {-1}
};


static struct MemoryReadAddress readmem_bbcb[] =
{
	{ 0x0000, 0x7fff, MRA_RAM          },
	{ 0x8000, 0xbfff, MRA_BANK2        },
	{ 0xc000, 0xfbff, MRA_ROM          },
	{ 0xfc00, 0xfdff, MRA_NOP          },  /* FRED & JIM Pages */
				                           /* Shiela Address Page &fe00 - &feff                    */
	{ 0xfe00, 0xfe07, BBC_6845_r       },  /* &00-&07  6845 CRTC     Video controller              */
	{ 0xfe08, 0xfe0f, MRA_NOP          },  /* &08-&0f  6850 ACIA     Serial Controller             */
	{ 0xfe10, 0xfe1f, MRA_NOP          },  /* &10-&1f  Serial ULA    Serial system chip            */
	{ 0xfe20, 0xfe2f, videoULA_r       },  /* &20-&2f  Video ULA     Video system chip             */
	{ 0xfe30, 0xfe3f, MRA_NOP          },  /* &30-&3f  84LS161       Paged ROM selector            */
	{ 0xfe40, 0xfe5f, via_0_r          },  /* &40-&5f  6522 VIA      SYSTEM VIA                    */
	{ 0xfe60, 0xfe7f, via_1_r          },  /* &60-&7f  6522 VIA      USER VIA                      */
	{ 0xfe80, 0xfe9f, bbc_wd1770_read  },  /* &80-&9f  8271/1770 FDC Floppy disc controller        */
//	{ 0xfe80, 0xfe9f, bbc_i8271_read   },
	{ 0xfea0, 0xfebf, MRA_NOP          },  /* &a0-&bf  68B54 ADLC    ECONET controller             */
	{ 0xfec0, 0xfedf, MRA_NOP          },  /* &c0-&df  uPD7002       Analogue to digital converter */
	{ 0xfee0, 0xfeff, MRA_NOP          },  /* &e0-&ff  Tube ULA      Tube system interface         */
	{ 0xff00, 0xffff, MRA_ROM          },
    {-1}
};

static struct MemoryWriteAddress writemem_bbcb[] =
{
	{ 0x0000, 0x7fff, memory_w         },
	{ 0x8000, 0xdfff, MWA_NOP          },
	{ 0xc000, 0xfdff, MWA_NOP          },
	{ 0xfc00, 0xfdff, MWA_NOP          },  /* FRED & JIM Pages */
				                           /* Shiela Address Page &fe00 - &feff                    */
	{ 0xfe00, 0xfe07, BBC_6845_w       },  /* &00-&07  6845 CRTC     Video controller              */
	{ 0xfe08, 0xfe0f, MWA_NOP          },  /* &08-&0f  6850 ACIA     Serial Controller             */
	{ 0xfe10, 0xfe1f, MWA_NOP          },  /* &10-&1f  Serial ULA    Serial system chip            */
	{ 0xfe20, 0xfe2f, videoULA_w       },  /* &20-&2f  Video ULA     Video system chip             */
	{ 0xfe30, 0xfe3f, page_selectb_w   },  /* &30-&3f  84LS161       Paged ROM selector            */
	{ 0xfe40, 0xfe5f, via_0_w          },  /* &40-&5f  6522 VIA      SYSTEM VIA                    */
	{ 0xfe60, 0xfe7f, via_1_w          },  /* &60-&7f  6522 VIA      USER VIA                      */
	{ 0xfe80, 0xfe9f, bbc_wd1770_write },  /* &80-&9f  8271/1770 FDC Floppy disc controller        */
//	{ 0xfe80, 0xfe9f, bbc_i8271_write  },
	{ 0xfea0, 0xfebf, MWA_NOP          },  /* &a0-&bf  68B54 ADLC    ECONET controller             */
	{ 0xfec0, 0xfedf, MWA_NOP          },  /* &c0-&df  uPD7002       Analogue to digital converter */
	{ 0xfee0, 0xfeff, MWA_NOP          },  /* &e0-&ff  Tube ULA      Tube system interface         */
	{ 0xff00, 0xffff, MWA_ROM          },
    {-1}
};

unsigned short bbc_colour_table[8]=
{
	0,1,2,3,4,5,6,7
};

unsigned char   bbc_palette[8*3]=
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


INPUT_PORTS_END

/* the BBC came with 4 rom sockets on the mother board as shown in the model A driver */
/* you could get a number of rom upgrade boards that took this up to 16 roms as in the */
/* model B driver */

ROM_START(bbca)
	ROM_REGION(0x20000,REGION_CPU1) /* ROM MEMORY */

	ROM_LOAD("os12.rom",    0x0C000, 0x4000, 0x3c14fc70 )

														  /* rom page 0  10000 */
														  /* rom page 1  14000 */
														  /* rom page 2  18000 */
	ROM_LOAD("basic2.rom",  0x1c000, 0x4000, 0x79434781 ) /* rom page 3  1c000 */
ROM_END


/*  0000- 7fff  ram */
/*  8000- bfff  not used, this area is mapped over with one of the roms at 10000 and above */
/*  c000- ffff  OS rom and memory mapped hardware at fc00-feff */
/* 10000-4ffff  16 paged rom banks mapped back into 8000-bfff by the page rom select */

/* I have only plugged a few roms in */
ROM_START(bbcb)
	ROM_REGION(0x50000,REGION_CPU1) /* ROM MEMORY */

	ROM_LOAD("os12.rom",    0x0C000, 0x4000, 0x3c14fc70 )


														  /* rom page 0  10000 */
														  /* rom page 1  14000 */
														  /* rom page 2  18000 */
														  /* rom page 3  1c000 */
	                                                      /* rom page 4  20000 */
	                                                      /* rom page 5  24000 */
	                                                      /* rom page 6  28000 */
	                                                      /* rom page 7  2c000 */
														  /* rom page 8  30000 */
														  /* rom page 9  34000 */
														  /* rom page 10 38000 */
														  /* rom page 11 3c000 */
	                                                      /* rom page 12 40000 */
	                                                      /* rom page 13 44000 */
	                                                      /* rom page 14 48000 */
	ROM_LOAD("basic2.rom",  0x4c000, 0x4000, 0x79434781 ) /* rom page 15 4c000 */


ROM_END


static int bbcb_vsync(void)
{
	via_0_ca1_w(0,0);
	via_0_ca1_w(0,1);
	check_disc_status();
	return 0;
}


static struct SN76496interface sn76496_interface =
{
	1,      /* 1 chip */
	{ 4000000 },    /* 4Mhz */
	{ 100 }
};

static struct MachineDriver machine_driver_bbca =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			2000000,        /* 2.00Mhz */
			readmem_bbca,
			writemem_bbca,
			0,
			0,

			bbcb_vsync,     1,              /* screen refresh interrupts */
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
			2000000,        /* 2.00Mhz */
			readmem_bbcb,
			writemem_bbcb,
			0,
			0,

			bbcb_vsync,     1,              /* screen refresh interrupts */
			bbcb_keyscan, 1000              /* scan keyboard */
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



static const struct IODevice io_bbca[] = {
	{
   		IO_CASSETTE,        /* type */
   		1,                  /* count */
   		"wav\0",    /* File extensions */
   		NULL,               /* private */
   		NULL,               /* id */
   		NULL,	/* init */
   		NULL,	/* exit */
   		NULL,               /* info */
   		NULL,               /* open */
   		NULL,               /* close */
   		NULL,               /* status */
   		NULL,               /* seek */
   		NULL,			   /* tell */
   		NULL,               /* input */
   		NULL,               /* output */
   		NULL,               /* input_chunk */
   		NULL                /* output_chunk */
	},
    { IO_END }
};

static const struct IODevice io_bbcb[] = {
	{
   		IO_CASSETTE,        /* type */
   		1,                  /* count */
   		"wav\0",    		/* File extensions */
   		NULL,               /* private */
   		NULL,               /* id */
   		NULL,				/* init */
   		NULL,				/* exit */
   		NULL,               /* info */
   		NULL,               /* open */
   		NULL,               /* close */
   		NULL,               /* status */
   		NULL,               /* seek */
   		NULL,			   	/* tell */
   		NULL,               /* input */
   		NULL,               /* output */
   		NULL,               /* input_chunk */
   		NULL                /* output_chunk */
	},	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"ssd\0",            /* file extensions */
		NULL,               /* private */
		NULL,				/* id */
		bbc_floppy_init,	/* init */
		bbc_floppy_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
    },
    { IO_END }
};


/*     year name	parent	machine	input	init	company */
COMPX (1981,bbca,	0,		bbca,	bbca,	0,	"Acorn","BBC Micro Model A",GAME_NOT_WORKING )
COMPX (1981,bbcb,	bbca,	bbcb,	bbca,	0,	"Acorn","BBC Micro Model B",GAME_NOT_WORKING )
