/******************************************************************************
 *	Sharp MZ700
 *
 *	machine driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *****************************************************************************/

#include "includes/mz700.h"
#include "devices/cassette.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(), (const char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

typedef UINT32 data_t;

static void *ne556_timer[2] = {NULL,};	  /* NE556 timer */
static UINT8 ne556_out[2] = {0,};		/* NE556 current output status */

static UINT8 mz700_motor_ff = 0;    /* cassette motor control flipflop */
static UINT8 mz700_motor_on = 0;	/* cassette motor key (play key) */

static READ_HANDLER ( pio_port_a_r );
static READ_HANDLER ( pio_port_b_r );
static READ_HANDLER ( pio_port_c_r );
static WRITE_HANDLER ( pio_port_a_w );
static WRITE_HANDLER ( pio_port_b_w );
static WRITE_HANDLER ( pio_port_c_w );

static ppi8255_interface ppi8255 = {
    1,
	{pio_port_a_r},
	{pio_port_b_r},
	{pio_port_c_r},
	{pio_port_a_w},
	{pio_port_b_w},
	{pio_port_c_w}
};

static int pio_port_a_output;
static int pio_port_c_output;

static void pit_clk_0(double clock);
static void pit_clk_1(double clock);
static void pit_irq_2(int which);

PIT8253_CONFIG pit8253 = {
	TYPE8253,
	{
		/* clockin	  irq callback	 clock change callback */
		{ 1108800.0,  NULL, 		 pit_clk_0	 },
		{	15611.0,  NULL, 		 pit_clk_1	 },
		{		1.0,  pit_irq_2,	 NULL		 },
	}
};

INTERRUPT_GEN(mz700_interrupt)
{
	if (readinputport(12) & 0x20)
	{
		mz700_motor_on = 0;
		device_status(image_from_devtype_and_index(IO_CASSETTE, 0), mz700_motor_ff && mz700_motor_on);
	}

	if (readinputport(12) & 0x40)
	{
		mz700_motor_on = 1;
		device_status(image_from_devtype_and_index(IO_CASSETTE, 0), mz700_motor_ff && mz700_motor_on);
	}


	if (readinputport(12) & 0x80)
		device_seek(image_from_devtype_and_index(IO_CASSETTE, 0), 0, SEEK_SET);
}

static void ne556_callback(int param)
{
	/* toggle the NE556 output signal */
	ne556_out[param] ^= 1;
}

void init_mz700(void)
{
	videoram=memory_region(REGION_CPU1)+0x12000;videoram_size=0x800;
	colorram=memory_region(REGION_CPU1)+0x12800;
    mz700_bank_w(4, 0);
}

MACHINE_INIT(mz700)
{
    ppi8255_init(&ppi8255);
    pit8253_config(0, &pit8253);
	ne556_timer[0] = timer_alloc(ne556_callback);
	timer_adjust(ne556_timer[0], TIME_IN_HZ(1.5), 0, TIME_IN_HZ(1.5));
	/*timer_pulse(TIME_IN_HZ(1.5), 0, ne556_callback)*/
	ne556_timer[1] = timer_alloc(ne556_callback);
	timer_adjust(ne556_timer[1], TIME_IN_HZ(34.5), 1, TIME_IN_HZ(34.5));
	/*timer_pulse(TIME_IN_HZ(34.5), 1, ne556_callback)*/
}

MACHINE_STOP(mz700)
{
	if (ne556_timer[0])
		timer_remove(ne556_timer[0]);
	ne556_timer[0] = NULL;
	if (ne556_timer[1])
		timer_remove(ne556_timer[1]);
	ne556_timer[1] = NULL;
}

/************************ PIT ************************************************/

/* timer 0 is the clock for the speaker output */
static void pit_clk_0(double clockout)
{
	beep_set_state(0, 1);
    beep_set_frequency(0, clockout);
}

/* timer 1 is the clock for timer 2 clock input */
static void pit_clk_1(double clockout)
{
	pit8253_set_clockin(0, 2, clockout);
}

/* timer 2 is the AM/PM (12 hour) interrupt */
static void pit_irq_2(int which)
{
	/* INTMSK: interrupt enabled? */
    if (pio_port_c_output & 0x04)
		cpu_set_irq_line(0, 0, HOLD_LINE);
}

/************************ PIO ************************************************/

