#include <stdio.h>
#include "pdp1/pdp1.h"
#include "vidhrdw/pdp1.h"
#include "driver.h"

/*
 * osd_fread (romfile, pdp1_memory, 16384);
 *
 * is supposed to read 4096 integers!
 * endianess!!!
 *
 * File load functions don't support reading anything but bytes!
 *
 * dirty hack,... see below
 * never did any endianess, the mac and amiga sources... probably
 * must change something in the load routine
 * (perhaps just leave the conversion out)
 *
 */

int pdp1_iot(int *io, int md);
int pdp1_load_rom (void);
int pdp1_id_rom (const char *name, const char *gamename);
void pdp1_plot(int x, int y);

int *pdp1_memory;

int pdp1_load_rom (void)
{
	FILE *romfile;
	int region;
	int i;

	/* The spacewar! is mandatory for now. */
	if (!(romfile = osd_fopen (Machine->gamedrv->name, "spacewar.bin",OSD_FILETYPE_ROM_CART, 0)))
	{
		if (errorlog) fprintf(errorlog,"PDP1: can't find SPACEWAR.BIN\n");
		return 1;
	}

	if (strlen(rom_name[0])==0)
	{
		if (errorlog)
			fprintf(errorlog,"PDP1: no file specified (doesn't matter, not used anyway)!\n");
	}
	else
	/* if (!(cartfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_ROM_CART, 0))) */
	{
		if (errorlog)
			fprintf(errorlog,"PDP1: file specified, but ignored");
	}

	/* Allocate memory and set up memory regions */
	for (region = 0;region < MAX_MEMORY_REGIONS;region++)
		Machine->memory_region[region] = 0;

	ROM = (unsigned char *) malloc(0x10000*sizeof(int));
 /* only 4096 are used for now, but pdp1 can address 65336 18 bit words when
  * extended.
  */
	if (ROM == NULL)
	{
		if (errorlog)
		fprintf(errorlog,"PDP1: Memory allocation failed!\n");
		return 1;
	}
	Machine->memory_region[0] = ROM;

	/* endianness!!! */
	osd_fread (romfile, ROM, 16384);
	osd_fclose (romfile);

	pdp1_memory = (int *) ROM; /* really bad! */
#ifdef LSB_FIRST
	/* for PC (INTEL, AMD...) endianness */
	for (i=0;i<16384;i+=4)
	{
		unsigned char temp;
		temp=ROM[i];
		ROM[i]=ROM[i+3];
		ROM[i+3]=temp;

		temp=ROM[i+1];
		ROM[i+1]=ROM[i+2];
		ROM[i+2]=temp;
	}
#endif

	return 0;
}

int pdp1_id_rom (const char *name, const char *gamename)
{
	/* This driver doesn't ID images yet */
	return 0;
}
static int setOPbasefunc(int i)
{
	/* just to get rid of the warnings */
	return -1;
}

void pdp1_init_machine(void)
{
	/* init pdp1 cpu */
	extern_iot=pdp1_iot;
	cpu_setOPbaseoverride(setOPbasefunc);
}

int pdp1_read_mem(int offset)
{
	return pdp1_memory[offset];
}

void pdp1_write_mem(int offset, int data)
{
	pdp1_memory[offset]=data;
}
/* these are the key-bits specified in driver\pdp1.c */
#define FIRE_PLAYER2              128
#define THRUST_PLAYER2            64
#define ROTATE_RIGHT_PLAYER2      32
#define ROTATE_LEFT_PLAYER2       16
#define FIRE_PLAYER1              8
#define THRUST_PLAYER1            4
#define ROTATE_RIGHT_PLAYER1      2
#define ROTATE_LEFT_PLAYER1       1

