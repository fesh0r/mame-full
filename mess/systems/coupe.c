/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton


	Sam Coupe Memory Map - Based around the current spectrum.c (for obvious reasons!!)

	CPU:
		0000-7fff Banked rom/ram
		8000-ffff Banked rom/ram


Interrupts:

Changes:

 V0.2	- Added FDC support. - Based on 1771 document. Coupe had a 1772... (any difference?)
		  	floppy supports only read sector single mode at present will add write sector
			in next version.
		  Fixed up palette - had red & green wrong way round.


 KT 26-Aug-2000 - Changed to use wd179x code. This is the same as the 1772.
                - Coupe supports the basic disk image format, but can be changed in
                  the future to support others
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/coupe.h"
#include "includes/wd179x.h"
#include "includes/basicdsk.h"

static struct MemoryReadAddress coupe_readmem[] = {
	{ 0x0000, 0x3FFF, MRA_BANK1 },
	{ 0x4000, 0x7FFF, MRA_BANK2 },
	{ 0x8000, 0xBFFF, MRA_BANK3 },
	{ 0xC000, 0xFFFF, MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress coupe_writemem[] = {
	{ 0x0000, 0x3FFF, MWA_BANK1 },
	{ 0x4000, 0x7FFF, MWA_BANK2 },
	{ 0x8000, 0xBFFF, MWA_BANK3 },
	{ 0xC000, 0xFFFF, MWA_BANK4 },
	{ -1 }  /* end of table */
};

int coupe_line_interrupt(void)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;
	int interrupted=0;	/* This is used to allow me to clear the STAT flag (easiest way I can do it!) */

	HPEN = CURLINE;

	if (LINE_INT<192)
	{
		if (CURLINE == LINE_INT)
		{
			/* No other interrupts can occur - NOT CORRECT!!! */
            STAT=0x1E;  
			cpu_cause_interrupt(0, Z80_IRQ_INT);
			interrupted=1;
		}
	}

	/* scan line on screen so draw last scan line (may need to alter this slightly!!) */
    if (CURLINE && (CURLINE-1) < 192) 
	{
		switch ((VMPR & 0x60)>>5)
		{
		case 0: /* mode 1 */
			drawMode1_line(bitmap,(CURLINE-1));
			break;
		case 1: /* mode 2 */
			drawMode2_line(bitmap,(CURLINE-1));
			break;
		case 2: /* mode 3 */
			drawMode3_line(bitmap,(CURLINE-1));
			break;
		case 3: /* mode 4 */
			drawMode4_line(bitmap,(CURLINE-1));
			break;
		}
	}

	CURLINE = (CURLINE + 1) % (192+10);

	if (CURLINE == 193)
	{
		if (interrupted)
			STAT&=~0x08;
		else
			STAT=0x17;

		cpu_cause_interrupt(0, Z80_IRQ_INT);
		interrupted=1;
	}

	if (!interrupted)
		STAT=0x1F;

	return ignore_interrupt();
}

unsigned char getSamKey1(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
		result &=readinputport(8) & 0x1F;
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0x1F;
		if (hi&0x40) result &= readinputport(6) & 0x1F;
		if (hi&0x20) result &= readinputport(5) & 0x1F;
		if (hi&0x10) result &= readinputport(4) & 0x1F;
		if (hi&0x08) result &= readinputport(3) & 0x1F;
		if (hi&0x04) result &= readinputport(2) & 0x1F;
		if (hi&0x02) result &= readinputport(1) & 0x1F;
		if (hi&0x01) result &= readinputport(0) & 0x1F;
	}

	return result;
}

unsigned char getSamKey2(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
	{
		/* does not map to any keys? */
	}
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0xE0;
		if (hi&0x40) result &= readinputport(6) & 0xE0;
		if (hi&0x20) result &= readinputport(5) & 0xE0;
		if (hi&0x10) result &= readinputport(4) & 0xE0;
		if (hi&0x08) result &= readinputport(3) & 0xE0;
		if (hi&0x04) result &= readinputport(2) & 0xE0;
		if (hi&0x02) result &= readinputport(1) & 0xE0;
		if (hi&0x01) result &= readinputport(0) & 0xE0;
	}

	return result;
}


