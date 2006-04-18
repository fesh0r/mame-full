#include "driver.h"
#include "includes/gamecom.h"
#include "cpu/sm8500/sm8500.h"

#define Y_PIXELS 200

int scanline;

void gamecom_video_init( void ) {
	scanline = 0;
}

INTERRUPT_GEN( gamecom_scanline ) {
	scanline = ( scanline + 1 ) % Y_PIXELS;

	// draw line
}

