/***************************************************************************
	commodore c65 home computer
	peter.trauner@jk.uni-linz.ac.at
    documention
     www.funet.fi
 ***************************************************************************/

#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/cia6526.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vic4567.h"
#include "includes/sid6581.h"
#include "mscommon.h"

#include "includes/c65.h"

static int c65_charset_select=0;

static int c64mode=0;

/*UINT8 *c65_basic; */
/*UINT8 *c65_kernal; */
UINT8 *c65_chargen;
/*UINT8 *c65_dos; */
/*UINT8 *c65_monitor; */
UINT8 *c65_interface;
/*UINT8 *c65_graphics; */

/* processor has only 1 mega address space !? */
/* and system 8 megabyte */
/* dma controller and bankswitch hardware ?*/
static READ_HANDLER(c65_read_mem)
{
	int data=0;

	if (offset<0x800000)
		data=c64_memory[offset];

#if 0
	if (offset<0x100000)
	else data=c64_memory[offset];
#endif

	return data;
}

static WRITE_HANDLER(c65_write_mem)
{
	if (offset<0x20000)
		c64_memory[offset]=data;
	else if (offset<0x80000) ;
	else if (offset<0x100000) {
		if (C65_MAIN_MEMORY==C65_512KB) c64_memory[offset]=data;
	} else if (offset<0x400000) ;
	else if (offset<0x800000) {
		if (C65_MAIN_MEMORY==C65_4096KB) c64_memory[offset]=data;
	}
#if 0
	if (offset<0x100000) program_write_byte_8(offset,data);
	else c64_memory[offset]=data;
#endif
}

static struct {
	int version;
	UINT8 data[4];
} dma;
/* dma chip at 0xd700
  used:
   writing banknumber to offset 2
   writing hibyte to offset 1
   writing lobyte to offset 0
    cpu holded, dma transfer (data at address) executed, cpu activated

  command data:
   0 command (0 copy, 3 fill)
   1,2 length
   3,4,5 source
   6,7,8 dest
   9 subcommand
   10 mod

   version 1:
   seldom copy (overlapping) from 0x402002 to 0x402008
   (making place for new line in basic area)
   for whats this bit 0x400000, or is this really the address?
   maybe means add counter to address for access,
   so allowing up or down copies, and reordering copies

   version 2:
   cmd 0x30 used for this
*/
static void c65_dma_port_w(int offset, int value)
{
	static int dump=0;
	PAIR pair, src, dst, len;
	UINT8 cmd, fill;
	int i;

	switch (offset&3) {
	case 2:
	case 1:
		dma.data[offset&3]=value;
		break;
	case 0:
		pair.b.h3=0;
		pair.b.h2=dma.data[2];
		pair.b.h=dma.data[1];
		pair.b.l=dma.data[0]=value;
		cmd=c65_read_mem(pair.d++);
		len.w.h=0;
		len.b.l=c65_read_mem(pair.d++);
		len.b.h=c65_read_mem(pair.d++);
		src.b.h3=0;
		fill=src.b.l=c65_read_mem(pair.d++);
		src.b.h=c65_read_mem(pair.d++);
		src.b.h2=c65_read_mem(pair.d++);
		dst.b.h3=0;
		dst.b.l=c65_read_mem(pair.d++);
		dst.b.h=c65_read_mem(pair.d++);
		dst.b.h2=c65_read_mem(pair.d++);

		switch (cmd) {
		case 0:
			if (src.d==0x3ffff) dump=1;
			if (dump)
				DBG_LOG(1,"dma copy job",
						("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
						 len.w.l, src.d, dst.d, c65_read_mem(pair.d),
						 c65_read_mem(pair.d+1) ) );
			if ( (dma.version==1)
				 &&( (src.d&0x400000)||(dst.d&0x400000)) ) {
				if ( !(src.d&0x400000) ) {
					dst.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(dst.d--,c65_read_mem(src.d++));
				} else if ( !(dst.d&0x400000) ) {
					src.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(dst.d++,c65_read_mem(src.d--));
				} else {
					src.d&=~0x400000;
					dst.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(--dst.d,c65_read_mem(--src.d));
				}
			} else {
				for (i=0; i<len.w.l; i++)
					c65_write_mem(dst.d++,c65_read_mem(src.d++));
			}
			break;
		case 3:
			DBG_LOG(3,"dma fill job",
					("len:%.4x value:%.2x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, fill, dst.d, c65_read_mem(pair.d),
					 c65_read_mem(pair.d+1)));
				for (i=0; i<len.w.l; i++)
					c65_write_mem(dst.d++,fill);
				break;
		case 0x30:
			DBG_LOG(1,"dma copy down",
					("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, src.d, dst.d, c65_read_mem(pair.d),
					 c65_read_mem(pair.d+1) ) );
			for (i=0; i<len.w.l; i++)
				c65_write_mem(dst.d--,c65_read_mem(src.d--));
			break;
		default:
			DBG_LOG(1,"dma job",
					("cmd:%.2x len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 cmd,len.w.l, src.d, dst.d, c65_read_mem(pair.d),
					 c65_read_mem(pair.d+1)));
		}
		break;
	default:
		DBG_LOG (1, "dma chip write", ("%.3x %.2x\n", offset,value));
		break;
	}
}

