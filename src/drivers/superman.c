/***************************************************************************


Taito X-system

driver by Richard Bush, Howie Cohen and Yochizo


Supported games:
----------------------------------------------------
 Name                    Company               Year
  Superman                Taito Corp.           1988
  Daisenpu (Japan)        Taito Corp.           1988
  Balloon Brothers        East Technology Corp. 1990
  Gigandes                East Technology Corp. 1990

Please tell me the games worked on this board.


Memory map:
----------------------------------------------------
  0x000000 - 0x07ffff : ROM
  0x300000   ??
  0x400000   ??
  0x500000 - 0x50000f : Dipswitches a & b, 4 bits to each word
  0x600000   ?? 0, 10, 0x4001, 0x4006
  0x700000   ??
  0x800000 - 0x800003 : sound chip
  0x900000 - 0x900fff : c-chip shared RAM space
  0xb00000 - 0xb00fff : palette RAM, words in the format xRRRRRGGGGGBBBBB
  0xc00000   ??
  0xd00000 - 0xd007ff : video attribute RAM
      0000 - 03ff : sprite y coordinate
      0400 - 07ff : tile x & y scroll
  0xe00000 - 0xe00fff : object RAM bank 1
      0000 - 03ff : sprite number (bit mask 0x3fff)
                    sprite y flip (bit mask 0x4000)
                    sprite x flip (bit mask 0x8000)
      0400 - 07ff : sprite x coordinate (bit mask 0x1ff)
                    sprite color (bit mask 0xf800)
      0800 - 0bff : tile number (bit mask 0x3fff)
                    tile y flip (bit mask 0x4000)
                    tile x flip (bit mask 0x8000)
      0c00 - 0fff : tile color (bit mask 0xf800)
  0xe01000 - 0xe01fff : unused(?) portion of object RAM
  0xe02000 - 0xe02fff : object RAM bank 2
  0xe03000 - 0xe03fff : unused(?) portion of object RAM

Interrupt:
----------------------------------------------------
  IRQ level 6 : Superman
  IRQ level 2 : Daisenpu, Balloon Brothers, Gigandes
  
Screen resolution:
----------------------------------------------------
  384 x 240   : Superman, Balloon Brothers, Gigandes
  384 x 224   : Daisenpu

Sound chip:
----------------------------------------------------
  YM2610 x 1   : Superman, Balloon Brothers, Gigandes
  YM2151 x 1   : Daisenpu

Known problems:
----------------------------------------------------
  - Daisenpu may have wrong road color due to incorrect backdrop
    color emulation.
  - A music tempo of Dansenpu is a bit wrong.
  - Gigandes appears background tile glitches after the demo of a cave stage.

TODO:
----------------------------------------------------
  - Fixed upper problems.
  - Correct TC0140SYT emulation.
  - Come out the games which is worked on this board.


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"

/* If you get rid of following comment, the music tempo of Daisenpu is correct. */
/* However, it is sometimes lost because interrupt handler may be wrong.        */
//#define DAISENPU_INTERRUPT_TRIGGED_FROM_YM2151


WRITE_HANDLER( supes_attribram_w );
READ_HANDLER( supes_attribram_r );
WRITE_HANDLER( supes_videoram_w );
READ_HANDLER( supes_videoram_r );
void superman_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
int superman_vh_start (void);
void superman_vh_stop (void);
int ballbros_vh_start (void);
int daisenpu_vh_start (void);

extern size_t supes_videoram_size;
extern size_t supes_attribram_size;

extern unsigned char *supes_videoram;
extern unsigned char *supes_attribram;

static unsigned char *ram; /* for high score save */

extern unsigned char *cchip_ram;

void cchip1_init_machine(void);
READ_HANDLER( cchip1_r );
WRITE_HANDLER( cchip1_w );

#define TC0140SYT_PORT01_FULL         (0x01)
#define TC0140SYT_PORT23_FULL         (0x02)
#define TC0140SYT_SET_OK              (0x00)
#define TC0140SYT_PORT01_FULL_S       (0x04)
#define TC0140SYT_PORT23_FULL_S       (0x08)

