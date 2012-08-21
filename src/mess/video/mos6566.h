/***************************************************************************

    MOS 6566/6567/6569 Video Interface Chip II (VIC-II) emulation

    Copyright the MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************
                            _____   _____
                   DB6   1 |*    \_/     | 40  Vcc
                   DB5   2 |             | 39  DB7
                   DB4   3 |             | 38  DB8
                   DB3   4 |             | 37  DB9
                   DB2   5 |             | 36  DB10
                   DB1   6 |             | 35  DB11
                   DB0   7 |             | 34  A13
                  _IRQ   8 |             | 33  A12
                    LP   9 |             | 32  A11
                   _CS  10 |   MOS6566   | 31  A10
                   R/W  11 |             | 30  A9
                    BA  12 |             | 29  A8
                   Vdd  13 |             | 28  A7
                 COLOR  14 |             | 27  A6
                 S/LUM  15 |             | 26  A5
                   AEC  16 |             | 25  A4
                   PH0  17 |             | 24  A3
                  PHIN  18 |             | 23  A2
                 PHCOL  19 |             | 22  A1
                   Vss  20 |_____________| 21  A0

                            _____   _____
                   DB6   1 |*    \_/     | 40  Vcc
                   DB5   2 |             | 39  DB7
                   DB4   3 |             | 38  DB8
                   DB3   4 |             | 37  DB9
                   DB2   5 |             | 36  DB10
                   DB1   6 |             | 35  DB11
                   DB0   7 |             | 34  A10
                  _IRQ   8 |             | 33  A9
                    LP   9 |   MOS6567   | 32  A8
                   _CS  10 |   MOS6569   | 31  A7
                   R/W  11 |   MOS8562   | 30  A6
                    BA  12 |   MOS8565   | 29  A5/A13
                   Vdd  13 |             | 28  A4/A12
                 COLOR  14 |             | 27  A3/A11
                 S/LUM  15 |             | 26  A2/A10
                   AEC  16 |             | 25  A1/A9
                   PH0  17 |             | 24  A0/A8
                  _RAS  18 |             | 23  A11
                   CAS  19 |             | 22  PHIN
                   Vss  20 |_____________| 21  PHCL

                            _____   _____
                    D6   1 |*    \_/     | 48  Vcc
                    D5   2 |             | 47  D7
                    D4   3 |             | 46  D8
                    D3   4 |             | 45  D9
                    D2   5 |             | 44  D10
                    D1   6 |             | 43  D11
                    D0   7 |             | 42  MA10
                  _IRQ   8 |             | 41  MA9
                   _LP   9 |             | 40  MA8
                    BA  10 |             | 39  A7
              _DMARQST  11 |             | 38  A6
                   AEC  12 |   MOS8564   | 37  MA5
                   _CS  13 |   MOS8566   | 36  MA4
                   R/W  14 |             | 35  MA3
               _DMAACK  15 |             | 34  MA2
                CHROMA  16 |             | 33  MA1
              SYNC/LUM  17 |             | 32  MA0
                 1 MHZ  18 |             | 31  MA11
                  _RAS  19 |             | 30  PHI IN
                  _CAS  20 |             | 29  PHI COLOR
                   MUX  21 |             | 28  K2
                _IOACC  22 |             | 27  K1
                 2 MHZ  23 |             | 26  K0
                   Vss  24 |_____________| 25  Z80 PHI

***************************************************************************/

#pragma once

#ifndef __MOS656X__
#define __MOS656X__

#include "emu.h"



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_MOS6566_ADD(_tag, _screen_tag, _clock, _config, _videoram_map, _colorram_map) \
	MCFG_DEVICE_ADD(_tag, MOS6566, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _videoram_map) \
	MCFG_DEVICE_ADDRESS_MAP(AS_1, _colorram_map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(VIC6566_VRETRACERATE) \
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES) \
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos6566_device, screen_update) \
	MCFG_PALETTE_LENGTH(16)

#define MCFG_MOS6567_ADD(_tag, _screen_tag, _clock, _config, _videoram_map, _colorram_map) \
	MCFG_DEVICE_ADD(_tag, MOS6567, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _videoram_map) \
	MCFG_DEVICE_ADDRESS_MAP(AS_1, _colorram_map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE) \
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES) \
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos6567_device, screen_update) \
	MCFG_PALETTE_LENGTH(16)

