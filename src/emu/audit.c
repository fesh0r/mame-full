/***************************************************************************

    audit.c

    ROM set auditing functions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "hash.h"
#include "audit.h"
#include "harddisk.h"
#include "sound/samples.h"



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static const game_driver *chd_gamedrv;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static int audit_one_rom(const rom_entry *rom, const game_driver *gamedrv, UINT32 validation, audit_record *record);
static int audit_one_disk(const rom_entry *rom, const game_driver *gamedrv, UINT32 validation, audit_record *record);
static int rom_used_by_parent(const game_driver *gamedrv, const rom_entry *romentry, const game_driver **parent);

static chd_interface_file *audit_chd_open(const char *filename, const char *mode);
static void audit_chd_close(chd_interface_file *file);
static UINT32 audit_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 audit_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 audit_chd_length(chd_interface_file *file);



/***************************************************************************
    HARD DISK INTERFACE
***************************************************************************/

static chd_interface audit_chd_interface =
{
	audit_chd_open,
	audit_chd_close,
	audit_chd_read,
	audit_chd_write,
	audit_chd_length
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    set_status - shortcut for setting status and
    substatus values
-------------------------------------------------*/

INLINE void set_status(audit_record *record, UINT8 status, UINT8 substatus)
{
	record->status = status;
	record->substatus = substatus;
}



/***************************************************************************
    CORE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    audit_images - validate the ROM and disk
    images for a game
-------------------------------------------------*/

int audit_images(int game, UINT32 validation, audit_record **audit)
{
	const game_driver *gamedrv = drivers[game];
	const rom_entry *region, *rom;
	audit_record *record;
	int foundany = FALSE;
	int allshared = TRUE;
	int records;

	/* determine the number of records we will generate */
	records = 0;
	for (region = rom_first_region(gamedrv); region != NULL; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom != NULL; rom = rom_next_file(rom))
			if (ROMREGION_ISROMDATA(region) || ROMREGION_ISDISKDATA(region))
			{
				if (allshared && !rom_used_by_parent(gamedrv, rom, NULL))
					allshared = FALSE;
				records++;
			}

	if (records > 0)
	{
		/* allocate memory for the records */
		*audit = malloc_or_die(sizeof(**audit) * records);
		memset(*audit, 0, sizeof(**audit) * records);
		record = *audit;

		/* iterate over regions and ROMs */
		for (region = rom_first_region(drivers[game]); region; region = rom_next_region(region))
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				int shared = rom_used_by_parent(gamedrv, rom, NULL);

				/* audit a file */
				if (ROMREGION_ISROMDATA(region))
				{
					if (audit_one_rom(rom, gamedrv, validation, record++) && (!shared || allshared))
						foundany = TRUE;
				}

				/* audit a disk */
				else if (ROMREGION_ISDISKDATA(region))
				{
					if (audit_one_disk(rom, gamedrv, validation, record++) && (!shared || allshared))
						foundany = TRUE;
				}
			}

		/* if we found nothing, we don't have the set at all */
		if (!foundany)
		{
			free(*audit);
			*audit = NULL;
			records = 0;
		}
	}
	return records;
}


/*-------------------------------------------------
    audit_samples - validate the samples for a
    game
-------------------------------------------------*/

