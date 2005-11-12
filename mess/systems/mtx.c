/*
** mtx.c : driver for Memotech MTX512
**
**
**
*/

#include "driver.h"
#include "cpuintrf.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80fmly.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms9928a.h"
#include "sound/sn76496.h"
#include "devices/cartslot.h"
#include "devices/printer.h"

unsigned char key_sense;
int mtx_loadsize;
int mtx_saveindex;

unsigned char relcpmh;
unsigned char rampage;
unsigned char rompage;

static unsigned char *mtx_loadbuffer = NULL;
static unsigned char *mtx_savebuffer = NULL;
static unsigned char *mtx_commonram = NULL;

static char mtx_prt_strobe = 0;
static char mtx_prt_data = 0;


#define MTX_SYSTEM_CLOCK		4000000

#define MTX_PRT_BUSY		1
#define MTX_PRT_NOERROR		2
#define MTX_PRT_EMPTY		4
#define MTX_PRT_SELECTED	8


static WRITE8_HANDLER ( mtx_cst_w )
{
}

static  READ8_HANDLER ( mtx_psg_r )
{
	return 0xff;
}

static WRITE8_HANDLER ( mtx_psg_w )
{
	SN76496_0_w(offset,data);
}

static mess_image * mtx_printer_image (void)
{
	return image_from_devtype_and_index (IO_PRINTER, 0);
}

static  READ8_HANDLER ( mtx_strobe_r )
{
	if (mtx_prt_strobe == 0)
		printer_output (mtx_printer_image (), mtx_prt_data);

	mtx_prt_strobe = 1;

	return 0;
}

static  READ8_HANDLER ( mtx_prt_r )
{
	mtx_prt_strobe = 0;

	return MTX_PRT_NOERROR | (printer_status (mtx_printer_image (), 0)
			? MTX_PRT_SELECTED : 0);
}

static  WRITE8_HANDLER ( mtx_prt_w )
{
	mtx_prt_data = data;
}

static  READ8_HANDLER ( mtx_vdp_r )
{
	if (offset & 0x01)
		return TMS9928A_register_r(0);
	else
		return TMS9928A_vram_r(0);
}

static WRITE8_HANDLER ( mtx_vdp_w )
{
	if (offset & 0x01)
		TMS9928A_register_w(0, data);
	else
		TMS9928A_vram_w(0, data);
}

static WRITE8_HANDLER ( mtx_sense_w )
{
	key_sense = data;
}

static  READ8_HANDLER ( mtx_key_lo_r )
{
	unsigned char rtn = 0;

	if (key_sense==0xfe)
		rtn = readinputport(0);

	if (key_sense==0xfd)
		rtn = readinputport(1);

	if (key_sense==0xfb)
		rtn = readinputport(2);

	if (key_sense==0xf7)
		rtn = readinputport(3);

	if (key_sense==0xef)
		rtn = readinputport(4);

	if (key_sense==0xdf)
		rtn = readinputport(5);

	if (key_sense==0xbf)
		rtn = readinputport(6);

	if (key_sense==0x7f)
		rtn = readinputport(7);

	return(rtn);
}

static  READ8_HANDLER ( mtx_key_hi_r )
{
	unsigned char rtn = 0;

	unsigned char tmp;
	tmp = ((readinputport(10) & 0x03) << 2) | 0xf0;
	rtn = tmp;

	if (key_sense==0xfe)
		rtn = (readinputport(8) & 0x03) | tmp;

	if (key_sense==0xfd)
		rtn = ((readinputport(8) >> 2) & 0x03) | tmp;

	if (key_sense==0xfb)
		rtn = ((readinputport(8) >> 4) & 0x03) | tmp;

	if (key_sense==0xf7)
		rtn = ((readinputport(8) >> 6) & 0x03) | tmp;

	if (key_sense==0xef)
		rtn = (readinputport(9) & 0x03) | tmp;

	if (key_sense==0xdf)
		rtn = ((readinputport(9) >> 2) & 0x03) | tmp;

	if (key_sense==0xbf)
		rtn = ((readinputport(9) >> 4) & 0x03) | tmp;

	if (key_sense==0x7f)
		rtn = ((readinputport(9) >> 6) & 0x03) | tmp;

	return(rtn);
}

