/***************************************************************************

  wswan.c

  Machine file to handle emulation of the Bandai WonderSwan.

  Anthony Kruize

***************************************************************************/

#include "driver.h"
#include "includes/wswan.h"
#include "image.h"

static UINT8 *ROMMap[128];
static UINT8 ROMBanks;
struct VDP vdp;
UINT8 *ws_ram;
UINT8 ws_portram[256];
static UINT8 ws_portram_init[256] =
{
	0x00, 0x00, 0x00/*?*/, 0xbb, 0x00, 0x00, 0x00, 0x26, 0xfe, 0xde, 0xf9, 0xfb, 0xdb, 0xd7, 0x7f, 0xf5,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x9e, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x99, 0xfd, 0xb7, 0xdf,
	0x30, 0x57, 0x75, 0x76, 0x15, 0x73, 0x77, 0x77, 0x20, 0x75, 0x50, 0x36, 0x70, 0x67, 0x50, 0x77,
	0x57, 0x54, 0x75, 0x77, 0x75, 0x17, 0x37, 0x73, 0x50, 0x57, 0x60, 0x77, 0x70, 0x77, 0x10, 0x73,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x87, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xdb, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x42, 0x00, 0x83, 0x00,
	0x2f, 0x3f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1
};

void wswan_handle_irqs( void ) {
	if ( ws_portram[0xb6] & WSWAN_IFLAG_HBLTMR ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_HBLTMR );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_VBL ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_VBL );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_VBLTMR ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_VBLTMR );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_LCMP ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_LCMP );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_SRX ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_SRX );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_RTC ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_RTC );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_KEY ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_KEY );
	} else if ( ws_portram[0xb6] & WSWAN_IFLAG_STX ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_STX );
	} else {
		cpunum_set_input_line( 0, 0, CLEAR_LINE );
	}
}

void wswan_set_irq_line(int irq) {
	ws_portram[0xb6] |= irq;
	wswan_handle_irqs();
}

void wswan_clear_irq_line(int irq) {
	ws_portram[0xb6] &= ~irq;
	wswan_handle_irqs();
}


MACHINE_RESET( wswan )
{
	/* Intialize ports */
	memcpy( ws_portram, ws_portram_init, 256 );

	/* Switch in the banks */
	memory_set_bankptr( 2, ROMMap[ROMBanks - 1] );
	memory_set_bankptr( 3, ROMMap[ROMBanks - 1] );
	memory_set_bankptr( 4, ROMMap[ROMBanks - 12] );
	memory_set_bankptr( 5, ROMMap[ROMBanks - 11] );
	memory_set_bankptr( 6, ROMMap[ROMBanks - 10] );
	memory_set_bankptr( 7, ROMMap[ROMBanks - 9] );
	memory_set_bankptr( 8, ROMMap[ROMBanks - 8] );
	memory_set_bankptr( 9, ROMMap[ROMBanks - 7] );
	memory_set_bankptr( 10, ROMMap[ROMBanks - 6] );
	memory_set_bankptr( 11, ROMMap[ROMBanks - 5] );
	memory_set_bankptr( 12, ROMMap[ROMBanks - 4] );
	memory_set_bankptr( 13, ROMMap[ROMBanks - 3] );
	memory_set_bankptr( 14, ROMMap[ROMBanks - 2] );
	memory_set_bankptr( 15, ROMMap[ROMBanks - 1] );

	vdp.vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, 0 );

	vdp.current_line = 0;
}

READ8_HANDLER( wswan_port_r )
{
	UINT8 value = ws_portram[offset];

	logerror( "port read %02X\n", offset );
	switch( offset )
	{
		case 0x02:		/* Current line */
			value = vdp.current_line;
			break;
		case 0xA0:		/* Hardware type */
			value = WSWAN_TYPE_MONO;
			break;
	}

	return value;
}