static READ_HANDLER ( pio_port_a_r )
{
	data8_t data = pio_port_a_output;
	LOG(2,"mz700_pio_port_a_r",("%02X\n", data));
	return data;
}

/* read keyboard row - indexed by a demux LS145 which is connected to PA0-3 */
static READ_HANDLER ( pio_port_b_r )
{
	data8_t demux_LS145, data = 0xff;

	if( setup_active() || onscrd_active() )
		return data;

    demux_LS145 = pio_port_a_output & 15;
    data = readinputport(1 + demux_LS145);
	LOG(2,"mz700_pio_port_b_r",("%02X\n", data));

    return data;
}

static READ_HANDLER (pio_port_c_r )
{
    data8_t data = pio_port_c_output & 0x0f;

    /*
     * bit 7 in     vertical blank
     * bit 6 in     NE556 output
     * bit 5 in     tape data (RDATA)
     * bit 4 in     motor (1 = on)
     */
    if (mz700_motor_on)
        data |= 0x10;

    if (device_input(image_from_devtype_and_index(IO_CASSETTE,0)) > 255)
        data |= 0x20;       /* set the RDATA status */

	if (ne556_out[0])
        data |= 0x40;           /* set the 556OUT status */

    data |= readinputport(0);   /* get VBLANK in bit 7 */

	LOG(2,"mz700_pio_port_c_r",("%02X\n", data));

    return data;
}

static WRITE_HANDLER (pio_port_a_w )
{
	LOG(2,"mz700_pio_port_a_w",("%02X\n", data));
	pio_port_a_output = data;
}

static WRITE_HANDLER ( pio_port_b_w )
{
	/*
	 * bit 7	NE556 reset
	 * bit 6	unused
	 * bit 5	unused
	 * bit 4	unused
	 * bit 3	demux LS145 D
	 * bit 2	demux LS145 C
	 * bit 1	demux LS145 B
	 * bit 0	demux LS145 A
     */
	LOG(2,"mz700_pio_port_b_w",("%02X\n", data));

	/* enable/disable NE556 cursor flash timer */
    timer_enable(ne556_timer[0], (data & 0x80) ? 0 : 1);
}

static WRITE_HANDLER ( pio_port_c_w )
{
    /*
     * bit 3 out    motor control (0 = on)
     * bit 2 out    INTMSK
     * bit 1 out    tape data (WDATA)
     * bit 0 out    unused
     */
	LOG(2,"mz700_pio_port_c_w",("%02X\n", data));
	pio_port_c_output = data;

	mz700_motor_ff = (data & 0x08) ? 1 : 0;
	device_status(image_from_devtype_and_index(IO_CASSETTE, 0), mz700_motor_ff & mz700_motor_on);

    device_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x02) ? 32767 : -32768);
}

/************************ MMIO ***********************************************/

READ_HANDLER ( mz700_mmio_r )
{
	data8_t data = 0x7e;

	switch (offset & 15)
	{
	/* the first four ports are connected to a 8255 PPI */
    case 0: case 1: case 2: case 3:
		data = ppi8255_r(0, offset & 3);
		break;

	case 4: case 5: case 6: case 7:
		data = pit8253_0_r(offset & 3);
        break;

	case 8:
		data = ne556_out[1] ? 0x01 : 0x00;
		data |= readinputport(12);	/* get joystick ports */
		if (cpu_gethorzbeampos() >= Machine->visible_area.max_x - 32)
			data |= 0x80;
		LOG(1,"mz700_e008_r",("%02X\n", data));
        break;
    }
    return data;
}

WRITE_HANDLER ( mz700_mmio_w )
{
	switch (offset & 15)
	{
	/* the first four ports are connected to a 8255 PPI */
    case 0: case 1: case 2: case 3:
		ppi8255_w(0, offset & 3, data);
        break;

	/* the next four ports are connected to a 8253 PIT */
    case 4: case 5: case 6: case 7:
		pit8253_0_w(offset & 3, data);
		break;

	case 8:
		LOG(1,"mz700_e008_w",("%02X\n", data));
		pit8253_0_gate_w(0, data & 1);
        break;
	}
}

/************************ BANK ***********************************************/

/* BANK1 0000-0FFF */
static void bank1_RAM(UINT8 *mem)
{
	cpu_setbank(1, &mem[0x00000]);
	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_w(1, 0, MWA_BANK1);
}

