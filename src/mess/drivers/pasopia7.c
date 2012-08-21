/**************************************************************************************************

    Toshiba Pasopia 7 (c) 1983 Toshiba

    preliminary driver by Angelo Salese

    TODO:
    - Z80PIO keyboard irqs doesn't work at all, kludged keyboard inputs to work for now
    - floppy support (but floppy images are unobtainable at current time)
    - cassette device;
    - beeper
    - LCD version has gfx bugs, it must use a different ROM charset for instance (apparently a 8 x 4
      one, 40/80 x 8 tilemap);

***************************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/i8255.h"
#include "machine/z80pio.h"
#include "machine/upd765.h"
#include "sound/sn76496.h"
#include "video/mc6845.h"
#include "rendlay.h"

class pasopia7_state : public driver_device
{
public:
	pasopia7_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_ppi0(*this, "ppi8255_0"),
	m_ppi1(*this, "ppi8255_1"),
	m_ppi2(*this, "ppi8255_2"),
	m_ctc(*this, "z80ctc"),
	m_pio(*this, "z80pio"),
	m_crtc(*this, "crtc"),
	m_fdc(*this, "fdc"),
	m_sn1(*this, "sn1"),
	m_sn2(*this, "sn2")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<i8255_device> m_ppi0;
	required_device<i8255_device> m_ppi1;
	required_device<i8255_device> m_ppi2;
	required_device<z80ctc_device> m_ctc;
	required_device<z80pio_device> m_pio;
	required_device<mc6845_device> m_crtc;
	required_device<device_t> m_fdc;
	required_device<device_t> m_sn1;
	required_device<device_t> m_sn2;
	DECLARE_READ8_MEMBER(vram_r);
	DECLARE_WRITE8_MEMBER(vram_w);
	DECLARE_WRITE8_MEMBER(pasopia7_memory_ctrl_w);
	DECLARE_READ8_MEMBER(fdc_r);
	DECLARE_WRITE8_MEMBER(pac2_w);
	DECLARE_READ8_MEMBER(pac2_r);
	DECLARE_WRITE8_MEMBER(ram_bank_w);
	DECLARE_WRITE8_MEMBER(pasopia7_6845_w);
	DECLARE_READ8_MEMBER(pasopia7_fdc_r);
	DECLARE_WRITE8_MEMBER(pasopia7_fdc_w);
	DECLARE_READ8_MEMBER(pasopia7_io_r);
	DECLARE_WRITE8_MEMBER(pasopia7_io_w);
	DECLARE_READ8_MEMBER(test_r);
	DECLARE_WRITE_LINE_MEMBER(testa_w);
	DECLARE_WRITE_LINE_MEMBER(testb_w);
	DECLARE_READ8_MEMBER(crtc_portb_r);
	DECLARE_WRITE8_MEMBER(screen_mode_w);
	DECLARE_WRITE8_MEMBER(plane_reg_w);
	DECLARE_WRITE8_MEMBER(video_attr_w);
	DECLARE_WRITE8_MEMBER(video_misc_w);
	DECLARE_WRITE8_MEMBER(nmi_mask_w);
	DECLARE_READ8_MEMBER(unk_r);
	DECLARE_READ8_MEMBER(nmi_reg_r);
	DECLARE_WRITE8_MEMBER(nmi_reg_w);
	DECLARE_READ8_MEMBER(nmi_porta_r);
	DECLARE_READ8_MEMBER(nmi_portb_r);
	UINT8 m_vram_sel;
	UINT8 m_mio_sel;
	UINT8 *m_p7_pal;
	UINT8 m_bank_reg;
	UINT16 m_cursor_addr;
	UINT8 m_cursor_blink;
	UINT8 m_cursor_raster;
	UINT8 m_plane_reg;
	UINT8 m_attr_data;
	UINT8 m_attr_wrap;
	UINT8 m_attr_latch;
	UINT8 m_pal_sel;
	UINT8 m_x_width;
	UINT8 m_gfx_mode;
	UINT8 m_nmi_mask;
	UINT8 m_nmi_enable_reg;
	UINT8 m_nmi_trap;
	UINT8 m_nmi_reset;
	UINT16 m_pac2_index[2];
	UINT32 m_kanji_index;
	UINT8 m_pac2_bank_select;
	UINT8 m_screen_type;
	int m_addr_latch;
	void pasopia_nmi_trap();
	DECLARE_DRIVER_INIT(p7_lcd);
	DECLARE_DRIVER_INIT(p7_raster);
};

#define VDP_CLOCK XTAL_3_579545MHz/4
#define LCD_CLOCK VDP_CLOCK/10

static VIDEO_START( pasopia7 )
{
	pasopia7_state *state = machine.driver_data<pasopia7_state>();
	state->m_p7_pal = auto_alloc_array(machine, UINT8, 0x10);
}

#define keyb_press(_val_,_charset_) \
	if(machine.input().code_pressed(_val_)) \
	{ \
		ram_space->write_byte(0xfda4,0x01); \
		ram_space->write_byte(0xfce1,_charset_); \
	} \

#define keyb_shift_press(_val_,_charset_) \
	if(machine.input().code_pressed(_val_) && machine.input().code_pressed(KEYCODE_LSHIFT)) \
	{ \
		ram_space->write_byte(0xfda4,0x01); \
		ram_space->write_byte(0xfce1,_charset_); \
	} \

/* cheap kludge to use the keyboard without going nuts with the debugger ... */
static void fake_keyboard_data(running_machine &machine)
{
	address_space *ram_space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	ram_space->write_byte(0xfda4,0x00); //clear flag

	keyb_press(KEYCODE_Z, 'z');
	keyb_press(KEYCODE_X, 'x');
	keyb_press(KEYCODE_C, 'c');
	keyb_press(KEYCODE_V, 'v');
	keyb_press(KEYCODE_B, 'b');
	keyb_press(KEYCODE_N, 'n');
	keyb_press(KEYCODE_M, 'm');

	keyb_press(KEYCODE_A, 'a');
	keyb_press(KEYCODE_S, 's');
	keyb_press(KEYCODE_D, 'd');
	keyb_press(KEYCODE_F, 'f');
	keyb_press(KEYCODE_G, 'g');
	keyb_press(KEYCODE_H, 'h');
	keyb_press(KEYCODE_J, 'j');
	keyb_press(KEYCODE_K, 'k');
	keyb_press(KEYCODE_L, 'l');

	keyb_press(KEYCODE_Q, 'q');
	keyb_press(KEYCODE_W, 'w');
	keyb_press(KEYCODE_E, 'e');
	keyb_press(KEYCODE_R, 'r');
	keyb_press(KEYCODE_T, 't');
	keyb_press(KEYCODE_Y, 'y');
	keyb_press(KEYCODE_U, 'u');
	keyb_press(KEYCODE_I, 'i');
	keyb_press(KEYCODE_O, 'o');
	keyb_press(KEYCODE_P, 'p');

	keyb_press(KEYCODE_0, '0');
	keyb_press(KEYCODE_1, '1');
	keyb_press(KEYCODE_2, '2');
	keyb_press(KEYCODE_3, '3');
	keyb_press(KEYCODE_4, '4');
	keyb_press(KEYCODE_5, '5');
	keyb_press(KEYCODE_6, '6');
	keyb_press(KEYCODE_7, '7');
	keyb_press(KEYCODE_8, '8');
	keyb_press(KEYCODE_9, '9');

	keyb_shift_press(KEYCODE_0, '=');
	keyb_shift_press(KEYCODE_1, '!');
	keyb_shift_press(KEYCODE_2, '"');
	keyb_shift_press(KEYCODE_3, '?');
	keyb_shift_press(KEYCODE_4, '$');
	keyb_shift_press(KEYCODE_5, '%');
	keyb_shift_press(KEYCODE_6, '&');
	keyb_shift_press(KEYCODE_7, '/');
	keyb_shift_press(KEYCODE_8, '(');
	keyb_shift_press(KEYCODE_9, ')');

	keyb_press(KEYCODE_ENTER, 0x0d);
	keyb_press(KEYCODE_SPACE, ' ');
	keyb_press(KEYCODE_STOP, '.');
	keyb_shift_press(KEYCODE_STOP, ':');
	keyb_press(KEYCODE_BACKSPACE, 0x08);
	keyb_press(KEYCODE_0_PAD, '@'); //@
	keyb_press(KEYCODE_COMMA, ',');
	keyb_shift_press(KEYCODE_COMMA, ';');
	keyb_press(KEYCODE_MINUS_PAD, '-');
	keyb_press(KEYCODE_PLUS_PAD, '+');
	keyb_press(KEYCODE_ASTERISK, '*');
}

