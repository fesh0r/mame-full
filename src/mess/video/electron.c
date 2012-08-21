/******************************************************************************
    Acorn Electron driver

    MESS Driver By:

    Wilbert Pol

******************************************************************************/

#include "emu.h"
#include "includes/electron.h"

/*
  From the ElectrEm site:

Timing is somewhat of a thorny issue on the Electron. It is almost certain the Electron could have been a much faster machine if BBC Micro OS level compatibility had not been not a design requirement.

When accessing the ROM regions, the CPU always runs at 2MHz. When accessing the FC (1 MHz bus) or FD (JIM) pages, the CPU always runs at 1MHz.

The timing for RAM accesses varies depending on the graphics mode, and how many bytes are required to be read by the video circuits per scanline. When accessing RAM in modes 4-6, the CPU is simply moved to a 1MHz clock. This occurs for any RAM access at any point during the frame.

In modes 0-3, if the CPU tries to access RAM at any time during which the video circuits are fetching bytes, it is halted by means of receiving a stopped clock until the video circuits next stop fetching bytes.

Each scanline is drawn in exactly 64us, and of that the video circuits fetch bytes for 40us. In modes 0, 1 and 2, 256 scanlines have pixels on, whereas in mode 3 only 250 scanlines are affected as mode 3 is a 'spaced' mode.

As opposed to one clock generator which changes pace, the 1MHz and 2MHz clocks are always available, so the ULA acts to simply change which clock is piped to the CPU. This means in half of all cases, a further 2MHz cycle is lost waiting for the 2MHz and 1MHz clocks to synchronise during a 2MHz to 1MHz step.

The video circuits run from a constant 2MHz clock, and generate 312 scanlines a frame, one scanline every 128 cycles. This actually gives means the Electron is running at 50.08 frames a second.

Creating a scanline numbering scheme where the first scanline with pixels is scanline 0, in all modes the end of display interrupt is generated at the end of scanline 255, and the RTC interrupt is generated upon the end of scanline 99.

From investigating some code for vertical split modes printed in Electron User volume 7, issue 7 it seems that the exact timing of the end of display interrupt is somewhere between 24 and 40 cycles after the end of pixels. This may coincide with HSYNC. I have no similarly accurate timing for the real time clock interrupt at this time.

Mode changes are 'immediate', so any change in RAM access timing occurs exactly after the write cycle of the changing instruction. Similarly palette changes take effect immediately. VSYNC is not signalled in any way.

*/

static TIMER_CALLBACK( electron_scanline_interrupt );


VIDEO_START( electron )
{
	electron_state *state = machine.driver_data<electron_state>();
	int i;
	for( i = 0; i < 256; i++ ) {
		state->m_map4[i] = ( ( i & 0x10 ) >> 3 ) | ( i & 0x01 );
		state->m_map16[i] = ( ( i & 0x40 ) >> 3 ) | ( ( i & 0x10 ) >> 2 ) | ( ( i & 0x04 ) >> 1 ) | ( i & 0x01 );
	}
	state->m_scanline_timer = machine.scheduler().timer_alloc(FUNC(electron_scanline_interrupt));
	state->m_scanline_timer->adjust( machine.primary_screen->time_until_pos(0), 0, machine.primary_screen->scan_period() );
}

INLINE UINT8 read_vram( electron_state *state, UINT16 addr )
{
	return state->m_ula.vram[ addr % state->m_ula.screen_size ];
}

INLINE void electron_plot_pixel(bitmap_ind16 &bitmap, int x, int y, UINT32 color)
{
	bitmap.pix16(y, x) = (UINT16)color;
}

