#include <stdio.h>
#include <stdarg.h>

#include "driver.h"

#include "includes/cbm.h"

/* safer replacement str[0]=0; */
int DECL_SPEC cbm_snprintf (char *str, size_t size, const char *format,...)
{
	va_list list;

	va_start (list, format);

	return vsprintf (str, format, list);
}

void *cbm_memset16 (void *dest, int value, size_t size)
{
	register int i;

	for (i = 0; i < size; i++)
		((short *) dest)[i] = value;
	return dest;
}

static struct
{
	int specified;
	unsigned short addr;
	UINT8 *data;
	int length;
}
quick;

int cbm_quick_init (int id)
{
	FILE *fp;
	int read;
	char *cp;

	memset (&quick, 0, sizeof (quick));

	if (device_filename(IO_QUICKLOAD, id) == NULL)
		return INIT_OK;

	quick.specified = 1;

	fp = image_fopen (IO_QUICKLOAD, id, OSD_FILETYPE_IMAGE_R, 0);
	if (!fp)
		return INIT_FAILED;

	quick.length = osd_fsize (fp);

	if ((cp = strrchr (device_filename(IO_QUICKLOAD, id), '.')) != NULL)
	{
		if (stricmp (cp, ".prg") == 0)
		{
			osd_fread_lsbfirst (fp, &quick.addr, 2);
			quick.length -= 2;
		}
		else if (stricmp (cp, ".p00") == 0)
		{
			char buffer[7];

			osd_fread (fp, buffer, sizeof (buffer));
			if (strncmp (buffer, "C64File", sizeof (buffer)) == 0)
			{
				osd_fseek (fp, 26, SEEK_SET);
				osd_fread_lsbfirst (fp, &quick.addr, 2);
				quick.length -= 28;
			}
		}
	}
	if (quick.addr == 0)
	{
		osd_fclose (fp);
		return INIT_FAILED;
	}
	if ((quick.data = malloc (quick.length)) == NULL)
	{
		osd_fclose (fp);
		return INIT_FAILED;
	}
	read = osd_fread (fp, quick.data, quick.length);
	osd_fclose (fp);
	return read != quick.length;
}

void cbm_quick_exit (int id)
{
	if (quick.data != NULL)
		free (quick.data);
}

int cbm_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x31] = memory[0x2f] = memory[0x2d] = addr & 0xff;
	memory[0x32] = memory[0x30] = memory[0x2e] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_pet_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x2e] = memory[0x2c] = memory[0x2a] = addr & 0xff;
	memory[0x2f] = memory[0x2d] = memory[0x2b] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_pet1_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x80] = memory[0x7e] = memory[0x7c] = addr & 0xff;
	memory[0x81] = memory[0x7f] = memory[0x7d] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbmb_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr+0x10000, quick.data, quick.length);
	memory[0xf0046] = addr & 0xff;
	memory[0xf0047] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm500_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0xf0046] = addr & 0xff;
	memory[0xf0047] = addr >> 8;
	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}

int cbm_c65_quick_open (int id, int mode, void *arg)
{
	int addr;
	UINT8 *memory = arg;

	if (quick.data == NULL)
		return 1;
	addr = quick.addr + quick.length;

	memcpy (memory + quick.addr, quick.data, quick.length);
	memory[0x82] = addr & 0xff;
	memory[0x83] = addr >> 8;

	logerror("quick loading %s at %.4x size:%.4x\n",
				 device_filename(IO_QUICKLOAD,id), quick.addr, quick.length);

	return 0;
}


CBM_ROM cbm_rom[0x20]= { {0} };