static void draw_cg4_screen(running_machine &machine, bitmap_ind16 &bitmap,const rectangle &cliprect,int width)
{
	UINT8 *vram = machine.root_device().memregion("vram")->base();
	int x,y,xi,yi;
	int count;

	for(yi=0;yi<8;yi++)
	{
		count = yi;
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<8*width;x+=8)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen_b,pen_r,pen_g,color;

					pen_b = (vram[count+0x0000]>>(7-xi)) & 1;
					pen_r = (vram[count+0x4000]>>(7-xi)) & 1;
					pen_g = 0;//(p7_vram[count+0x8000]>>(7-xi)) & 1;

					color =  pen_g<<2 | pen_r<<1 | pen_b<<0;

					bitmap.pix16(y+yi, x+xi) = machine.pens[color];
				}
				count+=8;
			}
		}
	}
}

static void draw_tv_screen(running_machine &machine, bitmap_ind16 &bitmap,const rectangle &cliprect,int width)
{
	pasopia7_state *state = machine.driver_data<pasopia7_state>();
	UINT8 *vram = machine.root_device().memregion("vram")->base();
	UINT8 *gfx_data = state->memregion("font")->base();
	int x,y,xi,yi;
	int count;

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<width;x++)
		{
			int tile = vram[count+0x8000];
			int attr = vram[count+0xc000];
			int color = attr & 7;

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;
					pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? color : 0;

					bitmap.pix16(y*8+yi, x*8+xi) = machine.pens[pen];
				}
			}

			// draw cursor
			if(state->m_cursor_addr*8 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(state->m_cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine.primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine.primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(state->m_cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							bitmap.pix16(y*8-yc+7, x*8+xc) = machine.pens[7];
						}
					}
				}
			}
			count+=8;
		}
	}
}

