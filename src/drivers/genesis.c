/***************************************************************************

Sega Genesis/MegaDrive Memory Map (preliminary)

Main CPU (68k)

$000000 - $3fffff : Cartridge ROM
$400000 - $9fffff : Reserved by Sega (unused?)
$a00000 - $a0ffff : shared RAM w/Z80
$a10000 - $a10fff : IO
$a11000 - $a11fff : Controls
	$a11000 : memory mode (16-bit)
	$a11100 : Z80 bus request (16)
	$a11200 : Z80 reset (16)
$c00000 - $dfffff : Video Display Processor (VDP)
	$c00000 : data (16-bit, mirrored)
	$c00004 : control (16, m)
	$c00008 : hv counter (16, m)
	$c00011 : psg 76489 (8)
$ff0000 - $ffffff : RAM

Interrupts:
	level 6 - vertical
	level 4 - horizontal
	level 2 - external

Sound CPU (Z80)

$0000 - $1fff : RAM
$2000 - $3fff : Reserved (RAM?)
$4001 - $4003 : ym2612 registers
$6000         : bank register
$7f11         : psg 76489
$8000 - $ffff : shared RAM w/68k


***************************************************************************/
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#include "driver.h"
#include "vidhrdw/generic.h"

#include "machine/genesis.h"
#include "vidhrdw/genesis.h"

#define ARMLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))

extern int data_width;

//char palmode 				= NTSC;
//char writefifo 				= FIFO_EMPTY;

int z80_68000_latch			= 0;
int	z80_latch_bitcount		= 0;

unsigned char cartridge_ram[0x1000]; /* any cartridge RAM */

void genesis_paletteram_w(int offset,int data);
int genesis_paletteram_r(int offset);
void genesis_spriteram_w(int offset,int data);
int genesis_spriteram_r(int offset);
void genesis_videoram1_w(int offset,int data);
int genesis_videoram1_r(int offset);
void genesis_videoram2_w(int offset,int data);
int genesis_videoram2_r(int offset);
void genesis_videoram3_w(int offset,int data);
int genesis_videoram3_r(int offset);
void genesis_videoram4_w(int offset,int data);
int genesis_videoram4_r(int offset);

void genesis_scrollY_w(int offset,int data);
void genesis_scrollX_w(int offset,int data);

void genesis_s_ram_w(int offset,int data);
int genesis_s_ram_r(int offset);

/* machine/genesis.c */
void genesis_init_machine (void);
int genesis_interrupt (void);
void genesis_ram_w (int offset,int data);
int genesis_ram_r (int offset);


void genesis_background_w(int offset,int data);
void genesis_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int genesis_s_interrupt(void);

void genesis_sound_port_w(int offset,int data);
int genesis_sound_port_r(int offset);
void genesis_sound_comm_w(int offset,int data);
int genesis_sound_comm_r(int offset);

int genesis_rYMport(int offset);
int genesis_rYMdata(int offset);
void genesis_wYMport(int offset,int data);
void genesis_wYMdata(int offset,int data);

int genesis_r_rd_a000(int offset);
int genesis_r_rd_a001(int offset);
void genesis_r_wr_a000(int offset,int data);
void genesis_r_wr_a001(int offset,int data);

/*void osd_ym2151_update(void); */

void genesis_r_wr_b000(int offset,int data);
void genesis_r_wr_c000(int offset,int data);
void genesis_r_wr_d000(int offset,int data);
void genesis_ym2612_w (int offset, int data);
int genesis_ym2612_r (int offset);

#if LSB_FIRST
	#define BYTE_XOR(a) ((a) ^ 1)
	
#else
	#define BYTE_XOR(a) (a)
	
#endif


int genesis_vdp_76489_r(int offset)
{
  return -1;
}
void genesis_vdp_76489_w(int offset, int data)
{
  SN76496_0_w(0, data);
}