static int c65_dma_port_r(int offset)
{
	/* offset 3 bit 7 in progress ? */
	DBG_LOG (2, "dma chip read", ("%.3x\n", offset));
    return 0x7f;
}

static void c65_6511_port_w(int offset, int value)
{
	if (offset==7) {
		c65_6511_port=value;
	}
	DBG_LOG (2, "r6511 write", ("%.2x %.2x\n", offset, value));
}

static int c65_6511_port_r(int offset)
{
	int data=0xff;
	if (offset==7) {
		if (C65_KEY_DIN) data &= ~1;
	}
	DBG_LOG (2, "r6511 read", ("%.2x\n", offset));

	return data;
}

/* one docu states custom 4191 disk controller
 (for 2 1MB MFM disk drives, 1 internal, the other extern (optional) 1565
 with integrated 512 byte buffer

 0->0 reset ?

 0->1, 0->0, wait until 2 positiv, 1->0 ???

 0->0, 0 not 0 means no drive ???, other system entries


 reg 0 write/read
  0,1 written
  bit 1 set
  bit 2 set
  bit 3 set
  bit 4 set


 reg 0 read
  bit 0
  bit 1
  bit 2
  0..2 ->$1d4

 reg 1 write
  $01 written
  $18 written
  $46 written
  $80 written
  $a1 written
  $01 written, dec
  $10 written

 reg 2 read/write?(lsr)
  bit 2
  bit 4
  bit 5 busy waiting until zero, then reading reg 7
  bit 6 operation not activ flag!? or set overflow pin used
  bit 7 busy flag?

 reg 3 read/write?(rcr)
  bit 1
  bit 3
  bit 7 busy flag?

 reg 4
  track??
  0 written
  read -> $1d2
  cmp #$50
  bcs


 reg 5
  sector ??
  1 written
  read -> $1d3
  cmp #$b bcc


 reg 6
  head ??
  0 written
  read -> $1d1
  cmp #2 bcc

 reg 7 read
  #4e written
  12 times 0, a1 a1 a1 fe  written

 reg 8 read
  #ff written
  16 times #ff written

 reg 9
  #60 written

might use the set overflow input

$21a6c 9a6c format
$21c97 9c97 write operation
$21ca0 9ca0 get byte?
$21cab 9cab read reg 7
$21caf 9caf write reg 7
$21cb3
*/
static struct {
	int state;

	UINT8 reg[0x0f];

	UINT8 buffer[0x200];
	int cpu_pos;
	int fdc_pos;

	UINT16 status;

	double time;
	int head,track,sector;
} c65_fdc= { 0 };

#define FDC_LOST 4
#define FDC_CRC 8
#define FDC_RNF 0x10
#define FDC_BUSY 0x80
#define FDC_IRQ 0x200

#define FDC_CMD_MOTOR_SPIN_UP 0x10

#if 0
static void c65_fdc_state(void)
{
	switch (c65_fdc.state) {
	case FDC_CMD_MOTOR_SPIN_UP:
		if (timer_get_time()-c65_fdc.time) {
			c65_fdc.state=0;
			c65_fdc.status&=~FDC_BUSY;
		}
		break;
	}
}
#endif