int audit_samples(int game, audit_record **audit)
{
	const game_driver *gamedrv = drivers[game];
	machine_config config;
	audit_record *record;
	int sndnum, sampnum;
	int records = 0;

	/* count the number of sample records attached to this driver */
	expand_machine_driver(gamedrv->drv, &config);
#if HAS_SAMPLES
	for (sndnum = 0; sndnum < ARRAY_LENGTH(config.sound); sndnum++)
		if (config.sound[sndnum].sound_type == SOUND_SAMPLES)
		{
			struct Samplesinterface *intf = (struct Samplesinterface *)config.sound[sndnum].config;

			if (intf->samplenames == NULL)
				continue;

			/* iterate over samples in this entry */
			for (sampnum = 0; intf->samplenames[sampnum] != NULL; sampnum++)
				if (intf->samplenames[sampnum][0] != '*')
					records++;
		}
#endif

	/* if no records, just quit now */
	if (records == 0)
		return records;

	/* allocate memory for the records */
	*audit = malloc_or_die(sizeof(**audit) * records);
	memset(*audit, 0, sizeof(**audit) * records);
	record = *audit;

	/* now iterate over sample entries */
	for (sndnum = 0; sndnum < ARRAY_LENGTH(config.sound); sndnum++)
		if (config.sound[sndnum].sound_type == SOUND_SAMPLES)
		{
			struct Samplesinterface *intf = (struct Samplesinterface *)config.sound[sndnum].config;
			const char *sharedname = NULL;

			/* iterate over samples in this entry */
			for (sampnum = 0; intf->samplenames[sampnum] != NULL; sampnum++)
				if (intf->samplenames[sampnum][0] == '*')
					sharedname = &intf->samplenames[sampnum][1];
				else
				{
					mame_file_error filerr;
					mame_file *file;
					char *fname;

					/* attempt to access the file from the game driver name */
					fname = assemble_3_strings(gamedrv->name, PATH_SEPARATOR, intf->samplenames[sampnum]);
					filerr = mame_fopen(SEARCHPATH_SAMPLE, fname, OPEN_FLAG_READ, &file);
					free(fname);

					/* attempt to access the file from the shared driver name */
					if (filerr != FILERR_NONE && sharedname != NULL)
					{
						fname = assemble_3_strings(sharedname, PATH_SEPARATOR, intf->samplenames[sampnum]);
						filerr = mame_fopen(SEARCHPATH_SAMPLE, fname, OPEN_FLAG_READ, &file);
						free(fname);
					}

					/* fill in the record */
					record->type = AUDIT_FILE_SAMPLE;
					record->name = intf->samplenames[sampnum];
					if (filerr == FILERR_NONE)
					{
						set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_GOOD);
						mame_fclose(file);
					}
					else
						set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND);
				}
		}

	return records;
}


/*-------------------------------------------------
    audit_summary - output a summary given a
    list of audit records
-------------------------------------------------*/

int audit_summary(int game, int count, const audit_record *records, int output)
{
	const game_driver *gamedrv = drivers[game];
	int overall_status = CORRECT;
	int notfound = 0;
	int recnum;

	/* no count or records means not found */
	if (count == 0 || records == NULL)
		return NOTFOUND;

	/* loop over records */
	for (recnum = 0; recnum < count; recnum++)
	{
		const audit_record *record = &records[recnum];
		int best_new_status = INCORRECT;

		/* skip anything that's fine */
		if (record->substatus == SUBSTATUS_GOOD)
			continue;

		/* count the number of missing items */
		if (record->status == AUDIT_STATUS_NOT_FOUND)
			notfound++;

		/* output the game name, file name, and length (if applicable) */
		if (output)
		{
			mame_printf_info("%-8s: %s", gamedrv->name, record->name);
			if (record->explength > 0)
				mame_printf_info(" (%d bytes)", record->explength);
			mame_printf_info(" - ");
		}

		/* use the substatus for finer details */
		switch (record->substatus)
		{
			case SUBSTATUS_GOOD_NEEDS_REDUMP:
				if (output) mame_printf_info("NEEDS REDUMP\n");
				best_new_status = BEST_AVAILABLE;
				break;

			case SUBSTATUS_FOUND_NODUMP:
				if (output) mame_printf_info("NO GOOD DUMP KNOWN\n");
				best_new_status = BEST_AVAILABLE;
				break;

			case SUBSTATUS_FOUND_BAD_CHECKSUM:
				if (output)
				{
					char hashbuf[512];

					mame_printf_info("INCORRECT CHECKSUM:\n");
					hash_data_print(record->exphash, 0, hashbuf);
					mame_printf_info("EXPECTED: %s\n", hashbuf);
					hash_data_print(record->hash, 0, hashbuf);
					mame_printf_info("   FOUND: %s\n", hashbuf);
				}
				break;

			case SUBSTATUS_FOUND_WRONG_LENGTH:
				if (output) mame_printf_info("INCORRECT LENGTH: %d bytes\n", record->length);
				break;

			case SUBSTATUS_NOT_FOUND:
				if (output) mame_printf_info("NOT FOUND\n");
				break;

			case SUBSTATUS_NOT_FOUND_NODUMP:
				if (output) mame_printf_info("NOT FOUND - NO GOOD DUMP KNOWN\n");
				best_new_status = BEST_AVAILABLE;
				break;

			case SUBSTATUS_NOT_FOUND_OPTIONAL:
				if (output) mame_printf_info("NOT FOUND BUT OPTIONAL\n");
				best_new_status = BEST_AVAILABLE;
				break;

			case SUBSTATUS_NOT_FOUND_PARENT:
				if (output) mame_printf_info("NOT FOUND (shared with parent)\n");
				break;

			case SUBSTATUS_NOT_FOUND_BIOS:
				if (output) mame_printf_info("NOT FOUND (BIOS)\n");
				break;
		}

		/* downgrade the overall status if necessary */
		overall_status = MAX(overall_status, best_new_status);
	}

	return (notfound == count) ? NOTFOUND : overall_status;
}



