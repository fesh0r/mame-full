/***************************************************************************
  machine.c
  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)
***************************************************************************/
#include "driver.h"
#include "machine/genesis.h"
#include "vidhrdw/genesis.h"
#include "cpu/z80/z80.h"
#include "zlib.h"

int z80running;
int port_a_io = 0;
int port_b_io = 0;

#define MRAM_SIZE	0x10000
#define SRAM_SIZE	0x10000

#define HALT		0
#define RESUME		1
static int genesis_isSMD(const unsigned char *,unsigned int);
static int genesis_isBIN(const unsigned char *,unsigned int);
int genesis_sharedram_size = 0x10000;
int genesis_soundram_size = 0x10000;

/*unsigned char *genesis_sharedram;*/
unsigned char genesis_sharedram[0x10000];
unsigned char *genesis_soundram;

static unsigned char *ROM;

MACHINE_INIT( genesis )
{
    genesis_soundram = memory_region(REGION_CPU2);
	if( !genesis_soundram )
	{
		logerror("REGION_CPU2 not initialized\n");
		return;
    }

    /* the following ensures that the Z80 begins without running away from 0 */
	/* 0x76 is just a forced 'halt' as soon as the CPU is initially run */
    genesis_soundram[0] = 0x76;
	genesis_soundram[0x38] = 0x76;
	cpu_setbank(1, &genesis_soundram[0]);
	cpu_setbank(2, &genesis_sharedram[0]);

	cpu_set_halt_line(1, ASSERT_LINE);

	z80running = 0;
	logerror("Machine init\n");
}

static int genesis_verify_cart(unsigned char *temp,unsigned int len)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* is this an SMD file..? */
	if (genesis_isSMD(&temp[0x200],len))
		retval = IMAGE_VERIFY_PASS;

	/* How about a BIN file..? */
	if ((retval == IMAGE_VERIFY_FAIL) && genesis_isBIN(&temp[0],len))
		retval = IMAGE_VERIFY_PASS;

	/* maybe a .md file? (rare) */
	if ((retval == IMAGE_VERIFY_FAIL) && (temp[0x080] == 'E') && (temp[0x081] == 'A') && (temp[0x082] == 'M' || temp[0x082] == 'G'))
		retval = IMAGE_VERIFY_PASS;

	if (retval == IMAGE_VERIFY_FAIL)
		logerror("Invalid Image!\n");

	return retval;
}

DEVICE_LOAD(genesis_cart)
{
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;

	logerror("ROM load/init regions\n");

    /* Allocate memory and set up memory regions */
	if (new_memory_region(REGION_CPU1, 0x405000,0))
	{
		logerror("new_memory_region failed!\n");
		return INIT_FAIL;
	}
	rawROM = memory_region(REGION_CPU1);
    ROM = rawROM /*+ 512 */;

    if (new_memory_region(REGION_CPU2, 0x10000,0))    /* Z80 region */
    {
        logerror("Memory allocation failed creating Z80 RAM region!\n");
        goto bad;
    }
	genesis_soundram = memory_region(REGION_CPU2);

    length = mame_fread(file, rawROM + 0x2000, 0x400200);
	logerror("image length = 0x%x\n", length);

	if (length < 1024 + 512)
		goto bad;						/* smallest known rom is 1.7K */

	if (genesis_verify_cart(&rawROM[0x2000],(unsigned)length) == IMAGE_VERIFY_FAIL)
		goto bad;

	if (genesis_isSMD(&rawROM[0x2200],(unsigned)length))	/* is this a SMD file..? */
	{

		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000 + 512;

		for (ptr = 0; ptr < (0x400000) / (8192); ptr += 2)
		{
			for (x = 0; x < 8192; x++)
			{
				*tmpROMnew++ = *(tmpROM + ((ptr + 1) * 8192) + x);
				*tmpROMnew++ = *(tmpROM + ((ptr + 0) * 8192) + x);
			}
		}

		relocate = 0;

	}
	else
	/* check if it's a MD file */
	if ((rawROM[0x2080] == 'E') &&
		(rawROM[0x2081] == 'A') &&
		(rawROM[0x2082] == 'M' || rawROM[0x2082] == 'G'))  /* is this a MD file..? */
	{

		tmpROMnew = malloc(length);
		secondhalf = &tmpROMnew[length >> 1];

		if (!tmpROMnew)
		{
			printf("Memory allocation failed reading roms!\n");
			goto bad;
		}

		memcpy(tmpROMnew, ROM + 0x2000, length);

		for (ptr = 0; ptr < length; ptr += 2)
		{

			ROM[ptr] = secondhalf[ptr >> 1];
			ROM[ptr + 1] = tmpROMnew[ptr >> 1];
		}
		free(tmpROMnew);
		relocate = 0;

	}
	else
	/* BIN it is, then */
	{
		relocate = 0x2000;
	}

	ROM = memory_region(REGION_CPU1);	/* 68000 ROM region */

	for (ptr = 0; ptr < 0x402000; ptr += 2)		/* mangle bytes for littleendian machines */
	{
#ifdef LSB_FIRST
		int temp = ROM[relocate + ptr];

		ROM[ptr] = ROM[relocate + ptr + 1];
		ROM[ptr + 1] = temp;
#else
		ROM[ptr] = ROM[relocate + ptr];
		ROM[ptr + 1] = ROM[relocate + ptr + 1];
#endif
	}

	return INIT_PASS;

bad:
	return INIT_FAIL;
}