static void mtx_ctc_interrupt(int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
	cpunum_set_input_line(0, 0, state);
}

static  READ8_HANDLER ( mtx_ctc_r )
{
	return z80ctc_0_r(offset);
}

static WRITE8_HANDLER ( mtx_ctc_w )
{
	//logerror("CTC W: %02x\r\n",data);
	if (offset < 3)
		z80ctc_0_w(offset,data);
}

static z80ctc_interface	mtx_ctc_intf =
{
	1,
	{MTX_SYSTEM_CLOCK},
	{0},
	{mtx_ctc_interrupt},
	{0},
	{0},
	{0}
};

static WRITE8_HANDLER ( mtx_bankswitch_w )
{

	unsigned char *romoffset;

	unsigned int bank1, bank2, bank3, bank4, bank5, bank6;

	bank1 = 0;
	bank2 = 0;
	bank3 = 0;
	bank4 = 0;
	bank5 = 0;
	bank6 = 0;

	// todo: cpm RAM mode (relcpmh)

	relcpmh = (data & 0x80) >> 7;
	rampage = (data & 0x0f);
	rompage = (data & 0x70) >> 4;

	switch(rompage)
	{
		case 0:
			bank1=0;
			bank2=0x2000;
			break;
		case 1:
			bank1=0;
			bank2=0x4000;
			break;
	}

	switch(rampage)
	{
		case 0:
			bank3=0x6000;
			bank4=0x4000;
			bank5=0x2000;
			bank6=0;
			break;

		case 1:
			bank3=0xe000;
			bank4=0xc000;
			bank5=0xa000;
			bank6=0x8000;
			break;

		case 2:
			bank3=0x16000;
			bank4=0x14000;
			bank5=0x12000;
			bank6=0x10000;
			break;

		case 3:
			bank3=0x1e000;
			bank4=0x1c000;
			bank5=0x1a000;
			bank6=0x18000;
			break;

		case 4:
			bank3=0x26000;
			bank4=0x24000;
			bank5=0x22000;
			bank6=0x20000;
			break;

		case 5:
			bank3=0x2e000;
			bank4=0x2c000;
			bank5=0x2a000;
			bank6=0x28000;
			break;

		case 6:
			bank3=0x36000;
			bank4=0x34000;
			bank5=0x32000;
			bank6=0x30000;
			break;

		case 7:
			bank3=0x3e000;
			bank4=0x3c000;
			bank5=0x3a000;
			bank6=0x38000;
			break;

		case 8:
			bank3=0x46000;
			bank4=0x44000;
			bank5=0x42000;
			bank6=0x40000;
			break;

		case 9:
			bank3=0x4e000;
			bank4=0x4c000;
			bank5=0x4a000;
			bank6=0x48000;
			break;

		case 10:
			bank3=0x56000;
			bank4=0x54000;
			bank5=0x52000;
			bank6=0x50000;
			break;

		case 11:
			bank3=0x5e000;
			bank4=0x5c000;
			bank5=0x5a000;
			bank6=0x58000;
			break;

		case 12:
			bank3=0x66000;
			bank4=0x64000;
			bank5=0x62000;
			bank6=0x60000;
			break;

		case 13:
			bank3=0x6e000;
			bank4=0x6c000;
			bank5=0x6a000;
			bank6=0x68000;
			break;

		case 14:
			bank3=0x76000;
			bank4=0x74000;
			bank5=0x72000;
			bank6=0x70000;
			break;

		case 15:
			bank3=0x7e000;
			bank4=0x7c000;
			bank5=0x7a000;
			bank6=0x78000;
			break;

	}

	// bankswitcherooney type thing (tm)

	romoffset = memory_region(REGION_CPU1) + 0x10000 + bank1;
	memory_set_bankptr(1, romoffset);
	romoffset = memory_region(REGION_CPU1) + 0x10000 + bank2;
	memory_set_bankptr(2, romoffset);

	memory_set_bankptr(3, mess_ram + bank3);
	memory_set_bankptr(11, mess_ram + bank3);

	memory_set_bankptr(4, mess_ram + bank4);
	memory_set_bankptr(12, mess_ram + bank4);

	memory_set_bankptr(5, mess_ram + bank5);
	memory_set_bankptr(13, mess_ram + bank5);

	memory_set_bankptr(6, mess_ram + bank6);
	memory_set_bankptr(14, mess_ram + bank6);

}

