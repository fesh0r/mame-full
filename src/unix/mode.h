#ifndef __MODE_H
#define __MODE_H

#include "sysdep/rc.h"

/* mode handling functions */
void mode_set_aspect_ratio(double display_resolution_aspect_ratio);
void mode_clip_aspect(int width, int height, int *corr_width, int *corr_height);
void mode_stretch_aspect(int width, int height, int *corr_width, int *corr_height);
/* match a given mode to the needed width, height and aspect ratio to
   perfectly display a game. This function returns 0 for a not usable mode
   and 100 for the perfect mode +10 for a mode with a well matched depth and
   +20 for a mode with the perfect depth (=120 for the really perfect mode).
   
   Params:
   depth	not really the standard definition of depth, but, the depth
                for all modes, except for depth 24 32bpp sparse where 32 should be
                passed. This is done to differentiate depth 24 24bpp
                packed pixel and depth 24 32bpp sparse.
*/
int mode_match(int width, int height, int depth, int dga);

extern struct rc_option aspect_opts[];
extern struct rc_option mode_opts[];

#endif