READ_HANDLER( coupe_port_r )
{
    if (offset==SSND_ADDR)  /* Sound address request */
		return SOUND_ADDR;

	if (offset==HPEN_PORT)
		return HPEN;

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:	/* This covers the total range of ports for 1 floppy controller */
    case DSK1_PORT+4:
		wd179x_set_side((offset >> 2) & 1);
		return wd179x_status_r(0);
	case DSK1_PORT+1:
    case DSK1_PORT+5:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_track_r(0);
	case DSK1_PORT+2:
    case DSK1_PORT+6:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_sector_r(0);
	case DSK1_PORT+3:
	case DSK1_PORT+7:
		wd179x_set_side((offset >> 2) & 1);
        return wd179x_data_r(0);
	case LPEN_PORT:
		return LPEN;
	case STAT_PORT:
		return ((getSamKey2((offset >> 8)&0xFF))&0xE0) | STAT;
	case LMPR_PORT:
		return LMPR;
	case HMPR_PORT:
		return HMPR;
	case VMPR_PORT:
		return VMPR;
	case KEYB_PORT:
		return (getSamKey1((offset >> 8)&0xFF)&0x1F) | 0xE0;
	case SSND_DATA:
		return SOUND_REG[SOUND_ADDR];
	default:
		logerror("Read Unsupported Port: %04x\n", offset);
		break;
	}

	return 0x0ff;
}


WRITE_HANDLER( coupe_port_w )
{
	if (offset==SSND_ADDR)						// Set sound address
	{
		SOUND_ADDR=data&0x1F;					// 32 registers max
		saa1099_control_port_0_w(0, SOUND_ADDR);
        return;
	}

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:							// This covers the total range of ports for 1 floppy controller
    case DSK1_PORT+4:
		wd179x_set_side((offset >> 2) & 1);
        wd179x_command_w(0, data);
		break;
    case DSK1_PORT+1:
    case DSK1_PORT+5:
		/* Track byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_track_w(0, data);
		break;
    case DSK1_PORT+2:
    case DSK1_PORT+6:
		/* Sector byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_sector_w(0, data);
        break;
    case DSK1_PORT+3:
	case DSK1_PORT+7:
		/* Data byte requested on address line */
		wd179x_set_side((offset >> 2) & 1);
        wd179x_data_w(0, data);
		break;
	case CLUT_PORT:
		CLUT[(offset >> 8)&0x0F]=data&0x7F;		// set CLUT data
		break;
	case LINE_PORT:
		LINE_INT=data;						// Line to generate interrupt on
		break;
    case LMPR_PORT:
		LMPR=data;
		coupe_update_memory();
		break;
    case HMPR_PORT:
		HMPR=data;
		coupe_update_memory();
		break;
    case VMPR_PORT:
		VMPR=data;
		coupe_update_memory();
		break;
    case BORD_PORT:
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
		break;
    case SSND_DATA:
		saa1099_write_port_0_w(0, data);
		SOUND_REG[SOUND_ADDR] = data;
		break;
    default:
		logerror("Write Unsupported Port: %04x,%02x\n", offset,data);
		break;
	}
}

static struct IOReadPort coupe_readport[] = {
	{0x0000, 0x0ffff, coupe_port_r},
	{ -1 }
};

static struct IOWritePort coupe_writeport[] = {
	{0x0000, 0x0ffff, coupe_port_w},
	{ -1 }
};

static struct GfxDecodeInfo coupe_gfxdecodeinfo[] = {
	{ -1 } /* end of array */
};

INPUT_PORTS_START( coupe )
	PORT_START // FE  0
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",KEYCODE_F3, IP_JOY_NONE )

	PORT_START // FD  1
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",KEYCODE_F5, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6",KEYCODE_F6, IP_JOY_NONE )

	PORT_START // FB  2
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F7",KEYCODE_F7, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F8",KEYCODE_F8, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F9",KEYCODE_F9, IP_JOY_NONE )

	PORT_START // F7  3
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC",KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB",KEYCODE_TAB, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK",KEYCODE_CAPSLOCK, IP_JOY_NONE )

	PORT_START // EF  4
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "+", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE",KEYCODE_BACKSPACE, IP_JOY_NONE )

	PORT_START // DF  5
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_OPENBRACE, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0", KEYCODE_F10, IP_JOY_NONE )

	PORT_START // BF  6
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "EDIT", KEYCODE_RALT, IP_JOY_NONE )

	PORT_START // 7F  7
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYMBOL", KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "INV", KEYCODE_SLASH, IP_JOY_NONE )

	PORT_START // FF  8
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LALT,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT,  IP_JOY_NONE )

INPUT_PORTS_END

