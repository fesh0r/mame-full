/******************************************************************************

	sord m5
	system driver

	Thankyou to Roman Stec and Jan P. Naidr for the documentation and much
	help.

	http://falabella.lf2.cuni.cz/~naidr/sord/

	PI-5 is the parallel interface using a 8255.
	FD-5 is the disc operating system and disc interface.
	FD-5 is connected to M5 via PI-5.


	Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "includes/centroni.h"
#include "devices/printer.h"
#include "machine/8255ppi.h"
#include "devices/cartslot.h"


#define SORD_DEBUG

/*********************************************************************************************/
/* FD5 disk interface */
/* - Z80 CPU */
/* - 27128 ROM (16K) */
/* - 2x6116 RAM */
/* - Intel8272/NEC765 */
/* - IRQ of NEC765 is connected to INT of Z80 */
/* PI-5 interface is required. mode 2 of the 8255 is used to communicate with the FD-5 */

#include "includes/nec765.h"
#include "image.h"

static MACHINE_INIT( sord_m5 );

static unsigned char fd5_databus;

MEMORY_READ_START( readmem_sord_fd5 )
	{0x0000, 0x03fff, MRA_ROM},	/* internal rom */
	{0x4000, 0x0ffff, MRA_RAM},
MEMORY_END


MEMORY_WRITE_START( writemem_sord_fd5 )
	{0x0000, 0x03fff, MWA_ROM}, /* internal rom */
	{0x4000, 0x0ffff, MWA_RAM},
MEMORY_END

static int obfa,ibfa, intra;
static int fd5_port_0x020_data;

/* stb and ack automatically set on read/write? */
static WRITE_HANDLER(fd5_communication_w)
{
	cpu_yield();

	fd5_port_0x020_data = data;
#ifdef SORD_DEBUG
	logerror("fd5 0x020: %02x %04x\n",data,activecpu_get_pc());
#endif
}

static READ_HANDLER(fd5_communication_r)
{
	int data;

	cpu_yield();

	data = (obfa<<3)|(ibfa<<2)|2;		
#ifdef SORD_DEBUG
	logerror("fd5 0x030: %02x %04x\n",data, activecpu_get_pc());
#endif

	return data;
}

static READ_HANDLER(fd5_data_r)
{
	cpu_yield();

#ifdef SORD_DEBUG
	logerror("fd5 0x010 r: %02x %04x\n",fd5_databus,activecpu_get_pc());
#endif
	
	ppi8255_set_input_acka(0,1);
	ppi8255_set_input_acka(0,0);
	ppi8255_set_input_acka(0,1);

	return fd5_databus;
}

static WRITE_HANDLER(fd5_data_w)
{
#ifdef SORD_DEBUG
	logerror("fd5 0x010 w: %02x %04x\n",data,activecpu_get_pc());
#endif

	fd5_databus = data;

	/* set stb on data write */
	ppi8255_set_input_stba(0,1);
	ppi8255_set_input_stba(0,0);
	ppi8255_set_input_stba(0,1);

	cpu_yield();
}

static WRITE_HANDLER(fd5_drive_control_w)
{
	int state;
	
	if (data==0)
		state = 0;
	else
		state = 1;

#ifdef SORD_DEBUG
	logerror("fd5 drive state w: %02x\n",state);
#endif
	
	floppy_drive_set_motor_state(image_instance(IO_FLOPPY, 0), state);
	floppy_drive_set_motor_state(image_instance(IO_FLOPPY, 0), state);
	floppy_drive_set_ready_state(image_instance(IO_FLOPPY, 1), 1,1);
	floppy_drive_set_ready_state(image_instance(IO_FLOPPY, 1), 1,1);
}

static WRITE_HANDLER(fd5_tc_w)
{
	nec765_set_tc_state(1);
	nec765_set_tc_state(0);
}

/* 0x020 fd5 writes to this port to communicate with m5 */
/* 0x010 data bus to read/write from m5 */
/* 0x030 fd5 reads from this port to communicate with m5 */
/* 0x040 */
/* 0x050 */
PORT_READ_START( readport_sord_fd5 )
	{ 0x000, 0x000, nec765_status_r},
	{ 0x001, 0x001, nec765_data_r},
	{ 0x010, 0x010, fd5_data_r},
	{ 0x030, 0x030, fd5_communication_r},
PORT_END

