#include <stdio.h>
#include "driver.h"
#include "image.h"
#include "includes/sms.h"

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif /* MIN */

UINT8 smsRomPageCount;
UINT8 smsBiosPageCount;
UINT8 smsFMDetect;
UINT8 smsVersion;

int systemType;

UINT8 biosPort;

UINT8 smsNVRam[NVRAM_SIZE];
int smsNVRAMSaved = 0;
UINT8 ggSIO[5] = { 0x7F, 0xFF, 0x00, 0xFF, 0x00 };

WRITE_HANDLER(sms_fm_detect_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		smsFMDetect = (data & 0x01);
	}
}

READ_HANDLER(sms_fm_detect_r) {
	if (biosPort & IO_CHIP) {
		return (0xFF);
	}

	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		return smsFMDetect;
	} else {
		return readinputport(0);
	}
}

WRITE_HANDLER(sms_version_w) {
	if ((data & 0x01) && (data & 0x04)) {
		smsVersion = (data & 0xA0);
	}
}

READ_HANDLER(sms_version_r) {
	UINT8 temp;

	if (biosPort & IO_CHIP) {
		return (0xFF);
	}

	/* Move bits 7,5 of port 3F into bits 7, 6 */
	temp = (smsVersion & 0x80) | (smsVersion & 0x20) << 1;

	/* Inverse version detect value for Japanese machines */
	if (IS_SMS_J_V21 || IS_SMS_J_M3 || IS_SMS_J_SS) {
		temp ^= 0xC0;
	}

	/* Merge version data with input port #2 data */
	temp = (temp & 0xC0) | (readinputport(1) & 0x3F);

	return (temp);
}

READ_HANDLER(sms_input_port_0_r) {
	if (biosPort & IO_CHIP) {
		return (0xFF);
	} else {
		return readinputport(0);
	}
}

WRITE_HANDLER(sms_YM2413_register_port_0_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		YM2413_register_port_0_w(offset, (data & 0x3F));
	}
}

WRITE_HANDLER(sms_YM2413_data_port_0_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		logerror("data_port_0_w %x %x\n", offset, data);
		YM2413_data_port_0_w(offset, data);
	}
}

READ_HANDLER(gg_input_port_2_r) {
	//logerror("joy 2 read, val: %02x, pc: %04x\n", (((IS_GG_UE || IS_GG_MAJ_UE) ? 0x40 : 0x00) | (readinputport(2) & 0x80)), activecpu_get_pc());
	return (((IS_GG_UE || IS_GG_MAJ_UE) ? 0x40 : 0x00) | (readinputport(2) & 0x80));
}

