/******************************************************************************

	Tatung Einstein
	system driver


	TMS9129 VDP Graphics
		16k ram

	Z80 CPU (4Mhz)

	Z80 CTC (4Mhz)
		channel 0 is serial transmit clock
		channel 1 is serial receive clock
		trigger for channel 0,1 and 2 is a 2mhz clock
		trigger for channel 3 is the terminal count of channel 2

	Intel 8251 Serial (2Mhz clock?)
	
	WD1770 Floppy Disc controller
		density is fixed, 4 drives and double sided supported

	AY-3-8910 PSG (2Mhz)
		port A and port B are connected to the keyboard. Port A is keyboard 
		line select, Port B is data.

	printer connected to port A of PIO
	user port is port B of PIO
	keyboard connected to port A and port B of PSG

	NOTES:
	- The ADC is not emulated!
	- printer not emulated

	Many thanks to Chris Coxall for the schematics of the TC-01, the dump of the
	system rom and a dump of a Xtal boot disc.

	Many thanks to Andrew Dunipace for his help with the 80-column card
	and Speculator hardware (Spectrum emulator).

	Kevin Thacker [MESS driver]



 ******************************************************************************/


/* I/O ports */
/* 0x00-0x07 PSG */
/* 0x08-0x0f VDP */
/* 0x10-0x17 SERIAL */
/* 0x18-0x1f FDC */
/* 0x20-0x27 MISC (see below) */
/* 0x28-0x2f CTC */
/* 0x30-0x37 PIO */
/* 0x38-0x3f ADC */

/* MISC */
/* 0x20 - bit 0 is keyboard int mask */
/* 0x21 - bit 0 is adc int mask */
/* 0x22 - alph */
/* 0x23 - drive select and side select */
/* 0x24 - rom */
/* 0x25 - bit 0 is fire int mask */
/* 0x25 - */
/* 0x26 - */
/* 0x27 - */

/* 0x40-0xff for expansion */


/* speculator */
/* 7e = 0, read from fe, 7e = 1, read from fe */

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/tms9928a.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "includes/basicdsk.h"
#include "includes/msm8251.h"
#include "includes/dsk.h"

#define EINSTEIN_SYSTEM_CLOCK 4000000

#define EINSTEIN_KEY_INT	(1<<0)
#define EINSTEIN_ADC_INT		(1<<1)
#define EINSTEIN_FIRE_INT		(1<<2)

static unsigned char *einstein_ram = NULL;

static int einstein_rom_enabled = 1;

static int einstein_int = 0;
static int einstein_int_mask = 0;

static void *einstein_ctc_trigger_timer = NULL;
static int einstein_ctc_trigger = 0;

/* KEYBOARD */
static void *einstein_keyboard_timer = NULL;
static int einstein_keyboard_line = 0;
static int einstein_keyboard_data = 0x0ff;

/**********************************************************/
/* 
	80 column board has a UM6845,2K ram and a char rom:

	0x040-0x047 used to access 2K ram. (bits 3-0 define a 256 byte row, bits 15-8 define 
				the offset in the row)
 	0x048 = crtc register index (w), 
	0x049 = crtc data register (w)

	0x04c 
		bit 2 = 50/60hz mode?
		bit 1 = 1
		bit 0 = vsync state?

	0: 126 (127) horizontal total
	1: 80 horizontal displayed
	2: 97 horizontal sync position
	3: &38
	4: 26 vertical total
	5: 19 vertical adjust
	6: 25 vertical displayed
	7: 26 vertical sync pos
	8: 0 no interlace
	9: 8 (9 scanlines tall)
	10: 32

  total scanlines: ((reg 9+1) * (reg 4+1))+reg 5 = 262
  127 cycles per line
  127*262 = 33274 cycles per frame, vsync len 375 cycles


*/
#include "vidhrdw/m6845.h"
static int einstein_80col_state;
static char *einstein_80col_ram = NULL;

WRITE_HANDLER(einstein_80col_ram_w)
{
	/* lower 3 bits of address define a 256-byte "row".
		upper 8 bits define the offset in the row,
		data bits define data to write */
	einstein_80col_ram[((offset & 0x07)<<8)|((offset>>8) & 0x0ff)] = data;
}

READ_HANDLER(einstein_80col_ram_r)
{
	return einstein_80col_ram[((offset & 0x07)<<8)|((offset>>8) & 0x0ff)];
}

