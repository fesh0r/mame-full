/*********************************************************************

	formats/ap2_disk.c

	Utility code for dealing with Apple II disk images

*********************************************************************/

#include "formats/ap2_disk.h"

static const UINT8 translate6[0x40] =
{
	0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
	0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
	0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
	0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
	0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
	0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
	0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};



/*-------------------------------------------------
	apple2_choose_image_type - given an extension
	figures out what image type the file is
-------------------------------------------------*/

int apple2_choose_image_type(const char *filetype, UINT64 size)
{
	int image_type = APPLE2_IMAGE_UNKNOWN;
	UINT64 expected_size = 0;
	
	switch(filetype[0]) {
	case 'p':
	case 'P':
		if (!strcmpi(filetype, "po"))
		{
			image_type = APPLE2_IMAGE_PO;
			expected_size = APPLE2_TRACK_COUNT * APPLE2_SECTOR_COUNT * APPLE2_SECTOR_SIZE;
		}
		break;

	case 'b':
	case 'B':
	case 'd':
	case 'D':
		if (!strcmpi(filetype, "do") || !strcmpi(filetype, "dsk") || !strcmpi(filetype, "bin"))
		{
			image_type = APPLE2_IMAGE_DO;
			expected_size = APPLE2_TRACK_COUNT * APPLE2_SECTOR_COUNT * APPLE2_SECTOR_SIZE;
		}
		break;
	}

	if (size != expected_size)
		image_type = APPLE2_IMAGE_UNKNOWN;
	return image_type;
}



/*-------------------------------------------------
	apple2_skew_sector - skews a logical sector to
	a physical sector based on the image type
-------------------------------------------------*/

int apple2_skew_sector(int sector, int image_type)
{
	static const unsigned char skewing[2][APPLE2_SECTOR_COUNT] =
	{
		{
			/* DOS order (*.do) */
			0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
			0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F
		},
		{
			/* ProDOS order (*.po) */
			0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
			0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F
		}
	};

	image_type--;

	assert(sector >= 0);
	assert(sector < APPLE2_SECTOR_COUNT);
	assert(image_type >= 0);
	assert(image_type < sizeof(skewing) / sizeof(skewing[0]));
	return skewing[image_type][sector];
}



/*-------------------------------------------------
	apple2_disk_encode_nib - given logical sector
	data, changes it to a nibble
-------------------------------------------------*/

void apple2_disk_encode_nib(UINT8 *nibble, const UINT8 *data, int volume, int track, int sector)
{
	int checksum;
	int xorvalue;
	int oldvalue;
	int i;

	/* Setup header values */
	checksum = volume ^ track ^ sector;

	nibble[ 7]     = 0xD5;
	nibble[ 8]     = 0xAA;
	nibble[ 9]     = 0x96;
	nibble[10]     = (volume >> 1) | 0xAA;
	nibble[11]     = volume | 0xAA;
	nibble[12]     = (track >> 1) | 0xAA;
	nibble[13]     = track | 0xAA;
	nibble[14]     = (sector >> 1) | 0xAA;
	nibble[15]     = sector | 0xAA;
	nibble[16]     = (checksum >> 1) | 0xAA;
	nibble[17]     = (checksum) | 0xAA;
	nibble[18]     = 0xDE;
	nibble[19]     = 0xAA;
	nibble[20]     = 0xEB;
	nibble[25]     = 0xD5;
	nibble[26]     = 0xAA;
	nibble[27]     = 0xAD;
	nibble[27+344] = 0xDE;
	nibble[27+345] = 0xAA;
	nibble[27+346] = 0xEB;
	xorvalue = 0;

	for (i = 0; i < 342; i++)
	{
		if (i >= 0x56)
		{
			/* 6 bit */
			oldvalue = data[i - 0x56];
			oldvalue = oldvalue >> 2;
		}
		else
		{
			/* 3 * 2 bit */
			oldvalue = 0;
			oldvalue |= (data[i + 0x00] & 0x01) << 1;
			oldvalue |= (data[i + 0x00] & 0x02) >> 1;
			oldvalue |= (data[i + 0x56] & 0x01) << 3;
			oldvalue |= (data[i + 0x56] & 0x02) << 1;
			oldvalue |= (data[i + 0xAC] & 0x01) << 5;
			oldvalue |= (data[i + 0xAC] & 0x02) << 3;
		}
		xorvalue ^= oldvalue;
		nibble[28+i] = translate6[xorvalue & 0x3F];
		xorvalue = oldvalue;
	}

	nibble[27+343] = translate6[xorvalue & 0x3F];
}



