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



***************************************************************************/

#include "driver.h"
#include "includes/apple2.h"
#include "image.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define TOTAL_TRACKS		35 /* total number of tracks we support, can be 40 */
#define NIBBLE_SIZE			374

static APPLE_DISKII_STRUCT a2_drives[2];

/* TODO: remove ugly hacked-in code */
static int track6[2];		/* current track #? */
static int disk6byte;		/* byte queued for writing? */
static int protected6[2];	/* is disk write-protected? */
static int runbyte6[2];		/* ??? */
static int sector6[2];		/* current sector #? */

static int read_state;			/* 1 = read, 0 = write */
static int a2_drives_num;

static unsigned char translate6[0x40] =
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

static unsigned char r_skewing6[0x10] =
{
	0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
	0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F
};

/***************************************************************************
  apple2_slot6_init
***************************************************************************/
void apple2_slot6_init(void)
{
	/* Set the two drive LEDs to OFF */
	set_led_status(0,0);
	set_led_status(2,0);

	/* TODO: remove following ugly hacked-in code */
	track6[0]     = track6[1]     = TOTAL_TRACKS; /* go to the middle of the disk */
	protected6[0] = protected6[1] = 0;
	sector6[0]    = sector6[1]    = 0;
	runbyte6[0]   = runbyte6[1]   = 0;
	disk6byte     = 0;
	read_state    = 1;
}

int apple2_floppy_init(int id, void *f, int open_mode)
{
	int t, s;
	int pos;
	int volume;
	int i;

	if (f == NULL)
		return INIT_PASS;

    a2_drives[id].data = image_malloc(IO_FLOPPY, id, NIBBLE_SIZE*16*TOTAL_TRACKS);
	if (!a2_drives[id].data)
		return INIT_FAIL;

	/* Default everything to sync byte 0xFF */
	memset(a2_drives[id].data, 0xff, NIBBLE_SIZE*16*TOTAL_TRACKS);

	/* TODO: support .nib and .po images */
	a2_drives[id].image_type = A2_DISK_DO;
	a2_drives[id].write_protect = 1;
	a2_drives[id].track = TOTAL_TRACKS; /* middle of the disk */
	a2_drives[id].volume = volume = 254;
	a2_drives[id].bytepos = 0;
	a2_drives[id].trackpos = 0;

	for (t = 0; t < TOTAL_TRACKS; t ++)
	{
		if (osd_fseek(f,256*16*t,SEEK_CUR)!=0)
		{
			LOG(("Couldn't find track %d.\n", t));
			return INIT_FAIL;
		}

		for (s = 0; s < 16; s ++)
		{
			int sec_pos;
			unsigned char data[256];
			int checksum;
			int xorvalue;
			int oldvalue;

			sec_pos = 256*r_skewing6[s] + t*256*16;
			if (osd_fseek(f,sec_pos,SEEK_SET)!=0)
			{
				LOG(("Couldn't find sector %d.\n", s));
				return INIT_FAIL;
			}

			if (osd_fread(f,data,256)<256)
			{
				LOG(("Couldn't read track %d sector %d (pos: %d).\n", t, s, sec_pos));
				return INIT_FAIL;
			}


			pos = NIBBLE_SIZE*s + (t*NIBBLE_SIZE*16);

			/* Setup header values */
			checksum = volume ^ t ^ s;

			a2_drives[id].data[pos+7]=0xD5;
			a2_drives[id].data[pos+8]=0xAA;
			a2_drives[id].data[pos+9]=0x96;
			a2_drives[id].data[pos+10]=(volume >> 1) | 0xAA;
			a2_drives[id].data[pos+11]= volume | 0xAA;
			a2_drives[id].data[pos+12]=(t >> 1) | 0xAA;
			a2_drives[id].data[pos+13]= t | 0xAA;
			a2_drives[id].data[pos+14]=(s >> 1) | 0xAA;
			a2_drives[id].data[pos+15]= s | 0xAA;
			a2_drives[id].data[pos+16]=(checksum >> 1) | 0xAA;
			a2_drives[id].data[pos+17]=(checksum) | 0xAA;
			a2_drives[id].data[pos+18]=0xDE;
			a2_drives[id].data[pos+19]=0xAA;
			a2_drives[id].data[pos+20]=0xEB;
			a2_drives[id].data[pos+25]=0xD5;
			a2_drives[id].data[pos+26]=0xAA;
			a2_drives[id].data[pos+27]=0xAD;
			a2_drives[id].data[pos+27+344]=0xDE;
			a2_drives[id].data[pos+27+345]=0xAA;
			a2_drives[id].data[pos+27+346]=0xEB;
			xorvalue = 0;

			for(i=0;i<342;i++)
			{
				if (i >= 0x56)
				{
					/* 6 bit */
					oldvalue=data[i - 0x56];
					oldvalue=oldvalue>>2;
					xorvalue ^= oldvalue;
					a2_drives[id].data[pos+28+i] = translate6[xorvalue & 0x3F];
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
					a2_drives[id].data[pos+28+i] = translate6[xorvalue & 0x3F];
					xorvalue = oldvalue;
				}
			}

			a2_drives[id].data[pos+27+343] = translate6[xorvalue & 0x3F];
		}
	}

#if 0
	{
		FILE *dump;

		dump = fopen ("a2_disk.dmp", "w");

		fwrite (a2_drives[id].data, 1, NIBBLE_SIZE*16*35, dump);
		fclose (dump);
	}
#endif

	return INIT_PASS;
}


