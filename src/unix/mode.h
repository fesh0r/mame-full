#ifndef __MODE_H
#define __MODE_H

#include "sysdep/rc.h"

/* mode handling functions */
void mode_check_params(double display_resolution_aspect_ratio);
int mode_match(int width, int height, int depth, int dga);
void mode_clip_aspect(int width, int height, int *corr_width, int *corr_height);
void mode_stretch_aspect(int width, int height, int *corr_width, int *corr_height);

extern struct rc_option aspect_opts[];
extern struct rc_option mode_opts[];

#endif
