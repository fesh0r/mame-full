/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

Amstrad hardware consists of:

- General Instruments AY-3-8912 (audio and keyboard scanning)
- Intel 8255PPI (keyboard, access to AY-3-8912, cassette etc)
- Z80A CPU
- 765 FDC (disc drive interface)
- "Gate Array" (custom chip by Amstrad controlling colour, mode,
rom/ram selection


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/m6845.h"
#include "includes/amstrad.h"
//#include "systems/i8255.h"
#include "machine/8255ppi.h"
#include "includes/nec765.h"
#include "includes/dsk.h"

void AmstradCPC_GA_Write(int);
void AmstradCPC_SetUpperRom(int);
void Amstrad_RethinkMemory(void);
void Amstrad_Init(void);
void amstrad_handle_snapshot(unsigned char *);


static unsigned char *snapshot = NULL;

extern unsigned char *Amstrad_Memory;
static int snapshot_loaded;

/* used to setup computer if a snapshot was specified */
OPBASE_HANDLER( amstrad_opbaseoverride )
{
	/* clear op base override */
	memory_set_opbase_handler(0,0);

	if (snapshot_loaded)
	{
		/* its a snapshot file - setup hardware state */
		amstrad_handle_snapshot(snapshot);

		/* free memory */
		free(snapshot);
		snapshot = 0;

		snapshot_loaded = 0;

	}

	return (cpu_get_pc() & 0x0ffff);
}

void amstrad_setup_machine(void)
{
	/* allocate ram - I control how it is accessed so I must
	allocate it somewhere - here will do */
	Amstrad_Memory = malloc(128*1024);
	if(!Amstrad_Memory) return;

	if (snapshot_loaded)
	{
		/* setup for snapshot */
		memory_set_opbase_handler(0,amstrad_opbaseoverride);
	}


	if (!snapshot_loaded)
	{
		Amstrad_Reset();
	}
}



int amstrad_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAIL;

		return INIT_PASS;
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
            return INIT_FAIL;
		return INIT_PASS;
    }

	return INIT_FAIL;
}

void amstrad_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