static void draw_mixed_screen(running_machine &machine, bitmap_ind16 &bitmap,const rectangle &cliprect,int width)
{
	pasopia7_state *state = machine.driver_data<pasopia7_state>();
	UINT8 *vram = machine.root_device().memregion("vram")->base();
	UINT8 *gfx_data = state->memregion("font")->base();
	int x,y,xi,yi;
	int count;

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<width;x++)
		{
			int tile = vram[count+0x8000];

			for(yi=0;yi<8;yi++)
			{
				int attr = vram[count+0xc000+yi];

				if(attr & 0x80)
				{
					for(xi=0;xi<8;xi++)
					{
						int pen,pen_b,pen_r,pen_g;

						pen_b = (vram[count+yi+0x0000]>>(7-xi)) & 1;
						pen_r = (vram[count+yi+0x4000]>>(7-xi)) & 1;
						pen_g = (vram[count+yi+0x8000]>>(7-xi)) & 1;

						pen =  pen_g<<2 | pen_r<<1 | pen_b<<0;

						bitmap.pix16(y*8+yi, x*8+xi) = machine.pens[pen];
					}
				}
				else
				{
					int color = attr & 7;

					for(xi=0;xi<8;xi++)
					{
						int pen;
						pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? color : 0;

						bitmap.pix16(y*8+yi, x*8+xi) = machine.pens[pen];
					}
				}
			}

			// draw cursor
			if(state->m_cursor_addr*8 == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(state->m_cursor_raster & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine.primary_screen->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine.primary_screen->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(state->m_cursor_raster & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							bitmap.pix16(y*8-yc+7, x*8+xc) = machine.pens[7];
						}
					}
				}
			}

			count+=8;
		}
	}
}

static SCREEN_UPDATE_IND16( pasopia7 )
{
	pasopia7_state *state = screen.machine().driver_data<pasopia7_state>();
	int width;

	bitmap.fill(screen.machine().pens[0], cliprect);

	fake_keyboard_data(screen.machine());

	width = state->m_x_width ? 80 : 40;

	if(state->m_gfx_mode)
		draw_mixed_screen(screen.machine(),bitmap,cliprect,width);
	else
	{
		draw_cg4_screen(screen.machine(),bitmap,cliprect,width);
		draw_tv_screen(screen.machine(),bitmap,cliprect,width);
	}

	return 0;
}

READ8_MEMBER( pasopia7_state::vram_r )
{
	UINT8 *vram = memregion("vram")->base();
	UINT8 res;

	if (m_vram_sel == 0)
	{
		UINT8 *work_ram = memregion("maincpu")->base();

		return work_ram[offset+0x8000];
	}

	if (m_pal_sel && (m_plane_reg & 0x70) == 0x00)
		return m_p7_pal[offset & 0xf];

	res = 0xff;

	if ((m_plane_reg & 0x11) == 0x11)
		res &= vram[offset | 0x0000];
	if ((m_plane_reg & 0x22) == 0x22)
		res &= vram[offset | 0x4000];
	if ((m_plane_reg & 0x44) == 0x44)
	{
		res &= vram[offset | 0x8000];
		m_attr_latch = vram[offset | 0xc000] & 0x87;
	}

	return res;
}