/* code taken directly from GoodGEN by Cowering */

static int genesis_isfunkySMD(const unsigned char *buf,unsigned int len)
{

	/* aq quiz */
	if (!strncmp("UZ(-01  ", (const char *) &buf[0xf0], 8))
		return 1;

    /* Phelios USA redump */
	/* target earth */
	/* klax (namcot) */
	if (buf[0x2080] == ' ' && buf[0x0080] == 'S' && buf[0x2081] == 'E' && buf[0x0081] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("OL R-AEAL", (const char *) &buf[0xf0], 9))
		return 1;

    /* devilish Mahjonng Tower */
    if (!strncmp("optrEtranet", (const char *) &buf[0xf3], 11))
		return 1;

	/* golden axe 2 beta */
	if (buf[0x0100] == 0x3c && buf[0x0101] == 0 && buf[0x0102] == 0 && buf[0x0103] == 0x3c)
		return 1;

    /* omega race */
	if (!strncmp("OEARC   ", (const char *) &buf[0x90], 8))
		return 1;

    /* budokan beta */
	if ((len >= 0x6708+8) && !strncmp(" NTEBDKN", (const char *) &buf[0x6708], 8))
		return 1;

    /* cdx pro 1.8 bios */
	if (!strncmp("so fCXP", (const char *) &buf[0x2c0], 7))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("sio-Wyo ", (const char *) &buf[0x0090], 8))
		return 1;

    /* onslaught */
	if (!strncmp("SS  CAL ", (const char *) &buf[0x0088], 8))
		return 1;

    /* tram terror pirate */
	if ((len >= 0x3648 + 8) && !strncmp("SG NEPIE", (const char *) &buf[0x3648], 8))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x0007] == 0x1c && buf[0x0008] == 0x0a && buf[0x0009] == 0xb8 && buf[0x000a] == 0x0a)
		return 1;

    /*tetris pirate */
	if ((len >= 0x1cbe + 5) && !strncmp("@TTI>", (const char *) &buf[0x1cbe], 5))
		return 1;

	return 0;
}


/* code taken directly from GoodGEN by Cowering */
int genesis_isSMD(const unsigned char *buf,unsigned int len)
{
	if (buf[0x2080] == 'S' && buf[0x80] == 'E' && buf[0x2081] == 'G' && buf[0x81] == 'A')
		return 1;
	return genesis_isfunkySMD(buf,len);
}



