/******************************************************************************

	sord m5
	system driver

	Thankyou to Roman Stec and Jan P. Naidr for the documentation and much
	help.

	http://falabella.lf2.cuni.cz/~naidr/sord/

	- some discs to test FD-5 
	- details on accessing cassette required
	- testing required

	Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "includes/basicdsk.h"
#include "cassette.h"


/*********************************************************************************************/
/* FD5 disk interface */
/* - Z80 CPU */
/* - 27128 ROM (16K) */
/* - 2x6116 RAM */
/* - Intel8272/NEC765 */

#include "includes/nec765.h"

static void sord_m5_init_machine(void);
static void sord_m5_shutdown_machine(void);

static unsigned char fd5_input;
static unsigned char fd5_output;
static unsigned char fd5_databus;

MEMORY_READ_START( readmem_sord_fd5 )
	{0x0000, 0x03fff, MRA_ROM},	/* internal rom */
	{0x4000, 0x0ffff, MRA_RAM},
MEMORY_END


MEMORY_WRITE_START( writemem_sord_fd5 )
	{0x0000, 0x03fff, MWA_ROM}, /* internal rom */
	{0x4000, 0x0ffff, MWA_RAM},
MEMORY_END

/* bit 0 = 1: ready to receive data? */
/* 0x07 */
WRITE_HANDLER(fd5_communication_w)
{
	logerror("fd5 write to m5: %02x\n",data);
	fd5_output = 0;
	if (data & 0x01)
	{
		fd5_output |= (1<<7);
	}
	if (data & 0x02)
	{
		fd5_output |= (1<<6);
	}
	if (data & 0x04)
	{
		fd5_output |= (1<<5);
	}
	if (data & 0x08)
	{
		fd5_output |= (1<<4);
	}
	cpu_yield();
}

/* bit 0 = 0 & bit 1 = 1, start transfer? */
/* bit 0 = 0 & bit 2 = 0, write to m5 */ 
/* bit 0 = 0 & bit 3 = 0, read from m5 */
READ_HANDLER(fd5_communication_r)
{
	logerror("fd5 read from m5\n");
	cpu_yield();
	return fd5_input;
}

READ_HANDLER(fd5_data_r)
{
	cpu_yield();
	logerror("fd5 read from fd5 databus %02x\n",fd5_databus);
	return fd5_databus;
}

WRITE_HANDLER(fd5_data_w)
{
	logerror("fd5 write to fd5 databus: %02x\n",data);
	fd5_databus = data;
	cpu_yield();
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
PORT_END

static struct nec765_interface sord_fd5_nec765_interface=
{
	NULL,
	NULL
};

static void sord_fd5_init(void)
{
	nec765_init(&sord_fd5_nec765_interface,NEC765A);
}

static void sord_fd5_exit(void)
{
	nec765_stop();
}

static void sord_m5_fd5_init_machine(void)
{
	sord_fd5_init();
	sord_m5_init_machine();
	fd5_input = 0x0ff;
	fd5_output = 0x0ff;
}

static void sord_m5_fd5_shutdown_machine(void)
{
	sord_fd5_exit();
	sord_m5_shutdown_machine();
}

READ_HANDLER(sord_fd5_data_r)
{
	cpu_yield();
	logerror("m5 read from fd5 databus %02x\n",fd5_databus);
	return fd5_databus;
}

WRITE_HANDLER(sord_fd5_data_w)
{
	logerror("m5 write to fd5 databus: %02x\n",data);
	fd5_databus = data;
	cpu_yield();
}


READ_HANDLER(sord_fd5_communication_r)
{
	logerror("read from fd5: %02x\n",fd5_output);
	cpu_yield();
	return fd5_output;
}

/* 111 */

/* port 0x071 */
/* 00, ff, 40  11111111 01000000 */
/* port 0x072 */
/* bit 7 = 1: fd5 is ready for data to be written to it */
/* bit 5 = 1: fd5 is ready for data to be read from it */
/* bit 2 = ?? */
WRITE_HANDLER(sord_fd5_communication_w)
{
	logerror("write to fd5: %02x\n",data);
	
	fd5_input = data;

	fd5_input = 0;
	if ((data & 0x010)==0)
	{
		fd5_input |= (1<<3);
	}
	if ((data & 0x020)==0)
	{
		fd5_input |= (1<<2);
	}
	if ((data & 0x040)==0)
	{
		fd5_input |= (1<<1);
	}
	if ((data & 0x080)==0)
	{
		fd5_input |= (1<<0);
	}


	cpu_yield();
}


/*********************************************************************************************/


void *video_timer = NULL;

static char cart_data[0x06fff-0x02000];

int		sord_cartslot_init(int id)
{
	void *file;

	if (device_filename(IO_CARTSLOT,id)==NULL)
		return INIT_FAIL;

	if (strlen(device_filename(IO_CARTSLOT,id))==0)
		return INIT_FAIL;

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* read whole file */
			osd_fread(file, cart_data, datasize);
		}
		osd_fclose(file);

		return INIT_PASS;

	}
	return INIT_FAIL;
}

