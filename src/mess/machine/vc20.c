/***************************************************************************

	commodore vic20 home computer
	Peter Trauner
	(peter.trauner@jk.uni-linz.ac.at)

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/
#include <ctype.h>
#include <stdio.h>

#include "driver.h"
#include "osd_cpu.h"
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 0
#include "cbm.h"

#include "mess/machine/vc20.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/6522via.h"
#include "mess/machine/vc20tape.h"
#include "mess/machine/c1551.h"
#include "mess/machine/cbmieeeb.h"
#include "mess/vidhrdw/vic6560.h"

static UINT8 keyboard[8] =
{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static int via1_portb, via0_ca2;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

static int ieee=0; /* ieee cartridge (interface and rom)*/

UINT8 *vc20_memory;
UINT8 *vc20_memory_9400;

/** via 0 addr 0x9110
 ca1 restore key (low)
 ca2 cassette motor on
 pa0 serial clock in
 pa1 serial data in
 pa2 also joy 0 in
 pa3 also joy 1 in
 pa4 also joy 2 in
 pa5 also joybutton/lightpen in
 pa6 cassette switch in
 pa7 inverted serial atn out
 pa2 till pa6, port b, cb1, cb2 userport
 irq connected to m6502 nmi
*/
static void vc20_via0_irq (int level)
{
	cpu_set_nmi_line (0, level);
}

static int vc20_via0_read_ca1 (int offset)
{
	return (KEYBOARD_EXTRA & KEY_RESTORE) ? 0 : 1;
}

static int vc20_via0_read_ca2 (int offset)
{
	DBG_LOG (1, "tape", ("motor read %d\n", via0_ca2));
	return via0_ca2;
}

static void vc20_via0_write_ca2 (int offset, int data)
{
	via0_ca2 = data ? 1 : 0;
	vc20_tape_motor (via0_ca2);
}

static int vc20_via0_read_porta (int offset)
{
	int value = 0xff;

	if (JOYSTICK)
		value &= readinputport (0) | JOY_VIA0_IGNORE;
	if (PADDLES)
		value &= readinputport (1) | JOY_VIA0_IGNORE;
	/* to short to be recognized normally */
	/* should be reduced to about 1 or 2 microseconds */
	/*  if(LIGHTPEN_BUTTON) value&=~0x20; */
	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~1;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~2;
	if (!vc20_tape_switch ())
		value &= ~0x40;
	return value;
}

static void vc20_via0_write_porta (int offset, int data)
{
	cbm_serial_atn_write (serial_atn = !(data & 0x80));
	DBG_LOG (1, "serial out", ("atn %s\n", serial_atn ? "high" : "low"));
}

/* via 1 addr 0x9120
 * port a input from keyboard (low key pressed in line of matrix
 * port b select lines of keyboard matrix (low)
 * pb7 also joystick right in! (low)
 * pb3 also cassette write
 * ca1 cassette read
 * ca2 inverted serial clk out
 * cb1 serial srq in
 * cb2 inverted serial data out
 * irq connected to m6502 irq
 */
static void vc20_via1_irq (int level)
{
	cpu_set_irq_line (0, M6502_INT_IRQ, level);
}

static int vc20_via1_read_porta (int offset)
{
	int value = 0xff;

	if (!(via1_portb & 0x01))
		value &= keyboard[0];

	if (!(via1_portb & 0x02))
		value &= keyboard[1];

	if (!(via1_portb & 0x04))
		value &= keyboard[2];

	if (!(via1_portb & 0x08))
		value &= keyboard[3];

	if (!(via1_portb & 0x10))
		value &= keyboard[4];

	if (!(via1_portb & 0x20))
		value &= keyboard[5];

	if (!(via1_portb & 0x40))
		value &= keyboard[6];

	if (!(via1_portb & 0x80))
		value &= keyboard[7];

	return value;
}

static int vc20_via1_read_ca1 (int offset)
{
	return vc20_tape_read ();
}

static void vc20_via1_write_ca2 (int offset, int data)
{
	cbm_serial_clock_write (serial_clock = !data);
}

static int vc20_via1_read_portb (int offset)
{
	int value = 0xff;

	if (JOYSTICK)
		value &= readinputport (0) | JOY_VIA1_IGNORE;
	if (PADDLES)
		value &= readinputport (1) | JOY_VIA1_IGNORE;

	return value;
}

