/******************************************************************************

 ******************************************************************************/
#include "driver.h"
#include "vidhrdw/m6847.h"
#include "includes/apf.h"

#include "machine/6821pia.h"
#include "includes/wd179x.h"
#include "devices/basicdsk.h"

 /*
0000- 2000-2003 PIA of M1000. is itself repeated until 3fff. 
She controls " keypads ", the way of video and the loudspeaker. Putting to 0 one of the 4 least 
significant bits of 2002, the corresponding line of keys is ***reflxed mng in 2000 (32 keys altogether). 
The other four bits of 2002 control the video way. Bit 0 of 2003 controls the interruptions. Bit 3 of 2003 controls 
the loudspeaker. 4000-47ff ROM of M1000. is repeated until 5fff and in e000-ffff. 6000-6003 PIA of the APF. 
is itself repeated until 63ff. She controls the keyboard of the APF and the cassette. The value of the 
number that represents the three bits less significant of 6002 they determine the line of eight 
keys that it is ***reflxed mng in 6000. Bit 4 of 6002 controls the motor of the cassette. Bit 5 of 
6002 activates or deactivates the recording. Bit 5 of 6002 indicates the level of recording in 
the cassette. 6400-64ff would be the interface Here series, optional. 6500-66ff would be the 
disk controller Here, optional. 6800-77ff ROM (" cartridge ") 7800-7fff Probably ROM. The BASIC 
leaves frees this zone. 8000-9fff ROM (" cartridge ") A000-BFFF ram (8 Kb) C000-DFFF ram 
additional (8 Kb) E000-FFFF ROM of M1000 (to see 4000-47FF) The interruption activates with 
the vertical synchronism of the video. routine that it executes is in the ROM of M1000 
and puts the video in way text during a short interval, so that the first line is seen of 
text screen in the superior part of the graphical screen. 
*/

/* 6600, 6500-6503 wd179x disc controller? 6400, 6401 */
static unsigned char keyboard_data;
static unsigned char pad_data;

static READ_HANDLER(apf_m1000_pia_in_a_func)
{
	logerror("pia 0 a r: %04x\n",offset);

	return pad_data;
}

static READ_HANDLER(apf_m1000_pia_in_b_func)
{
	
	logerror("pia 0 b r: %04x\n",offset);

	return 0x0ff;
}

static READ_HANDLER(apf_m1000_pia_in_ca1_func)
{
	logerror("pia 0 ca1 r: %04x\n",offset);

	return 0;
}

static READ_HANDLER(apf_m1000_pia_in_cb1_func)
{
	logerror("pia 0 cb1 r: %04x\n",offset);

	return 0x00;
}

static READ_HANDLER(apf_m1000_pia_in_ca2_func)
{
	logerror("pia 0 ca2 r: %04x\n",offset);

	return 0;
}

static READ_HANDLER(apf_m1000_pia_in_cb2_func)
{
	logerror("pia 0 cb2 r: %04x\n",offset);

	return 0x00;
}


static WRITE_HANDLER(apf_m1000_pia_out_a_func)
{
	logerror("pia 0 a w: %04x %02x\n",offset,data);
}

unsigned char previous_mode;

static WRITE_HANDLER(apf_m1000_pia_out_b_func)
{
	pad_data = 0x0ff;

	/* bit 7..4 video control */
	/* bit 3..0 keypad line select */
	if (data & 0x08)
		pad_data &= readinputport(3);
	if (data & 0x04)
		pad_data &= readinputport(2);
	if (data & 0x02)
		pad_data &= readinputport(1);
	if (data & 0x01)
		pad_data &= readinputport(0);

	/* 1b standard, 5b, db */
	/* 0001 */
	/* 0101 */
	/* 1101 */

	/* multi colour graphics mode */
	/* 158 = 1001 multi-colour graphics */
	/* 222 = 1101 mono graphics */
	//	if (((previous_mode^data) & 0x0f0)!=0)
	{
		/* not sure if this is correct - need to check */
		m6847_ag_w(0,	data & 0x80);
		m6847_gm0_w(0,	data & 0x10);
		m6847_gm1_w(0,	data & 0x20);
		m6847_gm2_w(0,	data & 0x40);
/*		m6847_set_cannonical_row_height(); */
		previous_mode = data;
	}
	//	schedule_full_refresh();

	logerror("pia 0 b w: %04x %02x\n",offset,data);
}