#define MCFG_MOS8562_ADD(_tag, _screen_tag, _clock, _config, _videoram_map, _colorram_map) \
	MCFG_DEVICE_ADD(_tag, MOS8562, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _videoram_map) \
	MCFG_DEVICE_ADDRESS_MAP(AS_1, _colorram_map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE) \
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES) \
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos8562_device, screen_update) \
	MCFG_PALETTE_LENGTH(16)

#define MCFG_MOS6569_ADD(_tag, _screen_tag, _clock, _config, _videoram_map, _colorram_map) \
	MCFG_DEVICE_ADD(_tag, MOS6569, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _videoram_map) \
	MCFG_DEVICE_ADDRESS_MAP(AS_1, _colorram_map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE) \
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES) \
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos6569_device, screen_update) \
	MCFG_PALETTE_LENGTH(16)

#define MCFG_MOS8565_ADD(_tag, _screen_tag, _clock, _config, _videoram_map, _colorram_map) \
	MCFG_DEVICE_ADD(_tag, MOS8565, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _videoram_map) \
	MCFG_DEVICE_ADDRESS_MAP(AS_1, _colorram_map) \
	MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE) \
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES) \
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, mos8565_device, screen_update) \
	MCFG_PALETTE_LENGTH(16)


#define MOS6566_INTERFACE(_name) \
	const mos6566_interface (_name) =

#define MOS6567_INTERFACE(_name) \
	const mos6566_interface (_name) =

#define MOS6569_INTERFACE(_name) \
	const mos6566_interface (_name) =



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VIC6566_CLOCK			(XTAL_8MHz / 8) // 1000000
#define VIC6567R56A_CLOCK		(XTAL_8MHz / 8) // 1000000
#define VIC6567_CLOCK			(XTAL_14_31818MHz / 14) // 1022727
#define VIC6569_CLOCK			(XTAL_17_734472MHz / 18) // 985248

#define VIC6566_DOTCLOCK		(VIC6566_CLOCK * 8) // 8000000
#define VIC6567R56A_DOTCLOCK	(VIC6567R56A_CLOCK * 8) // 8000000
#define VIC6567_DOTCLOCK		(VIC6567_CLOCK * 8) // 8181818
#define VIC6569_DOTCLOCK		(VIC6569_CLOCK * 8) // 7881988

#define VIC6567_CYCLESPERLINE	65
#define VIC6569_CYCLESPERLINE	63

#define VIC6567_LINES		263
#define VIC6569_LINES		312

#define VIC6566_VRETRACERATE		((float)VIC6566_CLOCK / 262 / 64)
#define VIC6567R56A_VRETRACERATE	((float)VIC6567R56A_CLOCK / 262 / 64)
#define VIC6567_VRETRACERATE		((float)VIC6567_CLOCK / 263 / 65)
#define VIC6569_VRETRACERATE		((float)VIC6569_CLOCK / 312 / 63)

#define VIC6566_HRETRACERATE	(VIC6566_CLOCK / VIC6566_CYCLESPERLINE)
#define VIC6567_HRETRACERATE	(VIC6567_CLOCK / VIC6567_CYCLESPERLINE)
#define VIC6569_HRETRACERATE	(VIC6569_CLOCK / VIC6569_CYCLESPERLINE)

#define VIC2_HSIZE		320
#define VIC2_VSIZE		200

#define VIC6567_VISIBLELINES	235
#define VIC6569_VISIBLELINES	284

#define VIC6567_FIRST_DMA_LINE	0x30
#define VIC6569_FIRST_DMA_LINE	0x30

#define VIC6567_LAST_DMA_LINE	0xf7
#define VIC6569_LAST_DMA_LINE	0xf7

#define VIC6567_FIRST_DISP_LINE	0x29
#define VIC6569_FIRST_DISP_LINE	0x10

#define VIC6567_LAST_DISP_LINE	(VIC6567_FIRST_DISP_LINE + VIC6567_VISIBLELINES - 1)
#define VIC6569_LAST_DISP_LINE	(VIC6569_FIRST_DISP_LINE + VIC6569_VISIBLELINES - 1)

