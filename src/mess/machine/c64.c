/***************************************************************************
	commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/z80/z80.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "cia6526.h"
#include "c1551.h"
#include "vc1541.h"
#include "vc20tape.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/vidhrdw/vdc8563.h"
#include "mess/sndhrdw/sid6581.h"

#include "c128.h"
#include "c65.h"

#include "c64.h"

/* keyboard lines */
UINT8 c64_keyline[10] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* expansion port lines input */
int c64_pal = 0;
UINT8 c64_game=1, c64_exrom=1;

/* cpu port */
UINT8 c64_port6510, c64_ddr6510;
int c128_va1617;
UINT8 *c64_vicaddr, *c128_vicaddr;
UINT8 *c64_memory;
UINT8 *c64_colorram;
UINT8 *c64_basic;
UINT8 *c64_kernal;
UINT8 *c64_chargen;
UINT8 *c64_roml=0;
UINT8 *c64_romh=0;

static int ultimax = 0;
int c64_tape_on = 1;
static int c64_cia1_on = 1;
static UINT8 cartridge = 0;
static enum
{
	CartridgeAuto = 0, CartridgeUltimax, CartridgeC64,
	CartridgeSuperGames, CartridgeRobocop2
}
cartridgetype = CartridgeAuto;
static UINT8 cia0porta;
static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 vicirq = 0, cia0irq = 0;

/*
 * cia 0
 * port a
 * 7-0 keyboard line select
 * 7,6: paddle select( 01 port a, 10 port b)
 * 4: joystick a fire button
 * 3,2: Paddles port a fire button
 * 3-0: joystick a direction
 * port b
 * 7-0: keyboard raw values
 * 4: joystick b fire button, lightpen select
 * 3,2: paddle b fire buttons (left,right)
 * 3-0: joystick b direction
 * flag cassette read input, serial request in
 * irq to irq connected
 */
int c64_cia0_port_a_r (int offset)
{
	int value = 0xff;

	if (JOYSTICK_SWAP) value = c64_keyline[8];
    else value = c64_keyline[9];
	return value;
}

int c64_cia0_port_b_r (int offset)
{
	int value = 0xff;

	if (!(cia0porta & 0x80))
		value &= c64_keyline[7];
	if (!(cia0porta & 0x40))
		value &= c64_keyline[6];
	if (!(cia0porta & 0x20))
		value &= c64_keyline[5];
	if (!(cia0porta & 0x10))
		value &= c64_keyline[4];
	if (!(cia0porta & 8))
		value &= c64_keyline[3];
	if (!(cia0porta & 4))
		value &= c64_keyline[2];
	if (!(cia0porta & 2))
		value &= c64_keyline[1];
	if (!(cia0porta & 1))
		value &= c64_keyline[0];
	
	if (JOYSTICK_SWAP) value &= c64_keyline[9];
	else value &= c64_keyline[8];

	if (c128)
	{
		if (!vic2e_k0_r ())
			value &= c128_keyline[0];
		if (!vic2e_k1_r ())
			value &= c128_keyline[1];
		if (!vic2e_k2_r ())
			value &= c128_keyline[2];
	}
	if (c65) {
		if (!(c65_6511_port&2))
			value&=c65_keyline[0];
		if (!(c65_6511_port&4))
			value&=c65_keyline[1];
	}

	return value;
}

void c64_cia0_port_a_w (int offset, int data)
{
	cia0porta = data;
}

static void c64_cia0_port_b_w (int offset, int data)
{
	vic2_lightpen_write (data & 0x10);
}

static void c64_irq (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6510", (errorlog, "irq %s\n", level ? "start" : "end"));
		if (c128) {
			if (0&&(cpu_getactivecpu()==0)) {
				cpu_set_irq_line (0, Z80_IRQ_INT, level);
			} else {
				cpu_set_irq_line (1, M6510_INT_IRQ, level);
			}			
		} else {
			cpu_set_irq_line (0, M6510_INT_IRQ, level);
		}
		old_level = level;
	}
}

void c64_tape_read (int offset, int data)
{
	cia6526_0_set_input_flag (data);
}

static void c64_cia0_interrupt (int level)
{
	if (level != cia0irq)
	{
		c64_irq (level || vicirq);
		cia0irq = level;
	}
}

void c64_vic_interrupt (int level)
{
#if 1
	if (level != vicirq)
	{
		c64_irq (level || cia0irq);
		vicirq = level;
	}
#endif
}

