#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include "image.h"
#include "includes/cbm.h"

static struct
{
	int specified;
	unsigned short addr;
	UINT8 *data;
	int length;
}
quick;

int cbm_quick_init (int id, mame_file *fp, int open_mode)
{
	int read_;
	const char *cp;

	memset (&quick, 0, sizeof (quick));

	if (fp == NULL)
		return INIT_PASS;

	quick.specified = 1;

	quick.length = mame_fsize (fp);

	cp = image_filetype(IO_QUICKLOAD, id);
	if (cp)
	{
		if (stricmp (cp, "prg") == 0)
		{
			mame_fread_lsbfirst (fp, &quick.addr, 2);
			quick.length -= 2;
		}
		else if (stricmp (cp, "p00") == 0)
		{
			char buffer[7];

			mame_fread (fp, buffer, sizeof (buffer));
			if (strncmp (buffer, "C64File", sizeof (buffer)) == 0)
			{
				mame_fseek (fp, 26, SEEK_SET);
				mame_fread_lsbfirst (fp, &quick.addr, 2);
				quick.length -= 28;
			}
		}
	}
	if (quick.addr == 0)
		return INIT_FAIL;

	if ((quick.data = (UINT8*) image_malloc (IO_QUICKLOAD, id, quick.length)) == NULL)
		return INIT_FAIL;

	read_ = mame_fread (fp, quick.data, quick.length);
	return read_ != quick.length;
}

