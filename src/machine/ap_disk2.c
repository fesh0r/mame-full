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
#include "drivers/apple2.h"

/* machine/apple2.c */
extern unsigned char *apple2_slot6;

static APPLE_DISKII_STRUCT a2_drives;

/* TODO: remove ugly hacked-in code */
static int track6[2];		/* current track #? */
static int disk6byte;		/* byte queued for writing? */
static int protected6[2];	/* is disk write-protected? */
static int runbyte6[2];		/* ??? */
static int sector6[2];		/* current sector #? */
static unsigned char sector_data[374];	/* converted sector data */
static int read_state;			/* 1 = read, 0 = write */

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
	osd_led_w(0,0);
	osd_led_w(2,0);

	/* TODO: remove following ugly hacked-in code */
	track6[0]     = track6[1]     = 84;
	protected6[0] = protected6[1] = 0;
	sector6[0]    = sector6[1]    = 0;
	runbyte6[0]   = runbyte6[1]   = 0;
	disk6byte     = 0;
	read_state    = 1;

	return;
}

/* For now, make disks read-only!!! */
static void WriteByte(int drive, int theByte)
{
	return;
}

static void ConvertSector(int drive)
{
	static int volume6 = 254;			/* volume # to use for disk (not stored in image)? */
	int checksum6;		/* checksum # to use for disk (not stored in image)? */
	int oldvalue6,exorvalue6;	/* ??? */
	void *f;
	int sec_pos;
	unsigned char data[258];
	int i;
	int position;
	int track;

	track = track6[drive]/2;

	if (errorlog)
		fprintf(errorlog,"Reading track %d sector %d\n",track,sector6[drive]);

	/* Default everything to sync byte 0xFF */
	memset(sector_data,0xFF,374);

	f = osd_fopen(Machine->gamedrv->name,floppy_name[drive],OSD_FILETYPE_IMAGE,0);
	if (f==NULL)
	{
		if (errorlog) fprintf(errorlog,"Couldn't open image.\n");
		return;
	}

	for (i=0;i<track;i++)
	{
		if (osd_fseek(f,256*16,SEEK_CUR)!=0)
		{
			if (errorlog) fprintf(errorlog,"Couldn't find track.\n");
			return;
		}
	}
	sec_pos = 256*r_skewing6[sector6[drive]];
	if (osd_fseek(f,sec_pos,SEEK_CUR)!=0)
	{
		if (errorlog) fprintf(errorlog,"Couldn't find sector.\n");
		return;
	}

	if (osd_fread(f,data,256)<256)
	{
		if (errorlog) fprintf(errorlog,"Couldn't read sector.\n");
		return;
	}
	data[256]=0;
	data[257]=0;

	osd_fclose(f);

	/* Setup header values */
	checksum6 = volume6 ^ track ^ sector6[drive];

	sector_data[6]=0xD5;
	sector_data[7]=0xAA;
	sector_data[8]=0x96;
	sector_data[9]=(volume6 >> 1) | 0xAA;
	sector_data[10]=volume6 | 0xAA;
	sector_data[11]=(track >> 1) | 0xAA;
	sector_data[12]=track | 0xAA;
	sector_data[13]=(sector6[drive] >> 1) | 0xAA;
	sector_data[14]=sector6[drive] | 0xAA;
	sector_data[15]=(checksum6 >> 1) | 0xAA;
	sector_data[16]=(checksum6) | 0xAA;
	sector_data[17]=0xDE;
	sector_data[18]=0xAA;
	sector_data[19]=0xEB;
	sector_data[25]=0xD5;
	sector_data[26]=0xAA;
	sector_data[27]=0xAD;
	sector_data[371]=0xDE;
	sector_data[372]=0xAA;
	sector_data[373]=0xEB;
	exorvalue6=0;

	for(i=0;i<342;i++)
	{
		if (i>=0x56)
		{
			/* 6 bit */
			oldvalue6=data[i - 0x56];
			oldvalue6=oldvalue6>>2;
			exorvalue6 ^= oldvalue6;
			sector_data[28+i] = translate6[exorvalue6 & 0x3F];
			exorvalue6 = oldvalue6;
		}
		else
		{
			/* 3 * 2 bit */
			oldvalue6 = 0;
			oldvalue6 |= (data[i] & 0x01) << 1;
			oldvalue6 |= (data[i] & 0x02) >> 1;
			oldvalue6 |= (data[i+0x56] & 0x01) << 3;
			oldvalue6 |= (data[i+0x56] & 0x02) << 1;
			oldvalue6 |= (data[i+0xAC] & 0x01) << 5;
			oldvalue6 |= (data[i+0xAC] & 0x02) << 3;
			exorvalue6 ^= oldvalue6;
			sector_data[28+i] = translate6[exorvalue6 & 0x3F];
			exorvalue6 = oldvalue6;
		}
	}

	sector_data[370] = translate6[exorvalue6 & 0x3F];
}

