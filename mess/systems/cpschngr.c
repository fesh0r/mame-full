/***************************************************************************

  Capcom System 1
  ===============

  Driver provided by:
  Paul Leaman (paul@vortexcomputing.demon.co.uk)

  M680000 for game, Z80, YM-2151 and OKIM6295 for sound.

  68000 clock speeds are unknown for all games (except where commented)

merged Street Fighter Zero for MESS

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

#include "drivers/cps1.h"       /* External CPS1 definitions */

/* in machine/kabuki.c */
void wof_decode(void);
void dino_decode(void);
void punisher_decode(void);
void slammast_decode(void);



static READ16_HANDLER( cps1_input2_r )
{
	int buttons=readinputport(5);
	return buttons << 8 | buttons;
}

static READ16_HANDLER( cps1_input3_r )
{
    int buttons=readinputport(6);
	return buttons << 8 | buttons;
}


static int cps1_sound_fade_timer;

static WRITE_HANDLER( cps1_snd_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int length = memory_region_length(REGION_CPU2) - 0x10000;
	int bankaddr;

	bankaddr = (data * 0x4000) & (length-1);
	cpu_setbank(1,&RAM[0x10000 + bankaddr]);

	if (data & 0xfe) logerror("%04x: write %02x to f004\n",cpu_get_pc(),data);
}

static WRITE16_HANDLER( cps1_sound_fade_w )
{
	if (ACCESSING_LSB)
		cps1_sound_fade_timer = data & 0xff;
}

static READ_HANDLER( cps1_snd_fade_timer_r )
{
	return cps1_sound_fade_timer;
}

static WRITE16_HANDLER( cps1_sound_command_w )
{
	if (ACCESSING_LSB)
		soundlatch_w(0,data & 0xff);
}

static READ16_HANDLER( cps1_input_r )
{
	int control=readinputport(offset);
	return (control<<8) | control;
}

static int dial[2];

static READ16_HANDLER( forgottn_dial_0_r )
{
	return ((readinputport(5) - dial[0]) >> (8*offset)) & 0xff;
}

static READ16_HANDLER( forgottn_dial_1_r )
{
	return ((readinputport(6) - dial[1]) >> (8*offset)) & 0xff;
}

static WRITE16_HANDLER( forgottn_dial_0_reset_w )
{
	dial[0] = readinputport(5);
}

static WRITE16_HANDLER( forgottn_dial_1_reset_w )
{
	dial[1] = readinputport(6);
}

static WRITE16_HANDLER( cps1_coinctrl_w )
{
//	usrintf_showmessage("coinctrl %04x",data);

	if (ACCESSING_MSB)
	{
		coin_counter_w(0,data & 0x0100);
		coin_counter_w(1,data & 0x0200);
		coin_lockout_w(0,~data & 0x0400);
		coin_lockout_w(1,~data & 0x0800);
	}

	if (ACCESSING_LSB)
	{
		/* mercs sets bit 0 */
		set_led_status(0,data & 0x02);
		set_led_status(1,data & 0x04);
		set_led_status(2,data & 0x08);
	}
}

static WRITE16_HANDLER( cpsq_coinctrl2_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(2,data & 0x01);
		coin_lockout_w(2,~data & 0x02);
		coin_counter_w(3,data & 0x04);
		coin_lockout_w(3,~data & 0x08);
/*
  	{
       char baf[40];
       sprintf(baf,"0xf1c004=%04x", data);
       usrintf_showmessage(baf);
       }
*/
    }
}

static int cps1_interrupt(void)
{
	/* Strider also has a IRQ4 handler. It is input port related, but the game */
	/* works without it (maybe it's used to multiplex controls). It is the */
	/* *only* game to have that. */
	return 2;
}

/********************************************************************
*
*  Q Sound
*  =======
*
********************************************************************/

static struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

static unsigned char *qsound_sharedram1,*qsound_sharedram2;