void genesis_ramlatch_w(int offset, int data) /* note value will be meaningless unless all bits are correctly set in */
{
  	if (offset !=0 ) return;
	if (z80_latch_bitcount == 0) z80_68000_latch = 0;
/*	if (errorlog) fprintf(errorlog, "latch update\n");*/
	z80_68000_latch |= ((data & 0x01) << (15+z80_latch_bitcount));
	z80_latch_bitcount++;
	if (z80_latch_bitcount > 8)
	{
		z80_latch_bitcount = 0;
		if (errorlog) fprintf (errorlog, "latch set, value %x\n", z80_68000_latch);
	}
}
void genesis_s_68000_ram_w (int offset, int data)
{
	int address = (z80_68000_latch) + offset;
//	if (address > 0xff0000) *(&mainram[address & 0xffff]) = data;

 //	if (address > 0xff0000) genesis_sharedram[offset] = data;
	if (errorlog) fprintf (errorlog, "z80 poke to address %x\n", address);
}
int genesis_s_68000_ram_r (int offset)
{
	int address = (z80_68000_latch) + offset;
/*  return -1;*/
//	if (address < 0x400000)
//		return Machine->memory_region[0][(z80_68000_latch) + offset];
//	else if (address > 0xff0000) return *(&mainram[address & 0xffff]);

	if (errorlog) fprintf (errorlog, "z80 read from address %x\n", address);

	/* Read the data out of the 68k ROM */
	if (address < 0x400000) return Machine->memory_region[0][BYTE_XOR(address)];
	/* else read the data out of the 68k RAM */
 	else if (address > 0xff0000) return genesis_sharedram[BYTE_XOR(offset)];

	return -1;
}

void genesis_soundram_w (int offset,int data)
{
	genesis_soundram[offset] = data;
}

int genesis_soundram_r (int offset)
{
	return genesis_sharedram[offset];
}

void genesis_sharedram_w (int offset,int data)
{
	genesis_soundram[offset] = data;
}

int genesis_sharedram_r (int offset)
{
	return genesis_sharedram[offset];
}


static struct MemoryReadAddress genesis_readmem[] =
{
	{ 0x000000, 0x3fffff, MRA_ROM },
	{ 0xff0000, 0xffffff, MRA_BANK2, (unsigned char **)&genesis_sharedram[0], (int *)&genesis_sharedram_size}, /* RAM */
	{ 0xc00014, 0xfeffff, MRA_NOP },

   	{ 0xc00000, 0xc00003, genesis_vdp_data_r },
	{ 0xc00004, 0xc00007, genesis_vdp_ctrl_r },
	{ 0xc00008, 0xc0000b, genesis_vdp_hv_r },
	{ 0xc00010, 0xc00013, MRA_NOP /*genesis_vdp_76489_r*/ },
	{ 0xa11204, 0xbfffff, MRA_NOP },

	{ 0xa11000, 0xa11203, genesis_ctrl_r },
	{ 0xa10000, 0xa1001f, genesis_io_r},
	{ 0xa00000, 0xa03fff, genesis_soundram_r },
//	{ 0xa04000, 0xa05fff, MRA_NOP },
/*	{ 0x200000, 0x200fff, cartridge_ram_r},*/
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress genesis_writemem[] =
{
	{ 0xff0000, 0xffffff, MWA_BANK2/*genesis_sharedram_w*/ },
   	{ 0xd00000, 0xd03fff, genesis_videoram1_w, &videoram, &videoram_size }, /*this is just a fake */
	{ 0xc00014, 0xcfffff, MWA_NOP },
	{ 0xc00000, 0xc00003, genesis_vdp_data_w },
	{ 0xc00004, 0xc00007, genesis_vdp_ctrl_w },
	{ 0xc00008, 0xc0000b, genesis_vdp_hv_w },
   	{ 0xc00010, 0xc00013, genesis_vdp_76489_w },
	{ 0xa11204, 0xbfffff, MWA_NOP },
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa10000, 0xa1001f, genesis_io_w},
	{ 0xa07010, 0xa07013, genesis_vdp_76489_w },
	{ 0xa06000, 0xa06003, genesis_ramlatch_w },
	{ 0xa00000, 0xa03fff, genesis_soundram_w },
//	{ 0xa04000, 0xa05fff, MWA_NOP },
/*	{ 0x200000, 0x200fff, cartridge_ram_w },	*/
	{ 0x000000, 0x3fffff, MWA_ROM },
	{ -1 }  /* end of table */
};




