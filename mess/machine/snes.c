/***************************************************************************

 Super Nintendo Entertainment System Driver - Written By Lee Hammerton aKa Savoury SnaX

 Acknowledgements

	I gratefully acknowledge the work of Karl Stenerud for his work on the processor
  cores used in this emulation and of course I hope you'll continue to work with me
  to improve this project.

	I would like to acknowledge the support of all those who helped me during SNEeSe and
  in doing so have helped get this project off the ground. There are many, many people
  to thank and so little space that I am keeping this as brief as I can :

		All snes technical document authors!
		All snes emulator authors!
			ZSnes
			Snes9x
			Snemul
			Nlksnes
			and the others....
		The original SNEeSe team members (other than myself ;-)) -
			Charles Bilyue - Your continued work on SNEeSe is fantastic!
			Santeri Saarimaa - Who'd have thought I'd come back to emulation ;-)

	***************************************************************************

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "snes.h"

unsigned char *SNES_ROM=NULL;

unsigned char *SNES_SRAM=NULL;				// Save ram / extra ram
unsigned char *SNES_WRAM=NULL;				// Work memory (ram) (4 * 32k pages)
unsigned char *SNES_VRAM=NULL;				// Video memory
unsigned char *SNES_CRAM=NULL;				// Colour memory
unsigned char *SNES_ORAM=NULL;				// Object memory (sprites)

unsigned char OAMADDRESS_L,OAMADDRESS_H;

unsigned char port21xx[256];
unsigned char wport21xx[2][256];

unsigned char port42xx[256];
unsigned char port43xx[256];

unsigned short palIndx[256+1];				// Used to map snes colours to hivalues

#ifdef EMULATE_SPC700
unsigned char SPCIN[4],SPCOUT[4];			// Used as crossover ports between spc and cpu
unsigned char spcPort[19];					// 3 extra for T0 T1 T2
void *spc700Timer;							// Used to allow me to clear the timer at quit
int spc700TimerExtra;						// Used to hold whether to increment T0 & T1
#endif

unsigned short fixedColour;					// Fixed color encoded into here

void snes_init_machine(void)
{
	int a;

	for (a=0;a<256;a++)
	{
		port21xx[a]=0;
		wport21xx[0][a]=0xCB;
		wport21xx[1][a]=0xCB;
		port42xx[a]=0;
		port43xx[a]=0;
	}

	SNES_WRAM=(unsigned char *)malloc(0x20000);			// 128k bytes of wram
	if (SNES_WRAM==NULL)
		logerror("Out of ram!\n");
	for (a=0;a<0x20000;a++)
		SNES_WRAM[a]=0xFF;
	SNES_VRAM=(unsigned char *)malloc(0x20000);			// 64k words of vram
	if (SNES_VRAM==NULL)
		logerror("Out of ram!\n");
	for (a=0;a<0x20000;a++)
		SNES_VRAM[a]=0xFF;
	SNES_CRAM=(unsigned char *)malloc(0x200);			// 512 bytes cgram
	if (SNES_CRAM==NULL)
		logerror("Out of ram!\n");
	for (a=0;a<0x200;a++)
		SNES_CRAM[a]=0xFF;
	SNES_ORAM=(unsigned char *)malloc(0x400);			// 1024 bytes oam
	if (SNES_ORAM==NULL)
		logerror("Out of ram!\n");
	for (a=0;a<0x400;a++)
		SNES_ORAM[a]=0xFF;

	cpu_setbank(1,SNES_WRAM);


#ifdef EMULATE_SPC700
	// Need to allocate a timer for the SPC700 core (I`ll make 1 go off every 15.6MSecs and then 8 of these will cause the 125MSec timer
	//to step).
	spc700Timer=timer_pulse(TIME_IN_MSEC(15.6),0,spcTimerTick);
	spc700TimerExtra=0;
#endif
}

extern unsigned char *SPCRamPtr;

void snes_shutdown_machine(void)
{
	int a;
	FILE *tmpFile;

#ifdef EMULATE_SPC700
	timer_remove(spc700Timer);
#endif

	tmpFile = fopen("SNES_ROM.DMP", "w");
	for (a=0;a<0x8000;a++)
	{
		if ((a&15)==0)
			fprintf(tmpFile,"%04X : ",a);
		if ((a&15)==15)
			fprintf(tmpFile,"%02X\n",SNES_ROM[a+0x8000]);
		else
			fprintf(tmpFile,"%02X,",SNES_ROM[a+0x8000]);
	}
	fclose(tmpFile);

	tmpFile = fopen("SNESORAM.DMP", "w");
	for (a=0;a<0x400;a++)
	{
		if ((a&15)==0)
			fprintf(tmpFile,"%04X : ",a);
		if ((a&15)==15)
			fprintf(tmpFile,"%02X\n",SNES_ORAM[a]);
		else
			fprintf(tmpFile,"%02X,",SNES_ORAM[a]);
	}
	fclose(tmpFile);
	free(SNES_ORAM);
	tmpFile = fopen("SNESCRAM.DMP", "w");
	for (a=0;a<512;a++)
	{
		if ((a&1)==0)
			fprintf(tmpFile,"%04X : ",a>>1);
		if ((a&1)==1)
			fprintf(tmpFile,"%02X\n",SNES_CRAM[a]);
		else
			fprintf(tmpFile,"%02X,",SNES_CRAM[a]);
	}
	fclose(tmpFile);
	free(SNES_CRAM);
	tmpFile = fopen("SNESVRAM.DMP", "w");
	for (a=0;a<0x20000;a++)
	{
		if ((a&15)==0)
			fprintf(tmpFile,"%04X : ",a);
		if ((a&15)==15)
			fprintf(tmpFile,"%02X\n",SNES_VRAM[a]);
		else
			fprintf(tmpFile,"%02X,",SNES_VRAM[a]);
	}
	free(SNES_VRAM);
	fclose(tmpFile);
	tmpFile = fopen("SNESWRAM.DMP", "w");
	for (a=0;a<0x20000;a++)
	{
		if ((a&15)==0)
			fprintf(tmpFile,"%04X : ",a);
		if ((a&15)==15)
			fprintf(tmpFile,"%02X\n",SNES_WRAM[a]);
		else
			fprintf(tmpFile,"%02X,",SNES_WRAM[a]);
	}
	fclose(tmpFile);
	free(SNES_WRAM);
#ifdef EMULATE_SPC700
	tmpFile = fopen("SNESSPC.DMP", "w");
	for (a=0;a<0x10000;a++)
	{
		if ((a&15)==0)
			fprintf(tmpFile,"%04X : ",a);
		if ((a&15)==15)
			fprintf(tmpFile,"%02X\n",SPCRamPtr[a]);
		else
			fprintf(tmpFile,"%02X,",SPCRamPtr[a]);
	}
	fclose(tmpFile);
#endif
}

/* ROM LOADING STUFF - Also allocates Save Ram - In Future will make SRAM File correct size */

