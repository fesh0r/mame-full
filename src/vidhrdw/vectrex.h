#ifndef VECTREX_V_H
#define VECTREX_V_H

#define VEC_PAL_BW          0
#define VEC_PAL_BZONE       2

#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE

#define VEC_SHIFT 16
#define VECTREX_SCALE 10
#define MAX_VECTORS 10000	/* Maximum # of points we can queue in a vector list */
#define TTL 65000               /* Number of cycles a vector persists on the screen */

void stop_vectrex(void);
int start_vectrex (void);
void vectrex_add_point(int scale, int x, int y, int offs, int pattern, int brightness);
void vectrex_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void vectrex_zero_integrators(int offset);
void vectrex_vh_update (struct osd_bitmap *bitmap, int full_refresh);
void vectrex_add_point_dot(int brightness);
void vectrex_screen_update (void);

#endif