#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/ppu2c03b.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"
#include "zlib.h"
#include "image.h"


/* Uncomment this to dump reams of ppu state info to the errorlog */
//#define LOG_PPU

/* Uncomment this to dump info about the inputs to the errorlog */
//#define LOG_JOY

/* Uncomment this to generate prg chunk files when the cart is loaded */
//#define SPLIT_PRG

#define BATTERY_SIZE 0x2000
UINT8 battery_data[BATTERY_SIZE];

struct nes_struct nes;
struct fds_struct nes_fds;

int ppu_scanlines_per_frame;

static UINT32 in_0[3];
static UINT32 in_1[3];
static UINT32 in_0_shift;
static UINT32 in_1_shift;

/* local prototypes */
static void init_nes_core (void);

static mess_image *cartslot_image(void)
{
	return image_from_devtype_and_index(IO_CARTSLOT, 0);
}

static void init_nes_core (void)
{
	/* We set these here in case they weren't set in the cart loader */
	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	battery_ram = nes.wram;

	/* Set up the memory handlers for the mapper */
	switch (nes.mapper)
	{
		case 20:
			nes.slow_banking = 0;
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4030, 0x403f, 0, fds_r);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0xdfff, 0, MRA8_RAM);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, MRA8_ROM);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4020, 0x402f, 0, fds_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0xdfff, 0, MWA8_RAM);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, MWA8_ROM);
			break;
		case 40:
			nes.slow_banking = 1;
			/* Game runs code in between banks, so we do things different */
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, MRA8_RAM);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, MRA8_ROM);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, nes_mid_mapper_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, nes_mapper_w);
			break;
		default:
			nes.slow_banking = 0;
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, MRA8_BANK5);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, MRA8_BANK1);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, MRA8_BANK2);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, MRA8_BANK3);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, MRA8_BANK4);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, nes_mid_mapper_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, nes_mapper_w);
			break;
	}

	/* Set up the mapper callbacks */
	{
		int i = 0;

		while (mmc_list[i].iNesMapper != -1)
		{
			if (mmc_list[i].iNesMapper == nes.mapper)
			{
				mmc_write_low = mmc_list[i].mmc_write_low;
				mmc_read_low = mmc_list[i].mmc_read_low;
				mmc_write_mid = mmc_list[i].mmc_write_mid;
				mmc_write = mmc_list[i].mmc_write;
				ppu_latch = mmc_list[i].ppu_latch;
//				mmc_irq = mmc_list[i].mmc_irq;
				break;
			}
			i ++;
		}
		if (mmc_list[i].iNesMapper == -1)
		{
			logerror ("Mapper %d is not yet supported, defaulting to no mapper.\n",nes.mapper);
			mmc_write_low = mmc_write_mid = mmc_write = NULL;
			mmc_read_low = NULL;
		}
	}

	/* Load a battery file, but only if there's no trainer since they share */
	/* overlapping memory. */
	if (nes.trainer) return;

	/* We need this because battery ram is loaded before the */
	/* memory subsystem is set up. When this routine is called */
	/* everything is ready, so we can just copy over the data */
	/* we loaded before. */
	memcpy (battery_ram, battery_data, BATTERY_SIZE);
}

void init_nes (void)
{
	ppu_scanlines_per_frame = NTSC_SCANLINES_PER_FRAME;
	init_nes_core ();
}

void init_nespal (void)
{
	ppu_scanlines_per_frame = PAL_SCANLINES_PER_FRAME;
	init_nes_core ();
}

static int ppu_vidaccess( int num, int address, int data )
{
	/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
	/* but, B-Wings, submath (j) seem to use this location differently... */
	if (nes.chr_chunks && (address & 0x3fff) < 0x2000)
	{
//		int vrom_loc;
//		vrom_loc = (nes_vram[(address & 0x1fff) >> 10] * 16) + (address & 0x3ff);
//		data = nes.vrom [vrom_loc];
	}
	return data;
}

MACHINE_INIT( nes )
{
	ppu2c03b_reset( 0, 1 );
	ppu2c03b_set_vidaccess_callback(0, ppu_vidaccess);
	ppu2c03b_set_scanlines_per_frame(0, ppu_scanlines_per_frame);

	if (nes.four_screen_vram)
	{
		/* TODO: figure out what to do here */
	}
	else
	{
		switch(nes.hard_mirroring) {
		case 0:
			ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			break;
		case 1:
			ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		}
	}

	/* Some carts have extra RAM and require it on at startup, e.g. Metroid */
	nes.mid_ram_enable = 1;

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset (nes.mapper);

	/* Reset the serial input ports */
	in_0_shift = 0;
	in_1_shift = 0;
}

