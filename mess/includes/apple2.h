/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2H
#define APPLE2H 1

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
#define VAR_ROMSWITCH	0x080000

#define VAR_DHIRES		VAR_AN3

extern UINT32 a2;

/* machine/apple2.c */
DRIVER_INIT( apple2 );
MACHINE_INIT( apple2 );

int  apple2_id_rom(int id);

int	apple2e_load_rom(int id);
int	apple2ee_load_rom(int id);

void apple2_interrupt(void);

READ_HANDLER ( apple2_c00x_r );
WRITE_HANDLER ( apple2_c00x_w );

READ_HANDLER ( apple2_c01x_r );
WRITE_HANDLER ( apple2_c01x_w );

READ_HANDLER ( apple2_c02x_r );
WRITE_HANDLER ( apple2_c02x_w );

READ_HANDLER ( apple2_c03x_r );
WRITE_HANDLER ( apple2_c03x_w );

READ_HANDLER ( apple2_c05x_r );
WRITE_HANDLER ( apple2_c05x_w );

READ_HANDLER ( apple2_c06x_r );

READ_HANDLER ( apple2_c07x_r );
WRITE_HANDLER ( apple2_c07x_w );

READ_HANDLER ( apple2_c08x_r );
WRITE_HANDLER ( apple2_c08x_w );

READ_HANDLER ( apple2_c0xx_slot1_r );
READ_HANDLER ( apple2_c0xx_slot2_r );
READ_HANDLER ( apple2_c0xx_slot3_r );
READ_HANDLER ( apple2_c0xx_slot4_r );
READ_HANDLER ( apple2_c0xx_slot5_r );
READ_HANDLER ( apple2_c0xx_slot7_r );
WRITE_HANDLER ( apple2_c0xx_slot1_w );
WRITE_HANDLER ( apple2_c0xx_slot2_w );
WRITE_HANDLER ( apple2_c0xx_slot3_w );
WRITE_HANDLER ( apple2_c0xx_slot4_w );
WRITE_HANDLER ( apple2_c0xx_slot5_w );
WRITE_HANDLER ( apple2_c0xx_slot7_w );
WRITE_HANDLER ( apple2_slot1_w );
WRITE_HANDLER ( apple2_slot2_w );
WRITE_HANDLER ( apple2_slot3_w );
WRITE_HANDLER ( apple2_slot4_w );
WRITE_HANDLER ( apple2_slot5_w );
WRITE_HANDLER ( apple2_slot7_w );
/*int  apple2_slot1_r(int offset);
int  apple2_slot2_r(int offset);
int  apple2_slot3_r(int offset);*/
READ_HANDLER ( apple2_slot4_r );
/*int  apple2_slot5_r(int offset);
int  apple2_slot6_r(int offset);*/
READ_HANDLER ( apple2_slot7_r );


/* machine/ap_disk2.c */
void apple2_slot6_init(void);
DEVICE_LOAD ( apple2_floppy );
READ_HANDLER ( apple2_c0xx_slot6_r );
WRITE_HANDLER ( apple2_c0xx_slot6_w );
WRITE_HANDLER ( apple2_slot6_w );


/* vidhrdw/apple2.c */
VIDEO_START( apple2 );
VIDEO_UPDATE( apple2 );
void apple2_video_touch(offs_t offset);

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