int cps1_qsound_interrupt(void)
{
#if 0
I have removed CPU_AUDIO_CPU from the Z(0 so this is no longer necessary
	/* kludge to pass the sound board test with sound disabled */
	if (Machine->sample_rate == 0)
		qsound_sharedram1[0xfff] = 0x77;
#endif

	return 2;
}


READ16_HANDLER( qsound_rom_r )
{
	unsigned char *rom = memory_region(REGION_USER1);

	if (rom) return rom[offset] | 0xff00;
	else
	{
		usrintf_showmessage("%06x: read sound ROM byte %04x",cpu_get_pc(),offset);
		return 0;
	}
}

static READ16_HANDLER( qsound_sharedram1_r )
{
	return qsound_sharedram1[offset] | 0xff00;
}

static WRITE16_HANDLER( qsound_sharedram1_w )
{
	if (ACCESSING_LSB)
		qsound_sharedram1[offset] = data;
}

static READ16_HANDLER( qsound_sharedram2_r )
{
	return qsound_sharedram2[offset] | 0xff00;
}

static WRITE16_HANDLER( qsound_sharedram2_w )
{
	if (ACCESSING_LSB)
		qsound_sharedram2[offset] = data;
}

static WRITE_HANDLER( qsound_banksw_w )
{
	/*
	Z80 bank register for music note data. It's odd that it isn't encrypted
	though.
	*/
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bankaddress=0x10000+((data&0x0f)*0x4000);
	if (bankaddress >= memory_region_length(REGION_CPU2))
	{
		logerror("WARNING: Q sound bank overflow (%02x)\n", data);
		bankaddress=0x10000;
	}
	cpu_setbank(1, &RAM[bankaddress]);
}


/********************************************************************
*
*  EEPROM
*  ======
*
*   The EEPROM is accessed by a serial protocol using the register
*   0xf1c006
*
********************************************************************/

static struct EEPROM_interface qsound_eeprom_interface =
{
	7,		/* address bits */
	8,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static struct EEPROM_interface pang3_eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static void qsound_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&qsound_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

static void pang3_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&pang3_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

READ16_HANDLER( cps1_eeprom_port_r )
{
	return EEPROM_read_bit();
}

WRITE16_HANDLER( cps1_eeprom_port_w )
{
	if (ACCESSING_LSB)
	{
		/*
		bit 0 = data
		bit 6 = clock
		bit 7 = cs
		*/
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}



static MEMORY_READ16_START( cps1_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM }, /* 68000 ROM */
	{ 0x800000, 0x800001, input_port_4_word_r }, /* Player input ports */
	{ 0x800010, 0x800011, input_port_4_word_r }, /* ?? */
	{ 0x800018, 0x80001f, cps1_input_r }, /* Input ports */
	{ 0x800020, 0x800021, MRA16_NOP }, /* ? Used by Rockman ? */
	{ 0x800052, 0x800055, forgottn_dial_0_r }, /* forgotten worlds */
	{ 0x80005a, 0x80005d, forgottn_dial_1_r }, /* forgotten worlds */
	{ 0x800176, 0x800177, cps1_input2_r }, /* Extra input ports */
	{ 0x8001fc, 0x8001fd, cps1_input2_r }, /* Input ports (SF Rev E) */
	{ 0x800100, 0x8001ff, cps1_output_r },   /* Output ports */
	{ 0x900000, 0x92ffff, MRA16_RAM },	/* SF2CE executes code from here */
	{ 0xf00000, 0xf0ffff, qsound_rom_r },		/* Slammasters protection */
	{ 0xf18000, 0xf19fff, qsound_sharedram1_r },	/* Q RAM */
	{ 0xf1c000, 0xf1c001, cps1_input2_r },   /* Player 3 controls (later games) */
	{ 0xf1c002, 0xf1c003, cps1_input3_r },   /* Player 4 controls (later games - muscle bombers) */
	{ 0xf1c006, 0xf1c007, cps1_eeprom_port_r },
	{ 0xf1e000, 0xf1ffff, qsound_sharedram2_r },	/* Q RAM */
	{ 0xff0000, 0xffffff, MRA16_RAM },   /* RAM */
MEMORY_END

static MEMORY_WRITE16_START( cps1_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },      /* ROM */
	{ 0x800030, 0x800031, cps1_coinctrl_w },
	{ 0x800040, 0x800041, forgottn_dial_0_reset_w },
	{ 0x800048, 0x800049, forgottn_dial_1_reset_w },
	{ 0x800180, 0x800181, cps1_sound_command_w },  /* Sound command */
	{ 0x800188, 0x800189, cps1_sound_fade_w },
	{ 0x800100, 0x8001ff, cps1_output_w, &cps1_output, &cps1_output_size },  /* Output ports */
	{ 0x900000, 0x92ffff, MWA16_RAM, &cps1_gfxram, &cps1_gfxram_size },
	{ 0xf18000, 0xf19fff, qsound_sharedram1_w }, /* Q RAM */
	{ 0xf1c004, 0xf1c005, cpsq_coinctrl2_w },   /* Coin control2 (later games) */
	{ 0xf1c006, 0xf1c007, cps1_eeprom_port_w },
	{ 0xf1e000, 0xf1ffff, qsound_sharedram2_w }, /* Q RAM */
	{ 0xff0000, 0xffffff, MWA16_RAM },        /* RAM */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf002, 0xf002, OKIM6295_status_0_r },
	{ 0xf008, 0xf008, soundlatch_r },
	{ 0xf00a, 0xf00a, cps1_snd_fade_timer_r }, /* Sound timer fade */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
	{ 0xf004, 0xf004, cps1_snd_bankswitch_w },
//	{ 0xf006, 0xf006, MWA_NOP }, /* ???? Unknown ???? */
MEMORY_END

static MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked (contains music data) */
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM, &qsound_sharedram1 },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_banksw_w },
	{ 0xf000, 0xffff, MWA_RAM, &qsound_sharedram2 },
