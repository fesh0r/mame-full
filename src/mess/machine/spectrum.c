/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

/* KT 31/1/00 - Added support for .Z80. At the moment only 48k files are supported! */
/* DJR 8/2/00 - Added checks to avoid trying to load 128K .Z80 files into 48K machine! */
/* DJR 20/2/00 - Added support for .TAP files. */
/*-----------------27/02/00 10:54-------------------
 KT 27/2/00 - Added my changes for the WAV support
--------------------------------------------------*/
#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

static unsigned char *pSnapshotData = NULL;
static unsigned long SnapshotDataSize = 0;
static unsigned long TapePosition = 0;
static void spectrum_setup_sna(unsigned char *pSnapshot, unsigned long SnapshotSize);
static void spectrum_setup_z80(unsigned char *pSnapshot, unsigned long SnapshotSize);
static int is48k_z80snapshot(unsigned char *pSnapshot, unsigned long SnapshotSize);
static int spectrum_opbaseoverride(int);
static int spectrum_tape_opbaseoverride(int);


typedef enum
{
        SPECTRUM_SNAPSHOT_NONE,
        SPECTRUM_SNAPSHOT_SNA,
        SPECTRUM_SNAPSHOT_Z80,
        SPECTRUM_TAPEFILE_TAP
} SPECTRUM_SNAPSHOT_TYPE;

static SPECTRUM_SNAPSHOT_TYPE spectrum_snapshot_type = SPECTRUM_SNAPSHOT_NONE;


void spectrum_init_machine(void)
{
	if (pSnapshotData != NULL)
	{
                if (spectrum_snapshot_type == SPECTRUM_TAPEFILE_TAP)
                {
                        if (errorlog) fprintf(errorlog, ".TAP file support enabled\n");
                        cpu_setOPbaseoverride(0, spectrum_tape_opbaseoverride);
                }
                else
                        cpu_setOPbaseoverride(0, spectrum_opbaseoverride);
	}
}

void spectrum_shutdown_machine(void)
{
}

int spectrum_rom_load(int id)
{
	void *file;

        file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);


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
                                const char *filename;

				pSnapshotData = data;
				SnapshotDataSize = datasize;

				osd_fread(file, data, datasize);

				osd_fclose(file);

                                filename = device_filename(IO_CARTSLOT, id);

                                if (!stricmp (filename+strlen(filename)-4,".tap"))
                                {
                                        spectrum_snapshot_type = SPECTRUM_TAPEFILE_TAP;
                                }

                                else if (datasize == 49179)
                                        spectrum_snapshot_type = SPECTRUM_SNAPSHOT_SNA;
                                else
                                {
                                        if (!is48k_z80snapshot(pSnapshotData, SnapshotDataSize))
                                        {
                                                if (errorlog) fprintf(errorlog, "Not a 48K .Z80 file\n");
                                                return 1;
                                        }
                                        spectrum_snapshot_type = SPECTRUM_SNAPSHOT_Z80;
                                }


				if (errorlog)
                                        fprintf(errorlog, "File loaded!\n");

				return 0;
			}

			osd_fclose(file);
		}

		return 1;
	}

	return 0;

}

void    spectrum_rom_exit(int id)
{
        if (pSnapshotData!=NULL)
        {
                /* free snapshot/tape data */
                free(pSnapshotData);
                pSnapshotData = NULL;

                /* reset type to none */
                spectrum_snapshot_type = SPECTRUM_SNAPSHOT_NONE;

                /* ensure op base is cleared */
                cpu_setOPbaseoverride(0,0);
        }

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

                        default:
                        /* SPECTRUM_TAPEFILE_TAP is handled by spectrum_tape_opbaseoverride */
                        break;
                }
        }
	return (cpu_get_reg(Z80_PC) & 0x0ffff);
}

