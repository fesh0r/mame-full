/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

static unsigned char *pSnapshotData=NULL;
static unsigned long SnapshotDataSize=0;
static void spectrum_setup_snapshot(unsigned char *pSnapshot, unsigned long SnapshotSize);
static int spectrum_opbaseoverride(int);

void    spectrum_init_machine(void)
{
        if (pSnapshotData!=NULL)
        {
                cpu_setOPbaseoverride(0, spectrum_opbaseoverride);
        }
}

void    spectrum_shutdown_machine(void) {
}

int     spectrum_rom_load(void)
{
        void *file;

        file = osd_fopen(Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);

        if (errorlog) fprintf(errorlog,"hmm!\r\n");

        if (file)
        {
            int datasize;
            unsigned char *data;

            datasize = osd_fsize(file);

            if (datasize!=0)
            {
                data = malloc(datasize);

                if (data!=NULL)
                {
                        pSnapshotData = data;
                        SnapshotDataSize = datasize;

                        osd_fread(file, data, datasize);

                        osd_fclose(file);

                        if (errorlog) fprintf(errorlog, "File loaded!\r\n");

                        return 0;
                }

                osd_fclose(file);
            }

            return 1;
        }

        return 0;

}

int spectrum_opbaseoverride(int pc)
{
        /* clear op base override */
        cpu_setOPbaseoverride(0,0);

        if (pSnapshotData)
        {
                /* snapshot loaded setup */
                spectrum_setup_snapshot(pSnapshotData,SnapshotDataSize);
        }

        return (cpu_get_reg(Z80_PC) & 0x0ffff);
}

void    spectrum_setup_snapshot(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
        int i;
        unsigned char *RAM;
        unsigned char lo,hi, data;
        unsigned short addr;

        cpu_set_reg(Z80_I, (pSnapshot[0] & 0x0ff));
        lo = pSnapshot[1] & 0x0ff;
        hi = pSnapshot[2] & 0x0ff;
        cpu_set_reg(Z80_HL2, (hi<<8) | lo);
        lo = pSnapshot[3] & 0x0ff;
        hi = pSnapshot[4] & 0x0ff;
        cpu_set_reg(Z80_DE2, (hi<<8) | lo);
        lo = pSnapshot[5] & 0x0ff;
        hi = pSnapshot[6] & 0x0ff;
        cpu_set_reg(Z80_BC2, (hi<<8) | lo);
        lo = pSnapshot[7] & 0x0ff;
        hi = pSnapshot[8] & 0x0ff;
        cpu_set_reg(Z80_AF2, (hi<<8) | lo);
        lo = pSnapshot[9] & 0x0ff;
        hi = pSnapshot[10] & 0x0ff;
        cpu_set_reg(Z80_HL, (hi<<8) | lo);
        lo = pSnapshot[11] & 0x0ff;
        hi = pSnapshot[12] & 0x0ff;
        cpu_set_reg(Z80_DE, (hi<<8) | lo);
        lo = pSnapshot[13] & 0x0ff;
        hi = pSnapshot[14] & 0x0ff;
        cpu_set_reg(Z80_BC, (hi<<8) | lo);
        lo = pSnapshot[15] & 0x0ff;
        hi = pSnapshot[16] & 0x0ff;
        cpu_set_reg(Z80_IY, (hi<<8) | lo);
        lo = pSnapshot[17] & 0x0ff;
        hi = pSnapshot[18] & 0x0ff;
        cpu_set_reg(Z80_IX, (hi<<8) | lo);
        data = (pSnapshot[19] & 0x02)>>1;
        cpu_set_reg(Z80_IFF2, data);
        cpu_set_reg(Z80_IFF1, data);
        data = (pSnapshot[20] & 0x0ff);
        cpu_set_reg(Z80_R, data);
        lo = pSnapshot[21] & 0x0ff;
        hi = pSnapshot[22] & 0x0ff;
        cpu_set_reg(Z80_AF, (hi<<8) | lo);
        lo = pSnapshot[23] & 0x0ff;
        hi = pSnapshot[24] & 0x0ff;
        cpu_set_reg(Z80_SP, (hi<<8) | lo);
        cpu_set_sp((hi<<8) | lo);
        data = (pSnapshot[25] & 0x0ff);
        cpu_set_reg(Z80_IM, data);

        // snapshot + 26 = border colour

        cpu_set_reg(Z80_NMI_STATE, 0);
        cpu_set_reg(Z80_IRQ_STATE, 0);
        cpu_set_reg(Z80_HALT, 0);

        RAM = memory_region(REGION_CPU1);

        // memory dump
//        memcpy(&RAM[16384], &pSnapshot[27],49152);
        for (i=0; i<49152; i++)
        {
                cpu_writemem16(i+16384, pSnapshot[27 + i]);
        }

        /* get pc from stack */
        addr = cpu_geturnpc();
        cpu_set_reg(Z80_PC, (addr & 0x0ffff));
        //cpu_set_pc((addr & 0x0ffff));

        addr = cpu_get_reg(Z80_SP);
        addr+=2;
        cpu_set_reg(Z80_SP, (addr & 0x0ffff));
        cpu_set_sp((addr & 0x0ffff));
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