/* For now, make disks read-only!!! */
static void WriteByte(int drive, int theByte)
{
	return;
}

static int ReadByte(int drive)
{
	int value;

	/* no image initialized for that drive ? */
	if (!image_exists(IO_FLOPPY, drive))
		return 0xFF;

	/* Our drives are always turned on baby, yeah!

	   The reason is that a real drive takes around a second to turn off, so
	   consecutive reads are done by DOS with enough haste that the drive is
	   never really off between them. For a perfect emulation, we should turn it off
	   via a timer around a second after the command to turn it off is sent. */
#if 0
	if (a2_drives[drive].motor == SWITCH_OFF)
	{
		return 0x00;
	}
#endif

	value = a2_drives[drive].data[a2_drives[drive].trackpos + a2_drives[drive].bytepos];

	a2_drives[drive].bytepos ++;
	if (a2_drives[drive].bytepos >= NIBBLE_SIZE*16)
	{
		a2_drives[drive].bytepos = 0;
	}

//	LOG(("pos: %d (track %d sector %d)\n", a2_drives[drive].bytepos, a2_drives[drive].track / 2, a2_drives[drive].bytepos / NIBBLE_SIZE));
	return value;
}

/***************************************************************************
  apple2_c0xx_slot6_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot6_r )
{
	int cur_drive;
	int phase;

	cur_drive = a2_drives_num;

	switch (offset)
	{
		case 0x00:		/* PHASE0OFF */
		case 0x02:		/* PHASE1OFF */
		case 0x04:		/* PHASE2OFF */
		case 0x06:		/* PHASE3OFF */
			phase = (offset >> 1);
			a2_drives[cur_drive].phase[phase] = SWITCH_OFF;
			break;
		case 0x01:		/* PHASE0ON */
		case 0x03:		/* PHASE1ON */
		case 0x05:		/* PHASE2ON */
		case 0x07:		/* PHASE3ON */
			phase = (offset >> 1);
			a2_drives[cur_drive].phase[phase] = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			phase -= (a2_drives[cur_drive].track % 4);
			if (phase < 0)				        phase += 4;
			if (phase==1)				        a2_drives[cur_drive].track++;
			if (phase==3)				        a2_drives[cur_drive].track--;
			if (a2_drives[cur_drive].track<0)	a2_drives[cur_drive].track=0;
			a2_drives[cur_drive].trackpos = (a2_drives[cur_drive].track/2) * NIBBLE_SIZE*16;
			LOG(("new track: %02x\n", a2_drives[cur_drive].track / 2));
			break;
		/* MOTOROFF */
		case 0x08:
			a2_drives[cur_drive].motor = SWITCH_OFF;
			set_led_status(0,0);	 /* We use 0 and 2 for drives */
			set_led_status(2,0);	 /* We use 0 and 2 for drives */
			break;
		/* MOTORON */
		case 0x09:
			a2_drives[cur_drive].motor = SWITCH_ON;
			set_led_status(cur_drive*2,1);		 /* We use 0 and 2 for drives */
			break;
		/* DRIVE1 */
		case 0x0A:
			a2_drives_num = 0;
			/* Only one drive can be "on" at a time */
			if (a2_drives[cur_drive].motor == SWITCH_ON)
			{
				set_led_status(0,1);
				set_led_status(2,0);
			}
			break;
		/* DRIVE2 */
		case 0x0B:
			a2_drives_num = 1;
			/* Only one drive can be "on" at a time */
			if (a2_drives[cur_drive].motor == SWITCH_ON)
			{
				set_led_status(0,0);
				set_led_status(2,1);
			}
			break;
		/* Q6L - set transistor Q6 low */
		case 0x0C:
			a2_drives[cur_drive].Q6 = SWITCH_OFF;
			/* TODO: remove following ugly hacked-in code */
			if (read_state)
			{
				return ReadByte(cur_drive);
			}
			else
			{
				WriteByte(cur_drive,disk6byte);
			}
			break;
		/* Q6H - set transistor Q6 high */
		case 0x0D:
			a2_drives[cur_drive].Q6 = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			if (a2_drives[cur_drive].write_protect)
			{
				return 0x80;
			}
			break;
		/* Q7L - set transistor Q7 low */
		case 0x0E:
			a2_drives[cur_drive].Q7 = SWITCH_OFF;
			/* TODO: remove following ugly hacked-in code */
			read_state = 1;
			if (a2_drives[cur_drive].write_protect)
			{
				return 0x80;
			}
			break;
		/* Q7H - set transistor Q7 high */
		case 0x0F:
			a2_drives[cur_drive].Q7 = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			read_state = 0;
			break;
	}

	return 0x00;
}

/***************************************************************************
  apple2_c0xx_slot6_w
***************************************************************************/
WRITE_HANDLER (  apple2_c0xx_slot6_w )
{
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

	return;
}

/***************************************************************************
  apple2_slot6_w
***************************************************************************/
WRITE_HANDLER (  apple2_slot6_w )
{
	return;
}