int cbm_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x31] = memory[0x2f] = memory[0x2d] = addr & 0xff;
	memory[0x32] = memory[0x30] = memory[0x2e] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_pet_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x2e] = memory[0x2c] = memory[0x2a] = addr & 0xff;
	memory[0x2f] = memory[0x2d] = memory[0x2b] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_pet1_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x80] = memory[0x7e] = memory[0x7c] = addr & 0xff;
	memory[0x81] = memory[0x7f] = memory[0x7d] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbmb_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr+0x10000, quick.data, quick.length);
	memory[0xf0046] = addr & 0xff;
	memory[0xf0047] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm500_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0xf0046] = addr & 0xff;
	memory[0xf0047] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_c65_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = (UINT8*)arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x82] = addr & 0xff;
	memory[0x83] = addr >> 8;

	logerror("quick loading %s at %.4x size:%.4x\n",
				 image_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

INT8 cbm_c64_game;
INT8 cbm_c64_exrom;
CBM_ROM cbm_rom[0x20]= { {0} };

void cbm_rom_exit(int id)
{
    int i;
    if (id!=0) return;
    for (i=0;(i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
	     &&(cbm_rom[i].size!=0);i++)
	{
		cbm_rom[i].chip=0;
		cbm_rom[i].size=0;
    }
}

static const struct IODevice *cbm_rom_find_device(void)
{
	return device_find(Machine->gamedrv, IO_CARTSLOT);
}

int cbm_rom_init(int id, mame_file *fp, int open_mode)
{
	int i;
	int size, j, read_;
	const char *filetype;
	int adr = 0;
	const struct IODevice *dev;

	if (id==0) {
	    cbm_c64_game=-1;
	    cbm_c64_exrom=-1;
	}

	if (fp == NULL)
		return INIT_PASS;

	for (i=0; (i<sizeof(cbm_rom) / sizeof(cbm_rom[0])) && (cbm_rom[i].size!=0); i++)
		;
	if (i >= sizeof(cbm_rom) / sizeof(cbm_rom[0]))
		return INIT_FAIL;

	dev = cbm_rom_find_device();

	size = mame_fsize (fp);

	filetype = image_filetype(IO_CARTSLOT, id);
	if (filetype && !stricmp(filetype, "prg"))
	{
		unsigned short in;

		mame_fread_lsbfirst (fp, &in, 2);
		logerror("rom prg %.4x\n", in);
		size -= 2;
		logerror("loading rom %s at %.4x size:%.4x\n",
			 image_filename(IO_CARTSLOT,id), in, size);
		cbm_rom[i].chip = (UINT8*) image_malloc(IO_CARTSLOT, id, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;
		cbm_rom[i].addr=in;
		cbm_rom[i].size=size;
		read_ = mame_fread (fp, cbm_rom[i].chip, size);
		if (read_ != size)
			return INIT_FAIL;
	}
	else if (filetype && !stricmp (filetype, "crt"))
	{
		unsigned short in;
		mame_fseek (fp, 0x18, SEEK_SET);
		mame_fread( fp, &cbm_c64_exrom, 1);
		mame_fread( fp, &cbm_c64_game, 1);
		mame_fseek (fp, 64, SEEK_SET);
		j = 64;
		logerror("loading rom %s size:%.4x\n",
			image_filename(IO_CARTSLOT,id), size);
		while (j < size)
		{
			unsigned short segsize;
			unsigned char buffer[10], number;

			mame_fread (fp, buffer, 6);
			mame_fread_msbfirst (fp, &segsize, 2);
			mame_fread (fp, buffer + 6, 3);
			mame_fread (fp, &number, 1);
			mame_fread_msbfirst (fp, &adr, 2);
			mame_fread_msbfirst (fp, &in, 2);
			logerror("%.4s %.2x %.2x %.4x %.2x %.2x %.2x %.2x %.4x:%.4x\n",
				buffer, buffer[4], buffer[5], segsize,
				buffer[6], buffer[7], buffer[8], number,
				adr, in);
			logerror("loading chip at %.4x size:%.4x\n", adr, in);

			cbm_rom[i].chip = (UINT8*) image_malloc(IO_CARTSLOT, id, size);
			if (!cbm_rom[i].chip)
				return INIT_FAIL;

			cbm_rom[i].addr=adr;
			cbm_rom[i].size=in;
			read_ = mame_fread (fp, cbm_rom[i].chip, in);
			i++;
			if (read_ != in)
				return INIT_FAIL;

			j += 16 + in;
		}
	}
	else if (filetype)
	{
		if (stricmp(filetype, "lo") == 0)
			adr = CBM_ROM_ADDR_LO;
		else if (stricmp (filetype, "hi") == 0)
			adr = CBM_ROM_ADDR_HI;
		else if (stricmp (filetype, "10") == 0)
			adr = 0x1000;
		else if (stricmp (filetype, "20") == 0)
			adr = 0x2000;
		else if (stricmp (filetype, "30") == 0)
			adr = 0x3000;
		else if (stricmp (filetype, "40") == 0)
			adr = 0x4000;
		else if (stricmp (filetype, "50") == 0)
			adr = 0x5000;
		else if (stricmp (filetype, "60") == 0)
			adr = 0x6000;
		else if (stricmp (filetype, "70") == 0)
			adr = 0x7000;
		else if (stricmp (filetype, "80") == 0)
			adr = 0x8000;
		else if (stricmp (filetype, "90") == 0)
			adr = 0x9000;
		else if (stricmp (filetype, "a0") == 0)
			adr = 0xa000;
		else if (stricmp (filetype, "b0") == 0)
			adr = 0xb000;
		else if (stricmp (filetype, "e0") == 0)
			adr = 0xe000;
		else if (stricmp (filetype, "f0") == 0)
			adr = 0xf000;
		else
			adr = CBM_ROM_ADDR_UNKNOWN;

		logerror("loading %s rom at %.4x size:%.4x\n",
			 image_filename(IO_CARTSLOT,id), adr, size);

		cbm_rom[i].chip = (UINT8*) image_malloc(IO_CARTSLOT, id, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;

		cbm_rom[i].addr=adr;
		cbm_rom[i].size=size;
		read_ = mame_fread (fp, cbm_rom[i].chip, size);

		if (read_ != size)
			return INIT_FAIL;
	}
	return INIT_PASS;
}

