/*
** mtx.c : driver for Memotech MTX512
**
**
**
*/

#include "driver.h"
#include "cpuintrf.h"
#include "vidhrdw/generic.h"
#include "machine/z80fmly.h"
#include "mess/vidhrdw/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"
#include "osdepend.h"

#include <stdio.h>

unsigned char key_sense;
static unsigned char *mtx_tapebuffer = NULL;

static unsigned char *mtx_ram = NULL;

#define MTX_SYSTEM_CLOCK		4000000

static struct SN76496interface mtx_psg_interface =
{
	1,
	{ MTX_SYSTEM_CLOCK },
	{ 100 }
};

static READ_HANDLER ( mtx_psg_r )
{
	return 0xff;
}

static WRITE_HANDLER ( mtx_psg_w )
{
        SN76496_0_w(offset,data);
}

static READ_HANDLER ( mtx_vdp_r )
{
	if (offset & 0x01)
		return TMS9928A_register_r();
	else
		return TMS9928A_vram_r();
}

static WRITE_HANDLER ( mtx_vdp_w )
{
	if (offset & 0x01)
		TMS9928A_register_w(data);
	else
		TMS9928A_vram_w(data);
}

static WRITE_HANDLER ( mtx_sense_w )
{
	key_sense = data;
}

static READ_HANDLER ( mtx_key_lo_r )
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

static READ_HANDLER ( mtx_key_hi_r )
{
	unsigned char rtn = 0;

	if (key_sense==0xfe)
		rtn = readinputport(8) & 0x03;

	if (key_sense==0xfd)
		rtn = (readinputport(8) >> 2) & 0x03;

	if (key_sense==0xfb)
		rtn = (readinputport(8) >> 4) & 0x03;

	if (key_sense==0xf7)
		rtn = (readinputport(8) >> 6) & 0x03;

	if (key_sense==0xef)
		rtn = readinputport(9) & 0x03;

	if (key_sense==0xdf)
		rtn = (readinputport(9) >> 2) & 0x03;

	if (key_sense==0xbf)
		rtn = (readinputport(9) >> 4) & 0x03;

	if (key_sense==0x7f)
		rtn = (readinputport(9) >> 6) & 0x03;

	return(rtn);
}

