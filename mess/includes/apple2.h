/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2_H
#define APPLE2_H

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

READ8_HANDLER ( apple2_c00x_r );
WRITE8_HANDLER ( apple2_c00x_w );

READ8_HANDLER ( apple2_c01x_r );
WRITE8_HANDLER ( apple2_c01x_w );

READ8_HANDLER ( apple2_c02x_r );
WRITE8_HANDLER ( apple2_c02x_w );

READ8_HANDLER ( apple2_c03x_r );
WRITE8_HANDLER ( apple2_c03x_w );

READ8_HANDLER ( apple2_c05x_r );
WRITE8_HANDLER ( apple2_c05x_w );

READ8_HANDLER ( apple2_c06x_r );

READ8_HANDLER ( apple2_c07x_r );
WRITE8_HANDLER ( apple2_c07x_w );

READ8_HANDLER ( apple2_c08x_r );
WRITE8_HANDLER ( apple2_c08x_w );

READ8_HANDLER ( apple2_c0xx_slot1_r );
READ8_HANDLER ( apple2_c0xx_slot2_r );
READ8_HANDLER ( apple2_c0xx_slot3_r );
READ8_HANDLER ( apple2_c0xx_slot4_r );
READ8_HANDLER ( apple2_c0xx_slot5_r );
READ8_HANDLER ( apple2_c0xx_slot7_r );
WRITE8_HANDLER ( apple2_c0xx_slot1_w );
WRITE8_HANDLER ( apple2_c0xx_slot2_w );
WRITE8_HANDLER ( apple2_c0xx_slot3_w );
WRITE8_HANDLER ( apple2_c0xx_slot4_w );
WRITE8_HANDLER ( apple2_c0xx_slot5_w );
WRITE8_HANDLER ( apple2_c0xx_slot7_w );
WRITE8_HANDLER ( apple2_slot1_w );
WRITE8_HANDLER ( apple2_slot2_w );
WRITE8_HANDLER ( apple2_slot3_w );
WRITE8_HANDLER ( apple2_slot4_w );
WRITE8_HANDLER ( apple2_slot5_w );
WRITE8_HANDLER ( apple2_slot7_w );
/*int  apple2_slot1_r(int offset);
int  apple2_slot2_r(int offset);
int  apple2_slot3_r(int offset);*/
READ8_HANDLER ( apple2_slot4_r );
/*int  apple2_slot5_r(int offset);
int  apple2_slot6_r(int offset);*/
READ8_HANDLER ( apple2_slot7_r );

void apple2_setvar(UINT32 val, UINT32 mask);
void apple2_setforceslotrom(UINT8 val);
UINT8 apple2_getforceslotrom(void);

/* machine/ap_disk2.c */
void apple2_slot6_init(void);
DEVICE_LOAD ( apple2_floppy );
READ8_HANDLER ( apple2_c0xx_slot6_r );
WRITE8_HANDLER ( apple2_c0xx_slot6_w );
WRITE8_HANDLER ( apple2_slot6_w );


/* vidhrdw/apple2.c */
VIDEO_START( apple2 );
VIDEO_START( apple2p );
VIDEO_START( apple2e );
VIDEO_UPDATE( apple2 );
void apple2_video_touch(offs_t offset);
void apple2_set_fgcolor(int color);
void apple2_set_bgcolor(int color);
int apple2_get_fgcolor(void);
int apple2_get_bgcolor(void);

/* keyboard wrappers */
#define pressed_specialkey(key)	(input_port_8_r(0) & (key))
#define SPECIALKEY_CAPSLOCK		0x01
#define SPECIALKEY_SHIFT		0x06
#define SPECIALKEY_CONTROL		0x08
#define SPECIALKEY_BUTTON0		0x10	/* open apple */
#define SPECIALKEY_BUTTON1		0x20	/* closed apple */
#define SPECIALKEY_BUTTON2		0x40
#define SPECIALKEY_RESET		0x80

/* bank constants; just because Apple II banking is so ridiculously complicated */
#define A2BANK_0000				1
#define A2BANK_0200_R			2
#define A2BANK_0200_W			3
#define A2BANK_0400_R			4
#define A2BANK_0400_W			5
#define A2BANK_0800_R			6
#define A2BANK_0800_W			7
#define A2BANK_2000_R			8
#define A2BANK_2000_W			9
#define A2BANK_4000_R			10
#define A2BANK_4000_W			11
#define A2BANK_C100				12
#define A2BANK_C200				13
#define A2BANK_C300				14
#define A2BANK_C400				15
#define A2BANK_C500				16
#define A2BANK_C600				17
#define A2BANK_C700				18
#define A2BANK_C800				19
#define A2BANK_D000_R			20
#define A2BANK_D000_W			21
#define A2BANK_E000_R			22
#define A2BANK_E000_W			23

#define MRA8_A2BANK_0000	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_0000))
#define MRA8_A2BANK_0200	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_0200_R))
#define MRA8_A2BANK_0400	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_0400_R))
#define MRA8_A2BANK_0800	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_0800_R))
#define MRA8_A2BANK_2000	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_2000_R))
#define MRA8_A2BANK_4000	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_4000_R))
#define MRA8_A2BANK_C100	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C100))
#define MRA8_A2BANK_C200	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C200))
#define MRA8_A2BANK_C300	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C300))
#define MRA8_A2BANK_C400	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C400))
#define MRA8_A2BANK_C500	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C500))
#define MRA8_A2BANK_C600	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C600))
#define MRA8_A2BANK_C700	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C700))
#define MRA8_A2BANK_C800	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_C800))
#define MRA8_A2BANK_D000	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_D000_R))
#define MRA8_A2BANK_E000	((read8_handler) (STATIC_BANK1 - 1 + A2BANK_E000_R))

#define MWA8_A2BANK_0000	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_0000))
#define MWA8_A2BANK_0200	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_0200_W))
#define MWA8_A2BANK_0400	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_0400_W))
#define MWA8_A2BANK_0800	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_0800_W))
#define MWA8_A2BANK_2000	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_2000_W))
#define MWA8_A2BANK_4000	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_4000_W))
#define MWA8_A2BANK_C100	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C100))
#define MWA8_A2BANK_C200	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C200))
#define MWA8_A2BANK_C300	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C300))
#define MWA8_A2BANK_C400	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C400))
#define MWA8_A2BANK_C500	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C500))
#define MWA8_A2BANK_C600	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C600))
#define MWA8_A2BANK_C700	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C700))
#define MWA8_A2BANK_C800	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_C800))
#define MWA8_A2BANK_D000	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_D000_W))
#define MWA8_A2BANK_E000	((write8_handler) (STATIC_BANK1 - 1 + A2BANK_E000_W))

#endif /* APPLE2_H */
