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
#ifndef __SYSDEP_DISPLAY_H
#define __SYSDEP_DISPLAY_H

#include "vidhrdw/vector.h" /* FIXME */
#include "begin_code.h"

struct sysdep_display_open_params {
   int widthscale;
   int heightscale;
   int scanlines;
   float aspect_ratio;
};

struct sysdep_display_prop_struct {
   int (*vector_aux_renderer)(point *start, int num_points);
   int hw_rotation;
};

#if 0
/* init / exit */
int sysdep_display_init(struct rc_struct *rc, const char *plugin_path);
void sysdep_display_exit(void);

/* create / destroy */
struct sysdep_display_struct *sysdep_display_create(const char *plugin);
void sysdep_display_destroy(struct sysdep_display_struct *display);

/* open / close */
int sysdep_display_open(struct sysdep_display_struct *display, int width,
   int height, int depth, struct sysdep_display_open_params *params,
   int params_present);
void sysdep_display_close(struct sysdep_display_struct *display);

/* blit */   
int sysdep_display_blit(struct sysdep_display_struct *display,
   struct sysdep_palette_struct *palette,
   struct sysdep_bitmap *bitmap, int src_x, int src_y, int dest_x, int dest_y,
   int src_width, int src_height, int scale_x, int scale_y, int use_dirty);
#endif

extern struct sysdep_display_prop_struct sysdep_display_properties;

#include "end_code.h"
#endif /* ifndef __SYSDEP_DISPLAY_H */
