/*********************************************************************

  mesintrf.h

  MESS supplement to usrintrf.c.

*********************************************************************/

#ifndef MESINTRF_H
#define MESINTRF_H

#include "osdepend.h"
#include "usrintrf.h"

int mess_ui_active();
int handle_mess_user_interface(struct mame_bitmap *bitmap);
int displayimageinfo(struct mame_bitmap *bitmap, int selected);

#define machine_reset machine_hard_reset

#endif /* MESINTRF_H */
