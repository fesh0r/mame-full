/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Changes:

  KT 31/1/00 - Added support for .Z80. At the moment only 48k files are supported!
  DJR 8/2/00 - Added checks to avoid trying to load 128K .Z80 files into 48K machine!
  DJR 20/2/00 - Added support for .TAP files.
  -----------------27/02/00 10:54-------------------
  KT 27/2/00 - Added my changes for the WAV support
  --------------------------------------------------
  DJR 14/3/00 - Fixed +3 tape loading and added option to 'rewind' tapes when end reached.
  DJR 21/4/00 - Added support for 128K .SNA and .Z80 files.
  DJR 21/4/00 - Ensure 48K Basic ROM is used when running 48K snapshots on 128K machine.
  DJR 03/5/00 - Fixed bug of not decoding last byte of .Z80 blocks.
  DJR 08/5/00 - Fixed TS2068 .TAP loading.
  DJR 19/5/00 - .TAP files are now classified as cassette files.
  DJR 02/6/00 - Added support for .SCR files (screendumps).

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "vidhrdw/border.h"
#include "cassette.h"

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

static unsigned char *pSnapshotData = NULL;
static unsigned long SnapshotDataSize = 0;
static unsigned long TapePosition = 0;
static void spectrum_setup_sna(unsigned char *pSnapshot, unsigned long SnapshotSize);
static void spectrum_setup_z80(unsigned char *pSnapshot, unsigned long SnapshotSize);
static void spectrum_setup_sp(unsigned char *pSnapshot, unsigned long SnapshotSize);
static OPBASE_HANDLER(spectrum_snapshot_opbaseoverride);
static OPBASE_HANDLER(spectrum_tape_opbaseoverride);
OPBASE_HANDLER(spectrum_opbaseoverride);

TIMEX_CART_TYPE timex_cart_type = TIMEX_CART_NONE;
UINT8 timex_cart_chunks = 0x00;
UINT8 * timex_cart_data;

typedef enum
{
	SPECTRUM_SNAPSHOT_NONE,
	SPECTRUM_SNAPSHOT_SNA,
	SPECTRUM_SNAPSHOT_Z80,
	SPECTRUM_SNAPSHOT_SP,
	SPECTRUM_TAPEFILE_TAP
}
SPECTRUM_SNAPSHOT_TYPE;

static SPECTRUM_SNAPSHOT_TYPE spectrum_snapshot_type = SPECTRUM_SNAPSHOT_NONE;

static struct
{
	const char *name;
	unsigned short addr;
	UINT8 *data;
	int length;
} quick;

OPBASE_HANDLER(spectrum_opbaseoverride)
{
	/* Execute code in spectrum 48k screen memory */
	if ((spectrum_128_port_7ffd_data == -1) && (spectrum_plus3_port_1ffd_data == -1))
	{
		if (address>=0x4000 && address<=0x57ff)
		{
			OP_ROM = OP_RAM = spectrum_characterram-0x4000;
			return -1;
		}
		if (address>=0x5800 && address<=0x5aff)
		{
			OP_ROM = OP_RAM = spectrum_colorram-0x5800;
			return -1;
		}
	}

	/* Hack for correct handling 0xffff interrupt vector */
	if (address == 0x0001)
		if (cpu_get_reg(REG_PREVIOUSPC)==0xffff)
		{
			cpu_set_reg(Z80_PC, 0xfff4);
			return 0xfff4;
		}
	return address;
}

void spectrum_init_machine(void)
{
	memory_set_opbase_handler(0, spectrum_opbaseoverride);
	if (pSnapshotData != NULL)
	{
		if (spectrum_snapshot_type == SPECTRUM_TAPEFILE_TAP)
		{
			logerror(".TAP file support enabled\n");
			memory_set_opbase_handler(0, spectrum_tape_opbaseoverride);
		}
		else
			memory_set_opbase_handler(0, spectrum_snapshot_opbaseoverride);
	}
}

void spectrum_shutdown_machine(void)
{
}

int spectrum_snap_load(int id)
{
	void *file;

	file = image_fopen(IO_SNAPSHOT, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);

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

				if (!stricmp(device_filename(IO_SNAPSHOT, id) + strlen(device_filename(IO_SNAPSHOT, id) ) - 4, ".sna"))
				{
					if ((SnapshotDataSize != 49179) && (SnapshotDataSize != 131103) && (SnapshotDataSize != 14787))
					{
						logerror("Invalid .SNA file size.\n");
						return 1;
					}
					spectrum_snapshot_type = SPECTRUM_SNAPSHOT_SNA;
				}
				else
				{
					if (!stricmp(device_filename(IO_SNAPSHOT, id) + strlen(device_filename(IO_SNAPSHOT, id) ) - 3, ".sp"))
					{
						if (data[0]!='S' && data[1]!='P')
						{
							logerror("Invalid .SP signature.\n");
							return 1;
						}
						spectrum_snapshot_type = SPECTRUM_SNAPSHOT_SP;
					}
					else
						spectrum_snapshot_type = SPECTRUM_SNAPSHOT_Z80;
				}
				logerror("File loaded!\n");
				return 0;
			}
			osd_fclose(file);
		}
		return 1;
	}
	return 0;
}