PORT_WRITE_START( writeport_sord_fd5 )
	{ 0x001, 0x001, nec765_data_w},
	{ 0x010, 0x010, fd5_data_w},	
	{ 0x020, 0x020, fd5_communication_w},
	{ 0x040, 0x040, fd5_drive_control_w},
	{ 0x050, 0x050, fd5_tc_w},
PORT_END

/* nec765 data request is connected to interrupt of z80 inside fd5 interface */
static void sord_fd5_fdc_interrupt(int state)
{
	if (state)
	{
		cpu_set_irq_line(1,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(1,0,CLEAR_LINE);

	}
}

static struct nec765_interface sord_fd5_nec765_interface=
{
	sord_fd5_fdc_interrupt,
	NULL
};

static void sord_fd5_init(void)
{
	nec765_init(&sord_fd5_nec765_interface,NEC765A);
}

static MACHINE_INIT( sord_m5_fd5 )
{
	floppy_drive_set_geometry(image_instance(IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_instance(IO_FLOPPY, 1), FLOPPY_DRIVE_SS_40);
	sord_fd5_init();
	machine_init_sord_m5();
	ppi8255_set_input_acka(0,1);
	ppi8255_set_input_stba(0,1);
}


static mess_image *cassette_image(void)
{
	return image_instance(IO_CASSETTE, 0);
}

/*********************************************************************************************/
/* PI-5 */

static READ_HANDLER(sord_ppi_porta_r)
{
	cpu_yield(); 

	return fd5_databus;
}

static READ_HANDLER(sord_ppi_portb_r)
{
	cpu_yield();

#ifdef SORD_DEBUG
	logerror("m5 read from pi5 port b %04x\n",activecpu_get_pc());
#endif

	return 0x0ff;
}

static READ_HANDLER(sord_ppi_portc_r)
{
	cpu_yield();

#ifdef SORD_DEBUG
	logerror("m5 read from pi5 port c %04x\n",activecpu_get_pc());
#endif

/* from fd5 */
/* 00 = 0000 = write */
/* 02 = 0010 = write */
/* 06 = 0110 = read */
/* 04 = 0100 = read */

/* m5 expects */
/*00 = READ */
/*01 = POTENTIAL TO READ BUT ALSO RESET */
/*10 = WRITE */
/*11 = FORCE RESET AND WRITE */

	/* FD5 bit 0 -> M5 bit 2 */
	/* FD5 bit 2 -> M5 bit 1 */
	/* FD5 bit 1 -> M5 bit 0 */
	return (
			/* FD5 bit 0-> M5 bit 2 */
			((fd5_port_0x020_data & 0x01)<<2) |
			/* FD5 bit 2-> M5 bit 1 */
			((fd5_port_0x020_data & 0x04)>>1) |
			/* FD5 bit 1-> M5 bit 0 */
			((fd5_port_0x020_data & 0x02)>>1)
			);
}

static WRITE_HANDLER(sord_ppi_porta_w)
{
	cpu_yield(); 

	fd5_databus = data;
}

static WRITE_HANDLER(sord_ppi_portb_w)
{
	cpu_yield();

	/* f0, 40 */
	/* 1111 */
	/* 0100 */

	if (data==0x0f0)
	{
		cpu_set_reset_line(1,ASSERT_LINE);
		cpu_set_reset_line(1,CLEAR_LINE);
	}
#ifdef SORD_DEBUG
	logerror("m5 write to pi5 port b: %02x %04x\n",data,activecpu_get_pc());
#endif
}

/* A,  B,  C,  D,  E,   F,  G,  H,  I,  J, K,  L,  M,   N, O, P, Q, R,   */
/* 41, 42, 43, 44, 45, 46, 47, 48, 49, 4a, 4b, 4c, 4d, 4e, 4f, 50, 51, 52*/

/* C,H,N */


static WRITE_HANDLER(sord_ppi_portc_w)
{
	cpu_yield();
#ifdef SORD_DEBUG
	logerror("m5 write to pi5 port c: %02x %04x\n",data,activecpu_get_pc());
#endif
}

static WRITE_HANDLER(sord_ppi_obfa_write)
{
//	logerror("ppi obfa write %02x %04x\n",data,activecpu_get_pc());
	obfa = data & 0x01;
	cpu_yield();
}

static WRITE_HANDLER(sord_ppi_intra_write)
{
//	logerror("ppi intra write %02x %04x\n",data,activecpu_get_pc());
	intra = data & 0x01;
	cpu_yield();
}

static WRITE_HANDLER(sord_ppi_ibfa_write)
{
//	logerror("ppi ibfa write %02x %04x\n",data,activecpu_get_pc());
	ibfa = data & 0x01;
	cpu_yield();
}

static ppi8255_mode2_interface sord_ppi8255_mode2_interface = 
{
	{sord_ppi_obfa_write},
	{sord_ppi_intra_write},
	{sord_ppi_ibfa_write}
};

static ppi8255_interface sord_ppi8255_interface =
{
	1,
	{sord_ppi_porta_r},
	{sord_ppi_portb_r},
	{sord_ppi_portc_r},
	{sord_ppi_porta_w},
	{sord_ppi_portb_w},
	{sord_ppi_portc_w}
};

/*********************************************************************************************/


static char cart_data[0x06fff-0x02000];

static DEVICE_LOAD( sord_cartslot )
{
	int datasize;

	/* get file size */
	datasize = mame_fsize(file);

	if (datasize!=0)
	{
		/* read whole file */
		mame_fread(file, cart_data, datasize);
	}
	return INIT_PASS;
}

static DEVICE_LOAD( sord_floppy )
{
	if (basicdsk_floppy_load(image, file, open_mode)==INIT_PASS)
	{
		/* 40 tracks, single sided, 256 bytes per sector, 18 sectors */
		basicdsk_set_geometry(image, 40, 1, 18, 256, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}



static DEVICE_LOAD( sord_cassette )
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(image, file, open_mode, &args);
}

static void sord_m5_ctc_interrupt(int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
	cpu_set_irq_line(0, 0, state);
}

static z80ctc_interface	sord_m5_ctc_intf =
{
	1,
	{3800000},
	{0},
	{sord_m5_ctc_interrupt},
	{0},
	{0},
    {0}
};

static READ_HANDLER ( sord_keyboard_r )
{
	return readinputport(offset);
}

MEMORY_READ_START( readmem_sord_m5 )
	{0x0000, 0x01fff, MRA_ROM},	/* internal rom */
	{0x2000, 0x06fff, MRA_BANK1},
	{0x7000, 0x0ffff, MRA_RAM},
MEMORY_END



MEMORY_WRITE_START( writemem_sord_m5 )
	{0x0000, 0x01fff, MWA_ROM}, /* internal rom */
	{0x02000, 0x06fff, MWA_NOP},	
	{0x7000, 0x0ffff, MWA_RAM},
MEMORY_END

static READ_HANDLER(sord_ctc_r)
{
	unsigned char data;

	data = z80ctc_0_r(offset & 0x03);

	logerror("sord ctc r: %04x %02x\n",(offset & 0x03), data);

	return data;
}

static WRITE_HANDLER(sord_ctc_w)
{
	logerror("sord ctc w: %04x %02x\n",(offset & 0x03), data);

	z80ctc_0_w(offset & 0x03, data);
}

static READ_HANDLER(sord_video_r)
{
	if (offset & 0x01)
	{
		return TMS9928A_register_r(offset);
	}

	return TMS9928A_vram_r(offset);
}

static WRITE_HANDLER(sord_video_w)
{
	if (offset & 0x01)
	{
		TMS9928A_register_w(offset,data);
		return;
	}

	TMS9928A_vram_w(offset,data);
}


static READ_HANDLER(sord_sys_r)
{
	unsigned char data;
	int printer_handshake;

	data = 0;

	/* cassette read */
	if (device_input(cassette_image()) >=0)
		data |=(1<<0);

	printer_handshake = centronics_read_handshake(0);

	/* if printer is not online, it is busy */
	if ((printer_handshake & CENTRONICS_ONLINE)!=0)
	{
		data|=(1<<1);
	}

	/* bit 7 must be 0 for saving and loading to work */

	logerror("sys read: %02x\n",data);

	return data;
}

/* write */
/* bit 0 is strobe to printer or cassette write data */
/* bit 1 is cassette remote */

/* read */
/* bit 0 is cassette read data */
/* bit 1 is printer busy */

static WRITE_HANDLER(sord_sys_w)
{
	int handshake;

	handshake = 0;
	if (data & (1<<0))
	{
		handshake = CENTRONICS_STROBE;
	}

	/* cassette remote */
	device_status(cassette_image(), ((data>>1) & 0x01));

	/* cassette data */
	device_output(cassette_image(), (data & (1<<0)) ? -32768 : 32767);

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(0, handshake, CENTRONICS_STROBE);

	logerror("sys write: %02x\n",data);
}

static WRITE_HANDLER(sord_printer_w)
{
//	logerror("centronics w: %02x\n",data);
	centronics_write_data(0,data);
}

PORT_READ_START( readport_sord_m5 )
	{ 0x000, 0x00f, sord_ctc_r},
	{ 0x010, 0x01f, sord_video_r},
	{ 0x030, 0x03f, sord_keyboard_r},
	{ 0x050, 0x050, sord_sys_r},
PORT_END

PORT_READ_START( readport_srdm5fd5 )
	{ 0x000, 0x00f, sord_ctc_r},
	{ 0x010, 0x01f, sord_video_r},
	{ 0x030, 0x03f, sord_keyboard_r},
	{ 0x050, 0x050, sord_sys_r},
	{ 0x070, 0x073, ppi8255_0_r},
PORT_END


PORT_WRITE_START( writeport_sord_m5 )
	{ 0x000, 0x00f, sord_ctc_w},
	{ 0x010, 0x01f, sord_video_w},
	{ 0x020, 0x02f, SN76496_0_w},
	{ 0x040, 0x040, sord_printer_w}, 
	{ 0x050, 0x050, sord_sys_w},
PORT_END

PORT_WRITE_START( writeport_srdm5fd5 )
	{ 0x000, 0x00f, sord_ctc_w},
	{ 0x010, 0x01f, sord_video_w},
	{ 0x020, 0x02f, SN76496_0_w},
	{ 0x040, 0x040, sord_printer_w}, 
	{ 0x050, 0x050, sord_sys_w},
	{ 0x070, 0x073, ppi8255_0_w},
PORT_END

//static void cassette_timer_callback(int dummy)
//{
//	int data;
//
//	data = 0;
//	/* cassette read */
//	if (device_input(cassette_image()) > 255)
//		data |=(1<<0);
//
//	z80ctc_0_trg2_w(0,data);
//}


static CENTRONICS_CONFIG sordm5_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		NULL
	},
};

static void sordm5_video_interrupt_callback(int state)
{
	if (state)
	{
		z80ctc_0_trg3_w(0,1);
		z80ctc_0_trg3_w(0,0);
	}
}

static MACHINE_INIT( sord_m5 )
{
	z80ctc_init(&sord_m5_ctc_intf);

	/* PI-5 interface connected to Sord M5 */
	ppi8255_init(&sord_ppi8255_interface);
	ppi8255_set_mode2_interface(&sord_ppi8255_mode2_interface);

//	cassette_timer = timer_pulse(TIME_IN_HZ(11025), 0, cassette_timer_callback);

	TMS9928A_reset ();
	z80ctc_reset(0);



	/* should be done in a special callback to work properly! */
	cpu_setbank(1, cart_data);

	centronics_config(0, sordm5_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
}

#define SORD_DUMP_RAM

#ifdef SORD_DUMP_RAM
static void sord_dump_ram(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, "sord.bin", FILETYPE_MEMCARD,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;

		for (i=0; i<65536; i++)
		{
			unsigned char data[1];

			data[0] = cpunum_read_byte(0,i);

			mame_fwrite(file, data, 1);
		}

		/* close file */
		mame_fclose(file);
	}
}

static void sordfd5_dump_ram(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, "sordfd5.bin", FILETYPE_MEMCARD,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;

		for (i=0; i<65536; i++)
		{
			unsigned char data[1];

			data[0] = cpunum_read_byte(1,i);
			
			mame_fwrite(file, data, 1);
		}

		/* close file */
		mame_fclose(file);
	}
}
#endif