/***************************************************************************
    UTILITIES
***************************************************************************/

/*-------------------------------------------------
    audit_one_rom - validate a single ROM entry
-------------------------------------------------*/

static int audit_one_rom(const rom_entry *rom, const game_driver *gamedrv, UINT32 validation, audit_record *record)
{
	const game_driver *drv;
	const rom_entry *chunk;
	UINT32 crc = 0;
	UINT8 crcs[4];
	int has_crc;

	/* fill in the record basics */
	record->type = AUDIT_FILE_ROM;
	record->name = ROM_GETNAME(rom);
	record->exphash = ROM_GETHASHDATA(rom);

	/* compute the expected length by summing the chunks */
	for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
		record->explength += ROM_GETLENGTH(chunk);

	/* see if we have a CRC and extract it if so */
	has_crc = hash_data_extract_binary_checksum(record->exphash, HASH_CRC, crcs);
	if (has_crc)
		crc = (crcs[0] << 24) | (crcs[1] << 16) | (crcs[2] << 8) | crcs[3];

	/* find the file and checksum it, getting the file length along the way */
	for (drv = gamedrv; drv != NULL; drv = driver_get_clone(drv))
	{
		mame_file_error filerr;
		mame_file *file;
		char *fname;

		/* open the file if we can */
		fname = assemble_3_strings(drv->name, PATH_SEPARATOR, ROM_GETNAME(rom));
	    if (has_crc)
			filerr = mame_fopen_crc(SEARCHPATH_ROM, fname, crc, OPEN_FLAG_READ, &file);
		else
			filerr = mame_fopen(SEARCHPATH_ROM, fname, OPEN_FLAG_READ, &file);
		free(fname);

		/* if we got it, extract the hash and length */
		if (filerr == FILERR_NONE)
		{
			hash_data_copy(record->hash, mame_fhash(file, validation));
			record->length = (UINT32)mame_fsize(file);
			mame_fclose(file);
			break;
		}
	}

	/* if we failed to find the file, set the appropriate status */
	if (drv == NULL)
	{
		const game_driver *parent;

		/* no good dump */
		if (hash_data_has_info(record->exphash, HASH_INFO_NO_DUMP))
			set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND_NODUMP);

		/* optional ROM */
		else if (ROM_ISOPTIONAL(rom))
			set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND_OPTIONAL);

		/* not found and used by parent */
		else if (rom_used_by_parent(gamedrv, rom, &parent))
			set_status(record, AUDIT_STATUS_NOT_FOUND, (parent->flags & NOT_A_DRIVER) ? SUBSTATUS_NOT_FOUND_BIOS : SUBSTATUS_NOT_FOUND_PARENT);

		/* just plain old not found */
		else
			set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND);
	}

	/* if we did find the file, do additional verification */
	else
	{
		/* length mismatch */
		if (record->explength != record->length)
			set_status(record, AUDIT_STATUS_FOUND_INVALID, SUBSTATUS_FOUND_WRONG_LENGTH);

		/* found but needs a dump */
		else if (hash_data_has_info(record->exphash, HASH_INFO_NO_DUMP))
			set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_FOUND_NODUMP);

		/* incorrect hash */
		else if (!hash_data_is_equal(record->exphash, record->hash, 0))
			set_status(record, AUDIT_STATUS_FOUND_INVALID, SUBSTATUS_FOUND_BAD_CHECKSUM);

		/* correct hash but needs a redump */
		else if (hash_data_has_info(record->exphash, HASH_INFO_BAD_DUMP))
			set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_GOOD_NEEDS_REDUMP);

		/* just plain old good */
		else
			set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_GOOD);
	}

	/* return TRUE if we found anything at all */
	return (drv != NULL);
}


/*-------------------------------------------------
    audit_one_disk - validate a single disk entry
-------------------------------------------------*/