void spectrum_snap_exit(int id)
{
	if (pSnapshotData != NULL)
	{
		/* free snapshot/tape data */
		free(pSnapshotData);
		pSnapshotData = NULL;

		/* ensure op base is cleared */
		memory_set_opbase_handler(0, spectrum_opbaseoverride);
	}

	/* reset type to none */
	spectrum_snapshot_type = SPECTRUM_SNAPSHOT_NONE;
}


static OPBASE_HANDLER(spectrum_snapshot_opbaseoverride)
{
	/* clear op base override */
	memory_set_opbase_handler(0, spectrum_opbaseoverride);

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

		case SPECTRUM_SNAPSHOT_SP:
			{
				/* .SP */
				spectrum_setup_sp(pSnapshotData, SnapshotDataSize);
			}
			break;

		default:
			/* SPECTRUM_TAPEFILE_TAP is handled by spectrum_tape_opbaseoverride */
			break;
		}
	}
	logerror("Snapshot loaded - new PC = %04x\n", cpu_get_reg(Z80_PC) & 0x0ffff);

	return (cpu_get_reg(Z80_PC) & 0x0ffff);
}

/*******************************************************************
 *
 *      Override load routine (0x0556 in 48K ROM) if loading .TAP files
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
static OPBASE_HANDLER(spectrum_tape_opbaseoverride)
{
	int i, tap_block_length, load_length;
	unsigned char lo, hi, a_reg;
	unsigned short load_addr, return_addr, af_reg, de_reg, sp_reg;
	static int data_loaded = 0;			/* Whether any data files (not headers) were loaded */

/*        logerror("PC=%02x\n", address); */
	/* It is not always possible to trap the call to the actual load
	 * routine so trap the LD-EDGE-1 and LD-EDGE-2 routines which
	 * check the earphone socket.
	 */

	if ((spectrum_128_port_7ffd_data == -1) && (spectrum_plus3_port_1ffd_data == -1))
	{
		if (address>=0x4000 && address<=0x57ff)
		{
			OP_ROM = OP_RAM = spectrum_characterram-0x4000;
			return -1;
		}
		if (address>=0x5800 && address<=0x5aff)
		{
			OP_ROM = OP_RAM = spectrum_colorram-0x5800;
			return -1;
		}
	}
	if (address == 0x0001)
		if (cpu_get_reg(REG_PREVIOUSPC)==0xffff)
		{
			cpu_set_reg(Z80_PC, 0xfff4);
			return 0xfff4;
		}

	if (ts2068_port_f4_data == -1)
	{
		if ((address < 0x05e3) || (address > 0x0604))
			return address;

		/* For Spectrum 128/+2/+3 check which rom is paged */
		if ((spectrum_128_port_7ffd_data != -1) || (spectrum_plus3_port_1ffd_data != -1))
		{
			if (spectrum_plus3_port_1ffd_data != -1)
			{
				if (!spectrum_plus3_port_1ffd_data & 0x04)
					return address;
			}
			if (!spectrum_128_port_7ffd_data & 0x10)
				return address;
		}
	}
	else
	{
		/* For TS2068 also check that EXROM is paged into bottom 8K.
		 * Code is not relocatable so don't need to check EXROM in other pages.
		 */
		if ((!ts2068_port_f4_data & 0x01) || (!ts2068_port_ff_data & 0x80))
			return address;
		if ((address < 0x018d) || (address > 0x01aa))
			return address;
	}
	lo = pSnapshotData[TapePosition] & 0x0ff;
	hi = pSnapshotData[TapePosition + 1] & 0x0ff;
	tap_block_length = (hi << 8) | lo;

	/* By the time that load has been trapped the block type and carry
	 * flags are in the AF' register. */
	af_reg = cpu_get_reg(Z80_AF2);
	a_reg = (af_reg & 0xff00) >> 8;

	if ((a_reg == pSnapshotData[TapePosition + 2]) && (af_reg & 0x0001))
	{
		/* Correct flag byte and carry flag set so try loading */
		load_addr = cpu_get_reg(Z80_IX);
		de_reg = cpu_get_reg(Z80_DE);

		load_length = MIN(de_reg, tap_block_length - 2);
		load_length = MIN(load_length, 65536 - load_addr);
		/* Actual number of bytes of block that can be loaded -
		 * Don't try to load past the end of memory */

		for (i = 0; i < load_length; i++)
			cpu_writemem16(load_addr + i, pSnapshotData[TapePosition + i + 3]);
		cpu_set_reg(Z80_IX, load_addr + load_length);
		cpu_set_reg(Z80_DE, de_reg - load_length);
		if (de_reg == (tap_block_length - 2))
		{
			/* Successful load - Set carry flag and A to 0 */
			if ((de_reg != 17) || (a_reg))
				data_loaded = 1;		/* Non-header file loaded */
			cpu_set_reg(Z80_AF, (af_reg & 0x00ff) | 0x0001);
			logerror("Loaded %04x bytes from address %04x onwards (type=%02x) using tape block at offset %ld\n", load_length,
					 load_addr, a_reg, TapePosition);
		}
		else
		{
			/* Wrong tape block size - reset carry flag */
			cpu_set_reg(Z80_AF, af_reg & 0xfffe);
			logerror("Bad block length %04x bytes wanted starting at address %04x (type=%02x) , Data length of tape block at offset %ld is %04x bytes\n",
					 de_reg, load_addr, a_reg, TapePosition, tap_block_length - 2);
		}
	}
	else
	{
		/* Wrong flag byte or verify selected so reset carry flag to indicate failure */
		cpu_set_reg(Z80_AF, af_reg & 0xfffe);
		if (af_reg & 0x0001)
			logerror("Failed to load tape block at offset %ld - type wanted %02x, got type %02x\n", TapePosition, a_reg,
					 pSnapshotData[TapePosition + 2]);
		else
			logerror("Failed to load tape block at offset %ld - verify selected\n", TapePosition);
	}

	TapePosition += (tap_block_length + 2);
	if (TapePosition >= SnapshotDataSize)
	{
		/* End of tape - either rewind or disable op base override */
		if (readinputport(16) & 0x40)
		{
			if (data_loaded)
			{
				TapePosition = 0;
				data_loaded = 0;
				logerror("All tape blocks used! - rewinding tape to start\n");
			}
			else
			{
				/* Disable .TAP support if no files were loaded to avoid getting caught in infinite loop */
				memory_set_opbase_handler(0, spectrum_opbaseoverride);
				logerror("No valid data loaded! - disabling .TAP support\n");
			}
		}
		else
		{
			memory_set_opbase_handler(0, spectrum_opbaseoverride);
			logerror("All tape blocks used! - disabling .TAP support\n");
		}
	}

	/* Leave the load routine by removing addresses from the stack
	 * until one outside the load routine is found.
	 * eg. SA/LD-RET at address 053f (00e5 on TS2068)
	 */
	do
	{
		return_addr = cpu_geturnpc();
		cpu_set_reg(Z80_PC, (return_addr & 0x0ffff));

		sp_reg = cpu_get_reg(Z80_SP);
		sp_reg += 2;
		cpu_set_reg(Z80_SP, (sp_reg & 0x0ffff));
		activecpu_set_sp((sp_reg & 0x0ffff));
	}
	while (((return_addr != 0x053f) && (return_addr < 0x0605) && (ts2068_port_f4_data == -1)) ||
		   ((return_addr != 0x00e5) && (return_addr < 0x01aa) && (ts2068_port_f4_data != -1)));

       	logerror("Load return address=%04x, SP=%04x\n", return_addr, sp_reg);

	return return_addr;
}