typedef struct TC0140SYT
{
	unsigned char portdata[4];	// Data on port (4 nibbles)
	unsigned char mainmode;		// Access mode on main cpu side
	unsigned char submode;		// Access mode on sub cpu side
	unsigned char status;		// Status read data
	unsigned char subnmi;		// 1 if sub cpu has nmi's enabled
	unsigned char subwantnmi;	// 1 if sub cpu has a pending nmi
} TC0140SYT;

static struct TC0140SYT tc0140syt;


#ifndef DAISENPU_INTERRUPT_TRIGGED_FROM_YM2151
int daisenpu_sound_interrupt(void)
{
	return Z80_IRQ_INT;
}
#endif

READ_HANDLER( superman_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport (0) & 0x0f;
		case 0x02:
			return ( readinputport (0) & 0xf0 ) >> 4;
		case 0x04:
			return readinputport (1) & 0x0f;
		case 0x06:
			return ( readinputport (1) & 0xf0 ) >> 4;
		default:
			logerror("superman_input_r offset: %04x\n", offset);
			return 0x00;
	}
}

static WRITE_HANDLER( taito68k_sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	int banknum = ( data - 1 ) & 3;

	cpu_setbank( 2, &RAM[ 0x10000 + ( banknum * 0x4000 ) ] );
}

READ_HANDLER( tc0140syt_mastercpu_r )
{
	static int toggle = 0;
	
	if ( !(offset & 2) )
	{
		logerror("tc0140syt : Master cpu read control port\n");
		toggle ^= 0xff;
		return toggle;
	}
	else
	{
		switch( tc0140syt.mainmode )
		{
			case 0x00:		// mode #0
				logerror("tc0140syt : Master cpu read portdata %01x\n", tc0140syt.portdata[0]);
				return tc0140syt.portdata[tc0140syt.mainmode ++];
				break;
				
			case 0x01:		// mode #1
				logerror("tc0140syt : Master cpu receives 0/1 : %01x%01x\n", tc0140syt.portdata[1],tc0140syt.portdata[0]);
				tc0140syt.status &= ~TC0140SYT_PORT01_FULL_S;
				return tc0140syt.portdata[tc0140syt.mainmode ++];
				break;
				
			case 0x02:		// mode #2
				logerror("tc0140syt : Master cpu read portdata %01x\n", tc0140syt.portdata[2]);
				return tc0140syt.portdata[tc0140syt.mainmode ++];
				break;
				
			case 0x03:		// mode #3
				logerror("tc0140syt : Master cpu receives 2/3 : %01x%01x\n", tc0140syt.portdata[3],tc0140syt.portdata[2]);
				tc0140syt.status &= ~TC0140SYT_PORT23_FULL_S;
				return tc0140syt.portdata[tc0140syt.mainmode ++];
				break;
				
			case 0x04:		// port status
				logerror("tc0140syt : Master cpu read status : %02x\n", tc0140syt.status);
				return tc0140syt.status;
				break;
				
			default:
				logerror("tc0140syt : Master cpu read in mode [%02x]\n", tc0140syt.mainmode);
				toggle ^= 0xff;
				return toggle;
		}
	}
}

