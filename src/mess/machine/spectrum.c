/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

/* KT 31/1/00 - Added support for .Z80. At the moment only 48k files are supported! */

static unsigned char *pSnapshotData = NULL;
static unsigned long SnapshotDataSize = 0;
static void spectrum_setup_sna(unsigned char *pSnapshot, unsigned long SnapshotSize);
static void spectrum_setup_z80(unsigned char *pSnapshot, unsigned long SnapshotSize);
static int spectrum_opbaseoverride(int);

static void spectrum_sh_update(int);


typedef enum
{
        SPECTRUM_SNAPSHOT_SNA,
        SPECTRUM_SNAPSHOT_Z80
} SPECTRUM_SNAPSHOT_TYPE;

static SPECTRUM_SNAPSHOT_TYPE spectrum_snapshot_type;

int spectrum_sh_state;

void spectrum_init_machine(void)
{
        timer_pulse(1.0/Machine->sample_rate, 0, spectrum_sh_update);

	if (pSnapshotData != NULL)
	{
		cpu_setOPbaseoverride(0, spectrum_opbaseoverride);
	}
}

void    spectrum_sh_update(int param)
{
        DAC_data_w(0, spectrum_sh_state * 127);
}

void spectrum_shutdown_machine(void)
{
}

int spectrum_rom_load(int id, const char *name)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);


	if (file)
	{
		int datasize;
		unsigned char *data;

		datasize = osd_fsize(file);

		if (datasize != 0)
		{
			data = malloc(datasize);

			if (data != NULL)
			{
				pSnapshotData = data;
				SnapshotDataSize = datasize;

				osd_fread(file, data, datasize);

				osd_fclose(file);

                                if (datasize == 49179)
                                        spectrum_snapshot_type = SPECTRUM_SNAPSHOT_SNA;
                                else
                                        spectrum_snapshot_type = SPECTRUM_SNAPSHOT_Z80;

				if (errorlog)
					fprintf(errorlog, "File loaded!\r\n");

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
	cpu_setOPbaseoverride(0, 0);

	if (pSnapshotData)
	{
                /* snapshot loaded setup */

                switch (spectrum_snapshot_type)
                {
                        case SPECTRUM_SNAPSHOT_SNA:
                        {
                                /* .SNA */
                                spectrum_setup_sna(pSnapshotData, SnapshotDataSize);
                        }
                        break;

                        case SPECTRUM_SNAPSHOT_Z80:
                        {
                                /* .Z80 */
                                spectrum_setup_z80(pSnapshotData, SnapshotDataSize);
                        }
                        break;
                }
        }
	return (cpu_get_reg(Z80_PC) & 0x0ffff);
}

void spectrum_setup_sna(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
     //   unsigned char *RAM;
	unsigned char lo, hi, data;
	unsigned short addr;

	cpu_set_reg(Z80_I, (pSnapshot[0] & 0x0ff));
	lo = pSnapshot[1] & 0x0ff;
	hi = pSnapshot[2] & 0x0ff;
	cpu_set_reg(Z80_HL2, (hi << 8) | lo);
	lo = pSnapshot[3] & 0x0ff;
	hi = pSnapshot[4] & 0x0ff;
	cpu_set_reg(Z80_DE2, (hi << 8) | lo);
	lo = pSnapshot[5] & 0x0ff;
	hi = pSnapshot[6] & 0x0ff;
	cpu_set_reg(Z80_BC2, (hi << 8) | lo);
	lo = pSnapshot[7] & 0x0ff;
	hi = pSnapshot[8] & 0x0ff;
	cpu_set_reg(Z80_AF2, (hi << 8) | lo);
	lo = pSnapshot[9] & 0x0ff;
	hi = pSnapshot[10] & 0x0ff;
	cpu_set_reg(Z80_HL, (hi << 8) | lo);
	lo = pSnapshot[11] & 0x0ff;
	hi = pSnapshot[12] & 0x0ff;
	cpu_set_reg(Z80_DE, (hi << 8) | lo);
	lo = pSnapshot[13] & 0x0ff;
	hi = pSnapshot[14] & 0x0ff;
	cpu_set_reg(Z80_BC, (hi << 8) | lo);
	lo = pSnapshot[15] & 0x0ff;
	hi = pSnapshot[16] & 0x0ff;
	cpu_set_reg(Z80_IY, (hi << 8) | lo);
	lo = pSnapshot[17] & 0x0ff;
	hi = pSnapshot[18] & 0x0ff;
	cpu_set_reg(Z80_IX, (hi << 8) | lo);
	data = (pSnapshot[19] & 0x02) >> 1;
	cpu_set_reg(Z80_IFF2, data);
	cpu_set_reg(Z80_IFF1, data);
	data = (pSnapshot[20] & 0x0ff);
	cpu_set_reg(Z80_R, data);
	lo = pSnapshot[21] & 0x0ff;
	hi = pSnapshot[22] & 0x0ff;
	cpu_set_reg(Z80_AF, (hi << 8) | lo);
	lo = pSnapshot[23] & 0x0ff;
	hi = pSnapshot[24] & 0x0ff;
	cpu_set_reg(Z80_SP, (hi << 8) | lo);
	cpu_set_sp((hi << 8) | lo);
	data = (pSnapshot[25] & 0x0ff);
	cpu_set_reg(Z80_IM, data);

	// snapshot + 26 = border colour

	cpu_set_reg(Z80_NMI_STATE, 0);
	cpu_set_reg(Z80_IRQ_STATE, 0);
	cpu_set_reg(Z80_HALT, 0);

   //     RAM = memory_region(REGION_CPU1);

	// memory dump
//	memcpy(&RAM[16384], &pSnapshot[27],49152);
	for (i = 0; i < 49152; i++)
	{
		cpu_writemem16(i + 16384, pSnapshot[27 + i]);
	}

	/* get pc from stack */
	addr = cpu_geturnpc();
	cpu_set_reg(Z80_PC, (addr & 0x0ffff));
	//cpu_set_pc((addr & 0x0ffff));

	addr = cpu_get_reg(Z80_SP);
	addr += 2;
	cpu_set_reg(Z80_SP, (addr & 0x0ffff));
	cpu_set_sp((addr & 0x0ffff));
}


static void spectrum_z80_decompress_block(unsigned char *pSource, int Dest, int size)
{
   unsigned char ch;
   int i;

   do
   {
       /* get byte */
       ch = pSource[0];

       /* either start 0f 0x0ed, 0x0ed, xx yy or
       single 0x0ed */
       if (ch == (unsigned char)0x0ed)
       {
         if (pSource[1] == (unsigned char)0x0ed)
         {

            /* 0x0ed, 0x0ed, xx yy */
            /* repetition */

            int count;
            int data;

            count = (pSource[2] & 0x0ff);

            if (count==0)
                return;

            data = (pSource[3] & 0x0ff);

            pSource+=4;

            if (count>size)
                count = size;

            size-=count;

            for (i=0; i<count; i++)
            {
                cpu_writemem16(Dest, data);
                Dest++;
            }
         }
         else
         {
           /* single 0x0ed */
           cpu_writemem16(Dest, ch);
           Dest++;
           pSource++;
           size--;

         }
       }
       else
       {
          /* not 0x0ed */
          cpu_writemem16(Dest, ch);
          Dest++;
          pSource++;
          size--;
       }

   }
   while (size>0);
}

/* for now, only 48k .Z80 are supported */
void spectrum_setup_z80(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
        //unsigned char *RAM;
	unsigned char lo, hi, data;


        /* AF */
        hi = pSnapshot[0] & 0x0ff;
        lo = pSnapshot[1] & 0x0ff;
        cpu_set_reg(Z80_AF, (hi << 8) | lo);
        /* BC */
        lo = pSnapshot[2] & 0x0ff;
        hi = pSnapshot[3] & 0x0ff;
        cpu_set_reg(Z80_BC, (hi << 8) | lo);
        /* HL */
        lo = pSnapshot[4] & 0x0ff;
        hi = pSnapshot[5] & 0x0ff;
        cpu_set_reg(Z80_HL, (hi << 8) | lo);

        /* program counter - 0 if not version 1.45 */

        /* SP */
        lo = pSnapshot[8] & 0x0ff;
        hi = pSnapshot[9] & 0x0ff;
        cpu_set_reg(Z80_SP, (hi << 8) | lo);
        cpu_set_sp((hi<<8) | lo);

        /* I */
        cpu_set_reg(Z80_I, (pSnapshot[10] & 0x0ff));

        /* R */
        data = (pSnapshot[11] & 0x07f) | ((pSnapshot[12] & 0x01)<<7);
	cpu_set_reg(Z80_R, data);

        lo = pSnapshot[13] & 0x0ff;
        hi = pSnapshot[14] & 0x0ff;
        cpu_set_reg(Z80_DE, (hi << 8) | lo);

        lo = pSnapshot[15] & 0x0ff;
        hi = pSnapshot[16] & 0x0ff;
        cpu_set_reg(Z80_BC2, (hi << 8) | lo);

        lo = pSnapshot[17] & 0x0ff;
        hi = pSnapshot[18] & 0x0ff;
        cpu_set_reg(Z80_DE2, (hi << 8) | lo);

        lo = pSnapshot[19] & 0x0ff;
        hi = pSnapshot[20] & 0x0ff;
        cpu_set_reg(Z80_HL2, (hi << 8) | lo);

        hi = pSnapshot[21] & 0x0ff;
        lo = pSnapshot[22] & 0x0ff;
        cpu_set_reg(Z80_AF2, (hi << 8) | lo);

        lo = pSnapshot[23] & 0x0ff;
        hi = pSnapshot[24] & 0x0ff;
        cpu_set_reg(Z80_IY, (hi << 8) | lo);

        lo = pSnapshot[25] & 0x0ff;
        hi = pSnapshot[26] & 0x0ff;
        cpu_set_reg(Z80_IX, (hi << 8) | lo);

        /* Interrupt Flip/Flop */
        if (pSnapshot[27]==0)
        {
                cpu_set_reg(Z80_IFF1, 0);
                //cpu_set_reg(Z80_IRQ_STATE, 0);
        }
        else
        {
                cpu_set_reg(Z80_IFF1, 1);

                //cpu_set_reg(Z80_IRQ_STATE, 1);

        }

	cpu_set_reg(Z80_NMI_STATE, 0);
	cpu_set_reg(Z80_IRQ_STATE, 0);
	cpu_set_reg(Z80_HALT, 0);


        /* IFF2 */
        if (pSnapshot[28]!=0)
        {
                data = 1;
        }
        else
        {
                data = 0;
        }
        cpu_set_reg(Z80_IFF2, data);

        /* Interrupt Mode */
        cpu_set_reg(Z80_IM, (pSnapshot[29] & 0x03));

        if ((pSnapshot[6] | pSnapshot[7])!=0)
        {
                if (errorlog) fprintf(errorlog,"Old 1.45 V of Z80 snapshot found!\r\n");

                /* program counter is specified. Old 1.45 */
                lo = pSnapshot[6] & 0x0ff;
                hi = pSnapshot[7] & 0x0ff;
                cpu_set_reg(Z80_PC, (hi << 8) | lo);
        
        
                if ((pSnapshot[12] & 0x020)==0)
                {
                        if (errorlog) fprintf(errorlog, "Not compressed\r\n");

                        /* not compressed */
                       for (i = 0; i < 49152; i++)
                       {
                          cpu_writemem16(i+16384, pSnapshot[30 + i]);
                       }
                }
                else
                {
                        if (errorlog) fprintf(errorlog, "Compressed\r\n");

                   /* compressed */
                   spectrum_z80_decompress_block(pSnapshot+30, 16384, 49152);


                }
         }
         else
         {
                unsigned char *pSource;
                int header_size;

                if (errorlog) fprintf(errorlog, "v2.0+ V of Z80 snapshot found!\r\n");

                header_size = 30 + 2 + ((pSnapshot[30] & 0x0ff) | ((pSnapshot[31] & 0x0ff)<<8));

                lo = pSnapshot[32] & 0x0ff;
                hi = pSnapshot[33] & 0x0ff;
                cpu_set_reg(Z80_PC, (hi << 8) | lo);
                                  
                for (i=0; i<16; i++)
                {
                        AY8910_control_port_0_w(0, i);

                        AY8910_write_port_0_w(0, pSnapshot[39+i]);
                }

                AY8910_control_port_0_w(0, pSnapshot[38]);


                pSource = pSnapshot + header_size;



             do
             {
                unsigned short length;
                unsigned char page;
                int Dest = 0;

                length = (pSource[0] & 0x0ff) | ((pSource[1] & 0x0ff)<<8);
                page = pSource[2];

                switch (page)
                {
                        case 4:
                        {
                            Dest = 0x08000;
                        }
                        break;

                        case 5:
                        {
                            Dest = 0x0c000;
                        }
                        break;

                        case  8:
                        {
                            Dest = 0x04000;
                        }
                        break;

                        default:
                            break;
                }
                     
                if ((page==4) || (page==5) || (page==8))
                {
                        if (length==0x0ffff)
                        {
                                /* block is uncompressed */
                                if (errorlog) fprintf(errorlog, "Not compressed\r\n");

                                /* not compressed */
                               for (i = 0; i < 49152; i++)
                               {
                                  cpu_writemem16(i+16384, pSnapshot[30 + i]);
                               }


                        }
                        else
                        {
                               if (errorlog) fprintf(errorlog, "Compressed\r\n");

                                /* block is compressed */
                                spectrum_z80_decompress_block(&pSource[3], Dest, 16383); 
                        }
                }

                /* go to next block */
                pSource+=(3+length);
            }
            while (((unsigned long)pSource-(unsigned long)pSnapshot)<SnapshotDataSize);
      }
}

int spectrum_rom_id(const char *name, const char *gamename)
{
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

void spectrum_nmi_generate(int param)
{
	cpu_cause_interrupt(0, Z80_NMI_INT);
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
