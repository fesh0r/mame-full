#include <stdio.h>
#include "driver.h"
#include "image.h"
#include "includes/sms.h"
#include "sound/2413intf.h"

UINT8 smsRomPageCount;
UINT8 smsBiosPageCount;
UINT8 smsFMDetect;
UINT8 smsVersion;
int smsPaused;

int systemType;

UINT8 biosPort;

UINT8 *BIOS;
UINT8 *ROM;

UINT8 *sms_mapper;
UINT8 smsNVRam[NVRAM_SIZE];
int smsNVRAMSaved = 0;
UINT8 ggSIO[5] = { 0x7F, 0xFF, 0x00, 0xFF, 0x00 };

WRITE8_HANDLER(sms_fm_detect_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		smsFMDetect = (data & 0x01);
	}
}

 READ8_HANDLER(sms_fm_detect_r) {
	if (biosPort & IO_CHIP) {
		return (0xFF);
	}

	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		return smsFMDetect;
	} else {
		return readinputport(0);
	}
}

WRITE8_HANDLER(sms_version_w) {
	if ((data & 0x01) && (data & 0x04)) {
		smsVersion = (data & 0xA0);
	}
}

 READ8_HANDLER(sms_version_r) {
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

 READ8_HANDLER(sms_input_port_0_r) {
	if ( ! IS_GG_ANY ) {
		if ( !(readinputport(2) & 0x80) && !smsPaused ) {
			smsPaused = 1;
			cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
			cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
		} else {
			smsPaused = 0;
		}
	}

	if (biosPort & IO_CHIP) {
		return (0xFF);
	} else {
		return readinputport(0);
	}
}

WRITE8_HANDLER(sms_YM2413_register_port_0_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		YM2413_register_port_0_w(offset, (data & 0x3F));
	}
}

WRITE8_HANDLER(sms_YM2413_data_port_0_w) {
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		logerror("data_port_0_w %x %x\n", offset, data);
		YM2413_data_port_0_w(offset, data);
	}
}

 READ8_HANDLER(gg_input_port_2_r) {
	//logerror("joy 2 read, val: %02x, pc: %04x\n", (((IS_GG_UE || IS_GG_MAJ_UE) ? 0x40 : 0x00) | (readinputport(2) & 0x80)), activecpu_get_pc());
	return (((IS_GG_UE || IS_GG_MAJ_UE) ? 0x40 : 0x00) | (readinputport(2) & 0x80));
}

 READ8_HANDLER(sms_mapper_r)
{
	return sms_mapper[offset];
}

WRITE8_HANDLER(sms_mapper_w)
{
	int page;
	UINT8 *SOURCE;
	size_t user_ram_size;

	offset &= 3;

	sms_mapper[offset] = data;

	if (biosPort & IO_BIOS_ROM)
	{
		if (!(biosPort & IO_CARTRIDGE))
		{
			page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;

			if ( ! ROM )
				return;
			SOURCE = ROM;
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

		if ( ! BIOS )
			return;

		SOURCE = BIOS;
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
					memory_set_bankptr( 4, smsNVRam + 0x4000 );
				} else {
#ifdef LOG_PAGING
					logerror("ram 0 paged.\n");
#endif
					memory_set_bankptr( 4, smsNVRam );
				}
			} else { /* it's rom */
				if (biosPort & IO_BIOS_ROM) {
					page = (smsRomPageCount > 0) ? sms_mapper[3] % smsRomPageCount : 0;
				} else {
					page = (smsBiosPageCount > 0) ? sms_mapper[3] % smsBiosPageCount : 0;
				}
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				memory_set_bankptr( 4, SOURCE + ( page * 0x4000 ) );
			}
			break;
		case 1: /* Select 16k ROM bank for 0400-3FFF */
#ifdef LOG_PAGING
			logerror("rom 0 paged in %x.\n", page);
#endif
			memory_set_bankptr( 2, SOURCE + ( page * 0x4000 ) + 0x0400 );
			break;
		case 2: /* Select 16k ROM bank for 4000-7FFF */
#ifdef LOG_PAGING
			logerror("rom 1 paged in %x.\n", page);
#endif
			memory_set_bankptr( 3, SOURCE + ( page * 0x4000 ) );
			break;
		case 3: /* Select 16k ROM bank for 8000-BFFF */
			/* Is it ram or rom? */
			if (!(sms_mapper[0] & 0x08)) { /* it's rom */
#ifdef LOG_PAGING
				logerror("rom 2 paged in %x.\n", page);
#endif
				memory_set_bankptr( 4, SOURCE + ( page * 0x4000 ) );
			}
			break;
	}
}