WRITE_HANDLER( tc0140syt_mastercpu_w )
{
	if ( !(offset & 2) )
	{
		// TODO : overflow handler
		tc0140syt.mainmode = data & 0xff;
		logerror("tc0140syt : Master cpu mode [%02x]\n", data & 0xff);
		if (data > 4)
		{
			logerror("tc0140syt : error 68000 entering unknown mode[%02x]\n", data & 0xff);
		}
	}
	else
	{
		switch( tc0140syt.mainmode )
		{
			case 0x00:		// mode #0
				tc0140syt.portdata[tc0140syt.mainmode ++] = data & 0x0f;
				logerror("tc0140syt : Master cpu written port 0, data %01x\n", data & 0x0f);
				break;
				
			case 0x01:		// mode #1
				tc0140syt.portdata[tc0140syt.mainmode ++] = data & 0x0f;
				tc0140syt.status |= TC0140SYT_PORT01_FULL;
				if (tc0140syt.subnmi)
					tc0140syt.subwantnmi = 1;
				logerror("Master cpu sends 0/1 : %01x%01x\n",tc0140syt.portdata[1],tc0140syt.portdata[0]);
         		break;
				
			case 0x02:		// mode #2
				tc0140syt.portdata[tc0140syt.mainmode ++] = data & 0x0f;
				logerror("tc0140syt : Master cpu written port 2, data %01\n", data & 0x0f);
				break;
				
			case 0x03:		// mode #3
				tc0140syt.portdata[tc0140syt.mainmode ++] = data & 0x0f;
				tc0140syt.status |= TC0140SYT_PORT23_FULL;
				if (tc0140syt.subnmi)
					tc0140syt.subwantnmi = 1;
				logerror("Master cpu sends 2/3 : %01x%01x\n",tc0140syt.portdata[3],tc0140syt.portdata[2]);
				break;
				
			case 0x04:		// port status
				tc0140syt.status = TC0140SYT_SET_OK;
				logerror("tc0140syt : Master cpu status ok.\n");
				break;
				
			default:
				logerror("tc0140syt: Master cpu written in mode [%02x] data[%02x]\n",tc0140syt.mainmode, data & 0xff);
		}
	}
}

READ_HANDLER( tc0140syt_slavecpu_r )
{
	static int toggle = 0;
	
	if ( offset == 0 )
	{
		logerror("tc0140syt : Slave cpu read control port\n");
		toggle ^= 0xff;
		return toggle;
	}
	else
	{
		switch ( tc0140syt.submode )
		{
			case 0x00:		// mode #0
				logerror("tc0140syt : Slave cpu read portdata %01x\n", tc0140syt.portdata[0]);
				return tc0140syt.portdata[tc0140syt.submode ++];
				break;
				
			case 0x01:		// mode #1
				logerror("tc0140syt : Slave cpu receives 0/1 : %01x%01x\n", tc0140syt.portdata[1],tc0140syt.portdata[0]);
				tc0140syt.status &= ~TC0140SYT_PORT01_FULL;
				return tc0140syt.portdata[tc0140syt.submode ++];
				break;
				
			case 0x02:		// mode #2
				logerror("tc0140syt : Slave cpu read portdata %01x\n", tc0140syt.portdata[2]);
				return tc0140syt.portdata[tc0140syt.submode ++];
				break;
				
			case 0x03:		// mode #3
				logerror("tc0140syt : Slave cpu receives 2/3 : %01x%01x\n", tc0140syt.portdata[3],tc0140syt.portdata[2]);
				tc0140syt.status &= ~TC0140SYT_PORT23_FULL;
				return tc0140syt.portdata[tc0140syt.submode ++];
				break;
				
			case 0x04:		// port status
				logerror("tc0140syt : Slave cpu read status : %02x\n", tc0140syt.status);
				return tc0140syt.status;
				break;
				
			default:
				logerror("tc0140syt : Slave cpu read in mode [%02x]\n", tc0140syt.submode);
				toggle ^= 0xff;
				return toggle;
		}
	}
}

