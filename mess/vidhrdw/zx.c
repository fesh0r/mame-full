/***************************************************************************
	zx.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"

void *ula_nmi = NULL;
void *ula_irq = NULL;
int ula_nmi_active, ula_irq_active;
int ula_frame_vsync = 0;
int ula_scancode_count = 0;
int ula_scanline_count = 0;
int old_x = 0;
int old_y = 0;
int old_c = 0;

char zx_frame_message[128];
int zx_frame_time = 0;

/*
 * Toggle the video output between black and white.
 * This happens whenever the ULA scanline IRQs are enabled/disabled.
 * Normally this is done during the synchronized zx_ula_r() function,
 * which outputs 8 pixels per code, but if the video sync is off
 * (during tape IO or sound output) zx_ula_bkgnd() is used to
 * simulate the display of a ZX80/ZX81.
 */
void zx_ula_bkgnd(int color)
{
	if (ula_frame_vsync == 0 && color != old_c)
	{
		int y, new_x, new_y;
		struct rectangle r;
		struct mame_bitmap *bitmap = Machine->scrbitmap;

		new_y = cpu_getscanline();
		new_x = cpu_gethorzbeampos();
		logerror("zx_ula_bkgnd: %3d,%3d - %3d,%3d\n", old_x, old_y, new_x, new_y);
		y = old_y;
		for (;;)
		{
			if (y == new_y)
			{
				r.min_x = old_x;
				r.max_x = new_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, Machine->pens[color], &r);
				break;
			}
			else
			{
				r.min_x = old_x;
				r.max_x = Machine->visible_area.max_x;
				r.min_y = r.max_y = y;
				fillbitmap(bitmap, Machine->pens[color], &r);
				old_x = 0;
			}
			if (++y == Machine->drv->screen_height)
				y = 0;
		}
		old_x = (new_x + 1) % Machine->drv->screen_width;
		old_y = new_y;
		old_c = color;
		DAC_data_w(0, color ? 255 : 0);
	}
}

/*
 * PAL:  310 total lines,
 *			  0.. 55 vblank
 *			 56..247 192 visible lines
 *			248..303 vblank
 *			304...	 vsync
 * NTSC: 262 total lines
 *			  0.. 31 vblank
 *			 32..223 192 visible lines
 *			224..233 vblank
 */
static void zx_ula_nmi(int param)
{
	/*
	 * An NMI is issued on the ZX81 every 64us for the blanked
	 * scanlines at the top and bottom of the display.
	 */
	struct rectangle r = Machine->visible_area;

	r.min_y = r.max_y = cpu_getscanline();
	fillbitmap(Machine->scrbitmap, Machine->pens[1], &r);
	logerror("ULA %3d[%d] NMI, R:$%02X, $%04x\n", cpu_getscanline(), ula_scancode_count, (unsigned) activecpu_get_reg(Z80_R), activecpu_get_pc());
	cpu_set_nmi_line(0, PULSE_LINE);
	if (++ula_scanline_count == Machine->drv->screen_height)
		ula_scanline_count = 0;
}

static void zx_ula_irq(int param)
{
	/*
	 * An IRQ is issued on the ZX80/81 whenever the R registers
	 * bit 6 goes low. In MESS this IRQ timed from the first read
	 * from the copy of the DFILE in the upper 32K in zx_ula_r().
	 */
	logerror("ULA %3d[%d] IRQ, R:$%02X, $%04x\n", cpu_getscanline(), ula_scancode_count, (unsigned) activecpu_get_reg(Z80_R), (unsigned) activecpu_get_pc());
	ula_irq_active = 0;
	if (++ula_scancode_count == 8)
		ula_scancode_count = 0;
	cpu_set_irq_line(0, 0, PULSE_LINE);
	if (++ula_scanline_count == Machine->drv->screen_height)
		ula_scanline_count = 0;
}


int zx_ula_r(int offs, int region)
{
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	int x, y, chr, data, ireg, rreg, cycles, offs0 = offs, halted = 0;
	UINT8 *chrgen, *rom = memory_region(REGION_CPU1);

	ula_frame_vsync = 3;

	chrgen = memory_region(region);
	ireg = activecpu_get_reg(Z80_I) << 8;
	rreg = activecpu_get_reg(Z80_R);
	cycles = 4 * (64 - (rreg & 63));
#if 1
	y = cpu_getscanline();
#else
	y = ula_scanline_count;
#endif
	logerror("ULA %3d[%d] VID, R:$%02X, $%04x:", y, ula_scancode_count, rreg, offs & 0x7fff);

	timer_adjust(ula_irq, TIME_IN_CYCLES(cycles, 0), 0, 0);
	ula_irq_active = 1;

	for (x = 0; x < 256; x += 8)
	{
		chr = rom[offs & 0x7fff];
		if (!halted)
			logerror(" %02x", chr);
		if (chr & 0x40)
		{
			halted = 1;
			rom[offs] = chr;
			data = 0x00;
		}
		else
		{
			data = chrgen[ireg | ((chr & 0x3f) << 3) | ula_scancode_count];
			rom[offs] = 0x00;
			if (chr & 0x80)
				data ^= 0xff;
			offs++;
		}
		drawgfx(bitmap, Machine->gfx[0], data, 0, 0, 0, x, y, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	if (!halted)
		logerror(" %02x", rom[offs & 0x7fff]);
	logerror("\n");
	return rom[offs0];
}

VIDEO_START( zx )
{
	ula_irq = timer_alloc(zx_ula_irq);
	ula_nmi = timer_alloc(zx_ula_nmi);
	ula_irq_active = 0;
	return video_start_generic_bitmapped();
}

VIDEO_UPDATE( zx )
{
	int full_refresh = 1;

	/* decrement video synchronization counter */
	if (ula_frame_vsync)
	{
		if (!--ula_frame_vsync)
			full_refresh = 1;
	}

	if (zx_frame_time > 0)
	{
		ui_text(bitmap, zx_frame_message, 2, Machine->visible_area.max_y - Machine->visible_area.min_y - 9);
		zx_frame_time--;
	}
}