static WRITE_HANDLER(apf_m1000_pia_out_ca2_func)
{
	logerror("pia 0 ca2 w: %04x %02x\n",offset,data);
}

static WRITE_HANDLER(apf_m1000_pia_out_cb2_func)
{
	speaker_level_w(0, data);
}

/* use bit 0 to identify state of irq from pia 0 */
/* use bit 1 to identify state of irq from pia 0 */
/* use bit 2 to identify state of irq from pia 1 */
/* use bit 3 to identify state of irq from pia 1 */
/* use bit 4 to identify state of irq from video */

unsigned char apf_ints;

void apf_update_ints(void)
{
	if (apf_ints!=0)
	{
		cpu_set_irq_line(0,0,HOLD_LINE);
		logerror("set int\n");
	}
	else
	{
		cpu_set_irq_line(0,0,CLEAR_LINE);
		logerror("clear int\n");
	}
}

static void	apf_m1000_irq_a_func(int state)
{
	//logerror("pia 0 irq a %d\n",state);

	if (state)
	{
		apf_ints|=1;
	}
	else
	{
		apf_ints&=~1;
	}

	apf_update_ints();
}


static void	apf_m1000_irq_b_func(int state)
{
	//logerror("pia 0 irq b %d\n",state);

	if (state)
	{
		apf_ints|=2;
	}
	else
	{
		apf_ints&=~2;
	}

	apf_update_ints();

}

struct pia6821_interface apf_m1000_pia_interface=
{
	apf_m1000_pia_in_a_func,
	apf_m1000_pia_in_b_func,
	apf_m1000_pia_in_ca1_func,
	apf_m1000_pia_in_cb1_func,
	apf_m1000_pia_in_ca2_func,
	apf_m1000_pia_in_cb2_func,
	apf_m1000_pia_out_a_func,
	apf_m1000_pia_out_b_func,
	apf_m1000_pia_out_ca2_func,
	apf_m1000_pia_out_cb2_func,
	apf_m1000_irq_a_func,
	apf_m1000_irq_b_func
};


static READ_HANDLER(apf_imagination_pia_in_a_func)
{
	logerror("pia 1 a r: %04x\n",offset);

	return keyboard_data;
}

static READ_HANDLER(apf_imagination_pia_in_b_func)
{
//	logerror("pia 1 b r: %04x\n",offset);
	unsigned char data;

	data = 0x000;

	if (device_input(image_from_devtype_and_index(IO_CASSETTE,0)) > 255)
		data =(1<<7);

	return data;
//	return 0x0ff;
}

static READ_HANDLER(apf_imagination_pia_in_ca1_func)
{
	logerror("pia 1 ca1 r: %04x\n",offset);

	return 0x00;
}

static READ_HANDLER(apf_imagination_pia_in_cb1_func)
{
	logerror("pia 1 cb1 r: %04x\n",offset);

	return 0x00;
}

static READ_HANDLER(apf_imagination_pia_in_ca2_func)
{
	logerror("pia 1 ca2 r: %04x\n",offset);

	return 0x00;
}

static READ_HANDLER(apf_imagination_pia_in_cb2_func)
{
	logerror("pia 1 cb2 r: %04x\n",offset);

	return 0x00;
}


static WRITE_HANDLER(apf_imagination_pia_out_a_func)
{
	logerror("pia 1 a w: %04x %02x\n",offset,data);
}

