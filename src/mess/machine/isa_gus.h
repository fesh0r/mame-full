/*
 *  Gravis Ultrasound ISA card
 *
 *  Started: 28/01/2012
 *
 *  I/O port map (info from the Gravis Ultrasound SDK documentation):
 *  Base port is 0x2X0 - where X is defined by a jumper
 *
 *  MIDI:
 *  0x3X0 - MIDI Control (read), MIDI Status (write)
 *  0x3X1 - MIDI Transmit (write), MIDI Receive (read)
 *  MIDI operates identically to a 6850 UART
 *
 *  Joystick:
 *  0x201 - Joystick trigger timer (write), Joystick data (read)
 *
 *  GF1 Synthesiser:
 *  0x3X2 - Page register (voice select)
 *  0x3X3 - Global Register select
 *  0x3X4 - Global Data (low byte)
 *  0x3X5 - Global Data (high byte)
 *  0x2X6 - IRQ status register (read only, active high)
 *  0x2X8 - Timer control register
 *  0x2X9 - Timer data
 *  0x3X7 - DRAM data (can be via DMA also)
 *
 *  Board:
 *  0x2X0: Mix control register (write only)
 *  0x2XB: IRQ/DMA control register (write only) - dependant on mix control bit 6
 *  0x2XF: Register controls (board rev 3.4+ only)
 *  0x7X6: Board version (read only, board rev 3.7+ only)
 *
 *  Mixer Control:
 *  0x7X6: Control port (write only)
 *  0x3X6: Data port (write only)
 */

#pragma once

#ifndef __ISA_GUS_H__
#define __ISA_GUS_H__

#include "emu.h"
#include "machine/isa.h"
#include "machine/6850acia.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> gf1_device

#define GF1_CLOCK 9878400

#define IRQ_2XF           0x00
#define IRQ_MIDI_TRANSMIT 0x01
#define IRQ_MIDI_RECEIVE  0x02
#define IRQ_TIMER1        0x04
#define IRQ_TIMER2        0x08
#define IRQ_SB            0x10
#define IRQ_WAVETABLE     0x20
#define IRQ_VOLUME_RAMP   0x40
#define IRQ_DRAM_TC_DMA   0x80

struct _gus_voice
{
	UINT8 voice_ctrl;
	UINT16 freq_ctrl;
	UINT32 start_addr;
	UINT32 end_addr;
	UINT8 vol_ramp_rate;
	UINT8 vol_ramp_start;
	UINT8 vol_ramp_end;
	UINT16 current_vol;
	UINT32 current_addr;
	UINT8 pan_position;
	UINT8 vol_ramp_ctrl;
	UINT32 vol_count;
	bool rollover;
    INT16 sample;  // current sample data
};
typedef struct _gus_voice gus_voice;

struct _gf1_interface
{
	devcb_write_line wave_irq_cb;
	devcb_write_line ramp_irq_cb;
	devcb_write_line timer1_irq_cb;
	devcb_write_line timer2_irq_cb;
	devcb_write_line sb_irq_cb;
	devcb_write_line dma_irq_cb;
	devcb_write_line drq1_cb;
	devcb_write_line drq2_cb;
	devcb_write_line nmi_cb;
};
typedef struct _gf1_interface gf1_interface;

