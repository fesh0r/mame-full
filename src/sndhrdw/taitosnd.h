void taito_soundsys_setz80_soundcpu( int cpu );
void taito_soundsys_setm68k_soundcpu( int cpu );

void taito_soundsys_irq_handler( int irq );

WRITE_HANDLER( taito_soundsys_c000_w );
WRITE_HANDLER( taito_soundsys_d000_w );
READ_HANDLER( taito_soundsys_a001_r );
WRITE_HANDLER( taito_soundsys_a000_w );
WRITE_HANDLER( taito_soundsys_a001_w );
WRITE_HANDLER( taito_soundsys_adpcm_trigger_w );

WRITE_HANDLER( taito_soundsys_sound_port_w );
WRITE_HANDLER( taito_soundsys_sound_comm_w );
READ_HANDLER( taito_soundsys_sound_comm_r );
WRITE_HANDLER( taito_soundsys_sound_w );
READ_HANDLER( taito_soundsys_sound_r );