WRITE8_MEMBER( pasopia7_state::vram_w )
{
	UINT8 *vram = memregion("vram")->base();

	if (m_vram_sel)
	{
		if (m_pal_sel && (m_plane_reg & 0x70) == 0x00)
		{
			m_p7_pal[offset & 0xf] = data & 0xf;
			return;
		}

		if (m_plane_reg & 0x10)
			vram[(offset & 0x3fff) | 0x0000] = (m_plane_reg & 1) ? data : 0xff;
		if (m_plane_reg & 0x20)
			vram[(offset & 0x3fff) | 0x4000] = (m_plane_reg & 2) ? data : 0xff;
		if (m_plane_reg & 0x40)
		{
			vram[(offset & 0x3fff) | 0x8000] = (m_plane_reg & 4) ? data : 0xff;
			m_attr_latch = m_attr_wrap ? m_attr_latch : m_attr_data;
			vram[(offset & 0x3fff) | 0xc000] = m_attr_latch;
		}
	}
	else
	{
		UINT8 *work_ram = memregion("maincpu")->base();

		work_ram[offset+0x8000] = data;
	}
}

WRITE8_MEMBER( pasopia7_state::pasopia7_memory_ctrl_w )
{
	UINT8 *work_ram = memregion("maincpu")->base();
	UINT8 *basic = memregion("basic")->base();

	switch(data & 3)
	{
		case 0:
		case 3: //select Basic ROM
			membank("bank1")->set_base(basic    + 0x00000);
			membank("bank2")->set_base(basic    + 0x04000);
			break;
		case 1: //select Basic ROM + BIOS ROM
			membank("bank1")->set_base(basic    + 0x00000);
			membank("bank2")->set_base(work_ram + 0x10000);
			break;
		case 2: //select Work RAM
			membank("bank1")->set_base(work_ram + 0x00000);
			membank("bank2")->set_base(work_ram + 0x04000);
			break;
	}

	m_bank_reg = data & 3;
	m_vram_sel = data & 4;
	m_mio_sel = data & 8;

	// bank4 is always RAM

//  printf("%02x\n",m_vram_sel);
}

#if 0
READ8_MEMBER( pasopia7_state::fdc_r )
{
	return machine().rand();
}
#endif


WRITE8_MEMBER( pasopia7_state::pac2_w )
{
    /*
    select register:
    4 = ram1;
    3 = ram2;
    2 = kanji ROM;
    1 = joy;
    anything else is nop
    */

	if(m_pac2_bank_select == 3 || m_pac2_bank_select == 4)
	{
		switch(offset)
		{
			case 0:	m_pac2_index[(m_pac2_bank_select-3) & 1] = (m_pac2_index[(m_pac2_bank_select-3) & 1] & 0x7f00) | (data & 0xff); break;
			case 1: m_pac2_index[(m_pac2_bank_select-3) & 1] = (m_pac2_index[(m_pac2_bank_select-3) & 1] & 0xff) | ((data & 0x7f) << 8); break;
			case 2: // PAC2 RAM write
			{
				UINT8 *pac2_ram;

				pac2_ram = memregion(((m_pac2_bank_select-3) & 1) ? "rampac2" : "rampac1")->base();

				pac2_ram[m_pac2_index[(m_pac2_bank_select-3) & 1]] = data;
			}
		}
	}
	else if(m_pac2_bank_select == 2) // kanji ROM
	{
		switch(offset)
		{
			case 0: m_kanji_index = (m_kanji_index & 0x1ff00) | ((data & 0xff) << 0); break;
			case 1: m_kanji_index = (m_kanji_index & 0x100ff) | ((data & 0xff) << 8); break;
			case 2: m_kanji_index = (m_kanji_index & 0x0ffff) | ((data & 0x01) << 16); break;
		}
	}

	if(offset == 3)
	{
		if(data & 0x80)
		{
			// ...
		}
		else
			m_pac2_bank_select = data & 7;
	}
}

READ8_MEMBER( pasopia7_state::pac2_r )
{
	if(offset == 2)
	{
		if(m_pac2_bank_select == 3 || m_pac2_bank_select == 4)
		{
			UINT8 *pac2_ram;

			pac2_ram = memregion(((m_pac2_bank_select-3) & 1) ? "rampac2" : "rampac1")->base();

			return pac2_ram[m_pac2_index[(m_pac2_bank_select-3) & 1]];
		}
		else if(m_pac2_bank_select == 2)
		{
			UINT8 *kanji_rom = memregion("kanji")->base();

			return kanji_rom[m_kanji_index];
		}
		else
		{
			printf("%02x\n",m_pac2_bank_select);
		}
	}

	return 0xff;
}

/* writes always occurs to the RAM banks, even if the ROMs are selected. */
WRITE8_MEMBER( pasopia7_state::ram_bank_w )
{
	UINT8 *work_ram = memregion("maincpu")->base();

	work_ram[offset] = data;
}