static void mtx_ctc_interrupt(int state)
{
	logerror("interrupting ctc %02x\r\n ",state);

         cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

static READ_HANDLER ( mtx_ctc_r )
{
	return z80ctc_0_r(offset);
}

static WRITE_HANDLER ( mtx_ctc_w )
{
        logerror("CTC W: %02x\r\n",data);

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

static WRITE_HANDLER ( mtx_bankswitch_w )
{

	unsigned char *romoffset;
	unsigned char relcpmh;
	unsigned char rampage;
	unsigned char rompage;
	unsigned int bank1, bank2, bank3, bank4, bank5;

	bank1 = 0;
	bank2 = 0;
	bank3 = 0;
	bank4 = 0;
	bank5 = 0;

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
			bank3=0;
			bank4=0x4000;
			bank5=0xc000;
			break;

		case 1:
			bank3=4 * 0x4000;
			bank4=2 * 0x4000;
			bank5=0xc000;
			break;

		case 2:
			bank3=5 * 0x4000;
			bank4=6 * 0x4000;
			bank5=0xc000;
			break;
		case 3:
			bank3=7 * 0x4000;
			bank4=8 * 0x4000;
			bank5=0xc000;
			break;
		case 4:
			bank3=9 * 0x4000;
			bank4=10 * 0x4000;
			bank5=0xc000;
			break;
		case 5:
			bank3=11 * 0x4000;
			bank4=12 * 0x4000;
			bank5=0xc000;
			break;
		case 6:
			bank3=13 * 0x4000;
			bank4=14 * 0x4000;
			bank5=0xc000;
			break;
		case 7:
			bank3=15 * 0x4000;
			bank4=16 * 0x4000;
			bank5=0xc000;
			break;
		case 8:
			bank3=17 * 0x4000;
			bank4=18 * 0x4000;
			bank5=0xc000;
			break;
		case 9:
			bank3=19 * 0x4000;
			bank4=20 * 0x4000;
			bank5=0xc000;
			break;
		case 10:
			bank3=21 * 0x4000;
			bank4=22 * 0x4000;
			bank5=0xc000;
			break;
		case 11:
			bank3=23 * 0x4000;
			bank4=24 * 0x4000;
			bank5=0xc000;
			break;
		case 12:
			bank3=25 * 0x4000;
			bank4=26 * 0x4000;
			bank5=0xc000;
			break;
		case 13:
			bank3=27 * 0x4000;
			bank4=28 * 0x4000;
			bank5=0xc000;
			break;
		case 14:
			bank3=29 * 0x4000;
			bank4=30 * 0x4000;
			bank5=0xc000;
			break;
		case 15:
			bank3=31 * 0x4000;
			bank4=32 * 0x4000;
			bank5=0xc000;
			break;
	}

	// bankswitcherooney type thing (tm)

	romoffset = memory_region(REGION_CPU1) + 0x10000 + bank1;
	cpu_setbank(1, romoffset);
	romoffset = memory_region(REGION_CPU1) + 0x10000 + bank2;
	cpu_setbank(2, romoffset);

	cpu_setbank(3, mtx_ram + bank3);
	cpu_setbank(8, mtx_ram + bank3);

	cpu_setbank(4, mtx_ram + bank4);
	cpu_setbank(9, mtx_ram + bank4);

	cpu_setbank(5, mtx_ram + bank5);
	cpu_setbank(10, mtx_ram + bank5);


}

static WRITE_HANDLER ( mtx_trap_write )
{
	int pc;

	pc = cpu_get_reg(Z80_PC);

        if((offset == 0x0aae) & (pc == 0x0ab1))
		{

			int start;
			int length;
			int filesize = 0;

			unsigned char *ramoffset;
			void *f;
			static char filename[32];

			start = cpu_get_reg(Z80_HL);
			length = cpu_get_reg(Z80_DE);

			logerror("PC %04x\nStart %04x, Length %04x, 0xFD67 %02x, 0xFD68 %02x\n", pc, start, length, mtx_ram[0xfd67], mtx_ram[0xfd68]);


			if(mtx_ram[0xfd68] == 0)
				{
				 //save
				}
				else
				{
                                                if(mtx_ram[0xfd67] == 0)
						{
								//load

							if((start == 0xc011) & (length == 0x12))
							{

								int i;
								for(i=0;i<=15;i++)
								{
									filename[i] = mtx_ram[0xc002 + i];
								}
								for(i=15; i>0 && filename[i] == 0x20;i--)
								filename[i+1] = '\0';
								if ((f = osd_fopen(Machine->gamedrv->name, filename,OSD_FILETYPE_IMAGE_R,0)) != 0)
									{
										filesize=osd_fsize(f);
										// check for buffer overflow....
										if(filesize<65536)
											{
												osd_fread(f,mtx_tapebuffer,filesize);
											}
												osd_fclose(f);
									}
							}
						if(start<=0xbfff)
							{
								ramoffset = &mtx_ram[start - 0x4000];
							}
							else
							{
								ramoffset = &mtx_ram[start];
							}
						if(filesize<65536)
							{
								memcpy(ramoffset, mtx_tapebuffer, length);
								memcpy(mtx_tapebuffer, mtx_tapebuffer + length, 0x10000 - length);
							}
						}
						else
						{
							//verify
						}
				}
		}
}

void mtx_init_machine(void)
{

	unsigned char *romoffset;
	mtx_ram = (unsigned char *)malloc(512*1024);
	if(!mtx_ram) return;
	memset(mtx_ram, 0, 512*1024);

	mtx_tapebuffer = (unsigned char *)malloc(65536);
	if(!mtx_tapebuffer) return;
	memset(mtx_tapebuffer, 0, 65536);

	z80ctc_init(&mtx_ctc_intf);

	cpu_setbankhandler_r(1, MRA_BANK1);
	cpu_setbankhandler_r(2, MRA_BANK2);
	cpu_setbankhandler_r(3, MRA_BANK3);
	cpu_setbankhandler_r(4, MRA_BANK4);
	cpu_setbankhandler_r(5, MRA_BANK5);

	cpu_setbankhandler_w(6, mtx_trap_write);
	cpu_setbankhandler_w(7, MWA_NOP);
	cpu_setbankhandler_w(8, MWA_BANK8);
	cpu_setbankhandler_w(9, MWA_BANK9);
	cpu_setbankhandler_w(10, MWA_BANK10);

	// set up memory configuration

	romoffset = memory_region(REGION_CPU1) + 0x10000;

	//Patch the Rom (Sneaky........ Who needs to trap opcodes?)
	romoffset[0x0aae] = 0x32;	// ld ($0aae),a
	romoffset[0x0aaf] = 0xae;
	romoffset[0x0ab0] = 0x0a;
	romoffset[0x0ab1] = 0xc9;	// ret
	cpu_setbank(1, romoffset);
	romoffset = memory_region(REGION_CPU1) + 0x12000;
	cpu_setbank(2, romoffset);

	cpu_setbank(3, mtx_ram);
	cpu_setbank(8, mtx_ram);

	cpu_setbank(4, mtx_ram + 0x4000);
	cpu_setbank(9, mtx_ram + 0x4000);

	cpu_setbank(5, mtx_ram + 0xc000);
	cpu_setbank(10, mtx_ram + 0xc000);



}

void mtx_exit_machine(void)
{
	if(mtx_ram!=NULL)
	{
		free(mtx_ram);
		free(mtx_tapebuffer);
	}

}

int mtx_vh_init(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);
}