WRITE8_HANDLER( wswan_port_w )
{
	logerror( "port write %02X <- %02X\n", offset, data );
	switch( offset )
	{
		case 0x00:		/* Display control */
			vdp.layer_bg_enable = data & 0x1;
			vdp.layer_fg_enable = (data & 0x2) >> 1;
			vdp.sprites_enable = (data & 0x4) >> 2;
			vdp.window_sprites_enable = (data & 0x8) >> 3;
			vdp.window_fg_mode = (data & 0x30) >> 4;
			break;
		case 0x01:		/* Background colour */
			/* FIXME: implement */
			break;
		case 0x02:		/* Current scanline - most likely read-only */
			logerror( "Write to current scanline! Current value: %d  Data to write: %d\n", vdp.current_line, data );
			/* Returning so we don't overwrite the value here, not that it
			 * really matters */
			return;
		case 0x03:		/* Line compare */
			vdp.line_compare = data;
			break;
		case 0x04:		/* Sprite table base address */
			vdp.sprite_table_address = data << 9;
			break;
		case 0x05:		/* Number of sprite to start drawing with */
			vdp.sprite_first = data;
			break;
		case 0x06:		/* Number of sprite to stop drawing with */
			vdp.sprite_last = data;
			break;
		case 0x07:		/* Screen addresses */
			vdp.layer_bg_address = (data & 0xf) << 11;
			vdp.layer_fg_address = (data & 0xf0) << 7;
			break;
		case 0x08:		/* Left coordinate of foreground window */
			vdp.window_fg_left = data;
			break;
		case 0x09:		/* Top coordinate of foreground window */
			vdp.window_fg_top = data;
			break;
		case 0x0A:		/* Right coordinate of foreground window */
			vdp.window_fg_right = data;
			break;
		case 0x0B:		/* Bottom coordinate of foreground window */
			vdp.window_fg_bottom = data;
			break;
		case 0x0C:		/* Left coordinate of sprite window */
			vdp.window_sprites_left = data;
			break;
		case 0x0D:		/* Top coordinate of sprite window */
			vdp.window_sprites_top = data;
			break;
		case 0x0E:		/* Right coordinate of sprite window */
			vdp.window_sprites_right = data;
			break;
		case 0x0F:		/* Bottom coordinate of sprite window */
			vdp.window_sprites_bottom = data;
			break;
		case 0x10:		/* Background layer X scroll */
			vdp.layer_bg_scroll_x = data;
			break;
		case 0x11:		/* Background layer Y scroll */
			vdp.layer_bg_scroll_y = data;
			break;
		case 0x12:		/* Foreground layer X scroll */
			vdp.layer_fg_scroll_x = data;
			break;
		case 0x13:		/* Foreground layer Y scroll */
			vdp.layer_fg_scroll_y = data;
			break;
		case 0x14:		/* LCD enable */
			vdp.lcd_enable = data & 0x1;
			break;
		case 0x15:		/* LCD icons */
			vdp.icons = data;	/* ummmmm */
			break;
		case 0x1c:		/* Palette colors 1 and 2 */
			vdp.main_palette[0] = data & 0x0F;
			vdp.main_palette[1] = ( data & 0xF0 ) >> 4;
			break;
		case 0x1d:		/* Palette colors 3 and 4 */
			vdp.main_palette[2] = data & 0x0F;
			vdp.main_palette[3] = ( data & 0xF0 ) >> 4;
			break;
		case 0x1e:		/* Palette colors 5 and 6 */
			vdp.main_palette[4] = data & 0x0F;
			vdp.main_palette[5] = ( data & 0xF0 ) >> 4;
			break;
		case 0x1f:		/* Palette colors 7 and 8 */
			vdp.main_palette[6] = data & 0x0F;
			vdp.main_palette[7] = ( data & 0xF0 ) >> 4;
			break;
		case 0x20:		/* tile/sprite palette settings */
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			break;
		case 0x40:		/* DMA source address (low) */
		case 0x41:		/* DMA source address (high) */
		case 0x42:		/* DMA source bank */
		case 0x43:		/* DMA destination bank */
		case 0x44:		/* DMA destination address (low) */
		case 0x45:		/* DMA destination address (hgih) */
		case 0x46:		/* Size of copied data (low) */
		case 0x47:		/* Size of copied data (high) */
			break;
		case 0x48:		/* DMA start/stop */
			if( data & 0x80 )
			{
				UINT32 src, dst;
				UINT16 length;

				src = ws_portram[0x40] + (ws_portram[0x41] << 8) + (ws_portram[0x42] << 16);
				dst = ws_portram[0x44] + (ws_portram[0x45] << 8) + (ws_portram[0x43] << 16);
				length = ws_portram[0x46] + (ws_portram[0x47] << 8);
				memcpy( &ws_ram[dst], &ws_ram[src], length );
				data = 0;
				printf( "DMA  src:%X dst:%X length:%d\n", src, dst, length );
			}
			break;
		case 0x60:		/* Video mode */
			/* FIXME: Is this WSC only? */
			break;
		case 0x80:		/* sound registers */
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
			break;
		case 0xa0:		/* Hardware type - this is probably read only */
			break;
		case 0xa2:		/* Timer control */
			vdp.timer_hblank_enable = data & 0x1;
			vdp.timer_hblank_mode = (data & 0x2) >> 1;
			vdp.timer_vblank_enable = (data & 0x4) >> 2;
			vdp.timer_vblank_mode = (data & 0x8) >> 3;
			break;
		case 0xa4:		/* HBlank timer frequency (low) */
			vdp.timer_hblank_freq &= 0xff00;
			vdp.timer_hblank_freq += data;
			break;
		case 0xa5:		/* HBlank timer frequenct (high) */
			vdp.timer_hblank_freq &= 0xff;
			vdp.timer_hblank_freq += data << 8;
			break;
		case 0xa6:		/* VBlank timer frequency (low) */
			vdp.timer_vblank_freq &= 0xff00;
			vdp.timer_vblank_freq += data;
			break;
		case 0xa7:		/* VBlank timer frequenct (high) */
			vdp.timer_vblank_freq &= 0xff;
			vdp.timer_vblank_freq += data << 8;
			break;
		case 0xb0:		/* Interrupt base */
			/* FIXME: what is this? */
			break;
		case 0xb2:		/* Interrupt enable */
			break;
		case 0xb3:		/* serial communication */
			if ( data & 0x80 ) {
				data |= 0x04;
			}
			break;
		case 0xb6:		/* Interrupt acknowledge */
			wswan_clear_irq_line( data );
			break;
		case 0xc0:		/* ROM bank select for banks 3-14 */
			printf( "ROM bank select - unsupported\n" );
			break;
		case 0xc1:		/* SRAM bank select */
			/* FIXME: unsupported */
/*			memory_set_bankptr( 1, RAMMap[data] ); */
			break;
		case 0xc2:		/* ROM bank select for bank 1 (0x2000-0x2fff) */
			memory_set_bankptr( 2, ROMMap[ROMBanks - (0xff - data) - 1]);
			break;
		case 0xc3:		/* ROM bank select for bank 2 (0x3000-0x3fff) */
			memory_set_bankptr( 3, ROMMap[ROMBanks - (0xff - data) - 1]);
			break;
		default:
			logerror( "Write to unsupported port: %X - %X\n", offset, data );
			break;
	}

	/* Update the port value */
	ws_portram[offset] = data;
}

enum enum_sram { SRAM_NONE=0, SRAM_64K, SRAM_256K, EEPROM_1K, EEPROM_16K, EEPROM_8K, SRAM_UNKNOWN };
const char* wswan_sram_str[] = {
	"none", "64K SRAM", "256K SRAM", "1K EEPROM", "16K EEPROM", "8K EEPROM", "Unknown"
};

static const char* wswan_determine_sram( UINT8 data ) {
	switch( data ) {
	case 0x00: return wswan_sram_str[ SRAM_NONE ];
	case 0x01: return wswan_sram_str[ SRAM_64K ];
	case 0x02: return wswan_sram_str[ SRAM_256K ];
	case 0x10: return wswan_sram_str[ EEPROM_1K ];
	case 0x20: return wswan_sram_str[ EEPROM_16K ];
	case 0x50: return wswan_sram_str[ EEPROM_8K ];
	}
	return wswan_sram_str[ SRAM_UNKNOWN ];
}

enum enum_romsize { ROM_4M=0, ROM_8M, ROM_16M, ROM_32M, ROM_64M, ROM_128M, ROM_UNKNOWN };
const char* wswan_romsize_str[] = {
	"4Mbit", "8Mbit", "16Mbit", "32Mbit", "64Mbit", "128Mbit", "Unknown"
};

static const char* wswan_determine_romsize( UINT8 data ) {
	switch( data ) {
	case 0x02:	return wswan_romsize_str[ ROM_4M ];
	case 0x03:	return wswan_romsize_str[ ROM_8M ];
	case 0x04:	return wswan_romsize_str[ ROM_16M ];
	case 0x06:	return wswan_romsize_str[ ROM_32M ];
	case 0x08:	return wswan_romsize_str[ ROM_64M ];
	case 0x09:	return wswan_romsize_str[ ROM_128M ];
	}
	return wswan_romsize_str[ ROM_UNKNOWN ];
}

DEVICE_LOAD(wswan_cart)
{
	UINT8 ii;

	if( new_memory_region( REGION_CPU1, 0x100000, 0) )
	{
		logerror( "Memory allocation failed reading rom!\n" );
		return INIT_FAIL;
	}

	ws_ram = memory_region( REGION_CPU1 );
	memset( ws_ram, 0, 0x100000 );
	ROMBanks = mame_fsize( file ) / 65536;

	for( ii = 0; ii < ROMBanks; ii++ )
	{
		if( (ROMMap[ii] = (UINT8 *)malloc( 0x10000 )) )
		{
			if( mame_fread( file, ROMMap[ii], 0x10000 ) != 0x10000 )
			{
				logerror( "Error while reading loading rom!\n" );
				return INIT_FAIL;
			}
		}
		else
		{
			logerror( "Memory allocation failed reading rom!\n" );
			return INIT_FAIL;
		}
	}

#ifdef MAME_DEBUG
	/* Spit out some info */
	printf( "ROM DETAILS\n" );
	printf( "\tDeveloper ID: %X\n", ROMMap[ROMBanks-1][0xfff6] );
	printf( "\tMinimum system: %s\n", ROMMap[ROMBanks-1][0xfff7] ? "WonderSwan Color" : "WonderSwan" );
	printf( "\tCart ID: %X\n", ROMMap[ROMBanks-1][0xfff8] );
	printf( "\tROM size: %s\n", wswan_determine_romsize( ROMMap[ROMBanks-1][0xfffa] ) );
	printf( "\tSRAM size: %s\n", wswan_determine_sram( ROMMap[ROMBanks-1][0xfffb] ) );
	printf( "\tFeatures: %X\n", ROMMap[ROMBanks-1][0xfffc] );
	printf( "\tRTC: %s\n", ( ROMMap[ROMBanks-1][0xfffd] ? "yes" : "no" ) );
	printf( "\tChecksum: %X%X\n", ROMMap[ROMBanks-1][0xffff], ROMMap[ROMBanks-1][0xfffe] );
#endif

	/* All done */
	return INIT_PASS;
}

INTERRUPT_GEN(wswan_scanline_interrupt)
{
	if( vdp.current_line < 144 )
		wswan_refresh_scanline();

	vdp.current_line = (vdp.current_line + 1) % 158; /*159?*/

	if( (ws_portram[0xb2] & WSWAN_IFLAG_VBL) && (vdp.current_line == 144) )
	{
		logerror( "Setting VBL interrupt line\n" );
		wswan_set_irq_line( WSWAN_IFLAG_VBL );
	}
}