static void vc20_via1_write_portb (int offset, int data)
{
/*  if( errorlog ) fprintf(errorlog, "via1_write_portb: $%02X\n", data); */
	vc20_tape_write (data & 8 ? 1 : 0);
	via1_portb = data;
}

static int vc20_via1_read_cb1 (int offset)
{
	DBG_LOG (1, "serial in", ("request read\n"));
	return cbm_serial_request_read ();
}

static void vc20_via1_write_cb2 (int offset, int data)
{
	cbm_serial_data_write (serial_data = !data);
}

/* ieee 6522 number 1 (via4)
 port b
  0 dav out
  1 nrfd out
  2 ndac out (or port a pin 2!?)
  3 eoi in
  4 dav in
  5 nrfd in
  6 ndac in
  7 atn in
 */
static int vc20_via4_read_portb(int offset)
{
	int data=0;
	if (cbm_ieee_eoi_r()) data|=8;
	if (cbm_ieee_dav_r()) data|=0x10;
	if (cbm_ieee_nrfd_r()) data|=0x20;
	if (cbm_ieee_ndac_r()) data|=0x40;
	if (cbm_ieee_atn_r()) data|=0x80;
	return data;
}

static void vc20_via4_write_portb(int offset, int data )
{
	cbm_ieee_dav_w(0,data&1);
	cbm_ieee_nrfd_w(0,data&2);
	cbm_ieee_ndac_w(0,data&4);
}

/* ieee 6522 number 2 (via5)
   port a data read
   port b data write
   cb1 srq in ?
   cb2 eoi out
   ca2 atn out
*/
static void vc20_via5_write_porta(int offset, int data)
{
	cbm_ieee_data_w(0,data);
}

static int vc20_via5_read_portb(int offset)
{
	return cbm_ieee_data_r();
}

static void vc20_via5_write_ca2(int offset,int level)
{
	cbm_ieee_atn_w(0,level);
}

static int vc20_via5_read_cb1( int offset)
{
	return cbm_ieee_srq_r();
}

static void vc20_via5_write_cb2( int offset, int data )
{
	cbm_ieee_eoi_w(0,data);
}

static struct via6522_interface via0 =
{
	vc20_via0_read_porta,
	0,								   /*via0_read_portb, */
	vc20_via0_read_ca1,
	0,								   /*via0_read_cb1, */
	vc20_via0_read_ca2,
	0,								   /*via0_read_cb2, */
	vc20_via0_write_porta,
	0,								   /*via0_write_portb, */
	vc20_via0_write_ca2,
	0,								   /*via0_write_cb2, */
	vc20_via0_irq
}, via1 =
{
	vc20_via1_read_porta,
	vc20_via1_read_portb,
	vc20_via1_read_ca1,
	vc20_via1_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	0,								   /*via1_write_porta, */
	vc20_via1_write_portb,
	vc20_via1_write_ca2,
	vc20_via1_write_cb2,
	vc20_via1_irq
}, 
/* via2,3 used by vc1541 and 2031 disk drives */
via4 =
{
	0, //vc20_via4_read_porta,
	vc20_via4_read_portb,
	0, //vc20_via4_read_ca1,
	0, //vc20_via5_read_cb1,
	0, // via1_read_ca2
	0,								   /*via1_read_cb2, */
	0,								   /*via1_write_porta, */
	vc20_via4_write_portb,
	0, //vc20_via5_write_ca2,
	0, //vc20_via5_write_cb2,
	vc20_via1_irq
}, via5 =
{
	0,//vc20_via5_read_porta,
	vc20_via5_read_portb,
	0, //vc20_via5_read_ca1,
	vc20_via5_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via5_write_porta,
	0,//vc20_via5_write_portb,
	vc20_via5_write_ca2,
	vc20_via5_write_cb2,
	vc20_via1_irq
};

WRITE_HANDLER ( vc20_write_9400 )
{
	vc20_memory_9400[offset] = data | 0xf0;
}


int vic6560_dma_read_color (int offset)
{
	return vc20_memory_9400[offset & 0x3ff];
}

int vic6560_dma_read (int offset)
{
	/* should read real system bus between 0x9000 and 0xa000 */
	return vc20_memory[VIC6560ADDR2VC20ADDR (offset)];
}