/* Initialise the palette */
static void coupe_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	unsigned char red,green,blue;
	int a;
	unsigned char coupe_palette[128*3];
	unsigned short coupe_colortable[128];		// 1-1 relationship to palette!

	for (a=0;a<128;a++)
	{
		/* decode colours for palette as follows :
		 * bit number		7		6		5		4		3		2		1		0
		 *						|		|		|		|		|		|		|
		 *				 nothing   G+4	   R+4	   B+4	  ALL+1    G+2	   R+2	   B+2
		 *
		 * these values scaled up to 0-255 range would give modifiers of :	+4 = +(4*36), +2 = +(2*36), +1 = *(1*36)
		 * not quite max of 255 but close enough for me!
		 */
		red=green=blue=0;
		if (a&0x01)
			blue+=2*36;
		if (a&0x02)
			red+=2*36;
		if (a&0x04)
			green+=2*36;
		if (a&0x08)
		{
			red+=1*36;
			green+=1*36;
			blue+=1*36;
		}
		if (a&0x10)
			blue+=4*36;
		if (a&0x20)
			red+=4*36;
		if (a&0x40)
			green+=4*36;

		coupe_palette[a*3+0]=red;
		coupe_palette[a*3+1]=green;
		coupe_palette[a*3+2]=blue;

		coupe_colortable[a]=a;
	}

	memcpy(sys_palette,coupe_palette,sizeof(coupe_palette));
	memcpy(sys_colortable,coupe_colortable,sizeof(coupe_colortable));
}

static struct Speaker_interface coupe_speaker_interface=
{
	1,
	{50},
};

static struct SAA1099_interface coupe_saa1099_interface=
{
	1,
	{{50,50}},
};

static struct MachineDriver machine_driver_coupe256 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			6000000,        /* 6 Mhz */
		    coupe_readmem,coupe_writemem,
			coupe_readport,coupe_writeport,
			coupe_line_interrupt,192 + 10,			/* 192 scanlines + 10 lines of vblank (approx).. */
		},
	},
	50, 0,								/* frames per second, vblank duration */
	1,
	coupe_init_machine_256,
	coupe_shutdown_machine,

	/* video hardware */
	64*8,                               /* screen width */
	24*8,                               /* screen height */
	{ 0, 64*8-1, 0, 24*8-1 },           /* visible_area */
	coupe_gfxdecodeinfo,				/* graphics decode info */
	128, 128,							/* colors used for the characters */
	coupe_init_palette,					/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	coupe_vh_start,
	coupe_vh_stop,
	coupe_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* standard spectrum sound */
		{
			SOUND_SPEAKER,
			&coupe_speaker_interface
		},
		{
			SOUND_SAA1099,
			&coupe_saa1099_interface
		},
    }

};

static struct MachineDriver machine_driver_coupe512 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			6000000,        /* 6 Mhz */
		    coupe_readmem,coupe_writemem,
			coupe_readport,coupe_writeport,
			coupe_line_interrupt,192 + 10,	/* 192 scanlines + 10 lines of vblank (approx).. */

		},
	},
	50, 0,	/* frames per second, vblank duration */
	1,
	coupe_init_machine_512,
	coupe_shutdown_machine,

	/* video hardware */
	64*8,                               /* screen width */
	24*8,                               /* screen height */
	{ 0, 64*8-1, 0, 24*8-1 },           /* visible_area */
	coupe_gfxdecodeinfo,				/* graphics decode info */
	128, 128,							/* colors used for the characters */
	coupe_init_palette,					/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	coupe_vh_start,
	coupe_vh_stop,
	coupe_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* standard spectrum sound */
		{
			SOUND_SPEAKER,
			&coupe_speaker_interface
		},
		{
			SOUND_SAA1099,
			&coupe_saa1099_interface
        },
    }

};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(coupe)
	ROM_REGION(0x48000,REGION_CPU1)
	ROM_LOAD("sam_rom0.rom", 0x40000, 0x4000, 0x9954CF1A)
	ROM_LOAD("sam_rom1.rom", 0x44000, 0x4000, 0xF031AED4)
ROM_END

ROM_START(coupe512)
	ROM_REGION(0x88000,REGION_CPU1)
	ROM_LOAD("sam_rom0.rom", 0x80000, 0x4000, 0x9954CF1A)
	ROM_LOAD("sam_rom1.rom", 0x84000, 0x4000, 0xF031AED4)
ROM_END

static const struct IODevice io_coupe[] =
{
	{
		IO_FLOPPY,				/* type */
		2,						/* count */
/* Only .DSK (raw dump images) are supported at present */
        "dsk\0",                /* file extensions */       
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		coupe_floppy_init,		/* init */
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
#define io_coupe256 io_coupe
#define io_coupe512 io_coupe

/*    YEAR  NAME      PARENT    MACHINE         INPUT     INIT          COMPANY                 		  FULLNAME */
COMP( 1989, coupe,	  0,		coupe256,		coupe,	  0,			"Miles Gordon Technology plc",    "Sam Coupe 256K RAM" )
COMP( 1989, coupe512, 0,		coupe512,		coupe,	  0,			"Miles Gordon Technology plc",    "Sam Coupe 512K RAM" )
