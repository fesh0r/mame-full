/***************************************************************************

  ap_disk2.c

  Machine file to handle emulation of the Apple Disk II controller.

  TODO:
    Allow # of drives and slot to be selectable.
	Redo the code to make it understandable.
	Allow disks to be writeable.
	Support more disk formats.
	Add a description of Apple Disk II Hardware?
	Make it faster?
	Add sound?
	Add proper microsecond timing?


	Note - only one driver light can be on at once; regardless of the motor
	state; if we support drive lights we must take this into consideration
***************************************************************************/

#include "driver.h"
#include "image.h"
#include "includes/apple2.h"
#include "devices/basicdsk.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define PROFILER_SLOT6	PROFILER_USER1

#define TOTAL_TRACKS		35 /* total number of tracks we support, can be 40 */
#define TOTAL_SECTORS		16
#define NIBBLE_SIZE			374
#define SECTOR_SIZE			256

#define Q6_MASK				0x10
#define Q7_MASK				0x20
#define TWEEN_TRACKS		0x40

#define TRSTATE_LOADED		0x01
#define TRSTATE_DIRTY		0x02

static int disk6byte;		/* byte queued for writing? */
static int read_state;		/* 1 = read, 0 = write */
static int a2_drives_num;

struct apple2_drive
{
	UINT8 state;			/* bits 0-3 are the phase; bits 4-5 is q6-7 */
	UINT8 transient_state;	/* state that just reflects the dirtiness of the track data */
	int position;
	UINT8 track_data[NIBBLE_SIZE * TOTAL_SECTORS];
};

static struct apple2_drive *apple2_drives;

static const unsigned char translate6[0x40] =
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

static const unsigned char r_skewing6[0x10] =
{
	0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
	0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F
};

/***************************************************************************
  apple2_slot6_init
***************************************************************************/
void apple2_slot6_init(void)
{
	apple2_drives = auto_malloc(sizeof(struct apple2_drive) * 2);
	if (!apple2_drives)
		return;
	memset(apple2_drives, 0, sizeof(struct apple2_drive) * 2);
}

DEVICE_LOAD(apple2_floppy)
{
	if (mame_fsize(file) != TOTAL_TRACKS*TOTAL_SECTORS*SECTOR_SIZE)
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image, file) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry(image, TOTAL_TRACKS, 1, TOTAL_SECTORS, SECTOR_SIZE, 0, 0, FALSE);
	floppy_drive_seek(image, -999);
	floppy_drive_seek(image, +TOTAL_TRACKS/2);
	apple2_drives[image_index_in_device(image)].state = TWEEN_TRACKS;
	apple2_drives[image_index_in_device(image)].transient_state = 0;
	return INIT_PASS;
}

/**************************************************************************/

static void load_current_track(mess_image *image, struct apple2_drive *disk)
{
	int track, sector;
	int volume = 254;
	char data[SECTOR_SIZE];
	int checksum;
	int xorvalue;
	int oldvalue;
	int i, pos;

	memset(data, 0, sizeof(data));
	memset(disk->track_data, 0xff, sizeof(disk->track_data));

	track = floppy_drive_get_current_track(image);

	for (sector = 0; sector < TOTAL_SECTORS; sector++)
	{
		floppy_drive_read_sector_data(image, 0, r_skewing6[sector], data, SECTOR_SIZE);

		pos = sector * NIBBLE_SIZE;

		/* Setup header values */
		checksum = volume ^ track ^ sector;

		disk->track_data[pos+ 7]     = 0xD5;
		disk->track_data[pos+ 8]     = 0xAA;
		disk->track_data[pos+ 9]     = 0x96;
		disk->track_data[pos+10]     = (volume >> 1) | 0xAA;
		disk->track_data[pos+11]     = volume | 0xAA;
		disk->track_data[pos+12]     = (track >> 1) | 0xAA;
		disk->track_data[pos+13]     = track | 0xAA;
		disk->track_data[pos+14]     = (sector >> 1) | 0xAA;
		disk->track_data[pos+15]     = sector | 0xAA;
		disk->track_data[pos+16]     = (checksum >> 1) | 0xAA;
		disk->track_data[pos+17]     = (checksum) | 0xAA;
		disk->track_data[pos+18]     = 0xDE;
		disk->track_data[pos+19]     = 0xAA;
		disk->track_data[pos+20]     = 0xEB;
		disk->track_data[pos+25]     = 0xD5;
		disk->track_data[pos+26]     = 0xAA;
		disk->track_data[pos+27]     = 0xAD;
		disk->track_data[pos+27+344] = 0xDE;
		disk->track_data[pos+27+345] = 0xAA;
		disk->track_data[pos+27+346] = 0xEB;
		xorvalue = 0;

		for(i = 0; i < 342; i++)
		{
			if (i >= 0x56)
			{
				/* 6 bit */
				oldvalue = data[i - 0x56];
				oldvalue = oldvalue >> 2;
				xorvalue ^= oldvalue;
				disk->track_data[pos+28+i] = translate6[xorvalue & 0x3F];
				xorvalue = oldvalue;
			}
			else
			{
				/* 3 * 2 bit */
				oldvalue = 0;
				oldvalue |= (data[i] & 0x01) << 1;
				oldvalue |= (data[i] & 0x02) >> 1;
				oldvalue |= (data[i+0x56] & 0x01) << 3;
				oldvalue |= (data[i+0x56] & 0x02) << 1;
				oldvalue |= (data[i+0xAC] & 0x01) << 5;
				oldvalue |= (data[i+0xAC] & 0x02) << 3;
				xorvalue ^= oldvalue;
				disk->track_data[pos+28+i] = translate6[xorvalue & 0x3F];
				xorvalue = oldvalue;
			}
		}

		disk->track_data[pos+27+343] = translate6[xorvalue & 0x3F];
	}
	disk->transient_state |= TRSTATE_LOADED;
}