static void vc20_memory_init(void)
{
	static int inited=0;
	int i;
	UINT8 *memory = memory_region (REGION_CPU1);

	if (inited) return;

/* memory[0x288]=0xff;// makes ae's graphics look correctly */
/* memory[0xd]=0xff; // for moneywars */
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < 0x400; i += 0x40)
	{
		memset (memory + i, i & 0x40 ? 0 : 0xff, 0x40);
		memset (memory+0x9400 + i, 0xf0 | (i & 0x40 ? 0 : 0xf), 0x40);
	}
/* for (i=0x1000;i<0x2000;i+=0x40) memset(memory+i,i&0x40?0:0xff,0x40); */

	/* i think roms look like 0xff */
	memset (memory + 0x400, 0xff, 0x1000 - 0x400);
	memset (memory + 0x2000, 0xff, 0x6000);
	memset (memory + 0xa000, 0xff, 0x1000);
	
	// clears ieee cartrige rom
	// memset (memory + 0xa000, 0xff, 0x2000);

	inited=1;
}

static void vc20_common_driver_init (void)
{
#ifdef VC1541
	VC1541_CONFIG vc1541= { 1, 8 };
#endif
	vc20_memory_init();

	vc20_tape_open (via_1_ca1_w);

	cbm_drive_open ();

	cbm_drive_attach_fs (0);
	cbm_drive_attach_fs (1);

#ifdef VC1541
	vc1541_config (0, 0, &vc1541);
#endif
	via_config (0, &via0);
	via_config (1, &via1);
}

/* currently not used, but when time comes */
void vc20_driver_shutdown (void)
{
	cbm_drive_close ();
	vc20_tape_close ();
}

void vc20_driver_init (void)
{
	vc20_common_driver_init ();
	vic6561_init (vic6560_dma_read, vic6560_dma_read_color);
}

void vic20_driver_init (void)
{
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
}

extern void vic20ieee_driver_init (void)
{
	ieee=1;
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
	via_config (4, &via4);
	via_config (5, &via5);
	cbm_ieee_open();
}

void vc20_init_machine (void)
{
	if (RAMIN0X0400)
	{
		install_mem_write_handler (0, 0x400, 0xfff, MWA_RAM);
		install_mem_read_handler (0, 0x400, 0xfff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x400, 0xfff, MWA_NOP);
		install_mem_read_handler (0, 0x400, 0xfff, MRA_ROM);
	}
	if (RAMIN0X2000)
	{
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_RAM);
		install_mem_read_handler (0, 0x2000, 0x3fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_NOP);
		install_mem_read_handler (0, 0x2000, 0x3fff, MRA_ROM);
	}
	if (RAMIN0X4000)
	{
		install_mem_write_handler (0, 0x4000, 0x5fff, MWA_RAM);
		install_mem_read_handler (0, 0x4000, 0x5fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x4000, 0x5fff, MWA_NOP);
		install_mem_read_handler (0, 0x4000, 0x5fff, MRA_ROM);
	}
	if (RAMIN0X6000)
	{
		install_mem_write_handler (0, 0x6000, 0x7fff, MWA_RAM);
		install_mem_read_handler (0, 0x6000, 0x7fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x6000, 0x7fff, MWA_NOP);
		install_mem_read_handler (0, 0x6000, 0x7fff, MRA_ROM);
	}
	if (ieee)
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_ROM);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_ROM);
	}
	else if	(RAMIN0XA000)
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_RAM);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_NOP);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_ROM);
	}

	cbm_serial_reset_write (0);
#ifdef VC1541
	vc1541_reset ();
#endif
	if (ieee) {
		cbm_drive_0_config (SERIAL8ON ? IEEE : 0, 8);
		cbm_drive_1_config (SERIAL9ON ? IEEE : 0, 9);
	} else {
		cbm_drive_0_config (SERIAL8ON ? SERIAL : 0, 8);
		cbm_drive_1_config (SERIAL9ON ? SERIAL : 0, 9);
	}
	via_reset ();
	via_0_ca1_w (0, vc20_via0_read_ca1(0) );
}

void vc20_shutdown_machine (void)
{
}