static void c65_fdc_w(int offset, int data)
{
	DBG_LOG (1, "fdc write", ("%.5x %.2x %.2x\n", activecpu_get_pc(), offset, data));
	switch (offset&0xf) {
	case 0:
		c65_fdc.reg[0]=data;
		break;
	case 1:
		c65_fdc.reg[1]=data;
		switch (data&0xf9) {
		case 0x20: // wait for motor spin up
			c65_fdc.status&=~(FDC_IRQ|FDC_LOST|FDC_CRC|FDC_RNF);
			c65_fdc.status|=FDC_BUSY;
			c65_fdc.time=timer_get_time();
			c65_fdc.state=FDC_CMD_MOTOR_SPIN_UP;
			break;
		case 0: // cancel
			c65_fdc.status&=~(FDC_BUSY);
			c65_fdc.state=0;
			break;
		case 0x80: // buffered write
		case 0x40: // buffered read
		case 0x81: // unbuffered write
		case 0x41: // unbuffered read
		case 0x30:case 0x31: // step
			break;
		}
		break;
	case 2: case 3: // read only
		break;
	case 4:
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.track=data;
		break;
	case 5:
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.sector=data;
		break;
	case 6:
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.head=data;
		break;
	case 7:
		c65_fdc.buffer[c65_fdc.cpu_pos++]=data;
		break;
	default:
		c65_fdc.reg[offset&0xf]=data;
		break;
	}
}

static int c65_fdc_r(int offset)
{
	UINT8 data=0;
	switch (offset&0xf) {
	case 0:
		data=c65_fdc.reg[0];
		break;
	case 1:
		data=c65_fdc.reg[1];
		break;
	case 2:
		data=c65_fdc.status;
		break;
	case 3:
		data=c65_fdc.status>>8;
		break;
	case 4:
		data=c65_fdc.track;
		break;
	case 5:
		data=c65_fdc.sector;
		break;
	case 6:
		data=c65_fdc.head;
		break;
	case 7:
		data=c65_fdc.buffer[c65_fdc.cpu_pos++];
		break;
	default:
		data=c65_fdc.reg[offset&0xf];
		break;
	}
	DBG_LOG (1, "fdc read", ("%.5x %.2x %.2x\n", activecpu_get_pc(), offset, data));
	return data;
}

/* version 1 ramcheck
   write 0:0
   read write read write 80000,90000,f0000
   write 0:8
   read write read write 80000,90000,f0000

   version 2 ramcheck???
   read 0:
   write 0:0
   read 0:
   first read and second read bit 0x80 set --> nothing
   write 0:0
   read 0
   write 0:ff
*/
static struct {
	UINT8 reg;
} expansion_ram= {0};
static READ_HANDLER(c65_ram_expansion_r)
{
	int data=0xff;
	if (C65_MAIN_MEMORY==C65_4096KB)
		data=expansion_ram.reg;
	DBG_LOG (1, "expansion read", ("%.5x %.2x %.2x\n", activecpu_get_pc(),offset,data));
	return data;
}

static WRITE_HANDLER(c65_ram_expansion_w)
{
	DBG_LOG (1, "expansion write", ("%.5x %.2x %.2x\n", activecpu_get_pc(), offset, data));
	expansion_ram.reg=data;

#if 0
	if ( (data==0)&&(C65_MAIN_MEMORY==C65_512KB) ) {
		memory_set_bankhandler_w (16, 0, MWA8_BANK16);
	} else {
		memory_set_bankhandler_w (16, 0, MWA8_NOP);
	}
#endif
}

static WRITE_HANDLER ( c65_write_io )
{
	switch(offset&0xf00) {
	case 0x000:
		if (offset < 0x80)
			vic3_port_w (offset & 0x7f, data);
		else if (offset < 0xa0) {
			c65_fdc_w(offset&0x1f,data);
		} else {
			c65_ram_expansion_w(offset&0x1f, data);
			/*ram expansion crtl optional */
		}
		break;
	case 0x100:case 0x200: case 0x300:
		vic3_palette_w(offset-0x100,data);
		break;
	case 0x400:
		if (offset<0x420) /* maybe 0x20 */
			sid6581_0_port_w (offset & 0x3f, data);
		else if (offset<0x440)
			sid6581_1_port_w(offset&0x3f, data);
		else
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x500:
		DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x600:
		c65_6511_port_w(offset&0xff,data);
		break;
	case 0x700:
		c65_dma_port_w(offset&0xff, data);
		break;
	}
}

static WRITE_HANDLER ( c65_write_io_dc00 )
{
	switch(offset&0xf00) {
	case 0x000:
		cia6526_0_port_w (offset & 0xff, data);
		break;
	case 0x100:
		cia6526_1_port_w (offset & 0xff, data);
		break;
	case 0x200:
	case 0x300:
		DBG_LOG (1, "io write", ("%.3x %.2x\n", offset+0xc00, data));
		break;
	}
}

