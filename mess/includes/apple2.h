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

#define VAR_80STORE		0x000001
#define VAR_RAMRD		0x000002
#define VAR_RAMWRT		0x000004
#define VAR_INTCXROM	0x000008
#define VAR_ALTZP		0x000010
#define VAR_SLOTC3ROM	0x000020
#define VAR_80COL		0x000040
#define VAR_ALTCHARSET	0x000080
#define VAR_TEXT		0x000100
#define VAR_MIXED		0x000200
#define VAR_PAGE2		0x000400
#define VAR_HIRES		0x000800
#define VAR_AN0			0x001000
#define VAR_AN1			0x002000
#define VAR_AN2			0x004000
#define VAR_AN3			0x008000
#define VAR_LCRAM		0x010000
#define VAR_LCRAM2		0x020000
#define VAR_LCWRITE		0x040000

extern UINT32 a2;

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
extern DRIVER_INIT( apple2 );
extern MACHINE_INIT( apple2 );

extern int  apple2_id_rom(int id);

extern int	apple2e_load_rom(int id);
extern int	apple2ee_load_rom(int id);

extern void apple2_interrupt(void);

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