/* time in micro seconds, (*io_register) */
int pdp1_iot(int *io, int md)
{
 pdp1_Regs Regs;
 int etime=0;
 pdp1_GetRegs(&Regs);
 switch (Regs.y&077)
 {
  case 00:
  {
   /* Waiting for output to finnish */
   etime=5;
   break;
  }
  case 01: /* RPA */
  {
  /*
   * This instruction reads one line of tape (all eight Channels) and transfers the resulting 8-bit code to
   * the Reader Buffer. If bits 5 and 6 of the rpa function are both zero (720001), the contents of the
   * Reader Buffer must be transferred to the IO Register by executing the rrb instruction. When the Reader
   * Buffer has information ready to be transferred to the IO Register, Status Register Bit 1 is set to
   * one. If bits 5 and 6 are different (730001 or 724001) the 8-bit code read from tape is automatically
   * transferred to the IO Register via the Reader Buffer and appears as follows:
   *
   * IO Bits        10 11 12 13 14 15 16 17
   * TAPE CHANNELS  8  7  6  5  4  3  2  1
   */
   int read_byte=0;
   if (errorlog)
    fprintf(errorlog,"\nWarning, RPA instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,Regs.y,Regs.pc);

   etime=10; /* probably some more */
   /* somehow read a byte... */

   /* ... */

   reader_buffer=read_byte;
   if (read_byte==0)
   {
    Regs.flag[1]=0;
    pdp1_SetRegs(&Regs);
    break;
   }
   else
    Regs.flag[1]=1;
   pdp1_SetRegs(&Regs);
   if (!((md>>11)&3))
   {
    ;/* work thru rrb */
   }
   if (((md>>11)&1)!=((md>>12)&1))
   {
    /* code to IO */
    *io=0;
    *io|=(reader_buffer&128)<<9;
    *io|=(reader_buffer&64)<<10;
    *io|=(reader_buffer&32)<<11;
    *io|=(reader_buffer&16)<<12;
    *io|=(reader_buffer&8)<<13;
    *io|=(reader_buffer&4)<<14;
    *io|=(reader_buffer&2)<<15;
    *io|=(reader_buffer&1)<<16;
   }
   break;
  }
  case 04: /* TYI */
  {
  /* When a typewriter key is struck, the code for the struck key is placed in the
   * typewriter buffer, Program Flag 1 is set, and the type-in status bit is set to
   * one. A program designed to accept typed-in data would periodically check
   * Program Flag 1, and if found to be set, an In-Out Transfer Instruction with
   * address 4 could be executed for the information to be transferred to the
   * In-Out Register. This In-Out Transfer should not use the optional in-out wait.
   * The information contained in the typewriter buffer is then transferred to the
   * right six bits of the In-Out Register. The tyi instruction automatically
   * clears the In-Out Register before transferring the information and also clears
   * the type-in status bit.
   */

   if (errorlog)
    fprintf(errorlog,"\nWarning, TYI instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,Regs.y,Regs.pc);

   etime=10; /* probably heaps more */
   *io=0;
   *io=fio_dec&077;
   fio_dec=0;
   concise=0;

   break;
  }
  case 07: /* DPY */
  {
   /*
    * PRECISION CRT DISPLAY (TYPE 30)
    *
    * This sixteen-inch cathode ray tube display is intended to be used as an on-line output device for the
    * PDP-1. It is useful for high speed presentation of graphs, diagrams, drawings, and alphanumerical
    * information. The unit is solid state, self-powered and completely buffered. It has magnetic focus and
    * deflection.
    *
    * Display characteristics are as follows:
    *
    *     Random point plotting
    *     Accuracy of points +/-3 per cent of raster size
    *     Raster size 9.25 by 9.25 inches
    *     1024 by 1024 addressable locations
    *     Fixed origin at center of CRT
    *     Ones complement binary arithmetic
    *     Plots 20,000 points per second
    *
    * Resolution is such that 512 points along each axis are discernable on the face of the tube.
    *
    * One instruction is added to the PDP-1 with the installation of this display:
    *
    * Display One Point On CRT
    * dpy Address 0007
    *
    * This instruction clears the light pen status bit and displays one point using bits 0 through 9 of the
    * AC to represent the (signed) X coordinate of the point and bits 0 through 9 of the IO as the (signed)
    * Y coordinate.
    *
    * Many variations of the Type 30 Display are available. Some of these include hardware for line and
    * curve generation.
    */
   int x;
   int y;
   etime=10; /* probably some more */
   x=(Regs.ac+0400000)&0777777;
   y=(Regs.io+0400000)&0777777;
   pdp1_plot(x,y);
   break;
  }
  case 011: /* IOT 011 (undocumented?), Input... */
  {
   int key_state=readinputport(0);
   etime=10; /* probably heaps more */
   *io=0;
   if (key_state&FIRE_PLAYER2)         *io |= 040000;
   if (key_state&THRUST_PLAYER2)       *io |= 0100000;
   if (key_state&ROTATE_LEFT_PLAYER2)  *io |= 0200000;
   if (key_state&ROTATE_RIGHT_PLAYER2) *io |= 0400000;
   if (key_state&FIRE_PLAYER1)         *io |= 01;
   if (key_state&THRUST_PLAYER1)       *io |= 02;
   if (key_state&ROTATE_LEFT_PLAYER1)  *io |= 04;
   if (key_state&ROTATE_RIGHT_PLAYER1) *io |= 010;
   break;
  }
  case 030: /* RRB */
  {
   if (errorlog)
    fprintf(errorlog,"\nWarning, RRB instruction not fully emulated: io=0%06o, y=0%06o, pc=0%06o",*io,Regs.y,Regs.pc);
   etime=5;
   Regs.flag[1]=0;
   pdp1_SetRegs(&Regs);
   *io=0;
   *io|=(reader_buffer&128)<<9;
   *io|=(reader_buffer&64)<<10;
   *io|=(reader_buffer&32)<<11;
   *io|=(reader_buffer&16)<<12;
   *io|=(reader_buffer&8)<<13;
   *io|=(reader_buffer&4)<<14;
   *io|=(reader_buffer&2)<<15;
   *io|=(reader_buffer&1)<<16;
   break;
  }
  case 033: /* CKS */
  {
   etime=5;
  }
  default:
  {
   etime=5;
   if (errorlog)
   {
    fprintf(errorlog,"\nNot supported IOT command: io=0%06o, y=0%06o, pc=0%06o",*io,Regs.y,Regs.pc);
   }
   break;
  }
 }
 return etime;
}