static void bank1_NOP(UINT8 *mem)
{
    cpu_setbank(1, &mem[0x00000]);
	memory_set_bankhandler_r(1, 0, MRA_NOP);
	memory_set_bankhandler_w(1, 0, MWA_NOP);
}

static void bank1_ROM(UINT8 *mem)
{
	cpu_setbank(1, &mem[0x10000]);
	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_w(1, 0, MWA_ROM);
}


/* BANK2 1000-1FFF */
static void bank2_RAM(UINT8 *mem)
{
	cpu_setbank(2, &mem[0x01000]);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_w(2, 0, MWA_BANK2);
}

static void bank2_ROM(UINT8 *mem)
{
	cpu_setbank(2, &mem[0x11000]);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_w(2, 0, MWA_ROM);
}


/* BANK3 8000-9FFF */
static void bank3_RAM(UINT8 *mem)
{
	cpu_setbank(3, &mem[0x08000]);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_w(3, 0, MWA_BANK3);
}

static void bank3_VID(UINT8 *mem)
{
	cpu_setbank(3, &mem[0x12000]);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_w(3, 0, videoram0_w);
}


/* BANK4 A000-BFFF */
static void bank4_RAM(UINT8 *mem)
{
	cpu_setbank(4, &mem[0x0a000]);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);
	memory_set_bankhandler_w(4, 0, MWA_BANK4);
}

static void bank4_VID(UINT8 *mem)
{
	cpu_setbank(4, &mem[0x14000]);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);
	memory_set_bankhandler_w(4, 0, videoram2_w);
}


/* BANK7 C000-CFFF */
static void bank5_RAM(UINT8 *mem)
{
	cpu_setbank(5, &mem[0x0c000]);
	memory_set_bankhandler_r(5, 0, MRA_BANK5);
	memory_set_bankhandler_w(5, 0, MWA_BANK5);
}

static void bank5_VID(UINT8 *mem)
{
	cpu_setbank(5, &mem[0x11000]);
	memory_set_bankhandler_r(5, 0, MRA_BANK5);
	memory_set_bankhandler_w(5, 0, pcgram_w);
}


/* BANK6 D000-D7FF */
static void bank6_NOP(UINT8 *mem)
{
	cpu_setbank(6, &mem[0x0d000]);
	memory_set_bankhandler_r(6, 0, MRA_NOP);
	memory_set_bankhandler_w(6, 0, MWA_NOP);
}

static void bank6_RAM(UINT8 *mem)
{
	cpu_setbank(6, &mem[0x0d000]);
	memory_set_bankhandler_r(6, 0, MRA_BANK6);
	memory_set_bankhandler_w(6, 0, MWA_BANK6);
}

static void bank6_VIO(UINT8 *mem)
{
	cpu_setbank(6, &mem[0x12000]);
	memory_set_bankhandler_r(6, 0, MRA_BANK6);
	memory_set_bankhandler_w(6, 0, videoram_w);
}


/* BANK9 D800-DFFF */
static void bank7_NOP(UINT8 *mem)
{
	cpu_setbank(7, &mem[0x0d800]);
	memory_set_bankhandler_r(7, 0, MRA_NOP);
	memory_set_bankhandler_w(7, 0, MWA_NOP);
}

static void bank7_RAM(UINT8 *mem)
{
	cpu_setbank(7, &mem[0x0d800]);
	memory_set_bankhandler_r(7, 0, MRA_BANK7);
	memory_set_bankhandler_w(7, 0, MWA_BANK7);
}

static void bank7_VIO(UINT8 *mem)
{
	cpu_setbank(7, &mem[0x12800]);
	memory_set_bankhandler_r(7, 0, MRA_BANK7);
	memory_set_bankhandler_w(7, 0, colorram_w);
}


/* BANK8 E000-FFFF */
static void bank8_NOP(UINT8 *mem)
{
	cpu_setbank(8, &mem[0x0e000]);
	memory_set_bankhandler_r(8, 0, MRA_NOP);
	memory_set_bankhandler_w(8, 0, MWA_NOP);

}

static void bank8_RAM(UINT8 *mem)
{
	cpu_setbank(8, &mem[0x0e000]);
	memory_set_bankhandler_r(8, 0, MRA_BANK8);
	memory_set_bankhandler_w(8, 0, MWA_BANK8);
}