/*-------------------------------------------------
	decode_nibbyte - decodes a nibblized byte
-------------------------------------------------*/

static int decode_nibbyte(UINT8 *nibint, const UINT8 *nibdata)
{
	if ((nibdata[0] & 0xAA) != 0xAA)
		return 1;
	if ((nibdata[1] & 0xAA) != 0xAA)
		return 1;

	*nibint =  (nibdata[0] & ~0xAA) << 1;
	*nibint |= (nibdata[1] & ~0xAA) << 0;
	return 0;
}



/*-------------------------------------------------
	get_untranslate6_map - create reverse of
	translate6
-------------------------------------------------*/

static const UINT8 *get_untranslate6_map(void)
{
	static UINT8 map[256];
	static int map_inited = 0;
	UINT8 i;

	if (!map_inited)
	{
		memset(map, 0xff, sizeof(map));
		for (i = 0; i < sizeof(translate6) / sizeof(translate6[0]); i++)
			map[translate6[i]] = i;
		map_inited = 1;
	}
	return map;
}



/*-------------------------------------------------
	apple2_disk_decode_nib - given nibble data,
	changes it to logical sector data
-------------------------------------------------*/

int apple2_disk_decode_nib(UINT8 *data, const UINT8 *nibble, int *volume, int *track, int *sector)
{
	UINT8 read_volume;
	UINT8 read_track;
	UINT8 read_sector;
	UINT8 read_checksum;
	int i;
	UINT8 b, xorvalue, newvalue;

	const UINT8 *untranslate6 = get_untranslate6_map();

	/* pick apart the volume/track/sector info and checksum */
	if (decode_nibbyte(&read_volume, &nibble[10]))
		return 1;
	if (decode_nibbyte(&read_track, &nibble[12]))
		return 1;
	if (decode_nibbyte(&read_sector, &nibble[14]))
		return 1;
	if (decode_nibbyte(&read_checksum, &nibble[16]))
		return 1;
	if (read_checksum != (read_volume ^ read_track ^ read_sector))
		return 1;

	/* decode the nibble core */
	xorvalue = 0;
	for (i = 0; i < 342; i++)
	{
		b = untranslate6[nibble[i+28]];
		if (b == 0xff)
			return 1;
		newvalue = b ^ xorvalue;

		if (i >= 0x56)
		{
			/* 6 bit */
			data[i - 0x56] |= (newvalue << 2);
		}
		else
		{
			/* 3 * 2 bit */
			data[i + 0x00] = ((newvalue >> 1) & 0x01) | ((newvalue << 1) & 0x02);
			data[i + 0x56] = ((newvalue >> 3) & 0x01) | ((newvalue >> 1) & 0x02);
			data[i + 0xAC] = ((newvalue >> 5) & 0x01) | ((newvalue >> 3) & 0x02);
		}
		xorvalue = newvalue;
	}

	/* success; write out values if pointers not null */
	if (volume)
		*volume = read_volume;
	if (track)
		*track = read_track;
	if (sector)
		*sector = read_sector;
	return 0;
}



/*-------------------------------------------------
	apple2_disk_nibble_test - tests the nibble
	encoding and decoding functions
-------------------------------------------------*/

void apple2_disk_nibble_test(void)
{
	UINT8 before_data[APPLE2_SECTOR_SIZE];
	UINT8 after_data[APPLE2_SECTOR_SIZE];
	UINT8 nibble_data[APPLE2_NIBBLE_SIZE];
	int volume = 254, track, sector;
	int after_volume, after_track, after_sector;
	int rc;
	int i, j;

	for (track = 0; track < APPLE2_TRACK_COUNT; track++)
	{
		for (sector = 0; sector < APPLE2_SECTOR_COUNT; sector++)
		{
			for (i = 0; i < APPLE2_SECTOR_SIZE; i++)
				before_data[i] = rand();

			apple2_disk_encode_nib(nibble_data, before_data, volume, track, sector);
			rc = apple2_disk_decode_nib(after_data, nibble_data, &after_volume, &after_track, &after_sector);

			assert(rc == 0);
			for (j = 0; j < APPLE2_SECTOR_SIZE; j++)
				assert(before_data[j] == after_data[j]);
			assert(volume == after_volume);
			assert(track == after_track);
			assert(sector == after_sector);
		}
	}
}



