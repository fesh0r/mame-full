/***************************************************************************
  machine.c
  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)
***************************************************************************/
#include "driver.h"
#include "mess/machine/genesis.h"
#include "mess/vidhrdw/genesis.h"
#include "cpu/z80/z80.h"


int z80running;
int	port_a_io = 0;
int	port_b_io = 0;
#define MRAM_SIZE	0x10000
#define SRAM_SIZE	0x10000

#define HALT		0
#define RESUME		1
void genesis_modify_display(int);
int genesis_sharedram_size = 0x10000;
int genesis_soundram_size = 0x10000;
/*unsigned char *genesis_sharedram;*/
unsigned char genesis_sharedram[0x10000];
unsigned char *genesis_soundram;

static unsigned char *ROM;

void genesis_init_machine (void)
{
  //	genesis_soundram = malloc(0x10000);
	/* the following ensures that the Z80 begins without running away from 0 */
	/* 0x76 is just a forced 'halt' as soon as the CPU is initially run */
	genesis_soundram[0] = 0x76;
	genesis_soundram[0x38]=0x76;
   	cpu_setbank(1, &genesis_soundram[0]);
	cpu_setbank(2, &genesis_sharedram[0]);

	cpu_set_halt_line (1,ASSERT_LINE);

	z80running = 0;
	if (errorlog) fprintf (errorlog, "Machine init\n");
}


int genesis_load_rom (int id)
{
	FILE *romfile;
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;

if (errorlog) fprintf (errorlog, "ROM load/init regions\n");


	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		printf("Genesis Requires Cartridge!\n");
		return INIT_FAILED;
	}
	/* Allocate memory and set up memory regions */
	if( new_memory_region(REGION_CPU1,0x405000) )
	{
		if(errorlog) fprintf(errorlog,"new_memory_region failed!\n");
		return 1;
	}
	rawROM = memory_region(REGION_CPU1);
	ROM = rawROM /*+ 512*/;

	length = osd_fread (romfile, rawROM+0x2000, 0x400200);
	if (!length) return 1;

	if (errorlog) fprintf(errorlog, "image length = 0x%x", length);

	if ((rawROM[0x2008] == 0xaa)
	 && (rawROM[0x2009] == 0xbb)
	 && (rawROM[0x200a] == 0x06) )	/* is this a SMD file..? */
	{

		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000+512;

		for(ptr = 0; ptr < (0x400000) / (8192); ptr += 2)
		{
			for(x = 0; x < 8192; x++)
			{
				*tmpROMnew++ = *(tmpROM + ((ptr + 1) * 8192) + x);
				*tmpROMnew++ = *(tmpROM + ((ptr + 0) * 8192) + x);
		  	}
		}

		relocate = 0;

	}
	else  /* check if it's a MD file */
	if ((rawROM[0x2080] == 'E')
	 && (rawROM[0x2081] == 'A')
	 && (rawROM[0x2082] == 'M' || rawROM[0x2082] == 'G') )	/* is this a MD file..? */
	{

		tmpROMnew = malloc(length);
		secondhalf = &tmpROMnew[length >> 1];

		if (!tmpROMnew)
		{
			printf ("Memory allocation failed reading roms!\n");
	   			goto bad;
		}

		memcpy(tmpROMnew, ROM+0x2000, length);

		for(ptr = 0; ptr < length; ptr+=2)
		{

			ROM[ptr    ] = secondhalf[ptr >> 1];
			ROM[ptr + 1] =  tmpROMnew[ptr >> 1];
		}
		free(tmpROMnew);
		relocate = 0;

	}
	else /* BIN it is, then */
	{
	  	relocate = 0x2000;
	}

	ROM = memory_region(REGION_CPU1); /* 68000 ROM region */

	if (new_memory_region(REGION_CPU2,0x10000)) /* Z80 region */
	{
		printf ("Memory allocation failed creating Z80 RAM region!\n");
		goto bad;
	}


	genesis_soundram = memory_region(REGION_CPU2);


   	for (ptr = 0; ptr < 0x402000; ptr+=2) /* mangle bytes for littleendian machines */
	{
	#ifdef LSB_FIRST
		int temp   = ROM[relocate+ptr  ];
		ROM[ptr  ] = ROM[relocate+ptr+1];
		ROM[ptr+1] = temp;
        #else

		ROM[ptr  ] = ROM[relocate+ptr  ];
		ROM[ptr+1] = ROM[relocate+ptr+1];
	#endif

	}


	osd_fclose (romfile);
	return 0;
bad:
	osd_fclose (romfile);
	return 1;
}
int genesis_id_rom (int id)
{
	FILE *romfile;
	unsigned char temp[0x110];
	int retval = 0;

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0))) return 0;

	osd_fread (romfile, temp, 0x110);

	/* is this an SMD file..? */
	if ((temp[0x008] == 0xaa) && (temp[0x009] == 0xbb) && (temp[0x00a] == 0x06)) retval = 1;

	/* How about a BIN file..? */
	if ((temp[0x100] == 'S') && (temp[0x101] == 'E') && (temp[0x102] == 'G') && (temp[0x103] == 'A')) retval = 1;

	if ((temp[0x080] == 'E') && (temp[0x081] == 'A') && (temp[0x082] == 'M' || temp[0x082] == 'G')) retval = 1;

	osd_fclose (romfile);
	return retval;
}