static int mtx_interrupt(void)
{
	TMS9928A_interrupt();
	return ignore_interrupt();
}

static struct MemoryReadAddress mtx_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_BANK1 },
	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x7fff, MRA_BANK3 },
	{ 0x8000, 0xbfff, MRA_BANK4 },
	{ 0xc000, 0xffff, MRA_BANK5 },
	{ -1 }
};

static struct MemoryWriteAddress mtx_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_BANK6 },
	{ 0x2000, 0x3fff, MWA_BANK7 },
	{ 0x4000, 0x7fff, MWA_BANK8 },
	{ 0x8000, 0xbfff, MWA_BANK9 },
	{ 0xc000, 0xffff, MWA_BANK10 },
	{ -1 }
};

static struct IOReadPort mtx_readport[] =
{
	{ 0x01, 0x02, mtx_vdp_r },
	{ 0x03, 0x03, mtx_psg_r },
	{ 0x05, 0x05, mtx_key_lo_r },
	{ 0x06, 0x06, mtx_key_hi_r },
	{ 0x08, 0x0b, mtx_ctc_r },
	{ -1 }
};

static struct IOWritePort mtx_writeport[] =
{
	{ 0x00, 0x00, mtx_bankswitch_w },
	{ 0x01, 0x02, mtx_vdp_w },
	{ 0x05, 0x05, mtx_sense_w },
	{ 0x06, 0x06, mtx_psg_w },
	{ 0x08, 0x0a, mtx_ctc_w },
	{ -1 }
};