static int ReadByte(int drive)
{
	int value;

	/* no image name given for that drive ? */
	if (!floppy_name[drive])
		return 0xFF;

	if (a2_drives.motor==SWITCH_OFF)
		return 0x00;

	value = sector_data[runbyte6[drive]];
	runbyte6[drive]++;
	if (runbyte6[drive]>373)
	{
		sector6[drive] = (sector6[drive] + 1) & 0x0F;
		ConvertSector(drive);
		runbyte6[drive]=0;
	}

	return value;
}

/***************************************************************************
  apple2_c0xx_slot6_r
***************************************************************************/
int apple2_c0xx_slot6_r(int offset)
{
	int cur_drive;
	int phase;

	cur_drive = a2_drives.drive_num;

	switch (offset)
	{
		case 0x00:		/* PHASE0OFF */
		case 0x02:		/* PHASE1OFF */
		case 0x04:		/* PHASE2OFF */
		case 0x06:		/* PHASE3OFF */
			phase = (offset >> 1);
			a2_drives.drive[cur_drive].phase[phase] = SWITCH_OFF;		
			break;
		case 0x01:		/* PHASE0ON */
		case 0x03:		/* PHASE1ON */
		case 0x05:		/* PHASE2ON */
		case 0x07:		/* PHASE3ON */
			phase = (offset >> 1);
			a2_drives.drive[cur_drive].phase[phase] = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			phase -= (track6[cur_drive] % 4);
			if (phase < 0)				phase += 4;
			if (phase==1)				track6[cur_drive]++;
			if (phase==3)				track6[cur_drive]--;
			if (track6[cur_drive]<0)	track6[cur_drive]=0;
			ConvertSector(cur_drive);
			break;
		/* MOTOROFF */
		case 0x08:		
			a2_drives.motor = SWITCH_OFF;
			osd_led_w(0,0);		/* We use 0 and 2 for drives */
			osd_led_w(2,0);		/* We use 0 and 2 for drives */
			break;
		/* MOTORON */
		case 0x09:
			a2_drives.motor = SWITCH_ON;
			osd_led_w(cur_drive*2,1);		/* We use 0 and 2 for drives */
			ConvertSector(cur_drive);
			break;
		/* DRIVE1 */
		case 0x0A:
			a2_drives.drive_num = 0;
			ConvertSector(0);
			/* Only one drive can be "on" at a time */
			if (a2_drives.motor == SWITCH_ON)
			{
				osd_led_w(0,1);
				osd_led_w(2,0);
			}
			break;
		/* DRIVE2 */
		case 0x0B:		
			a2_drives.drive_num = 1;			
			ConvertSector(1);
			/* Only one drive can be "on" at a time */
			if (a2_drives.motor == SWITCH_ON)
			{
				osd_led_w(0,0);
				osd_led_w(2,1);
			}
			break;
		/* Q6L - set transistor Q6 low */
		case 0x0C:		
			a2_drives.drive[cur_drive].Q6 = SWITCH_OFF;
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
			a2_drives.drive[cur_drive].Q6 = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			if (protected6[cur_drive])
			{
				return 0x80;
			}
			break;
		/* Q7L - set transistor Q7 low */
		case 0x0E:
			a2_drives.drive[cur_drive].Q7 = SWITCH_OFF;
			/* TODO: remove following ugly hacked-in code */
			read_state = 1;
			if (protected6[cur_drive])
			{
				return 0x80;
			}
			break;
		/* Q7H - set transistor Q7 high */
		case 0x0F:
			a2_drives.drive[cur_drive].Q7 = SWITCH_ON;
			/* TODO: remove following ugly hacked-in code */
			read_state = 0;
			break;
	}

	return 0x00;
}

/***************************************************************************
  apple2_c0xx_slot6_w
***************************************************************************/
void apple2_c0xx_slot6_w(int offset, int data)
{
	switch (offset)
	{
		case 0x0D:	/* Store byte for writing */
			/* TODO: remove following ugly hacked-in code */
			disk6byte = data;
			break;
		default:	/* Otherwise, do same as slot6_r ? */
			apple2_c0xx_slot6_r(offset);
			break;
	}

	return;
}

/***************************************************************************
  apple2_slot6_r
***************************************************************************/
int apple2_slot6_r(int offset)
{
	return apple2_slot6[offset];
}

/***************************************************************************
  apple2_slot6_w
***************************************************************************/
void apple2_slot6_w(int offset, int data)
{
	return;
}