int vc20_rom_load (int id)
{
	const char *name = device_filename(IO_CARTSLOT,id);
    UINT8 *mem = memory_region (REGION_CPU1);
	FILE *fp;
	int size, read;
	char *cp;
	int addr = 0;

	vc20_memory_init();

	if (name==NULL) return 1;

	if (!vc20_rom_id (id))
		return 1;
	fp = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
	if (!fp)
	{
		logerror("%s file not found\n", name);
		return 1;
	}

	size = osd_fsize (fp);

	if ((cp = strrchr (name, '.')) != NULL)
	{
		if ((cp[1] != 0) && (cp[2] == '0') && (cp[3] == 0))
		{
			switch (toupper (cp[1]))
			{
			case 'A':
				addr = 0xa000;
				break;
			case '2':
				addr = 0x2000;
				break;
			case '4':
				addr = 0x4000;
				break;
			case '6':
				addr = 0x6000;
				break;
			}
		}
		else
		{
			if (stricmp (cp, ".prg") == 0)
			{
				unsigned short in;

				osd_fread_lsbfirst (fp, &in, 2);
				logerror("rom prg %.4x\n", in);
				addr = in;
				size -= 2;
			}
		}
	}
	if (addr == 0)
	{
		if (size == 0x4000)
		{							   /* I think rom at 0x4000 */
			addr = 0x4000;
		}
		else
		{
			addr = 0xa000;
		}
	}

	logerror("loading rom %s at %.4x size:%.4x\n",name, addr, size);
	read = osd_fread (fp, mem + addr, size);
	osd_fclose (fp);
	if (read != size)
		return 1;
	return 0;
}

int vc20_rom_id (int id)
{
	const char *name = device_filename(IO_CARTSLOT,id);
    FILE *romfile;
	unsigned char magic[] =
	{0x41, 0x30, 0x20, 0xc3, 0xc2, 0xcd};	/* A0 CBM at 0xa004 (module offset 4) */
	unsigned char buffer[sizeof (magic)];
	char *cp;
	int retval;

	logerror("vc20_rom_id %s\n", name);
	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("rom %s not found\n", name);
		return 0;
	}

	retval = 0;

	osd_fseek (romfile, 4, SEEK_SET);
	osd_fread (romfile, buffer, sizeof (magic));
	osd_fclose (romfile);

	if (!memcmp (buffer, magic, sizeof (magic)))
		retval = 1;

	if ((cp = strrchr (name, '.')) != NULL)
	{
		if ((stricmp (cp + 1, "a0") == 0)
			|| (stricmp (cp + 1, "20") == 0)
			|| (stricmp (cp + 1, "40") == 0)
			|| (stricmp (cp + 1, "60") == 0)
			|| (stricmp (cp + 1, "bin") == 0)
			|| (stricmp (cp + 1, "rom") == 0)
			|| (stricmp (cp + 1, "prg") == 0))
			retval = 1;
	}

		if (retval)
			logerror("rom %s recognized\n", name);
		else
			logerror("rom %s not recognized\n", name);

	return retval;
}

int vc20_frame_interrupt (void)
{
	static int quickload = 0;

	if (!quickload && QUICKLOAD)
		cbm_quick_open (0, 0, vc20_memory);
	quickload = QUICKLOAD;

	via_0_ca1_w (0, vc20_via0_read_ca1 (0));
	keyboard[0] = KEYBOARD_ROW (0);

	keyboard[1] = KEYBOARD_ROW (1);

	keyboard[2] = KEYBOARD_ROW (2);
	if (KEYBOARD_EXTRA & KEY_CURSOR_LEFT)
		keyboard[2] &= ~0x80;		   /* CURSOR RIGHT */

	keyboard[3] = KEYBOARD_ROW (3);
	if (KEYBOARD_EXTRA & KEY_CURSOR_UP)
		keyboard[3] &= ~0x80;		   /* CURSOR DOWN */
	if (KEYBOARD_EXTRA & KEY_SHIFTLOCK)
		keyboard[3] &= ~0x02;		   /* LEFT SHIFT */

	keyboard[4] = KEYBOARD_ROW (4);
	if (KEYBOARD_EXTRA & (KEY_CURSOR_LEFT | KEY_CURSOR_UP))
		keyboard[4] &= ~0x40;		   /* RIGHT SHIFT */

	keyboard[5] = KEYBOARD_ROW (5);

	keyboard[6] = KEYBOARD_ROW (6);

	keyboard[7] = KEYBOARD_ROW (7);

	vc20_tape_config (DATASSETTE, DATASSETTE_TONE);
	vc20_tape_buttons (DATASSETTE_PLAY, DATASSETTE_RECORD, DATASSETTE_STOP);
	osd_led_w (1 /*KB_CAPSLOCK_FLAG */ , (KEYBOARD_EXTRA & KEY_SHIFTLOCK) ? 1 : 0);

	return ignore_interrupt ();
}
