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
#include "cbm.h"
#include "cia6526.h"
#include "c1551.h"
#include "vc1541.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/sndhrdw/sid6581.h"

#include "c65.h"

unsigned char c65_keyline[2] = { 0xff, 0xff };
int c65=0;
UINT8 c65_6511_port=0xff;

static int c64mode=0, c65_dosmode=1,
	c65_monitormode=0, c65_write_io, c65_write_io_dc00;

UINT8 *c65_basic;
UINT8 *c65_kernal;
UINT8 *c65_chargen;
UINT8 *c65_dos;
UINT8 *c65_monitor;
UINT8 *c65_interface;
UINT8 *c65_graphics;

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

   seldom copy (overlapping) from 0x402002 to 0x402008
   (making place for new line in basic area)
   for whats this bit 0x400000, or is this really the address?
   maybe means add counter to address for access,
   so allowing up or down copies, and reordering copies
*/
static void c65_dma_port_w(int offset, int value)
{
	static UINT8 c65_dma[4];
	static PAIR pair, src, dst, len;
	UINT8 *addr;
	int i;

	switch (offset&3) {
	case 2:
	case 1:
		c65_dma[offset&3]=value;
		break;
	case 0:
		pair.b.h3=0;
		pair.b.h2=c65_dma[2];
		pair.b.h=c65_dma[1];
		pair.b.l=c65_dma[0]=value;
		addr=c64_memory+pair.d;
		len.w.h=0; len.b.h=addr[2]; len.b.l=addr[1];
		src.b.h3=0;src.b.h2=addr[5];src.b.h=addr[4];src.b.l=addr[3];
		dst.b.h3=0;dst.b.h2=addr[8];dst.b.h=addr[7];dst.b.l=addr[6];

		switch (addr[0]) {
		case 0:
			if ( (src.d+len.w.l>=0x100000)||(dst.d+len.w.l>=0x100000)) {
				logerror("dma copy job len:%.4x src:%.6x "
							"dst:%.6x sub:%.2x modrm:%.2x\n",
							len.w.l, src.d, dst.d, addr[9], addr[10]);
			} else {
				DBG_LOG(2,"dma copy job",
						("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
						 len.w.l, src.d, dst.d, addr[9], addr[10]));
			}
			if (C65_MAIN_MEMORY==C65_512KB) {
				for (i=0; (i<len.w.l)&&(dst.d<0x100000)&&(src.d<0x100000); i++) {
					if ((dst.d<0x20000)||(dst.d>=0x80000))
						c64_memory[dst.d++]=c64_memory[src.d++];
				}
			} else {
				for (i=0; (i<len.w.l)&&(dst.d<0x800000)&&(src.d<0x100000); i++) {
					if (dst.d<0x20000)
						c64_memory[dst.d++]=c64_memory[src.d++];
				}
			}
			break;
		case 3:
			if (dst.d+len.w.l>=0x100000) {
				logerror("dma fill job len:%.4x value:%.2x "
							"dst:%.6x sub:%.2x modrm:%.2x\n",
							len.w.l, addr[3], dst.d, addr[9], addr[10]);
				return;
			} else {
				DBG_LOG(2,"dma fill job",
						("len:%.4x value:%.2x dst:%.6x sub:%.2x modrm:%.2x\n",
						 len.w.l, addr[3], dst.d, addr[9], addr[10]));
			}
			if (C65_MAIN_MEMORY==C65_512KB) {
				for (i=0; (i<len.w.l)&&(dst.d<0x100000); i++) {
					if ((dst.d<0x20000)||(dst.d>=0x80000))
						c64_memory[dst.d++]=addr[3];
				}
			} else {
				for (i=0; (i<len.w.l)&&(dst.d<0x800000); i++) {
					if (dst.d<0x20000)
						c64_memory[dst.d++]=addr[3];
				}
			}
			break;
		default:
			DBG_LOG(1,"dma job",
					("cmd:%.2x len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 addr[0],len.w.l, src.d, dst.d, addr[9], addr[10]));
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
	DBG_LOG (1, "dma chip read", ("%.3x\n", offset));
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
	DBG_LOG (2, "r6511 read", ("%.2x\n", offset));
	return 0xff;
}

static void wd1770_port_w(int offset, int value)
{
	DBG_LOG (1, "wd1770 write", ("%.2x %.2x\n", offset, value));
}

static int wd1770_port_r(int offset)
{
	DBG_LOG (1, "wd1770 read", ("%.2x\n", offset));
	return 0;
}

WRITE_HANDLER ( c65_write_d000 )
{
	if (!c65_write_io) {
		c64_memory[0xd000 + offset] = data;
	} else {
		switch(offset&0xf00) {
		case 0x000:
			if (offset < 0x80)
				vic2_port_w (offset & 0x7f, data);
			else if (offset < 0xa0) {
				wd1770_port_w(offset&0x1f,data);
			} else {
				DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
				/*ram expansion crtl optional */
			}
			break;
		case 0x100:case 0x200: case 0x300:
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
			/*ramdac ((offset&0x3ff)-0x100); */
			break;
		case 0x400:
			if (offset<0x440) /* maybe 0x20 */
				sid6581_0_port_w (offset & 0x3f, data);
			else if (offset<0x480)
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
		case 0x800:case 0x900:case 0xa00:case 0xb00:
			c64_colorram[offset & 0x3ff] = data/* | 0xf0*/;
		case 0xc00:
			if (!c65_write_io_dc00)
				c64_memory[0xd000 + offset] = data;
			else
				cia6526_0_port_w (offset & 0xff, data);
			break;
		case 0xd00:
			if (!c65_write_io_dc00)
				c64_memory[0xd000 + offset] = data;
			else
				cia6526_1_port_w (offset & 0xff, data);
			break;
		case 0xe00:
		case 0xf00:
			if (!c65_write_io_dc00)
				c64_memory[0xd000 + offset] = data;
			else
				DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
			break;
		}
	}
}

static READ_HANDLER ( c65_read_io )
{
	switch(offset&0xf00) {
	case 0x000:
		if (offset < 0x80)
			return vic2_port_r (offset & 0x7f);
		if (offset < 0xa0) {
			return wd1770_port_r(offset&0x1f);
		} else {
			DBG_LOG (1, "io read", ("%.3x\n", offset));
			/*return; ram expansion crtl optional */
		}
		break;
	case 0x100:case 0x200: case 0x300:
		DBG_LOG (1, "io read", ("%.3x\n", offset));
	    /*return ramdac ((offset&0x3ff)-0x100); */
		break;
	case 0x400:
		if (offset<0x440)
			return sid6581_0_port_r (offset & 0x3f);
		if (offset<0x480)
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
	case 0x800:case 0x900:case 0xa00:case 0xb00:
		return c64_colorram[offset & 0x3ff];
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

static void c65_bankswitch_interface(int value)
{
	if (value&0x01) {
		c65_write_io_dc00=0;
		cpu_setbankhandler_r (10, MRA_RAM);
		cpu_setbank (10, c64_memory+0xdc00);
	} else {
		c65_write_io_dc00=1;
		cpu_setbankhandler_r (10, c65_read_io_dc00);
	}
#if 0
	if (value&0x08) { cpu_setbank (6, c65_interface); }
	else { cpu_setbank (4, c64_memory + 0x8000); }
	if (value&0x10) { cpu_setbank (6, c65_interface); }
	else { cpu_setbank (5, c64_memory + 0xa000); }
#endif
	if (value&0x20) { cpu_setbank (6, c65_interface); }
	else { cpu_setbank (6, c64_memory + 0xc000); }
#if 0
	if (value&0x40) { cpu_setbank (6, c65_interface); }
	else { cpu_setbank (6, c64_memory + 0x9000); }
	if (value&0x80) { cpu_setbank (6, c65_interface); }
	else { cpu_setbank (8, c64_memory + 0xe000); }
#endif
}
/*
 8 Megabyte entire system space

 bank register values in basic and monitor
 bit 7 on io area activ, 6..0 bank selection (128 banks)

 base 1 MB system area (banks 0 till 15)
   512 kb ram expansion
   256 kb reserved
   128 kb rom
 0 128 kB ram

 seldom mapper operation (chapter1.txt)
 mapper granularity 32k
*/
void c65_bankswitch (void)
{
	DBG_LOG (1, "bankswitch", ("%.2x\n", c64_port6510));
	if (!c64mode) {
		if (c65_dosmode) {
            /* ram in cpu port position!*/
			/* c65 dos */
			/* 0000 - 1fff 10000 11fff
			   2000 - 7fff dont care!?
			   8000 - bfff dos
			   d000 - dbff io
			   dc00 - dfff colorram
			   e000 - ffff kernel ?
			*/
			cpu_setbankhandler_r(9, MRA_BANK9);
			cpu_setbankhandler_w(9, MWA_BANK9);
			cpu_setbank (9, c64_memory);
			cpu_setbank (1, c64_memory+ 0x10002);
			cpu_setbank (2, c64_memory + 0x2000); /* dont care */
			cpu_setbank (3, c64_memory + 0x6000); /* dont care */
			cpu_setbank (4, c65_dos);
			cpu_setbank (5, c65_dos + 0x2000);
			/*cpu_setbank (6, c65_interface); */
			cpu_setbankhandler_r (7, c65_read_io);
			c65_write_io = 1;
			cpu_setbank (8, c65_kernal);
		} else {
			cpu_setbankhandler_r(9, c64_m6510_port_r);
			cpu_setbankhandler_w(9, c64_m6510_port_w);
			if (c65_monitormode) {
				/* c65 monitor */
				cpu_setbank (1, c64_memory+2);
				cpu_setbank (2, c64_memory + 0x2000);
				cpu_setbank (3, c65_monitor);
				cpu_setbank (4, c64_memory + 0x8000); /* dont care */
				cpu_setbank (5, c64_basic); /* dont care*/
				/*cpu_setbank (6, c65_interface); */
				cpu_setbankhandler_r (7, c65_read_io);
				c65_write_io = 1;
				cpu_setbank (8, c65_kernal);
			} else {
				/* c65 basic */
				cpu_setbank (1, c64_memory+2);
				cpu_setbank (2, c65_basic);
				cpu_setbank (3, c65_basic + 0x4000);
				cpu_setbank (4, c65_graphics);
				cpu_setbank (5, c65_graphics+0x2000);
				/*cpu_setbank (6, c65_interface); */
				cpu_setbankhandler_r (7, c65_read_io);
				c65_write_io = 1;
				cpu_setbank (8, c65_kernal);
			}
		}
	} else {
		static int old = -1, data, loram, hiram, charen;

		cpu_setbankhandler_r(9, c64_m6510_port_r);
		cpu_setbankhandler_w(9, c64_m6510_port_w);
		data = ((c64_port6510 & c64_ddr6510) | (c64_ddr6510 ^ 0xff)) & 7;
		if (data == old)
			return;

		DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
		loram = (data & 1) ? 1 : 0;
		hiram = (data & 2) ? 1 : 0;
		charen = (data & 4) ? 1 : 0;

		cpu_setbank (1, c64_memory+2);
		cpu_setbank (2, c64_memory + 0x2000);
		cpu_setbank (3, c64_memory + 0x6000);

		if ((!c64_game && c64_exrom)
			|| (loram && hiram && !c64_exrom))
			{
				cpu_setbank (4, c64_roml);
			}
		else
			{
				cpu_setbank (4, c64_memory + 0x8000);
			}

		if ((!c64_game && c64_exrom && hiram)
			|| (!c64_exrom))
			{
				cpu_setbank (5, c64_romh);
			}
		else if (loram && hiram)
			{
				cpu_setbank (5, c64_basic);
			}
		else
			{
				cpu_setbank (5, c64_memory + 0xa000);
			}
		if ((!c64_game && c64_exrom)
			|| (charen && (loram || hiram)))
			{
				cpu_setbankhandler_r (7, c65_read_io);
				c65_write_io = 1;
			}
		else
			{
				cpu_setbankhandler_r (7, MRA_BANK5);
				c65_write_io = 0;
				if (!charen && (loram || hiram))
					{
						cpu_setbank (7, c64_chargen);
					}
				else
					{
						cpu_setbank (7, c64_memory + 0xd000);
					}
			}

		if (!c64_game && c64_exrom)
			{
				cpu_setbank (8, c64_romh);
			}
		else
			{
				if (hiram)
					{
						cpu_setbank (8, c64_kernal);
					}
				else
					{
						cpu_setbank (8, c64_memory + 0xe000);
					}
			}
		old = data;
	}
}

void c65_map(int a, int x, int y, int z)
{
	PAIR test;
	test.b.h3=a;test.b.h2=x;test.b.h=y;test.b.l=z;

	if (test.d==0x00000000) {
		c64mode=1;c65_dosmode=0;c65_monitormode=0;
	} else if (test.d==0x00118031) {
		c64mode=0;c65_dosmode=1;c65_monitormode=0;
	} else if (test.d==0xa0820083) {
		c64mode=0;c65_dosmode=0;c65_monitormode=1;
	} else {
		c64mode=0;c65_dosmode=0;c65_monitormode=0;
	}
	c65_bankswitch();
	logerror("m65ce02 map a:%.2x x:%.2x y:%.2x z:%.2x\n",
				a, x, y, z);
}


void c65_write_0002 (int offset, int data)
{
	c64_memory[2 + offset] = data;
}

WRITE_HANDLER ( c65_write_2000 )
{
	c64_memory[0x2000 + offset] = data;
}

WRITE_HANDLER ( c65_write_8000 )
{
	c64_memory[0x8000 + offset] = data;
}

WRITE_HANDLER ( c65_write_a000 )
{
	c64_memory[0xa000 + offset] = data;
}

WRITE_HANDLER ( c65_write_e000 )
{
	c64_memory[0xe000 + offset] = data;
}

void c65_colorram_write (int offset, int value)
{
	c64_colorram[offset & 0x3ff] = value | 0xf0;
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
		if (offset < 0x2000)
			return c65_chargen[offset & 0xfff];
		return c64_vicaddr[offset & 0x3fff];
	}
	return c64_vicaddr[offset & 0x3fff];
}

static int c65_dma_read_color (int offset)
{
	if (c64mode) return c64_colorram[offset&0x3ff]&0xf;
	return c64_colorram[offset & 0x7ff] & 0xf;
}

static void c65_common_driver_init (void)
{
	c65=1;
	c64_tape_on=0;
	/*    memset(c65_memory, 0, 0xfd00); */
	cbm_drive_open ();

	cbm_drive_attach_fs (0);
	cbm_drive_attach_fs (1);

	sid6581_0_init (c64_paddle_read,c64_pal);
	sid6581_1_init (NULL,c64_pal);
	c64_cia0.todin50hz = c64_cia1.todin50hz = c64_pal;
	cia6526_config (0, &c64_cia0);
	cia6526_config (1, &c64_cia1);
	vic4567_init (c64_pal, c65_dma_read, c65_dma_read_color,
				  c64_vic_interrupt, c65_bankswitch_interface);
	raster1.display_state=c65_state;
}

void c65_driver_init (void)
{
	c65_common_driver_init ();
}

void c65pal_driver_init (void)
{
	c64_pal = 1;
	c65_common_driver_init ();
}

void c65_driver_shutdown (void)
{
	cbm_drive_close ();
}

void c65_init_machine (void)
{
	memset(c64_memory+0x40000, 0xff, 0xc0000);

	sid6581_0_reset();
	sid6581_1_reset();
	sid6581_0_configure(SID8580);
	sid6581_1_configure(SID8580);

	cbm_serial_reset_write (0);
	cbm_drive_0_config (SERIAL8ON ? SERIAL : 0);
	cbm_drive_1_config (SERIAL9ON ? SERIAL : 0);
	cia6526_reset ();
	c64_vicaddr = c64_memory;

	c64_port6510 = 0xff;
	c64_ddr6510 = 0;
	c64mode = 0;

	c64_rom_recognition ();
	c64_rom_load();

	c65_bankswitch ();
}

void c65_shutdown_machine (void)
{
}

/*only for debugging */
void c65_status (char *text, int size)
{
#if VERBOSE_DBG
#if 0
	snprintf (text, size, "c65 vic:%.4x m6510:%d c64:%d dos:%d",
			  c64_vicaddr - c64_memory, c64_port6510 & 7, c64mode, c65_dosmode);
#endif
	snprintf (text, size,
			  "c65 %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x",
			  c64_keyline[0],
			  c64_keyline[1],
			  c64_keyline[2],
			  c64_keyline[3],
			  c64_keyline[4],
			  c64_keyline[5],
			  c64_keyline[6],
			  c64_keyline[7],
			  c64_keyline[8],
			  c64_keyline[9],
			  c65_keyline[0],c65_keyline[1]);
#endif
}

void c65_state (PRASTER *this)
{
#if VERBOSE_DBG
	int y;
	char text[70];

	y = Machine->gamedrv->drv->visible_area.max_y + 1
		- Machine->uifont->height;

#if 0
	cia6526_status (text, sizeof (text));
	praster_draw_text (this, text, &y);

	snprintf (text, size, "c65 vic:%.4x m6510:%d c64:%d dos:%d",
			  c64_vicaddr - c64_memory, c64_port6510 & 7, c64mode, c65_dosmode);
#endif
	snprintf (text, sizeof(text),
			  "c65 %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x",
			  c64_keyline[0],
			  c64_keyline[1],
			  c64_keyline[2],
			  c64_keyline[3],
			  c64_keyline[4],
			  c64_keyline[5],
			  c64_keyline[6],
			  c64_keyline[7],
			  c64_keyline[8],
			  c64_keyline[9],
			  c65_keyline[0],c65_keyline[1]);
	praster_draw_text (this, text, &y);
#endif
}