READ_HANDLER(einstein_80col_state_r)
{
	/* fake vsync for now */
	einstein_80col_state^=0x01;
	return einstein_80col_state;
}

static int Einstein_6845_RA = 0;
static int Einstein_scr_x = 0;
static int Einstein_scr_y = 0;
static int Einstein_HSync = 0;
static int Einstein_VSync = 0;
static int Einstein_DE = 0;

// called when the 6845 changes the character row
void Einstein_Set_RA(int offset, int data)
{
	Einstein_6845_RA=data;
}


// called when the 6845 changes the HSync
void Einstein_Set_HSync(int offset, int data)
{
	Einstein_HSync=data;
	if(!Einstein_HSync)
	{
		Einstein_scr_y++;
		Einstein_scr_x = -40;
	}
}

// called when the 6845 changes the VSync
void Einstein_Set_VSync(int offset, int data)
{
	Einstein_VSync=data;
	if (!Einstein_VSync)
	{
		Einstein_scr_y = 0;
	}
}

void Einstein_Set_DE(int offset, int data)
{
	Einstein_DE = data;
}


static struct crtc6845_interface
einstein_crtc6845_interface= {
	0,// Memory Address register
	Einstein_Set_RA,// Row Address register
	Einstein_Set_HSync,// Horizontal status
	Einstein_Set_VSync,// Vertical status
	Einstein_Set_DE,// Display Enabled status
	0,// Cursor status
};

/* 80 column card init */
void	einstein_80col_init(void)
{
	/* 2K RAM */
	einstein_80col_ram = malloc(2048);

	/* initialise 6845 */
	crtc6845_config(&einstein_crtc6845_interface);

	einstein_80col_state=(1<<2)|(1<<1);
}

/* 80 column card exit */
void	einstein_80col_exit(void)
{
	if (einstein_80col_ram!=NULL)
	{
		free(einstein_80col_ram);
		einstein_80col_ram = NULL;
	}
}

READ_HANDLER(einstein_80col_r)
{
	switch (offset & 0x0f)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			return einstein_80col_ram_r(offset);
		case 0x0c:
			return einstein_80col_state_r(offset);
		default:
			break;
	}
	return 0x0ff;
}


WRITE_HANDLER(einstein_80col_w)
{
	switch (offset & 0x0f)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			einstein_80col_ram_w(offset,data);
			break;
		case 8:
			crtc6845_address_w(offset,data);
			break;
		case 9:
			crtc6845_register_w(offset,data);
			break;
		default:
			break;
	}
}

void einstein_ctc_trigger_callback(int dummy)
{
	einstein_ctc_trigger^=1;

	/* channel 0 and 1 have a 2Mhz input clock for triggering */
	z80ctc_0_trg0_w(0,einstein_ctc_trigger);
	z80ctc_0_trg1_w(0,einstein_ctc_trigger);
}

/* refresh keyboard data. It is refreshed when the keyboard line is written */
void einstein_scan_keyboard(void)
{
	unsigned char data = 0x0ff;
	int i;

	for (i=0; i<8; i++)
	{
		if ((einstein_keyboard_line & (1<<i))==0)
		{
			data &= readinputport(i);
		}
	}

	einstein_keyboard_data = data;
}

static void einstein_update_interrupts(void)
{




}

void	einstein_keyboard_timer_callback(int dummy)
{
	einstein_scan_keyboard();

	if ((readinputport(9) & 0x03)!=0)
	{
		einstein_int |= EINSTEIN_FIRE_INT;
	}
	else
	{
		einstein_int &= ~EINSTEIN_FIRE_INT;
	}

	/* keyboard data changed? */
	if (einstein_keyboard_data!=0x0ff)
	{
		/* generate interrupt */
		einstein_int |= EINSTEIN_KEY_INT;
		
		einstein_update_interrupts();

//		if (einstein_int & einstein_int_mask)
//		{
//			cpu_cause_interrupt(0, 0x0ff);
//		}
	}
}



int einstein_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY, id)==NULL)
		return INIT_PASS;

	return dsk_floppy_load(id);
}

