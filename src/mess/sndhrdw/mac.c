/***************************************************************************
	mac.c
	Sound handler
****************************************************************************/
#include "driver.h"

static int macplus_stream;
static int mac_buffer = 1;
static UINT32 current_position = 0;
static int sample_enable = 0;

#define MAC_MAIN_BUF_OFFSET		0x0300
#define MAC_ALT_BUF_OFFSET		0x5F00
#define MAC_BUF_SIZE			( 0x2f8 >> 1 )
#define MAC_SAMPLE_RATE			( MAC_BUF_SIZE * 60 )

/************************************/
/* Stream updater                   */
/************************************/
static void macplus_sound_update( int num, INT16 *buffer, int length ) {
	UINT32 current = current_position;
	UINT8 *base = memory_region(REGION_CPU1);;

	/* if we're not enabled, just fill with 0 */
	if ( !sample_enable || Machine->sample_rate == 0)
	{
		memset( buffer, 0, length * sizeof( INT16 ) );
		return;
	}
	
	if ( mac_buffer )
		base = &base[0x400000-MAC_MAIN_BUF_OFFSET];
	else
		base = &base[0x400000-MAC_ALT_BUF_OFFSET];

	/* fill in the sample */
	while ( length-- ) {
		*buffer++ = ( READ_WORD( &base[current << 1] ) ^ 0x8000 );
		current++;
		current %= MAC_BUF_SIZE;
	}

	current_position = current;
}

/************************************/
/* Sound handler start              */
/************************************/
int macplus_sh_start( const struct MachineSound *msound ) {
	
	macplus_stream = stream_init( "Mac Sound", 100, MAC_SAMPLE_RATE, 0, macplus_sound_update );
	
	return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void macplus_sh_stop( void ) {
}

/************************************/
/* Sound handler update 			*/
/************************************/
void macplus_sh_update( void ) {
}

void macplus_enable_sound( int on ) {
	stream_update( macplus_stream, 0 );
	sample_enable = on;
}

void macplus_set_buffer( int buffer ) {
	stream_update( macplus_stream, 0 );
	mac_buffer = buffer;
}

void macplus_set_volume( int volume ) {
	stream_update( macplus_stream, 0 );
	
	volume = ( 100 / 7 ) * volume;
		
	mixer_set_volume( macplus_stream, volume );
}