WRITE8_MEMBER( pasopia7_state::pasopia7_6845_w )
{
	if(offset == 0)
	{
		m_addr_latch = data;
		m_crtc->address_w(space, offset, data);
	}
	else
	{
		/* FIXME: this should be inside the MC6845 core! */
		if(m_addr_latch == 0x0a)
			m_cursor_raster = data;
		if(m_addr_latch == 0x0e)
			m_cursor_addr = ((data<<8) & 0x3f00) | (m_cursor_addr & 0xff);
		else if(m_addr_latch == 0x0f)
			m_cursor_addr = (m_cursor_addr & 0x3f00) | (data & 0xff);

		m_crtc->register_w(space, offset, data);

		/* double pump the pixel clock if we are in 640 x 200 mode */
		if(m_screen_type == 1) // raster
			m_crtc->set_clock( (m_x_width) ? VDP_CLOCK*2 : VDP_CLOCK);
		else // lcd
			m_crtc->set_clock( (m_x_width) ? LCD_CLOCK*2 : LCD_CLOCK);
	}
}

void pasopia7_state::pasopia_nmi_trap()
{
	if(m_nmi_enable_reg)
	{
		m_nmi_trap |= 2;

		if(m_nmi_mask == 0)
			cputag_set_input_line(machine(), "maincpu", INPUT_LINE_NMI, ASSERT_LINE);
	}
}

READ8_MEMBER( pasopia7_state::pasopia7_fdc_r )
{
	switch(offset)
	{
		case 4: return upd765_status_r(m_fdc, 0);
		case 5: return upd765_data_r(m_fdc, 0);
		//case 6: bit 7 interrupt bit
	}

	return 0xff;
}