static READ_HANDLER ( c65_read_io )
{
	switch(offset&0xf00) {
	case 0x000:
		if (offset < 0x80)
			return vic3_port_r (offset & 0x7f);
		if (offset < 0xa0) {
			return c65_fdc_r(offset&0x1f);
		} else {
			return c65_ram_expansion_r(offset&0x1f);
			/*return; ram expansion crtl optional */
		}
		break;
	case 0x100:case 0x200: case 0x300:
	/* read only !? */
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x400:
		if (offset<0x420)
			return sid6581_0_port_r (offset & 0x3f);
		if (offset<0x440)
			return sid6581_1_port_r(offset&0x3f);
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x500:
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x600:
		return c65_6511_port_r(offset&0xff);
	case 0x700:
		return c65_dma_port_r(offset&0xff);
	}
	return 0xff;
}

static READ_HANDLER ( c65_read_io_dc00 )
{
	switch(offset&0x300) {
	case 0x000:
		return cia6526_0_port_r (offset & 0xff);
	case 0x100:
		return cia6526_1_port_r (offset & 0xff);
	case 0x200:
	case 0x300:
		DBG_LOG (1, "io read", ("%.3x\n", offset+0xc00));
		break;
	}
	return 0xff;
}


/*
d02f:
 init a5 96 written (seems to be switch to c65 or vic3 mode)
 go64 0 written
*/
static int c65_io_on=0, c65_io_dc00_on=0;

/* bit 1 external sync enable (genlock)
   bit 2 palette enable
   bit 6 vic3 c65 character set */
static void c65_bankswitch_interface(int value)
{
	static int old=0;
	read8_handler rh;
	write8_handler wh;

	DBG_LOG (2, "c65 bankswitch", ("%.2x\n",value));

	if (c65_io_on)
	{
		if (value&1)
		{
			cpu_setbank (8, c64_colorram + 0x400);
			cpu_setbank (9, c64_colorram + 0x400);
			rh = MRA8_BANK8;
			wh = MWA8_BANK9;
		}
		else
		{
			rh = c65_read_io_dc00;
			wh = c65_write_io_dc00;
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0dc00, 0x0dfff, 0, 0, rh);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0dc00, 0x0dfff, 0, 0, wh);
	}

	c65_io_dc00_on=!(value&1);
#if 0
	/* cartridge roms !?*/
	if (value&0x08) { cpu_setbank (1, c64_roml); }
	else { cpu_setbank (1, c64_memory + 0x8000); }
	if (value&0x10) { cpu_setbank (2, c64_basic); }
	else { cpu_setbank (2, c64_memory + 0xa000); }
#endif
	if ((old^value)&0x20) { /* bankswitching faulty when doing actual page */
		if (value&0x20) { cpu_setbank (3, c65_interface); }
		else { cpu_setbank (3, c64_memory + 0xc000); }
	}
	c65_charset_select=value&0x40;
#if 0
	/* cartridge roms !?*/
	if (value&0x80) { cpu_setbank (8, c64_kernal); }
	else { cpu_setbank (6, c64_memory + 0xe000); }
#endif
	old=value;
}

void c65_bankswitch (void)
{
	static int old = -1;
	int data, loram, hiram, charen;
	read8_handler rh4, rh8;
	write8_handler wh5, wh9;

	data = ((c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff)) & 7;
	if (data == old)
		return;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		cpu_setbank (1, c64_roml);
	}
	else
	{
		cpu_setbank (1, c64_memory + 0x8000);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		cpu_setbank (2, c64_romh);
	}
	else if (loram && hiram)
	{
		cpu_setbank (2, c64_basic);
	}
	else
	{
		cpu_setbank (2, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		c65_io_on = 1;
		rh4 = c65_read_io;
		wh5 = c65_write_io;
		cpu_setbank (6, c64_colorram);
		cpu_setbank (7, c64_colorram);

		if (c65_io_dc00_on)
		{
			rh8 = c65_read_io_dc00;
			wh9 = c65_write_io_dc00;
		}
		else
		{
			rh8 = MRA8_BANK8;
			wh9 = MWA8_BANK9;
			cpu_setbank (8, c64_colorram+0x400);
			cpu_setbank (9, c64_colorram+0x400);
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0dc00, 0x0dfff, 0, 0, rh8);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0dc00, 0x0dfff, 0, 0, wh9);
	}
	else
	{
		c65_io_on = 0;
		rh4 = MRA8_BANK4;
		wh5 = MWA8_BANK5;
		cpu_setbank(5, c64_memory+0xd000);
		cpu_setbank(7, c64_memory+0xd800);
		cpu_setbank(9, c64_memory+0xdc00);
		if (!charen && (loram || hiram))
		{
			cpu_setbank (4, c64_chargen);
			cpu_setbank (6, c64_chargen+0x800);
			cpu_setbank (8, c64_chargen+0xc00);
		}
		else
		{
			cpu_setbank (4, c64_memory + 0xd000);
			cpu_setbank (6, c64_memory + 0xd800);
			cpu_setbank (8, c64_memory + 0xdc00);
		}
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0d000, 0x0d7ff, 0, 0, rh4);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0d000, 0x0d7ff, 0, 0, wh5);

	if (!c64_game && c64_exrom)
	{
		cpu_setbank (10, c64_romh);
	}
	else
	{
		if (hiram)
		{
			cpu_setbank (10, c64_kernal);
		}
		else
		{
			cpu_setbank (10, c64_memory + 0xe000);
		}
	}
	old = data;
}

