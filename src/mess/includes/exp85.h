#ifndef __EXP85__
#define __EXP85__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"u100"
#define I8155_TAG		"u106"
#define I8355_TAG		"u105"

class exp85_state : public driver_device
{
public:
	exp85_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, I8085A_TAG),
		  m_terminal(*this, TERMINAL_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<serial_terminal_device> m_terminal;
	required_device<cassette_image_device> m_cassette;
	required_device<device_t> m_speaker;

	virtual void machine_start();

	DECLARE_READ8_MEMBER( i8355_a_r );
	DECLARE_WRITE8_MEMBER( i8355_a_w );
	DECLARE_READ_LINE_MEMBER( sid_r );
	DECLARE_WRITE_LINE_MEMBER( sod_w );
	DECLARE_WRITE_LINE_MEMBER( terminal_w );
	DECLARE_INPUT_CHANGED_MEMBER( trigger_reset );
	DECLARE_INPUT_CHANGED_MEMBER( trigger_rst75 );

	/* cassette state */
	int m_tape_control;
};

#endif