void	sord_cartslot_exit(int id)
{
}

int sord_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY,id)==NULL)
		return INIT_PASS;

	if (basicdsk_floppy_init(id)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1,0);
		return INIT_PASS;
	}

	return INIT_FAIL;
}



int sord_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void sord_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


static void sord_m5_ctc_interrupt(int state)
{
	//logerror("interrupting ctc %02x\r\n ",state);
    cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

static WRITE_HANDLER(sord_video_interrupt)
{
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

int sord_m5_vh_init(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);
}

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
	return z80ctc_0_r(offset & 0x03);
}

static WRITE_HANDLER(sord_ctc_w)
{
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

static WRITE_HANDLER(sord_diskhw_w)
{
/*
7 -  HI/NOR	- high / normal
6 - OFF/ON	- LED lamp, select
5 -  ON/OFF	- wait
4 -  ON/OFF	- motor
3 -  FM/MFM	- density
2 -  8"/5.25"	- drive
1 -   1/0	- not used ( pozdeji se pouzije pro prepinani drajvru 0 a 1 )
0 -   2/1	- frequency MHz
*/
	if (data & (1<<3))
	{
		wd179x_set_density(DEN_FM_HI);
	}
	else
	{
		wd179x_set_density(DEN_MFM_LO);
	}
}

/* 3,2 written */
/*
+----------+--------------------+-------------------------------+
|   bit    | vstup 		| vystup			|
| 76543210 +--------------------+-------------------------------+
| 00000001 | vstup OLD z MGF	| SAVE na MGF a DSTB na PRT	|
| 00000010 | BUSY od PRT	| STS na PRT a na civku rele	|
| 00000100 | COM pro PRT	| ---				|
| 10000000 | klavesa <RESET>	| ---				|
+----------+--------------------+-------------------------------+
*/
static READ_HANDLER(sord_sys_r)
{
	return 0x0ff;

//	return ((readinputport(16) & 0x01)<<7);
}

static WRITE_HANDLER(sord_sys_w)
{

}



PORT_READ_START( readport_sord_m5 )
	{ 0x000, 0x00f, sord_ctc_r},
	{ 0x010, 0x01f, sord_video_r},
	{ 0x030, 0x03f, sord_keyboard_r},
	{ 0x050, 0x050, sord_sys_r},
	{ 0x070, 0x070, sord_fd5_data_r},
	{ 0x072, 0x072, sord_fd5_communication_r},

//	{ 0x078, 0x078, wd179x_status_r},
//	{ 0x079, 0x079, wd179x_track_r},
//	{ 0x07a, 0x07a, wd179x_sector_r},
//	{ 0x07b, 0x07b, wd179x_data_r},
PORT_END

PORT_WRITE_START( writeport_sord_m5 )
	{ 0x000, 0x00f, sord_ctc_w},
	{ 0x010, 0x01f, sord_video_w},
	{ 0x020, 0x02f, SN76496_0_w},
	{ 0x070, 0x070, sord_fd5_data_w},
	{ 0x071, 0x071, sord_fd5_communication_w},	
	//	{ 0x050, 0x050, sord_sys_w},
//	{ 0x078, 0x078, wd179x_command_w},
//	{ 0x079, 0x079, wd179x_track_w},
//	{ 0x07a, 0x07a, wd179x_sector_w},
//	{ 0x07b, 0x07b, wd179x_data_w},
//	{ 0x07c, 0x07c, sord_diskhw_w},
PORT_END


static int sord_m5_interrupt(void)
{
	TMS9928A_interrupt();
	return ignore_interrupt();
}

static void video_timer_callback(int dummy)
{
	z80ctc_0_trg3_w(0,1);
	z80ctc_0_trg3_w(0,0);
}

void sord_m5_init_machine(void)
{
	z80ctc_init(&sord_m5_ctc_intf);
	
	video_timer = timer_pulse(TIME_IN_MSEC(16.7), 0, video_timer_callback);

	wd179x_init(WD_TYPE_179X,NULL);
	TMS9928A_reset ();
	z80ctc_reset(0);

	wd179x_reset();

	/* should be done in a special callback to work properly! */
	cpu_setbank(1, cart_data);

}

#define SORD_DUMP_RAM

#ifdef SORD_DUMP_RAM
void sord_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "sord.bin", OSD_FILETYPE_NVRAM,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;

		for (i=0; i<65536; i++)
		{
			unsigned char data[1];

			data[0] = cpu_readmem16(i);

			osd_fwrite(file, data, 1);
		}

		/* close file */
		osd_fclose(file);
	}
}
#endif


