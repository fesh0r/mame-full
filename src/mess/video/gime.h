/*********************************************************************

    mc6847.h

    Implementation of Motorola 6847 video hardware chip

***************************************************************************/

#pragma once

#ifndef __GIME__
#define __GIME__

#include "video/mc6847.h"
#include "machine/6883sam.h"
#include "machine/ram.h"


//**************************************************************************
//  GIME CONFIG/INTERFACE
//**************************************************************************

/* interface */
typedef struct _gime_interface gime_interface;
struct _gime_interface
{
	const char *m_screen_tag;	/* screen we are acting on */
	const char *m_maincpu_tag;	/* tag of main CPU */
	const char *m_ram_tag;		/* tag of RAM device */
	const char *m_ext_tag;		/* tag of expansion device */

	/* if specified, this gets called for every change of the HSYNC pin */
	devcb_write_line			m_out_hsync_func;

	/* if specified, this gets called for every change of the FSYNC pin */
	devcb_write_line			m_out_fsync_func;

	/* if specified, this gets called for every change of the IRQ pin */
	devcb_write_line			m_out_irq_func;

	/* if specified, this gets called for every change of the FIRQ pin */
	devcb_write_line			m_out_firq_func;

	/* if specified, this reads from the floating bugs */
	devcb_read8					m_in_floating_bus_func;
};



//**************************************************************************
//  GIME CORE
//**************************************************************************

class cococart_slot_device;
\
class gime_base_device : public mc6847_friend_device, public sam6883_friend_device
{
public:
	// read/write
	DECLARE_READ8_MEMBER( read ) { return read(offset); }
	DECLARE_WRITE8_MEMBER( write ) { write(offset, data); }

	// used to turn on/off reading/writing to $FF40-$FF5F
	bool spare_chip_select_enabled(void) { return m_gime_registers[0] & 0x04 ? true : false; }

	// the GIME seems to intercept writes to $FF22 (not precisely sure how)
	void ff22_write(UINT8 data) { m_ff22_value = data; }

	// updates the cart ROM
	void update_cart_rom(void);

	/* updates the screen -- this will call begin_update(),
       followed by update_row() reapeatedly and after all row
       updating is complete, end_update() */
	bool update_composite(bitmap_rgb32 &bitmap, const rectangle &cliprect);
	bool update_rgb(bitmap_rgb32 &bitmap, const rectangle &cliprect);

	// interrupt outputs
	bool firq_r(void) { return m_firq != 0x00; }
	bool irq_r(void) { return m_irq != 0x00; }

	// interrupt inputs
	void set_il0(bool value) { set_interrupt_value(INTERRUPT_EI0, value); }
	void set_il1(bool value) { set_interrupt_value(INTERRUPT_EI1, value); }
	void set_il2(bool value) { set_interrupt_value(INTERRUPT_EI2, value); }

protected:
	gime_base_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const UINT8 *fontdata);

    // device-level overrides
    virtual void device_start(void);
	virtual void device_reset(void);
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual void device_post_load(void);
	virtual ioport_constructor device_input_ports() const;

	// other overrides
	virtual void new_frame(void);
	virtual void horizontal_sync_changed(bool line);
	virtual void enter_bottom_border(void);
	virtual void record_border_scanline(UINT16 physical_scanline);
	virtual void record_body_scanline(UINT16 physical_scanline, UINT16 logical_scanline);