WRITE8_HANDLER(sms_bios_w) {
	biosPort = data;

	logerror("bios write %02x, pc: %04x\n", data, activecpu_get_pc());

	setup_rom();
}

WRITE8_HANDLER(sms_cartram_w) {
	int page;
	UINT8 *SOURCE;

	if (sms_mapper[0] & 0x08) {
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (sms_mapper[0] & 0x04) {
			smsNVRam[offset + 0x4000] = data;
		} else {
			smsNVRam[offset] = data;
		}
	} else {
		if (offset == 0) { /* Codemasters mapper */
			if (biosPort & IO_BIOS_ROM) {
				page = (smsRomPageCount > 0) ? data % smsRomPageCount : 0;
			} else {
				page = (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0;
			}
			SOURCE = (biosPort & IO_BIOS_ROM) ? ROM : BIOS;
			if ( ! SOURCE )
				return;
			memory_set_bankptr( 4, SOURCE + ( page * 0x4000 ) );
#ifdef LOG_PAGING
			logerror("rom 2 paged in %x codemasters.\n", page);
#endif
		} else {
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}

/*WRITE8_HANDLER(sms_ram_w) {
	UINT8 *RAM = memory_region(REGION_CPU1);

	RAM[0xC000 + (offset & 0x1FFF)] = data;
	if ((offset & 0x1FFF) <= 0x1FFB) {
		RAM[0xE000 + (offset & 0x1FFF)] = data;
	}
}*/

WRITE8_HANDLER(gg_sio_w) {
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

 READ8_HANDLER(gg_sio_r) {
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

 READ8_HANDLER(gg_psg_r) {
	return 0xFF;
}

WRITE8_HANDLER(gg_psg_w) {
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
				mame_fwrite(file, smsNVRam, sizeof(UINT8) * NVRAM_SIZE);
			}
		} else {
			mame_fread(file, smsNVRam, sizeof(UINT8) * NVRAM_SIZE);
		}
	} else {
		/* initially zero out SRAM */
		memset(smsNVRam, 0, sizeof(UINT8) * NVRAM_SIZE);
	}
}

void setup_rom(void)
{
	BIOS = memory_region(REGION_USER1);
	ROM = memory_region(REGION_USER2);

	smsBiosPageCount = ( BIOS ? memory_region_length(REGION_USER1) / 0x4000 : 0 );
	smsRomPageCount = ( ROM ? memory_region_length(REGION_USER2) / 0x4000 : 0 );

	/* Load up first 32K of image */
	memory_set_bankptr( 1, memory_region(REGION_CPU1) );
	memory_set_bankptr( 2, memory_region(REGION_CPU1) + 0x0400 );
	memory_set_bankptr( 3, memory_region(REGION_CPU1) + 0x4000 );
	memory_set_bankptr( 4, memory_region(REGION_CPU1) + 0x8000 );

	if (!(biosPort & IO_BIOS_ROM))
	{
		if (IS_GG_UE || IS_GG_J)
		{
			if ( ! ROM )
				return;

			memory_set_bankptr( 1, ROM );			/* first 2 bank are switched in */
			memory_set_bankptr( 2, ROM + 0x0400 );
			memory_set_bankptr( 3, ROM + ( (smsRomPageCount > 1) ? 0x4000 : 0 ) );
			logerror("bios general loaded.\n");
		}
		else if (IS_GG_MAJ_UE || IS_GG_MAJ_J)
		{
			if ( ! BIOS )
				return;

			memory_set_bankptr( 1, BIOS );
			logerror("bios 0x0400 loaded.\n");

			if ( ! ROM )
				return;

			memory_set_bankptr( 2, ROM + 0x0400 );
			memory_set_bankptr( 3, ROM + ( (smsRomPageCount > 1) ? 0x4000 : 0 ) );
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
						if ( ! BIOS )
							return;

						memory_set_bankptr( 1, BIOS );
						memory_set_bankptr( 2, BIOS + 0x0400 );
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
						if ( ! BIOS )
							return;

						memory_set_bankptr( 1, BIOS );
						memory_set_bankptr( 2, BIOS + 0x0400 );
						memory_set_bankptr( 3, BIOS + 0x4000 );
						logerror("bios full loaded.\n");
						break;
					case CONSOLE_SMS:
						if ( ! ROM )
							return;

						memory_set_bankptr( 1, ROM );
						memory_set_bankptr( 2, ROM + 0x0400 );
						memory_set_bankptr( 3, ROM + ( (smsRomPageCount > 1 ) ? 0x4000 : 0 ) );
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
					if ( ! ROM )
						return;

					memory_set_bankptr( 1, ROM );
					memory_set_bankptr( 2, ROM + 0x0400 );
					memory_set_bankptr( 3, ROM + ( (smsRomPageCount > 1) ? 0x4000 : 0 ) );
					logerror("cart full loaded.\n");
				}
				else if (!(biosPort & IO_CARD) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION))
				{
					/* Clear out card rom */
				}
				else if (!(biosPort & IO_EXPANSION) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_CARD))
				{
					/* Clear out expansion rom */
				}
				break;

			case CONSOLE_GG_UE:
			case CONSOLE_GG_J:
			case CONSOLE_GG_MAJ_UE:
			case CONSOLE_GG_MAJ_J:
				/* Load up first 32K of image */
				if ( ! ROM )
					return;

				memory_set_bankptr( 1, ROM );
				memory_set_bankptr( 2, ROM + 0x0400 );
				memory_set_bankptr( 3, ROM + ( (smsRomPageCount > 1) ? 0x4000 : 0 ) );
				logerror("cart full loaded.\n");
				break;
		}
	}
}