/*******************************************************************
 *
 *      Update the memory and paging of the spectrum being emulated
 *
 *      if port_7ffd_data is -1 then machine is 48K - no paging
 *      if port_1ffd_data is -1 then machine is 128K
 *      if neither port is -1 then machine is +2a/+3
 *
 *      Note: the 128K .SNA and .Z80 file formats do not store the
 *      port 1FFD setting so it is necessary to calculate the appropriate
 *      value for the ROM paging.
 *
 *******************************************************************/
static void spectrum_update_paging(void)
{
	if (spectrum_128_port_7ffd_data == -1)
		return;
	if (spectrum_plus3_port_1ffd_data == -1)
		spectrum_128_update_memory();

	else
	{
		if (spectrum_128_port_7ffd_data & 0x10)
			/* Page in Spec 48K basic ROM */
			spectrum_plus3_port_1ffd_data = 0x04;
		else
			spectrum_plus3_port_1ffd_data = 0;
		spectrum_plus3_update_memory();
	}
}

/* Page in the 48K Basic ROM. Used when running 48K snapshots on a 128K machine. */
static void spectrum_page_basicrom(void)
{
	if (spectrum_128_port_7ffd_data == -1)
		return;
	spectrum_128_port_7ffd_data |= 0x10;
	spectrum_update_paging();
}

/* Dump the state of registers after loading a snapshot to the log file for debugging */
static void dump_registers(void)
{
	logerror("PC   = %04x\n", cpu_get_reg(Z80_PC));
	logerror("SP   = %04x\n", cpu_get_reg(Z80_SP));
	logerror("AF   = %04x\n", cpu_get_reg(Z80_AF));
	logerror("BC   = %04x\n", cpu_get_reg(Z80_BC));
	logerror("DE   = %04x\n", cpu_get_reg(Z80_DE));
	logerror("HL   = %04x\n", cpu_get_reg(Z80_HL));
	logerror("IX   = %04x\n", cpu_get_reg(Z80_IX));
	logerror("IY   = %04x\n", cpu_get_reg(Z80_IY));
	logerror("AF'  = %04x\n", cpu_get_reg(Z80_AF2));
	logerror("BC'  = %04x\n", cpu_get_reg(Z80_BC2));
	logerror("DE'  = %04x\n", cpu_get_reg(Z80_DE2));
	logerror("HL'  = %04x\n", cpu_get_reg(Z80_HL2));
	logerror("I    = %02x\n", cpu_get_reg(Z80_I));
	logerror("R    = %02x\n", cpu_get_reg(Z80_R));
	logerror("IFF1 = %02x\n", cpu_get_reg(Z80_IFF1));
	logerror("IFF2 = %02x\n", cpu_get_reg(Z80_IFF2));
	logerror("IM   = %02x\n", cpu_get_reg(Z80_IM));
	logerror("NMI  = %02x\n", cpu_get_reg(Z80_NMI_STATE));
	logerror("IRQ  = %02x\n", cpu_get_reg(Z80_IRQ_STATE));
}