/**************************************************************************/

/* For now, make disks read-only!!! */
static void write_byte(mess_image *img, struct apple2_drive *disk, int theByte)
{
}

static int read_byte(mess_image *img, struct apple2_drive *disk)
{
	int value;

	/* no image initialized for that drive ? */
	if (!image_exists(img))
		return 0xFF;

	/* load track if need be */
	if ((disk->transient_state & TRSTATE_LOADED) == 0)
		load_current_track(img, disk);

	value = disk->track_data[disk->position];

	disk->position++;
	disk->position %= (sizeof(disk->track_data) / sizeof(disk->track_data[0]));
	return value;
}

static void seek_disk(mess_image *img, struct apple2_drive *disk, signed int step)
{
	int track;
	int pseudo_track;
	
	track = floppy_drive_get_current_track(img);
	pseudo_track = (track * 2) + (disk->state & TWEEN_TRACKS ? 1 : 0);
	
	pseudo_track += step;
	if (pseudo_track < 0)
		pseudo_track = 0;
	else if (pseudo_track/2 >= TOTAL_TRACKS)
		pseudo_track = TOTAL_TRACKS*2-1;

	if (pseudo_track/2 != track)
	{
		floppy_drive_seek(img, pseudo_track/2 - floppy_drive_get_current_track(img));
		disk->transient_state &= ~TRSTATE_LOADED;
	}

	if (pseudo_track & 1)
		disk->state |= TWEEN_TRACKS;
	else
		disk->state &= ~TWEEN_TRACKS;
}

/***************************************************************************
  apple2_c0xx_slot6_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot6_r )
{
	struct apple2_drive *cur_disk;
	mess_image *cur_image;
	unsigned int phase;
	data8_t result = 0x00;

	profiler_mark(PROFILER_SLOT6);

	cur_image = image_from_devtype_and_index(IO_FLOPPY, a2_drives_num);
	cur_disk = &apple2_drives[a2_drives_num];

	switch (offset) {
	case 0x00:		/* PHASE0OFF */
	case 0x01:		/* PHASE0ON */
	case 0x02:		/* PHASE1OFF */
	case 0x03:		/* PHASE1ON */
	case 0x04:		/* PHASE2OFF */
	case 0x05:		/* PHASE2ON */
	case 0x06:		/* PHASE3OFF */
	case 0x07:		/* PHASE3ON */
		phase = (offset >> 1);
		if ((offset & 1) == 0)
		{
			/* phase OFF */
			cur_disk->state &= ~(1 << phase);
		}
		else
		{
			/* phase ON */
			cur_disk->state |= (1 << phase);

			phase -= floppy_drive_get_current_track(cur_image) * 2;
			if (cur_disk->state & TWEEN_TRACKS)
				phase--;
			phase %= 4;

			switch(phase) {
			case 1:
				seek_disk(cur_image, cur_disk, +1);
				break;
			case 3:
				seek_disk(cur_image, cur_disk, -1);
				break;
			}
		}
		break;

	case 0x08:		/* MOTOROFF */
	case 0x09:		/* MOTORON */
		floppy_drive_set_motor_state(cur_image, (offset & 1));
		break;
		
	case 0x0A:		/* DRIVE1 */
	case 0x0B:		/* DRIVE2 */
		a2_drives_num = (offset & 1);
		break;
	
	case 0x0C:		/* Q6L - set transistor Q6 low */
		cur_disk->state &= ~Q6_MASK;
		if (read_state)
			result = read_byte(cur_image, cur_disk);
		else
			write_byte(cur_image, cur_disk, disk6byte);
		break;
	
	case 0x0D:		/* Q6H - set transistor Q6 high */
		cur_disk->state |= Q6_MASK;
		if (floppy_drive_get_flag_state(cur_image, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
			result = 0x80;
		break;

	case 0x0E:		/* Q7L - set transistor Q7 low */
		cur_disk->state &= ~Q7_MASK;
		read_state = 1;
		if (floppy_drive_get_flag_state(cur_image, FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
			result = 0x80;
		break;

	case 0x0F:		/* Q7H - set transistor Q7 high */
		cur_disk->state |= Q7_MASK;
		read_state = 0;
		break;
	}

	profiler_mark(PROFILER_END);
	return result;
}

/***************************************************************************
  apple2_c0xx_slot6_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot6_w )
{
	profiler_mark(PROFILER_SLOT6);
	switch (offset)
	{
		case 0x0D:	/* Store byte for writing */
			/* TODO: remove following ugly hacked-in code */
			disk6byte = data;
			break;
		default:	/* Otherwise, do same as slot6_r ? */
			LOG(("slot6_w\n"));
			apple2_c0xx_slot6_r(offset);
			break;
	}

	profiler_mark(PROFILER_END);
}

/***************************************************************************
  apple2_slot6_w
***************************************************************************/
WRITE_HANDLER (  apple2_slot6_w )
{
	return;
}

