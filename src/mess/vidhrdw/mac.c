#include "driver.h"
#include "vidhrdw/generic.h"

/* from machine */
extern void macplus_scc_mouse_irq( int x, int y );

static void *mouse_timer;
static int mouse_bit_x = 0, mouse_bit_y = 0;
static UINT16 *old_display;

int macplus_mouse_x( void ) {
	return mouse_bit_x;
}

int macplus_mouse_y( void ) {
	return mouse_bit_y;
}

static void mouse_callback( int param ) {
	static int	last_mx = 0, last_my = 0;
	static int	count_x = 0, count_y = 0;
	int			new_mx, new_my;
	int			x_needs_update = 0, y_needs_update = 0;
	
	new_mx = readinputport( 1 );
	new_my = readinputport( 2 );
	
	/* if the mouse hasnt moved, update any remaining counts and then return */
	if ( last_mx == new_mx  && last_my == new_my ) {
		if ( count_x ) {
			if ( count_x < 0 ) {
				count_x++;
				mouse_bit_x = 0;
			} else {
				count_x--;
				mouse_bit_x = 1;
			}
			x_needs_update = 1;
		} else {
			if ( count_y ) {
				if ( count_y < 0 ) {
					count_y++;
					mouse_bit_y = 1;
				} else {
					count_y--;
					mouse_bit_y = 0;
				}
				
				y_needs_update = 1;
			} else {
				return;
			}
		}
	} else {
		
		/* see if it moved in the x coord */
		if ( new_mx != last_mx ) {
			int		diff = new_mx - last_mx;
			
			/* check for wrap */
			if ( diff > 200 || diff < -200 ) {
				if ( diff < 0 )
					diff = - ( 256 + diff );
				else
					diff = 256 - diff;
			}
			
			count_x += diff;
			
			if ( count_x < 0 ) {
				count_x++;
				mouse_bit_x = 0;
			} else {
				count_x--;
				mouse_bit_x = 1;
			}
			
			x_needs_update = 1;
		}
			
		/* see if it moved in the x coord */
		if ( new_my != last_my ) {
			int		diff = new_my - last_my;
			
			/* check for wrap */
			if ( diff > 200 || diff < -200 ) {
				if ( diff < 0 )
					diff = - ( 256 + diff );
				else
					diff = 256 - diff;
			}
			
			count_y += diff;
			
			if ( count_y < 0 ) {
				count_y++;
				mouse_bit_y = 1;
			} else {
				count_y--;
				mouse_bit_y = 0;
			}
			
			y_needs_update = 1;
		}

		last_mx = new_mx;
		last_my = new_my;
	}
	
	/* assert Port B External Interrupt on the SCC */
	macplus_scc_mouse_irq( x_needs_update, y_needs_update );
}

int macplus_vh_start( void ) {
	videoram_size = ( 512 * 384 / 8 );
	
	/* check for mouse changes at 16 irqs per frame */
	mouse_timer = timer_pulse( TIME_IN_HZ( 16*60 ), 0, mouse_callback );
	if (!mouse_timer)
		return 1;

	old_display = (UINT16 *) malloc(videoram_size);
	if (!old_display) {
		timer_remove( mouse_timer );
		return 1;
	}
	memset(old_display, 0, videoram_size);

	return 0;
}

void macplus_vh_stop( void ) {
	timer_remove( mouse_timer );
	free(old_display);
}

void macplus_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ) {
	UINT16	data;
	UINT16	*old;
	UINT8	*v;
	int		fg, bg, x, y;

	v = videoram;
	bg = Machine->pens[0];
	fg = Machine->pens[1];
	old = old_display;

	for (y = 0; y < 342; y++) {
		for ( x = 0; x < 32; x++ ) {
			data = READ_WORD( v );
			if (full_refresh || (data != *old)) {
				plot_pixel( bitmap, ( x << 4 ) + 0x00, y, ( data & 0x8000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x01, y, ( data & 0x4000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x02, y, ( data & 0x2000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x03, y, ( data & 0x1000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x04, y, ( data & 0x0800 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x05, y, ( data & 0x0400 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x06, y, ( data & 0x0200 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x07, y, ( data & 0x0100 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x08, y, ( data & 0x0080 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x09, y, ( data & 0x0040 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0a, y, ( data & 0x0020 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0b, y, ( data & 0x0010 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0c, y, ( data & 0x0008 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0d, y, ( data & 0x0004 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0e, y, ( data & 0x0002 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0f, y, ( data & 0x0001 ) ? fg : bg );
				*old = data;
			}
			v += 2;
			old++;
		}
	}
}