/*
 * cia 1
 * port a
 * 7 serial bus data input
 * 6 serial bus clock input
 * 5 serial bus data output
 * 4 serial bus clock output
 * 3 serial bus atn output
 * 2 rs232 data output
 * 1-0 vic-chip system memory bank select
 * 
 * port b
 * 7 user rs232 data set ready
 * 6 user rs232 clear to send
 * 5 user
 * 4 user rs232 carrier detect
 * 3 user rs232 ring indicator
 * 2 user rs232 data terminal ready
 * 1 user rs232 request to send
 * 0 user rs232 received data
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
int c64_cia1_port_a_r (int offset)
{
	int value = 0xff;

	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~0x40;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~0x80;
	return value;
}

static void c64_cia1_port_a_w (int offset, int data)
{
	static int helper[4] =
	{0xc000, 0x8000, 0x4000, 0x0000};

	cbm_serial_clock_write (serial_clock = !(data & 0x10));
	cbm_serial_data_write (serial_data = !(data & 0x20));
	cbm_serial_atn_write (serial_atn = !(data & 8));
	c64_vicaddr = c64_memory + helper[data & 3];
	if (c128) {
		c128_vicaddr = c64_memory + helper[data & 3] + c128_va1617;
	}
}

static void c64_cia1_interrupt (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (1, "mos6510", (errorlog, "nmi %s\n", level ? "start" : "end"));
		//      cpu_set_nmi_line(0, level);
		old_level = level;
	}
}

struct cia6526_interface c64_cia0 =
{
	c64_cia0_port_a_r,
	c64_cia0_port_b_r,
	c64_cia0_port_a_w,
	c64_cia0_port_b_w,
	0,								   /*c64_cia0_pc_w */
	0,								   /*c64_cia0_sp_r */
	0,								   /*c64_cia0_sp_w */
	0,								   /*c64_cia0_cnt_r */
	0,								   /*c64_cia0_cnt_w */
	c64_cia0_interrupt,
	0xff, 0xff, 0
}, c64_cia1 =
{
	c64_cia1_port_a_r,
	0,								   /*c64_cia1_port_b_r, */
	c64_cia1_port_a_w,
	0,								   /*c64_cia1_port_b_w, */
	0,								   /*c64_cia1_pc_w */
	0,								   /*c64_cia1_sp_r */
	0,								   /*c64_cia1_sp_w */
	0,								   /*c64_cia1_cnt_r */
	0,								   /*c64_cia1_cnt_w */
	c64_cia1_interrupt,
	0xc7, 0xff, 0
};

static void c64_bankswitch (void);
static void c64_robocop2_w(int offset, int value)
{
	/* robocop2 0xe00
	 * 80 94 80 94 80 
	 * 80 81 80 82 83 80 
	 */
	c64_roml=cbm_rom[value&0xf].chip;
	c64_romh=cbm_rom[(value&0xf)+0x10].chip;
	if (value & 0x80)
		{
			c64_game = value & 0x10;
			c64_exrom = 1;
		}
	else
		{
			c64_game = c64_exrom = 1;
		}
	if (c128)
		c128_bankswitch_64 ();
	else
		c64_bankswitch ();	
}

static void c64_supergames_w(int offset, int value)
{
	/* supergam 0xf00
	 * 4 9 4
	 * 4 0 c
	 */
	c64_roml=cbm_rom[value&3].chip;
	c64_romh=cbm_rom[value&3].chip+0x2000;
	if (value & 4)
		{
			c64_game = 0;
			c64_exrom = 1;
		}
	else
		{
			c64_game = c64_exrom = 1;
		}
	if (value == 0xc)
		{
			c64_game = c64_exrom = 0;
		}
	if (c128)
		c128_bankswitch_64 ();
	else
		c64_bankswitch ();
}

void c64_write_io (int offset, int value)
{
	if (offset < 0x400)
		vic2_port_w (offset & 0x3ff, value);
	else if (offset < 0x800)
		sid6581_0_port_w (offset & 0x3ff, value);
	else if (offset < 0xc00)
		c64_colorram[offset & 0x3ff] = value | 0xf0;
	else if (offset < 0xd00)
		cia6526_0_port_w (offset & 0xff, value);
	else if (offset < 0xe00)
	{
		if (c64_cia1_on)
			cia6526_1_port_w (offset & 0xff, value);
		else
			DBG_LOG (1, "io write", (errorlog, "%.3x %.2x\n", offset, value));
	}
	else if (offset < 0xf00)
	{
		/* i/o 1 */
		if (cartridge && (cartridgetype == CartridgeRobocop2))
		{
			c64_robocop2_w(offset&0xff, value);
		}
		else
			DBG_LOG (1, "io write", (errorlog, "%.3x %.2x\n", offset, value));
	}
	else
	{
		/* i/o 2 */
		if (cartridge && (cartridgetype == CartridgeSuperGames))
		{
			c64_supergames_w(offset&0xff, value);
		}
		else
			DBG_LOG (1, "io write", (errorlog, "%.3x %.2x\n", offset, value));
	}
}

int c64_read_io (int offset)
{
	if (offset < 0x400)
		return vic2_port_r (offset & 0x3ff);
	else if (offset < 0x800)
		return sid6581_0_port_r (offset & 0x3ff);
	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];
	else if (offset < 0xd00)
		return cia6526_0_port_r (offset & 0xff);
	else if (c64_cia1_on && (offset < 0xe00))
		return cia6526_1_port_r (offset & 0xff);
	DBG_LOG (1, "io read", (errorlog, "%.3x\n", offset));
	return 0xff;
}

