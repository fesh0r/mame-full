#ifndef __HD6845S_HEADER_INCLUDED__
#define __HD6845S_HEADER_INCLUDED__

#define HD6845S_TOTAL_LINES	312

/* defines for register names */
#define HD6845S_H_TOT		0
#define HD6845S_H_DISP		1
#define HD6845S_H_SYNC_POS	2
#define HD6845S_SYNC		3
#define HD6845S_V_TOT		4
#define HD6845S_V_ADJ		5
#define HD6845S_V_DISP		6
#define HD6845S_V_SYNC_POS	7
#define HD6845S_INTERLACE	8
#define HD6845S_MAX_RASTER	9
#define HD6845S_CURSOR_START	10
#define HD6845S_CURSOR_END	11
#define HD6845S_SCREEN_ADDR_H	12
#define HD6845S_SCREEN_ADDR_L	13
#define HD6845S_CURSOR_ADDR_H	14
#define HD6845S_CURSOR_ADDR_L	15
#define HD6845S_LIGHTPEN_H	16
#define HD6845S_LIGHTPEN_L	17

#define HD6845S_STATE		18
#define HD6845S_MA              19
#define HD6845S_RA              20
#define HD6845S_LC              21

/* set if display vertical chars, cleared
if inhibit display */
#define HD6845S_VDISP		0x0001

#define HD6845S_MONITOR_HSYNC_LEN       8

typedef struct {
	/* selected register index */
        unsigned char    RegIndex;

    /* in register order */
        int    Registers[17];

	/* raster counter */
        unsigned char    RasterCount;
	/* horizontal char counter */
        unsigned char    HorizontalCount;
	/* line counter */
        unsigned char    LineCounter;
	/* vertical adjust counter */
        unsigned char    VerticalAdjustCount;
//	/* horizontal sync counter */
//      unsigned char    HorizontalSyncCount;
} CRTC6845;

extern  int     hd6845s_vh_start(void);
extern  void    hd6845s_vh_stop(void);

extern  int     hd6845s_index_r(void);
extern  int     hd6845s_register_r(void);

extern  void    hd6845s_index_w(int data);
extern  void    hd6845s_register_w(int data);
extern int 	hd6845s_getreg(int);

extern void     hd6845s_update_clocks(int clocks);

#endif