SCREEN_UPDATE_IND16( electron )
{
	electron_state *state = screen.machine().driver_data<electron_state>();
	int i;
	int x = 0;
	int pal[16];
	int scanline = screen.vpos();
	rectangle r = cliprect;
	r.min_y = r.max_y = scanline;

	/* set up palette */
	switch( state->m_ula.screen_mode )
	{
	case 0: case 3: case 4: case 6: case 7: /* 2 colour mode */
		pal[0] = state->m_ula.current_pal[0];
		pal[1] = state->m_ula.current_pal[8];
		break;
	case 1: case 5: /* 4 colour mode */
		pal[0] = state->m_ula.current_pal[0];
		pal[1] = state->m_ula.current_pal[2];
		pal[2] = state->m_ula.current_pal[8];
		pal[3] = state->m_ula.current_pal[10];
		break;
	case 2:	/* 16 colour mode */
		for( i = 0; i < 16; i++ )
			pal[i] = state->m_ula.current_pal[i];
	}

	/* draw line */
	switch( state->m_ula.screen_mode )
	{
	case 0:
		for( i = 0; i < 80; i++ )
		{
			UINT8 pattern = read_vram( state, state->m_ula.screen_addr + (i << 3) );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)& 1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)& 1] );
		}
		state->m_ula.screen_addr++;
		if ( ( scanline & 0x07 ) == 7 )
			state->m_ula.screen_addr += 0x278;
		break;

	case 1:
		for( i = 0; i < 80; i++ )
		{
			UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
		}
		state->m_ula.screen_addr++;
		if ( ( scanline & 0x07 ) == 7 )
			state->m_ula.screen_addr += 0x278;
		break;

	case 2:
		for( i = 0; i < 80; i++ )
		{
			UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map16[pattern>>0]] );
		}
		state->m_ula.screen_addr++;
		if ( ( scanline & 0x07 ) == 7 )
			state->m_ula.screen_addr += 0x278;
		break;

	case 3:
		if ( ( scanline > 249 ) || ( scanline % 10 >= 8 ) )
			bitmap.fill(7, r );
		else
		{
			for( i = 0; i < 80; i++ )
			{
				UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)&1] );
			}
			state->m_ula.screen_addr++;
		}
		if ( scanline % 10 == 9 )
			state->m_ula.screen_addr += 0x278;
		break;

	case 4:
	case 7:
		for( i = 0; i < 40; i++ )
		{
			UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)&1] );
			electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)&1] );
		}
		state->m_ula.screen_addr++;
		if ( ( scanline & 0x07 ) == 7 )
			state->m_ula.screen_addr += 0x138;
		break;

	case 5:
		for( i = 0; i < 40; i++ )
		{
			UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>3]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>2]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>1]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
			electron_plot_pixel( bitmap, x++, scanline, pal[state->m_map4[pattern>>0]] );
		}
		state->m_ula.screen_addr++;
		if ( ( scanline & 0x07 ) == 7 )
			state->m_ula.screen_addr += 0x138;
		break;

	case 6:
		if ( ( scanline > 249 ) || ( scanline % 10 >= 8 ) )
			bitmap.fill(7, r );
		else
		{
			for( i = 0; i < 40; i++ )
			{
				UINT8 pattern = read_vram( state, state->m_ula.screen_addr + i * 8 );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>7)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>6)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>5)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>4)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>3)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>2)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>1)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)&1] );
				electron_plot_pixel( bitmap, x++, scanline, pal[(pattern>>0)&1] );
			}
			state->m_ula.screen_addr++;
			if ( ( scanline % 10 ) == 7 )
				state->m_ula.screen_addr += 0x138;
		}
		break;
	}

	return 0;
}

static TIMER_CALLBACK( electron_scanline_interrupt )
{
	electron_state *state = machine.driver_data<electron_state>();
	switch (machine.primary_screen->vpos())
	{
	case 43:
		electron_interrupt_handler( machine, INT_SET, INT_RTC );
		break;
	case 199:
		electron_interrupt_handler( machine, INT_SET, INT_DISPLAY_END );
		break;
	case 0:
		state->m_ula.screen_addr = state->m_ula.screen_start - state->m_ula.screen_base;
		break;
	}
}