INPUT_PORTS_START( mtx512 )
 PORT_START /* 0 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 '", KEYCODE_7, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 )", KEYCODE_9, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "- =", KEYCODE_MINUS, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\ |", KEYCODE_BACKSLASH, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD7 PAGE )", KEYCODE_7_PAD, IP_JOY_NONE)

 PORT_START /* 1 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESCAPE", KEYCODE_ESC, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 \"", KEYCODE_2, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $", KEYCODE_4, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 &", KEYCODE_6, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 (", KEYCODE_8, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 0", KEYCODE_0, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "^ ~", KEYCODE_EQUALS, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD8 EOL", KEYCODE_8_PAD, IP_JOY_NONE)

 PORT_START /* 2 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W", KEYCODE_W, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R", KEYCODE_R, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I", KEYCODE_I, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P", KEYCODE_P, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "[ {", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD5 CURSORUP", KEYCODE_5_PAD, IP_JOY_NONE)

 PORT_START /* 3 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E", KEYCODE_E, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T", KEYCODE_T, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U", KEYCODE_U, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O", KEYCODE_O, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "@ `", KEYCODE_OPENBRACE, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LINEFEED", KEYCODE_DOWN, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD1 CURSORLEFT", KEYCODE_1_PAD, IP_JOY_NONE)

 PORT_START /* 4 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ALPHALOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S", KEYCODE_S, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F", KEYCODE_F, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "; +", KEYCODE_COLON, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "] }", KEYCODE_BACKSLASH2, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD3 CURSORRIGHT", KEYCODE_3_PAD, IP_JOY_NONE)

 PORT_START /* 5 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D", KEYCODE_D, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G", KEYCODE_G, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ": *", KEYCODE_QUOTE, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD2 HOME", KEYCODE_2_PAD, IP_JOY_NONE)

 PORT_START /* 6 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFTSHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X", KEYCODE_X, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHTSHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD(.) CURSORDOWN", KEYCODE_DEL_PAD, IP_JOY_NONE)

 PORT_START /* 7 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z", KEYCODE_Z, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C", KEYCODE_C, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(.) >", KEYCODE_STOP, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "_ _", KEYCODE_MINUS_PAD, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD0 INS", KEYCODE_0_PAD, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPADENTER CLS", KEYCODE_ENTER_PAD, IP_JOY_NONE)

/* Hi Bits */

 PORT_START /* 8 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD9 BRK", KEYCODE_9_PAD, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BS", KEYCODE_BACKSPACE, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5", KEYCODE_F5, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD4 TAB", KEYCODE_4_PAD, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD6 DEL", KEYCODE_6_PAD, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6", KEYCODE_F6, IP_JOY_NONE)

 PORT_START /* 9 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(none)", IP_KEY_NONE, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "F7", KEYCODE_F7, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(none)", IP_KEY_NONE, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3", KEYCODE_F3, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(none)", IP_KEY_NONE, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F8", KEYCODE_F8, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE)

INPUT_PORTS_END

static Z80_DaisyChain mtx_daisy_chain[] =
{
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};

static struct GfxDecodeInfo mtx_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static struct MachineDriver machine_driver_mtx512 =
{
	/* BASIC HARDWARE */
	{
		{
			CPU_Z80,
			MTX_SYSTEM_CLOCK,
			mtx_readmem, mtx_writemem, mtx_readport, mtx_writeport,
                        mtx_interrupt, 1,
			0, 0,	/* every scanline */
                        mtx_daisy_chain

		}
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	mtx_init_machine,
	mtx_exit_machine,

	/* video hardware */

	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	mtx_gfxdecodeinfo,
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,
	mtx_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */

	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&mtx_psg_interface
		}
	}
};

ROM_START (mtx512)
	ROM_REGION (0x20000, REGION_CPU1)
	ROM_LOAD ("osrom", 0x10000, 0x2000, 0x9ca858cc)
	ROM_LOAD ("basicrom", 0x12000, 0x2000, 0x87b4e59c)
	ROM_LOAD ("assemrom", 0x14000, 0x2000, 0x9d7538c3)
ROM_END

static const struct IODevice io_mtx512[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",			/* file extensions */
		NULL,               /* private */
		NULL,				/* id */
		NULL,				/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL                /* output_chunk */
	},
	{ IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY          FULLNAME */
COMP( 1983, mtx512,   0,        mtx512,   mtx512,   0,        "Memotech Ltd.", "MTX 512" )
