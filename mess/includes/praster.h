#ifndef __PRASTER_H_
#define __PRASTER_H_
/***************************************************************************

  Pet's Rasterengine

  * first design goal, allow usage for two or more cathod ray tube in 1 driver
    use praster1 for first and praster2 for second video adapter
    if needed add praster3 and so on

  * allow implementation and usage of different rastering, caching
    strategies without change of video chip emulation

  * use same rastering, caching strategies for several chip with this driver

***************************************************************************/

/* insert following into the Machine structure */
extern int praster_vh_start (void);
extern void praster_vh_stop (void);
extern void praster_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
int praster_raster_irq (void);
int praster_vretrace_irq (void);


typedef enum {
	PRASTER_MONOTEXT, PRASTER_TEXT, PRASTER_GRAPHIC,
	PRASTER_GFXTEXT
} PRASTER_MODE;
/*
  text, monotext
  text.textsize.x number of char columns on screen
  text.textsize.y number of char lines on screen
  text.charsize.x text.charsize.y size of a char

  text.fontsize.y number of bytes in fontram per character
  text.vissize.x text.vissize.y size of the character bitmap used from fontram
*/

/* basic structure for 1 cathod ray tube */
typedef struct _PRASTER {
	int on;
	void (*display_state)(struct _PRASTER *this); /* calls machine after rastering frame*/
    /* for rasterline, rastercolumn, and lightpen positions */
	struct {
		struct { int x, y; } size; /* Pixels */
		struct { int y; } current;
		struct { int x, y; } screenpos;
		UINT16 framecolor;
		UINT16 backgroundcolor;
	} raytube;

	struct {
		int no_rastering; /* save time, and do not write to bitmap */
		struct osd_bitmap *bitmap; /* output for rastered image */
/*		struct { int x, y; } pos;    left upper position to be rendered in     */
/*		struct { int x, y; } size;    size to be rendered    */
		UINT16 *pens;
	} display;

	PRASTER_MODE mode; /* of the raster engine */

	struct {
		int reverse;
		struct { int x, y; } size;
		struct { int x, y; } charsize;
		struct { int y; } fontsize;
		struct { int x, y; } visiblesize;
		UINT8 *dirtybuffer;
	} text;

	struct {
		struct { int x, y; } size;
	} graphic;

	/* memory layout */
	struct {
		UINT8 *ram;
		int mask;
		struct { int offset, mask, size; } videoram, colorram, fontram;
	} memory;

	int linediff;
	UINT16 monocolor[2];

	struct {
		int on;
		int pos; /* position in text screen */
		int ybegin, yend; /* first charline filled with attr color, last charline */

		int blinking;
		int delay; /* blinkdelay in vertical retraces */
		int counter; /* delay counter */
		int displayed; /* cursor displayed */
	} cursor;

	/* private area */
} PRASTER;

extern PRASTER raster1, raster2;

extern void praster_1_init (void);
extern void praster_2_init (void);

extern void praster_1_update(void);
extern void praster_2_update(void);
extern void praster_1_cursor_update(void);
extern void praster_2_cursor_update(void);

extern WRITE_HANDLER ( praster_1_videoram_w );
extern WRITE_HANDLER ( praster_2_videoram_w );
extern READ_HANDLER  ( praster_1_videoram_r );
extern READ_HANDLER  ( praster_2_videoram_r );


/* use to draw text in display_state function */
extern void praster_draw_text (PRASTER *this, char *text, int *y);

#endif