static void einstein_z80fmly_interrupt(int state)
{
	cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

WRITE_HANDLER(einstein_serial_transmit_clock)
{
	msm8251_transmit_clock();
}

WRITE_HANDLER(einstein_serial_receive_clock)
{
	msm8251_receive_clock();
}

WRITE_HANDLER(einstein_ctc_ch2_zc0)
{
	/* the terminal count of channel 2 is connected to the trigger for channel 3 */
	z80ctc_0_trg3_w(0,data);
}


static z80ctc_interface	einstein_ctc_intf =
{
	1,
	{EINSTEIN_SYSTEM_CLOCK},
	{0},
	{einstein_z80fmly_interrupt},
	{einstein_serial_transmit_clock},
	{einstein_serial_receive_clock},
    {einstein_ctc_ch2_zc0}
};


static z80pio_interface einstein_pio_intf = 
{
	1,
	{einstein_z80fmly_interrupt},
	{0},
	{NULL}
};

#if 0
void einstein_int_reset(int code)
{
}

int	einstein_interrupt(int code)
{

}
#endif

static Z80_DaisyChain einstein_daisy_chain[] =
{
    {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
	{z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
//	{einstein_int_reset, einstein_interrupt, NULL, 0},
    {0,0,0,-1}
};

int einstein_vh_init(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);
}

static READ_HANDLER(einstein_vdp_r)
{
	/*logerror("vdp r: %04x\n",offset); */

	if (offset & 0x01)
	{
		return TMS9928A_register_r(offset & 0x01);
	}

	return TMS9928A_vram_r(offset & 0x01);
}

static WRITE_HANDLER(einstein_vdp_w)
{
	/* logerror("vdp w: %04x %02x\n",offset,data); */

	if (offset & 0x01)
	{
		TMS9928A_register_w(offset & 0x01,data);
		return;
	}

	TMS9928A_vram_w(offset & 0x01,data);
}

static WRITE_HANDLER(einstein_fdc_w)
{
	int reg = offset & 0x03;

	logerror("fdc w: PC: %04x %04x %02x\n",cpu_get_pc(),offset,data); 

	switch (reg)
	{
		case 0:
		{
			wd179x_command_w(reg, data);
		}
		break;
	
		case 1:
		{
			wd179x_track_w(reg, data);
		}
		break;
	
		case 2:
		{
			wd179x_sector_w(reg, data);
		}
		break;

		case 3:
		{
			wd179x_data_w(reg, data);
		}
		break;
	}
}


static READ_HANDLER(einstein_fdc_r)
{
	int reg = offset & 0x03;

	logerror("fdc r: PC: %04x %04x\n",cpu_get_pc(),offset); 

	switch (reg)
	{
		case 0:
		{
			return wd179x_status_r(reg);
		}
		break;
	
		case 1:
		{
			return wd179x_track_r(reg);
		}
		break;
	
		case 2:
		{
			return wd179x_sector_r(reg);
		}
		break;

		case 3:
		{
			return wd179x_data_r(reg);
		}
		break;
	}

	return 0x0ff;
}

static WRITE_HANDLER(einstein_pio_w)
{
	logerror("pio w: %04x %02x\n",offset,data);

	if ((offset & 0x01)==0)
	{
		z80pio_d_w( 0, (offset>>1) & 0x01,data);
		return;
	}

	z80pio_c_w( 0, (offset>>1) & 0x01,data);
}

static READ_HANDLER(einstein_pio_r)
{
	logerror("pio r: %04x\n",offset);

	if ((offset & 0x01)==0)
	{
		return z80pio_d_r( 0, (offset>>1) & 0x01);
	}

	return z80pio_c_r( 0, (offset>>1) & 0x01);
}

static READ_HANDLER(einstein_ctc_r)
{
	logerror("ctc r: %04x\n",offset); 
	
	return z80ctc_0_r(offset & 0x03);
}

static WRITE_HANDLER(einstein_ctc_w)
{
	logerror("ctc w: %04x %02x\n",offset,data); 

	z80ctc_0_w(offset & 0x03,data);
}

static WRITE_HANDLER(einstein_serial_w)
{
	int reg = offset & 0x01;

	/* logerror("serial w: %04x %02x\n",offset,data); */

	if ((reg)==0)
	{
		msm8251_data_w(reg,data);
		return;
	}
	
	msm8251_control_w(reg,data);
}


static READ_HANDLER(einstein_serial_r)
{
	int reg = offset & 0x01;

	/* logerror("serial r: %04x\n",offset); */

	if ((reg)==0)
	{
		return msm8251_data_r(reg);
	}
	
	return msm8251_status_r(reg);
}

/* 

	AY-3-8912 PSG:

	NOTE: BC2 is connected to +5v

	BDIR	BC1		FUNCTION
	0		0		INACTIVE
	0		1		READ FROM PSG
	1		0		WRITE TO PSG
	1		1		LATCH ADDRESS

	/PSG select, /WR connected to NOR -> BDIR

	when /PSG=0 and /WR=0, BDIR is 1. (write to psg or latch address)

	/PSG select, A0 connected to NOR -> BC1
	when A0 = 0, BC1 is 1. when A0 = 1, BC1 is 0.

	when /PSG = 1, BDIR is 0 and BC1 is 0.


*/


static WRITE_HANDLER(einstein_psg_w)
{
	int reg = offset & 0x03;

	/*logerror("psg w: %04x %02x\n",offset,data); */

	switch (reg)
	{
		/* case 0 and 1 are not handled */
		case 2:
		{
			AY8910_control_port_0_w(0, data);
		}
		break;

		case 3:
		{
			AY8910_write_port_0_w(0, data);
		}
		break;

		default:
			break;
	}
}

static READ_HANDLER(einstein_psg_r)
{
	int reg = offset & 0x03;

	switch (reg)
	{
		/* case 0 and 1 are not handled */
		case 2:
			return AY8910_read_port_0_r(0);
	
		default:
			break;
	}

	return 0x0ff;
}
	

/* int priority */
/* keyboard int->ctc/adc->pio */


MEMORY_READ_START( readmem_einstein )
	{0x0000, 0x01fff, MRA_BANK1},
	{0x2000, 0x0ffff, MRA_BANK2},
MEMORY_END



MEMORY_WRITE_START( writemem_einstein )
	{0x0000, 0x0ffff, MWA_BANK3},
MEMORY_END

static void einstein_page_rom(void)
{
	if (einstein_rom_enabled)
	{
		cpu_setbank(1, memory_region(REGION_CPU1)+0x010000);
	}
	else
	{
		cpu_setbank(1, einstein_ram);
	}
}

static WRITE_HANDLER(einstein_drive_w)
{
	/* bit 4: side select */
	/* bit 3: select drive 3 */
	/* bit 2: select drive 2 */
	/* bit 1: select drive 1 */
	/* bit 0: select drive 0 */

	logerror("drive w: PC: %04x %04x %02x\n",cpu_get_pc(),offset,data); 

	wd179x_set_side((data>>4) & 0x01);
	
	if (data & 0x01)
	{
		wd179x_set_drive(0);
	}
	else
	if (data & 0x02)
	{
		wd179x_set_drive(1);
	}
	else
	if (data & 0x04)
	{
		wd179x_set_drive(2);
	}
	else
	if (data & 0x08)
	{
		wd179x_set_drive(3);
	}
}


static WRITE_HANDLER(einstein_unmapped_w)
{
}

static READ_HANDLER(einstein_unmapped_r)
{
	return 0x0ff;
}


static WRITE_HANDLER(einstein_rom_w)
{
	einstein_rom_enabled^=1;
	einstein_page_rom();
}

static READ_HANDLER(einstein_key_int_r)
{
	logerror("mask r\n");

	/* clear key int */
	einstein_int &= ~EINSTEIN_KEY_INT;

	/* bit 7: 0=shift pressed */
	/* bit 6: 0=control pressed */
	/* bit 5: 0=graph pressed */
	/* bit 4: 0=error from printer? */
	/* bit 3: pe? */
	/* bit 2: 1=printer busy */
	/* bit 1: fire 1 */
	/* bit 0: fire 0 */
	return ((readinputport(8) & 0x07)<<5) | (readinputport(9) & 0x03) | 0x01c;
}


static WRITE_HANDLER(einstein_key_int_w)
{
	logerror("mask: %02x\n",data);

	/* set mask from bit 0 */
	if (data & 0x01)
	{
		einstein_int_mask |= EINSTEIN_KEY_INT;
	}
	else
	{
		einstein_int_mask &= ~EINSTEIN_KEY_INT;
	}
}

READ_HANDLER(einstein_port_r)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			return einstein_psg_r(offset);
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return einstein_vdp_r(offset);
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			return einstein_serial_r(offset);
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			return einstein_fdc_r(offset);
		case 0x20:
			return einstein_key_int_r(offset);
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			return einstein_ctc_r(offset);
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			return einstein_pio_r(offset);
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
			return einstein_80col_r(offset);

		default:
			break;
	}
	return 0x00;
}