static WRITE_HANDLER(apf_imagination_pia_out_b_func)
{
	/* bits 2..0 = keyboard line */
	/* bit 3 = ??? */
	/* bit 4 = cassette motor */
	/* bit 5 = ?? */
	/* bit 6 = cassette write data */
	/* bit 7 = ??? */

	int keyboard_line;

	keyboard_line = data & 0x07;

	keyboard_data = readinputport(keyboard_line+4);

	/* bit 4: cassette motor control */
	device_status(image_from_devtype_and_index(IO_CASSETTE,0), ((data>>4) & 0x01));
	/* bit 6: cassette write */
	device_output(image_from_devtype_and_index(IO_CASSETTE,0), (data & (1<<6)) ? -32768 : 32767);


	logerror("pia 1 b w: %04x %02x\n",offset,data);
}

static WRITE_HANDLER(apf_imagination_pia_out_ca2_func)
{
	//logerror("pia 1 ca2 w: %04x %02x\n",offset,data);
}

static WRITE_HANDLER(apf_imagination_pia_out_cb2_func)
{
	//logerror("pia 1 cb2 w: %04x %02x\n",offset,data);
}

static void	apf_imagination_irq_a_func(int state)
{
	//logerror("pia 1 irq a %d\n",state);
	if (state)
	{
		apf_ints|=4;
	}
	else
	{
		apf_ints&=~4;
	}

	apf_update_ints();

}

static void	apf_imagination_irq_b_func(int state)
{
	//logerror("pia 1 irq b %d\n",state);

	if (state)
	{
		apf_ints|=8;
	}
	else
	{
		apf_ints&=~8;
	}

	apf_update_ints();

}

struct pia6821_interface apf_imagination_pia_interface=
{
	apf_imagination_pia_in_a_func,
	apf_imagination_pia_in_b_func,
	apf_imagination_pia_in_ca1_func,
	apf_imagination_pia_in_cb1_func,
	apf_imagination_pia_in_ca2_func,
	apf_imagination_pia_in_cb2_func,
	apf_imagination_pia_out_a_func,
	apf_imagination_pia_out_b_func,
	apf_imagination_pia_out_ca2_func,
	apf_imagination_pia_out_cb2_func,
	apf_imagination_irq_a_func,
	apf_imagination_irq_b_func
};


extern unsigned char *apf_video_ram;

static void apf_common_init(void)
{
	unsigned char *rom_ptr = memory_region(REGION_CPU1) + 0x010000;

	apf_ints = 0;

	cpu_setbank(1, rom_ptr);
	cpu_setbank(2, rom_ptr);
	cpu_setbank(3, rom_ptr);
	cpu_setbank(4, rom_ptr);
	cpu_setbank(5, rom_ptr);
	cpu_setbank(6, rom_ptr);
	cpu_setbank(7, rom_ptr);
	cpu_setbank(8, rom_ptr);

	pia_config(0, PIA_STANDARD_ORDERING,&apf_m1000_pia_interface);

	pia_reset();

}

static READ_HANDLER(apf_pia_0_r)
{
	return pia_0_r(offset & 0x03);
}

static WRITE_HANDLER(apf_pia_0_w)
{
	pia_0_w(offset & 0x03, data);
}

static READ_HANDLER(apf_pia_1_r)
{
	return pia_1_r(offset & 0x03);
}

static WRITE_HANDLER(apf_pia_1_w)
{
	pia_1_w(offset & 0x03, data);
}

static MACHINE_INIT( apf_imagination )
{
	pia_config(1, PIA_STANDARD_ORDERING,&apf_imagination_pia_interface);

	apf_common_init();

	wd179x_init(WD_TYPE_179X,NULL);
}

static MACHINE_INIT( apf_m1000 )
{
	apf_common_init();
}

static WRITE_HANDLER(apf_dischw_w)
{
	int drive;

	/* bit 3 is index of drive to select */
	drive = (data>>3) & 0x01;

	wd179x_set_drive(drive);

	logerror("disc w %04x %04x\n",offset,data);
}

static READ_HANDLER(serial_r)
{
	logerror("serial r %04x\n",offset);
	return 0x00;
}

static WRITE_HANDLER(serial_w)
{
	logerror("serial w %04x %04x\n",offset,data);
}