/* load CPCEMU style snapshots */
void amstrad_handle_snapshot(unsigned char *pSnapshot)
{
	int RegData;
	int i;


	/* init Z80 */
	RegData = (pSnapshot[0x011] & 0x0ff) | ((pSnapshot[0x012] & 0x0ff)<<8);
	cpu_set_reg(Z80_AF, RegData);

	RegData = (pSnapshot[0x013] & 0x0ff) | ((pSnapshot[0x014] & 0x0ff)<<8);
	cpu_set_reg(Z80_BC, RegData);

	RegData = (pSnapshot[0x015] & 0x0ff) | ((pSnapshot[0x016] & 0x0ff)<<8);
	cpu_set_reg(Z80_DE, RegData);

	RegData = (pSnapshot[0x017] & 0x0ff) | ((pSnapshot[0x018] & 0x0ff)<<8);
	cpu_set_reg(Z80_HL, RegData);

	RegData = (pSnapshot[0x019] & 0x0ff) ;
	cpu_set_reg(Z80_R, RegData);

	RegData = (pSnapshot[0x01a] & 0x0ff);
	cpu_set_reg(Z80_I, RegData);

	if ((pSnapshot[0x01b] & 1)==1)
	{
		cpu_set_reg(Z80_IFF1, 1);
	}
	else
	{
		cpu_set_reg(Z80_IFF1, 0);
	}

	if ((pSnapshot[0x01c] & 1)==1)
	{
		cpu_set_reg(Z80_IFF2, 1);
	}
	else
	{
		cpu_set_reg(Z80_IFF2, 0);
	}

	RegData = (pSnapshot[0x01d] & 0x0ff) | ((pSnapshot[0x01e] & 0x0ff)<<8);
	cpu_set_reg(Z80_IX, RegData);

	RegData = (pSnapshot[0x01f] & 0x0ff) | ((pSnapshot[0x020] & 0x0ff)<<8);
	cpu_set_reg(Z80_IY, RegData);

	RegData = (pSnapshot[0x021] & 0x0ff) | ((pSnapshot[0x022] & 0x0ff)<<8);
	cpu_set_reg(Z80_SP, RegData);
	cpu_set_sp(RegData);

	RegData = (pSnapshot[0x023] & 0x0ff) | ((pSnapshot[0x024] & 0x0ff)<<8);

	cpu_set_reg(Z80_PC, RegData);
//	cpu_set_pc(RegData);

	RegData = (pSnapshot[0x025] & 0x0ff);
	cpu_set_reg(Z80_IM, RegData);

	RegData = (pSnapshot[0x026] & 0x0ff) | ((pSnapshot[0x027] & 0x0ff)<<8);
	cpu_set_reg(Z80_AF2, RegData);

	RegData = (pSnapshot[0x028] & 0x0ff) | ((pSnapshot[0x029] & 0x0ff)<<8);
	cpu_set_reg(Z80_BC2, RegData);

	RegData = (pSnapshot[0x02a] & 0x0ff) | ((pSnapshot[0x02b] & 0x0ff)<<8);
	cpu_set_reg(Z80_DE2, RegData);

	RegData = (pSnapshot[0x02c] & 0x0ff) | ((pSnapshot[0x02d] & 0x0ff)<<8);
	cpu_set_reg(Z80_HL2, RegData);

	/* init GA */
	for (i=0; i<17; i++)
	{
		AmstradCPC_GA_Write(i);

		AmstradCPC_GA_Write(((pSnapshot[0x02f + i] & 0x01f) | 0x040));
	}

	AmstradCPC_GA_Write(pSnapshot[0x02e] & 0x01f);

	AmstradCPC_GA_Write(((pSnapshot[0x040] & 0x03f) | 0x080));

	AmstradCPC_GA_Write(((pSnapshot[0x041] & 0x03f) | 0x0c0));

	/* init CRTC */
	for (i=0; i<18; i++)
	{
                crtc6845_address_w(0,i);
                crtc6845_register_w(0, pSnapshot[0x043+i] & 0x0ff);
	}

    crtc6845_address_w(0,i);

	/* upper rom selection */
	AmstradCPC_SetUpperRom(pSnapshot[0x055]);

	/* PPI */
	ppi8255_w(0,3,pSnapshot[0x059] & 0x0ff);

	ppi8255_w(0,0,pSnapshot[0x056] & 0x0ff);
	ppi8255_w(0,1,pSnapshot[0x057] & 0x0ff);
	ppi8255_w(0,2,pSnapshot[0x058] & 0x0ff);

	/* PSG */
	for (i=0; i<16; i++)
	{
		AY8910_control_port_0_w(0,i);

		AY8910_write_port_0_w(0,pSnapshot[0x05b + i] & 0x0ff);
	}

	AY8910_control_port_0_w(0,pSnapshot[0x05a]);

	{
		int MemSize;
		int MemorySize;

		MemSize = (pSnapshot[0x06b] & 0x0ff) | ((pSnapshot[0x06c] & 0x0ff)<<8);

		if (MemSize==128)
		{
			MemorySize = 128*1024;
		}
		else
		{
			MemorySize = 64*1024;
		}

		memcpy(Amstrad_Memory, &pSnapshot[0x0100], MemorySize);

	}

	Amstrad_RethinkMemory();

}

/* load image */
int amstrad_load(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/* load snapshot */
int amstrad_snapshot_load(int id)
{
	snapshot_loaded = 0;

	if (amstrad_load(IO_SNAPSHOT,id,&snapshot))
	{
		snapshot_loaded = 1;
		return INIT_PASS;
	}

	return INIT_FAIL;
}

#ifdef IMAGE_VERIFY
/* check if a snapshot file is valid to load */
int amstrad_snapshot_id(int id)
{
	int valid;
	unsigned char *snapshot_data;

	valid = ID_FAILED;

	/* load snapshot */
	if (amstrad_load(IO_SNAPSHOT, id, &snapshot_data))
	{
		/* snapshot loaded, check it is valid */

		if (memcmp(snapshot_data, "MV - SNA", 8)==0)
		{
			valid = ID_OK;
		}

		/* free the file */
		free(snapshot_data);
	}

	return valid;

}
#endif

void amstrad_snapshot_exit(int id)
{
	if (snapshot!=NULL)
		free(snapshot);

	snapshot_loaded = 0;

}