INPUT_PORTS_START(sord_m5)
	/* line 0 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "FUNC", KEYCODE_LALT, IP_JOY_NONE)
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LSHIFT", KEYCODE_LSHIFT, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_RSHIFT, IP_JOY_NONE) 
	PORT_BIT  (0x10, 0x00, IPT_UNUSED) 
	PORT_BIT  (0x20, 0x00, IPT_UNUSED) 
	PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	/* line 1 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE) 
	/* line 2 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE) 
	/* line 3 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE) 
	/* line 4 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) 
	/* line 5 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^", KEYCODE_EQUALS, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "_", KEYCODE_MINUS_PAD, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE) 
	/* line 6 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_PLUS_PAD, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE) 
	/* line 7 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 1 RIGHT", IP_KEY_NONE, JOYCODE_1_RIGHT) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 1 UP", IP_KEY_NONE, JOYCODE_1_UP) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 1 LEFT", IP_KEY_NONE, JOYCODE_1_LEFT) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 1 DOWN", IP_KEY_NONE, JOYCODE_1_DOWN) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 2 RIGHT", IP_KEY_NONE, JOYCODE_2_RIGHT) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 2 UP", IP_KEY_NONE, JOYCODE_2_UP) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 2 LEFT", IP_KEY_NONE, JOYCODE_2_LEFT) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "JOY 2 DOWN", IP_KEY_NONE, JOYCODE_2_DOWN) 
	/* line 8 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED) 
	/* line 9 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED) 
	/* line 10 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_2_PAD, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_3_PAD, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_4_PAD, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_5_PAD, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_6_PAD, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_7_PAD, IP_JOY_NONE) 
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_8_PAD, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT", KEYCODE_9_PAD, IP_JOY_NONE) 
/*	PORT_BIT (0x0ff, 0x000, IPT_UNUSED) */
	/* line 11 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 12 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 13 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 14 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	/* line 15 */
	PORT_START
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "RESET", KEYCODE_ESC, IP_JOY_NONE) 