/*******************************************************************
 *
 *      Load a .SP file.
 *
 *      There are only 48K .SP files and their format is as follows:
 *      Offset  Size    Description (all registers stored with LSB first)
 *      0       2       "SP" (signature)
 *      2       2       Program length in bytes (normally 49152 for 48K snaps, or 16384 for 16K snaps)
 *      4       2       Program location (normally 16384)
 *      6       8       BC,DE,HL,AF
 *      14      4       IX,IY
 *      18      8       BC',DE',HL',AF'
 *      26      2       R,I
 *      28      4       SP,PC
 *      32      2       0 (reserved for future use)
 *      34      1	Border color
 *      35      1       0 (reserved for future use)
 *      36      2       Status word
 *      38      -       RAM dump
 *
 *	Status word:
 *	Bit	Description
 *	15-8	Reserved for future use
 *	7-6	Reserved for internal use (0)
 *	5	Flash: 0=INK/1=PAPER
 *	4	Interrupt pending for execution
 *	3	If 1, IM 0; if 0, bit 1 determines interrupt mode
 *	      	(Spectrum v 0.99e had this behaviour reversed, and this
 *		bit was not used in versions previous to v 0.99e)
 *	2	IFF2 (internal use)
 *	1	Interrupt Mode (if bit 3 reset): 0=>IM1, 1=>IM2
 *	0	IFF1: 0=DI/1=EI
 *
 *******************************************************************/
void spectrum_setup_sp(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
	UINT8 lo, hi, data;
	UINT16 offset, size;
	UINT16 status;

	lo = pSnapshot[2] & 0x0ff;
	hi = pSnapshot[3] & 0x0ff;
	size = (hi << 8) | lo;
	logerror ("Snapshot data size: %d.\n", size);
	lo = pSnapshot[4] & 0x0ff;
	hi = pSnapshot[5] & 0x0ff;
	offset = (hi << 8) | lo;
	logerror ("Snapshot data offset: %d.\n", offset);

	lo = pSnapshot[6] & 0x0ff;
	hi = pSnapshot[7] & 0x0ff;
	cpu_set_reg(Z80_BC, (hi << 8) | lo);
	lo = pSnapshot[8] & 0x0ff;
	hi = pSnapshot[9] & 0x0ff;
	cpu_set_reg(Z80_DE, (hi << 8) | lo);
	lo = pSnapshot[10] & 0x0ff;
	hi = pSnapshot[11] & 0x0ff;
	cpu_set_reg(Z80_HL, (hi << 8) | lo);
	lo = pSnapshot[12] & 0x0ff;
	hi = pSnapshot[13] & 0x0ff;
	cpu_set_reg(Z80_AF, (hi << 8) | lo);

	lo = pSnapshot[14] & 0x0ff;
	hi = pSnapshot[15] & 0x0ff;
	cpu_set_reg(Z80_IX, (hi << 8) | lo);
	lo = pSnapshot[16] & 0x0ff;
	hi = pSnapshot[17] & 0x0ff;
	cpu_set_reg(Z80_IY, (hi << 8) | lo);

	lo = pSnapshot[18] & 0x0ff;
	hi = pSnapshot[19] & 0x0ff;
	cpu_set_reg(Z80_BC2, (hi << 8) | lo);
	lo = pSnapshot[20] & 0x0ff;
	hi = pSnapshot[21] & 0x0ff;
	cpu_set_reg(Z80_DE2, (hi << 8) | lo);
	lo = pSnapshot[22] & 0x0ff;
	hi = pSnapshot[23] & 0x0ff;
	cpu_set_reg(Z80_HL2, (hi << 8) | lo);
	lo = pSnapshot[24] & 0x0ff;
	hi = pSnapshot[25] & 0x0ff;
	cpu_set_reg(Z80_AF2, (hi << 8) | lo);

	cpu_set_reg(Z80_R, (pSnapshot[26] & 0x0ff));
	cpu_set_reg(Z80_I, (pSnapshot[27] & 0x0ff));

	lo = pSnapshot[28] & 0x0ff;
	hi = pSnapshot[29] & 0x0ff;
	cpu_set_reg(Z80_SP, (hi << 8) | lo);
	lo = pSnapshot[30] & 0x0ff;
	hi = pSnapshot[31] & 0x0ff;
	cpu_set_reg(Z80_PC, (hi << 8) | lo);

	lo = pSnapshot[32] & 0x0ff;
	hi = pSnapshot[33] & 0x0ff;
	if (((hi << 8) | lo)!=0)
		logerror("Unknown meaning of word on position 32: %04x.\n", (hi << 8) | lo);

	/* Set border colour */
	PreviousFE = (PreviousFE & 0xf8) | (pSnapshot[34] & 0x07);
	EventList_Reset();
	set_last_border_color(pSnapshot[34] & 0x07);
	force_border_redraw();

	if ((pSnapshot[35]&0x0ff)!=0)
		logerror("Unknown meaning of byte on position 35: %02x.\n", pSnapshot[35]&0x0ff);

        lo = pSnapshot[36] & 0x0ff;
	hi = pSnapshot[37] & 0x0ff;
	status = (hi << 8) | lo;

	data = (status & 0x01);
	cpu_set_reg(Z80_IFF1, data);
	data = (status & 0x04)>>2;
	cpu_set_reg(Z80_IFF2, data);

	data = (status & 0x08)>>3;
	if (data)
		cpu_set_reg(Z80_IM, 0);
	else
	{
		data = (status & 0x02)>>1;
		if (data)
			cpu_set_reg(Z80_IM, 2);
		else
			cpu_set_reg(Z80_IM, 1);
	}

	data = (status & 0x10)>>4;
	cpu_set_reg(Z80_IRQ_STATE, data);

	cpu_set_reg(Z80_NMI_STATE, 0);
	cpu_set_reg(Z80_HALT, 0);

	spectrum_page_basicrom();

	/* memory dump */
	for (i = 0; i < size; i++)
		cpu_writemem16(i + offset, pSnapshot[38 + i]);

	dump_registers();
}

