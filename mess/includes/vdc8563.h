#ifndef __VDC8563_H_
#define __VDC8563_H_
/***************************************************************************

  CBM Video Device Chip 8563

  copyright 2000 peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include "praster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* call to init videodriver */
extern void vdc8563_init (int ram16konly);

extern void vdc8563_set_rastering(int on);

extern int vdc8563_vh_start (void);
void vdc8563_vh_stop(void);
	void vdc8563_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern unsigned char vdc8563_palette[16 * 3];

/* to be called when writting to port */
extern WRITE_HANDLER ( vdc8563_port_w );

/* to be called when reading from port */
extern READ_HANDLER ( vdc8563_port_r );

extern void vdc8563_state (void);

#ifdef __cplusplus
}
#endif

#endif