MACHINE_STOP( nes )
{
	/* Write out the battery file if necessary */
	if (nes.battery)
		image_battery_save(cartslot_image(), battery_ram, BATTERY_SIZE);
}

READ_HANDLER ( nes_IN0_r )
{
	int dip;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	retVal = 0x40;

	retVal |= ((in_0[0] >> in_0_shift) & 0x01);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0x0f)
	{
		case 0x01: /* zapper */
			{
				UINT16 pix;
				retVal |= 0x08;  /* no sprite hit */

				/* If button 1 is pressed, indicate the light gun trigger is pressed */
				retVal |= ((in_0[0] & 0x01) << 4);

				/* Look at the screen and see if the cursor is over a bright pixel */
				pix = read_pixel(Machine->scrbitmap, in_0[1], in_0[2]);
				if ((pix == Machine->pens[0x20]) || (pix == Machine->pens[0x30]) ||
					(pix == Machine->pens[0x33]) || (pix == Machine->pens[0x34]))
				{
					retVal &= ~0x08; /* sprite hit */
				}
			}
			break;
		case 0x02: /* multitap */
			/* Handle data line 1's serial output */
//			retVal |= ((in_0[1] >> in_0_shift) & 0x01) << 1;
			break;
	}

#ifdef LOG_JOY
	logerror ("joy 0 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_0_shift, in_0[0]);
#endif

	in_0_shift ++;
	return retVal;
}

READ_HANDLER ( nes_IN1_r )
{
	int dip;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4017 leaves 0x40 there. */
	retVal = 0x40;

	/* Handle data line 0's serial output */
	retVal |= ((in_1[0] >> in_1_shift) & 0x01);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0xf0)
	{
		case 0x10: /* zapper */
			{
				UINT16 pix;
				retVal |= 0x08;  /* no sprite hit */

				/* If button 1 is pressed, indicate the light gun trigger is pressed */
				retVal |= ((in_1[0] & 0x01) << 4);

				/* Look at the screen and see if the cursor is over a bright pixel */
				pix = read_pixel(Machine->scrbitmap, in_1[1], in_1[2]);
				if ((pix == Machine->pens[0x20]) || (pix == Machine->pens[0x30]) ||
					(pix == Machine->pens[0x33]) || (pix == Machine->pens[0x34]))
				{
					retVal &= ~0x08; /* sprite hit */
				}
			}
			break;
		case 0x20: /* multitap */
			/* Handle data line 1's serial output */
//			retVal |= ((in_1[1] >> in_1_shift) & 0x01) << 1;
			break;
		case 0x30: /* arkanoid dial */
			/* Handle data line 2's serial output */
			retVal |= ((in_1[2] >> in_1_shift) & 0x01) << 3;

			/* Handle data line 3's serial output - bits are reversed */
//			retVal |= ((in_1[3] >> in_1_shift) & 0x01) << 4;
			retVal |= ((in_1[3] << in_1_shift) & 0x80) >> 3;
			break;
	}

#ifdef LOG_JOY
	logerror ("joy 1 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_1_shift, in_1[0]);
#endif

	in_1_shift ++;
	return retVal;
}