void sord_m5_shutdown_machine(void)
{
	sord_dump_ram();

	if (video_timer)
	{
		timer_remove(video_timer);
		video_timer = NULL;
	}

	wd179x_exit();
}

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
    PORT_BITX (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "_", KEYCODE_0_PAD, IP_JOY_NONE) 
    PORT_BITX (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE) 
	/* line 6 */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) 
    PORT_BITX (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) 
    PORT_BITX (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE) 
    PORT_BITX (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE) 
    PORT_BITX (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) 
    PORT_BITX (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_1_PAD, IP_JOY_NONE) 
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
	PORT_BIT (0x0ff, 0x000, IPT_UNUSED)
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

static struct MachineDriver machine_driver_sord_m5 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			3800000,
			readmem_sord_m5,		   /* MemoryReadAddress */
			writemem_sord_m5,		   /* MemoryWriteAddress */
			readport_sord_m5,		   /* IOReadPort */
			writeport_sord_m5,		   /* IOWritePort */
            sord_m5_interrupt, 1,
			0, 0,	/* every scanline */
			sord_m5_daisy_chain
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	sord_m5_init_machine,			   /* init machine */
	sord_m5_shutdown_machine,
	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	sord_m5_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver machine_driver_sord_m5_fd5 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			3800000,
			readmem_sord_m5,		   /* MemoryReadAddress */
			writemem_sord_m5,		   /* MemoryWriteAddress */
			readport_sord_m5,		   /* IOReadPort */
			writeport_sord_m5,		   /* IOWritePort */
            sord_m5_interrupt, 1,
			0, 0,	/* every scanline */
			sord_m5_daisy_chain
		},
		{
			CPU_Z80,
			4000000,
			readmem_sord_fd5,
			writemem_sord_fd5,
			readport_sord_fd5,
			writeport_sord_fd5,
			0,0,
			0,0,
			NULL
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	sord_m5_fd5_init_machine,			   /* init machine */
	sord_m5_fd5_shutdown_machine,
	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	sord_m5_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(sordm5)
	ROM_REGION(0x010000, REGION_CPU1,0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, 0x078848d39)
ROM_END


ROM_START(sordm5fd5)
	ROM_REGION(0x010000, REGION_CPU1,0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, 0x078848d39)
	ROM_REGION(0x010000, REGION_CPU2,0)
	ROM_LOAD("sordfd5.rom",0x0000, 0x04000, 0x01)
ROM_END

#define sord_m5_cart_device \
	{ \
		IO_CARTSLOT,\
		1,						/* count */\
		"rom\0",                /* file extensions */\
		IO_RESET_NONE,			/* reset if file changed */\
		NULL,					/* id */\
		sord_cartslot_init,		/* init */\
		sord_cartslot_exit,		/* exit */\
		NULL,					/* info */\
		NULL,					/* open */\
		NULL,					/* close */\
		NULL,					/* status */\
		NULL,					/* seek */\
		NULL,					/* tell */\
		NULL,					/* input */\
		NULL,					/* output */\
		NULL,					/* input_chunk */\
		NULL					/* output_chunk */\
	}


static const struct IODevice io_sordm5[] =
{
	sord_m5_cart_device,
	{IO_END},
};

static const struct IODevice io_sordm5fd5[] = 
{
	sord_m5_cart_device,
	{
		IO_FLOPPY,				/* type */
		4,						/* count */
		"dsk\0",                /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL, /*basicdsk_floppy_id,*/ 	/* id */
		sord_floppy_init, /* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	IO_CASSETTE_WAVE(1,"wav\0",NULL,sord_cassette_init,sord_cassette_exit),
	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 1983, sordm5,      0,            sord_m5,          sord_m5,      0,       "Sord", "Sord M5")
COMPX( 1983, sordm5fd5,	0,	sord_m5_fd5, sord_m5, 0, "Sord", "Sord M5 + FD5 disk interface", GAME_NOT_WORKING)