WRITE_HANDLER(sms_mapper_w)
{
	int page;
	UINT8 *RAM = memory_region(REGION_CPU1);
	UINT8 *USER_RAM;
	size_t user_ram_size;

	offset &= 3;

	RAM[0xDFFC + offset] = data;
	RAM[0xFFFC + offset] = data;


	if (biosPort & IO_BIOS_ROM)
	{
		if (!(biosPort & IO_CARTRIDGE))
		{
			page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;

			USER_RAM = memory_region(REGION_USER2);
			if (USER_RAM == NULL)
				return;
			user_ram_size = memory_region_length(REGION_USER2);
		}
		else
		{
			/* nothing to page in */
			return;
		}
	}
	else
	{
		page = (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0;

		USER_RAM = memory_region(REGION_USER1);
		if (!USER_RAM)
			return;

		if (((page + 1) * 0x4000) > memory_region_length(REGION_USER1))
			return;

		user_ram_size = memory_region_length(REGION_USER1);
	}

	switch(offset) {
		case 0: /* Control */
			/* Is it ram or rom? */
			if (data & 0x08) { /* it's ram */
				smsNVRAMSaved = 1;			/* SRAM should be saved on exit. */
				if (data & 0x04) {
#ifdef LOG_PAGING
					logerror("ram 1 paged.\n");
#endif
					memcpy(&RAM[0x8000], &smsNVRam[0x4000], 0x4000);
				} else {
#ifdef LOG_PAGING
					logerror("ram 0 paged.\n");
#endif
					memcpy(&RAM[0x8000], &smsNVRam[0x0000], 0x4000);
				}
			} else { /* it's rom */
				if (biosPort & IO_BIOS_ROM) {
					page = (smsRomPageCount > 0) ? RAM[0xFFFF] % smsRomPageCount : 0;
				} else {
					page = (smsBiosPageCount > 0) ? RAM[0xFFFF] % smsBiosPageCount : 0;
				}
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				memcpy(&RAM[0x8000], &USER_RAM[0x0000 + (page * 0x4000)], 0x4000);
			}
			break;
		case 1: /* Select 16k ROM bank for 0400-3FFF */
#ifdef LOG_PAGING
			logerror("rom 0 paged in %x.\n", page);
#endif
			memcpy(&RAM[0x0400], &USER_RAM[0x0000 + ((page * 0x4000) + 0x0400)], 0x3C00);
			break;
		case 2: /* Select 16k ROM bank for 4000-7FFF */
#ifdef LOG_PAGING
			logerror("rom 1 paged in %x.\n", page);
#endif
			memcpy(&RAM[0x4000], &USER_RAM[0x0000 + (page * 0x4000)], 0x4000);
			break;
		case 3: /* Select 16k ROM bank for 8000-BFFF */
			/* Is it ram or rom? */
			if (!(RAM[0xFFFC] & 0x08)) { /* it's rom */
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				memcpy(&RAM[0x8000], &USER_RAM[0x0000 + (page * 0x4000)], 0x4000);
			}
			break;
	}
}

WRITE_HANDLER(sms_bios_w) {
	biosPort = data;

	logerror("bios write %02x, pc: %04x\n", data, activecpu_get_pc());

	setup_rom();
}

WRITE_HANDLER(sms_cartram_w) {
	int page;
	UINT8 *RAM = memory_region(REGION_CPU1);
	UINT8 *USER_RAM;

	if (RAM[0xFFFC] & 0x08) {
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (RAM[0xFFFC] & 0x04) {
			smsNVRam[offset + 0x4000] = data;
		} else {
			smsNVRam[offset] = data;
		}
		RAM[offset + 0x8000] = data;
	} else {
		if (offset == 0) { /* Codemasters mapper */
			if (biosPort & IO_BIOS_ROM) {
				page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;
			} else {
				page = (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0;
			}
			USER_RAM = (biosPort & IO_BIOS_ROM) ? memory_region(REGION_USER2) : memory_region(REGION_USER1);
			if (USER_RAM == NULL) {
				return;
			}
			memcpy(&RAM[0x8000], &USER_RAM[0x0000 + (page * 0x4000)], 0x4000);
#ifdef LOG_PAGING
			logerror("rom 2 paged in %x codemasters.\n", page);
#endif
		} else {
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}

WRITE_HANDLER(sms_ram_w) {
	UINT8 *RAM = memory_region(REGION_CPU1);

	RAM[0xC000 + (offset & 0x1FFF)] = data;
	if ((offset & 0x1FFF) <= 0x1FFB) {
		RAM[0xE000 + (offset & 0x1FFF)] = data;
	}
}

READ_HANDLER(gg_dummy_r) {
	return 0xFF;
}

WRITE_HANDLER(gg_sio_w) {
	logerror("*** write %02X to SIO register #%d\n", data, offset);

	ggSIO[offset & 0x07] = data;
	switch(offset & 7) {
		case 0x00: /* Parallel Data */
			break;
		case 0x01: /* Data Direction/ NMI Enable */
			break;
		case 0x02: /* Serial Output */
			break;
		case 0x03: /* Serial Input */
			break;
		case 0x04: /* Serial Control / Status */
			break;
	}
}

READ_HANDLER(gg_sio_r) {
	logerror("*** read SIO register #%d\n", offset);

	switch(offset & 7) {
		case 0x00: /* Parallel Data */
			return ggSIO[0];
			break;
		case 0x01: /* Data Direction/ NMI Enable */
			break;
		case 0x02: /* Serial Output */
			break;
		case 0x03: /* Serial Input */
			break;
		case 0x04: /* Serial Control / Status */
			break;
	}

	return (0x00);
}

READ_HANDLER(gg_psg_r) {
	return 0xFF;
}

WRITE_HANDLER(gg_psg_w) {
	logerror("write %02X to psg at offset #%d.\n",data , offset);

	/* D7 = Noise Left */
	/* D6 = Tone3 Left */
	/* D5 = Tone2 Left */
	/* D4 = Tone1 Left */

	/* D3 = Noise Right */
	/* D2 = Tone3 Right */
	/* D1 = Tone2 Right */
	/* D0 = Tone1 Right */
}

NVRAM_HANDLER(sms) {
	if (file) {
		if (read_or_write) {
			if (smsNVRAMSaved) {
				mame_fwrite(file, &smsNVRam[0x0000], sizeof(UINT8) * NVRAM_SIZE);
			}
		} else {
			mame_fread(file, &smsNVRam[0x0000], sizeof(UINT8) * NVRAM_SIZE);
		}
	} else {
		/* initially zero out SRAM */
		memset(smsNVRam, 0, sizeof(UINT8) * NVRAM_SIZE);
	}
}

void setup_rom(void)
{
	UINT8 *RAM;
	UINT8 *USER_RAM;
	size_t memregion_length;

	/* Load up first 32K of image */
	RAM = memory_region(REGION_CPU1);
	if (!(biosPort & IO_BIOS_ROM))
	{
		if (IS_GG_UE || IS_GG_J)
		{
			USER_RAM = memory_region(REGION_USER2);
			if (!USER_RAM)
				return;

			memregion_length = memory_region_length(REGION_USER2);
			smsBiosPageCount = 0;
			smsRomPageCount = memregion_length / 0x4000;

			memcpy(&RAM[0x0000], &USER_RAM[0x0000], MIN(0x4000, memregion_length));			/* Only the first 2 banks are paged in */
			if (memregion_length >= 0x4000)
				memcpy(&RAM[0x4000], &USER_RAM[0x4000], MIN(0x4000, memregion_length - 0x4000));
			logerror("bios general loaded.\n");
		}
		else if (IS_GG_MAJ_UE || IS_GG_MAJ_J)
		{
			USER_RAM = memory_region(REGION_USER1);
			if (!USER_RAM)
				return;

			smsBiosPageCount = (memory_region_length(REGION_USER1) / 0x4000);

			memcpy(&RAM[0x0000], &USER_RAM[0x0000], 0x0400);
			logerror("bios 0x0400 loaded.\n");

			USER_RAM = memory_region(REGION_USER2);
			if (!USER_RAM)
				return;

			smsRomPageCount = (memory_region_length(REGION_USER2) / 0x4000);
			memcpy(&RAM[0x0400], &USER_RAM[0x0400], 0x3C00);			/* Only the first 2 banks are paged in */
			memcpy(&RAM[0x4000], &USER_RAM[0x4000], 0x4000);
			logerror("bios general loaded.\n");
		}
		else
		{
			if ((biosPort & IO_EXPANSION) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_CARD))
			{
				switch (systemType) {
					case CONSOLE_SMS_U_V13:
					case CONSOLE_SMS_E_V13:
					case CONSOLE_SMS_U_HACK_V13:
					case CONSOLE_SMS_E_HACK_V13:
					case CONSOLE_SMS_J_V21:
					case CONSOLE_SMS_J_M3:
					case CONSOLE_SMS_J_SS:
						USER_RAM = memory_region(REGION_USER1);
						if (!USER_RAM)
							return;

						smsBiosPageCount = (memory_region_length(REGION_USER1) / 0x4000);
						memcpy(&RAM[0x0000], &USER_RAM[0x0000], 0x2000);
						logerror("bios 0x2000 loaded.\n");
						break;
					case CONSOLE_SMS_U_ALEX:
					case CONSOLE_SMS_E_ALEX:
					case CONSOLE_SMS_E_SONIC:
					case CONSOLE_SMS_B_SONIC:
					case CONSOLE_SMS_U_HOSH_V24:
					case CONSOLE_SMS_E_HOSH_V24:
					case CONSOLE_SMS_U_HO_V34:
					case CONSOLE_SMS_E_HO_V34:
//					case CONSOLE_SMS_U_MD_3D:
//					case CONSOLE_SMS_E_MD_3D:
						USER_RAM = memory_region(REGION_USER1);
						if (!USER_RAM)
							return;

						smsBiosPageCount = (memory_region_length(REGION_USER1) / 0x4000);
						memcpy(&RAM[0x0000], &USER_RAM[0x0000], 0x4000);			/* Only the first 2 banks are paged in */
						memcpy(&RAM[0x4000], &USER_RAM[0x4000], 0x4000);
						logerror("bios full loaded.\n");
						break;
					case CONSOLE_SMS:
						USER_RAM = memory_region(REGION_USER2);
						if (!USER_RAM)
							return;

						smsRomPageCount = (memory_region_length(REGION_USER2) / 0x4000);
						memcpy(&RAM[0x0000], &USER_RAM[0x0000], 0x4000);			/* Only the first 2 banks are paged in */
						memcpy(&RAM[0x4000], &USER_RAM[0x4000], 0x4000);
						logerror("bios general loaded.\n");
						break;
				}
			}
		}
	}
	else
	{
		switch (systemType) {
			case CONSOLE_SMS_U_V13:
			case CONSOLE_SMS_E_V13:
			case CONSOLE_SMS_U_HACK_V13:
			case CONSOLE_SMS_E_HACK_V13:
			case CONSOLE_SMS_J_V21:
			case CONSOLE_SMS_J_M3:
			case CONSOLE_SMS_J_SS:
			case CONSOLE_SMS_U_ALEX:
			case CONSOLE_SMS_E_ALEX:
			case CONSOLE_SMS_E_SONIC:
			case CONSOLE_SMS_B_SONIC:
			case CONSOLE_SMS_U_HOSH_V24:
			case CONSOLE_SMS_E_HOSH_V24:
			case CONSOLE_SMS_U_HO_V34:
			case CONSOLE_SMS_E_HO_V34:
			case CONSOLE_SMS_U_MD_3D:
			case CONSOLE_SMS_E_MD_3D:
			case CONSOLE_SMS:
			case CONSOLE_SMS_PAL:
				if (!(biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION) && (biosPort & IO_CARD))
				{
					/* Load up first 32K of image */
					USER_RAM = memory_region(REGION_USER2);
					if (!USER_RAM)
						return;

					memregion_length = memory_region_length(REGION_USER2);
					smsRomPageCount = memregion_length / 0x4000;
					memcpy(&RAM[0x0000], &USER_RAM[0x0000], MIN(0x4000, memregion_length));			/* Only the first 2 banks are paged in */
					if (memregion_length >= 0x4000)
						memcpy(&RAM[0x4000], &USER_RAM[0x4000], MIN(0x4000, memregion_length - 0x4000));
					logerror("cart full loaded.\n");
				}
				else if (!(biosPort & IO_CARD) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION))
				{
					/* Clear out card rom */
					memset(RAM, 0, sizeof(UINT8) * 0x8000);
				}
				else if (!(biosPort & IO_EXPANSION) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_CARD))
				{
					/* Clear out expansion rom */
					memset(RAM, 0, sizeof(UINT8) * 0x8000);
				}
				break;

			case CONSOLE_GG_UE:
			case CONSOLE_GG_J:
			case CONSOLE_GG_MAJ_UE:
			case CONSOLE_GG_MAJ_J:
				/* Load up first 32K of image */
				USER_RAM = memory_region(REGION_USER2);
				if (!USER_RAM)
					return;

				smsRomPageCount = (memory_region_length(REGION_USER2) / 0x4000);
				memcpy(&RAM[0x0000], &USER_RAM[0x0000], 0x4000);			/* Only the first 2 banks are paged in */
				memcpy(&RAM[0x4000], &USER_RAM[0x4000], 0x4000);
				logerror("cart full loaded.\n");
				break;
		}
	}
}

static int sms_verify_cart(char * magic, int size) {
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000) {
		if (!strncmp(&magic[0x7FF0], "TMR SEGA", 8)) {
			/* Technically, it should be this, but remove for now until verified:
			if (!strcmp(sysname, "gamegear")) {
				if ((unsigned char)magic[0x7FFD] < 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			if (!strcmp(sysname, "sms")) {
				if ((unsigned char)magic[0x7FFD] >= 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			*/
			retval = IMAGE_VERIFY_PASS;
		}
	}

		/* Check at $81f0 also */
		//if (!retval) {
	 //	 if (!strncmp(&magic[0x81f0], "TMR SEGA", 8)) {
				/* Technically, it should be this, but remove for now until verified:
				if (!strcmp(sysname, "gamegear")) {
					if ((unsigned char)magic[0x81fd] < 0x50)
						retval = IMAGE_VERIFY_PASS;
				}
				if (!strcmp(sysname, "sms")) {
					if ((unsigned char)magic[0x81fd] >= 0x50)
						retval = IMAGE_VERIFY_PASS;
				}
				*/
		 //	 retval = IMAGE_VERIFY_PASS;
		//	}
		//}

	return retval;
}