WRITE_HANDLER( tc0140syt_slavecpu_w )
{
	if( offset == 0 )
	{
		tc0140syt.submode = data & 0xff;
		logerror("tc0140syt : Slave cpu mode [%02x]\n", data & 0xff);
		if (tc0140syt.submode > 6)
			logerror("tc0140syt error : Slave cpu unknown mode[%02x]\n", data & 0x0f);
	}
	else
	{
		switch ( tc0140syt.submode )
		{
			case 0x00:		// mode #0
				tc0140syt.portdata[tc0140syt.submode ++] = data & 0x0f;
				logerror("tc0140syt : Slave cpu written port 0, data %01x\n", data & 0x0f);
				break;
				
			case 0x01:		// mode #1
				tc0140syt.portdata[tc0140syt.submode ++] = data & 0x0f;
				tc0140syt.status |= TC0140SYT_PORT01_FULL_S;
				logerror("Slave cpu sends 0/1 : %01x%01x\n",tc0140syt.portdata[1],tc0140syt.portdata[0]);
				break;
				
			case 0x02:		// mode #2
				logerror("tc0140syt : Slave cpu written port 2, data %01x\n", data & 0x0f);
				tc0140syt.portdata[tc0140syt.submode ++] = data & 0x0f;
				break;
				
			case 0x03:		// mode #3
				tc0140syt.portdata[tc0140syt.submode ++] = data & 0x0f;
				tc0140syt.status |= TC0140SYT_PORT23_FULL_S;
				logerror("Slave cpu sends 2/3 : %01x%01x\n",tc0140syt.portdata[3],tc0140syt.portdata[2]);
				break;
				
			case 0x04:		// port status
				tc0140syt.status = TC0140SYT_SET_OK;
				logerror("tc0140syt : Slave cpu status ok.\n");
				break;
				
			case 0x05:		// nmi disable
				tc0140syt.subnmi = 0;
				break;
				
			case 0x06:		// nmi enable
				tc0140syt.subnmi = 1;
				break;
				
			default:
				logerror("tc0140syt: Slave cpu written in mode [%02x] data[%02x]\n",tc0140syt.submode, data & 0xff);
		}
	}
}

static struct MemoryReadAddress superman_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x500000, 0x50000f, superman_input_r },	/* DSW A/B */
	{ 0x800000, 0x800003, taito_soundsys_sound_r },
//	{ 0x800000, 0x800003, tc0140syt_mastercpu_r },
	{ 0x900000, 0x900fff, cchip1_r } ,
	{ 0xb00000, 0xb00fff, paletteram_word_r },
	{ 0xd00000, 0xd007ff, supes_attribram_r },
	{ 0xe00000, 0xe03fff, supes_videoram_r },
	{ 0xf00000, 0xf03fff, MRA_BANK1 },	/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress superman_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x800000, 0x800003, taito_soundsys_sound_w },
//	{ 0x800000, 0x800003, tc0140syt_mastercpu_w },
	{ 0x900000, 0x900fff, cchip1_w },
	{ 0xb00000, 0xb00fff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0xd00000, 0xd007ff, supes_attribram_w, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, supes_videoram_w, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA_BANK1, &ram },			/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ballbros_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x500000, 0x50000f, superman_input_r },	/* DSW A/B */
//	{ 0x800000, 0x800003, taito_soundsys_sound_r },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_r },
	{ 0x900000, 0x900fff, cchip1_r } ,
	{ 0xb00000, 0xb00fff, paletteram_word_r },
	{ 0xd00000, 0xd007ff, supes_attribram_r },
	{ 0xe00000, 0xe03fff, supes_videoram_r },
	{ 0xf00000, 0xf03fff, MRA_BANK1 },	/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ballbros_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
//	{ 0x800000, 0x800003, taito_soundsys_sound_w },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_w },
	{ 0x900000, 0x900fff, cchip1_w },
	{ 0xb00000, 0xb00fff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0xd00000, 0xd007ff, supes_attribram_w, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, supes_videoram_w, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA_BANK1, &ram },			/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress gigandes_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x500000, 0x50000f, superman_input_r },	/* DSW A/B */
//	{ 0x800000, 0x800003, taito_soundsys_sound_r },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_r },
	{ 0x900000, 0x900fff, cchip1_r } ,
	{ 0xb00000, 0xb00fff, paletteram_word_r },
	{ 0xd00000, 0xd007ff, supes_attribram_r },
	{ 0xe00000, 0xe03fff, supes_videoram_r },
	{ 0xf00000, 0xf03fff, MRA_BANK1 },	/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress gigandes_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
