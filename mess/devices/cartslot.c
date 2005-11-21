#include "cartslot.h"
#include "mess.h"
#include "mscommon.h"

const game_driver *cartslot_gamedriver_hack;



static int is_cart_roment(const rom_entry *roment)
{
	return ROMENTRY_GETTYPE(roment) == ROMENTRYTYPE_CARTRIDGE;
}



static int parse_rom_name(const rom_entry *roment, int *position, const char **extensions)
{
	const char *data = roment->_hashdata;

	if (data[0] != '#')
		return -1;
	if (!isdigit(data[1]))
		return -1;
	if (data[2] != '\0')
		return -1;

	if (position)
		*position = data[1] - '0';
	if (extensions)
		*extensions = &data[3];
	return 0;
}



static int load_cartridge(const rom_entry *romrgn, const rom_entry *roment, mame_file *file)
{
	UINT32 region, flags;
	offs_t offset, length, read_length, pos = 0, len;
	UINT8 *ptr;
	UINT8 clear_val;
	int type, datawidth, littleendian, i, j;

	region = ROMREGION_GETTYPE(romrgn);
	offset = ROM_GETOFFSET(roment);
	length = ROM_GETLENGTH(roment);
	flags = ROM_GETFLAGS(roment);
	ptr = ((UINT8 *) memory_region(region)) + offset;

	if (file)
	{
		/* must this be full size */
		if (flags & ROM_FULLSIZE)
		{
			if (mame_fsize(file) != length)
				return INIT_FAIL;
		}

		/* read the ROM */
		pos = read_length = mame_fread(file, ptr, length);

		/* do we need to mirror the ROM? */
		if (flags & ROM_MIRROR)
		{
			while(pos < length)
			{
				len = MIN(read_length, length - pos);
				memcpy(ptr + pos, ptr, len);
				pos += len;
			}
		}

		/* postprocess this region */
		type = ROMREGION_GETTYPE(romrgn);
		littleendian = ROMREGION_ISLITTLEENDIAN(romrgn);
		datawidth = ROMREGION_GETWIDTH(romrgn) / 8;

		/* if the region is inverted, do that now */
		if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
		{
			int cputype = Machine->drv->cpu[type - REGION_CPU1].cpu_type;
			if (cputype != 0)
			{
				datawidth = cputype_databus_width(cputype, ADDRESS_SPACE_PROGRAM) / 8;
				littleendian = (cputype_endianness(cputype) == CPU_IS_LE);
			}
		}

		/* swap the endianness if we need to */
#ifdef LSB_FIRST
		if (datawidth > 1 && !littleendian)
#else
		if (datawidth > 1 && littleendian)
#endif
		{
			for (i = 0; i < length; i += datawidth)
			{
				UINT8 temp[8];
				memcpy(temp, &ptr[i], datawidth);
				for (j = datawidth - 1; j >= 0; j--)
					ptr[i + j] = temp[datawidth - 1 - j];
			}
		}
	}

	/* clear out anything that remains */
	if (!(flags & ROM_NOCLEAR))
	{
		clear_val = (flags & ROM_FILL_FF) ? 0xFF : 0x00;
		memset(ptr + pos, clear_val, length - pos);
	}
	return INIT_PASS;
}



static int process_cartridge(mess_image *image, mame_file *file)
{
	const rom_entry *romrgn, *roment;
	int position, result;

	romrgn = rom_first_region(Machine->gamedrv);
	while(romrgn)
	{
		roment = romrgn + 1;
		while(!ROMENTRY_ISREGIONEND(roment))
		{
			if (is_cart_roment(roment))
			{
				parse_rom_name(roment, &position, NULL);
				if (position == image_index_in_device(image))
				{
					result = load_cartridge(romrgn, roment, file);
					if (!result)
						return result;
				}
			}
			roment++;
		}
		romrgn = rom_next_region(romrgn);
	}
	return INIT_PASS;
}



static DEVICE_INIT(cartslot_specified)
{
	process_cartridge(image, NULL);
	return 0;
}

static DEVICE_LOAD(cartslot_specified)
{
	return process_cartridge(image, file);
}

static DEVICE_UNLOAD(cartslot_specified)
{
	process_cartridge(image, NULL);
}



void cartslot_device_getinfo(struct IODevice *iodev)
{
	const rom_entry *romrgn, *roment;
	int position;
	UINT32 flags;

	/* specify basics */
	iodev->type = IO_CARTSLOT;
	iodev->file_extensions = "bin\0";
	iodev->reset_on_load = 1;
	iodev->load_at_init = 1;
	iodev->readable = 1;

	/* try to find ROM_CART_LOADs in the ROM declaration */
	if (cartslot_gamedriver_hack)
	{
		romrgn = rom_first_region(cartslot_gamedriver_hack);
		while(romrgn)
		{
			roment = romrgn + 1;
			while(!ROMENTRY_ISREGIONEND(roment))
			{
				if (is_cart_roment(roment) && !parse_rom_name(roment, &position, &iodev->file_extensions))
				{
					flags = ROM_GETFLAGS(roment);

					/* reject any unsupported flags */
					if (flags & (ROM_GROUPMASK | ROM_SKIPMASK | ROM_REVERSEMASK
						| ROM_BITWIDTHMASK | ROM_BITSHIFTMASK
						| ROM_INHERITFLAGSMASK | ROM_BIOSFLAGSMASK))
					{
						osd_die("Unsupported ROM cart flags 0x%08X", flags);
					}

					iodev->count = MAX(position + 1, iodev->count);
					iodev->init = device_init_cartslot_specified;
					iodev->load = device_load_cartslot_specified;
					iodev->unload = device_unload_cartslot_specified;
					iodev->must_be_loaded = (flags & ROM_OPTIONAL) ? 0 : 1;
				}
				roment++;
			}
			romrgn = rom_next_region(romrgn);
		}
	}
}