/*******************************************************************
 *
 *      Load a 48K or 128K .SNA file.
 *
 *      48K Format as follows:
 *      Offset  Size    Description (all registers stored with LSB first)
 *      0       1       I
 *      1       18      HL',DE',BC',AF',HL,DE,BC,IY,IX
 *      19      1       Interrupt (bit 2 contains IFF2 1=EI/0=DI
 *      20      1       R
 *      21      4       AF,SP
 *      25      1       Interrupt Mode (0=IM0/1=IM1/2=IM2)
 *      26      1       Border Colour (0..7)
 *      27      48K     RAM dump 0x4000-0xffff
 *      PC is stored on stack.
 *
 *      128K Format as follows:
 *      Offset  Size    Description
 *      0       27      Header as 48K
 *      27      16K     RAM bank 5 (0x4000-0x7fff)
 *      16411   16K     RAM bank 2 (0x8000-0xbfff)
 *      32795   16K     RAM bank n (0xc000-0xffff - currently paged bank)
 *      49179   2       PC
 *      49181   1       port 7FFD setting
 *      49182   1       TR-DOS rom paged (1=yes)
 *      49183   16K     remaining RAM banks in ascending order
 *
 *      The bank in 0xc000 is always included even if it is page 2 or 5
 *      in which case it is included twice.
 *
 *******************************************************************/
