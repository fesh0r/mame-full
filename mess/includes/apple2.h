/***************************************************************************

	apple2.h

	Include file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2_H
#define APPLE2_H

#include <mame.h>

#define APDISK_DEVTAG	"apdsk_525"

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
void apple2_init_common(void);
DRIVER_INIT( apple2 );
MACHINE_INIT( apple2 );

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

/* machine/ap_disk2.c */
void apple2_slot6_init(void);
DEVICE_LOAD ( apple2_floppy );
READ8_HANDLER ( apple2_c0xx_slot6_r );
WRITE8_HANDLER ( apple2_c0xx_slot6_w );
WRITE8_HANDLER ( apple2_slot6_w );
data8_t apple2_getfloatingbusvalue(void);


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
#define pressed_specialkey(key)	(readinputportbytag("keyb_special") & (key))
#define SPECIALKEY_CAPSLOCK		0x01
#define SPECIALKEY_SHIFT		0x06
#define SPECIALKEY_CONTROL		0x08
#define SPECIALKEY_BUTTON0		0x10	/* open apple */
#define SPECIALKEY_BUTTON1		0x20	/* closed apple */
#define SPECIALKEY_BUTTON2		0x40
#define SPECIALKEY_RESET		0x80

/* -----------------------------------------------------------------------
 * New Apple II memory manager
 * ----------------------------------------------------------------------- */

#define APPLE2_MEM_AUX		0x40000000
#define APPLE2_MEM_SLOT		0x80000000
#define APPLE2_MEM_ROM		0xC0000000
#define APPLE2_MEM_FLOATING	0xFFFFFFFF
#define APPLE2_MEM_MASK		0x00FFFFFF

typedef enum
{
	A2MEM_IO		= 0,	/* this is always handlers; never banked memory */
	A2MEM_MONO		= 1,	/* this is a bank where read and write are always in unison */
	A2MEM_DUAL		= 2		/* this is a bank where read and write can go different places */
} bank_disposition_t;

struct apple2_meminfo
{
	UINT32 read_mem;
	read8_handler read_handler;
	UINT32 write_mem;
	write8_handler write_handler;
};

struct apple2_memmap_entry
{
	offs_t begin;
	offs_t end;
	void (*get_meminfo)(offs_t begin, offs_t end, struct apple2_meminfo *meminfo);
	bank_disposition_t bank_disposition;
};

struct apple2_memmap_config
{
	int first_bank;
	UINT8 *auxmem;
	UINT32 auxmem_length;
	const struct apple2_memmap_entry *memmap;
};

void apple2_setup_memory(const struct apple2_memmap_config *config);
void apple2_update_memory(void);

WRITE8_HANDLER( apple2_mainram0400_w );
WRITE8_HANDLER( apple2_auxram0400_w );
WRITE8_HANDLER( apple2_mainram2000_w );
WRITE8_HANDLER( apple2_auxram2000_w );

#endif /* APPLE2_H */