class gf1_device :
		public device_t,
		public device_sound_interface,
		public gf1_interface
{
public:
		// construction/destruction
        gf1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

        // current IRQ/DMA channel getters
        UINT8 gf1_irq() { return m_gf1_irq; }
        UINT8 midi_irq() { if(m_irq_combine == 0) return m_midi_irq; else return m_gf1_irq; }
        UINT8 dma_channel1() { return m_dma_channel1; }
        UINT8 dma_channel2() { if(m_dma_combine == 0) return m_dma_channel2; else return m_dma_channel1; }

        DECLARE_READ8_MEMBER(global_reg_select_r);
        DECLARE_WRITE8_MEMBER(global_reg_select_w);
        DECLARE_READ8_MEMBER(global_reg_data_r);
        DECLARE_WRITE8_MEMBER(global_reg_data_w);
        DECLARE_READ8_MEMBER(dram_r);
        DECLARE_WRITE8_MEMBER(dram_w);
        DECLARE_READ8_MEMBER(adlib_r);
        DECLARE_WRITE8_MEMBER(adlib_w);
        DECLARE_READ8_MEMBER(adlib_cmd_r);
        DECLARE_WRITE8_MEMBER(adlib_cmd_w);
        DECLARE_READ8_MEMBER(mix_ctrl_r);
        DECLARE_WRITE8_MEMBER(mix_ctrl_w);
        DECLARE_READ8_MEMBER(stat_r);
        DECLARE_WRITE8_MEMBER(stat_w);
        DECLARE_READ8_MEMBER(sb_r);
        DECLARE_WRITE8_MEMBER(sb_w);
        DECLARE_WRITE8_MEMBER(sb2x6_w);

        // DMA signals
    	UINT8 dack_r(int line);
    	void dack_w(int line,UINT8 data);
    	void eop_w(int state);

		// optional information overrides
		virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
		virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
		virtual void device_config_complete();

        // voice-specific registers
        gus_voice m_voice[32];

        // global registers (not voice-specific)
        UINT8 m_dma_dram_ctrl;
        UINT16 m_dma_start_addr;
        UINT32 m_dram_addr;
        UINT8 m_timer_ctrl;
        UINT8 m_timer1_count;
        UINT8 m_timer2_count;
        UINT8 m_timer1_value;
        UINT8 m_timer2_value;
        UINT16 m_sampling_freq;
        UINT8 m_sampling_ctrl;
        UINT8 m_joy_trim_dac;
        UINT8 m_reset;
        UINT8 m_active_voices;
        UINT8 m_irq_source;

        void set_irq(UINT8 source, UINT8 voice);
        void reset_irq(UINT8 source);
        void update_volume_ramps();

        UINT8* m_wave_ram;

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
        virtual void device_stop();

        devcb_resolved_write_line m_wave_irq_func;
        devcb_resolved_write_line m_ramp_irq_func;
        devcb_resolved_write_line m_timer1_irq_func;
        devcb_resolved_write_line m_timer2_irq_func;
        devcb_resolved_write_line m_sb_irq_func;
        devcb_resolved_write_line m_dma_irq_func;
        devcb_resolved_write_line m_drq1;
        devcb_resolved_write_line m_drq2;
        devcb_resolved_write_line m_nmi_func;

private:
        // internal state
        sound_stream* m_stream;

        emu_timer* m_timer1;
        emu_timer* m_timer2;
        emu_timer* m_dmatimer;
        emu_timer* m_voltimer;

        UINT8 m_current_voice;
        UINT8 m_current_reg;
        UINT8 m_port;
        UINT8 m_irq;
        UINT8 m_dma;

        UINT8 m_adlib_cmd;
        UINT8 m_mix_ctrl;
        UINT8 m_gf1_irq;
        UINT8 m_midi_irq;
        UINT8 m_dma_channel1;
        UINT8 m_dma_channel2;
        UINT8 m_irq_combine;
        UINT8 m_dma_combine;
        UINT8 m_adlib_timer_cmd;
        UINT8 m_adlib_timer1_enable;
        UINT8 m_adlib_timer2_enable;
        UINT8 m_adlib_status;
        UINT8 m_adlib_data;
        UINT8 m_voice_irq_fifo[32];
        UINT8 m_voice_irq_ptr;
        UINT8 m_voice_irq_current;
        UINT8 m_dma_16bit;  // set by bit 6 of the DMA DRAM control reg
        UINT8 m_statread;
        UINT8 m_sb_data_2xc;
        UINT8 m_sb_data_2xe;
        UINT8 m_reg_ctrl;
        UINT8 m_fake_adlib_status;
        UINT32 m_dma_current;
        UINT16 m_volume_table[4096];

    	static const device_timer_id ADLIB_TIMER1 = 0;
    	static const device_timer_id ADLIB_TIMER2 = 1;
    	static const device_timer_id DMA_TIMER = 2;
    	static const device_timer_id VOL_RAMP_TIMER = 3;
};

class isa16_gus_device :
		public device_t,
		public device_isa16_card_interface
{
public:
		isa16_gus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		void set_irq(UINT8 source);
        void reset_irq(UINT8 source);
		void set_midi_irq(UINT8 source);
        void reset_midi_irq(UINT8 source);

		DECLARE_READ8_MEMBER(board_r);
		DECLARE_READ8_MEMBER(synth_r);
		DECLARE_WRITE8_MEMBER(board_w);
		DECLARE_WRITE8_MEMBER(synth_w);
		DECLARE_READ8_MEMBER(adlib_r);
		DECLARE_WRITE8_MEMBER(adlib_w);
        DECLARE_READ8_MEMBER(joy_r);
        DECLARE_WRITE8_MEMBER(joy_w);
        DECLARE_WRITE_LINE_MEMBER(midi_irq);
        DECLARE_WRITE_LINE_MEMBER(wavetable_irq);
        DECLARE_WRITE_LINE_MEMBER(volumeramp_irq);
        DECLARE_WRITE_LINE_MEMBER(timer1_irq);
        DECLARE_WRITE_LINE_MEMBER(timer2_irq);
        DECLARE_WRITE_LINE_MEMBER(sb_irq);
        DECLARE_WRITE_LINE_MEMBER(dma_irq);
        DECLARE_WRITE_LINE_MEMBER(drq1_w);
        DECLARE_WRITE_LINE_MEMBER(drq2_w);
        DECLARE_WRITE_LINE_MEMBER(nmi_w);

        // DMA overrides
    	virtual UINT8 dack_r(int line);
    	virtual void dack_w(int line,UINT8 data);
    	virtual void eop_w(int state);

        // optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual ioport_constructor device_input_ports() const;
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
        virtual void device_stop();
		virtual void device_config_complete() { m_shortname = "isa_gus"; }
private:
        gf1_device* m_gf1;
        acia6850_device* m_midi;

        UINT8 m_irq_status;
        attotime m_joy_time;
};

// device type definition
extern const device_type GGF1;
extern const device_type ISA16_GUS;

#endif  /* __ISA_GUS_H__ */
