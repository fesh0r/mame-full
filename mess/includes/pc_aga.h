/*
  pc cga/mda combi adapters

  one type hardware switchable between cga and mda/hercules
  another type software switchable between cga and mda/hercules

  some support additional modes like
  commodore pc10 320x200 in 16 colors
*/

#include "driver.h"

typedef enum AGA_MODE { AGA_OFF, AGA_COLOR, AGA_MONO } AGA_MODE;
void pc_aga_set_mode(AGA_MODE mode);

extern void pc_aga_timer(void);
extern int  pc_aga_vh_start(void);
extern void pc_aga_vh_stop(void);
extern void pc_aga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern WRITE_HANDLER ( pc_aga_videoram_w );
READ_HANDLER( pc_aga_videoram_r );

extern WRITE_HANDLER ( pc200_videoram_w );
READ_HANDLER( pc200_videoram_r );

extern WRITE_HANDLER ( pc_aga_mda_w );
extern READ_HANDLER ( pc_aga_mda_r );
extern WRITE_HANDLER ( pc_aga_cga_w );
extern READ_HANDLER ( pc_aga_cga_r );

extern WRITE_HANDLER( pc200_cga_w );
extern READ_HANDLER ( pc200_cga_r );
extern void pc200_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