INPUT_PORTS_END


static Z80_DaisyChain sord_m5_daisy_chain[] =
{
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};


static struct SN76496interface sn76496_interface =
{
    1,  		/* 1 chip 		*/
    {3579545},  /* 3.579545 MHz */
    { 100 }
};

static INTERRUPT_GEN( sord_interrupt )
{
	if (TMS9928A_interrupt())
		cpu_set_irq_line(0, 0, PULSE_LINE);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	sordm5_video_interrupt_callback
};

static MACHINE_DRIVER_START( sord_m5 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3800000)
	MDRV_CPU_MEMORY(readmem_sord_m5,writemem_sord_m5)
	MDRV_CPU_PORTS(readport_sord_m5,writeport_sord_m5)
	MDRV_CPU_VBLANK_INT(sord_interrupt, 1)
	MDRV_CPU_CONFIG( sord_m5_daisy_chain )
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( sord_m5 )

	/* video hardware */
	MDRV_TMS9928A( &tms9928a_interface )

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sord_m5_fd5 )
	MDRV_IMPORT_FROM( sord_m5 )

	MDRV_CPU_REPLACE("main", Z80_MSX, 3800000)
	MDRV_CPU_PORTS(readport_srdm5fd5,writeport_srdm5fd5)

	MDRV_CPU_ADD(Z80, 3800000)
	MDRV_CPU_MEMORY(readmem_sord_fd5,writemem_sord_fd5)
	MDRV_CPU_PORTS(readport_sord_fd5,writeport_sord_fd5)

	MDRV_INTERLEAVE(20)
	MDRV_MACHINE_INIT( sord_m5_fd5 )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(sordm5)
	ROM_REGION(0x010000, REGION_CPU1,0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, 0x078848d39)
ROM_END


ROM_START(srdm5fd5)
	ROM_REGION(0x010000, REGION_CPU1,0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, 0x078848d39)
	ROM_REGION(0x010000, REGION_CPU2,0)
	ROM_LOAD("sordfd5.rom",0x0000, 0x04000, 0x01)
ROM_END

SYSTEM_CONFIG_START(sordm5)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE_PRINTER			(1)
	CONFIG_DEVICE_CASSETTE			(1, "",			device_load_sord_cassette)
	CONFIG_DEVICE_CARTSLOT_REQ		(1, "rom\0",	NULL, NULL, device_load_sord_cartslot, NULL, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(srdm5fd5)
	CONFIG_IMPORT_FROM(sordm5)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",	device_load_sord_floppy)
SYSTEM_CONFIG_END

/*    YEAR  NAME		PARENT  MACHINE			INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP( 1983, sordm5,		0,		sord_m5,		sord_m5,	0,		sordm5,		"Sord",		"Sord M5")
COMPX(1983, srdm5fd5,	0,		sord_m5_fd5,	sord_m5,	0,		srdm5fd5,	"Sord",		"Sord M5 + PI5 + FD5", GAME_NOT_WORKING)
