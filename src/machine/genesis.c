/***************************************************************************
  machine.c
  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)
***************************************************************************/
#include "driver.h"
#include "machine/genesis.h"
#include "vidhrdw/genesis.h"
#include "z80/z80.h"


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
void genesis_init_machine (void)
{
  //	genesis_soundram = malloc(0x10000);
	/* the following ensures that the Z80 begins without running away from 0 */
	/* 0x76 is just a forced 'halt' as soon as the CPU is initially run */
	genesis_soundram[0] = 0x76;
	genesis_soundram[0x38]=0x76;
   	cpu_setbank(1, &genesis_soundram[0]);
	cpu_setbank(2, &genesis_sharedram[0]);

	cpu_halt (1,HALT);
  	
	z80running = 0;
	if (errorlog) fprintf (errorlog, "Machine init\n");
}

					  
int genesis_load_rom (void)
{
	FILE *romfile;
	unsigned char *tmpROMnew, *tmpROM;
	int region;
	unsigned char *rawROM;
	int relocate;

	int ptr, x;

if (errorlog) fprintf (errorlog, "ROM load/init regsions\n");

     
	if (!(romfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_ROM_CART, 0))) return 1;
	/* Allocate memory and set up memory regions */
	for (region = 0;region < MAX_MEMORY_REGIONS;region++)
		Machine->memory_region[region] = 0;
   	rawROM = calloc (0x405000, 1);
	ROM = rawROM /*+ 512*/;
   /*	genesis_sharedram = calloc (MRAM_SIZE, 1);
	genesis_soundram = calloc (SRAM_SIZE, 1);  */
	
	if (!rawROM /*|| !genesis_sharedram || !genesis_soundram*/)
	{
		printf ("Memory allocation failed reading roms!\n");
		free (rawROM);
	   /*	free (genesis_sharedram);
		free (genesis_soundram); */
		goto bad;
	}

   	

	osd_fread (romfile, rawROM+0x2000, 0x400200);

	if ((rawROM[0x2008] == 0xaa)
	 && (rawROM[0x2009] == 0xbb)
	 && (rawROM[0x200a] == 0x06) )	/* is this an SMD file..? */
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
	else
	{
	  	relocate = 0x2000;
	}

	Machine->memory_region[0] = ROM;
	Machine->memory_region[1] = malloc(0x10000);
	genesis_soundram = Machine->memory_region[1];
	 
  	
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
int genesis_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
	unsigned char temp[0x110];
	int retval = 0;
	
	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_ROM_CART, 0))) return 0;

	osd_fread (romfile, temp, 0x110);
	
	/* is this an SMD file..? */
	if ((temp[0x008] == 0xaa) && (temp[0x009] == 0xbb) && (temp[0x00a] == 0x06)) retval = 1;

	/* How about a BIN file..? */
	if ((temp[0x100] == 'S') && (temp[0x101] == 'E') && (temp[0x102] == 'G') && (temp[0x103] == 'A')) retval = 1;

	osd_fclose (romfile);
	return retval;
}

int genesis_interrupt (void)
{
static int inter = 0;
//inter ++;
if (inter > 223) inter = 0;
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
	return -1;
}
return -1;
}
void genesis_io_w (int offset, int data)
{
  	data = COMBINE_WORD(0, data);
	if (errorlog) fprintf(errorlog, "genesis_io_w %x, %x\n", offset, data);
  	switch (offset)
		{
			case 2: /* joystick port a IO bit set */
			if (errorlog) fprintf(errorlog, "port a set to %x\n", port_a_io);
				port_a_io = data & 0xff;
				break;
			case 4: /* joystick port b IO bit set */
				port_b_io = data & 0xff;
				break;
			case 8:
			 if (errorlog) fprintf(errorlog, "port a dir set to %x\n", data & 0xff);

				break;
			case 0x0a:
				break;
 		}
}
int genesis_io_r (int offset)
{

	int returnval = 0x80;

   //	fprintf(errorlog, "inputport 3 is %d\n", readinputport(3));

	switch (readinputport(4))

	{

		case 0:

			switch (Machine->memory_region[0][ACTUAL_BYTE_ADDRESS(0x1f0)])

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




	if (errorlog) fprintf(errorlog, "genesis_io_r %x\n", offset);
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
int genesis_ctrl_r (int offset)
{
	//int returnval;
	if (errorlog) fprintf(errorlog, "genesis_ctrl_r %x\n", offset);
	switch (offset)
		{
			case 0:	 /* DRAM mode is write only */
				return 0xffff;
				break;
			case 0x100: /* return Z80 CPU Function Stop Accessible or not */
				if (errorlog) fprintf(errorlog, "Returning z80 state\n");
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
void genesis_ctrl_w (int offset, int data)
{
  data = COMBINE_WORD(0, data);
 
	if (errorlog) fprintf(errorlog, "genesis_ctrl_w %x, %x\n", offset, data);

	switch (offset)
		{
			case 0: /* set DRAM mode... we have to ignore this for production cartridges */
				return;
				break;
			case 0x100: /* Z80 BusReq */
				if (data == 0x100)
					{
					  	z80running = 0;
					 	cpu_halt(1,HALT); /* halt Z80 */
						if (errorlog) fprintf(errorlog, "z80 stopped by 68k BusReq\n");
					}
					else
					{
					  	z80running = 1;
						cpu_setbank(1, &genesis_soundram[0]);

					 	cpu_halt(1,RESUME);  
						if (errorlog) fprintf(errorlog, "z80 started, BusReq ends\n");
					}
				return;
				break;
			case 0x200: /* Z80 CPU Reset */
				if (data == 0x00)
				{ 
			       	cpu_halt(1,HALT);
					cpu_reset(1);
			   
			   	  	cpu_halt(1,HALT); 
				  	if (errorlog) fprintf(errorlog, "z80 reset, ram is %p\n", &genesis_soundram[0]);
			   	  	z80running = 0;
				  	return;
				}
				else
				{
				   if (errorlog) fprintf(errorlog, "z80 out of reset\n");
				}
				return;	 
					
				break;
		}
}
#if 0
int cartridge_ram_r (int offset)
{
/*  if (errorlog) fprintf(errorlog, "cartridge ram read.. %x\n", offset);*/
	return cartridge_ram[offset];
}
void cartridge_ram_w (int offset, int data)
{
/*  if (errorlog) fprintf(errorlog, "cartridge ram write.. %x to %x\n", data, offset);*/
	cartridge_ram[offset] = data;
}
#endif