static int audit_one_disk(const rom_entry *rom, const game_driver *gamedrv, UINT32 validation, audit_record *record)
{
	chd_file *source;
	chd_error err;

	/* fill in the record basics */
	record->type = AUDIT_FILE_DISK;
	record->name = ROM_GETNAME(rom);
	record->exphash = ROM_GETHASHDATA(rom);

	/* open the disk */
	chd_gamedrv = gamedrv;
	chd_set_interface(&audit_chd_interface);
	err = open_disk_image(gamedrv, rom, &source);

	/* if we failed, report the error */
	if (err != CHDERR_NONE)
	{
		/* out of memory */
		if (err == CHDERR_OUT_OF_MEMORY)
			set_status(record, AUDIT_STATUS_ERROR, SUBSTATUS_ERROR);

		/* not found but it's not good anyway */
		else if (hash_data_has_info(record->exphash, HASH_INFO_NO_DUMP))
			set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND_NODUMP);

		/* not found at all */
		else
			set_status(record, AUDIT_STATUS_NOT_FOUND, SUBSTATUS_NOT_FOUND);
	}

	/* if we succeeded, validate it */
	else
	{
		static const UINT8 nullhash[HASH_BUF_SIZE] = { 0 };
		chd_header header = *chd_get_header(source);

		/* if there's an MD5 or SHA1 hash, add them to the output hash */
		if (memcmp(nullhash, header.md5, sizeof(header.md5)) != 0)
			hash_data_insert_binary_checksum(record->hash, HASH_MD5, header.md5);
		if (memcmp(nullhash, header.sha1, sizeof(header.sha1)) != 0)
			hash_data_insert_binary_checksum(record->hash, HASH_SHA1, header.sha1);

		/* found but needs a dump */
		if (hash_data_has_info(record->exphash, HASH_INFO_NO_DUMP))
			set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_GOOD_NEEDS_REDUMP);

		/* incorrect hash */
		else if (!hash_data_is_equal(record->exphash, record->hash, 0))
			set_status(record, AUDIT_STATUS_FOUND_INVALID, SUBSTATUS_FOUND_BAD_CHECKSUM);

		/* just plain good */
		else
			set_status(record, AUDIT_STATUS_GOOD, SUBSTATUS_GOOD);

		chd_close(source);
	}

	/* return TRUE if we found anything at all */
	return (source != NULL);
}


/*-------------------------------------------------
    rom_used_by_parent - determine if a given
    ROM is also used by the parent
-------------------------------------------------*/

static int rom_used_by_parent(const game_driver *gamedrv, const rom_entry *romentry, const game_driver **parent)
{
	const char *hash = ROM_GETHASHDATA(romentry);
	const game_driver *drv;

	/* iterate up the parent chain */
	for (drv = driver_get_clone(gamedrv); drv != NULL; drv = driver_get_clone(drv))
	{
		const rom_entry *region;
		const rom_entry *rom;

		/* see if the parent has the same ROM or not */
		for (region = rom_first_region(drv); region; region = rom_next_region(region))
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				if (hash_data_is_equal(ROM_GETHASHDATA(rom), hash, 0))
				{
					if (parent != NULL)
						*parent = drv;
					return TRUE;
				}
	}

	return FALSE;
}



/***************************************************************************
    CHD INTERFACES
***************************************************************************/

/*-------------------------------------------------
    audit_chd_open - interface for opening
    a hard disk image
-------------------------------------------------*/

static chd_interface_file *audit_chd_open(const char *filename, const char *mode)
{
	const game_driver *drv;

	/* attempt reading up the chain through the parents */
	for (drv = chd_gamedrv; drv != NULL; drv = driver_get_clone(drv))
	{
		mame_file_error filerr;
		mame_file *file;
		char *fname;

		fname = assemble_3_strings(drv->name, PATH_SEPARATOR, filename);
		filerr = mame_fopen(SEARCHPATH_IMAGE, fname, OPEN_FLAG_READ, &file);
		free(fname);

		if (filerr == FILERR_NONE)
			return (chd_interface_file *)file;
	}
	return NULL;
}


/*-------------------------------------------------
    audit_chd_close - interface for closing
    a hard disk image
-------------------------------------------------*/

static void audit_chd_close(chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}


/*-------------------------------------------------
    audit_chd_read - interface for reading
    from a hard disk image
-------------------------------------------------*/

static UINT32 audit_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    audit_chd_write - interface for writing
    to a hard disk image
-------------------------------------------------*/

static UINT32 audit_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	return 0;
}


/*-------------------------------------------------
    audit_chd_length - interface for getting
    the length a hard disk image
-------------------------------------------------*/

static UINT64 audit_chd_length(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}