WRITE8_MEMBER( pasopia7_state::pasopia7_fdc_w )
{
	switch(offset)
	{
		case 0: upd765_tc_w(m_fdc, 0); break;
		case 2: upd765_tc_w(m_fdc, 1); break;
		case 5: upd765_data_w(m_fdc, 0, data); break;
		case 6:
			upd765_reset_w(m_fdc, data & 0x80);
			floppy_mon_w(floppy_get_device(machine(), 0), (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
			floppy_drive_set_ready_state(floppy_get_device(machine(), 0), (data & 0x40), 0);
			break;
	}
}


READ8_MEMBER( pasopia7_state::pasopia7_io_r )
{
	UINT16 io_port;

	if(m_mio_sel)
	{
		address_space *ram_space = m_maincpu->memory().space(AS_PROGRAM);

		m_mio_sel = 0;
		//printf("%08x\n",offset);
		//return 0x0d; // hack: this is used for reading the keyboard data, we can fake it a little ... (modify fda4)
		return ram_space->read_byte(offset);
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x30 && io_port <= 0x33)
		printf("[%02x]\n",offset & 3);

	if(io_port >= 0x08 && io_port <= 0x0b)
		return m_ppi0->read(space, io_port & 3);
	else
	if(io_port >= 0x0c && io_port <= 0x0f)
		return m_ppi1->read(space, io_port & 3);
//  else if(io_port == 0x10 || io_port == 0x11) { M6845 read }
	else
	if(io_port >= 0x18 && io_port <= 0x1b)
		return pac2_r(space, io_port & 3);
	else
	if(io_port >= 0x20 && io_port <= 0x23)
	{
		pasopia_nmi_trap();
		return m_ppi2->read(space, io_port & 3);
	}
	else
	if(io_port >= 0x28 && io_port <= 0x2b)
		return m_ctc->read(space,io_port & 3);
	else
	if(io_port >= 0x30 && io_port <= 0x33)
		return m_pio->read_alt(space, io_port & 3);
//  else if(io_port == 0x3a)                    { SN1 }
//  else if(io_port == 0x3b)                    { SN2 }
//  else if(io_port == 0x3c)                    { bankswitch }
	else
	if(io_port >= 0xe0 && io_port <= 0xe6)
		return pasopia7_fdc_r(space, offset & 7);
	else
	{
		logerror("(PC=%06x) Read i/o address %02x\n",cpu_get_pc(m_maincpu),io_port);
	}

	return 0xff;
}

WRITE8_MEMBER( pasopia7_state::pasopia7_io_w )
{
	UINT16 io_port;

	if(m_mio_sel)
	{
		address_space *ram_space = m_maincpu->memory().space(AS_PROGRAM);
		m_mio_sel = 0;
		ram_space->write_byte(offset, data);
		return;
	}

	io_port = offset & 0xff; //trim down to 8-bit bus

	if(io_port >= 0x30 && io_port <= 0x33)
		printf("[%02x] <- %02x\n",offset & 3,data);

	if(io_port >= 0x08 && io_port <= 0x0b)
		m_ppi0->write(space, io_port & 3, data);
	else
	if(io_port >= 0x0c && io_port <= 0x0f)
		m_ppi1->write(space, io_port & 3, data);
	else
	if(io_port >= 0x10 && io_port <= 0x11)
		pasopia7_6845_w(space, io_port-0x10, data);
	else
	if(io_port >= 0x18 && io_port <= 0x1b)
		pac2_w(space, io_port & 3, data);
	else
	if(io_port >= 0x20 && io_port <= 0x23)
	{
		m_ppi2->write(space, io_port & 3, data);
		pasopia_nmi_trap();
	}
	else
	if(io_port >= 0x28 && io_port <= 0x2b)
		m_ctc->write(space, io_port & 3, data);
	else
	if(io_port >= 0x30 && io_port <= 0x33)
		m_pio->write_alt(space, io_port & 3, data);
	else
	if(io_port == 0x3a)
		sn76496_w(m_sn1, 0, data);
	else
	if(io_port == 0x3b)
		sn76496_w(m_sn2, 0, data);
	else
	if(io_port == 0x3c)
		pasopia7_memory_ctrl_w(space,0, data);
	else
	if(io_port >= 0xe0 && io_port <= 0xe6)
		pasopia7_fdc_w(space, offset & 7, data);
	else
	{
		logerror("(PC=%06x) Write i/o address %02x = %02x\n",cpu_get_pc(m_maincpu),offset,data);
	}
}

static ADDRESS_MAP_START(pasopia7_mem, AS_PROGRAM, 8, pasopia7_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_WRITE( ram_bank_w )
	AM_RANGE( 0x0000, 0x3fff ) AM_ROMBANK("bank1")
	AM_RANGE( 0x4000, 0x7fff ) AM_ROMBANK("bank2")
	AM_RANGE( 0x8000, 0xbfff ) AM_READWRITE(vram_r, vram_w )
	AM_RANGE( 0xc000, 0xffff ) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START(pasopia7_io, AS_IO, 8, pasopia7_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff) AM_READWRITE( pasopia7_io_r, pasopia7_io_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pasopia7 )
INPUT_PORTS_END

static const gfx_layout p7_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout p7_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static GFXDECODE_START( pasopia7 )
	GFXDECODE_ENTRY( "font",   0x00000, p7_chars_8x8,    0, 0x10 )
	GFXDECODE_ENTRY( "kanji",  0x00000, p7_chars_16x16,  0, 0x10 )
GFXDECODE_END

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static Z80CTC_INTERFACE( z80ctc_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),		// interrupt handler
	DEVCB_DEVICE_LINE_MEMBER("z80ctc", z80ctc_device, trg1),		// ZC/TO0 callback
	DEVCB_DEVICE_LINE_MEMBER("z80ctc", z80ctc_device, trg2),		// ZC/TO1 callback, beep interface
	DEVCB_DEVICE_LINE_MEMBER("z80ctc", z80ctc_device, trg3)		// ZC/TO2 callback
};

READ8_MEMBER( pasopia7_state::test_r )
{
	return machine().rand();
}

WRITE_LINE_MEMBER( pasopia7_state::testa_w )
{
	printf("A %02x\n",state);
}

WRITE_LINE_MEMBER( pasopia7_state::testb_w )
{
	printf("B %02x\n",state);
}

static Z80PIO_INTERFACE( z80pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), //doesn't work?
	DEVCB_DRIVER_MEMBER(pasopia7_state, test_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pasopia7_state, testa_w),
	DEVCB_DRIVER_MEMBER(pasopia7_state, test_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pasopia7_state, testb_w)
};

static const z80_daisy_config p7_daisy[] =
{
	{ "z80ctc" },
	{ "z80pio" },
//  { "fdc" }, /* TODO */
	{ NULL }
};

READ8_MEMBER( pasopia7_state::crtc_portb_r )
{
	// --x- ---- vsync bit
	// ---x ---- hardcoded bit, defines if the system screen is raster (1) or LCD (0)
	// ---- x--- disp bit
	UINT8 vdisp = (machine().primary_screen->vpos() < (m_screen_type ? 200 : 28)) ? 0x08 : 0x00; //TODO: check LCD vpos trigger
	UINT8 vsync = vdisp ? 0x00 : 0x20;

	return 0x40 | (m_attr_latch & 0x87) | vsync | vdisp | (m_screen_type << 4);
}

WRITE8_MEMBER( pasopia7_state::screen_mode_w )
{
	if(data & 0x5f)
		printf("GFX MODE %02x\n",data);

	m_x_width = data & 0x20;
	m_gfx_mode = data & 0x80;

//  printf("%02x\n",m_gfx_mode);
}

WRITE8_MEMBER( pasopia7_state::plane_reg_w )
{
	//if(data & 0x11)
	//printf("PLANE %02x\n",data);
	m_plane_reg = data;
}

WRITE8_MEMBER( pasopia7_state::video_attr_w )
{
	//printf("VIDEO ATTR %02x | TEXT_PAGE %02x\n",data & 0xf,data & 0x70);
	m_attr_data = (data & 0x7) | ((data & 0x8)<<4);
}

//#include "debugger.h"

WRITE8_MEMBER( pasopia7_state::video_misc_w )
{
	/*
        --x- ---- blinking
        ---x ---- attribute wrap
        ---- x--- pal disable
        ---- xx-- palette selector (both bits enables this, odd hook-up)
    */
//  if(data & 2)
//  {
//    printf("VIDEO MISC %02x\n",data);
//    debugger_break(device->machine());
//  }
	m_cursor_blink = data & 0x20;
	m_attr_wrap = data & 0x10;
//  m_pal_sel = data & 0x02;
}

WRITE8_MEMBER( pasopia7_state::nmi_mask_w )
{
	/*
    --x- ---- (related to the data rec)
    ---x ---- data rec out
    ---- --x- sound off
    ---- ---x reset NMI & trap
    */
//  printf("SYSTEM MISC %02x\n",data);

	if(data & 1)
	{
		m_nmi_reset &= ~4;
		m_nmi_trap &= ~2;
		//cputag_set_input_line(machine(), "maincpu", INPUT_LINE_NMI, CLEAR_LINE); //guess
	}

}

/* TODO: investigate on these. */
READ8_MEMBER( pasopia7_state::unk_r )
{
	return 0xff;//machine().rand();
}

READ8_MEMBER( pasopia7_state::nmi_reg_r )
{
	//printf("C\n");
	return 0xfc | m_bank_reg;//machine().rand();
}

WRITE8_MEMBER( pasopia7_state::nmi_reg_w )
{
	/*
        x--- ---- NMI mask
        -x-- ---- NMI enable trap on PPI8255 2 r/w
    */
	m_nmi_mask = data & 0x80;
	m_nmi_enable_reg = data & 0x40;
}

READ8_MEMBER( pasopia7_state::nmi_porta_r )
{
	return 0xff;
}

READ8_MEMBER( pasopia7_state::nmi_portb_r )
{
	return 0xf9 | m_nmi_trap | m_nmi_reset;
}

static I8255_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_DRIVER_MEMBER(pasopia7_state, unk_r),			/* Port A read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, screen_mode_w),	/* Port A write */
	DEVCB_DRIVER_MEMBER(pasopia7_state, crtc_portb_r),	/* Port B read */
	DEVCB_NULL,						/* Port B write */
	DEVCB_NULL,						/* Port C read */
	DEVCB_NULL						/* Port C write */
};