static int genesis_isfunkyBIN(const unsigned char *buf,unsigned int len)
{
	/* all the special cases for crappy headered roms */
	/* aq quiz */
	if (!strncmp("QUIZ (G-3051", (const char *) &buf[0x1e0], 12))
		return 1;

	/* phelios USA redump */
	/* target earth */
	/* klax namcot */
	if (buf[0x0104] == 'A' && buf[0x0101] == 'S' && buf[0x0102] == 'E' && buf[0x0103] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("WORLD PRO-B", (const char *) &buf[0x1e0], 11))
		return 1;

    /* devlish mahj tower */
	if (!strncmp("DEVILISH MAH", (const char *) &buf[0x120], 12))
		return 1;

    /* golden axe 2 beta */
	if ((len >= 0xe40a+4) && !strncmp("SEGA", (const char *) &buf[0xe40a], 4))
		return 1;

    /* omega race */
	if (!strncmp(" OMEGA RAC", (const char *) &buf[0x120], 10))
		return 1;

    /* budokan beta */
	if ((len >= 0x4e18+8) && !strncmp("BUDOKAN.", (const char *) &buf[0x4e18], 8))
		return 1;

    /* cdx 1.8 bios */
	if ((len >= 0x588+8) && !strncmp(" CDX PRO", (const char *) &buf[0x588], 8))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("Ishido - ", (const char *) &buf[0x120], 9))
		return 1;

    /* onslaught */
	if (!strncmp("(C)ACLD 1991", (const char *) &buf[0x118], 12))
		return 1;

    /* trampoline terror pirate */
	if ((len >= 0x2c70+9) && !strncmp("DREAMWORK", (const char *) &buf[0x2c70], 9))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x000f] == 0x1c && buf[0x0010] == 0x00 && buf[0x0011] == 0x0a && buf[0x0012] == 0x5c)
		return 1;

    /* tetris pirate */
	if ((len >= 0x397f+6) && !strncmp("TETRIS", (const char *) &buf[0x397f], 6))
		return 1;

    return 0;
}

static int genesis_isBIN(const unsigned char *buf,unsigned int len)
{
	if (buf[0x0100] == 'S' && buf[0x0101] == 'E' && buf[0x0102] == 'G' && buf[0x0103] == 'A')
		return 1;
	return genesis_isfunkyBIN(buf,len);
}

/* code taken directly from GoodGEN by Cowering
 * same effect as code in genesis_load_rom() except for .smd
 * where (size % 16384) != 0
 */

static int genesis_smd2bin(unsigned char *inbuf, unsigned int len)
{
	unsigned long i, j, offset = 0;
	unsigned char *tbuf;

	if (len < 16384)
		return 0;
	tbuf = malloc(len + 32768);
	if (!tbuf)
		return 0;

	for (i = 0; i < len; i += 16384)
	{
		for (j = 0; j < 8192; j++)
		{
			tbuf[offset + (j << 1) + 1] = inbuf[i + j];
		}
		for (j = 8192; j < 16384; j++)
		{
			tbuf[offset + ((j - 8192) << 1)] = inbuf[i + j];
		}
		offset += 16384;
	}
	memcpy(inbuf, tbuf, len);
	free(tbuf);
	return 1;
}



static int genesis_md2bin(unsigned char *inbuf, unsigned int len)
{
	unsigned long i, j, offset = 0;
	unsigned char *tbuf;

	if (len < 16384)
		return 0;
	tbuf = malloc(len + 32768);
	if (tbuf)
	{
		j = len / 2;
		for (i = 0; i < j; i++)
		{
			tbuf[offset] = inbuf[j + i];
			tbuf[offset + 1] = inbuf[i];
			offset += 2;
		}
		memcpy(inbuf, tbuf, len);
		free(tbuf);
		return 1;
	}
	else
	{
		return 0;
	}
}



void genesis_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length < 1700)
		return;						/* smallest known working ROM */
	if ((length >= 0x2081 + 1700) && genesis_isSMD((unsigned char *) &data[0x200], length))
	{
		if (genesis_smd2bin((unsigned char *) &data[0x200], length - 0x200))
		{
			hash_compute(dest, &data[0x200], length - 0x200, functions);
		}
	}
	else if (genesis_isBIN(data, length))
	{
		hash_compute(dest, data, length, functions);
	}
	else if ((data[0x080] == 'E') && (data[0x081] == 'A') && (data[0x082] == 'M' || data[0x082] == 'G'))
	{
		if (genesis_md2bin((unsigned char *) data, length))
		{
			hash_compute(dest, data, length, functions);
		}
	}
}



void genesis_interrupt(void)
{
	static int inter = 0;

/*	inter ++; */
/*	if (inter > 223) inter = 0; */
/*	genesis_modify_display(inter); */
	if (inter == 0)
	{
/*		static int inter=0; */
/*		inter = (inter+1); */
/*		if (inter < 20) return -1; */
		if (vdp_v_interrupt /* && vdp_display_enable */ )
		{
			logerror("Interrupt\n");
			cpu_set_irq_line(0, 6, HOLD_LINE);
		}
		if (vdp_h_interrupt /* && vdp_display_enable */ )
		{
			logerror("H Interrupt\n");
			cpu_set_irq_line(0, 4, HOLD_LINE);
		}
/*
	else
	{
        printf("denied\n");
		cpu_set_irq_line(0, 4, HOLD_LINE);
	}
*/
	}
}