WRITE_HANDLER(einstein_port_w)
{
	switch (offset & 0x0ff)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			einstein_psg_w(offset,data);
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			einstein_vdp_w(offset,data);
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			einstein_serial_w(offset,data);
			break;
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			einstein_fdc_w(offset,data);
			break;
		case 0x20:
			einstein_key_int_w(offset,data);
			break;
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			einstein_ctc_w(offset,data);
			break;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			einstein_pio_w(offset,data);
			break;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
			einstein_80col_w(offset,data);
			break;

		default:
			break;
	}
}

PORT_READ_START( readport_einstein )
	{0x0000,0x0ffff, einstein_port_r},
PORT_END

PORT_WRITE_START( writeport_einstein )
	{0x0000,0x0ffff, einstein_port_w},
PORT_END

#if 0
PORT_READ_START( readport_einstein )
	{0x000, 0x007, einstein_psg_r},
	{0x008, 0x00f, einstein_vdp_r},
	{0x010, 0x017, einstein_serial_r},
	{0x018, 0x01f, einstein_fdc_r},
	{0x020, 0x020, einstein_key_int_r},	
	{0x028, 0x02f, einstein_ctc_r},
	{0x030, 0x037, einstein_pio_r},
//	{0x040, 0x0ff, einstein_unmapped_r},
PORT_END

