#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include "image.h"
#include "includes/cbm.h"

UINT8 *cbmb_memory;

static int general_cbm_loadsnap(mame_file *fp, const char *file_type, int snapshot_size, UINT8 *memory,
	void (*cbmloadsnap)(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length))
{
	char buffer[7];
	UINT8 *data = NULL;
	UINT32 bytesread;
	UINT16 address = 0;

	assert(memory);

	if (!file_type)
		goto error;

	if (!stricmp(file_type, "prg"))
	{
		/* prg files */
	}
	else if (!stricmp(file_type, "p00"))
	{
		/* p00 files */
		if (mame_fread(fp, buffer, sizeof(buffer)) != sizeof(buffer))
			goto error;
		if (memcmp(buffer, "C64File", sizeof(buffer)))
			goto error;
		mame_fseek(fp, 26, SEEK_SET);
		snapshot_size -= 26;
	}
	else
	{
		goto error;
	}

	mame_fread_lsbfirst(fp, &address, 2);
	snapshot_size -= 2;

	data = malloc(snapshot_size);
	if (!data)
		goto error;

	bytesread = mame_fread(fp, data, snapshot_size);
	if (bytesread != snapshot_size)
		goto error;

	cbmloadsnap(memory, data, address, address + snapshot_size, snapshot_size);
	free(data);
	return INIT_PASS;

error:
	if (data)
		free(data);
	return INIT_FAIL;
}

static void cbm_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address, data, length);
	memory[0x31] = memory[0x2f] = memory[0x2d] = hiaddress & 0xff;
	memory[0x32] = memory[0x30] = memory[0x2e] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbm_c16 )
{
	extern UINT8 *c16_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, c16_memory, cbm_quick_open);
}

QUICKLOAD_LOAD( cbm_c64 )
{
	extern UINT8 *c64_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, c64_memory, cbm_quick_open);
}

QUICKLOAD_LOAD( cbm_vc20 )
{
	extern UINT8 *vc20_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, vc20_memory, cbm_quick_open);
}

static void cbm_pet_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address, data, length);
	memory[0x2e] = memory[0x2c] = memory[0x2a] = hiaddress & 0xff;
	memory[0x2f] = memory[0x2d] = memory[0x2b] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbm_pet )
{
	extern UINT8 *pet_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, pet_memory, cbm_pet_quick_open);
}

static void cbm_pet1_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address, data, length);
	memory[0x80] = memory[0x7e] = memory[0x7c] = hiaddress & 0xff;
	memory[0x81] = memory[0x7f] = memory[0x7d] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbm_pet1 )
{
	extern UINT8 *pet_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, pet_memory, cbm_pet1_quick_open);
}

static void cbmb_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address + 0x10000, data, length);
	memory[0xf0046] = hiaddress & 0xff;
	memory[0xf0047] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbmb )
{
	extern UINT8 *cbmb_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, cbmb_memory, cbmb_quick_open);
}

static void cbm500_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address, data, length);
	memory[0xf0046] = hiaddress & 0xff;
	memory[0xf0047] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbm500 )
{
	extern UINT8 *cbmb_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, cbmb_memory, cbm500_quick_open);
}

static void cbm_c65_quick_open(UINT8 *memory, UINT8 *data, UINT16 address, UINT16 hiaddress, int length)
{
	memcpy(memory + address, data, length);
	memory[0x82] = hiaddress & 0xff;
	memory[0x83] = hiaddress >> 8;
}

QUICKLOAD_LOAD( cbm_c65 )
{
	extern UINT8 *c64_memory;
	return general_cbm_loadsnap(fp, file_type, quickload_size, c64_memory, cbm_c65_quick_open);
}

/* ----------------------------------------------------------------------- */

INT8 cbm_c64_game;
INT8 cbm_c64_exrom;
CBM_ROM cbm_rom[0x20]= { {0} };

DEVICE_UNLOAD(cbm_rom)
{
	int id = image_index_in_device(image);
	cbm_rom[id].size = 0;
	cbm_rom[id].chip = 0;
}

static const struct IODevice *cbm_rom_find_device(void)
{
	return device_find(Machine->gamedrv, IO_CARTSLOT);
}

DEVICE_INIT(cbm_rom)
{
	int id = image_index_in_device(image);
	if (id == 0)
	{
		cbm_c64_game = -1;
		cbm_c64_exrom = -1;
	}
	return INIT_PASS;
}

DEVICE_LOAD(cbm_rom)
{
	int i;
	int size, j, read_;
	const char *filetype;
	int adr = 0;
	const struct IODevice *dev;

	for (i=0; (i<sizeof(cbm_rom) / sizeof(cbm_rom[0])) && (cbm_rom[i].size!=0); i++)
		;
	if (i >= sizeof(cbm_rom) / sizeof(cbm_rom[0]))
		return INIT_FAIL;

	if (!file)
	{
		return INIT_PASS;
	}

	dev = cbm_rom_find_device();

	size = mame_fsize (file);

	filetype = image_filetype(image);
	if (filetype && !stricmp(filetype, "prg"))
	{
		unsigned short in;

		mame_fread_lsbfirst (file, &in, 2);
		logerror("rom prg %.4x\n", in);
		size -= 2;

		logerror("loading rom %s at %.4x size:%.4x\n", image_filename(image), in, size);

		cbm_rom[i].chip = (UINT8 *) image_malloc(image, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;

		cbm_rom[i].addr=in;
		cbm_rom[i].size=size;
		read_ = mame_fread (file, cbm_rom[i].chip, size);
		if (read_ != size)
			return INIT_FAIL;
	}
	else if (filetype && !stricmp (filetype, "crt"))
	{
		unsigned short in;
		mame_fseek (file, 0x18, SEEK_SET);
		mame_fread( file, &cbm_c64_exrom, 1);
		mame_fread( file, &cbm_c64_game, 1);
		mame_fseek (file, 64, SEEK_SET);
		j = 64;

		logerror("loading rom %s size:%.4x\n", image_filename(image), size);

		while (j < size)
		{
			unsigned short segsize;
			unsigned char buffer[10], number;

			mame_fread (file, buffer, 6);
			mame_fread_msbfirst (file, &segsize, 2);
			mame_fread (file, buffer + 6, 3);
			mame_fread (file, &number, 1);
			mame_fread_msbfirst (file, &adr, 2);
			mame_fread_msbfirst (file, &in, 2);
			logerror("%.4s %.2x %.2x %.4x %.2x %.2x %.2x %.2x %.4x:%.4x\n",
				buffer, buffer[4], buffer[5], segsize,
				buffer[6], buffer[7], buffer[8], number,
				adr, in);
			logerror("loading chip at %.4x size:%.4x\n", adr, in);

			cbm_rom[i].chip = (UINT8*) image_malloc(image, size);
			if (!cbm_rom[i].chip)
				return INIT_FAIL;

			cbm_rom[i].addr=adr;
			cbm_rom[i].size=in;
			read_ = mame_fread (file, cbm_rom[i].chip, in);
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
				image_filename(image), adr, size);

		cbm_rom[i].chip = (UINT8*) image_malloc(image, size);
		if (!cbm_rom[i].chip)
			return INIT_FAIL;

		cbm_rom[i].addr=adr;
		cbm_rom[i].size=size;
		read_ = mame_fread (file, cbm_rom[i].chip, size);

		if (read_ != size)
			return INIT_FAIL;
	}
	return INIT_PASS;
}