void c65_colorram_write (int offset, int value)
{
	c64_colorram[offset & 0x7ff] = value | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c65_dma_read (int offset)
{
	if (!c64_game && c64_exrom)
	{
		if (offset < 0x3000)
			return c64_memory[offset];
		return c64_romh[offset & 0x1fff];
	}
	if ((c64_vicaddr == c64_memory) || (c64_vicaddr == c64_memory + 0x8000))
	{
		if (offset < 0x1000)
			return c64_vicaddr[offset & 0x3fff];
		if (offset < 0x2000) {
			if (c65_charset_select)
				return c65_chargen[offset & 0xfff];
			else
				return c64_chargen[offset & 0xfff];
		}
		return c64_vicaddr[offset & 0x3fff];
	}
	return c64_vicaddr[offset & 0x3fff];
}

static int c65_dma_read_color (int offset)
{
	if (c64mode) return c64_colorram[offset&0x3ff]&0xf;
	return c64_colorram[offset & 0x7ff];
}

static void c65_common_driver_init (void)
{
	c65=1;
	c64_tape_on=0;
	/*memset(c64_memory+0x40000, 0, 0x800000-0x40000); */
	cbm_drive_open ();

	cia6526_init();

	c64_cia0.todin50hz = c64_cia1.todin50hz = c64_pal;
	cia6526_config (0, &c64_cia0);
	cia6526_config (1, &c64_cia1);
	vic4567_init (c64_pal, c65_dma_read, c65_dma_read_color,
				  c64_vic_interrupt, c65_bankswitch_interface);
	statetext_add_function(c65_state);
}

void c65_driver_init (void)
{
	dma.version=2;
	c65_common_driver_init ();

}

void c65_driver_alpha1_init (void)
{
	dma.version=1;
	c65_common_driver_init ();
}

void c65pal_driver_init (void)
{
	dma.version=1;
	c64_pal = 1;
	c65_common_driver_init ();
}

void c65_driver_shutdown (void)
{
	cbm_drive_close ();
}

MACHINE_INIT( c65 )
{
	memset(c64_memory+0x40000, 0xff, 0xc0000);

	sid6581_reset(0);
	sid6581_reset(1);

	cbm_serial_reset_write (0);
	cbm_drive_0_config (SERIAL8ON ? SERIAL : 0, 10);
	cbm_drive_1_config (SERIAL9ON ? SERIAL : 0, 11);
	cia6526_reset ();
	c64_vicaddr = c64_memory;

	c64_port6510 = 0xff;
	c64_ddr6510 = 0;
	c64mode = 0;

	c64_rom_recognition ();
	c64_rom_load();

	c65_bankswitch_interface(0xff);
	c65_bankswitch ();
}

void c65_state (void)
{
	char text[70];

#if VERBOSE_DBG
	cia6526_status (text, sizeof (text));
	statetext_display_text (text);

	snprintf (text, sizeof(text), "c65 vic:%.4x m6510:%d c64:%d",
			  c64_vicaddr - c64_memory, c64_port6510 & 7, c64mode);
#endif
	cbm_drive_0_status (text, sizeof (text));
	statetext_display_text (text);

	cbm_drive_1_status (text, sizeof (text));
	statetext_display_text (text);
}
