/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

void    spectrum_init_machine(void) {
}

void    spectrum_shutdown_machine(void) {
}

int     spectrum_rom_load(void) {

  	 return 0;

}

//static int onlyonce=1;

int 	load_snap (void) {

//void    *snapfile; /* for loading the .sna file */
//UINT8	lo,hi;
//unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	return 1;	/* small hack for now */

#if 0 // TODO: use z80_getregs
	if (onlyonce) {

		if (strlen(rom_name[0])==0) {
			if (errorlog)
				fprintf(errorlog,"warning : no image file specified\n");
			return 0;
		} else {
			if (!(snapfile = osd_fopen (Machine->gamedrv->name,rom_name[0],OSD_FILETYPE_ROM_CART, 0))) {
				if (errorlog)
					fprintf(errorlog,"Unable to open file %s\n",rom_name[0]);
				return 0;
			}

			/* Load the snap file, only .sna supported for now */
			osd_fread(snapfile,&regs.I,1);
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.HL2.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.DE2.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.BC2.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.AF2.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.HL.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.DE.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.BC.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.IY.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.IX.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			regs.IFF2 = (lo & 0x02) >> 1;
			osd_fread(snapfile,&lo,1);
			regs.R = lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.AF.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);
			osd_fread(snapfile,&hi,1);
			regs.SP.W.l = (hi << 8) | lo;
			osd_fread(snapfile,&lo,1);	/* IntMode */
			regs.IM = lo;

			osd_fread(snapfile,&lo,1);	/* Bordercolor, not used */

			osd_fread(snapfile,&RAM[16384],49152);	/* load the rest of the snapshot */
			/* get the PC from the stack */
			lo = RAM[regs.SP.W.l];
			hi = RAM[regs.SP.W.l+1];
			regs.PC.W.l = (hi << 8) | lo;

			regs.SP.W.l += 2;
			RAM[regs.SP.W.l - 1] = 0;
			RAM[regs.SP.W.l - 2] = 0;

			osd_fclose(snapfile);

			if (errorlog)
				fprintf(errorlog,"Changed the register set\n");
			onlyonce = 0;
		}
	}
	return 1;
#endif
}

int     spectrum_rom_id(const char *name, const char *gamename) {
	return 1;
}


/*************************************
 *
 *              Port handlers.
 *
 *************************************/


/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

void    spectrum_nmi_generate (int param) {
	cpu_cause_interrupt (0, Z80_NMI_INT);
}

/*************************************
 *                                   *
 *      Memory handlers              *
 *                                   *
 *************************************/

/*************************************
 *      Keyboard                     *
 *************************************/

/*************************************
 *      Video RAM                    *
 *************************************/