//	{ 0x800000, 0x800003, taito_soundsys_sound_w },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_w },
	{ 0x900000, 0x900fff, cchip1_w },
	{ 0xb00000, 0xb00fff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0xd00000, 0xd007ff, supes_attribram_w, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, supes_videoram_w, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA_BANK1, &ram },			/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress daisenpu_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x500000, 0x50000f, superman_input_r },	/* DSW A/B */
//	{ 0x800000, 0x800003, taito_soundsys_sound_r },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_r },
	{ 0x900000, 0x900fff, cchip1_r } ,
	{ 0xb00000, 0xb00fff, paletteram_word_r },
	{ 0xd00000, 0xd00fff, supes_attribram_r },
	{ 0xe00000, 0xe03fff, supes_videoram_r },
	{ 0xf00000, 0xf03fff, MRA_BANK1 },	/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress daisenpu_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
//	{ 0x800000, 0x800003, taito_soundsys_sound_w },
	{ 0x800000, 0x800003, tc0140syt_mastercpu_w },
	{ 0x900000, 0x900fff, cchip1_w },
	{ 0xb00000, 0xb00fff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0xd00000, 0xd00fff, supes_attribram_w, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, supes_videoram_w, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA_BANK1, &ram },			/* Main RAM */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress superman_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taito_soundsys_a001_r },
//	{ 0xe200, 0xe201, tc0140syt_slavecpu_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress superman_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taito_soundsys_a000_w },
	{ 0xe201, 0xe201, taito_soundsys_a001_w },
//	{ 0xe200, 0xe201, tc0140syt_slavecpu_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, taito68k_sound_bankswitch_w }, /* bankswitch? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ballbros_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe003, 0xe003, YM2610_read_port_0_r },
	{ 0xe200, 0xe200, MRA_NOP },
//	{ 0xe201, 0xe201, taito_soundsys_a001_r },
	{ 0xe200, 0xe201, tc0140syt_slavecpu_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ballbros_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
//	{ 0xe200, 0xe200, taito_soundsys_a000_w },
//	{ 0xe201, 0xe201, taito_soundsys_a001_w },
	{ 0xe200, 0xe201, tc0140syt_slavecpu_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, taito68k_sound_bankswitch_w }, /* bankswitch? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress daisenpu_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe001, YM2151_status_port_0_r },
//	{ 0xe200, 0xe200, MRA_NOP },
//	{ 0xe201, 0xe201, taito_soundsys_a001_r },
	{ 0xe200, 0xe201, tc0140syt_slavecpu_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress daisenpu_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
//	{ 0xe200, 0xe200, taito_soundsys_a000_w },
//	{ 0xe201, 0xe201, taito_soundsys_a001_w },
	{ 0xe200, 0xe201, tc0140syt_slavecpu_w },
	{ 0xe400, 0xe403, MWA_NOP },
//	{ 0xee00, 0xee00, MWA_NOP },
//	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf200, 0xf200, taito68k_sound_bankswitch_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( superman )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "50k and every 150k" )
	PORT_DIPSETTING(    0x04, "Bonus 2??" )
	PORT_DIPSETTING(    0x08, "Bonus 3??" )
	PORT_DIPSETTING(    0x00, "Bonus 4??" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ballbros )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gigandes )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( daisenpu )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "70k, 270k and 470k" )
	PORT_DIPSETTING(    0x08, "50k, 200k and 350k" )
	PORT_DIPSETTING(    0x04, "100k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

#define NUM_TILES 16384
static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 sprites */
	NUM_TILES,	/* 16384 of them */
	4,	       /* 4 bits per pixel */
	{ 64*8*NUM_TILES + 8, 64*8*NUM_TILES + 0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*16, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },

	64*8	/* every sprite takes 64 consecutive bytes */
};
#undef NUM_TILES