static WRITE_HANDLER(apf_wd179x_command_w)
{
	wd179x_command_w(offset,~data);
}

static WRITE_HANDLER(apf_wd179x_track_w)
{
	wd179x_track_w(offset,~data);
}

static WRITE_HANDLER(apf_wd179x_sector_w)
{
	wd179x_sector_w(offset,~data);
}

static WRITE_HANDLER(apf_wd179x_data_w)
{
	wd179x_data_w(offset,~data);
}

static READ_HANDLER(apf_wd179x_status_r)
{
	return ~wd179x_status_r(offset);
}

static READ_HANDLER(apf_wd179x_track_r)
{
	return ~wd179x_track_r(offset);
}

static READ_HANDLER(apf_wd179x_sector_r)
{
	return ~wd179x_sector_r(offset);
}

static READ_HANDLER(apf_wd179x_data_r)
{
	return wd179x_data_r(offset);
}

static MEMORY_READ_START( apf_imagination_readmem )
	{ 0x00000, 0x003ff, apf_video_r},
	{ 0x00400, 0x007ff, apf_video_r},
	{ 0x00800, 0x00bff, apf_video_r},
	{ 0x00c00, 0x00fff, apf_video_r},
	{ 0x01000, 0x01fff, apf_video_r},
	{ 0x01000, 0x013ff, apf_video_r},
	{ 0x01400, 0x017ff, apf_video_r},
	{ 0x01800, 0x01bff, apf_video_r},
	{ 0x01c00, 0x01fff, apf_video_r},
	{ 0x02000, 0x03fff, apf_pia_0_r},		
	{ 0x04000, 0x047ff, MRA_BANK1},
	{ 0x04800, 0x04fff, MRA_BANK2},
	{ 0x05000, 0x057ff, MRA_BANK3},
	{ 0x05800, 0x05fff, MRA_BANK4},
	{ 0x06000, 0x063ff, apf_pia_1_r},
	{ 0x06400, 0x064ff, serial_r},
	{ 0x06500, 0x06500, apf_wd179x_status_r},
	{ 0x06501, 0x06501, apf_wd179x_track_r},
	{ 0x06502, 0x06502, apf_wd179x_sector_r},
	{ 0x06503, 0x06503, apf_wd179x_data_r},
	{ 0x06800, 0x077ff, MRA_ROM},
	{ 0x07800, 0x07fff, MRA_NOP},
	{ 0x08000, 0x09fff, MRA_ROM},
	{ 0x0a000, 0x0dfff, MRA_RAM},
	{ 0x0e000, 0x0e7ff, MRA_BANK5},
	{ 0x0e800, 0x0efff, MRA_BANK6},
	{ 0x0f000, 0x0f7ff, MRA_BANK7},
	{ 0x0f800, 0x0ffff, MRA_BANK8},
MEMORY_END


static MEMORY_WRITE_START( apf_imagination_writemem )
	{ 0x00000, 0x003ff, apf_video_w},
	{ 0x00400, 0x007ff, apf_video_w},
	{ 0x00800, 0x00bff, apf_video_w},
	{ 0x00c00, 0x00fff, apf_video_w},
	{ 0x01000, 0x01fff, apf_video_w},
	{ 0x01000, 0x013ff, apf_video_w},
	{ 0x01400, 0x017ff, apf_video_w},
	{ 0x01800, 0x01bff, apf_video_w},
	{ 0x01c00, 0x01fff, apf_video_w},
	{ 0x02000, 0x02003, pia_0_w},
	{ 0x04000, 0x05fff, MWA_ROM},
	{ 0x06000, 0x063ff, apf_pia_1_w},
	{ 0x06400, 0x064ff, serial_w},
	{ 0x06500, 0x06500, apf_wd179x_command_w},
	{ 0x06501, 0x06501, apf_wd179x_track_w},
	{ 0x06502, 0x06502, apf_wd179x_sector_w},
	{ 0x06503, 0x06503, apf_wd179x_data_w},
	{ 0x06600, 0x06600, apf_dischw_w},
	{ 0x0a000, 0x0dfff, MWA_RAM},
	{ 0x0e000, 0x0ffff, MWA_ROM},
