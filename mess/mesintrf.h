/*********************************************************************

  mesintrf.h

  MESS supplement to usrintrf.c.

*********************************************************************/

#ifndef MESINTRF_H
#define MESINTRF_H

#include "osdepend.h"
#include "palette.h"
#include "usrintrf.h"

int mess_ui_active(void);
int handle_mess_user_interface(struct mame_bitmap *bitmap);
int ui_display_image_info(int selected);

#define machine_reset machine_hard_reset

#endif /* MESINTRF_H */