static void bank8_VIO(UINT8 *mem)
{
	cpu_setbank(8, &mem[0x16000]);
	memory_set_bankhandler_r(8, 0, mz700_mmio_r);
	memory_set_bankhandler_w(8, 0, mz700_mmio_w);
}


WRITE_HANDLER ( mz700_bank_w )
{
    static int mz700_locked = 0;
	static int vio_mode = 0;
	static int vio_lock = 0;
	UINT8 *mem = memory_region(REGION_CPU1);

    switch (offset)
	{
	case 0: /* 0000-0FFF RAM */
		LOG(1,"mz700_bank_w",("0: 0000-0FFF RAM\n"));
		bank1_RAM(mem);
		mz700_locked = 0;
        break;

	case 1: /* D000-FFFF RAM */
		LOG(1,"mz700_bank_w",("1: D000-FFFF RAM\n"));
		bank6_RAM(mem);
		bank7_RAM(mem);
		bank8_RAM(mem);
        mz700_locked = 0;
		vio_mode = 1;
        break;

	case 2: /* 0000-0FFF ROM */
		LOG(1,"mz700_bank_w",("2: 0000-0FFF ROM\n"));
		bank1_ROM(mem);
		mz700_locked = 0;
        break;

	case 3: /* D000-FFFF videoram, memory mapped io */
		LOG(1,"mz700_bank_w",("3: D000-FFFF videoram, memory mapped io\n"));
		bank6_VIO(mem);
		bank7_VIO(mem);
		bank8_VIO(mem);
		mz700_locked = 0;
		vio_mode = 3;
        break;

	case 4: /* 0000-0FFF ROM	D000-FFFF videoram, memory mapped io */
		LOG(1,"mz700_bank_w",("4: 0000-0FFF ROM; D000-FFFF videoram, memory mapped io\n"));
		bank1_ROM(mem);
		bank6_VIO(mem);
		bank7_VIO(mem);
		bank8_VIO(mem);
        mz700_locked = 0;
		vio_mode = 3;
        break;

	case 5: /* 0000-0FFF no chg D000-FFFF locked */
		LOG(1,"mz700_bank_w",("5: D000-FFFF locked\n"));
		if (mz700_locked == 0)
		{
			vio_lock = vio_mode;
			mz700_locked = 1;
			bank6_NOP(mem);
			bank7_NOP(mem);
			bank8_NOP(mem);
		}
        break;

	case 6: /* 0000-0FFF no chg D000-FFFF unlocked */
		LOG(1,"mz700_bank_w",("6: D000-FFFF unlocked\n"));
		if (mz700_locked == 1)
			mz700_bank_w(vio_lock, 0); /* old config for D000-DFFF */
        break;
    }
}

/************************ CASSETTE *******************************************/

#define LO  -32768
#define HI	+32767

#define SHORT_PULSE 2
#define LONG_PULSE	4

#define BYTE_SAMPLES (LONG_PULSE+8*LONG_PULSE)

#define SILENCE 	8000

/* long gap and tape mark */
#define LGAP        22000
#define LTM_1		40
#define LTM_0		40
#define LTM_L		1

/* short gap and tape mark */
#define SGAP        11000
#define STM_1		20
#define STM_0		20
#define STM_L		1

static int fill_wave_1(INT16 *buffer, int offs)
{
	buffer[offs++] = HI;
	buffer[offs++] = HI;
	buffer[offs++] = LO;
    buffer[offs++] = LO;
    return LONG_PULSE;
}

static int fill_wave_0(INT16 *buffer, int offs)
{
	buffer[offs++] = HI;
	buffer[offs++] = LO;
	return SHORT_PULSE;
}

static int fill_wave_b(INT16 *buffer, int offs, int byte)
{
    int i, count = 0;

	/* data bits are preceded by a long pulse */
    count += fill_wave_1(buffer, offs + count);

    for( i = 7; i >= 0; i-- )
	{
		if( (byte >> i) & 1 )
			count += fill_wave_1(buffer, offs + count);
		else
			count += fill_wave_0(buffer, offs + count);
	}
	return count;
}