static int sms_verify_cart(UINT8 *magic, int size) {
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000) {
		if (!strncmp((char*)&magic[0x7FF0], "TMR SEGA", 8)) {
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

DEVICE_INIT( sms_cart )
{
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
		return INIT_FAIL;
	}

	biosPort = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if (IS_SMS || IS_SMS_PAL) {
		biosPort &= ~(IO_CARTRIDGE);
		biosPort |= IO_BIOS_ROM;
	}

	return INIT_PASS;
}

DEVICE_LOAD( sms_cart )
{
	int size;

	/* Get file size */
	size = mame_fsize(file);

	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		mame_fseek(file, 512, SEEK_SET);
		size -= 512;
	}

	if ( ! size ) {
		logerror("ROM image too small!\n");
		return INIT_FAIL;
	}

	/* Create a new memory region to hold the ROM. */
	/* Make sure the region holds only complete (0x4000) rom banks */
	if (new_memory_region(REGION_USER2, ((size&0x3FFF) ? (((size>>14)+1)<<14) : size), ROM_REQUIRED)) {
		logerror("Memory allocation failed reading roms!\n");
		return INIT_FAIL;
	}
	ROM = memory_region(REGION_USER2);

	/* Load ROM banks */
	size = mame_fread(file, ROM, size);

	/* check the image */
	if (!IS_SMS && !IS_SMS_PAL && !IS_GG_UE && !IS_GG_J) {
		if (sms_verify_cart(ROM, size) == IMAGE_VERIFY_FAIL) {
			logerror("Invalid Image\n");
			return INIT_FAIL;
		}
	}

	return INIT_PASS;
}

MACHINE_INIT(sms)
{
	smsVersion = 0x00;
	if (IS_SMS_J_V21 || IS_SMS_J_SS) {
		smsFMDetect = 0x01;
	}

	memset( memory_region(REGION_CPU1), 0xff, 0x10000 );

	sms_mapper = memory_get_write_ptr( 0, ADDRESS_SPACE_PROGRAM, 0xDFFC );

	setup_rom();
}

