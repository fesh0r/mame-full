#ifndef __VIDHRDW_PET_H_
#define __VIDHRDW_PET_H_
/***************************************************************************

  commodore pet discrete video circuit

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include "crtc6845.h"
#include "praster.h"

/* call to init videodriver */
extern void pet_init (UINT8 *memory);

#define pet_vh_start praster_vh_start
#define pet_vh_stop praster_vh_stop
#define pet_vh_screenrefresh praster_vh_screenrefresh
#define pet_raster_irq praster_raster_irq

//#define crtc6845_update praster_2_update
#define pet_videoram_w praster_2_videoram_w

#endif