static int fill_wave(INT16 *buffer, int length, UINT8 *code)
{
	static INT16 *beg;
    static UINT16 csum = 0;
	static int header = 1, bytecount = 0;
	int count = 0;

    if( code == CODE_HEADER )
    {
		int i;

		/* is there insufficient space for the LGAP? */
		if( count + LGAP * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700 fill_wave",("LGAP %d samples\n", LGAP * SHORT_PULSE));
        /* fill long gap */
		for( i = 0; i < LGAP; i++ )
			count += fill_wave_0(buffer, count);

        /* make a long tape mark */
		/* is there insufficient space for the LTM 1? */
		if( count + LTM_1 * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("LTM 1 %d samples\n", LTM_1 * LONG_PULSE));
		for( i = 0; i < LTM_1; i++ )
			count += fill_wave_1(buffer, count);

		/* is there insufficient space for the LTM 0? */
		if( count + LTM_0 * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("LTM 0 %d samples\n", LTM_0 * SHORT_PULSE));
		for( i = 0; i < LTM_0; i++ )
			count += fill_wave_0(buffer, count);

		/* is there insufficient space for the L? */
		if( count + LTM_L * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L %d samples\n", LONG_PULSE));
        count += fill_wave_1(buffer, count);

		/* reset header, bytecount and checksum */
        header = 1;
        bytecount = 0;
        csum = 0;

		/* HDR begins here */
        beg = buffer + count;

        return count;
    }

	if( code == CODE_TRAILER )
    {
		int i, file_length;
		INT16 *end = buffer;

		/* is there insufficient space for the CHKF? */
		if( count + 2 * BYTE_SAMPLES > length )
            return -1;
		LOG(1,"mz700_fill_wave",("CHKF 0x%04X\n", csum));
		count += fill_wave_b(buffer, count, csum >> 8);
		count += fill_wave_b(buffer, count, csum & 0xff);

        /* is there insufficient space for the L */
		if( count + LTM_L * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L\n"));
        count += fill_wave_1(buffer, count);

		/* is there insufficient space for the 256S pulses? */
		if( count + 256 * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("256S\n"));
        for (i = 0; i < 256; i++)
			count += fill_wave_0(buffer, count);

		file_length = (int)(end - beg) / sizeof(INT16);
		/* is there insufficient space for the FILEC ? */
		if( count + file_length > length )
            return -1;
		LOG(1,"mz700_fill_wave",("FILEC %d samples\n", file_length));
		memcpy(buffer + count, beg, file_length * sizeof(INT16));
		count += file_length;

		/* is there insufficient space for the CHKF ? */
		if( count + 2 * BYTE_SAMPLES > length )
            return -1;
		LOG(1,"mz700_fill_wave",("CHKF 0x%04X\n", csum));
		count += fill_wave_b(buffer, count, csum >> 8);
		count += fill_wave_b(buffer, count, csum & 0xff);

		/* is there insufficient space for the L ? */
		if( count + STM_L * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L %d samples\n", LONG_PULSE));
        count += fill_wave_1(buffer, count);

		LOG(1,"mz700_fill_wave",("silence %d samples\n", SILENCE));
        /* silence at the end */
		for( i = 0; i < SILENCE; i++ )
			buffer[count++] = 0;

		return count;
    }

	if( header == 1 && bytecount == 128 )
	{
		int i, hdr_length;
		INT16 *end = buffer;

		/* is there insufficient space for the CHKH ? */
		if( count + 2 * BYTE_SAMPLES > length )
            return -1;
		LOG(1,"mz700_fill_wave",("CHKH 0x%04X\n", csum & 0xffff));
		count += fill_wave_b(buffer, count, (csum >> 8) & 0xff);
		count += fill_wave_b(buffer, count, csum & 0xff);

		/* is there insufficient space for the L ? */
		if( count + LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L %d samples\n", LONG_PULSE));
        count += fill_wave_1(buffer, count);

		/* is there insufficient space for the 256S ? */
		if( count + 256 * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("256S\n"));
        for (i = 0; i < 256; i++)
			count += fill_wave_0(buffer, count);

		hdr_length = (int)(end - beg) / sizeof(INT16);
		/* is there insufficient space for the HDRC ? */
		if( count + hdr_length > length )
            return -1;
		LOG(1,"mz700_fill_wave",("HDRC %d samples\n", hdr_length));
		memcpy(buffer + count, beg, hdr_length * sizeof(INT16));
		count += hdr_length;

		/* is there insufficient space for CHKH ? */
		if( count + 2 * BYTE_SAMPLES > length )
            return -1;
		LOG(1,"mz700_fill_wave",("CHKH 0x%04X\n", csum & 0xffff));
		count += fill_wave_b(buffer, count, (csum >> 8) & 0xff);
		count += fill_wave_b(buffer, count, csum & 0xff);

		/* is there insufficient space for the L ? */
		if( count + LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L %d samples\n", LONG_PULSE));
        count += fill_wave_1(buffer, count);

		/* is there sufficient space for the SILENCE? */
		if( count + SILENCE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("SILENCE %d samples\n", SILENCE));
		/* fill silence */
		for( i = 0; i < SILENCE; i++ )
			buffer[count++] = 0;

        /* is there sufficient space for the SGAP? */
		if( count + SGAP * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("SGAP %d samples\n", SGAP * SHORT_PULSE));
		/* fill short gap */
		for( i = 0; i < SGAP; i++ )
			count += fill_wave_0(buffer, count);

        /* make a short tape mark */

        /* is there sufficient space for the STM 1? */
		if( count + STM_1 * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("STM 1 %d samples\n", STM_1 * LONG_PULSE));
		for( i = 0; i < STM_1; i++ )
			count += fill_wave_1(buffer, count);

		/* is there sufficient space for the STM 0? */
		if( count + STM_0 * SHORT_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("STM 0 %d samples\n", STM_0 * SHORT_PULSE));
		for( i = 0; i < STM_0; i++ )
			count += fill_wave_0(buffer, count);

		/* is there sufficient space for the L? */
		if( count + STM_L * LONG_PULSE > length )
            return -1;
		LOG(1,"mz700_fill_wave",("L %d samples\n", LONG_PULSE));
        count += fill_wave_1(buffer, count);

		bytecount = 0;
		header = 0;
		csum = 0;

		/* FILE begins here */
        beg = buffer + count;
	}

	if( length < BYTE_SAMPLES )
        return -1;

    if (*code & 0x01) csum++;
    if (*code & 0x02) csum++;
    if (*code & 0x04) csum++;
    if (*code & 0x08) csum++;
    if (*code & 0x10) csum++;
    if (*code & 0x20) csum++;
    if (*code & 0x40) csum++;
    if (*code & 0x80) csum++;

    bytecount++;

	count += fill_wave_b(buffer, count, *code);

    return count;
}

#define MZ700_WAVESAMPLES_HEADER	(	\
		LGAP * SHORT_PULSE +			\
		LTM_1 * LONG_PULSE +			\
		LTM_0 * SHORT_PULSE +			\
		LTM_L * LONG_PULSE +			\
		2 * 2 * BYTE_SAMPLES +			\
	SILENCE +							\
	SGAP * SHORT_PULSE +				\
		STM_1 * LONG_PULSE +			\
		STM_0 * SHORT_PULSE +			\
		STM_L * LONG_PULSE +			\
		2 * 2 * BYTE_SAMPLES)


static struct cassette_args mz700_cassette_args =
{
	0,												/* initial_status */
	fill_wave,										/* fill_wave */
	NULL,											/* calc_chunk_info */
	5120,											/* input_smpfreq */
	MZ700_WAVESAMPLES_HEADER,						/* header_samples */
	1,												/* trailer_samples */
	1,												/* chunk_size */
	2 * BYTE_SAMPLES,								/* chunk_samples */
	0												/* create_smpfreq */
};

DEVICE_LOAD( mz700_cassette )
{
	return cassette_init(image, file, &mz700_cassette_args);
}

/******************************************************************************
 *	Sharp MZ800
 *
 *
 ******************************************************************************/

static UINT16 mz800_ramaddr = 0;
static UINT8 mz800_display_mode = 0;
static UINT8 mz800_port_e8 = 0;
static UINT8 mz800_palette[4];
static UINT8 mz800_palette_bank;

/* port CE */
READ_HANDLER( mz800_crtc_r )
{
	data8_t data = 0x00;
	LOG(1,"mz800_crtc_r",("%02X\n",data));
    return data;
}

/* port D0 - D7 / memory E000 - FFFF */
READ_HANDLER( mz800_mmio_r )
{
	data8_t data = 0x7e;

	switch (offset & 15)
	{
	/* the first four ports are connected to a 8255 PPI */
    case 0: case 1: case 2: case 3:
		data = ppi8255_r(0, offset & 3);
		break;

	case 4: case 5: case 6: case 7:
		data = pit8253_0_r(offset & 3);
        break;

	default:
		data = memory_region(REGION_CPU1)[0x16000 + offset];
        break;
    }
    return data;
}

/* port E0 - E9 */
READ_HANDLER( mz800_bank_r )
{
	UINT8 *mem = memory_region(REGION_CPU1);
    data8_t data = 0xff;

    switch (offset)
    {
	case 0: /* 1000-1FFF PCG ROM */
		LOG(1,"mz800_bank_r",("0: 1000-1FFF PCG ROM"));
        bank2_ROM(mem);
		if ((mz800_display_mode & 0x08) == 0)
		{
			LOG(1,0,("; 8000-9FFF videoram"));
            bank3_VID(mem);
			if (mz800_display_mode & 0x04)
			{
				LOG(1,0,("; A000-BFFF videoram"));
                /* 640x480 mode so A000-BFFF is videoram too */
				bank4_VID(mem);
            }
			else
			{
				LOG(1,0,("; A000-BFFF RAM"));
				bank4_RAM(mem);
            }
		}
		else
		{
			LOG(1,0,("; C000-CFFF PCG RAM"));
            /* make C000-CFFF PCG RAM */
			bank5_RAM(mem);
        }
		LOG(1,0,("\n"));
        break;

    case 1: /* make 1000-1FFF and C000-CFFF RAM */
		LOG(1,"mz800_bank_r",("1: 1000-1FFF RAM"));
        bank2_RAM(mem);
		if ((mz800_display_mode & 0x08) == 0)
		{
			LOG(1,0,("; 8000-9FFF RAM; A000-BFFF RAM"));
            /* make 8000-BFFF RAM */
            bank3_RAM(mem);
			bank4_RAM(mem);
		}
		else
		{
			LOG(1,0,("; C000-CFFF RAM"));
            /* make C000-CFFF RAM */
			bank5_RAM(mem);
        }
		LOG(1,0,("\n"));
        break;

    case 8: /* get MZ700 enable bit 7 ? */
		data = mz800_port_e8;
		break;
    }
	return data;
}

/* port EA */
READ_HANDLER( mz800_ramdisk_r )
{
	UINT8 *mem = memory_region(REGION_USER1);
	data8_t data = mem[mz800_ramaddr];
	LOG(2,"mz800_ramdisk_r",("[%04X] -> %02X\n", mz800_ramaddr, data));
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_r",("address wrap 0000\n"));
    return data;
}

/* port CC */
WRITE_HANDLER( mz800_write_format_w )
{
	LOG(1,"mz800_write_format_w",("%02X\n", data));
}

/* port CD */
WRITE_HANDLER( mz800_read_format_w )
{
	LOG(1,"mz800_read_format_w",("%02X\n", data));
}

/* port CE
 * bit 3	1: MZ700 mode		0: MZ800 mode
 * bit 2	1: 640 horizontal	0: 320 horizontal
 * bit 1	1: 4bpp/2bpp		0: 2bpp/1bpp
 * bit 0	???
 */
WRITE_HANDLER( mz800_display_mode_w )
{
	UINT8 *mem = memory_region(REGION_CPU1);
	LOG(1,"mz800_display_mode_w",("%02X\n", data));
    mz800_display_mode = data;
	if ((mz800_display_mode & 0x08) == 0)
	{
		bank8_RAM(mem);
	}
}

/* port CF */
WRITE_HANDLER( mz800_scroll_border_w )
{
	LOG(1,"mz800_scroll_border_w",("%02X\n", data));
}

/* port D0-D7 */
WRITE_HANDLER( mz800_mmio_w )
{
	/* just wrap to the mz700 handler */
    mz700_mmio_w(offset,data);
}

/* port E0-E9 */
WRITE_HANDLER ( mz800_bank_w )
{
    static int mz800_locked = 0;
    static int vio_mode = 0;
    static int vio_lock = 0;
    UINT8 *mem = memory_region(REGION_CPU1);

    switch (offset)
    {
    case 0: /* 0000-0FFF RAM */
		LOG(1,"mz800_bank_w",("0: 0000-0FFF RAM\n"));
        bank1_RAM(mem);
        mz800_locked = 0;
        break;

    case 1: /* D000-FFFF RAM */
		LOG(1,"mz800_bank_w",("1: D000-FFFF RAM\n"));
		bank6_RAM(mem);
		bank7_RAM(mem);
		bank8_RAM(mem);
        mz800_locked = 0;
        vio_mode = 1;
        break;

    case 2: /* 0000-0FFF ROM */
		LOG(1,"mz800_bank_w",("2: 0000-0FFF ROM\n"));
        bank1_ROM(mem);
        mz800_locked = 0;
        break;

    case 3: /* D000-FFFF videoram, memory mapped io */
		LOG(1,"mz800_bank_w",("3: D000-FFFF videoram, memory mapped io\n"));
		bank6_VIO(mem);
		bank7_VIO(mem);
		bank8_VIO(mem);
        mz800_locked = 0;
        vio_mode = 3;
        break;

    case 4: /* 0000-0FFF ROM    D000-FFFF videoram, memory mapped io */
		LOG(1,"mz800_bank_w",("4: 0000-0FFF ROM; D000-FFFF videoram, memory mapped io\n"));
        bank1_ROM(mem);
		bank6_VIO(mem);
		bank7_VIO(mem);
		bank8_VIO(mem);
        mz800_locked = 0;
        vio_mode = 3;
        break;

    case 5: /* 0000-0FFF no chg D000-FFFF locked */
		LOG(1,"mz800_bank_w",("5: D000-FFFF locked\n"));
        if (mz800_locked == 0)
        {
            vio_lock = vio_mode;
            mz800_locked = 1;
			bank6_NOP(mem);
			bank7_NOP(mem);
			bank8_NOP(mem);
        }
        break;

    case 6: /* 0000-0FFF no chg D000-FFFF unlocked */
		LOG(1,"mz800_bank_w",("6: D000-FFFF unlocked\n"));
        if (mz800_locked == 1)
            mz800_bank_w(vio_lock, 0); /* old config for D000-DFFF */
        break;

	case 8: /* set MZ700 enable bit 7 ? */
		mz800_port_e8 = data;
		if (mz800_port_e8 & 0x80)
		{
			bank6_VIO(mem);
			bank7_VIO(mem);
			bank8_VIO(mem);
		}
		else
		{
			bank6_RAM(mem);
			bank7_RAM(mem);
			bank8_RAM(mem);
        }
        break;
    }
}

/* port EA */
WRITE_HANDLER( mz800_ramdisk_w )
{
	UINT8 *mem = memory_region(REGION_USER1);
	LOG(2,"mz800_ramdisk_w",("[%04X] <- %02X\n", mz800_ramaddr, data));
	mem[mz800_ramaddr] = data;
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_w",("address wrap 0000\n"));
}

/* port EB */
WRITE_HANDLER( mz800_ramaddr_w )
{
	mz800_ramaddr = (cpunum_get_reg(0, Z80_BC) & 0xff00) | (data & 0xff);
	LOG(1,"mz800_ramaddr_w",("%04X\n", mz800_ramaddr));
}

/* port F0 */
WRITE_HANDLER( mz800_palette_w )
{
	if (data & 0x40)
	{
        mz800_palette_bank = data & 3;
		LOG(1,"mz800_palette_w",("bank: %d\n", mz800_palette_bank));
    }
	else
	{
		int idx = (data >> 4) & 3;
		int val = data & 15;
		LOG(1,"mz800_palette_w",("palette[%d] <- %d\n", idx, val));
		mz800_palette[idx] = val;
	}
}

/* videoram wrappers */
WRITE_HANDLER( videoram0_w ) { videoram_w(offset + 0x0000, data); }
WRITE_HANDLER( videoram1_w ) { videoram_w(offset + 0x1000, data); }
WRITE_HANDLER( videoram2_w ) { videoram_w(offset + 0x2000, data); }
WRITE_HANDLER( videoram3_w ) { videoram_w(offset + 0x3000, data); }
WRITE_HANDLER( pcgram_w ) { videoram_w(offset + 0x4000, data); }

void init_mz800(void)
{
	UINT8 *mem = memory_region(REGION_CPU1);

	videoram=memory_region(REGION_CPU1)+0x12000;videoram_size=0x5000;
	colorram=memory_region(REGION_CPU1)+0x12800;

    mem[0x10001] = 0x4a;
	mem[0x10002] = 0x00;
    memcpy(&mem[0x00000], &mem[0x10000], 0x02000);

    mem = memory_region(REGION_USER1);
	memset(&mem[0x00000], 0xff, 0x10000);

    mz800_display_mode_w(0,0x08);   /* set MZ700 mode */
	mz800_bank_r(1);
	mz800_bank_w(4, 0);
}