static unsigned char mtx_peek(int address)
{
	int base_address = 0;
	unsigned char rtn = 0;
	int offset = address & 0x1fff;

	switch(rampage)
	{
		case 0:
			base_address=0;
			break;

		case 1:
			base_address=0x8000;
			break;

		case 2:
			base_address=0x10000;
			break;

		case 3:
			base_address=0x18000;
			break;

		case 4:
			base_address=0x20000;
			break;

		case 5:
			base_address=0x28000;
			break;

		case 6:
			base_address=0x30000;
			break;

		case 7:
			base_address=0x38000;
			break;

		case 8:
			base_address=0x40000;
			break;

		case 9:
			base_address=0x48000;
			break;

		case 10:
			base_address=0x50000;
			break;

		case 11:
			base_address=0x58000;
			break;

		case 12:
			base_address=0x60000;
			break;

		case 13:
			base_address=0x68000;
			break;

		case 14:
			base_address=0x70000;
			break;

		case 15:
			base_address=0x78000;
			break;

	}

	if(address>=0x4000 && address<=0x5fff) rtn = mess_ram[base_address + 0x6000 + offset];
	if(address>=0x6000 && address<=0x7fff) rtn = mess_ram[base_address + 0x4000 + offset];
	if(address>=0x8000 && address<=0x9fff) rtn = mess_ram[base_address + 0x2000 + offset];
	if(address>=0xa000 && address<=0xbfff) rtn = mess_ram[base_address + offset];
	if(address>=0xc000 && address<=0xffff) rtn = mtx_commonram[address - 0xc000];
return(rtn);

}

static void mtx_poke(int address, unsigned char data)
{
	int base_address = 0;
	int offset = address & 0x1fff;

	switch(rampage)
	{
		case 0:
			base_address=0;
			break;

		case 1:
			base_address=0x8000;
			break;

		case 2:
			base_address=0x10000;
			break;

		case 3:
			base_address=0x18000;
			break;

		case 4:
			base_address=0x20000;
			break;

		case 5:
			base_address=0x28000;
			break;

		case 6:
			base_address=0x30000;
			break;

		case 7:
			base_address=0x38000;
			break;

		case 8:
			base_address=0x40000;
			break;

		case 9:
			base_address=0x48000;
			break;

		case 10:
			base_address=0x50000;
			break;

		case 11:
			base_address=0x58000;
			break;

		case 12:
			base_address=0x60000;
			break;

		case 13:
			base_address=0x68000;
			break;

		case 14:
			base_address=0x70000;
			break;

		case 15:
			base_address=0x78000;
			break;

	}

	if(address>=0x4000 && address<=0x5fff) mess_ram[base_address + 0x6000 + offset] = data;
	if(address>=0x6000 && address<=0x7fff) mess_ram[base_address + 0x4000 + offset] = data;
	if(address>=0x8000 && address<=0x9fff) mess_ram[base_address + 0x2000 + offset] = data;
	if(address>=0xa000 && address<=0xbfff) mess_ram[base_address + offset] = data;
	if(address>=0xc000 && address<=0xffff) mtx_commonram[address - 0xc000] = data;

}

/*
 * A filename is at most 14 characters long, always ending with a space.
 * The save file can be at most 65536 long.
 * Note: the empty string is saved as a single space.
 */
