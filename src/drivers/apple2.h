/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2H
#define APPLE2H 1

#define SWITCH_OFF 0
#define SWITCH_ON  1

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
	int ALZTP;
	int SLOTC3ROM;

	/* Annunciators */
	int AN0;
	int AN1;
	int AN2;
	int AN3;

} APPLE2_STRUCT;

typedef struct
{
	/* Drive stepper motor phase magnets */
	char phase[4];

	char Q6;
	char Q7;

} APPLE_DRIVE;

typedef struct
{
	/* Misc controller latches */
	char drive_num;
	char motor;

	APPLE_DRIVE drive[2];

} APPLE_DISKII_STRUCT;

#endif