/*******************************************************************
 *
 *      Override load routine (0x0556 in ROM) if loading .TAP files
 *      Tape blocks are as follows.
 *      2 bytes length of block excluding these 2 bytes (LSB first)
 *      1 byte  flag byte (0x00 for headers, 0xff for data)
 *      n bytes data
 *      1 byte  checksum
 *
 *      The load routine uses the following registers:
 *      IX              Start address for block
 *      DE              Length of block
 *      A               Flag byte (as above)
 *      Carry Flag      Set for Load, reset for verify
 *
 *      On exit the carry flag is set if loading/verifying was successful.
 *
 *      Note: it is not always possible to trap the exact entry to the
 *      load routine so things get rather messy!
 *
 *******************************************************************/
int spectrum_tape_opbaseoverride(int pc)
{
        int i, tap_block_length, load_length;
        unsigned char lo, hi, a_reg;
        unsigned short addr, af_reg, de_reg, sp_reg;

//        if (errorlog) fprintf(errorlog, "PC=%02x\n", pc);

        if ((pc >= 0x05e3) && (pc <= 0x0604))
        /* It is not always possible to trap the call to the actual load
           routine so trap the LD-EDGE-1 and LD-EDGE-2 routines which
           check the earphone socket.
        */
        {
                lo = pSnapshotData[TapePosition] & 0x0ff;
                hi = pSnapshotData[TapePosition+1] & 0x0ff;
                tap_block_length = (hi << 8) | lo;

                /* By the time that load has been trapped the block type and
                   carry flags are in the AF' register.
                */
                af_reg = cpu_get_reg (Z80_AF2);
                a_reg = (af_reg & 0xff00) >> 8;

                if ((a_reg == pSnapshotData[TapePosition+2]) && (af_reg & 0x0001))
                {
                        /* Correct flag byte and carry flag set so try loading */
                        addr = cpu_get_reg(Z80_IX);
                        de_reg = cpu_get_reg(Z80_DE);

                        load_length = MIN(de_reg, tap_block_length-2);
                        load_length = MIN(load_length, 65536-addr);
						/* Actual number of bytes of block that can be loaded */

                        for (i = 0; i < load_length; i++)
                                cpu_writemem16(addr+i, pSnapshotData[TapePosition+i+3]);
                        cpu_set_reg(Z80_IX, addr+load_length);
                        cpu_set_reg(Z80_DE, de_reg-load_length);
                        if (de_reg == (tap_block_length-2))
                        {
                                /* Successful load - Set carry flag and A to 0 */
                                cpu_set_reg(Z80_AF, (af_reg & 0x00ff) | 0x0001);
                                if (errorlog) fprintf(errorlog, "Loaded %04x bytes from address %04x onwards (type=%02x) using tape block at offset %ld\n", load_length, addr, a_reg, TapePosition);
                        }
                        else
                        {
                                /* Wrong tape block size */
                                cpu_set_reg(Z80_AF, af_reg & 0xfffe);
                                if (errorlog) fprintf(errorlog, "Bad block length %04x bytes wanted starting at address %04x (type=%02x) , Data length of tape block at offset %ld is %04x bytes\n", de_reg, addr, a_reg, TapePosition, tap_block_length-2);
                        }
                }
                else
                {
                        /* Wrong flag byte or verify selected so reset carry flag to indicate failure */
                        cpu_set_reg(Z80_AF, af_reg & 0xfffe);
                        if (errorlog) fprintf(errorlog, "Failed to load tape block at offset %ld - type wanted %02x, got type %02x\n", TapePosition, a_reg, pSnapshotData[TapePosition+2]);
                }

                TapePosition+= (tap_block_length+2);
                if (TapePosition >= SnapshotDataSize)
                {
                        /* Tape loaded so clear op base override */
                        cpu_setOPbaseoverride(0, 0);
                        if (errorlog) fprintf(errorlog, "All tape blocks used!\n");
                }

                /* Leave the load routine by removing addresses from the stack
                   until one outside the load routine is found.
                   eg. SA/LD-RET at address 053f
                */
                do
                {
                        addr = cpu_geturnpc();
                        cpu_set_reg(Z80_PC, (addr & 0x0ffff));

                        sp_reg = cpu_get_reg(Z80_SP);
                        sp_reg += 2;
                        cpu_set_reg(Z80_SP, (sp_reg & 0x0ffff));
                        cpu_set_sp((sp_reg & 0x0ffff));
                } while ((addr != 0x053f) && (addr < 0x0605));
                return addr;
        }
        else
                return -1;
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
        data = (pSnapshot[19] & 0x04) >> 2;
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
                if (errorlog) fprintf(errorlog,"Old 1.45 V of Z80 snapshot found!\n");

                /* program counter is specified. Old 1.45 */
                lo = pSnapshot[6] & 0x0ff;
                hi = pSnapshot[7] & 0x0ff;
                cpu_set_reg(Z80_PC, (hi << 8) | lo);


                if ((pSnapshot[12] & 0x020)==0)
                {
                        if (errorlog) fprintf(errorlog, "Not compressed\n");

                        /* not compressed */
                       for (i = 0; i < 49152; i++)
                       {
                          cpu_writemem16(i+16384, pSnapshot[30 + i]);
                       }
                }
                else
                {
                        if (errorlog) fprintf(errorlog, "Compressed\n");

                   /* compressed */
                   spectrum_z80_decompress_block(pSnapshot+30, 16384, 49152);


                }
         }
         else
         {
                unsigned char *pSource;
                int header_size;

                if (errorlog) fprintf(errorlog, "v2.0+ V of Z80 snapshot found!\n");

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
                                if (errorlog) fprintf(errorlog, "Not compressed\n");

                                /* not compressed */
                               for (i = 0; i < 49152; i++)
                               {
                                  cpu_writemem16(i+16384, pSnapshot[30 + i]);
                               }


                        }
                        else
                        {
                               if (errorlog) fprintf(errorlog, "Compressed\n");

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

/*******************************************************************
 *
 *      Returns 1 if the specified z80 snapshot is a 48K snapshot
 *      and 0 if it is a Spectrum 128K or SamRam snapshot.
 *
 *******************************************************************/
int is48k_z80snapshot(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
        unsigned char lo,hi, data;

        if (SnapshotSize < 30)
                return 0;       /* Invalid file */

        lo = pSnapshot[6] & 0x0ff;
        hi = pSnapshot[7] & 0x0ff;
        if (hi || lo)
                return 1;       /* V1.45 - 48K only */

        lo = pSnapshot[30] & 0x0ff;
        hi = pSnapshot[31] & 0x0ff;
        data = pSnapshot[34] & 0x0ff;   /* Hardware mode */

        if ((hi == 0) && (lo == 23))
        {       /* V2.01 format */
                if ((data == 0) || (data == 1))
                        return 1;
        }
        else
        {       /* V3.0 format */
                if ((data == 0) || (data == 1) || (data == 3))
                        return 1;
        }
        return 0;
}

int spectrum_rom_id(int id)
{
	return 1;
}

/*-----------------27/02/00 10:54-------------------
 SPECTRUM WAVE CASSETTE SUPPORT
--------------------------------------------------*/

int spectrum_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAILED;

		return INIT_OK;
	}

	/* HJB 02/18: no file, create a new file instead */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 22050; /* maybe 11025 Hz would be sufficient? */
		/* open in write mode */
        if (device_open(IO_CASSETTE, id, 1, &wa))
            return INIT_FAILED;
		return INIT_OK;
    }

	return INIT_FAILED;
}

void spectrum_cassette_exit(int id)
{
		device_close(IO_CASSETTE, id);
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

void spectrum_nmi_generate(int param)
{
	cpu_cause_interrupt(0, Z80_NMI_INT);
}


