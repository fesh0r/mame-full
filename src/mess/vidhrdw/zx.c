/***************************************************************************
	zx.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

unsigned char *ROM;

void *ula_nmi = NULL;
void *ula_irq = NULL;
int ula_frame_vsync = 0;
int ula_scan_counter = 0;
int old_x = 0;
int old_y = 0;
int old_c = 0;

char zx_frame_message[128];
int zx_frame_time = 0;

/* Toggle the 'background' color between black and white.
 * This happens whenever the ULA scanline IRQs are enabled/disabled.
 * Normally this done during the synchronized zx_ula_r() function,
 * which outputs 8 pixels per code, but if the video sync is off,
 * during tape IO and/or sound output, zx_ula_bkgnd() is used to
 * simulate the display of a ZX80/ZX81.
 */
void zx_ula_bkgnd(int color)
{
	if( !ula_frame_vsync && color != old_c )
	{
		int y, new_x, new_y;
		struct rectangle r;

        new_y = cpu_getscanline();
		new_x = cpu_gethorzbeampos();
		LOG((errorlog, "zx_ula_bkgnd: %3d,%3d - %3d,%3d\n", old_x, old_y, new_x, new_y));
		y = old_y;
		for( ; ; )
		{
			if( y == new_y )
			{
				r.min_x = old_x;
				r.max_x = new_x;
				r.min_y = r.max_y = y;
				fillbitmap(tmpbitmap, Machine->pens[color], &r);
				break;
			}
			else
			{
				r.min_x = old_x;
				r.max_x = Machine->drv->visible_area.max_x;
				r.min_y = r.max_y = y;
				fillbitmap(tmpbitmap, Machine->pens[color], &r);
				old_x = 0;
			}
			if( ++y == Machine->drv->screen_height )
				y = 0;
		}
		old_x = (new_x + 1) % Machine->drv->screen_width;
		old_y = new_y;
		old_c = color;
		if( color )
			DAC_signed_data_w(0, +127);
		else
			DAC_signed_data_w(0, -128);
    }
}

/* An NMI is issued on the ZX81 every 64us for the blanked
 * scanlines at the top and bottom of the display.
 * PAL:  310 total lines,
 *			  0.. 55 vblank
 *			 56..247 192 visible lines
 *			248..303 vblank
 *			304...	 vsync
 * NTSC: 261 total lines
 *			  0.. 31 vblank
 *			 32..223 192 visible lines
 *			224..233 vblank
 */
void zx_ula_nmi(int param)
{
	if( ++ula_scan_counter == 8 )
        ula_scan_counter = 0;
    cpu_set_nmi_line(0, PULSE_LINE);
}

/* An IRQ is issued on the ZX80/81 whenever the R registers
 * bit 5 goes low. In MESS this IRQ timed from the first read
 * from the copy of the DFILE in the upper 32K in zx_ula_r().
 */
void zx_ula_irq(int param)
{
	if( ++ula_scan_counter == 8 )
		ula_scan_counter = 0;
    ula_irq = NULL;
	cpu_set_irq_line(0, 0, PULSE_LINE);
}

int zx_ula_r(int offs, int region)
{
	int x, y, chr, data, rreg, ireg, intr, offs0 = offs, halted = 0;
	UINT8 *chrgen;

	ula_frame_vsync = 3;

	/* IRQ for this scanline already executed? */
    if( ula_irq )
		return ROM[offs0];

	chrgen = memory_region(region);
	ireg = cpu_get_reg(Z80_I) << 8;

	rreg = cpu_get_reg(Z80_R);
	intr = 4 * (64 - (rreg & 63)) - 1;

	y = cpu_getscanline();

	LOG((errorlog, "ULA %3d, IRQ after %3d cycles (R:$%02X), $%04x:", y, intr, rreg, offs&0x7fff));
	ula_irq = timer_set( TIME_IN_CYCLES(intr,0), 0, zx_ula_irq);

    for( x = 0; x < 256; x += 8 )
	{
		chr = ROM[offs&0x7fff];
		if( !halted ) LOG((errorlog, " %02x", chr));
		if( chr & 0x40 )
		{
			halted = 1;
			ROM[offs] = chr;
			data = 0x00;
		}
		else
		{
			data = chrgen[ireg | ((chr & 0x3f) << 3) | ula_scan_counter];
			ROM[offs] = 0x00;
			if( chr & 0x80 )
				data ^= 0xff;
			offs++;
		}
		drawgfx(tmpbitmap, Machine->gfx[0], data, 0, 0,0, x,y, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
    }
	if( !halted ) LOG((errorlog, " %02x", ROM[offs&0x7fff]));
    LOG((errorlog, "\n"));
	return ROM[offs0];
}

void zx_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* decrement video synchronization counter */
	if( ula_frame_vsync )
		ula_frame_vsync--;

    copybitmap(bitmap, tmpbitmap, 0,0, 0,0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
	fillbitmap(tmpbitmap, Machine->pens[1], &Machine->drv->visible_area);

    if( zx_frame_time > 0 )
    {
		ui_text(zx_frame_message, 2, Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y - 9);
		zx_frame_time--;
    }
}