void cbm_rom_exit(int id)
{
	int i;
	if (id!=0) return;
	for (i=0;(i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
			 &&(cbm_rom[i].size!=0);i++) {
		free(cbm_rom[i].chip);
		cbm_rom[i].chip=0;cbm_rom[i].size=0;
	}
}

static const struct IODevice *cbm_rom_find_device(void)
{
	int i;
	for (i=0; (Machine->gamedrv->dev[i].count)
			 &&(Machine->gamedrv->dev[i].type!=IO_CARTSLOT);
		 i++) ;
	return Machine->gamedrv->dev[i].count!=0?Machine->gamedrv->dev+i:NULL;
}

int cbm_rom_init(int id)
{
	FILE *fp;
	int i;
	int size, j, read;
	char *cp;
	unsigned int adr = 0;
	const struct IODevice *dev;

	if (device_filename(IO_CARTSLOT,id) == NULL)
		return INIT_OK;

	for (i=0;(i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))&&(cbm_rom[i].size!=0);i++)
		;
	if (i>=sizeof(cbm_rom)/sizeof(cbm_rom[0])) return INIT_FAILED;

	dev=cbm_rom_find_device();
	if ( (dev->id!=NULL) && !dev->id(id) ) return INIT_FAILED;

	fp = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
	if (!fp)
	{
		logerror("%s file not found\n", device_filename(IO_CARTSLOT,id));
		return INIT_FAILED;
	}

	size = osd_fsize (fp);

	if ((cp = strrchr (device_filename(IO_CARTSLOT,id), '.')) != NULL)
	{
		if (stricmp (cp, ".prg") == 0)
		{
			unsigned short in;

			osd_fread_lsbfirst (fp, &in, 2);
			logerror("rom prg %.4x\n", in);
			size -= 2;
			logerror("loading rom %s at %.4x size:%.4x\n",
						 device_filename(IO_CARTSLOT,id), in, size);
			if (!(cbm_rom[i].chip=malloc(size)) ) {
				osd_fclose(fp);
				return INIT_FAILED;
			}
			cbm_rom[i].addr=in;
			cbm_rom[i].size=size;
			read = osd_fread (fp, cbm_rom[i].chip, size);
			osd_fclose (fp);
			if (read != size)
				return INIT_FAILED;
		}
		else if (stricmp (cp, ".crt") == 0)
		{
			unsigned short in;

			osd_fseek (fp, 64, SEEK_SET);
			j = 64;
			logerror("loading rom %s size:%.4x\n",
						 device_filename(IO_CARTSLOT,id), size);
			while (j < size)
			{
				unsigned short segsize;
				unsigned char buffer[10], number;

				osd_fread (fp, buffer, 6);
				osd_fread_msbfirst (fp, &segsize, 2);
				osd_fread (fp, buffer + 6, 3);
				osd_fread (fp, &number, 1);
				osd_fread_msbfirst (fp, &adr, 2);
				osd_fread_msbfirst (fp, &in, 2);
				logerror("%.4s %.2x %.2x %.4x %.2x %.2x %.2x %.2x %.4x:%.4x\n",
							 buffer, buffer[4], buffer[5], segsize,
							 buffer[6], buffer[7], buffer[8], number,
							 adr, in);
				logerror("loading chip at %.4x size:%.4x\n", adr, in);


				if (!(cbm_rom[i].chip=malloc(size)) ) {
					osd_fclose(fp);
					return INIT_FAILED;
				}
				cbm_rom[i].addr=adr;
				cbm_rom[i].size=in;
				read = osd_fread (fp, cbm_rom[i].chip, in);
				i++;
				if (read != in)
				{
					osd_fclose (fp);
					return INIT_FAILED;
				}
				j += 16 + in;
			}
			osd_fclose (fp);
		}
		else
		{
			if (stricmp (cp, ".lo") == 0)
				adr = CBM_ROM_ADDR_LO;
			else if (stricmp (cp, ".hi") == 0)
				adr = CBM_ROM_ADDR_HI;
			else if (stricmp (cp, ".10") == 0)
				adr = 0x1000;
			else if (stricmp (cp, ".20") == 0)
				adr = 0x2000;
			else if (stricmp (cp, ".30") == 0)
				adr = 0x3000;
			else if (stricmp (cp, ".40") == 0)
				adr = 0x4000;
			else if (stricmp (cp, ".50") == 0)
				adr = 0x5000;
			else if (stricmp (cp, ".60") == 0)
				adr = 0x6000;
			else if (stricmp (cp, ".70") == 0)
				adr = 0x7000;
			else if (stricmp (cp, ".80") == 0)
				adr = 0x8000;
			else if (stricmp (cp, ".90") == 0)
				adr = 0x9000;
			else if (stricmp (cp, ".a0") == 0)
				adr = 0xa000;
			else if (stricmp (cp, ".b0") == 0)
				adr = 0xb000;
			else if (stricmp (cp, ".e0") == 0)
				adr = 0xe000;
			else if (stricmp (cp, ".f0") == 0)
				adr = 0xf000;
			else adr = CBM_ROM_ADDR_UNKNOWN;
			logerror("loading %s rom at %.4x size:%.4x\n",
						 device_filename(IO_CARTSLOT,id), adr, size);
			if (!(cbm_rom[i].chip=malloc(size)) ) {
				osd_fclose(fp);
				return INIT_FAILED;
			}
			cbm_rom[i].addr=adr;
			cbm_rom[i].size=size;
			read = osd_fread (fp, cbm_rom[i].chip, size);

			osd_fclose (fp);
			if (read != size)
				return INIT_FAILED;
		}
	}
	return INIT_OK;
}