#define VIC6567_RASTER_2_EMU(a) ((a >= VIC6567_FIRST_DISP_LINE) ? (a - VIC6567_FIRST_DISP_LINE) : (a + 222))
#define VIC6569_RASTER_2_EMU(a) (a - VIC6569_FIRST_DISP_LINE)

#define VIC6567_FIRSTCOLUMN	50
#define VIC6569_FIRSTCOLUMN	50

#define VIC6567_VISIBLECOLUMNS	418
#define VIC6569_VISIBLECOLUMNS	403

#define VIC6567_X_2_EMU(a)	(a)
#define VIC6569_X_2_EMU(a)	(a)

#define VIC6567_STARTVISIBLELINES ((VIC6567_LINES - VIC6567_VISIBLELINES)/2)
#define VIC6569_STARTVISIBLELINES 16 /* ((VIC6569_LINES - VIC6569_VISIBLELINES)/2) */

#define VIC6567_FIRSTRASTERLINE 34
#define VIC6569_FIRSTRASTERLINE 0

#define VIC6567_COLUMNS 512
#define VIC6569_COLUMNS 504

#define VIC6567_STARTVISIBLECOLUMNS ((VIC6567_COLUMNS - VIC6567_VISIBLECOLUMNS)/2)
#define VIC6569_STARTVISIBLECOLUMNS ((VIC6569_COLUMNS - VIC6569_VISIBLECOLUMNS)/2)

#define VIC6567_FIRSTRASTERCOLUMNS 412
#define VIC6569_FIRSTRASTERCOLUMNS 404

#define VIC6569_FIRST_X 0x194
#define VIC6567_FIRST_X 0x19c

#define VIC6569_FIRST_VISIBLE_X 0x1e0
#define VIC6567_FIRST_VISIBLE_X 0x1e8

#define VIC6569_MAX_X 0x1f7
#define VIC6567_MAX_X 0x1ff

#define VIC6569_LAST_VISIBLE_X 0x17c
#define VIC6567_LAST_VISIBLE_X 0x184

#define VIC6569_LAST_X 0x193
#define VIC6567_LAST_X 0x19b



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> mos6566_interface

struct mos6566_interface
{
	const char			*m_screen_tag;
	const char			*m_cpu_tag;

	devcb_write_line	m_out_irq_cb;
	devcb_write_line	m_out_rdy_cb;

	devcb_read8			m_in_x_cb;
	devcb_read8			m_in_y_cb;
	devcb_read8			m_in_button_cb;

	devcb_read8			m_in_rdy_cb;
};


// ======================> mos6566_device

