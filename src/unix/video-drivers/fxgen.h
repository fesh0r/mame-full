#ifndef __FXGEN_H
#define __FXGEN_H

#include "sysdep/sysdep_display.h"

int  InitGlide(void);
void ExitGlide(void);
int  InitParams(void);
int  InitVScreen(void);
void CloseVScreen(void);
void VScreenCatchSignals(void);
void VScreenRestoreSignals(void);
void UpdateFXDisplay(struct mame_bitmap *bitmap,
	  struct rectangle *dirty_area,  struct rectangle *vis_area,
	  struct sysdep_palette_struct *palette, unsigned int flags);

#endif