int genesis_interrupt (void)
{
static int inter = 0;
//inter ++;
//if (inter > 223) inter = 0;
//genesis_modify_display(inter);
if (inter == 0)
{
//static int inter=0;
//	inter = (inter+1);
//	if (inter < 20) return -1;
	if (vdp_v_interrupt /*&& vdp_display_enable*/)
	{
		if (errorlog) fprintf(errorlog, "Interrupt\n");
		return 6;  /*Interrupt vector 6 is V interrupt, 4 is H interrupt and 2 is ext */
	}
	if (vdp_h_interrupt /*&& vdp_display_enable*/)
	{
  		if (errorlog) fprintf(errorlog, "H Interrupt\n");
		return 4;  /*Interrupt vector 6 is V interrupt, 4 is H interrupt and 2 is ext */
	}
/*	else
		return 4;*/
		/*printf("denied\n");*/
	return 0;
}
return 0;
}
WRITE_HANDLER ( genesis_io_w )
{
  	data = COMBINE_WORD(0, data);
  //	if (errorlog) fprintf(errorlog, "genesis_io_w %x, %x\n", offset, data);
  	switch (offset)
		{
			case 2: /* joystick port a IO bit set */
		  //	if (errorlog) fprintf(errorlog, "port a set to %x\n", port_a_io);
				port_a_io = data & 0xff;
				break;
			case 4: /* joystick port b IO bit set */
				port_b_io = data & 0xff;
				break;
			case 8:
		  //	 if (errorlog) fprintf(errorlog, "port a dir set to %x\n", data & 0xff);

				break;
			case 0x0a:
				break;
 		}
}
READ_HANDLER ( genesis_io_r )
{

	int returnval = 0x80;

   //	fprintf(errorlog, "inputport 3 is %d\n", readinputport(3));

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

		case 1:	/* USA */

			returnval = 0x80;

			break;

		case 2:	/* Japan */

			returnval = 0x00;

			break;

		case 3:	/* Europe */

			returnval = 0xc0;

			break;

	}


  //	if (errorlog) fprintf(errorlog, "genesis_io_r %x\n", offset);
	switch (offset)
		{
			case 0:
  				// if (errorlog) fprintf(errorlog,"coo!\n");
  				return returnval; /* was just NTSC, overseas (USA) no FDD, now auto */
				break;
			case 2: /* joystick port a */
				if (port_a_io == 0x00)
					return readinputport(1);
				else
					return readinputport(0);
				break;
			case 4: /* joystick port b */
				if (port_b_io == 0x00)
					return readinputport(3);
				else
					return readinputport(2);
				break;
 		}
 	return 0x00;
}
READ_HANDLER ( genesis_ctrl_r )
{
	//int returnval;
   //	if (errorlog) fprintf(errorlog, "genesis_ctrl_r %x\n", offset);
	switch (offset)
		{
			case 0:	 /* DRAM mode is write only */
				return 0xffff;
				break;
			case 0x100: /* return Z80 CPU Function Stop Accessible or not */
			 //	if (errorlog) fprintf(errorlog, "Returning z80 state\n");
				return (z80running ? 0x0100 : 0x0);
				/* docs comflict here, page 91 says 0 == z80 has access */
				/* page 76 says 0 means you can access the space */
				break;
			case 0x200: /* Z80 CPU Reset - write only */
				return 0xffff;
				break;
		}
		return 0x00;

}
WRITE_HANDLER ( genesis_ctrl_w )
{
  data = COMBINE_WORD(0, data);

  //	if (errorlog) fprintf(errorlog, "genesis_ctrl_w %x, %x\n", offset, data);

	switch (offset)
		{
			case 0: /* set DRAM mode... we have to ignore this for production cartridges */
				return;
				break;
			case 0x100: /* Z80 BusReq */
				if (data == 0x100)
					{
					  	z80running = 0;
						cpu_set_halt_line(1,ASSERT_LINE); /* halt Z80 */
				   //		if (errorlog) fprintf(errorlog, "z80 stopped by 68k BusReq\n");
					}
					else
					{
					  	z80running = 1;
						cpu_setbank(1, &genesis_soundram[0]);

						cpu_set_halt_line(1,CLEAR_LINE);
				   //		if (errorlog) fprintf(errorlog, "z80 started, BusReq ends\n");
					}
				return;
				break;
			case 0x200: /* Z80 CPU Reset */
				if (data == 0x00)
				{
					cpu_set_halt_line(1,ASSERT_LINE);
					cpu_set_reset_line(1,PULSE_LINE);

					cpu_set_halt_line(1,ASSERT_LINE);
				  //	if (errorlog) fprintf(errorlog, "z80 reset, ram is %p\n", &genesis_soundram[0]);
			   	  	z80running = 0;
				  	return;
				}
				else
				{
				 //  if (errorlog) fprintf(errorlog, "z80 out of reset\n");
				}
				return;

				break;
		}
}
#if 0
READ_HANDLER ( cartridge_ram_r )
{
/*  if (errorlog) fprintf(errorlog, "cartridge ram read.. %x\n", offset);*/
	return cartridge_ram[offset];
}
WRITE_HANDLER ( cartridge_ram_w )
{
/*  if (errorlog) fprintf(errorlog, "cartridge ram write.. %x to %x\n", data, offset);*/
	cartridge_ram[offset] = data;
}
#endif