static struct GfxLayout ballbros_tilelayout =
{
	16,16,  /* 16*16 sprites */
	4096,	/* 4096 of them */
	4,	       /* 4 bits per pixel */
	{ 0x20000*3*8, 0x20000*2*8, 0x20000*1*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*8, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },

	32*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo superman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ballbros_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &ballbros_tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ballbros_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2151interface ym2151_interface =
{
	1,	/* 1 chip */
	4000000,	/* 4 MHz ?????? */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
#ifdef DAISENPU_INTERRUPT_TRIGGED_FROM_YM2151
	{ irqhandler },
#else
	{ 0 },
#endif
	{ 0 }
};

static struct MachineDriver machine_driver_superman =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			superman_readmem,superman_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			superman_sound_readmem, superman_sound_writemem,0,0,
			daisenpu_sound_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	superman_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	superman_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};


static struct MachineDriver machine_driver_ballbros =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			ballbros_readmem,ballbros_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			ballbros_sound_readmem, ballbros_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	ballbros_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ballbros_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ballbros_ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_gigandes =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			gigandes_readmem,gigandes_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			ballbros_sound_readmem, ballbros_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	superman_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	superman_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ballbros_ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_daisenpu =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			daisenpu_readmem,daisenpu_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			daisenpu_sound_readmem, daisenpu_sound_writemem,0,0,
#ifdef DAISENPU_INTERRUPT_TRIGGED_FROM_YM2151
			ignore_interrupt,0			/* IRQs are triggered by the YM2151 */
#else
			daisenpu_sound_interrupt,1
#endif
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 3*8, 31*8-1 },

	superman_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	daisenpu_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( superman )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "a10_09.bin", 0x00000, 0x20000, 0x640f1d58 )
	ROM_LOAD_ODD ( "a05_07.bin", 0x00000, 0x20000, 0xfddb9953 )
	ROM_LOAD_EVEN( "a08_08.bin", 0x40000, 0x20000, 0x79fc028e )
	ROM_LOAD_ODD ( "a03_13.bin", 0x40000, 0x20000, 0x9f446a44 )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "d18_10.bin", 0x00000, 0x4000, 0x6efe79e8 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "f01_14.bin", 0x000000, 0x80000, 0x89368c3e ) /* Plane 0, 1 */
	ROM_LOAD( "h01_15.bin", 0x080000, 0x80000, 0x910cc4f9 )
	ROM_LOAD( "j01_16.bin", 0x100000, 0x80000, 0x3622ed2f ) /* Plane 2, 3 */
	ROM_LOAD( "k01_17.bin", 0x180000, 0x80000, 0xc34f27e0 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "e18_01.bin", 0x00000, 0x80000, 0x3cf99786 )
ROM_END