static void mtx_save_hack(int start, int length)
{
	int i;
	int saved = 0;
	mame_file *f;
	static char filename[16] = "";

//	logerror("mtx_save_hack: start=%#x  length=%#x (%d)  index=%#x (%d)\n",
//			start, length, length, mtx_saveindex, mtx_saveindex);

	if ((start > 0xc000) && (length == 20))
	{
		for (i = 0; i < 18; i++)
			mtx_savebuffer[i] = mtx_peek(start + i);

		mtx_saveindex = 18;

		memcpy(filename, mtx_savebuffer + 1, 15);

		for (i = 14; i > 0 && filename[i] == 0x20; i--)
			;

		filename[i + 1] = '\0';
	}
	else
	{
		if (mtx_saveindex + length > 65536)
			length = 65536 - mtx_saveindex;

		for (i = 0; i < length; i++)
			mtx_savebuffer[mtx_saveindex + i] = mtx_peek(start + i);

		mtx_saveindex += length;
	}

	if (start == 0xc000)
	{
		logerror("saving buffer into '%s', ", filename);

		if ((f = mame_fopen(Machine->gamedrv->name, filename,
						FILETYPE_IMAGE, 1)) != 0)
		{
			saved = mame_fwrite(f, mtx_savebuffer, mtx_saveindex);
			mame_fclose(f);
		}

		logerror("saved %d bytes\n", saved);
		filename[0] = '\0';
	}
}

/*
 * A filename is at most 14 characters long, ending with a space.
 * At most 65536 bytes are loaded from the file.
 * Note: the empty string is loaded as a single space (ugly),
 *       not the 'next' file.
 */
static void mtx_load_hack(int start, int length)
{
	int i;
	int filesize;
	mame_file *f;
	static char filename[16] = "";

//	logerror("mtx_load_hack: start=%#x  length=%#x (%d)  size=%#x (%d)\n",
//			start, length, length, mtx_loadsize, mtx_loadsize);

	if ((start > 0xc000) && (length == 18) && (mtx_loadsize <= 0))
	{
		for (i = 0; i < 15; i++)
			filename[i] = mtx_peek(start - 0xf + i);

		for (i = 14; i > 0 && filename[i] == 0x20; i--)
			;

		filename[i+1] = '\0';
		logerror("loading '%s' into buffer, ", filename);
		if ((f = mame_fopen(Machine->gamedrv->name, filename,
						FILETYPE_IMAGE, 0)) != 0)
		{
			filesize = mame_fsize(f);
			if (filesize > 65536)
				filesize = 65536;

			mtx_loadsize = mame_fread(f, mtx_loadbuffer, filesize);
			mame_fclose(f);
		}

		logerror("loaded %d bytes\n", mtx_loadsize);
	}

	if (mtx_loadsize > 0)
	{
		if (length > mtx_loadsize)
		{
			logerror("file '%s' is too short\n", filename);
			length = mtx_loadsize;
		}

		for (i = 0; i < length; i++)
			mtx_poke(start + i, mtx_loadbuffer[i]);

		memcpy(mtx_loadbuffer, mtx_loadbuffer + length,
				mtx_loadsize - length);
		mtx_loadsize -= length;
	}
}

static void mtx_verify_hack(int start, int length)
{
	// logerror("mtx_verify_hack:  start=0x%x  length=0x%x (%d)  not implemented\n", start, length, length);
}

static WRITE8_HANDLER ( mtx_trap_write )
{
	int pc;
	int start;
	int length;

	pc = activecpu_get_reg(Z80_PC);
	if((offset == 0x0aae) && (pc == 0x0ab1))
	{
		start = activecpu_get_reg(Z80_HL);
		length = activecpu_get_reg(Z80_DE);

		// logerror("PC %04x\nStart %04x, Length %04x, 0xFD67 %02x, 0xFD68 %02x index 0x%04x\n", pc, start, length, mess_ram[0xfd67], mess_ram[0xfd68], mtx_loadsize);

		if(mtx_peek(0xfd68) == 0)
			mtx_save_hack(start, length);
		else if(mtx_peek(0xfd67) == 0)
			mtx_load_hack(start, length);
		else
			mtx_verify_hack(start, length);
	}
}

static void mtx_printer_getinfo(struct IODevice *dev)
{
	printer_device_getinfo (dev);
	dev->count = 1;
}