class mos6566_device :  public device_t,
					    public device_memory_interface,
					    public device_execute_interface,
					    public mos6566_interface
{
public:
	// construction/destruction
	mos6566_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	mos6566_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );

	DECLARE_WRITE_LINE_MEMBER( lp_w );

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	enum
	{
		TYPE_6566,	// NTSC-M (SRAM)
		TYPE_6567,	// NTSC-M (NMOS)
		TYPE_8562,	// NTSC-M (HMOS)
		TYPE_8564,	// NTSC-M VIC-IIe (C128)

		TYPE_6569,	// PAL-B
		TYPE_6572,	// PAL-N
		TYPE_6573,	// PAL-M
		TYPE_8565,	// PAL-B (HMOS)
		TYPE_8566,	// PAL-B VIC-IIe (C128)
		TYPE_8569	// PAL-N VIC-IIe (C128)
	};

	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
	virtual void execute_run();

    int m_icount;
	int m_variant;

	const address_space_config		m_videoram_space_config;
	const address_space_config		m_colorram_space_config;

	inline void vic2_set_interrupt( int mask );
	inline void vic2_clear_interrupt( int mask );
	inline UINT8 read_videoram(offs_t offset);
	inline UINT8 read_colorram(offs_t offset);
	inline void vic2_idle_access();
	inline void vic2_spr_ptr_access( int num );
	inline void vic2_spr_data_access( int num, int bytenum );
	inline void vic2_display_if_bad_line();
	inline void vic2_suspend_cpu();
	inline void vic2_resume_cpu();
	inline void vic2_refresh_access();
	inline void vic2_fetch_if_bad_line();
	inline void vic2_rc_if_bad_line();
	inline void vic2_sample_border();
	inline void vic2_check_sprite_dma();
	inline void vic2_matrix_access();
	inline void vic2_graphics_access();
	inline void vic2_draw_background();
	inline void vic2_draw_mono( UINT16 p, UINT8 c0, UINT8 c1 );
	inline void vic2_draw_multi( UINT16 p, UINT8 c0, UINT8 c1, UINT8 c2, UINT8 c3 );
	void vic2_draw_graphics();
	void vic2_draw_sprites();

	screen_device *m_screen;			// screen which sets bitmap properties
	cpu_device *m_cpu;

	UINT8 m_rdy_cycles;
	UINT8 m_reg[0x80];

	int m_on;								/* rastering of the screen */

	UINT16 m_chargenaddr, m_videoaddr, m_bitmapaddr;

	bitmap_ind16 *m_bitmap;

	UINT16 m_colors[4], m_spritemulti[4];

	int m_rasterline;
	UINT64 m_cycles_counter;
	UINT8 m_cycle;
	UINT16 m_raster_x;
	UINT16 m_graphic_x;

	/* convert multicolor byte to background/foreground for sprite collision */
	UINT16 m_expandx[256];
	UINT16 m_expandx_multi[256];

	/* Display */
	UINT16 m_dy_start;
	UINT16 m_dy_stop;

	/* GFX */
	UINT8 m_draw_this_line;
	UINT8 m_is_bad_line;
	UINT8 m_bad_lines_enabled;
	UINT8 m_display_state;
	UINT8 m_char_data;
	UINT8 m_gfx_data;
	UINT8 m_color_data;
	UINT8 m_last_char_data;
	UINT8 m_matrix_line[40];						// Buffer for video line, read in Bad Lines
	UINT8 m_color_line[40];						// Buffer for color line, read in Bad Lines
	UINT8 m_vblanking;
	UINT16 m_ml_index;
	UINT8 m_rc;
	UINT16 m_vc;
	UINT16 m_vc_base;
	UINT8 m_ref_cnt;

	/* Sprites */
	UINT8 m_spr_coll_buf[0x400];					// Buffer for sprite-sprite collisions and priorities
	UINT8 m_fore_coll_buf[0x400];					// Buffer for foreground-sprite collisions and priorities
	UINT8 m_spr_draw_data[8][4];					// Sprite data for drawing
	UINT8 m_spr_exp_y;
	UINT8 m_spr_dma_on;
	UINT8 m_spr_draw;
	UINT8 m_spr_disp_on;
	UINT16 m_spr_ptr[8];
	UINT8 m_spr_data[8][4];
	UINT16 m_mc_base[8];						// Sprite data counter bases
	UINT16 m_mc[8];							// Sprite data counters

	/* Border */
	UINT8 m_border_on;
	UINT8 m_ud_border_on;
	UINT8 m_border_on_sample[5];
	UINT8 m_border_color_sample[0x400 / 8];			// Samples of border color at each "displayed" cycle

	/* Cycles */
	UINT64 m_first_ba_cycle;
	UINT8 m_device_suspended;

	/* IRQ */
	devcb_resolved_write_line		m_out_irq_func;

	/* RDY */
	devcb_resolved_write_line		m_out_rdy_func;
	devcb_resolved_read8      m_in_rdy_workaround_func;

	/* lightpen */
	devcb_resolved_read8 m_in_lightpen_button_func;
	devcb_resolved_read8 m_in_lightpen_x_func;
	devcb_resolved_read8 m_in_lightpen_y_func;
};


// ======================> mos6567_device

class mos6567_device :  public mos6566_device
{
public:
    // construction/destruction
    mos6567_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
    mos6567_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> mos8562_device

class mos8562_device :  public mos6567_device
{
public:
    // construction/destruction
    mos8562_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> mos6569_device

class mos6569_device :  public mos6566_device
{
public:
    // construction/destruction
    mos6569_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
    mos6569_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
    virtual void execute_run();
};


// ======================> mos8565_device

class mos8565_device :  public mos6569_device
{
public:
    // construction/destruction
    mos8565_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// device type definition
extern const device_type MOS6566;
extern const device_type MOS6567;
extern const device_type MOS8562;
extern const device_type MOS6569;
extern const device_type MOS8565;



#endif