private:
	typedef mc6847_friend_device super;

	struct scanline_record
	{
		UINT8 m_border;
		UINT8 m_line_in_row;
		UINT8 m_ff22_value;
		UINT8 m_ff98_value;
		UINT8 m_ff99_value;
		UINT8 m_mode[160];
		UINT8 m_data[160];
		UINT16 m_palette[160];
	};

	typedef UINT32 (gime_base_device::*get_data_func)(UINT32, UINT8 *, UINT8 *);

	class palette_resolver
	{
	public:
		palette_resolver(gime_base_device *gime, const pixel_t *palette);
		const pixel_t *get_palette(UINT16 palette_rotation);
		pixel_t lookup(UINT8 color);

	private:
		gime_base_device *m_gime;
		const pixel_t *m_palette;
		pixel_t m_resolved_palette[16];
		int m_current_resolved_palette;
	};

	enum
	{
		INTERRUPT_TMR		= 0x20,
		INTERRUPT_HBORD		= 0x10,
		INTERRUPT_VBORD		= 0x08,
		INTERRUPT_EI2		= 0x04,
		INTERRUPT_EI1		= 0x02,
		INTERRUPT_EI0		= 0x01
	};

	typedef enum
	{
		GIME_TIMER_HBORD,
		GIME_TIMER_CLOCK
	} timer_type_t;

	// timer constants
	static const device_timer_id TIMER_GIME_CLOCK = 4;

	// statics
	static const UINT8 hires_font[128][12];

	// callbacks
	devcb_resolved_write_line m_res_out_irq_func;
	devcb_resolved_write_line m_res_out_firq_func;
	devcb_resolved_read8 m_res_in_floating_bus_func;

	// device state
	UINT8 m_gime_registers[16];
	UINT8 m_mmu[16];
	UINT8 m_ff22_value;
	UINT8 m_interrupt_value;
	UINT8 m_irq;
	UINT8 m_firq;
	UINT16 m_timer_value;
	bool m_is_blinking;

	// video state
	bool m_legacy_video;
	UINT32 m_video_position;
	UINT8 m_line_in_row;
	scanline_record m_scanlines[25+192+26];
	bool m_displayed_rgb;

	// palette state
	UINT8 m_palette_rotated[1024][16];
	UINT16 m_palette_rotated_position;
	bool m_palette_rotated_position_used;

	// incidentals
	ram_device *m_ram;
	emu_timer *m_gime_clock_timer;
	cococart_slot_device *m_cart_device;
	UINT8 *m_rom;
	UINT8 *m_cart_rom;
	pixel_t m_composite_palette[64];
	pixel_t m_composite_bw_palette[64];
	pixel_t m_rgb_palette[64];
	UINT8 m_dummy_bank[0x2000];

	// timer constants
	static const device_timer_id TIMER_FRAME = 0;
	static const device_timer_id TIMER_HSYNC_OFF = 1;
	static const device_timer_id TIMER_HSYNC_ON = 2;
	static const device_timer_id TIMER_FSYNC_OFF = 3;
	static const device_timer_id TIMER_FSYNC_ON = 4;

	// read/write
	UINT8 read(offs_t offset);
	UINT8 read_gime_register(offs_t offset);
	UINT8 read_mmu_register(offs_t offset);
	UINT8 read_palette_register(offs_t offset);
	UINT8 read_floating_bus(void);
	void write(offs_t offset, UINT8 data);
	void write_gime_register(offs_t offset, UINT8 data);
	void write_mmu_register(offs_t offset, UINT8 data);
	void write_palette_register(offs_t offset, UINT8 data);
	void write_sam_register(offs_t offset);

	// memory
	void update_memory(void);
	void update_memory(int bank);
	UINT8 *memory_pointer(UINT32 address);

	// interrupts
	void interrupt_rising_edge(UINT8 interrupt);
	void recalculate_irq(void);
	void recalculate_firq(void);

	ATTR_FORCE_INLINE void set_interrupt_value(UINT8 interrupt, bool value)
	{
		/* save the old interrupt value */
		UINT8 old_interrupt_value = m_interrupt_value;

		/* update the interrupt value */
		if (value)
			m_interrupt_value |= interrupt;
		else
			m_interrupt_value &= ~interrupt;

		/* was this a rising edge? */
		if (value && ((old_interrupt_value & interrupt) == 0))
		{
			interrupt_rising_edge(interrupt);
		}
	}

	// timer
	timer_type_t timer_type(void);
	const char *timer_type_string(void);
	void reset_timer(void);
	void timer_elapsed(void);

	// video
	bool update_screen(bitmap_rgb32 &bitmap, const rectangle &cliprect, const pixel_t *palette);
	void update_geometry(void);
	void update_border(UINT16 physical_scanline);
	pixel_t get_composite_color(int color);
	pixel_t get_rgb_color(int color);
	offs_t get_video_base(void);
	UINT16 get_lines_per_row(void);
	UINT32 get_data_mc6847(UINT32 video_position, UINT8 *data, UINT8 *mode);
	UINT32 get_data_without_attributes(UINT32 video_position, UINT8 *data, UINT8 *mode);
	UINT32 get_data_with_attributes(UINT32 video_position, UINT8 *data, UINT8 *mode);
	bool is_blinking(void);

	// template function for doing video update collection
	template<UINT8 xres, get_data_func get_data, bool record_mode>
	UINT32 record_scanline_res(int scanline);

	// rendering sampled graphics
	typedef UINT32 (gime_base_device::*emit_samples_proc)(const scanline_record *scanline, int sample_start, int sample_count, pixel_t *pixels, const pixel_t *palette);
	UINT32 emit_dummy_samples(const scanline_record *scanline, int sample_start, int sample_count, pixel_t *pixels, const pixel_t *palette);
	UINT32 emit_mc6847_samples(const scanline_record *scanline, int sample_start, int sample_count, pixel_t *pixels, const pixel_t *palette);
	template<int xscale>
	UINT32 emit_gime_text_samples(const scanline_record *scanline, int sample_start, int sample_count, pixel_t *pixels, const pixel_t *palette);
	template<int xscale, int bits_per_pixel>
	UINT32 emit_gime_graphics_samples(const scanline_record *scanline, int sample_start, int sample_count, pixel_t *pixels, const pixel_t *palette);
	template<int sample_count, emit_samples_proc emit_samples>
	void render_scanline(const scanline_record *scanline, pixel_t *pixels, int min_x, int max_x, palette_resolver *resolver);
};


//**************************************************************************
//  VARIATIONS
//**************************************************************************

class gime_ntsc_device : public gime_base_device
{
public:
	gime_ntsc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};

class gime_pal_device : public gime_base_device
{
public:
	gime_pal_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};

extern const device_type GIME_NTSC;
extern const device_type GIME_PAL;

#endif /* __GIME__ */
