#include "driver.h"
#include "osdepend.h"
#define NC_NUM_COLOURS 2

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

int nc_vh_start(void);
void nc_vh_stop(void);
void nc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void nc_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);

void    nc_set_card_present_state(int);
void    nc_set_card_write_protect_state(int);
int     nc_pcmcia_card_id(int);
int    nc_pcmcia_card_load(int);
void    nc_pcmcia_card_exit(int);

