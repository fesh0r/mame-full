/* Sysdep display object

   Copyright 2000-2004 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __SYSDEP_DISPLAY_PRIV_H
#define __SYSDEP_DISPLAY_PRIV_H

#include "mode.h"
#include "effect.h"
#include "video-drivers/blit_funcs.h"
#include "sysdep/sysdep_display.h"
#include "begin_code.h"

/* from sysdep_display.c */
extern struct sysdep_display_open_params sysdep_display_params;

void sysdep_display_orient_bounds(struct rectangle *bounds, int width, int height);
void sysdep_display_check_bounds(struct mame_bitmap *bitmap, struct rectangle *vis_in_dest_out, struct rectangle *dirty_area, int x_align);

/* from the sysdep display driver */
int  sysdep_display_driver_open(int reopen);
void sysdep_display_driver_close(void);
int  sysdep_display_driver_resize(void);
int  sysdep_display_driver_update_keyboard(void);

#include "end_code.h"
#endif /* ifndef __SYSDEP_DISPLAY_PRIV_H */