static MACHINE_INIT( mtx512 )
{
	unsigned char *romoffset;

	mtx_commonram = (unsigned char *)auto_malloc(16384);
	if(!mtx_commonram)
		return;
	memset(mtx_commonram, 0, 16384);

	mtx_loadbuffer = (unsigned char *)auto_malloc(65536);
	if(!mtx_loadbuffer)
		return;
	memset(mtx_loadbuffer, 0, 65536);

	mtx_savebuffer = (unsigned char *)auto_malloc(65536);
	if(!mtx_savebuffer)
		return;
	memset(mtx_savebuffer, 0, 65536);

	z80ctc_init(&mtx_ctc_intf);

	// set up memory configuration

	romoffset = memory_region(REGION_CPU1) + 0x10000;

	//Patch the Rom (Sneaky........ Who needs to trap opcodes?)
	romoffset[0x0aae] = 0x32;	// ld ($0aae),a
	romoffset[0x0aaf] = 0xae;
	romoffset[0x0ab0] = 0x0a;
	romoffset[0x0ab1] = 0xc9;	// ret
	memory_set_bankptr(1, romoffset);
	romoffset = memory_region(REGION_CPU1) + 0x12000;
	memory_set_bankptr(2, romoffset);

	memory_set_bankptr(3, mess_ram + 0x6000);
	memory_set_bankptr(11, mess_ram + 0x6000);

	memory_set_bankptr(4, mess_ram + 0x4000);
	memory_set_bankptr(12, mess_ram + 0x4000);

	memory_set_bankptr(5, mess_ram + 0x2000);
	memory_set_bankptr(13, mess_ram + 0x2000);

	memory_set_bankptr(6, mess_ram);
	memory_set_bankptr(14, mess_ram);

	memory_set_bankptr(7, mtx_commonram);
	memory_set_bankptr(15, mtx_commonram);

	memory_set_bankptr(8, mtx_commonram + 0x2000);
	memory_set_bankptr(16, mtx_commonram + 0x2000);

	mtx_loadsize = 0;
	mtx_saveindex = 0;
}

static INTERRUPT_GEN( mtx_interrupt )
{
	TMS9928A_interrupt();
}

