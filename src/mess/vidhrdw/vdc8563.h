#ifndef __VDC8563_H_
#define __VDC8563_H_
/***************************************************************************

  CBM Video Device Chip 8563

***************************************************************************/

/*
 * if you need this chip in another mame/mess emulation than let it me know
 * I will split this from the c128 driver
 * peter.trauner@jk.uni-linz.ac.at
 * 16. november 1999
 * look at mess/systems/c128.c and mess/machine/c128.c
 * on how to use it
 */

/* only enough to boot c128 kernal */

#if 0
/* call to init videodriver */
extern void vic6560_init (int (*dma_read) (int), int (*dma_read_color) (int));
extern void vic6561_init (int (*dma_read) (int), int (*dma_read_color) (int));

extern int vic6560_vh_start (void);
extern void vic6560_vh_stop (void);
extern void vic6560_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
extern unsigned char vic6560_palette[16 * 3];

int vic656x_raster_interrupt (void);

#endif

/* to be called when writting to port */
extern void vdc8563_port_w (int offset, int data);

/* to be called when reading from port */
extern int vdc8563_port_r (int offset);

#endif