/*
 * two devices access bus, cpu and vic
 * 
 * romh, roml chip select lines on expansion bus
 * loram, hiram, charen bankswitching select by cpu
 * exrom, game bankswitching select by cartridge
 * va15, va14 bank select by cpu for vic
 * 
 * exrom, game: normal c64 mode
 * exrom, !game: ultimax mode
 * 
 * romh: 8k rom at 0xa000 (hiram && !game && exrom)
 * or 8k ram at 0xe000 (!game exrom)
 * roml: 8k rom at 0x8000 (loram hiram !exrom)
 * or 8k ram at 0x8000 (!game exrom)
 * roml vic: upper 4k rom at 0x3000, 0x7000, 0xb000, 0xd000 (!game exrom)
 * 
 * basic rom: 8k rom at 0xa000 (loram hiram game)
 * kernal rom: 8k rom at 0xe000 (hiram !exrom, hiram game)
 * char rom: 4k rom at 0xd000 (!exrom charen hiram
 * game charen !hiram loram
 * game charen hiram)
 * cpu
 * 
 * (write colorram)
 * gr_w = !read&&!cas&&((address&0xf000)==0xd000)
 * 
 * i_o = !game exrom !read ((address&0xf000)==0xd000)
 * !game exrom ((address&0xf000)==0xd000)
 * charen !hiram loram !read ((address&0xf000)==0xd000)
 * charen !hiram loram ((address&0xf000)==0xd000)
 * charen hiram !read ((address&0xf000)==0xd000)
 * charen hiram ((address&0xf000)==0xd000)
 * 
 * vic
 * char rom: x101 (game, !exrom)
 * romh: 0011 (!game, exrom)
 * 
 * exrom !game (ultimax mode)
 * addr    CPU     VIC-II
 * ----    ---     ------
 * 0000    RAM     RAM
 * 1000    -       RAM
 * 2000    -       RAM
 * 3000    -       ROMH (upper half)
 * 4000    -       RAM
 * 5000    -       RAM
 * 6000    -       RAM
 * 7000    -       ROMH
 * 8000    ROML    RAM
 * 9000    ROML    RAM
 * A000    -       RAM
 * B000    -       ROMH
 * C000    -       RAM
 * D000    I/O     RAM
 * E000    ROMH    RAM
 * F000    ROMH    ROMH
 */
