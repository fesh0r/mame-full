/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2H
#define APPLE2H 1

#define SWITCH_OFF 0
#define SWITCH_ON  1

#define A2_DISK_DO	0 /* DOS order */
#define A2_DISK_PO  1 /* ProDOS/Pascal order */
#define A2_DISK_NIB 2 /* Raw nibble format */

typedef struct
{
	/* IOU Variables */
	int PAGE2;
	int HIRES;
	int MIXED;
	int TEXT;
	int COL80;
	int ALTCHARSET;

	/* MMU Variables */
	int STORE80;
	int RAMRD;
	int RAMWRT;
	int INTCXROM;
	int ALTZP;
	int SLOTC3ROM;
	int LC_RAM;
	int LC_RAM2;
	int LC_WRITE;

	/* Annunciators */
	int AN0;
	int AN1;
	int AN2;
	int AN3;

} APPLE2_STRUCT;

extern APPLE2_STRUCT a2;

typedef struct
{
	/* Drive stepper motor phase magnets */
	char phase[4];

	char Q6;
	char Q7;
	
	unsigned char *data;
	
	int track;
	int sector; /* not needed? */
	int volume;
	int bytepos;
	int trackpos;
	int write_protect;
	int image_type;

	/* Misc controller latches */
	char drive_num;
	char motor;

} APPLE_DISKII_STRUCT;

#endif


