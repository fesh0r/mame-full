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
	- 2mhz clock to CTC not emulated
	- trigger for channel 3 not emulated

	Many thanks to Chris Coxall for the schematics of the TC-01, the dump of the
	system rom and a dump of a Xtal boot disc.

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

/* KEYBOARD */
static void *einstein_keyboard_timer = NULL;
static int einstein_keyboard_line = 0;
static int einstein_keyboard_data = 0x0ff;

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


void	einstein_keyboard_timer_callback(int dummy)
{
	/* keyboard data changed? */
	if (einstein_keyboard_data!=0x0ff)
	{
		/* generate interrupt */


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


static z80ctc_interface	einstein_ctc_intf =
{
	1,
	{EINSTEIN_SYSTEM_CLOCK},
	{0},
	{einstein_z80fmly_interrupt},
	{0},
	{0},
    {0}
};


static z80pio_interface einstein_pio_intf = 
{
	1,
	{einstein_z80fmly_interrupt},
	{0},
	{NULL}
};

void einstein_int_reset(int code)
{
}

int	einstein_interrupt(int code)
{

}

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
	return ((readinputport(8) & 0x07)<<5);
}


static WRITE_HANDLER(einstein_key_int_w)
{
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



PORT_READ_START( readport_einstein )
	{0x000, 0x007, einstein_psg_r},
	{0x008, 0x00f, einstein_vdp_r},
	{0x010, 0x017, einstein_serial_r},
	{0x018, 0x01f, einstein_fdc_r},
	{0x020, 0x020, einstein_key_int_r},	
	{0x028, 0x02f, einstein_ctc_r},
	{0x030, 0x037, einstein_pio_r},
	{0x040, 0x0ff, einstein_unmapped_r},
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
	{0x040, 0x0ff, einstein_unmapped_w},
PORT_END



#define EINSTEIN_DUMP_RAM

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

	wd179x_init(NULL);

	einstein_int=0;

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_SS_40);


	/* the einstein keyboard can generate a interrupt */
	/* the int is actually clocked at the system clock 4Mhz, but this would be too fast for our
	driver. So we update at 50Hz and hope this is good enough. */
	einstein_keyboard_timer = timer_pulse(TIME_IN_HZ(50), 0, einstein_keyboard_timer_callback);
}


void einstein_shutdown_machine(void)
{
	einstein_dump_ram();

	if (einstein_ram)
	{
		free(einstein_ram);
		einstein_ram = NULL;
	}

	if (einstein_keyboard_timer)
	{
		timer_remove(einstein_keyboard_timer);
		einstein_keyboard_timer = NULL;
	}
	
	wd179x_exit();
}

INPUT_PORTS_START(einstein)
	/* line 0 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_LALT, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", KEYCODE_1_PAD, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", KEYCODE_2_PAD, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "?", KEYCODE_3_PAD, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 1 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", KEYCODE_MINUS, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "||", KEYCODE_BACKSLASH, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 2 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 3 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "DELETE", KEYCODE_BACKSPACE, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 4 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 5 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 6 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* line 7 */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) 
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE) 
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE) 
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) 
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) 
	PORT_BITX(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE) 
	PORT_BITX(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE) 
	PORT_BIT(0x080, 0x080, IPT_UNUSED)
	/* extra */
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRPH", KEYCODE_F1, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CONTROL", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CONTROL", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)

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


static struct MachineDriver machine_driver_einstein =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
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
	VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	einstein_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

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
	ROM_REGION(0x010000+0x02000, REGION_CPU1,0)
	ROM_LOAD("einstein.rom",0x10000, 0x02000, 0x0ec134953)
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

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 19??, einstein,      0,            einstein,          einstein,      0,       "Tatung", "Tatung Einstein TC-01")