WRITE_HANDLER ( nes_IN0_w )
{
	int dip;

	if (data & 0x01) return;
#ifdef LOG_JOY
	logerror ("joy 0 bits read: %d\n", in_0_shift);
#endif

	/* Toggling bit 0 high then low resets both controllers */
	in_0_shift = 0;
	in_1_shift = 0;

	in_0[0] = readinputport (0);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0x0f)
	{
		case 0x01: /* zapper */
			in_0[1] = readinputport (3); /* x-axis */
			in_0[2] = readinputport (4); /* y-axis */
			break;

		case 0x02: /* multitap */
			in_0[0] |= (readinputport (8) << 8);
			in_0[0] |= (0x08 << 16); /* OR in the 4-player adapter id, channel 0 */

			/* Optional: copy the data onto the second channel */
//			in_0[1] = in_0[0];
//			in_0[1] |= (0x04 << 16); /* OR in the 4-player adapter id, channel 1 */
			break;
	}

	in_1[0] = readinputport (1);

	switch (dip & 0xf0)
	{
		case 0x10: /* zapper */
			if (dip & 0x01)
			{
				/* zapper is also on port 1, use 2nd player analog inputs */
				in_1[1] = readinputport (5); /* x-axis */
				in_1[2] = readinputport (6); /* y-axis */
			}
			else
			{
				in_1[1] = readinputport (3); /* x-axis */
				in_1[2] = readinputport (4); /* y-axis */
			}
			break;

		case 0x20: /* multitap */
			in_1[0] |= (readinputport (9) << 8);
			in_1[0] |= (0x04 << 16); /* OR in the 4-player adapter id, channel 0 */;

			/* Optional: copy the data onto the second channel */
//			in_1[1] = in_1[0];
//			in_1[1] |= (0x08 << 16); /* OR in the 4-player adapter id, channel 1 */
			break;

		case 0x30: /* arkanoid dial */
			in_1[3] = (UINT8) ((UINT8) readinputport (10) + (UINT8)0x52) ^ 0xff;
//			in_1[3] = readinputport (10) ^ 0xff;
//			in_1[3] = 0x02;

			/* Copy the joypad data onto the third channel */
			in_1[2] = in_1[0] /*& 0x01*/;
			break;
	}
}

WRITE_HANDLER ( nes_IN1_w )
{
}

DEVICE_LOAD(nes_cart)
{
	const char *mapinfo;
	int mapint1=0,mapint2=0,mapint3=0,mapint4=0,goodcrcinfo = 0;
	char magic[4];
	char skank[8];
	int local_options = 0;
	char m;
	int i;

	/* Verify the file is in iNES format */
	mame_fread (file, magic, 4);

	if ((magic[0] != 'N') ||
		(magic[1] != 'E') ||
		(magic[2] != 'S'))
		goto bad;

	mapinfo = image_extrainfo(image);
	if (mapinfo)
	{
		if (4 == sscanf(mapinfo,"%d %d %d %d",&mapint1,&mapint2,&mapint3,&mapint4))
		{
			nes.mapper = mapint1;
			local_options = mapint2;
			nes.prg_chunks = mapint3;
			nes.chr_chunks = mapint4;
			logerror("NES.CRC info: %d %d %d %d\n",mapint1,mapint2,mapint3,mapint4);
			goodcrcinfo = 1;
		} else
		{
			logerror("NES: [%s], Invalid mapinfo found\n",mapinfo);
		}
	} else
	{
		logerror("NES: No extrainfo found\n");
	}
	if (!goodcrcinfo)
	{
		// image_extrainfo() resets the file position back to start.
		// Let's skip past the magic header once again.
		mame_fseek (file, 4, SEEK_SET);

		mame_fread (file, &nes.prg_chunks, 1);
		mame_fread (file, &nes.chr_chunks, 1);
		/* Read the first ROM option byte (offset 6) */
		mame_fread (file, &m, 1);

		/* Interpret the iNES header flags */
		nes.mapper = (m & 0xf0) >> 4;
		local_options = m & 0x0f;


		/* Read the second ROM option byte (offset 7) */
		mame_fread (file, &m, 1);

		/* Check for skanky headers */
		mame_fread (file, &skank, 8);

		/* If the header has junk in the unused bytes, assume the extra mapper byte is also invalid */
		/* We only check the first 4 unused bytes for now */
		for (i = 0; i < 4; i ++)
		{
			logerror("%02x ", skank[i]);
			if (skank[i] != 0x00)
			{
				logerror("(skank: %d)", i);
//				m = 0;
			}
		}
		logerror("\n");

		nes.mapper = nes.mapper | (m & 0xf0);
	}

	nes.hard_mirroring = local_options & 0x01;
	nes.battery = local_options & 0x02;
	nes.trainer = local_options & 0x04;
	nes.four_screen_vram = local_options & 0x08;

	if (nes.battery) logerror("-- Battery found\n");
	if (nes.trainer) logerror("-- Trainer found\n");
	if (nes.four_screen_vram) logerror("-- 4-screen VRAM\n");

	/* Free the regions that were allocated by the ROM loader */
	free_memory_region (REGION_CPU1);
	free_memory_region (REGION_GFX1);

	/* Allocate them again with the proper size */
	if (new_memory_region(REGION_CPU1, 0x10000 + (nes.prg_chunks+1) * 0x4000,0))
		goto outofmemory;
	if (nes.chr_chunks && new_memory_region(REGION_GFX1, nes.chr_chunks * 0x2000,0))
		goto outofmemory;

	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	/* Position past the header */
	mame_fseek (file, 16, SEEK_SET);

	/* Load the 0x200 byte trainer at 0x7000 if it exists */
	if (nes.trainer)
	{
		mame_fread (file, &nes.wram[0x1000], 0x200);
	}

	/* Read in the program chunks */
	if (nes.prg_chunks == 1)
	{
		mame_fread (file, &nes.rom[0x14000], 0x4000);
		/* Mirror this bank into $8000 */
		memcpy (&nes.rom[0x10000], &nes.rom [0x14000], 0x4000);
	}
	else
		mame_fread (file, &nes.rom[0x10000], 0x4000 * nes.prg_chunks);

#ifdef SPLIT_PRG
{
	FILE *prgout;
	char outname[255];

	for (i = 0; i < nes.prg_chunks; i ++)
	{
		sprintf (outname, "%s.p%d", battery_name, i);
		prgout = fopen (outname, "wb");
		if (prgout)
		{
			fwrite (&nes.rom[0x10000 + 0x4000 * i], 1, 0x4000, prgout);
			fclose (prgout);
		}
	}
}
#endif

	logerror("**\n");
	logerror("Mapper: %d\n", nes.mapper);
	logerror("PRG chunks: %02x, size: %06x\n", nes.prg_chunks, 0x4000*nes.prg_chunks);

	/* Read in any chr chunks */
	if (nes.chr_chunks > 0)
	{
		mame_fread (file, nes.vrom, nes.chr_chunks * 0x2000);
		if (nes.mapper == 2)
			logerror("Warning: VROM has been found in VRAM-based mapper. Either the mapper is set wrong or the ROM image is incorrect.\n");
	}

	logerror("CHR chunks: %02x, size: %06x\n", nes.chr_chunks, 0x4000*nes.chr_chunks);
	logerror("**\n");

	/* Attempt to load a battery file for this ROM. If successful, we */
	/* must wait until later to move it to the system memory. */
	if (nes.battery)
		image_battery_load(image, battery_data, BATTERY_SIZE);

	return INIT_PASS;

outofmemory:
	logerror("Memory allocation failed reading roms!\n");
	return INIT_FAIL;

bad:
	logerror("BAD section hit during LOAD ROM.\n");
	return INIT_FAIL;
}