ROM_START( ballbros )
	ROM_REGION( 0x40000, REGION_CPU1 )
	ROM_LOAD_EVEN( "10a", 0x00000, 0x20000, 0x4af0e858 )
	ROM_LOAD_ODD ( "5a",  0x00000, 0x20000, 0x0b983a69 )
	
	ROM_REGION( 0x1c000, REGION_CPU2 )
	ROM_LOAD( "8d", 0x00000, 0x4000, 0xd1c515af)
	ROM_CONTINUE(   0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x080000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "3", 0x000000, 0x20000, 0xec3e0537 ) /* Plane 0, 1 */
	ROM_LOAD( "2", 0x020000, 0x20000, 0xbb441717 )
	ROM_LOAD( "1", 0x040000, 0x20000, 0x8196d624 )
	ROM_LOAD( "0", 0x060000, 0x20000, 0x1cc584e5 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples 1 */
	ROM_LOAD( "east-11", 0x00000, 0x80000, 0x92111f96 )
	
	ROM_REGION( 0x80000, REGION_SOUND2 )	/* adpcm samples 2 */
	ROM_LOAD( "east-10", 0x00000, 0x80000, 0xca0ac419 )
ROM_END

ROM_START( gigandes )
	ROM_REGION( 0x80000, REGION_CPU1 )     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "1", 0x00000, 0x20000, 0x290c50e0 )
	ROM_LOAD_ODD ( "3", 0x00000, 0x20000, 0x9cef82af )
	ROM_LOAD_EVEN( "2", 0x40000, 0x20000, 0xdd94b4d0 )
	ROM_LOAD_ODD ( "4", 0x40000, 0x20000, 0xa647310a )

	ROM_REGION( 0x1c000, REGION_CPU2 )     /* 64k for Z80 code */
	ROM_LOAD( "5", 0x00000, 0x4000, 0xb24ab5f4 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "6", 0x000000, 0x80000, 0x75eece28 ) /* Plane 0, 1 */
	ROM_LOAD( "7", 0x080000, 0x80000, 0xb179a76a )
	ROM_LOAD( "9", 0x100000, 0x80000, 0x5c5e6898 ) /* Plane 2, 3 */
	ROM_LOAD( "8", 0x180000, 0x80000, 0x52db30e9 )

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* adpcm samples */
	ROM_LOAD( "11", 0x00000, 0x80000, 0x92111f96 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* adpcm samples */
	ROM_LOAD( "10", 0x00000, 0x80000, 0xca0ac419 )
ROM_END

ROM_START( daisenpu )
	ROM_REGION( 0x40000, REGION_CPU1 )
	ROM_LOAD_EVEN( "b87-06", 0x00000, 0x20000, 0xcf236100 )
	ROM_LOAD_ODD ( "b87-05", 0x00000, 0x20000, 0x7f15edc7 )
	
	ROM_REGION( 0x14000, REGION_CPU2 )
	ROM_LOAD( "b87-07", 0x00000, 0x4000, 0xe2e0efa0)
	ROM_CONTINUE(       0x10000, 0x4000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b87-02", 0x000000, 0x80000, 0x89ad43a0 ) /* Plane 0, 1 */
	ROM_LOAD( "b87-01", 0x080000, 0x80000, 0x81e82ae1)
	ROM_LOAD( "b87-04", 0x100000, 0x80000, 0x958434b6 ) /* Plane 2, 3 */
	ROM_LOAD( "b87-03", 0x180000, 0x80000, 0xce155ae0 )
ROM_END


void init_daisenpu(void)
{
	unsigned char *ROM68K = memory_region(REGION_CPU1);
	unsigned char *ROMZ80 = memory_region(REGION_CPU2);
	
	/* Fix 68000 rom check sum */
	WRITE_WORD(&ROM68K[0xc5f8], 0x4e71);
	WRITE_WORD(&ROM68K[0xc606], 0x4e71);
	
	/* Fix OBJ ram check */
	WRITE_WORD(&ROM68K[0xc6c8], 0x4e71);
	WRITE_WORD(&ROM68K[0xc6ca], 0x4e71);

	/* Fix Z80 rom check sum */
	ROMZ80[0x007f] = 0x00;
	ROMZ80[0x0080] = 0x00;
	
	memset(&tc0140syt, 0x00, sizeof( TC0140SYT ));
	tc0140syt.mainmode = 0xff;
	tc0140syt.submode  = 0xff;
}

void init_ballbros(void)
{
	memset(&tc0140syt, 0x00, sizeof( TC0140SYT ));
	tc0140syt.mainmode = 0xff;
	tc0140syt.submode  = 0xff;
}


/*   ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR      COMPANY               FULLNAME )       */
GAME( 1988, superman, 0,        superman, superman, 0,        ROT0,         "Taito Corporation", "Superman"         )
GAME( 1992, ballbros, 0,        ballbros, ballbros, ballbros, ROT0,         "East Technology"  , "Balloon Brothers" )
GAME( 1990, gigandes, 0,        gigandes, gigandes, ballbros, ROT0,         "East Technology"  , "Gigandes"         )
GAME( 1988, daisenpu, 0,        daisenpu, daisenpu, daisenpu, ROT270_16BIT, "Taito Corporation", "Daisenpu (Japan)" )