int snes_load_rom (int id)
{
	const char *rom_name = device_filename(IO_CARTSLOT,id);
	char tempPath[256]="NVRAM\\";
	//UINT32 aa;
    FILE *romfile,*sramFile;
	int numBanks,a;

	if(!rom_name)
	{
		printf("SNES requires cartridge!\n");
		return INIT_FAIL;
	}

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		return INIT_FAIL;
	}

    /* Allocate memory and set up memory regions */
/*	if (new_memory_region(REGION_CPU1, 0x408000))
	{
		printf ("Memory allocation failed reading roms!\n");
		osd_fclose (romfile);
		return 1;
	}
*/
	logerror("SNES_ROM %08x\n",SNES_ROM);

	SNES_ROM = memory_region(REGION_CPU1);

	logerror("SNES_ROM %08x\n",SNES_ROM);

	//osd_fseek (romfile,0,SEEK_END);
	//numBanks=(osd_ftell(romfile));
	numBanks=((osd_fsize(romfile)-512)>>16);
	printf("Number of banks : %04x\n", numBanks);

	/* Position past the header */
//	osd_fseek (romfile, 512, SEEK_SET);

	/* Read in the PRG_Rom chunks */

	//numBanks+=1;							// Need to look at another way of doing this since the above method seems to cause a few problems!!
	//numBanks-=1;

	numBanks*=2;

	if (numBanks>16)
	{
		printf("Unable to load roms with more than 16 pages of 32k at present sorry!\nLoaded as much as I can!\n");
		numBanks=16;
//		osd_fclose(romfile);
//		return 1;
	}

	osd_fread(romfile,&SNES_ROM[0x8000],512);			// skip over header

	for (a=0;a<numBanks;a++)
	{
		osd_fread(romfile,&SNES_ROM[a*0x10000 + 0x8000],0x8000);
		//osd_fread(romfile,&SNES_ROM[a*0x10000],0x10000);
	}
	//for (aa=0; aa < 0x3F; aa++)
  	//{
    	//memcpy(SNES_ROM+(aa<<16)+0x8000, SNES_ROM+(aa<<0x0f), 32768);
  	//}

	SNES_SRAM=(unsigned char *)malloc(0x50000);			// 5 banks of sram
	if (SNES_SRAM==NULL)
		logerror("Out of ram!\n");
	for (a=0;a<0x50000;a++)
		SNES_SRAM[a]=0xFF;								// Snes save ram starts as -1

	strcat(tempPath,rom_name);
	sramFile = fopen(tempPath, "r");
	if (sramFile)
	{
		fread(SNES_SRAM,1,0x50000,sramFile);
		fclose(sramFile);
	}

	osd_fclose (romfile);
	return 0;
}

void snes_exit_rom(int id)
{
	const char *rom_name = device_filename(IO_CARTSLOT,id);
	char tempPath[256]="NVRAM\\";
    FILE *sramFile;
	// De-allocate Save Ram File - saving it first

	if (rom_name) {
		strcat(tempPath,rom_name);
		sramFile = fopen(tempPath, "w");
		if (sramFile)
		{
			fwrite(SNES_SRAM,1,0x50000,sramFile);
			fclose(sramFile);
		}
	}

	free(SNES_SRAM);
}

/* Simple Id done as - divide total size of rom by 32*1024... if remainder is 512 smc header present file is valid, else
  header not present... don't load file! */

int snes_id_rom (int id)
{
    FILE *romfile;
	unsigned char magic[4];
	int retval=0,filePos;

	if (!(romfile = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0))) return 0;

	osd_fseek (romfile,0,SEEK_END);
	filePos=osd_ftell(romfile);
	printf("file position = %d , %08X\n",filePos,filePos);

	if ( (filePos % (32*1024))==512)
		retval=1;
	osd_fread (romfile, magic, 4);
	osd_fclose (romfile);
	return retval;
}