DEVICE_LOAD( sms_cart )
{
	int size;
	UINT8 *USER_RAM, *RAM;

	if (!strcmp(Machine->gamedrv->name, "sms")) {
		systemType = CONSOLE_SMS;
	} else if (!strcmp(Machine->gamedrv->name, "smspal")) {
		systemType = CONSOLE_SMS_PAL;
	} else if (!strcmp(Machine->gamedrv->name, "smsu13")) {
		systemType = CONSOLE_SMS_U_V13;
	} else if (!strcmp(Machine->gamedrv->name, "smse13")) {
		systemType = CONSOLE_SMS_E_V13;
	} else if (!strcmp(Machine->gamedrv->name, "smsu13h")) {
		systemType = CONSOLE_SMS_U_HACK_V13;
	} else if (!strcmp(Machine->gamedrv->name, "smse13h")) {
		systemType = CONSOLE_SMS_E_HACK_V13;
	} else if (!strcmp(Machine->gamedrv->name, "smsuam")) {
		systemType = CONSOLE_SMS_U_ALEX;
	} else if (!strcmp(Machine->gamedrv->name, "smseam")) {
		systemType = CONSOLE_SMS_E_ALEX;
	} else if (!strcmp(Machine->gamedrv->name, "smsesh")) {
		systemType = CONSOLE_SMS_E_SONIC;
	} else if (!strcmp(Machine->gamedrv->name, "smsbsh")) {
		systemType = CONSOLE_SMS_B_SONIC;
	} else if (!strcmp(Machine->gamedrv->name, "smsumd3d")) {
		systemType = CONSOLE_SMS_U_MD_3D;
	} else if (!strcmp(Machine->gamedrv->name, "smsemd3d")) {
		systemType = CONSOLE_SMS_E_MD_3D;
	} else if (!strcmp(Machine->gamedrv->name, "smsuhs24")) {
		systemType = CONSOLE_SMS_U_HOSH_V24;
	} else if (!strcmp(Machine->gamedrv->name, "smsehs24")) {
		systemType = CONSOLE_SMS_E_HOSH_V24;
	} else if (!strcmp(Machine->gamedrv->name, "smsuh34")) {
		systemType = CONSOLE_SMS_U_HO_V34;
	} else if (!strcmp(Machine->gamedrv->name, "smseh34")) {
		systemType = CONSOLE_SMS_E_HO_V34;
	} else if (!strcmp(Machine->gamedrv->name, "smsj21")) {
		systemType = CONSOLE_SMS_J_V21;
	} else if (!strcmp(Machine->gamedrv->name, "smsm3")) {
		systemType = CONSOLE_SMS_J_M3;
	} else if (!strcmp(Machine->gamedrv->name, "smsss")) {
		systemType = CONSOLE_SMS_J_SS;
	} else if (!strcmp(Machine->gamedrv->name, "gamegear")) {
		systemType = CONSOLE_GG_UE;
	} else if (!strcmp(Machine->gamedrv->name, "gamegj")) {
		systemType = CONSOLE_GG_J;
	} else if (!strcmp(Machine->gamedrv->name, "gamg")) {
		systemType = CONSOLE_GG_MAJ_UE;
	} else if (!strcmp(Machine->gamedrv->name, "gamgj")) {
		systemType = CONSOLE_GG_MAJ_J;
	} else {
		logerror("Invalid system name.\n");
		return (INIT_FAIL);
	}

	/* Get file size */
	size = mame_fsize(file);

	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		mame_fseek(file, 512, SEEK_SET);
		size -= 512;
	}

	/* Get base of CPU1 memory region */
	if (new_memory_region(REGION_USER2, size, ROM_REQUIRED)) {
		logerror("Memory allocation failed reading roms!\n");
		return (INIT_FAIL);
	}
	USER_RAM = memory_region(REGION_USER2);

	/* Load ROM banks */
	size = mame_fread(file, &USER_RAM[0x0000], size);

	/* check the image */
	if (!IS_SMS && !IS_SMS_PAL && !IS_GG_UE && !IS_GG_J) {
		if (sms_verify_cart((char*)&USER_RAM[0x0000], size) == IMAGE_VERIFY_FAIL) {
			logerror("Invalid Image\n");
			return INIT_FAIL;
		}
	}

	biosPort = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if (IS_SMS || IS_SMS_PAL) {
		biosPort &= ~(IO_CARTRIDGE);
		biosPort |= IO_BIOS_ROM;
	}

	/* initially zero out CPU_RAM */
	RAM = memory_region(REGION_CPU1);
	memset(RAM, 0, sizeof(UINT8) * CPU_ADDRESSABLE_SIZE);

	setup_rom();

	return (INIT_PASS);
}

MACHINE_INIT(sms) {
	smsVersion = 0x00;
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		smsFMDetect = 0x01;
	}
}

MACHINE_STOP(sms) {
//	currentLine = 0;
}