MEMORY_END

static MEMORY_READ_START(apf_m1000_readmem)
	{ 0x00000, 0x003ff, apf_video_r},
	{ 0x00400, 0x007ff, apf_video_r},
	{ 0x00800, 0x00bff, apf_video_r},
	{ 0x00c00, 0x00fff, apf_video_r},
	{ 0x01000, 0x01fff, apf_video_r},
	{ 0x01000, 0x013ff, apf_video_r},
	{ 0x01400, 0x017ff, apf_video_r},
	{ 0x01800, 0x01bff, apf_video_r},
	{ 0x01c00, 0x01fff, apf_video_r},
	{ 0x02000, 0x03fff, apf_pia_0_r},		
	{ 0x04000, 0x047ff, MRA_BANK1},
	{ 0x04800, 0x04fff, MRA_BANK2},
	{ 0x05000, 0x057ff, MRA_BANK3},
	{ 0x05800, 0x05fff, MRA_BANK4},
	{ 0x06800, 0x077ff, MRA_ROM},
	{ 0x08000, 0x09fff, MRA_ROM},
	{ 0x0a000, 0x0dfff, MRA_RAM},
	{ 0x0e000, 0x0e7ff, MRA_BANK5},
	{ 0x0e800, 0x0efff, MRA_BANK6},
	{ 0x0f000, 0x0f7ff, MRA_BANK7},
	{ 0x0f800, 0x0ffff, MRA_BANK8},
MEMORY_END

static MEMORY_WRITE_START( apf_m1000_writemem )
	{ 0x00000, 0x003ff, apf_video_w},
	{ 0x00400, 0x007ff, apf_video_w},
	{ 0x00800, 0x00bff, apf_video_w},
	{ 0x00c00, 0x00fff, apf_video_w},
	{ 0x01000, 0x01fff, apf_video_w},
	{ 0x01000, 0x013ff, apf_video_w},
	{ 0x01400, 0x017ff, apf_video_w},
	{ 0x01800, 0x01bff, apf_video_w},
	{ 0x01c00, 0x01fff, apf_video_w},
	{ 0x02000, 0x03fff, apf_pia_0_w},		
	{ 0x04000, 0x05fff, MWA_ROM},
	{ 0x0a000, 0x0dfff, MWA_RAM},
	{ 0x0e000, 0x0ffff, MWA_ROM},
MEMORY_END