void spectrum_setup_sna(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i, j, usedbanks[8];
	long bank_offset;
	unsigned char lo, hi, data;
	unsigned short addr;

	if ((SnapshotDataSize != 49179) && (spectrum_128_port_7ffd_data == -1))
	{
		logerror("Can't load 128K .SNA file into 48K Machine\n");
		return;
	}

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
	activecpu_set_sp((hi << 8) | lo);
	data = (pSnapshot[25] & 0x0ff);
	cpu_set_reg(Z80_IM, data);

	/* Set border colour */
	PreviousFE = (PreviousFE & 0xf8) | (pSnapshot[26] & 0x07);
	EventList_Reset();
	set_last_border_color(pSnapshot[26] & 0x07);
	force_border_redraw();

	cpu_set_reg(Z80_NMI_STATE, 0);
	cpu_set_reg(Z80_IRQ_STATE, 0);
	cpu_set_reg(Z80_HALT, 0);

	if (SnapshotDataSize == 49179)
		/* 48K Snapshot */
		spectrum_page_basicrom();
	else
	{
		/* 128K Snapshot */
		spectrum_128_port_7ffd_data = (pSnapshot[49181] & 0x0ff);
		spectrum_update_paging();
	}

	/* memory dump */
	for (i = 0; i < 49152; i++)
	{
		cpu_writemem16(i + 16384, pSnapshot[27 + i]);
	}

	if (SnapshotDataSize == 49179)
	{
		/* get pc from stack */
		addr = cpu_geturnpc();
		cpu_set_reg(Z80_PC, (addr & 0x0ffff));

		addr = cpu_get_reg(Z80_SP);
		addr += 2;
		cpu_set_reg(Z80_SP, (addr & 0x0ffff));
		activecpu_set_sp((addr & 0x0ffff));
	}
	else
	{
		/* Set up other RAM banks */
		bank_offset = 49183;
		for (i = 0; i < 8; i++)
			usedbanks[i] = 0;

		usedbanks[5] = 1;				/* 0x4000-0x7fff */
		usedbanks[2] = 1;				/* 0x8000-0xbfff */
		usedbanks[spectrum_128_port_7ffd_data & 0x07] = 1;	/* Banked memory */

		for (i = 0; i < 8; i++)
		{
			if (!usedbanks[i])
			{
				logerror("Loading bank %d from offset %ld\n", i, bank_offset);
				spectrum_128_port_7ffd_data &= 0xf8;
				spectrum_128_port_7ffd_data += i;
				spectrum_update_paging();
				for (j = 0; j < 16384; j++)
					cpu_writemem16(j + 49152, pSnapshot[bank_offset + j]);
				bank_offset += 16384;
			}
		}

		/* Reset paging */
		spectrum_128_port_7ffd_data = (pSnapshot[49181] & 0x0ff);
		spectrum_update_paging();

		/* program counter */
		lo = pSnapshot[49179] & 0x0ff;
		hi = pSnapshot[49180] & 0x0ff;
		cpu_set_reg(Z80_PC, (hi << 8) | lo);
	}
	dump_registers();
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
		 * single 0x0ed */
		if (ch == (unsigned char) 0x0ed)
		{
			if (pSource[1] == (unsigned char) 0x0ed)
			{

				/* 0x0ed, 0x0ed, xx yy */
				/* repetition */

				int count;
				int data;

				count = (pSource[2] & 0x0ff);

				if (count == 0)
					return;

				data = (pSource[3] & 0x0ff);

				pSource += 4;

				if (count > size)
					count = size;

				size -= count;

				for (i = 0; i < count; i++)
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
	while (size > 0);
}

typedef enum {
	SPECTRUM_Z80_SNAPSHOT_INVALID,
	SPECTRUM_Z80_SNAPSHOT_48K_OLD,
	SPECTRUM_Z80_SNAPSHOT_48K,
	SPECTRUM_Z80_SNAPSHOT_SAMRAM,
	SPECTRUM_Z80_SNAPSHOT_128K,
	SPECTRUM_Z80_SNAPSHOT_TS2068
}
SPECTRUM_Z80_SNAPSHOT_TYPE;

SPECTRUM_Z80_SNAPSHOT_TYPE spectrum_identify_z80 (unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	unsigned char lo, hi, data;

	if (SnapshotSize < 30)
		return SPECTRUM_Z80_SNAPSHOT_INVALID;	/* Invalid file */

	lo = pSnapshot[6] & 0x0ff;
	hi = pSnapshot[7] & 0x0ff;
	if (hi || lo)
		return SPECTRUM_Z80_SNAPSHOT_48K_OLD;	/* V1.45 - 48K only */

	lo = pSnapshot[30] & 0x0ff;
	hi = pSnapshot[31] & 0x0ff;
	data = pSnapshot[34] & 0x0ff;			/* Hardware mode */

	if ((hi == 0) && (lo == 23))
	{						/* V2.01 */							/* V2.01 format */
		switch (data)
		{
			case 0:
			case 1:	return SPECTRUM_Z80_SNAPSHOT_48K;
			case 2:	return SPECTRUM_Z80_SNAPSHOT_SAMRAM;
			case 3:	
			case 4:	return SPECTRUM_Z80_SNAPSHOT_128K;
			case 128: return SPECTRUM_Z80_SNAPSHOT_TS2068;
		}
	}

	if ((hi == 0) && (lo == 54))
	{						/* V3.0x */							/* V2.01 format */
		switch (data)
		{
			case 0:
			case 1:
			case 3:	return SPECTRUM_Z80_SNAPSHOT_48K;
			case 2:	return SPECTRUM_Z80_SNAPSHOT_SAMRAM;
			case 4:
			case 5:
			case 6:	return SPECTRUM_Z80_SNAPSHOT_128K;
			case 128: return SPECTRUM_Z80_SNAPSHOT_TS2068;
		}
	}

	return SPECTRUM_Z80_SNAPSHOT_INVALID;
}

/* now supports 48k & 128k .Z80 files */
void spectrum_setup_z80(unsigned char *pSnapshot, unsigned long SnapshotSize)
{
	int i;
	unsigned char lo, hi, data;
	SPECTRUM_Z80_SNAPSHOT_TYPE z80_type;

	z80_type = spectrum_identify_z80(pSnapshotData, SnapshotDataSize);

	switch (z80_type)
	{
		case SPECTRUM_Z80_SNAPSHOT_INVALID:
				logerror("Invalid .Z80 file\n");
				return;
		case SPECTRUM_Z80_SNAPSHOT_48K_OLD:
		case SPECTRUM_Z80_SNAPSHOT_48K:
				logerror("48K .Z80 file\n");
				if (!strcmp(Machine->gamedrv->name,"ts2068"))
					logerror("48K .Z80 file in TS2068\n");
				break;
		case SPECTRUM_Z80_SNAPSHOT_128K:
				logerror("128K .Z80 file\n");
				if (spectrum_128_port_7ffd_data == -1)
				{
					logerror("Not a 48K .Z80 file\n");
					return;
				}
				if (!strcmp(Machine->gamedrv->name,"ts2068"))
				{
					logerror("Not a TS2068 .Z80 file\n");
					return;
				}
				break;
		case SPECTRUM_Z80_SNAPSHOT_TS2068:
				logerror("TS2068 .Z80 file\n");				if (strcmp(Machine->gamedrv->name,"ts2068"))
				if (strcmp(Machine->gamedrv->name,"ts2068"))
					logerror("Not a TS2068 machine\n");
				break;
		case SPECTRUM_Z80_SNAPSHOT_SAMRAM:
				logerror("Hardware not supported - .Z80 file\n");
				return;
	}

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

	/* SP */
	lo = pSnapshot[8] & 0x0ff;
	hi = pSnapshot[9] & 0x0ff;
	cpu_set_reg(Z80_SP, (hi << 8) | lo);
	activecpu_set_sp((hi << 8) | lo);

	/* I */
	cpu_set_reg(Z80_I, (pSnapshot[10] & 0x0ff));

	/* R */
	data = (pSnapshot[11] & 0x07f) | ((pSnapshot[12] & 0x01) << 7);
	cpu_set_reg(Z80_R, data);

	/* Set border colour */
	PreviousFE = (PreviousFE & 0xf8) | ((pSnapshot[12] & 0x0e) >> 1);
	EventList_Reset();
	set_last_border_color((pSnapshot[12] & 0x0e) >> 1);
	force_border_redraw();

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
	if (pSnapshot[27] == 0)
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
	if (pSnapshot[28] != 0)
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

	if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K_OLD)
	{
		lo = pSnapshot[6] & 0x0ff;
		hi = pSnapshot[7] & 0x0ff;
		cpu_set_reg(Z80_PC, (hi << 8) | lo);

		spectrum_page_basicrom();
                
		if ((pSnapshot[12] & 0x020) == 0)
		{
			logerror("Not compressed\n");	/* not compressed */
			for (i = 0; i < 49152; i++)
				cpu_writemem16(i + 16384, pSnapshot[30 + i]);
		}
		else
		{
			logerror("Compressed\n");	/* compressed */
			spectrum_z80_decompress_block(pSnapshot + 30, 16384, 49152);
		}
	}
	else
	{
		unsigned char *pSource;
		int header_size;

		header_size = 30 + 2 + ((pSnapshot[30] & 0x0ff) | ((pSnapshot[31] & 0x0ff) << 8));

		lo = pSnapshot[32] & 0x0ff;
		hi = pSnapshot[33] & 0x0ff;
		cpu_set_reg(Z80_PC, (hi << 8) | lo);

		if ((z80_type == SPECTRUM_Z80_SNAPSHOT_128K) || ((z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068) && !strcmp(Machine->gamedrv->name,"ts2068")))
		{
			/* Only set up sound registers for 128K machine or TS2068! */
			for (i = 0; i < 16; i++)
			{
				AY8910_control_port_0_w(0, i);
				AY8910_write_port_0_w(0, pSnapshot[39 + i]);
			}
			AY8910_control_port_0_w(0, pSnapshot[38]);
		}

		pSource = pSnapshot + header_size;

		if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K)
			/* Ensure 48K Basic ROM is used */
			spectrum_page_basicrom();

		do
		{
			unsigned short length;
			unsigned char page;
			int Dest = 0;

			length = (pSource[0] & 0x0ff) | ((pSource[1] & 0x0ff) << 8);
			page = pSource[2];

			if (z80_type == SPECTRUM_Z80_SNAPSHOT_48K || z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068)
			{
				switch (page)
				{
					case 4:	Dest = 0x08000;	break;
					case 5:	Dest = 0x0c000;	break;
					case 8:	Dest = 0x04000;	break;
					default: Dest = 0; break;
				}
			}
			else
			{
				/* 3 = bank 0, 4 = bank 1 ... 10 = bank 7 */
				if ((page >= 3) && (page <= 10))
				{
					/* Page the appropriate bank into 0xc000 - 0xfff */
					spectrum_128_port_7ffd_data = page - 3;
					spectrum_update_paging();
					Dest = 0x0c000;
				}
				else
					/* Other values correspond to ROM pages */
					Dest = 0x0;
			}

			if (Dest != 0)
			{
				if (length == 0x0ffff)
				{
					/* block is uncompressed */
					logerror("Not compressed\n");

					/* not compressed */
					for (i = 0; i < 16384; i++)
						cpu_writemem16(i + Dest, pSource[i]);
				}
				else
				{
					logerror("Compressed\n");

					/* block is compressed */
					spectrum_z80_decompress_block(&pSource[3], Dest, 16384);
				}
			}

			/* go to next block */
			pSource += (3 + length);
		}
		while (((unsigned long) pSource - (unsigned long) pSnapshot) < SnapshotDataSize);

		if ((spectrum_128_port_7ffd_data != -1) && (z80_type != SPECTRUM_Z80_SNAPSHOT_48K))
		{
			/* Set up paging */
			spectrum_128_port_7ffd_data = (pSnapshot[35] & 0x0ff);
			spectrum_update_paging();
		}
		if ((z80_type == SPECTRUM_Z80_SNAPSHOT_48K) && !strcmp(Machine->gamedrv->name,"ts2068"))
		{
			ts2068_port_f4_data = 0x03;
			ts2068_port_ff_data = 0x00;
			ts2068_update_memory();
		}
		if (z80_type == SPECTRUM_Z80_SNAPSHOT_TS2068 && !strcmp(Machine->gamedrv->name,"ts2068"))
		{
			ts2068_port_f4_data = pSnapshot[35];
			ts2068_port_ff_data = pSnapshot[36];
			ts2068_update_memory();
		}
	}
	dump_registers();
}