UINT32 nes_partialcrc(const unsigned char *buf, size_t size)
{
	UINT32 crc;
	if (size < 17)
		return 0;
	crc = (UINT32) crc32(0L, &buf[16], size-16);
	logerror("NES Partial CRC: %08lx %d\n", (long) crc, (int)size);
	return crc;
}

DEVICE_LOAD(nes_disk)
{
	unsigned char magic[4];

	/* See if it has a fucking redundant header on it */
	mame_fread (file, magic, 4);
	if ((magic[0] == 'F') &&
		(magic[1] == 'D') &&
		(magic[2] == 'S'))
	{
		/* Skip past the fucking redundant header */
		mame_fseek (file, 0x10, SEEK_SET);
	}
	else
		/* otherwise, point to the start of the image */
		mame_fseek (file, 0, SEEK_SET);

	/* clear some of the cart variables we don't use */
	nes.trainer = 0;
	nes.battery = 0;
	nes.prg_chunks = nes.chr_chunks = 0;

	nes.mapper = 20;
	nes.four_screen_vram = 0;
	nes.hard_mirroring = 0;

	nes_fds.sides = 0;
	nes_fds.data = NULL;

	/* read in all the sides */
	while (!mame_feof (file))
	{
		nes_fds.sides ++;
		nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);
		if (!nes_fds.data)
			return INIT_FAIL;
		mame_fread (file, nes_fds.data + ((nes_fds.sides-1) * 65500), 65500);
	}

	/* adjust for eof */
	nes_fds.sides --;
	nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);

	logerror ("Number of sides: %d\n", nes_fds.sides);

	return INIT_PASS;

//bad:
	logerror("BAD section hit during disk load.\n");
	return 1;
}

DEVICE_UNLOAD(nes_disk)
{
	/* TODO: should write out changes here as well */
	nes_fds.data = NULL;
}

void ppu_mirror_custom (int page, int address)
{
	osd_die("Unimplemented");
}

void ppu_mirror_custom_vrom (int page, int address)
{
	osd_die("Unimplemented");
}