static struct MemoryReadAddress genesis_s_readmem[] =
{
 	{ 0x0000, 0x3fff, MRA_BANK1, (unsigned char **)&genesis_soundram[0], (int *)&genesis_soundram_size/*genesis_soundram_r*/ },
  //	{ 0x4000, 0x7fff, genesis_ym2612_r },
	{ 0x8000, 0xffff, genesis_s_68000_ram_r },
 	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress genesis_s_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_BANK1 /*genesis_soundram_w*/ },
   //	{ 0x4000, 0x5fff, genesis_ym2612_w },
	{ 0x6000, 0x6000, genesis_ramlatch_w },
   	{ 0x7f11, 0x7f11, SN76496_0_w },  
   	{ 0x8000, 0xffff, genesis_s_68000_ram_w }, 
  	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },
	{ 0x7f, 0x7f, SN76496_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( genesis_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )



	PORT_START	/* IN3 - fake */

	PORT_DIPNAME( 0x03, 0x00, "Country", IP_KEY_NONE )

	PORT_DIPSETTING(    0x00, "Auto" )

	PORT_DIPSETTING(    0x01, "USA" )

	PORT_DIPSETTING(    0x02, "Japan" )

	PORT_DIPSETTING(    0x03, "Europe" )
INPUT_PORTS_END

											  

/* genesis doesn't have a color PROM, it uses a RAM to generate colors */
/* and change them during the game. */

static struct SN76496interface sn76496_interface =
{
	1,	/* 1 chip */
	34318180/8,	/*  1.7897725 Mhz */
	{ 255*2, 255*2 }
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	7580000,	/* 3.58 MHZ ? */
	{ 255 },
	{ 0 }
};

#if 0
static struct DACinterface DAC_interface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};
#endif

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0, /* number of memory regions */
			genesis_readmem,genesis_writemem,0,0, /* zeros are ioport read/write */
			genesis_interrupt,1	/* 1 interrupt per frame */
		},
	 	{
	 		CPU_Z80 | CPU_AUDIO_CPU,
	 		3500000,	/* 4 Mhz */
	 		2,
	 		genesis_s_readmem,genesis_s_writemem,0,writeport,
	 		genesis_s_interrupt,1
	 	}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	genesis_init_machine,

	/* video hardware */  
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	0,/*gfxdecodeinfo,*/
	64,64/sizeof(unsigned short), /* genesis uses 4 color schemes of 16 colors each, 0 of each bank is transparent*/
	genesis_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	genesis_vh_start,
	genesis_vh_stop,
	genesis_vh_screenrefresh,

	/* sound hardware */
	/* sound hardware */
	0,0,0,0,
	{
//		{
// 			SOUND_YM2151_ALT,
//			SOUND_YM2151,
//			&ym2151_interface
//		},
		{
			SOUND_SN76496,
			&sn76496_interface
#if 0
		},
		{
			SOUND_DAC,
			&DAC_interface
#endif
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
#if 0
ROM_START( genesis_rom )
	ROM_REGION( 0x400000 )

	ROM_REGION( 0x10000 ) /* Z80 memory */
ROM_END
#endif

struct GameDriver genesis_driver =
{
	"Sega Megadrive/Genesis",
	"genesis",
	"Gareth S. Long\n\n\nIn Memory Of Haruki Ikeda",
	&machine_driver,

	genesis_load_rom,
	genesis_id_rom,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,

	genesis_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