ADDRESS_MAP_START( mtx_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_READWRITE( MRA8_BANK1, mtx_trap_write )
	AM_RANGE( 0x2000, 0x3fff) AM_READWRITE( MRA8_BANK2, MWA8_NOP )
	AM_RANGE( 0x4000, 0x5fff) AM_READWRITE( MRA8_BANK3, MWA8_BANK11 )
	AM_RANGE( 0x6000, 0x7fff) AM_READWRITE( MRA8_BANK4, MWA8_BANK12 )
	AM_RANGE( 0x8000, 0x9fff) AM_READWRITE( MRA8_BANK5, MWA8_BANK13 )
	AM_RANGE( 0xa000, 0xbfff) AM_READWRITE( MRA8_BANK6, MWA8_BANK14 )
	AM_RANGE( 0xc000, 0xdfff) AM_READWRITE( MRA8_BANK7, MWA8_BANK15 )
	AM_RANGE( 0xe000, 0xffff) AM_READWRITE( MRA8_BANK8, MWA8_BANK16 )
ADDRESS_MAP_END

ADDRESS_MAP_START( mtx_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE( 0x00, 0x00) AM_READWRITE( mtx_strobe_r, mtx_bankswitch_w )
	AM_RANGE( 0x01, 0x02) AM_READWRITE( mtx_vdp_r, mtx_vdp_w )
	AM_RANGE( 0x03, 0x03) AM_READWRITE( mtx_psg_r, mtx_cst_w )
	AM_RANGE( 0x04, 0x04) AM_READWRITE( mtx_prt_r, mtx_prt_w )
	AM_RANGE( 0x05, 0x05) AM_READWRITE( mtx_key_lo_r, mtx_sense_w )
	AM_RANGE( 0x06, 0x06) AM_READWRITE( mtx_key_hi_r, mtx_psg_w )
	AM_RANGE( 0x08, 0x0b) AM_READWRITE( mtx_ctc_r, mtx_ctc_w )
ADDRESS_MAP_END

INPUT_PORTS_START( mtx512 )
 PORT_START /* 0 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD7 PAGE )") PORT_CODE(KEYCODE_7_PAD)

 PORT_START /* 1 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESCAPE") PORT_CODE(KEYCODE_ESC)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 0") PORT_CODE(KEYCODE_0)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD8 EOL") PORT_CODE(KEYCODE_8_PAD)

 PORT_START /* 2 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_CLOSEBRACE)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD5 CURSORUP") PORT_CODE(KEYCODE_5_PAD)

 PORT_START /* 3 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E") PORT_CODE(KEYCODE_E)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_OPENBRACE)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_DOWN)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD1 CURSORLEFT") PORT_CODE(KEYCODE_1_PAD)

 PORT_START /* 4 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALPHALOCK") PORT_CODE(KEYCODE_CAPSLOCK)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_BACKSLASH2)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD3 CURSORRIGHT") PORT_CODE(KEYCODE_3_PAD)

 PORT_START /* 5 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD2 HOME") PORT_CODE(KEYCODE_2_PAD)

 PORT_START /* 6 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFTSHIFT") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X") PORT_CODE(KEYCODE_X)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHTSHIFT") PORT_CODE(KEYCODE_RSHIFT)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD(.) CURSORDOWN") PORT_CODE(KEYCODE_DEL_PAD)

 PORT_START /* 7 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(.) >") PORT_CODE(KEYCODE_STOP)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_ _") PORT_CODE(KEYCODE_MINUS_PAD)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD0 INS") PORT_CODE(KEYCODE_0_PAD)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPADENTER CLS") PORT_CODE(KEYCODE_ENTER_PAD)

/* Hi Bits */

 PORT_START /* 8 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD9 BRK") PORT_CODE(KEYCODE_9_PAD)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD4 TAB") PORT_CODE(KEYCODE_4_PAD)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMERICPAD6 DEL") PORT_CODE(KEYCODE_6_PAD)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)

 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(none)")
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
  PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
  PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)

 PORT_START /* 10 */
  PORT_DIPNAME(0x02, 0x00, "Country Code Switch 1")
  PORT_DIPSETTING(0x02, DEF_STR(Off) )
  PORT_DIPSETTING(0x00, DEF_STR(On) )
  PORT_DIPNAME(0x01, 0x00, "Country Code Switch 0")
  PORT_DIPSETTING(0x01, DEF_STR(Off) )
  PORT_DIPSETTING(0x00, DEF_STR(On) )
  PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static struct z80_irq_daisy_chain mtx_daisy_chain[] =
{
	{z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{0,0,0,0,-1}
};

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	NULL
};

static MACHINE_DRIVER_START( mtx512 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MTX_SYSTEM_CLOCK)
	MDRV_CPU_PROGRAM_MAP(mtx_mem, 0)
	MDRV_CPU_IO_MAP(mtx_io, 0)
	MDRV_CPU_VBLANK_INT(mtx_interrupt, 1)
	MDRV_CPU_CONFIG(mtx_daisy_chain)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( mtx512 )

    /* video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76496, MTX_SYSTEM_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START (mtx512)
	ROM_REGION (0x20000, REGION_CPU1,0)
	ROM_LOAD ("osrom", 0x10000, 0x2000, CRC(9ca858cc) SHA1(3804503a58f0bcdea96bb6488833782ebd03976d))
	ROM_LOAD ("basicrom", 0x12000, 0x2000, CRC(87b4e59c) SHA1(c49782a82a7f068c1195cd967882ba9edd546eaf))
	ROM_LOAD ("assemrom", 0x14000, 0x2000, CRC(9d7538c3) SHA1(d1882c4ea61a68b1715bd634ded5603e18a99c5f))
ROM_END

SYSTEM_CONFIG_START(mtx512)
	CONFIG_RAM_DEFAULT(512 * 1024)
	CONFIG_DEVICE(mtx_printer_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT     CONFIG,  COMPANY          FULLNAME */
COMP( 1983, mtx512,   0,		0,		mtx512,   mtx512,   0,       mtx512,  "Memotech Ltd.", "MTX 512" , 0)