INPUT_PORTS_START( apf_m1000)
	/* line 0 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "w", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "e", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "r", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "t", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "u", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "i", KEYCODE_I, IP_JOY_NONE)

	/* line 1 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "o", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "p", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "a", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "s", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "d", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "g", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "h", KEYCODE_H, IP_JOY_NONE)

	/* line 2 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "j", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "l", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "x", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "c", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "v", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b", KEYCODE_B, IP_JOY_NONE)

	/* line 3 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "m", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
INPUT_PORTS_END


INPUT_PORTS_START( apf_imagination)
	/* temp assignments , these are the keypad */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "q", KEYCODE_0_PAD, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "w", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "e", KEYCODE_2_PAD, IP_JOY_NONE)
    PORT_BIT (0x0f8, 0xf8, IPT_UNUSED)

	PORT_START
    PORT_BIT (0x0ff, 0xff, IPT_UNUSED)

	PORT_START
    PORT_BIT (0x0ff, 0xff, IPT_UNUSED)

	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE)
    PORT_BIT (0x0fe, 0xfe, IPT_UNUSED)

	/* assignments with ** are known to be correct */
	/* keyboard line 0 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)/**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)/**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)/**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)/**/
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)/**/
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)/**/
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) /**/
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) /**/

	/* keyboard line 1 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) /**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) /**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)/**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)/**/
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)/**/
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)/**/
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)/**/
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) /**/

	/* keyboard line 2 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)/**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)/**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)/**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE) /**/
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE) /**/
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)/**/
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)/**/
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)/**/

	/* keyboard line 3 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) /**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) /**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)/**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)/**/
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)/**/
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)/**/
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)/**/
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)/**/

	/* keyboard line 4 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) /**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) /**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) /**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE) /**/
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) /**/
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) /**/
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) /**/
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE) /**/

	/* keyboard line 5 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE) /**/ 
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE) /**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE) /**/
	PORT_BIT (0x08, 0x08, IPT_UNUSED)
	PORT_BIT (0x10, 0x10, IPT_UNUSED) 
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE) /**/
	PORT_BIT (0x40, 0x40, IPT_UNUSED) 
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE) /**/

	/* line 6 */
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE) /**/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE) /**/
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE) /**/
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "FUNCTION?", KEYCODE_LCONTROL, IP_JOY_NONE) /**/
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "REPEAT", KEYCODE_TAB, IP_JOY_NONE) /**/

	PORT_BITX( 0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_3_PAD, IP_JOY_NONE) 
	PORT_BITX( 0x020, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_4_PAD, IP_JOY_NONE) 
	PORT_BITX( 0x040, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_5_PAD, IP_JOY_NONE) /* same as X */
	PORT_BITX( 0x080, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_6_PAD, IP_JOY_NONE) /* same as Z */

	/* line 7 */
	PORT_START
	PORT_BITX( 0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_7_PAD, IP_JOY_NONE) 
	PORT_BITX( 0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_8_PAD, IP_JOY_NONE) 
	PORT_BITX( 0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_9_PAD, IP_JOY_NONE) 

	/*
	PORT_BIT (0x01, 0x01, IPT_UNUSED)
	PORT_BIT (0x02, 0x02, IPT_UNUSED)
	PORT_BIT (0x04, 0x04, IPT_UNUSED) */
	PORT_BIT (0x08, 0x08, IPT_UNUSED)
	PORT_BIT (0x10, 0x10, IPT_UNUSED)
	PORT_BIT (0x20, 0x20, IPT_UNUSED)
	PORT_BIT (0x40, 0x40, IPT_UNUSED)
	PORT_BIT (0x80, 0x80, IPT_UNUSED)

INPUT_PORTS_END


/* sound output */

static	struct	Speaker_interface apf_sh_interface =
{
	1,
	{ 100 },
	{ 0 },
	{ NULL }
};


static MACHINE_DRIVER_START( apf_imagination )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6800, 3750000)        /* 7.8336 Mhz */
	MDRV_CPU_MEMORY(apf_imagination_readmem,apf_imagination_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_INIT( apf_imagination )

	/* video hardware */
	MDRV_M6847_NTSC( apf )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, apf_sh_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apf_m1000 )
	MDRV_IMPORT_FROM( apf_imagination )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( apf_m1000_readmem,apf_m1000_writemem )
	MDRV_MACHINE_INIT( apf_m1000 )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apfimag)
	ROM_REGION(0x10000+0x0800,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x00800, 0x2a331a33)
	ROM_LOAD("basic_68.rom",0x06800, 0x01000, 0xef049ab8)
	ROM_LOAD("basic_80.rom",0x08000, 0x02000, 0xa4c69fae)
ROM_END

ROM_START(apfm1000)
	ROM_REGION(0x10000+0x0800,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x0800, 0x2a331a33)
ROM_END

SYSTEM_CONFIG_START( apfimag )
	CONFIG_DEVICE_CASSETTE			(1, "apt\0", device_load_apf_cassette)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(2, "apd\0", device_load_apfimag_floppy)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR	NAME		PARENT	COMPAT	MACHINE				INPUT				INIT    CONFIG		COMPANY               FULLNAME */
COMPX(1977, apfimag,	0,		0,		apf_imagination,	apf_imagination,	0,		apfimag,	"APF Electronics Inc",  "APF Imagination Machine" ,GAME_NOT_WORKING)
COMPX(1978,	apfm1000,	0,		0,		apf_m1000,			apf_m1000,			0,		NULL,		"APF Electronics Inc",  "APF M-1000" ,GAME_NOT_WORKING)