static void c64_bankswitch (void)
{
	static int old = -1, data, loram, hiram, charen;

	data = ((c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff)) & 7;
	if (data == old)
		return;

	DBG_LOG (1, "bankswitch", (errorlog, "%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		cpu_setbank (1, c64_roml);
		cpu_setbankhandler_w (2, MWA_NOP);
	}
	else
	{
		cpu_setbank (1, c64_memory + 0x8000);
		cpu_setbankhandler_w (2, MWA_RAM);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		cpu_setbank (3, c64_romh);
	}
	else if (loram && hiram)
	{
		cpu_setbank (3, c64_basic);
	}
	else
	{
		cpu_setbank (3, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		cpu_setbankhandler_r (5, c64_read_io);
		cpu_setbankhandler_w (6, c64_write_io);
	}
	else
	{
		cpu_setbankhandler_r (5, MRA_BANK5);
		cpu_setbankhandler_w (6, MWA_BANK6);
		cpu_setbank (6, c64_memory + 0xd000);
		if (!charen && (loram || hiram))
		{
			cpu_setbank (5, c64_chargen);
		}
		else
		{
			cpu_setbank (5, c64_memory + 0xd000);
		}
	}

	if (!c64_game && c64_exrom)
	{
		cpu_setbank (7, c64_romh);
		cpu_setbankhandler_w (8, MWA_NOP);
	}
	else
	{
		cpu_setbankhandler_w (8, MWA_RAM);
		if (hiram)
		{
			cpu_setbank (7, c64_kernal);
		}
		else
		{
			cpu_setbank (7, c64_memory + 0xe000);
		}
	}
	old = data;
}

/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  p0 output loram
  p1 output hiram
  p2 output charen
  p3 output cassette data
  p4 input cassette switch
  p5 output cassette motor
  p6,7 not available on M6510
 */
void c64_m6510_port_w (int offset, int data)
{
	if (offset)
	{
		if (c64_port6510 != data)
			c64_port6510 = data;
	}
	else
	{
		if (c64_ddr6510 != data)
			c64_ddr6510 = data;
	}
	data = (c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff);
	if (c64_tape_on) {
		vc20_tape_write (!(data & 8));
		vc20_tape_motor (data & 0x20);
	}
	if (c128)
		c128_bankswitch_64 ();
	else if (c65)
		c65_bankswitch();
	else if (!ultimax)
		c64_bankswitch ();
}

int c64_m6510_port_r (int offset)
{
	if (offset)
	{
		int data = (c64_ddr6510 & c64_port6510) | (c64_ddr6510 ^ 0xff);

		if (c64_tape_on && !(c64_ddr6510 & 0x10) && !vc20_tape_switch ())
			data &= ~0x10;
		if (c128 && !c128_capslock_r ())
			data &= ~0x40;
		return data;
	}
	else
	{
		return c64_ddr6510;
	}
}

int c64_paddle_read (int which)
{
	int pot1=0xff, pot2=0xff, pot3=0xff, pot4=0xff, temp;
	if (PADDLES34) {
		if (which) pot4=PADDLE4_VALUE;
		else pot3=PADDLE3_VALUE;
	}
	if (JOYSTICK2_2BUTTON&&which) {
		if (JOYSTICK_2_BUTTON2) pot4=0x00;
	}
	if (MOUSE2) {
	}
	if (PADDLES12) {
		if (which) pot2=PADDLE2_VALUE;
		else pot1=PADDLE1_VALUE;
	}
	if (JOYSTICK1_2BUTTON&&which) {
		if (JOYSTICK_1_BUTTON2) pot1=0x00;
	}
	if (MOUSE1) {
	}
	if (JOYSTICK_SWAP) {
		temp=pot1;pot1=pot2;pot2=pot1;
		temp=pot3;pot3=pot4;pot4=pot3;
	}
	switch (cia0porta & 0xc0) {
	case 0x40:
		if (which) return pot2;
		return pot1;
	case 0x80:
		if (which) return pot4;
			return pot3;
	default:
		return 0;
	}
}

int c64_colorram_read (int offset)
{
	return c64_colorram[offset & 0x3ff];
}

void c64_colorram_write (int offset, int value)
{
	c64_colorram[offset & 0x3ff] = value | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c64_dma_read (int offset)
{
	if (!c64_game && c64_exrom)
	{
		if (offset < 0x3000)
			return c64_memory[offset];
		return c64_romh[offset & 0x1fff];
	}
	if (((c64_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
		return c64_chargen[offset & 0xfff];
	return c64_vicaddr[offset];
}

static int c64_dma_read_ultimax (int offset)
{
	if (offset < 0x3000)
		return c64_memory[offset];
	return c64_romh[offset & 0x1fff];
}

static int c64_dma_read_color (int offset)
{
	return c64_colorram[offset & 0x3ff] & 0xf;
}

static void c64_common_driver_init (void)
{
	/*    memset(c64_memory, 0, 0xfd00); */

	if (c64_tape_on)
		vc20_tape_open (c64_tape_read);

	if (c64_cia1_on)
	{
		cbm_drive_open ();

		cbm_drive_attach_fs (0);
		cbm_drive_attach_fs (1);

#ifdef VC1541
		vc1541_driver_init ();
#endif
	}

	sid6581_0_init (c64_paddle_read);
	c64_cia0.todin50hz = c64_pal;
	cia6526_config (0, &c64_cia0);
	if (c64_cia1_on)
	{
		c64_cia1.todin50hz = c64_pal;
		cia6526_config (1, &c64_cia1);
	}

	if (ultimax)
	{
		vic6567_init (0, c64_pal, c64_dma_read_ultimax, c64_dma_read_color,
					  c64_vic_interrupt);
	}
	else 
	{
		vic6567_init (0, c64_pal, c64_dma_read, c64_dma_read_color,
					  c64_vic_interrupt);
	}
	raster1.display_state=c64_state;
}

void c64_driver_init (void)
{
	c64_common_driver_init ();
}

void c64pal_driver_init (void)
{
	c64_pal = 1;
	c64_common_driver_init ();
}

void ultimax_driver_init (void)
{
	ultimax = 1;
    c64_cia1_on = 0;
	c64_common_driver_init ();
	if (cbm_rom[0].size==0) {
	  printf("no cartridge found\n");
	  exit(1);
	}
}

void c64gs_driver_init (void)
{
	c64_tape_on = 0;
    c64_cia1_on = 1;
	c64_common_driver_init ();
}

void sx64_driver_init (void)
{
	c64_tape_on = 0;
	c64_pal = 1;
	c64_common_driver_init ();
}

void c64_driver_shutdown (void)
{
	if (!ultimax)
	{
		cbm_drive_close ();
	}
	if (c64_tape_on)
		vc20_tape_close ();
}

void c64_common_init_machine (void)
{
#ifdef VC1541
	vc1541_machine_init ();
#endif
	if (c64_cia1_on)
	{
		cbm_serial_reset_write (0);
		cbm_drive_0_config (SERIAL8ON ? SERIAL : 0);
		cbm_drive_1_config (SERIAL9ON ? SERIAL : 0);
		serial_clock = serial_data = serial_atn = 1;
	}
	cia6526_reset ();
	c64_vicaddr = c64_memory;
	vicirq = cia0irq = 0;
	c64_port6510 = 0xff;
	c64_ddr6510 = 0;
}

void c64_init_machine (void)
{
	c64_common_init_machine ();

	c64_rom_recognition ();
	c64_rom_load();

	if (c128)
		c128_bankswitch_64 ();
	if (!ultimax)
		c64_bankswitch ();
}

void c64_shutdown_machine (void)
{
}

int c64_rom_id (int id)
{
	/* magic lowrom at offset 0x8003: $c3 $c2 $cd $38 $30 */
	/* jumped to offset 0 (0x8000) */
	int retval = 0;
	unsigned char magic[] =
	{0xc3, 0xc2, 0xcd, 0x38, 0x30}, buffer[sizeof (magic)];
	FILE *romfile;
	char *cp;
	const char *name=device_filename(IO_CARTSLOT,id);

	if (errorlog)
		fprintf (errorlog, "c64_rom_id %s\n", name);
	retval = 0;
	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if (errorlog)
			fprintf (errorlog, "rom %s not found\n", name);
		return 0;
	}

	osd_fseek (romfile, 3, SEEK_SET);
	osd_fread (romfile, buffer, sizeof (magic));
	osd_fclose (romfile);

	if (memcmp (magic, buffer, sizeof (magic)) == 0)
	{
		/* cartridgetype=CartridgeC64; */
		retval = 1;
	}
	else if ((cp = strrchr (name, '.')) != NULL)
	{
		if ((stricmp (cp + 1, "prg") == 0)
			|| (stricmp (cp + 1, "crt") == 0)
			|| (stricmp (cp + 1, "80") == 0)
			|| (stricmp (cp + 1, "90") == 0)
			|| (stricmp (cp + 1, "e0") == 0)
			|| (stricmp (cp + 1, "f0") == 0)
			|| (stricmp (cp + 1, "a0") == 0)
			|| (stricmp (cp + 1, "b0") == 0)
			|| (stricmp (cp + 1, "lo") == 0) || (stricmp (cp + 1, "hi") == 0))
			retval = 1;
	}

	if (errorlog)
	{
		if (retval)
			fprintf (errorlog, "rom %s recognized\n", name);
		else
			fprintf (errorlog, "rom %s not recognized\n", name);
	}
	return retval;
}

#define BETWEEN(value1,value2,bottom,top) \
    ( ((value2)>=(bottom))&&((value1)<(top)) )

void c64_rom_recognition (void)
{
	int i;
	for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
			 &&(cbm_rom[i].size!=0); i++) {
		cartridge=1;
		if ( BETWEEN(0xa000, 0xbfff, cbm_rom[i].addr, 
					 cbm_rom[i].addr+cbm_rom[i].size) ) {
			cartridgetype=CartridgeC64;
		} else if ( BETWEEN(0xe000, 0xffff, cbm_rom[i].addr,
							cbm_rom[i].addr+cbm_rom[i].size) ) {
			cartridgetype=CartridgeUltimax;
		}
	}
	if (i==4) cartridgetype=CartridgeSuperGames;
	if (i==32) cartridgetype=CartridgeRobocop2;
}

void c64_rom_load(void)
{
	int i;

	c64_exrom = 1;
	c64_game = 1;
	if (cartridge)
	{
		if (AUTO_MODULE && (cartridgetype == CartridgeAuto))
		{
			if (errorlog)
				fprintf (errorlog, "Cartridge type not recognized using Machine type\n");
		}
		if (C64_MODULE && (cartridgetype == CartridgeUltimax))
		{
			if (errorlog)
				fprintf (errorlog, "Cartridge could be ultimax type!?\n");
		}
		if (ULTIMAX_MODULE && (cartridgetype == CartridgeC64))
		{
			if (errorlog)
				fprintf (errorlog, "Cartridge could be c64 type!?\n");
		}
		if (C64_MODULE)
			cartridgetype = CartridgeC64;
		else if (ULTIMAX_MODULE)
			cartridgetype = CartridgeUltimax;
		else if (SUPERGAMES_MODULE)
			cartridgetype = CartridgeSuperGames;
		else if (ROBOCOP2_MODULE)
			cartridgetype = CartridgeRobocop2;
		if (ultimax || (cartridgetype == CartridgeUltimax)) {
			c64_game = 0;
		} else {
			c64_exrom = 0;
		}
		if (ultimax) {
			for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
					 &&(cbm_rom[i].size!=0); i++) {
				if (cbm_rom[i].addr==CBM_ROM_ADDR_LO) {
					memcpy(c64_memory+0x8000+0x2000-cbm_rom[i].size, 
						   cbm_rom[i].chip, cbm_rom[i].size);
				} else if ((cbm_rom[i].addr==CBM_ROM_ADDR_HI)
						   ||(cbm_rom[i].addr==CBM_ROM_ADDR_UNKNOWN)) {
					memcpy(c64_memory+0xe000+0x2000-cbm_rom[i].size, 
						   cbm_rom[i].chip, cbm_rom[i].size);
				} else {
					memcpy(c64_memory+cbm_rom[i].addr, cbm_rom[i].chip, 
						   cbm_rom[i].size);
				}
			}
        } else if ( (cartridgetype==CartridgeRobocop2)
					||(cartridgetype==CartridgeSuperGames) ) {
			c64_roml=0;
			c64_romh=0;
			for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
					 &&(cbm_rom[i].size!=0); i++) {
				if (!c64_roml 
					&& ((cbm_rom[i].addr==CBM_ROM_ADDR_UNKNOWN)
						||(cbm_rom[i].addr==CBM_ROM_ADDR_LO)
						||(cbm_rom[i].addr==0x8000)) ) {
					c64_roml=cbm_rom[i].chip;
				}
				if (!c64_romh 
					&& ((cbm_rom[i].addr==CBM_ROM_ADDR_HI)
						||(cbm_rom[i].addr==0xa000) ) ){
					c64_romh=cbm_rom[i].chip;
				}
				if (!c64_romh 
					&& (cbm_rom[i].addr==0x8000)
					&&(cbm_rom[i].size=0x4000) ){
					c64_romh=cbm_rom[i].chip+0x2000;
				}
			}
		} else /*if ((cartridgetype == CartridgeC64)||
				 (cartridgetype == CartridgeUltimax) )*/{
			c64_roml=malloc(0x4000);
			c64_romh=c64_roml+0x2000;
			for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
					 &&(cbm_rom[i].size!=0); i++) {
				if ((cbm_rom[i].addr==CBM_ROM_ADDR_UNKNOWN)
					||(cbm_rom[i].addr==CBM_ROM_ADDR_LO) ) {
					memcpy(c64_roml+0x2000-cbm_rom[i].size, 
						   cbm_rom[i].chip, cbm_rom[i].size);
				} else if ( ((cartridgetype == CartridgeC64)
					  &&(cbm_rom[i].addr==CBM_ROM_ADDR_HI))
					 ||((cartridgetype==CartridgeUltimax)
						&&(cbm_rom[i].addr==CBM_ROM_ADDR_HI)) ) {
					memcpy(c64_romh+0x2000-cbm_rom[i].size, 
						   cbm_rom[i].chip, cbm_rom[i].size);
				} else if (cbm_rom[i].addr<0xc000) {
					memcpy(c64_roml+cbm_rom[i].addr-0x8000, cbm_rom[i].chip,
						   cbm_rom[i].size);
				} else {
					memcpy(c64_romh+cbm_rom[i].addr-0xe000, 
						   cbm_rom[i].chip, cbm_rom[i].size);
				}
			}
		}
	}
}

int c64_frame_interrupt (void)
{
	static int quickload = 0;
	static int nmilevel = 0;
	static int monitor=0;
	int value, value2;

	if (nmilevel != KEY_RESTORE)
	{
		if (c128) {
			if (cpu_getactivecpu()==0) { /* z80 */
				cpu_set_nmi_line (0, KEY_RESTORE);
			} else {
				cpu_set_nmi_line (1, KEY_RESTORE);
			}
		} else {
			cpu_set_nmi_line (0, KEY_RESTORE);
		}
		nmilevel = KEY_RESTORE;
	}

	if (!quickload && QUICKLOAD)
		cbm_quick_open (0, 0, c64_memory);
	quickload = QUICKLOAD;

	if (c128) {
		if (MONITOR_TV!=monitor) {
			if (MONITOR_TV) {
				vic2_set_rastering(0);
				vdc8563_set_rastering(1);
#if 0
				osd_set_display(656,216,0);
#endif
			} else {
				vic2_set_rastering(1);
				vdc8563_set_rastering(0);
#if 0
				osd_set_display(336,216,0);
#endif
			}
			vdc8563_update();
			monitor=MONITOR_TV;
		}
	}

	value = 0xff;
	if (c128) {
		if (C128_KEY_CURSOR_DOWN)
			value &= ~0x80;
		if (C128_KEY_F5)
			value &= ~0x40;
		if (C128_KEY_F3)
			value &= ~0x20;
		if (C128_KEY_F1)
			value &= ~0x10;
		if (C128_KEY_F7)
			value &= ~8;
		if (C128_KEY_CURSOR_RIGHT)
			value &= ~4;
	} else if (c65) {
		if (C65_KEY_CURSOR_DOWN)
			value &= ~0x80;
		if (C65_KEY_F5)
			value &= ~0x40;
		if (C65_KEY_F3)
			value &= ~0x20;
		if (C65_KEY_F1)
			value &= ~0x10;
		if (C65_KEY_F7)
			value &= ~8;
		if (C65_KEY_CURSOR_RIGHT)
			value &= ~4;
	} else {
		if (KEY_CURSOR_DOWN)
			value &= ~0x80;
		if (KEY_F5)
			value &= ~0x40;
		if (KEY_F3)
			value &= ~0x20;
		if (KEY_F1)
			value &= ~0x10;
		if (KEY_F7)
			value &= ~8;
		if (KEY_CURSOR_RIGHT)
			value &= ~4;
	}
	if (KEY_RETURN)
		value &= ~2;
	if (KEY_DEL)
		value &= ~1;
	c64_keyline[0] = value;

	value = 0xff;
	if (KEY_LEFT_SHIFT)
		value &= ~0x80;
	if (KEY_E)
		value &= ~0x40;
	if (KEY_S)
		value &= ~0x20;
	if (KEY_Z)
		value &= ~0x10;
	if (KEY_4) value &= ~8;
	if (KEY_A)
		value &= ~4;
	if (KEY_W)
		value &= ~2;
	if (KEY_3) value &= ~1;
	c64_keyline[1] = value;

	value = 0xff;
	if (KEY_X)
		value &= ~0x80;
	if (KEY_T)
		value &= ~0x40;
	if (KEY_F)
		value &= ~0x20;
	if (KEY_C)
		value &= ~0x10;
	if (KEY_6) value &= ~8;
	if (KEY_D)
		value &= ~4;
	if (KEY_R)
		value &= ~2;
	if (KEY_5) value &= ~1;
	c64_keyline[2] = value;

	value = 0xff;
	if (KEY_V)
		value &= ~0x80;
	if (KEY_U)
		value &= ~0x40;
	if (KEY_H)
		value &= ~0x20;
	if (KEY_B)
		value &= ~0x10;
	if (KEY_8) value &= ~8;
	if (KEY_G)
		value &= ~4;
	if (KEY_Y)
		value &= ~2;
	if (KEY_7) value &= ~1;
	c64_keyline[3] = value;

	value = 0xff;
	if (KEY_N)
		value &= ~0x80;
	if (KEY_O)
		value &= ~0x40;
	if (KEY_K)
		value &= ~0x20;
	if (KEY_M)
		value &= ~0x10;
	if (KEY_0)
		value &= ~8;
	if (KEY_J)
		value &= ~4;
	if (KEY_I)
		value &= ~2;
	if (KEY_9)
		value &= ~1;
	c64_keyline[4] = value;

	value = 0xff;
	if (KEY_COMMA)
		value &= ~0x80;
	if (KEY_ATSIGN)
		value &= ~0x40;
	if (KEY_SEMICOLON)
		value &= ~0x20;
	if (KEY_POINT)
		value &= ~0x10;
	if (KEY_MINUS)
		value &= ~8;
	if (KEY_L)
		value &= ~4;
	if (KEY_P)
		value &= ~2;
	if (KEY_PLUS)
		value &= ~1;
	c64_keyline[5] = value;

	
	value = 0xff;
	if (KEY_SLASH)
		value &= ~0x80;
	if (KEY_ARROW_UP)
		value &= ~0x40;
	if (KEY_EQUALS)
		value &= ~0x20;
	if (c128) {
		if (C128_KEY_RIGHT_SHIFT)
		value &= ~0x10;
	} else if (c65) {
		if (C65_KEY_RIGHT_SHIFT)
		value &= ~0x10;
	} else {
		if (KEY_RIGHT_SHIFT)
		value &= ~0x10;
	}
	if (KEY_HOME)
		value &= ~8;
	if (KEY_COLON)
		value &= ~4;
	if (KEY_ASTERIX)
		value &= ~2;
	if (KEY_POUND)
		value &= ~1;
	c64_keyline[6] = value;

	value = 0xff;
	if (c65) {
		if (C65_KEY_STOP)
			value &= ~0x80;
		if (C65_KEY_SPACE)
			value &= ~0x10;
		if (C65_KEY_CTRL)
			value &= ~4;
	} else {
		if (KEY_STOP)
			value &= ~0x80;
		if (KEY_SPACE)
			value &= ~0x10;
		if (KEY_CTRL)
			value &= ~4;
	}
	if (KEY_Q)
		value &= ~0x40;
	if (KEY_CBM)
		value &= ~0x20;
	if (KEY_2) value &= ~8;
	if (KEY_ARROW_LEFT)
		value &= ~2;
	if (KEY_1) value &= ~1;
	c64_keyline[7] = value;

	value = 0xff;
	if (JOYSTICK1||JOYSTICK1_2BUTTON) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_RIGHT)
			value &= ~8;
		if (JOYSTICK_1_LEFT)
			value &= ~4;
		if (JOYSTICK_1_DOWN)
			value &= ~2;
		if (JOYSTICK_1_UP)
			value &= ~1;
	} else if (PADDLES12) {
		if (PADDLE2_BUTTON)
			value &= ~8;
		if (PADDLE1_BUTTON)
			value &= ~4;
	} else if (MOUSE1) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_BUTTON2)
			value &= ~1;
	}
	c64_keyline[8] = value;

	value2 = 0xff;
	if (JOYSTICK2||JOYSTICK2_2BUTTON) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_RIGHT)
			value2 &= ~8;
		if (JOYSTICK_2_LEFT)
			value2 &= ~4;
		if (JOYSTICK_2_DOWN)
			value2 &= ~2;
		if (JOYSTICK_2_UP)
			value2 &= ~1;
	} else if (PADDLES34) {
		if (PADDLE4_BUTTON)
			value2 &= ~8;
		if (PADDLE3_BUTTON)
			value2 &= ~4;
	} else if (MOUSE2) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_BUTTON2)
			value2 &= ~1;
	}
	c64_keyline[9] = value2;

	if ( c128 ) {
		value = 0xff;
		if (KEY_NUM1)
			value &= ~0x80;
		if (KEY_NUM7)
			value &= ~0x40;
		if (KEY_NUM4)
			value &= ~0x20;
		if (KEY_NUM2)
			value &= ~0x10;
		if (KEY_TAB)
			value &= ~8;
		if (KEY_NUM5)
			value &= ~4;
		if (KEY_NUM8)
			value &= ~2;
		if (KEY_HELP)
			value &= ~1;
		c128_keyline[0] = value;
		
		value = 0xff;
		if (KEY_NUM3)
			value &= ~0x80;
		if (KEY_NUM9)
			value &= ~0x40;
		if (KEY_NUM6)
			value &= ~0x20;
		if (KEY_NUMENTER)
			value &= ~0x10;
		if (KEY_LINEFEED)
			value &= ~8;
		if (KEY_NUMMINUS)
			value &= ~4;
		if (KEY_NUMPLUS)
			value &= ~2;
		if (KEY_ESCAPE)
			value &= ~1;
		c128_keyline[1] = value;
		
		value = 0xff;
		if (KEY_NOSCRL)
			value &= ~0x80;
		if (KEY_RIGHT)
			value &= ~0x40;
		if (KEY_LEFT)
			value &= ~0x20;
		if (KEY_DOWN)
			value &= ~0x10;
		if (KEY_UP)
			value &= ~8;
		if (KEY_NUMPOINT)
			value &= ~4;
		if (KEY_NUM0)
			value &= ~2;
		if (KEY_ALT)
			value &= ~1;
		c128_keyline[2] = value;
	}

	if (c65) {
		value = 0xff;
		if (C65_KEY_ESCAPE)
			value &= ~0x80;
		if (C65_KEY_F13)
			value &= ~0x40; //?
		if (C65_KEY_F11)
			value &= ~0x20; //?
		if (C65_KEY_F9)
			value &= ~0x10; //?
		if (C65_KEY_HELP)
			value &= ~8;
		if (C65_KEY_ALT) //? non blocking
			value &= ~4;
		if (C65_KEY_TAB)
			value &= ~2;
		if (C65_KEY_NOSCRL) //?
			value &= ~1;
		c65_keyline[0] = value;
		value = 0xff;
		if (C65_KEY_DIN) value &= ~0x80; //?
		//if (KEY_5) value &= ~0x8; // left
		//if (KEY_6) value &= ~0x4; // down
		c65_keyline[1] = value;
	}

	vic2_frame_interrupt ();

	if (c64_tape_on) {
		vc20_tape_config (DATASSETTE, DATASSETTE_TONE);
		vc20_tape_buttons (DATASSETTE_PLAY, DATASSETTE_RECORD, DATASSETTE_STOP);
	}
	osd_led_w (1 /*KB_CAPSLOCK_FLAG */ , KEY_SHIFTLOCK ? 1 : 0);
	osd_led_w (0 /*KB_NUMLOCK_FLAG */ , JOYSTICK_SWAP ? 1 : 0);

	return ignore_interrupt ();
}

void c64_state(PRASTER *this)
{
	int y;
	char text[70];
	
	y = Machine->gamedrv->drv->visible_area.max_y + 1 - Machine->uifont->height;
	
#if VERBOSE_DBG
#if 0
	cia6526_status (text, sizeof (text));
	praster_draw_text (this, text, &y);

	snprintf (text, size, "c64 vic:%.4x m6510:%d exrom:%d game:%d",
			  c64_vicaddr - c64_memory, c64_port6510 & 7,
			  c64_exrom, c64_game);
	praster_draw_text (this, text, &y);
#endif
	vdc8563_status(text, sizeof(text));
	praster_draw_text (this, text, &y);
#endif
	
	vc20_tape_status (text, sizeof (text));
	praster_draw_text (this, text, &y);
#ifdef VC1541
	vc1541_drive_status (text, sizeof (text));
#else
	cbm_drive_0_status (text, sizeof (text));
#endif
	praster_draw_text (this, text, &y);
	
	cbm_drive_1_status (text, sizeof (text));
	praster_draw_text (this, text, &y);
}