MEMORY_END

INPUT_PORTS_START( sfzch )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Pause", KEYCODE_F1, IP_JOY_NONE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE  )	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END



/********************************************************************

			Graphics Layout macros

  These are not really needed, and are used for documentation only.

********************************************************************/


#define DECODE_GFX 0

static struct GfxLayout tilelayout8 =
{
	8,8,
#if DECODE_GFX
	RGN_FRAC(1,2),
#else
	0,
#endif
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout16 =
{
	16,16,
#if DECODE_GFX
	RGN_FRAC(1,4),
#else
	0,
#endif
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+1, RGN_FRAC(1,4)+2, RGN_FRAC(1,4)+3,
	  RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+5, RGN_FRAC(1,4)+6, RGN_FRAC(1,4)+7,
	  0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxLayout tilelayout32 =
{
	32,32,
#if DECODE_GFX
	RGN_FRAC(1,4),
#else
	0,
#endif
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{
		RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+1, RGN_FRAC(1,4)+2, RGN_FRAC(1,4)+3,
		RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+5, RGN_FRAC(1,4)+6, RGN_FRAC(1,4)+7,
		0, 1, 2, 3, 4, 5, 6, 7,
		16+RGN_FRAC(1,4)+0, 16+RGN_FRAC(1,4)+1, 16+RGN_FRAC(1,4)+2, 16+RGN_FRAC(1,4)+3,
		16+RGN_FRAC(1,4)+4, 16+RGN_FRAC(1,4)+5, 16+RGN_FRAC(1,4)+6, 16+RGN_FRAC(1,4)+7,
		16+0, 16+1, 16+2, 16+3, 16+4, 16+5, 16+6, 16+7
	},
	{
		 0*32,  1*32,  2*32,  3*32,  4*32,  5*32,  6*32,  7*32,
		 8*32,  9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
		24*32, 25*32, 26*32, 27*32, 28*32, 29*32, 30*32, 31*32
	},
	32*32
};

static struct GfxDecodeInfo cps1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout16, 0x000, 32 },	/* sprites */
	{ REGION_GFX1, 0, &tilelayout8,  0x200, 32 },	/* tiles 8x8 */
	{ REGION_GFX1, 0, &tilelayout16, 0x400, 32 },	/* tiles 16x16 */
	{ REGION_GFX1, 0, &tilelayout32, 0x600, 32 },	/* tiles 32x32 */
	/* stars use colors 0x800-087ff and 0xa00-0a7ff */
	{ -1 } /* end of array */
};

static void cps1_irq_handler_mus(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,  /* 1 chip */
	3579580,    /* 3.579580 MHz ? */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ cps1_irq_handler_mus }
};

static struct OKIM6295interface okim6295_interface_6061 =
{
	1,  /* 1 chip */
	{ 6061 },
	{ REGION_SOUND1 },
	{ 30 }
};

static struct OKIM6295interface okim6295_interface_7576 =
{
	1,  /* 1 chip */
	{ 7576 },
	{ REGION_SOUND1 },
	{ 30 }
};

static struct MachineDriver machine_driver_sfzch =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			cps1_readmem,cps1_writemem,0,0,
			cps1_interrupt, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
    60, 3000,
	1,
	0,
	0,

	/* video hardware */
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },

	cps1_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	cps1_eof_callback,
	cps1_vh_start,
	cps1_vh_stop,
	cps1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{ { SOUND_YM2151,  &ym2151_interface },
	  { SOUND_OKIM6295,  &okim6295_interface_7576 }
	},
	0	
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

#define CODE_SIZE 0x200000


ROM_START( sfzch )
	ROM_REGION( CODE_SIZE, REGION_CPU1,0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzch23",        0x000000, 0x80000, 0x1140743f )
	ROM_LOAD16_WORD_SWAP( "sfza22",         0x080000, 0x80000, 0x8d9b2480 )
	ROM_LOAD16_WORD_SWAP( "sfzch21",        0x100000, 0x80000, 0x5435225d )
	ROM_LOAD16_WORD_SWAP( "sfza20",         0x180000, 0x80000, 0x806e8f38 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sfz03",         0x000000, 0x80000, 0x9584ac85 )
	ROM_LOAD( "sfz07",         0x080000, 0x80000, 0xbb2c734d )
	ROM_LOAD( "sfz12",         0x100000, 0x80000, 0xf122693a )
	ROM_LOAD( "sfz16",         0x180000, 0x80000, 0x19a5abd6 )
	ROM_LOAD( "sfz01",         0x200000, 0x80000, 0x0dd53e62 )
	ROM_LOAD( "sfz05",         0x280000, 0x80000, 0x2b47b645 )
	ROM_LOAD( "sfz10",         0x300000, 0x80000, 0x2a7d675e )
	ROM_LOAD( "sfz14",         0x380000, 0x80000, 0x09038c81 )
	ROM_LOAD( "sfz04",         0x400000, 0x80000, 0xb983624c )
	ROM_LOAD( "sfz08",         0x480000, 0x80000, 0x454f7868 )
	ROM_LOAD( "sfz13",         0x500000, 0x80000, 0x7cf942c8 )
	ROM_LOAD( "sfz17",         0x580000, 0x80000, 0x248b3b73 )
	ROM_LOAD( "sfz02",         0x600000, 0x80000, 0x94c31e3f )
	ROM_LOAD( "sfz06",         0x680000, 0x80000, 0x74fd9fb1 )
	ROM_LOAD( "sfz11",         0x700000, 0x80000, 0xe35546c8 )
	ROM_LOAD( "sfz15",         0x780000, 0x80000, 0x1aa17391 )

	ROM_REGION( 0x18000, REGION_CPU2,0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz09",         0x00000, 0x08000, 0xc772628b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1,0 )	/* Samples */
	ROM_LOAD( "sfz18",         0x00000, 0x20000, 0x61022b2d )
	ROM_LOAD( "sfz19",         0x20000, 0x20000, 0x3b5886d5 )
ROM_END



static const struct IODevice io_sfzch[] = {

    { IO_END }
};


CONS( 1995, sfzch,    0,        sfzch,     sfzch,    0,        "Capcom", "CPS Changer (Street Fighter ZERO)" )

#include "drivers/cps2.c"