static I8255_INTERFACE( ppi8255_intf_1 )
{
	DEVCB_NULL,						/* Port A read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, plane_reg_w),		/* Port A write */
	DEVCB_NULL,						/* Port B read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, video_attr_w),	/* Port B write */
	DEVCB_NULL,						/* Port C read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, video_misc_w)		/* Port C write */
};

static I8255_INTERFACE( ppi8255_intf_2 )
{
	DEVCB_DRIVER_MEMBER(pasopia7_state, nmi_porta_r),		/* Port A read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, nmi_mask_w),		/* Port A write */
	DEVCB_DRIVER_MEMBER(pasopia7_state, nmi_portb_r),		/* Port B read */
	DEVCB_NULL,						/* Port B write */
	DEVCB_DRIVER_MEMBER(pasopia7_state, nmi_reg_r),		/* Port C read */
	DEVCB_DRIVER_MEMBER(pasopia7_state, nmi_reg_w)		/* Port C write */
};

static MACHINE_RESET( pasopia7 )
{
	pasopia7_state *state = machine.driver_data<pasopia7_state>();
	UINT8 *bios = state->memregion("maincpu")->base();

	state->membank("bank1")->set_base(bios + 0x10000);
	state->membank("bank2")->set_base(bios + 0x10000);
//  state->membank("bank3")->set_base(bios + 0x10000);
//  state->membank("bank4")->set_base(bios + 0x10000);

	state->m_nmi_reset |= 4;
}

