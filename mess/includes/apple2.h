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




/* machine/apple2.c */

extern UINT8 *apple2_slot_rom;
extern UINT8 *apple2_slot1;
extern UINT8 *apple2_slot2;
extern UINT8 *apple2_slot3;
extern UINT8 *apple2_slot4;
extern UINT8 *apple2_slot5;
extern UINT8 *apple2_slot6;
extern UINT8 *apple2_slot7;

extern APPLE2_STRUCT a2;


extern MACHINE_INIT( apple2e );

extern int  apple2_id_rom(int id);

extern int	apple2e_load_rom(int id);
extern int	apple2ee_load_rom(int id);

extern void apple2_interrupt(void);
extern void apple2_slotrom_disable(int offset, int data);

extern READ_HANDLER ( apple2_c00x_r );
extern WRITE_HANDLER ( apple2_c00x_w );

extern READ_HANDLER ( apple2_c01x_r );
extern WRITE_HANDLER ( apple2_c01x_w );

extern READ_HANDLER ( apple2_c03x_r );
extern WRITE_HANDLER ( apple2_c03x_w );

extern READ_HANDLER ( apple2_c05x_r );
extern WRITE_HANDLER ( apple2_c05x_w );

extern READ_HANDLER ( apple2_c06x_r );

extern READ_HANDLER ( apple2_c07x_r );
extern WRITE_HANDLER ( apple2_c07x_w );

extern READ_HANDLER ( apple2_c08x_r );
extern WRITE_HANDLER ( apple2_c08x_w );

extern READ_HANDLER ( apple2_c0xx_slot1_r );
extern READ_HANDLER ( apple2_c0xx_slot2_r );
extern READ_HANDLER ( apple2_c0xx_slot3_r );
extern READ_HANDLER ( apple2_c0xx_slot4_r );
extern READ_HANDLER ( apple2_c0xx_slot5_r );
extern READ_HANDLER ( apple2_c0xx_slot7_r );
extern WRITE_HANDLER ( apple2_c0xx_slot1_w );
extern WRITE_HANDLER ( apple2_c0xx_slot2_w );
extern WRITE_HANDLER ( apple2_c0xx_slot3_w );
extern WRITE_HANDLER ( apple2_c0xx_slot4_w );
extern WRITE_HANDLER ( apple2_c0xx_slot5_w );
extern WRITE_HANDLER ( apple2_c0xx_slot7_w );
extern WRITE_HANDLER ( apple2_slot1_w );
extern WRITE_HANDLER ( apple2_slot2_w );
extern WRITE_HANDLER ( apple2_slot3_w );
extern WRITE_HANDLER ( apple2_slot4_w );
extern WRITE_HANDLER ( apple2_slot5_w );
extern WRITE_HANDLER ( apple2_slot7_w );
/*extern int  apple2_slot1_r(int offset);
extern int  apple2_slot2_r(int offset);
extern int  apple2_slot3_r(int offset);*/
extern READ_HANDLER ( apple2_slot4_r );
/*extern int  apple2_slot5_r(int offset);
extern int  apple2_slot6_r(int offset);*/
extern READ_HANDLER ( apple2_slot7_r );


/* machine/ap_disk2.c */
extern void apple2_slot6_init(void);
extern int apple2_floppy_load(int id, mame_file *fp, int open_mode);

extern READ_HANDLER ( apple2_c0xx_slot6_r );
extern WRITE_HANDLER ( apple2_c0xx_slot6_w );
extern WRITE_HANDLER ( apple2_slot6_w );


/* vidhrdw/apple2.c */
extern VIDEO_START( apple2e );
extern VIDEO_START( apple2c );
extern VIDEO_UPDATE( apple2 );
extern void apple2_video_touch(offs_t offset);

/* keyboard wrappers */
#define pressed_specialkey(key)	(input_port_8_r(0) & (key))
#define SPECIALKEY_CAPSLOCK		0x01
#define SPECIALKEY_SHIFT		0x06
#define SPECIALKEY_CONTROL		0x08
#define SPECIALKEY_BUTTON0		0x10	/* open apple */
#define SPECIALKEY_BUTTON1		0x20	/* closed apple */
#define SPECIALKEY_BUTTON2		0x40
#define SPECIALKEY_RESET		0x80

#endif