PORT_WRITE_START( writeport_einstein )
	{0x000, 0x007, einstein_psg_w},
	{0x008, 0x00f, einstein_vdp_w},
	{0x010, 0x017, einstein_serial_w},
	{0x018, 0x01f, einstein_fdc_w},
	{0x020, 0x020, einstein_key_int_w},
	{0x023, 0x023, einstein_drive_w},
	{0x024, 0x024, einstein_rom_w},
	{0x028, 0x02f, einstein_ctc_w},
	{0x030, 0x037, einstein_pio_w},
//	{0x040, 0x0ff, einstein_unmapped_w},
PORT_END
#endif



//#define EINSTEIN_DUMP_RAM

#ifdef EINSTEIN_DUMP_RAM
void einstein_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "einstein.bin", OSD_FILETYPE_NVRAM,OSD_FOPEN_WRITE);
 
	if (file)
	{
		osd_fwrite(file, einstein_ram, 0x10000);

		/* close file */
		osd_fclose(file);
	}
}
#endif

void einstein_init_machine(void)
{
	einstein_ram = malloc(65536);
	memset(einstein_ram, 0x0aa, 65536);
	cpu_setbank(2,einstein_ram+0x02000);
	cpu_setbank(3,einstein_ram);

	z80ctc_init(&einstein_ctc_intf);
	z80pio_init(&einstein_pio_intf);

	z80ctc_reset(0);
	z80pio_reset(0);

	TMS9928A_reset ();

	einstein_rom_enabled = 1;
	einstein_page_rom();

	wd179x_init(WD_TYPE_177X,NULL);

	einstein_int=0;

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_SS_40);


	/* the einstein keyboard can generate a interrupt */
	/* the int is actually clocked at the system clock 4Mhz, but this would be too fast for our
	driver. So we update at 50Hz and hope this is good enough. */
	einstein_keyboard_timer = timer_pulse(TIME_IN_HZ(50), 0, einstein_keyboard_timer_callback);

	/* the input to channel 0 and 1 of the ctc is a 2mhz clock */
	einstein_ctc_trigger = 0;
	einstein_ctc_trigger_timer = timer_pulse(TIME_IN_HZ(1000000), 0, einstein_ctc_trigger_callback);
}

void einstein2_init_machine(void)
{
	einstein_init_machine();
	
	einstein_80col_init();
}


void einstein_shutdown_machine(void)
{
#ifdef EINSTEIN_DUMP_RAM
	einstein_dump_ram();
#endif
	if (einstein_ram)
	{
		free(einstein_ram);
		einstein_ram = NULL;
	}

	if (einstein_ctc_trigger_timer)
	{
		timer_remove(einstein_ctc_trigger_timer);
		einstein_ctc_trigger_timer = NULL;
	}

	if (einstein_keyboard_timer)
	{
		timer_remove(einstein_keyboard_timer);
		einstein_keyboard_timer = NULL;
	}
	
	wd179x_exit();
}

void einstein2_shutdown_machine(void)
{
	einstein_shutdown_machine();

	einstein_80col_exit();
}

INPUT_PORTS_START(einstein)
	/* line 0 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_LALT, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", KEYCODE_1_PAD, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0", KEYCODE_F1, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", KEYCODE_3_PAD, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC", KEYCODE_ESC, IP_JOY_NONE) 
	/* line 1 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", KEYCODE_MINUS, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "||", KEYCODE_BACKSLASH, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE) 
	/* line 2 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5", KEYCODE_F6, IP_JOY_NONE) 
	/* line 3 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "DELETE", KEYCODE_BACKSPACE, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4", KEYCODE_F5, IP_JOY_NONE) 
	/* line 4 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3", KEYCODE_F4, IP_JOY_NONE) 
	/* line 5 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_F3, IP_JOY_NONE) 
	/* line 6 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F2, IP_JOY_NONE) 
	/* line 7 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE) 
	PORT_BITX(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6", KEYCODE_F7, IP_JOY_NONE) 
	/* extra */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRPH", KEYCODE_F1, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	
	/* fire buttons for analogue joysticks */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1,	"Joystick 1 Button 1", CODE_DEFAULT, CODE_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1|IPF_PLAYER2,	"Joystick 2 Button 1", CODE_DEFAULT, CODE_NONE)

	/* analog joystick 1 x axis */
	PORT_START
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)

	/* analog joystick 1 y axis */
	PORT_START 
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_1_UP,JOYCODE_1_DOWN)

	/* analog joystick 2 x axis */
	PORT_START
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)

	/* analog joystick 2 Y axis */
	PORT_START
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_UP,JOYCODE_2_DOWN)

INPUT_PORTS_END

static WRITE_HANDLER(einstein_port_a_write)
{
	einstein_keyboard_line = data;

	logerror("line: %02x\n",einstein_keyboard_line);

	/* re-scan the keyboard */
	einstein_scan_keyboard();
}

static READ_HANDLER(einstein_port_b_read)
{
	einstein_scan_keyboard();

	logerror("key: %02x\n",einstein_keyboard_data);

	return einstein_keyboard_data;	
}

static struct AY8910interface einstein_ay_interface =
{
	1,								   /* 1 chips */
	2000000,						   /* 2.0 MHz  */
	{25, 25},
	{NULL},
	{einstein_port_b_read},
	{einstein_port_a_write},
	{NULL}
};

/*
	0: 126 (127) horizontal total
	1: 80 horizontal displayed
	2: 97 horizontal sync position
	3: &38
	4: 26 vertical total
	5: 19 vertical adjust
	6: 25 vertical displayed
	7: 26 vertical sync pos
	8: 0 no interlace
	9: 8 (9 scanlines tall)
	10: 32

  total scanlines: ((reg 9+1) * (reg 4+1))+reg 5 = 262
  127 cycles per line
  127*262 = 33274 cycles per frame, vsync len 375 cycles
*/

void einstein_80col_plot_char_line(int x,int y, struct osd_bitmap *bitmap)
{
	if (Einstein_DE)
	{

		unsigned char *data = memory_region(REGION_CPU1)+0x012000;
		int w;
		unsigned char data_byte;
		int char_code;

		char_code = einstein_80col_ram[crtc6845_memory_address_r(0)&0x07ff];
		
		data_byte = data[(char_code<<3) + Einstein_6845_RA];

		for (w=0; w<8;w++)
		{
			if (data_byte & 0x080)
			{
				plot_pixel(bitmap, x+w, y,1);
			}
			else
			{
				plot_pixel(bitmap, x+w, y,0);
			}

			data_byte = data_byte<<1;

		}
	}
	else
	{
		plot_pixel(bitmap, x+0, y, 0);
		plot_pixel(bitmap, x+1, y, 0);
		plot_pixel(bitmap, x+2, y, 0);
		plot_pixel(bitmap, x+3, y, 0);
		plot_pixel(bitmap, x+4, y, 0);
		plot_pixel(bitmap, x+5, y, 0);
		plot_pixel(bitmap, x+6, y, 0);
		plot_pixel(bitmap, x+7, y, 0);
	}

}

void einstein_80col_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	long c=0; // this is used to time out the screen redraw, in the case that the 6845 is in some way out state.

	c=0;

	// loop until the end of the Vertical Sync pulse
	while((Einstein_VSync)&&(c<33274))
	{
		// Clock the 6845
		crtc6845_clock();
		c++;
	}

	// loop until the Vertical Sync pulse goes high
	// or until a timeout (this catches the 6845 with silly register values that would not give a VSYNC signal)
	while((!Einstein_VSync)&&(c<33274))
	{
		while ((Einstein_HSync)&&(c<33274))
		{
			crtc6845_clock();
			c++;
		}
		// Do all the clever split mode changes in here before the next while loop

		while ((!Einstein_HSync)&&(c<33274))
		{
			// check that we are on the emulated screen area.
			if ((Einstein_scr_x>=0) && (Einstein_scr_x<640) && (Einstein_scr_y>=0) && (Einstein_scr_y<400))
			{
				einstein_80col_plot_char_line(Einstein_scr_x, Einstein_scr_y, bitmap);
			}

			Einstein_scr_x+=8;

			// Clock the 6845
			crtc6845_clock();
			c++;
		}
	}
}

void einstein2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	TMS9928A_refresh(bitmap,full_refresh);
	einstein_80col_refresh(bitmap,full_refresh);
}

void einstein_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	TMS9928A_refresh(bitmap,full_refresh);
}



static struct MachineDriver machine_driver_einstein =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80 | CPU_16BIT_PORT,  /* type */
			EINSTEIN_SYSTEM_CLOCK,
			readmem_einstein,		   /* MemoryReadAddress */
			writemem_einstein,		   /* MemoryWriteAddress */
			readport_einstein,		   /* IOReadPort */
			writeport_einstein,		   /* IOWritePort */
            0, 0,
			0, 0,	
			einstein_daisy_chain
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	einstein_init_machine,			   /* init machine */
	einstein_shutdown_machine,
	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	einstein_vh_init,
	TMS9928A_stop,
	einstein_vh_screenrefresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&einstein_ay_interface
		},
	}
};

static struct MachineDriver machine_driver_einstein2 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80 | CPU_16BIT_PORT,  /* type */
			EINSTEIN_SYSTEM_CLOCK,
			readmem_einstein,		   /* MemoryReadAddress */
			writemem_einstein,		   /* MemoryWriteAddress */
			readport_einstein,		   /* IOReadPort */
			writeport_einstein,		   /* IOWritePort */
            0, 0,
			0, 0,	
			einstein_daisy_chain
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	einstein_init_machine,			   /* init machine */
	einstein_shutdown_machine,
	/* video hardware */
//	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	640,400, {0,640-1, 0, 400-1},
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	einstein_vh_init,
	TMS9928A_stop,
	einstein_vh_screenrefresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&einstein_ay_interface
		},
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(einstein)
	ROM_REGION(0x010000+0x02000+0x0800, REGION_CPU1,0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, 0x0ec134953)
ROM_END

ROM_START(einstein2)
	ROM_REGION(0x010000+0x02000+0x0800, REGION_CPU1,0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, 0x0ec134953)
	ROM_LOAD("charrom.rom",0x012000, 0x0800, 0x0)
ROM_END

static const struct IODevice io_einstein[] =
{
	{ 
		IO_FLOPPY,					/* type */ 
		4,							/* count */ 
		"dsk\0",                    /* file extensions */ 
		IO_RESET_NONE,				/* reset if file changed */ 
		0,
		einstein_floppy_init,			/* init */ 
		dsk_floppy_exit,			/* exit */ 
		NULL,						/* info */ 
		NULL,						/* open */ 
		NULL,						/* close */ 
		floppy_status,              /* status */ 
		NULL,                       /* seek */ 
		NULL,						/* tell */ 
		NULL,						/* input */ 
		NULL,						/* output */ 
		NULL,						/* input_chunk */ 
		NULL						/* output_chunk */ 
	},
	{IO_END}
};

#define io_einstein2 io_einstein

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 19??, einstein,      0,            einstein,          einstein,      0,       "Tatung", "Tatung Einstein TC-01")
COMP( 19??, einstein2,      0,            einstein2,          einstein,      0,       "Tatung", "Tatung Einstein TC-01 + 80 column device + Speculator")