static PALETTE_INIT( p7_raster )
{
	int i;

	for( i = 0; i < 8; i++)
		palette_set_color_rgb(machine, i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

/* TODO: palette values are mostly likely to be wrong in there */
static PALETTE_INIT( p7_lcd )
{
	int i;

	palette_set_color_rgb(machine, 0, 0xa0, 0xa8, 0xa0);

	for( i = 1; i < 8; i++)
		palette_set_color_rgb(machine, i, 0x30, 0x38, 0x10);
}

static const struct upd765_interface pasopia7_upd765_interface =
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL, //DRQ, TODO
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static const floppy_interface pasopia7_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	NULL,
	NULL
};

static MACHINE_CONFIG_START( p7_base, pasopia7_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(pasopia7_mem)
	MCFG_CPU_IO_MAP(pasopia7_io)
	MCFG_CPU_CONFIG(p7_daisy)

	MCFG_MACHINE_RESET(pasopia7)

	/* Audio */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("sn1", SN76489A, 1996800) // unknown clock / divider
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("sn2", SN76489A, 1996800) // unknown clock / divider
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* Devices */
	MCFG_Z80CTC_ADD( "z80ctc", XTAL_4MHz, z80ctc_intf )
	MCFG_Z80PIO_ADD( "z80pio", XTAL_4MHz, z80pio_intf )
	MCFG_I8255_ADD( "ppi8255_0", ppi8255_intf_0 )
	MCFG_I8255_ADD( "ppi8255_1", ppi8255_intf_1 )
	MCFG_I8255_ADD( "ppi8255_2", ppi8255_intf_2 )
	MCFG_UPD765A_ADD("fdc", pasopia7_upd765_interface)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(pasopia7_floppy_interface)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( p7_raster, p7_base )
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 32-1)
	MCFG_VIDEO_START(pasopia7)
	MCFG_SCREEN_UPDATE_STATIC(pasopia7)
	MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT(p7_raster)
	MCFG_GFXDECODE( pasopia7 )

	MCFG_MC6845_ADD("crtc", H46505, VDP_CLOCK, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( p7_lcd, p7_base )
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MCFG_VIDEO_START(pasopia7)
	MCFG_SCREEN_UPDATE_STATIC(pasopia7)
	MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT(p7_lcd)
	MCFG_GFXDECODE( pasopia7 )

	MCFG_MC6845_ADD("crtc", H46505, LCD_CLOCK, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
	MCFG_DEFAULT_LAYOUT( layout_lcd )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pasopia7 )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x10000, 0x4000, CRC(b8111407) SHA1(ac93ae62db4c67de815f45de98c79cfa1313857d))

	ROM_REGION( 0x8000, "basic", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(8a58fab6) SHA1(5e1a91dfb293bca5cf145b0a0c63217f04003ed1))

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(6109e308) SHA1(5c21cf1f241ef1fa0b41009ea41e81771729785f))

	ROM_REGION( 0x8000, "rampac1", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac1.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x8000, "rampac2", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac2.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )
ROM_END

/* using an identical ROMset from now, but the screen type is different */
ROM_START( pasopia7lcd )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bios.rom", 0x10000, 0x4000, CRC(b8111407) SHA1(ac93ae62db4c67de815f45de98c79cfa1313857d))

	ROM_REGION( 0x8000, "basic", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x8000, CRC(8a58fab6) SHA1(5e1a91dfb293bca5cf145b0a0c63217f04003ed1))

	ROM_REGION( 0x800, "font", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, BAD_DUMP CRC(a91c45a9) SHA1(a472adf791b9bac3dfa6437662e1a9e94a88b412))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(6109e308) SHA1(5c21cf1f241ef1fa0b41009ea41e81771729785f))

	ROM_REGION( 0x8000, "rampac1", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac1.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x8000, "rampac2", ROMREGION_ERASEFF )
//  ROM_LOAD( "rampac2.bin", 0x0000, 0x8000, CRC(0e4f09bd) SHA1(4088906d57e4f6085a75b249a6139a0e2eb531a1) )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )
ROM_END


DRIVER_INIT_MEMBER(pasopia7_state,p7_raster)
{

	m_screen_type = 1;
}

DRIVER_INIT_MEMBER(pasopia7_state,p7_lcd)
{

	m_screen_type = 0;
}


/* Driver */

COMP( 1983, pasopia7,    0,              0,       p7_raster,     pasopia7, pasopia7_state,   p7_raster,  "Toshiba", "Pasopia 7 (Raster)", GAME_NOT_WORKING )
COMP( 1983, pasopia7lcd, pasopia7,       0,       p7_lcd,        pasopia7, pasopia7_state,   p7_lcd,     "Toshiba", "Pasopia 7 (LCD)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )
