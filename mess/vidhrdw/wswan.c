/***************************************************************************

  wswan.c

  File to handle video emulation of the Bandai WonderSwan.

  Anthony Kruize
  Wilbert Pol

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/wswan.h"

void wswan_draw_background(void) {
	UINT16	map_addr;
	UINT8	start_column;
	int	column;

	map_addr = vdp.layer_bg_address + ( ( ( vdp.current_line + vdp.layer_bg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( vdp.layer_bg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		int	tile_data, tile_number, tile_palette, tile_line;
		int	plane0, plane1, x, x_offset, pal[4];

		tile_data = ( vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
		            | vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		tile_number = tile_data & 0x01FF;
		tile_palette = ( tile_data >> 9 ) & 0x0F;

		tile_line = ( vdp.current_line + vdp.layer_bg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			tile_line = 7 - tile_line;
		}

		pal[0] = ws_portram[ 0x20 + ( tile_palette << 1 ) ] & 0x07;
		pal[1] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) ] >> 4 ) & 0x07;
		pal[2] = ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] & 0x07;
		pal[3] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] >> 4 ) & 0x07;

		plane0 = vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 ) ];
		plane1 = ( vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( tile_line << 1 ) + 1 ] << 1 );

		for( x = 0; x < 8; x++ ) {
			int col = ( plane1 & 2 ) | ( plane0 & 1 );
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( vdp.layer_bg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( vdp.layer_bg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
				if ( pal[col] || tile_palette < 8 ) {
					plot_pixel( tmpbitmap, x_offset, vdp.current_line, Machine->pens[vdp.main_palette[pal[col]]] );
				}
			}
			plane1 = plane1 >> 1;
			plane0 = plane0 >> 1;
		}
	}
}

void wswan_draw_foreground_0( void ) {
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = vdp.layer_fg_address + ( ( ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		int	plane0, plane1, x, x_offset, y_offset, pal[4];
		int	tile_data = ( vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
                                    | vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		y_offset = ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			y_offset = 7 - y_offset;
		}

		pal[0] = ws_portram[ 0x20 + ( tile_palette << 1 ) ] & 0x07;
		pal[1] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) ] >> 4 ) & 0x07;
		pal[2] = ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] & 0x07;
		pal[3] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] >> 4 ) & 0x07;

		plane0 = vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) ];
		plane1 = ( vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) + 1 ] << 1 );

		for( x = 0; x < 8; x++ ) {
			int	col = ( plane1 & 2 ) | ( plane0 & 1 );
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
				if ( col || !(tile_palette & 4) ) {
					plot_pixel( tmpbitmap, x_offset, vdp.current_line, Machine->pens[vdp.main_palette[pal[col]]] );
				}
			}
			plane1 = plane1 >> 1;
			plane0 = plane0 >> 1;
		}
	}
}

void wswan_draw_foreground_2( void ) {
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = vdp.layer_fg_address + ( ( ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		int	plane0, plane1, x, x_offset, y_offset, pal[4];
		int	tile_data = ( vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
			            | vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		y_offset = ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			y_offset = 7 - y_offset;
		}

		pal[0] = ws_portram[ 0x20 + ( tile_palette << 1 ) ] & 0x07;
		pal[1] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) ] >> 4 ) & 0x07;
		pal[2] = ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] & 0x07;
		pal[3] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] >> 4 ) & 0x07;

		plane0 = vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) ];
		plane1 = ( vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) + 1 ] << 1 );

		for( x = 0; x < 8; x++ ) {
			int	col = ( plane1 & 2 ) | ( plane0 & 1 );
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( x_offset >= 0 && x_offset >= vdp.window_fg_left && x_offset < vdp.window_fg_right && x_offset < WSWAN_X_PIXELS ) {
				if ( col || !(tile_palette & 4) ) {
					plot_pixel( tmpbitmap, x_offset, vdp.current_line, Machine->pens[vdp.main_palette[pal[col]]] );
				}
			}
			plane1 = plane1 >> 1;
			plane0 = plane0 >> 1;
		}
	}
}

void wswan_draw_foreground_3( void ) {
	UINT16	map_addr;
	UINT8	start_column;
	int	column;
	map_addr = vdp.layer_fg_address + ( ( ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0xF8 ) << 3 );
	start_column = ( vdp.layer_fg_scroll_x >> 3 );
	for( column = 0; column < 29; column++ ) {
		int	plane0, plane1, x, x_offset, y_offset, pal[4];
		int	tile_data = ( vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) + 1 ] << 8 )
			            | vdp.vram[ map_addr + ( ( ( start_column + column ) & 0x1F ) << 1 ) ];
		int	tile_number = tile_data & 0x01FF;
		int	tile_palette = ( tile_data >> 9 ) & 0x0F;

		y_offset = ( vdp.current_line + vdp.layer_fg_scroll_y ) & 0x07;
		if ( tile_data & 0x8000 ) {
			y_offset = 7 - y_offset;
		}

		pal[0] = ws_portram[ 0x20 + ( tile_palette << 1 ) ] & 0x07;
		pal[1] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) ] >> 4 ) & 0x07;
		pal[2] = ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] & 0x07;
		pal[3] = ( ws_portram[ 0x20 + ( tile_palette << 1 ) + 1 ] >> 4 ) & 0x07;

		plane0 = vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) ];
		plane1 = ( vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) + 1 ] << 1 );

		for( x = 0; x < 8; x++ ) {
			int	col = ( plane1 & 2 ) | ( plane0 & 1 );
			if ( tile_data & 0x4000 ) {
				x_offset = x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			} else {
				x_offset = 7 - x + ( column << 3 ) - ( vdp.layer_fg_scroll_x & 0x07 );
			}
			if ( ( x_offset >= 0 && x_offset < vdp.window_fg_left ) || ( x_offset >= vdp.window_fg_right && x_offset < WSWAN_X_PIXELS ) ) {
				if ( col || !(tile_palette & 4) ) {
					plot_pixel( tmpbitmap, x_offset, vdp.current_line, Machine->pens[vdp.main_palette[pal[col]]] );
				}
			}
			plane1 = plane1 >> 1;
			plane0 = plane0 >> 1;
		}
	}
}

void wswan_handle_sprites( int mask ) {
	int	i;
	for( i = vdp.sprite_first; i < vdp.sprite_last; i++ ) {
		UINT8	x, y;
		UINT16	tile_data;
		int	y_offset;

		tile_data = ( vdp.vram[ vdp.sprite_table_address + i * 4 + 1 ] << 8 )
		            | vdp.vram[ vdp.sprite_table_address + i * 4 ];
		y = vdp.vram[ vdp.sprite_table_address + i * 4 + 2 ];
		x = vdp.vram[ vdp.sprite_table_address + i * 4 + 3 ];
		y_offset = vdp.current_line - y;
		if ( ( y_offset >= 0 ) && ( y_offset < 8 ) && ( ( tile_data & mask ) == mask ) ) {
			int	plane0, plane1, pal[4], j, x_offset;
			int	tile_number = tile_data & 0x01FF;
			int	tile_palette = ( tile_data >> 9 ) & 0x07;
			if ( tile_data & 0x8000 ) {
				y_offset = 7 - y_offset;
			}

			pal[0] = ws_portram[ 0x30 + ( tile_palette << 1 ) ] & 0x07;
			pal[1] = ( ws_portram[ 0x30 + ( tile_palette << 1 ) ] >> 4 ) & 0x07;
			pal[2] = ws_portram[ 0x30 + ( tile_palette << 1 ) + 1 ] & 0x07;
			pal[3] = ( ws_portram[ 0x30 + ( tile_palette << 1 ) + 1 ] >> 4 ) & 0x07;

			plane0 = vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) ];
			plane1 = ( vdp.vram[ 0x2000 + ( tile_number * 16 ) + ( y_offset << 1 ) + 1 ] << 1 );

			if ( ( ( tile_data & 0x1000 ) == 0x0000 ) || ( vdp.current_line >= vdp.window_sprites_top && vdp.current_line < vdp.window_sprites_bottom ) ) {
				for ( j = 0; j < 8; j++ ) {
					int col = ( plane1 & 2 ) | ( plane0 & 1 );
					if ( tile_data & 0x4000 ) {
						x_offset = x + j;
					} else {
						x_offset = x + 7 - j;
					}
					if ( ( ( tile_data & 0x1000 ) == 0x0000 ) || ( x_offset >= vdp.window_sprites_left && x_offset < vdp.window_sprites_right ) ) {
						if ( x_offset >= 0 && x_offset < WSWAN_X_PIXELS ) {
							if ( col || tile_palette < 4 ) {
								plot_pixel( tmpbitmap, x_offset, vdp.current_line, Machine->pens[vdp.main_palette[pal[col]]] );
							}
						}
					}
					plane1 = plane1 >> 1;
					plane0 = plane0 >> 1;
				}
			}
		}
	}
}

void wswan_refresh_scanline(void)
{
	rectangle rec;
	rec.min_x = 0;
	rec.max_x = WSWAN_X_PIXELS;
	rec.min_y = rec.max_y = vdp.current_line;
	if ( ws_portram[0x14] ) {
		fillbitmap( tmpbitmap, Machine->pens[ ws_portram[0x01] & 0x0F ], &rec );
	} else {
		fillbitmap( tmpbitmap, Machine->pens[ 0 ], &rec );
		return;
	}

	/*
	 * Draw background layer
	 */
	if ( vdp.layer_bg_enable ) {
		wswan_draw_background();
	}

	/*
	 * Draw sprites between background and foreground layers
	 */
	if ( vdp.sprites_enable ) {
		wswan_handle_sprites( 0 );
	}

	/*
	 * Draw foreground layer, taking window settings into account
	 */
	if ( vdp.layer_fg_enable ) {
		switch( vdp.window_fg_mode ) {
		case 0:	/* FG inside & outside window area */
			wswan_draw_foreground_0();
			break;
		case 1:	/* ??? */
			printf( "Unknown foreground mode 1 set\n" );
			break;
		case 2:	/* FG only inside window area */
			if ( vdp.current_line >= vdp.window_fg_top && vdp.current_line < vdp.window_fg_bottom ) {
				wswan_draw_foreground_2();
			}
			break;
		case 3:	/* FG only outside window area */
			if ( vdp.current_line < vdp.window_fg_top || vdp.current_line >= vdp.window_fg_bottom ) {
				wswan_draw_foreground_0();
			} else {
				wswan_draw_foreground_3();
			}
			break;
		}
	}

	/*
	 * Draw sprites in front of foreground layer
	 */
	if ( vdp.sprites_enable ) {
		wswan_handle_sprites( 0x2000 );
	}
}