WRITE16_HANDLER(genesis_io_w)
{
	data &= ~mem_mask;
/*	logerror("genesis_io_w %x, %x\n", offset, data); */
	switch (offset)
	{
	case 1:							/* joystick port a IO bit set */
		/* logerror("port a set to %x\n", port_a_io); */
		port_a_io = data & 0xff;
		break;
	case 2:							/* joystick port b IO bit set */
		port_b_io = data & 0xff;
		break;
	case 4:
		/* logerror("port a dir set to %x\n", data & 0xff); */
		break;
	case 5:
		break;
	}
}
READ16_HANDLER(genesis_io_r)
{

	int returnval = 0x80;

/*	logerror("inputport 3 is %d\n", readinputport(3)); */

	switch (readinputport(4))

	{

	case 0:

		switch (memory_region(REGION_CPU1)[ACTUAL_BYTE_ADDRESS(0x1f0)])

		{

		case 'J':
			returnval = 0x00;
			break;

		case 'E':
			returnval = 0xc0;
			break;

		case 'U':
			returnval = 0x80;
			break;

		}

		break;

	case 1:							/* USA */
		returnval = 0x80;
		break;

	case 2:							/* Japan */
		returnval = 0x00;
		break;

	case 3:							/* Europe */
		returnval = 0xc0;
		break;

	}


/*	logerror("genesis_io_r %x\n", offset); */
	switch (offset)
	{
	case 0:
/*		logerror("coo!\n"); */
		return returnval;				/* was just NTSC, overseas (USA) no FDD, now auto */
		break;
	case 1:							/* joystick port a */
		/* Pin 7 is the select signal */
		if ( port_a_io & 0x40 )
			return readinputport(0);
		else
			return readinputport(1);
		break;
	case 2:							/* joystick port b */
		if ( port_b_io & 0x40 )
			return readinputport(2);
		else
			return readinputport(3);
		break;
	}
	return 0x00;
}

READ16_HANDLER(genesis_ctrl_r)
{
/*	int returnval; */

/*  logerror("genesis_ctrl_r %x\n", offset); */
	switch (offset)
	{
	case 0:							/* DRAM mode is write only */
		return 0xffff;
		break;
	case 0x80:						/* return Z80 CPU Function Stop Accessible or not */
		/* logerror("Returning z80 state\n"); */
		return (z80running ? 0x0100 : 0x0);
		/* docs comflict here, page 91 says 0 == z80 has access */
		/* page 76 says 0 means you can access the space */
		break;
	case 0x100:						/* Z80 CPU Reset - write only */
		return 0xffff;
		break;
	}
	return 0x00;

}

WRITE16_HANDLER(genesis_ctrl_w)
{
	data &= ~mem_mask;

/*	logerror("genesis_ctrl_w %x, %x\n", offset, data); */

	switch (offset)
	{
	case 0:							/* set DRAM mode... we have to ignore this for production cartridges */
		return;
		break;
	case 0x80:						/* Z80 BusReq */
		if (data == 0x100)
		{
			z80running = 0;
			cpu_set_halt_line(1, ASSERT_LINE);	/* halt Z80 */
			/* logerror("z80 stopped by 68k BusReq\n"); */
		}
		else
		{
			z80running = 1;
			cpu_setbank(1, &genesis_soundram[0]);

			cpu_set_halt_line(1, CLEAR_LINE);
			/* logerror("z80 started, BusReq ends\n"); */
		}
		return;
		break;
	case 0x100:						/* Z80 CPU Reset */
		if (data == 0x00)
		{
			cpu_set_halt_line(1, ASSERT_LINE);
			cpu_set_reset_line(1, PULSE_LINE);

			cpu_set_halt_line(1, ASSERT_LINE);
			/* logerror("z80 reset, ram is %p\n", &genesis_soundram[0]); */
			z80running = 0;
			return;
		}
		else
		{
			/* logerror("z80 out of reset\n"); */
		}
		return;

		break;
	}
}