/*-----------------27/02/00 10:54-------------------
 SPECTRUM WAVE CASSETTE SUPPORT
--------------------------------------------------*/

int spectrum_cassette_init(int id)
{
	void *file;
	struct cassette_args args;

	if ((device_filename(IO_CASSETTE, id) != NULL) &&
		!stricmp(device_filename(IO_CASSETTE, id) + strlen(device_filename(IO_CASSETTE, id) ) - 4, ".tap"))
	{
		int datasize;
		unsigned char *data;

		file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
		logerror(".TAP file found\n");
		if (file)
			datasize = osd_fsize(file);
		else
			datasize = 0;
		if (datasize != 0)
		{
			data = malloc(datasize);

			if (data != NULL)
			{
				pSnapshotData = data;
				SnapshotDataSize = datasize;

				osd_fread(file, data, datasize);
				osd_fclose(file);

				/* Always reset tape position when loading new tapes */
				TapePosition = 0;
				memory_set_opbase_handler(0, spectrum_tape_opbaseoverride);
				spectrum_snapshot_type = SPECTRUM_TAPEFILE_TAP;
				logerror(".TAP file successfully loaded\n");
				return INIT_PASS;
			}
		}
		osd_fclose(file);
		return INIT_FAIL;
	}

	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void spectrum_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
	spectrum_snap_exit(id);
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

int spec_quick_init(int id)
{
	FILE *fp;
	int read;

	memset(&quick, 0, sizeof (quick));

	if (device_filename(IO_QUICKLOAD, id) == NULL)
		return INIT_PASS;

/*	quick.name = name; */

	fp = image_fopen(IO_QUICKLOAD, id, OSD_FILETYPE_IMAGE, 0);
	if (!fp)
		return INIT_FAIL;

	quick.length = osd_fsize(fp);
	quick.addr = 0x4000;

	if ((quick.data = malloc(quick.length)) == NULL)
	{
		osd_fclose(fp);
		return INIT_FAIL;
	}
	read = osd_fread(fp, quick.data, quick.length);
	osd_fclose(fp);
	return read != quick.length;
}

void spec_quick_exit(int id)
{
	if (quick.data != NULL)
		free(quick.data);
}

int spec_quick_open(int id, int mode, void *arg)
{
	int i;

	if (quick.data == NULL)
		return 1;

	for (i = 0; i < quick.length; i++)
	{
		cpu_writemem16(i + quick.addr, quick.data[i]);
	}
	logerror("quick loading %s at %.4x size:%.4x\n",
			 device_filename(IO_QUICKLOAD, id), quick.addr, quick.length);

	return 0;
}

int spectrum_cart_load(int id)
{
	void *file;

	logerror("Trying to load cartridge!\n");
	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
	if (file)
	{
		int datasize;
		unsigned char *data, *ROM = memory_region(REGION_CPU1);

		datasize = osd_fsize(file);

		/* Cartridges are always 16K in size (as they replace the BASIC ROM)*/
		if (datasize == 0x4000)
		{
			data = malloc(datasize);
			if (data != NULL)
			{
				osd_fread(file, data, datasize);
				osd_fclose(file);
				memcpy(ROM, data, 0x4000);
				free(data);
				logerror("Cart loaded!\n");
				return 0;
			}
			osd_fclose(file);
		}
		return 1;
	}
	return 0;
}

int timex_cart_load(int id)
{
	void *file;
	int file_size;
	UINT8 * file_data;

	int chunks_in_file = 0;

	int i;

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
		return INIT_PASS;
	}

	logerror ("Trying to load cart\n");

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
	if (file==NULL)
	{
		logerror ("Error opening cart file\n");
		return INIT_FAIL;
	}

	file_size = osd_fsize(file);

	if (file_size < 0x09)
	{
		osd_fclose(file);
		logerror ("Bad file size\n");
		return INIT_FAIL;
	}

	file_data = malloc(file_size);
	if (file_data == NULL)
        {
		osd_fclose(file);
		logerror ("Memory allocating error\n");
		return INIT_FAIL;
	}

	osd_fread(file, file_data, file_size);
	osd_fclose(file);

	for (i=0; i<8; i++)
		if(file_data[i+1]&0x02)	chunks_in_file++;

	if (chunks_in_file*0x2000+0x09 != file_size)
	{
		free (file_data);
		logerror ("File corrupted\n");
		return INIT_FAIL;
	}

	switch (file_data[0x00])
	{
		case 0x00:	logerror ("DOCK cart\n");
				timex_cart_type = TIMEX_CART_DOCK;
				timex_cart_data = (UINT8*) malloc (0x10000);
				if (!timex_cart_data)
				{
					free (file_data);
					logerror ("Memory allocate error\n");
					return INIT_FAIL;
				}
				chunks_in_file = 0;
				for (i=0; i<8; i++)
				{
					timex_cart_chunks = timex_cart_chunks | ((file_data[i+1]&0x01)<<i);
					if (file_data[i+1]&0x02)
					{
						memcpy (timex_cart_data+i*0x2000, file_data+0x09+chunks_in_file*0x2000, 0x2000);
						chunks_in_file++;
					}
					else
					{
						if (file_data[i+1]&0x01)
							memset (timex_cart_data+i*0x2000, 0x00, 0x2000);
						else
							memset (timex_cart_data+i*0x2000, 0xff, 0x2000);
					}
				}
				free (file_data);
				break;

		default:	logerror ("Cart type not supported\n");
				free (file_data);
				timex_cart_type = TIMEX_CART_NONE;
				return INIT_FAIL;
				break;
	}

	logerror ("Cart loaded\n");
	logerror ("Chunks %02x\n", timex_cart_chunks);
	return INIT_PASS;
}

void timex_cart_exit(int id)
{
	if (timex_cart_data)
	{
		free (timex_cart_data);
		timex_cart_data = NULL;
	}
	timex_cart_type = TIMEX_CART_NONE;
	timex_cart_chunks = 0x00;
}
