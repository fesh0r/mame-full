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
#include "devices/flopdrv.h"
#include "devices/mflopimg.h"
#include "includes/apple2.h"
#include "formats/ap2_dsk.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define PROFILER_SLOT6	PROFILER_USER1

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
	UINT8 transient_state;
	int position;
	int spin_count; 		/* simulate drive spin to fool RWTS test at $BD34 */
	UINT8 track_data[APPLE2_NIBBLE_SIZE * APPLE2_SECTOR_COUNT];
};

static struct apple2_drive *apple2_drives;

static void apple2_floppy_unload(mess_image *image);


/***************************************************************************
  apple2_slot6_init
***************************************************************************/

void apple2_slot6_init(void)
{
	int floppy_count, i;
	mess_image *image;

	floppy_count = device_count(IO_FLOPPY);

	apple2_drives = auto_malloc(sizeof(struct apple2_drive) * floppy_count);
	if (!apple2_drives)
		return;
	memset(apple2_drives, 0, sizeof(struct apple2_drive) * floppy_count);

	for (i = 0; i < floppy_count; i++)
	{
		image = image_from_devtag_and_index(APDISK_DEVTAG, i);
		floppy_install_unload_proc(image, apple2_floppy_unload);

		/* seek middle sector */
		if (image_exists(image))
		{
			floppy_drive_seek(image, -999);
			floppy_drive_seek(image, +35/2);
		}
	}
}



/**************************************************************************/

static void load_current_track(mess_image *image, struct apple2_drive *disk)
{
	int len = sizeof(disk->track_data);
	floppy_drive_read_track_data_info_buffer(image, 0, disk->track_data, &len);
	disk->transient_state |= TRSTATE_LOADED;
}



static void save_current_track(mess_image *image, struct apple2_drive *disk)
{
	int len = sizeof(disk->track_data);

	if (disk->transient_state & TRSTATE_DIRTY)
	{
		floppy_drive_write_track_data_info_buffer(image, 0, disk->track_data, &len);
		disk->transient_state &= ~TRSTATE_DIRTY;
	}
}



static void apple2_floppy_unload(mess_image *image)
{
	struct apple2_drive *cur_disk;
	cur_disk = &apple2_drives[image_index_in_device(image)];
	save_current_track(image, cur_disk);
	cur_disk->transient_state &= ~TRSTATE_LOADED;
}



/* reads/writes a byte; write_value is -1 for read only */
static UINT8 process_byte(mess_image *img, struct apple2_drive *disk, int write_value)
{
	UINT8 read_value;

	/* no image initialized for that drive ? */
	if (!image_exists(img))
		return 0xFF;

	/* load track if need be */
	if ((disk->transient_state & TRSTATE_LOADED) == 0)
		load_current_track(img, disk);

	/* perform the read */
	read_value = disk->track_data[disk->position];

	/* perform the write, if applicable */
	if (write_value >= 0)
	{
		disk->track_data[disk->position] = write_value;
		disk->transient_state |= TRSTATE_DIRTY;
	}

	disk->position++;
	disk->position %= (sizeof(disk->track_data) / sizeof(disk->track_data[0]));

	/* when writing; save the current track after every full sector write */
	if ((write_value >= 0) && ((disk->position % APPLE2_NIBBLE_SIZE) == 0))
		save_current_track(img, disk);

	return read_value;
}



UINT8 apple2_slot6_readbyte(mess_image *image)
{
	struct apple2_drive *cur_disk;
	cur_disk = &apple2_drives[image_index_in_device(image)];
	return process_byte(image, cur_disk, -1);
}



void apple2_slot6_writebyte(mess_image *image, UINT8 byte)
{
	struct apple2_drive *cur_disk;
	cur_disk = &apple2_drives[image_index_in_device(image)];
	process_byte(image, cur_disk, byte);
}



static void seek_disk(mess_image *img, struct apple2_drive *disk, signed int step)
{
	int track;
	int pseudo_track;

	save_current_track(img, disk);
	
	track = floppy_drive_get_current_track(img);
	pseudo_track = (track * 2) + (disk->state & TWEEN_TRACKS ? 1 : 0);
	
	pseudo_track += step;
	if (pseudo_track < 0)
		pseudo_track = 0;
	else if (pseudo_track/2 >= APPLE2_TRACK_COUNT)
		pseudo_track = APPLE2_TRACK_COUNT*2-1;

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



void apple2_slot6_set_lines(mess_image *cur_image, UINT8 new_state)
{
	int image_index;
	struct apple2_drive *cur_disk;
	unsigned int phase;
	UINT8 old_state;

	image_index = image_index_in_device(cur_image);
	cur_disk = &apple2_drives[image_index];

	old_state = cur_disk->state;
	cur_disk->state = new_state;

	if (new_state > old_state)
	{
		phase = 0;
		switch(old_state ^ new_state)
		{
			case 1:	phase = 0; break;
			case 2:	phase = 1; break;
			case 4:	phase = 2; break;
			case 8:	phase = 3; break;
		}

		phase -= floppy_drive_get_current_track(cur_image) * 2;
		if (cur_disk->state & TWEEN_TRACKS)
			phase--;
		phase %= 4;

		switch(phase)
		{
			case 1:
				seek_disk(cur_image, cur_disk, +1);
				break;
			case 3:
				seek_disk(cur_image, cur_disk, -1);
				break;
		}
	}
}



/***************************************************************************
  apple2_c0xx_slot6_r
***************************************************************************/

READ8_HANDLER ( apple2_c0xx_slot6_r )
{
	struct apple2_drive *cur_disk;
	mess_image *cur_image;
	unsigned int phase;
	data8_t result = 0x00;
	UINT8 new_state;

	profiler_mark(PROFILER_SLOT6);

	cur_image = image_from_devtag_and_index(APDISK_DEVTAG, a2_drives_num);
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
			new_state = cur_disk->state & ~(1 << phase);
		}
		else
		{
			/* phase ON */
			new_state = cur_disk->state | (1 << phase);
		}

		apple2_slot6_set_lines(cur_image, new_state);
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
		{
			if ((++cur_disk->spin_count & 0x0F) != 0)
				result = process_byte(cur_image, cur_disk, -1);
		}
		else
		{
			process_byte(cur_image, cur_disk, disk6byte);
		}
		break;
	
	case 0x0D:		/* Q6H - set transistor Q6 high */
		cur_disk->state |= Q6_MASK;
		if (!image_is_writable(cur_image))
			result = 0x80;
		break;

	case 0x0E:		/* Q7L - set transistor Q7 low */
		cur_disk->state &= ~Q7_MASK;
		read_state = 1;
		if (!image_is_writable(cur_image))
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

WRITE8_HANDLER ( apple2_c0xx_slot6_w )
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

