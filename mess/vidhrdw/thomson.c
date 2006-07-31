/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#include "math.h"
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "machine/mc6846.h"
#include "machine/thomson.h"
#include "vidhrdw/thomson.h"


#define VERBOSE 0


#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define PRINT(x) printf x


/* One GPL is what is drawn in 1 us by the video system in the active window.
   Most of the time, it corresponds to a 8-pixel wide horizontal span.
   For some TO8/9/9+/MO6 modes, it can be 4-pixel or 16-pixel wide.
   There are always 40 GPLs in the active width, and it is always defined by
   two bytes in video memory (0x2000 bytes appart).
*/

#define THOM_GPL_PER_LINE 40


/****************** dynamic screen size *****************/

static UINT16 thom_bwidth;
static UINT16 thom_bheight;
/* border size */

static UINT8  thom_hires;
/* 0 = low res: 320x200 active area (faster)
   1 = hi res:  640x400 active area (can represent 640x200 video mode)
 */

static int thom_update_screen_size( void )
{
  UINT8 p = readinputport( THOM_INPUT_VCONFIG );
  int new_w, new_h, changed = 0;
  switch ( p & 3 ) {
  case 0:  thom_bwidth = 56; thom_bheight = 47; break; /* as in original */
  case 1:  thom_bwidth = 16; thom_bheight = 16; break; /* small */
  default: thom_bwidth =  0; thom_bheight =  0; break; /* none */
  }
  thom_hires = ( p & 4 ) ? 1 : 0;

  new_w = ( 320 + thom_bwidth * 2 ) * ( thom_hires + 1 ) - 1;
  new_h = ( 200 + thom_bheight * 2 ) * (thom_hires + 1 ) - 1;
  if ( ( Machine->visible_area[0].max_x != new_w ) ||
       ( Machine->visible_area[0].max_y != new_h ) ) 
    changed = 1;
  set_visible_area( 0, 0, new_w, 0, new_h );

  return changed;
}



/*********************** video timing ******************************/

/* we use our own video timing to precisely cope with VBLANK and HBLANK */

static mame_timer* thom_video_timer; /* time elapsed from begining of frame */

/* elapsed time from begining of frame, in us */
INLINE unsigned thom_video_elapsed ( void )
{
  unsigned u;
  mame_time elapsed = mame_timer_timeelapsed( thom_video_timer );
  u = SUBSECONDS_TO_DOUBLE( elapsed.subseconds ) / TIME_IN_USEC( 1 );
  if ( u >= 19968 ) u = 19968;
  return u;
}

/* current scanline, in [0,THOM_TOTAL_HEIGHT]
   if we are in the right part of the screen, returns the next scanline
   if in VBLANK, returns THOM_TOTAL_HEIGHT
*/
INLINE unsigned thom_get_scanline ( void )
{
  unsigned l = ( thom_video_elapsed() + 27 ) >> 6;
  if ( l > THOM_TOTAL_HEIGHT ) l = THOM_TOTAL_HEIGHT;
  return l;
}

struct thom_vsignal thom_get_vsignal ( void )
{
  struct thom_vsignal v;
  int gpl = thom_video_elapsed() - 64 * THOM_BORDER_HEIGHT - 7;
  if ( gpl < 0 ) gpl += 19968;

  v.inil = ( gpl & 63 ) <= 40;

  v.init = gpl < (64 * THOM_ACTIVE_HEIGHT - 14);

  v.lt3 = ( gpl & 8 ) ? 1 : 0;

  v.line = gpl >> 6;

  v.count = v.line * 320 + ( gpl & 63 ) * 8;

  return v;
}


/************************** lightpen *******************************/

struct thom_vsignal thom_get_lightpen_vsignal ( int xdec, int ydec, int xdec2 )
{
  struct thom_vsignal v;
  int x = readinputport( THOM_INPUT_LIGHTPEN     );
  int y = readinputport( THOM_INPUT_LIGHTPEN + 1 );
  int gpl;

  if ( thom_hires ) { x /= 2; y /= 2; }
  x += xdec - thom_bwidth;
  y += ydec - thom_bheight;

  gpl = (x >> 3) + y * 64;
  if ( gpl < 0 ) gpl += 19968;
  
  v.inil = (gpl & 63) <= 41;

  v.init = (gpl <= 64 * THOM_ACTIVE_HEIGHT - 14);

  v.lt3 = ( gpl & 8 ) ? 1 : 0; 

  v.line = y;

  if ( v.inil && v.init )
    v.count = 
      ( gpl >> 6 ) * 320 +  /* line */
      ( gpl & 63 ) *   8 +  /* gpl inside line */
      ( (x + xdec2) & 7 );  /* pixel inside gpl */
  else v.count = 0;

  return v;
}


/* number of lightpen call-backs per frame */
static int thom_lightpen_nb;

/* called thom_lightpen_nb times */
static mame_timer *thom_lightpen_timer;

/* lightpen callback function to call from timer */
static void (*thom_lightpen_cb) ( int );

void thom_set_lightpen_callback ( int nb, void (*cb) ( int step ) )
{
  LOG (( "%f thom_set_lightpen_callback called\n", timer_get_time() ));
  thom_lightpen_nb = nb;
  thom_lightpen_cb = cb;
}

static void thom_lightpen_step ( int step )
{
  if ( thom_lightpen_cb ) thom_lightpen_cb( step );
  if ( step < thom_lightpen_nb )
    timer_adjust( thom_lightpen_timer, TIME_IN_USEC( 64 ), step + 1, 0 );
}


/***************** scan-line oriented engine ***********************/

/* This code, common for all Thomson machines, emulates the TO8
   video hardware, with its 16-colors chosen among 4096, 9 video modes,
   and 4 video pages. Moreover, it emulates changing the palette several times
   per frame to simulate more than 16 colors per frame (and the same for mode 
   and page switchs).

   TO7, TO7/70 and MO5 video harware are much simpler (8 or 16 fixed colors,
   one mode and one video page). Although the three are different, thay can all
   be emulated by the TO8 video hardware.
   Thus, we use the same TO8-emualtion code to deal with these simpler
   hardware (although it is somewhat of an overkill).
 */


/* ----------- state change buffer ---------- */

/* state to supplements the contents of the video memory */
struct thom_vstate {
  UINT16 pal[16]; /* palette: 16-colors among 4096 */
  UINT16 border;  /* border color index, in 0-15 */
  UINT8  mode;    /* video mode */
  UINT8  page;    /* video page shown, in 0-3 */
};

/* initial palette */
static const struct thom_vstate thom_vstate_init = { 
  { 0x1000, /* 0: black */        0x000f, /* 1: red */
    0x00f0, /* 2: geen */         0x00ff, /* 3: yellow */
    0x0f00, /* 4: blue */         0x0f0f, /* 5: purple */
    0x0ff0, /* 6: cyan */         0x0fff, /* 7: white */
    0x0777, /* 8: gray */         0x033a, /* 9: pink */
    0x03a3, /* a: light green */  0x03aa, /* b: light yellow */
    0x0a33, /* c: light blue */   0x0a3a, /* d: redish pink */
    0x0ee7, /* e: light cyan */   0x007b, /* f: orange */
  },
  0, 0, 0
};

/* buffer storing state changes, by increasing scan-line */

static struct thom_vbuf {
  struct thom_vstate s;     /* new state */
  UINT16             line;  /* scan-line at which the change occured */
} * thom_vbuf;
/* needs to be saved in snapshots */

static UINT32 thom_vbuf_last;

#define THOM_VBUF_SIZE ( THOM_TOTAL_HEIGHT + 2 )


/* Append a new state-change entry, if necessary.
   After the function return, thom_vbuf[thom_vbuf_last] can be safely modified
   to reflect the new state.
 */
INLINE void thom_vbuf_append ( void )
{
  unsigned line = thom_get_scanline();
  if ( line == thom_vbuf[thom_vbuf_last].line ) return;
  if ( line < thom_vbuf[thom_vbuf_last].line ) {
    thom_vbuf[thom_vbuf_last].line = line;
    return;
  }
  assert( thom_vbuf_last < THOM_VBUF_SIZE - 1 );
  memcpy( & thom_vbuf[thom_vbuf_last+1].s, 
	  & thom_vbuf[thom_vbuf_last].s, 
	  sizeof( struct thom_vstate )    );
  thom_vbuf_last++;
  thom_vbuf[thom_vbuf_last].line = line;
}

void thom_set_palette ( unsigned index, UINT16 color )
{
  assert( index < 16 );
  if ( color != 0x1000 ) color &= 0xfff;
  if ( thom_vbuf[thom_vbuf_last].s.pal[index] == color ) return;
  thom_vbuf_append();
  thom_vbuf[thom_vbuf_last].s.pal[index] = color;
  LOG (( "%f thom_set_palette: set %i to $%04X at line %i\n",
	 timer_get_time(), index, color, thom_vbuf[thom_vbuf_last].line ));
}

void thom_set_border_color ( unsigned index )
{
  if ( thom_vbuf[thom_vbuf_last].s.border == index ) return;
  assert( index < 16 );
  thom_vbuf_append();
  thom_vbuf[thom_vbuf_last].s.border = index;
  LOG (( "%f thom_set_border_color: %i at line %i\n",
	 timer_get_time(), index, thom_vbuf[thom_vbuf_last].line ));
}

void thom_set_video_mode ( unsigned mode )
{
  if ( thom_vbuf[thom_vbuf_last].s.mode == mode ) return;
  assert( mode < THOM_VMODE_NB );
  thom_vbuf_append();
  thom_vbuf[thom_vbuf_last].s.mode = mode;
  LOG (( "%f thom_set_video_mode: %i at line %i\n",
	 timer_get_time(), mode, thom_vbuf[thom_vbuf_last].line ));
}

void thom_set_video_page ( unsigned page )
{
  if ( thom_vbuf[thom_vbuf_last].s.page == page ) return;
  assert( page < THOM_NB_PAGES );
  thom_vbuf_append();
  thom_vbuf[thom_vbuf_last].s.page = page;
  LOG (( "%f thom_set_video_page: %i at line %i\n",
	 timer_get_time(), page, thom_vbuf[thom_vbuf_last].line ));
}


/* -------------- scan-line cache --------------- */


/* scan-line cache to back-up each scan-line in video memory */
static struct thom_vcache {

  /* cache valid except for GPLs in [dirty_min,dirty_max] */
  UINT16 cache[640];
  UINT8  dirty_min, dirty_max;

  /* state */
  UINT16 pal[16];  /* palette: 16-colors among 4096 */
  UINT8  mode;     /* video mode */
  UINT8  inherits; /* =1 if state is the same as preceding scan-line */

  /* pointer to start of line in video memory */
  UINT8* vram;

} * thom_vcache; /* no need to save this in snapshots */

INLINE void thom_dirty_line ( unsigned page, unsigned line )
{
  struct thom_vcache* v = thom_vcache + line + page * THOM_ACTIVE_HEIGHT;
  assert( page < THOM_NB_PAGES );
  assert( line < THOM_ACTIVE_HEIGHT );
  v->dirty_min = 0;
  v->dirty_max = THOM_GPL_PER_LINE - 1;
  v->inherits = 0;
}

INLINE void thom_dirty_gpl ( unsigned page, unsigned line, unsigned gpl )
{
  struct thom_vcache* v = thom_vcache + line + page * THOM_ACTIVE_HEIGHT;
  assert( page < THOM_NB_PAGES );
  assert( line < THOM_ACTIVE_HEIGHT );
  assert( gpl < THOM_GPL_PER_LINE );
  if ( v->dirty_min > gpl ) v->dirty_min = gpl;
  if ( v->dirty_max < gpl ) v->dirty_max = gpl;
}


/* -------------- drawing --------------- */

typedef void ( *thom_vcache_update ) ( struct thom_vcache* v );

#define UPDATE( name, res )					\
  static void name##_vcache_update_##res ( struct thom_vcache* v )	\
  {									\
    const int min = v->dirty_min;					\
    const int max = v->dirty_max;					\
    UINT8*  vram  = v->vram + min;					\
    UINT16* dst   = v->cache + min * res;				\
    unsigned gpl;							\
    for ( gpl = min; gpl <= max; gpl++, dst += res, vram++ ) {		\
      UINT8 rama = vram[ 0      ];					\
      UINT8 ramb = vram[ 0x2000 ];				

#define UPDATE_HI( name )  UPDATE( name, 16 )
#define UPDATE_LOW( name ) UPDATE( name,  8 )

#define END_UPDATE } }


/* 320x200, 16-colors, constraints: 2 distinct colors per GPL (8 pixels) */
/* this also works for TO7, assuming the 2 top bits of each color byte are 1 */
UPDATE_HI( to770 )
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ ((ramb & 7) | ((ramb>>4) & 8)) ^ 8 ] ];
  c[1] = Machine->pens[ v->pal[ ((ramb >> 3) & 15) ^ 8 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ rama & 1 ];
}
END_UPDATE
UPDATE_LOW( to770 )
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ ((ramb & 7) | ((ramb>>4) & 8)) ^ 8 ] ];
  c[1] = Machine->pens[ v->pal[ ((ramb >> 3) & 15) ^ 8 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1 )
    dst[ 7 - i ] = c[ rama & 1 ];
}
END_UPDATE

/* as above, different (more logical but TO7-incompatible) color encoding */
UPDATE_HI( mo5 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ ramb & 15 ] ];
  c[1] = Machine->pens[ v->pal[ ramb >> 4 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ rama & 1 ];
}
END_UPDATE
UPDATE_LOW( mo5 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ ramb & 15 ] ];
  c[1] = Machine->pens[ v->pal[ ramb >> 4 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1 )
    dst[ 7 - i ] = c[ rama & 1 ];
}
END_UPDATE

/* as to770, but with pastel color bit unswitched */
UPDATE_HI( to9 )
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ (ramb & 7) | ((ramb>>4) & 8) ] ];
  c[1] = Machine->pens[ v->pal[ (ramb >> 3) & 15 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ rama & 1 ];
}
END_UPDATE
UPDATE_LOW( to9 )
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ (ramb & 7) | ((ramb>>4) & 8) ] ];
  c[1] = Machine->pens[ v->pal[ (ramb >> 3) & 15 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1 )
    dst[ 7 - i ] = c[ rama & 1 ];
}
END_UPDATE

/* 320x200, 4-colors, no constraints */
UPDATE_HI( bitmap4 ) 
{
  int i;
  pen_t c[2][2];
  c[0][0] = Machine->pens[ v->pal[ 0 ] ];
  c[0][1] = Machine->pens[ v->pal[ 1 ] ];
  c[1][0] = Machine->pens[ v->pal[ 2 ] ];
  c[1][1] = Machine->pens[ v->pal[ 3 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1, ramb >>= 1 )
    dst[ 15 - i ] =  dst[ 14 - i ] = c[ rama & 1 ] [ ramb & 1 ];
}
END_UPDATE
UPDATE_LOW( bitmap4 ) 
{
  int i;
  pen_t c[2][2];
  c[0][0] = Machine->pens[ v->pal[ 0 ] ];
  c[0][1] = Machine->pens[ v->pal[ 1 ] ];
  c[1][0] = Machine->pens[ v->pal[ 2 ] ];
  c[1][1] = Machine->pens[ v->pal[ 3 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1, ramb >>= 1 )
    dst[ 7 - i ] = c[ rama & 1 ] [ ramb & 1 ];
}
END_UPDATE

/* as above, but using 2-bit pixels instead of 2 planes of 1-bit pixels  */
UPDATE_HI( bitmap4alt ) 
{
  int i;
  pen_t c[4];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 1 ] ];
  c[2] = Machine->pens[ v->pal[ 2 ] ];
  c[3] = Machine->pens[ v->pal[ 3 ] ];
  for ( i = 0; i < 8; i += 2, ramb >>= 2 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ ramb & 3 ];
  for ( i = 0; i < 8; i += 2, rama >>= 2 )
    dst[ 7 - i ] = dst[ 6 - i ] = c[ rama & 3 ];
}
END_UPDATE
UPDATE_LOW( bitmap4alt ) 
{
  int i;
  pen_t c[4];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 1 ] ];
  c[2] = Machine->pens[ v->pal[ 2 ] ];
  c[3] = Machine->pens[ v->pal[ 3 ] ];
  for ( i = 0; i < 4; i++, ramb >>= 2 )
    dst[ 7 - i ] = c[ ramb & 3 ];
  for ( i = 0; i < 4; i++, rama >>= 2 )
    dst[ 3 - i ] = c[ rama & 3 ];
}
END_UPDATE

/* 160x200, 16-colors, no constraints */
UPDATE_HI( bitmap16 ) 
{
  dst[ 0] = dst[ 1] = dst[ 2] = dst[ 3] = Machine->pens[ v->pal[ rama >> 4 ] ];
  dst[ 4] = dst[ 5] = dst[ 6] = dst[ 7] = Machine->pens[ v->pal[ rama & 15 ] ];
  dst[ 8] = dst[ 9] = dst[10] = dst[11] = Machine->pens[ v->pal[ ramb >> 4 ] ];
  dst[12] = dst[13] = dst[14] = dst[15] = Machine->pens[ v->pal[ ramb & 15 ] ];
}
END_UPDATE
UPDATE_LOW( bitmap16 ) 
{
  dst[0] = dst[1] = Machine->pens[ v->pal[ rama >> 4 ] ];
  dst[2] = dst[3] = Machine->pens[ v->pal[ rama & 15 ] ];
  dst[4] = dst[5] = Machine->pens[ v->pal[ ramb >> 4 ] ];
  dst[6] = dst[7] = Machine->pens[ v->pal[ ramb & 15 ] ];
}
END_UPDATE

/* 640x200 (80 text column), 2-colors, no constraints */
UPDATE_HI( mode80 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 1 ] ];
  for ( i = 0; i < 8; i++, ramb >>= 1 )
    dst[ 15 - i ] = c[ ramb & 1 ];
  for ( i = 0; i < 8; i++, rama >>= 1 )
    dst[  7 - i ] = c[ rama & 1 ];
}
END_UPDATE
UPDATE_LOW( mode80 ) 
{
  /* merge pixels */
  int i;
  pen_t c[4];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = c[2] = c[3] = Machine->pens[ v->pal[ 1 ] ];
  for ( i = 0; i < 4; i++, ramb >>= 2 )
    dst[ 7 - i ] = c[ ramb & 3 ];
  for ( i = 0; i < 4; i++, rama >>= 2 )
    dst[ 3 - i ] = c[ rama & 3 ];
}
END_UPDATE

/* 320x200, 2-colors, two pages (untested) */
UPDATE_HI( page1 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 1 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ rama & 1 ];
  (void)ramb;
}
END_UPDATE
UPDATE_LOW( page1 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 1 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1 )
    dst[ 7 - i ] = c[ rama & 1 ];
  (void)ramb;
}
END_UPDATE

UPDATE_HI( page2 ) 
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 2 ] ];
  for ( i = 0; i < 16; i += 2, ramb >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = c[ ramb & 1 ];
  (void)rama;
}
END_UPDATE
UPDATE_LOW( page2 )
{
  int i;
  pen_t c[2];
  c[0] = Machine->pens[ v->pal[ 0 ] ];
  c[1] = Machine->pens[ v->pal[ 2 ] ];
  for ( i = 0; i < 8; i++, ramb >>= 1 )
    dst[ 7 - i ] = c[ ramb & 1 ];
  (void)rama;
}
END_UPDATE

/* 320x200, 2-colors, two overlaid pages (untested) */
UPDATE_HI( overlay ) 
{
  int i;
  pen_t c[2][2];
  c[0][0] = Machine->pens[ v->pal[ 0 ] ];
  c[0][1] = c[1][1] = Machine->pens[ v->pal[ 1 ] ];
  c[1][0] = Machine->pens[ v->pal[ 2 ] ];
  for ( i = 0; i < 16; i += 2, rama >>= 1, ramb >>= 1 )
    dst[ 15 - i ] =  dst[ 14 - i ] = c[ ramb & 1 ] [ rama & 1 ];
}
END_UPDATE
UPDATE_LOW( overlay ) 
{
  int i;
  pen_t c[2][2];
  c[0][0] = Machine->pens[ v->pal[ 0 ] ];
  c[0][1] = c[1][1] = Machine->pens[ v->pal[ 1 ] ];
  c[1][0] = Machine->pens[ v->pal[ 2 ] ];
  for ( i = 0; i < 8; i++, rama >>= 1, ramb >>= 1 )
    dst[ 7 - i ] = c[ ramb & 1 ] [ rama & 1 ];
}
END_UPDATE

/* 160x200, 4-colors, four overlaid pages (untested) */
UPDATE_HI( overlay3 ) 
{
  static const int p[2][2][2][2] = 
    { { { { 0, 1 }, { 2, 1 }, }, { { 4, 1 }, { 2, 1 } } }, 
      { { { 8, 1 }, { 2, 1 }, }, { { 4, 1 }, { 2, 1 } } } };
  int i;
  for ( i = 0; i < 16; i += 4, rama >>= 1, ramb >>= 1 )
    dst[ 15 - i ] = dst[ 14 - i ] = dst[ 13 - i ] = dst[ 12 - i ] = 
      Machine->pens[ v->pal[ p[ ramb & 1 ] [ (ramb >> 4) & 1 ] 
 			      [ rama & 1 ] [ (rama >> 4) & 1 ] ] ];
}
END_UPDATE
UPDATE_LOW( overlay3 ) 
{
  static const int p[2][2][2][2] = 
    { { { { 0, 1 }, { 2, 1 }, }, { { 4, 1 }, { 2, 1 } } }, 
      { { { 8, 1 }, { 2, 1 }, }, { { 4, 1 }, { 2, 1 } } } };
  int i;
  for ( i = 0; i < 8; i += 2, rama >>= 1, ramb >>= 1 )
    dst[ 7 - i ] = dst[ 6 - i ] =
      Machine->pens[ v->pal[ p[ ramb & 1 ] [ (ramb >> 4) & 1 ] 
 			      [ rama & 1 ] [ (rama >> 4) & 1 ] ] ];
}
END_UPDATE

#define FUN(x) { x##_vcache_update_8, x##_vcache_update_16 }

const thom_vcache_update thom_vcache_funcs[THOM_VMODE_NB][2] = {
  FUN(to770),    FUN(mo5),    FUN(bitmap4), FUN(bitmap4alt),  FUN(mode80),  
  FUN(bitmap16), FUN(page1),  FUN(page2),   FUN(overlay),     FUN(overlay3),
  FUN(to9)
};


/* -------------- misc --------------- */

static UINT32 thom_mode_point;
static UINT8  thom_led;

static UINT32 thom_floppy_wcount;
static UINT32 thom_floppy_rcount;


void thom_set_mode_point ( int point )
{
  assert( point >= 0 && point <= 1 );
  thom_mode_point = ( ! point ) * 0x2000;
  memory_set_bank( THOM_VRAM_BANK, ! point );
}

void thom_set_caps_led ( int led )
{
  thom_led = led;
}

void thom_floppy_icon ( int write )
{
  /* stays up for a few frames*/
  if ( write ) thom_floppy_wcount = 25; 
  else         thom_floppy_rcount = 25;
}

typedef enum { OSD_NORMAL, OSD_ON, OSD_OFF } osd_mode;

/* -------------- main update function --------------- */

/* relevant scan-line state from the last frame update */
static struct thom_view {
  pen_t border;
  UINT8 page;
} * thom_view;
/* no need to save this in snapshots */

static mame_bitmap* thom_screen; 
/* no need to save this in snapshots */


VIDEO_UPDATE ( thom )
{
  const int f = thom_hires ? 2 : 1;
  rectangle rect_left  = { 0, thom_bwidth * f - 1, 0, 0 };
  rectangle rect_right = 
    { (thom_bwidth+320) * f, (thom_bwidth*2+320)*f - 1, 0, 0 };
  rectangle rect_full  = { 0, (thom_bwidth*2+320)*f - 1, 0, 0 };
  int c=0, y=0, l=0, ypos=0;
  int bupdate = 0, wupdate = 0, gupdate = 0; /* profile */

  /* we assume update is called only at the end of a frame (not necessarily at
     the end of each frame) 
  */
  assert( cliprect->max_y == ( thom_bheight*2 + 200 ) * (thom_hires+1) - 1 );

  /* append a sentinel */
  thom_vbuf[thom_vbuf_last+1].line = 999;

  /* update upper border */
  y += THOM_BORDER_HEIGHT - thom_bheight;
  while ( 1 ) {
    pen_t border = Machine->pens[ thom_vbuf[c].s.pal[ thom_vbuf[c].s.border ] ];
    for (; y < thom_vbuf[c+1].line; y++, ypos += f ) {
      if ( y == THOM_BORDER_HEIGHT ) goto end_up;
      if ( border != thom_view[y].border ) {
	/* update border */
	thom_view[y].border = border;
	rect_full.min_y = ypos; 
	rect_full.max_y = ypos + thom_hires;
	fillbitmap( thom_screen, border, &rect_full );
	bupdate++;
      }
    }
    c++;
  }
 end_up:

  /* update active area */
  while ( 1 ) {
    pen_t border = Machine->pens[ thom_vbuf[c].s.pal[ thom_vbuf[c].s.border ] ];
    unsigned mode = thom_vbuf[c].s.mode;
    unsigned page = thom_vbuf[0].s.page;
    struct thom_vcache* v = thom_vcache + page * THOM_ACTIVE_HEIGHT + l;
    unsigned inherits = 0;

    for (; y < thom_vbuf[c+1].line; y++, l++, v++, ypos += f ) {

      if ( y == THOM_BORDER_HEIGHT + 200 ) goto end_active;

      if ( border != thom_view[y].border ) {
	/* update border */
	thom_view[y].border = border;
	rect_left .min_y = rect_right.min_y = ypos;
	rect_left .max_y = rect_right.max_y = ypos + thom_hires;
	fillbitmap( thom_screen, border, &rect_left  );
	fillbitmap( thom_screen, border, &rect_right );
	bupdate++;
      }
      if ( ( inherits && v->inherits ) ||
	   ( !memcmp( v->pal, thom_vbuf[c].s.pal, 32 ) &&
	     mode == v->mode && 
	     page == thom_view[y].page ) ) {
	/* same palette / mode / page */
	v->inherits = inherits;
	inherits = 1;
      }
      else {
	/* palette / mode / page modified => full scan-line update */
	memcpy( v->pal, thom_vbuf[c].s.pal, 32 );
	v->mode = mode;
	thom_view[y].page = page;
	v->inherits = 0;
	inherits = 0;
 	v->dirty_min = 0;
	v->dirty_max = THOM_GPL_PER_LINE - 1;
	wupdate++;
      }

      if ( v->dirty_min <= v->dirty_max ) {
	/* scan-line update */
	gupdate += v->dirty_max - v->dirty_min + 1;
	thom_vcache_funcs[mode][thom_hires] ( v );
	draw_scanline16( thom_screen, 
			 (thom_bwidth + v->dirty_min * 8) * f, ypos,
			 (v->dirty_max - v->dirty_min + 1) * 8 * f,
			 v->cache + v->dirty_min * 8 *f, NULL, -1 );
	if ( thom_hires )
	  draw_scanline16( thom_screen, 
			   (thom_bwidth + v->dirty_min * 8) * f, ypos + 1,
			   (v->dirty_max - v->dirty_min + 1) * 8 * f,
			   v->cache + v->dirty_min * 8 *f, NULL, -1 );
	v->dirty_min = THOM_GPL_PER_LINE;
	v->dirty_max = 0;
      }
    }
    c++;
  }
 end_active:

  /* update lower border */
  while ( 1 ) {
    pen_t border = Machine->pens[ thom_vbuf[c].s.pal[ thom_vbuf[c].s.border ] ];
    for (; y < thom_vbuf[c+1].line; y++, ypos += f ) {
      if ( y == THOM_BORDER_HEIGHT + thom_bheight + 200 ) goto end_down;
      if ( border != thom_view[y].border ) {
	/* update border */
	thom_view[y].border = border;
	rect_full.min_y = ypos;
	rect_full.max_y = ypos + thom_hires;
	fillbitmap( thom_screen, border, &rect_full );
	bupdate++;
      }
    }
    c++;
  }
  end_down:

  /* final copy */
  copybitmap( bitmap, thom_screen, 0, 0, 0, 0, NULL, TRANSPARENCY_NONE, 0 );

	/* config */
	if ( readinputport( THOM_INPUT_CONFIG ) & 1 ) 
	{
	}
	else
	{
		draw_crosshair( bitmap, 
			readinputport ( THOM_INPUT_LIGHTPEN     ), 
			readinputport ( THOM_INPUT_LIGHTPEN + 1 ), 
			cliprect, 0 ); 
	}
 
	LOG(( "thom_video_update: %i borders, %i whole lines, %i gpls\n",
		bupdate, wupdate, gupdate ));

	return 0;
}



/* -------------- frame start ------------------ */

static mame_timer *thom_init_timer;

static void (*thom_init_cb) ( int );

void thom_set_init_callback ( void (*cb) ( int init ) )
{
  LOG (( "thom_set_init_callback called\n" ));
  thom_init_cb = cb;
}

static void thom_set_init ( int init )
{
  LOG (( "%f thom_set_init: %i\n", timer_get_time(), init ));
  if ( thom_init_cb ) thom_init_cb( init );
  if ( ! init )
    timer_adjust( thom_init_timer, TIME_IN_USEC( 64 * THOM_ACTIVE_HEIGHT - 24 ),
		  1-init, 0 );
}

/* call this at the very begining of each new frame */
VIDEO_EOF ( thom )
{
  struct thom_vsignal l = thom_get_lightpen_vsignal( 0, -1, 0 );
  
  LOG (( "%f thom: video eof called, %i state change(s)\n", 
	 timer_get_time(), thom_vbuf_last ));

  /* floppy indicator count */
  if ( thom_floppy_wcount ) thom_floppy_wcount--;
  if ( thom_floppy_rcount ) thom_floppy_rcount--;

  /* reset video frame time */
  mame_timer_adjust( thom_video_timer, time_zero, 0, time_never );

  /* reset video state change buffer */
  if ( thom_vbuf_last ) {
    memcpy( & thom_vbuf[0].s, 
	    & thom_vbuf[thom_vbuf_last].s, 
	    sizeof( struct thom_vstate )    );
    thom_vbuf_last = 0;
  }

  /* schedule first init signal */
  timer_adjust( thom_init_timer, TIME_IN_USEC( 64 * THOM_BORDER_HEIGHT + 7 ), 
		0, 0 );

  /* schedule first lightpen signal */
  l.line &= ~1; /* hack (avoid lock in MO6 palette selection) */
  timer_adjust( thom_lightpen_timer,
		TIME_IN_USEC( 64 * ( THOM_BORDER_HEIGHT + l.line - 2 ) + 16 ),
		0, 0 );

  /* update screen size according to user options */
  if ( thom_update_screen_size() )
    /* need to invalidate cache */
    thom_video_postload ();
}




/* -------------- initialization --------------- */

VIDEO_START ( thom )
{
  unsigned i, j;

  LOG (( "thom: video start called\n" ));

  thom_screen = 
    auto_bitmap_alloc( THOM_TOTAL_WIDTH * 2, THOM_TOTAL_HEIGHT * 2 );

  thom_vcache = auto_malloc( sizeof( struct thom_vcache ) * 
			     THOM_ACTIVE_HEIGHT * THOM_NB_PAGES );

   for ( j = 0; j < THOM_NB_PAGES; j++ )
    for ( i = 0; i < THOM_ACTIVE_HEIGHT; i++)
      thom_vcache[i + j * THOM_ACTIVE_HEIGHT].vram = 
	thom_vram + i * 40 + j * 0x4000;

  thom_view = auto_malloc( sizeof( struct thom_view ) * THOM_TOTAL_HEIGHT );
  memset( thom_view, 0, sizeof( struct thom_view ) * THOM_TOTAL_HEIGHT );

  thom_vbuf = auto_malloc( sizeof( struct thom_vbuf ) * THOM_VBUF_SIZE );
  memset( thom_vbuf, 0, sizeof( struct thom_vbuf ) * THOM_VBUF_SIZE );
 
  thom_vbuf_last = 0;
  thom_vbuf[0].line = 0;
  memcpy( & thom_vbuf[0].s, & thom_vstate_init, sizeof( struct thom_vstate ) );
 
  state_save_register_global( thom_vbuf_last );
  for ( i = 0; i < THOM_VBUF_SIZE; i++ ) {
    state_save_register_item_array( "globals", i, thom_vbuf[i].s.pal );
    state_save_register_item( "globals", i, thom_vbuf[i].s.border );
    state_save_register_item( "globals", i, thom_vbuf[i].s.mode );
    state_save_register_item( "globals", i, thom_vbuf[i].s.page );
    state_save_register_item( "globals", i, thom_vbuf[i].line );
  }

  thom_mode_point = 0;
  state_save_register_global( thom_mode_point );
  memory_set_bank( THOM_VRAM_BANK, 0 );

  thom_led = -1;
  state_save_register_global( thom_led );

  thom_floppy_rcount = 0;
  thom_floppy_wcount = 0;
  state_save_register_global( thom_floppy_wcount );
  state_save_register_global( thom_floppy_rcount );

  state_save_register_func_postload( thom_video_postload );  
  thom_video_postload ();

  thom_video_timer = mame_timer_alloc( NULL );

  thom_lightpen_nb = 0;
  thom_lightpen_cb = NULL;
  thom_lightpen_timer = mame_timer_alloc( thom_lightpen_step );

  thom_init_cb = NULL;
  thom_init_timer = mame_timer_alloc( thom_set_init );
  
  video_eof_thom();

  state_save_register_global( thom_bwidth );
  state_save_register_global( thom_bheight );
  state_save_register_global( thom_hires );

  return 0;
}

/* wipe-up video cache (not video memory nor state buffer) */
void thom_video_postload ( void )
{
  int i, j;
  fillbitmap( thom_screen, Machine->pens[0], NULL );

  for ( i = 0; i < THOM_TOTAL_HEIGHT; i++ ) {
    thom_view[i].border = Machine->pens[0];
    thom_view[i].page = -1;
  }
  
  for ( j = 0; j < THOM_NB_PAGES; j++ )
    for ( i = 0; i < THOM_ACTIVE_HEIGHT; i++)
      thom_dirty_line( j, i );
}

PALETTE_INIT ( thom )
{
  float gamma = 0.6;
  unsigned i;

  LOG (( "thom: palette init called\n" ));

  for ( i = 0; i < 4097; i++ )  {
    UINT8 r = 255. * pow( (i & 15) / 15., gamma );
    UINT8 g = 255. * pow( ((i>> 4) & 15) / 15., gamma );
    UINT8 b = 255. * pow( ((i >> 8) & 15) / 15., gamma );
    UINT8 alpha = i & 0x1000 ? 0 : 255;
    palette_set_color( i, r, g, b );
    (void)alpha; /* TODO: transparency */
  }
}


/***************************** TO7 / T9000 *************************/

/* write to video memory through addresses 0x4000-0x5fff */
WRITE8_HANDLER ( to7_vram_w )
{
  assert( offset >= 0 && offset < 0x2000 );
  /* force two topmost color bits to 1 */
  if ( thom_mode_point ) data |= 0xc0;
  if ( thom_vram[ offset + thom_mode_point ] == data ) return;
  thom_vram[ offset + thom_mode_point ] = data;
  /* dirty visible change */
  if (offset >= 8000) return;
  thom_dirty_gpl( 0, offset / 40, offset % 40 );
}

/* bits 0-13 : latched gpl of lightpen position */
/* bit    14:  latched INIT */
/* bit    15:  latched INIL */
unsigned to7_lightpen_gpl ( int decx, int decy )
{
  int x = readinputport( THOM_INPUT_LIGHTPEN     );
  int y = readinputport( THOM_INPUT_LIGHTPEN + 1 );
  if ( thom_hires ) { x /= 2; y /= 2; }
  x -= thom_bwidth;
  y -= thom_bheight;
  if ( x < 0 || y < 0 || x >= 320 || y >= 200 ) return 0;
  x += decx;
  y += decy;
  return y*40 + x/8 + (x < 320 ? 0x4000 : 0) + 0x8000;
}



/***************************** TO7/70 ******************************/

/* write to video memory through addresses 0x4000-0x5fff (TO) 
   or 0x0000-0x1fff (MO) */
WRITE8_HANDLER ( to770_vram_w )
{
  assert( offset >= 0 && offset < 0x2000 );
  if ( thom_vram[ offset + thom_mode_point ] == data ) return;
  thom_vram[ offset + thom_mode_point ] = data;
  /* dirty visible change */
  if ( offset >= 8000 ) return;
  thom_dirty_gpl( 0, offset / 40, offset % 40 );
}


/***************************** TO8 ******************************/

/* write to video memory through system space (always page 1) */
WRITE8_HANDLER ( to8_sys_lo_w )
{
  UINT8* dst = thom_vram + offset + 0x6000;
  assert( offset >= 0 && offset < 0x2000 );
  if ( *dst == data ) return;
  *dst = data;
  /* dirty visible change */
  if ( offset >= 8000 ) return;
  thom_dirty_gpl( 1, offset / 40, offset % 40 );
}

WRITE8_HANDLER ( to8_sys_hi_w )
{
  UINT8* dst = thom_vram + offset + 0x4000;
  assert( offset >= 0 && offset < 0x2000 );
  if ( *dst == data ) return;
  *dst = data;
  /* dirty visible change */
  if ( offset >= 8000 ) return;
  thom_dirty_gpl( 1, offset / 40, offset % 40 );
}

/* write to video memory through data space */
WRITE8_HANDLER ( to8_data_lo_w )
{
  UINT8* dst = thom_vram + offset + 0x4000 * to8_data_vpage + 0x2000;
  assert( offset >= 0 && offset < 0x2000 );
  if ( *dst == data ) return;
  *dst = data;
  /* dirty visible change */
  if ( to8_data_vpage >= 4 || offset >= 8000 ) return;
  thom_dirty_gpl( to8_data_vpage, offset / 40, offset % 40 );
}

WRITE8_HANDLER ( to8_data_hi_w )
{
  UINT8* dst = thom_vram + offset + 0x4000 * to8_data_vpage;
  assert( offset >= 0 && offset < 0x2000 );
  if ( *dst == data ) return;
  *dst = data;
  /* dirty visible change */
  if ( to8_data_vpage >= 4 || offset >= 8000 ) return;
  thom_dirty_gpl( to8_data_vpage, offset / 40, offset % 40 );
}

/* write to video memory page through cartridge addresses space */
WRITE8_HANDLER ( to8_vcart_w )
{
  UINT8* dst = thom_vram + offset + 0x4000 * to8_cart_vpage;
  assert( offset>=0 && offset < 0x4000 );
  if ( *dst == data ) return;
  *dst = data;
  /* dirty visible change */
  offset &= 0x1fff;
  if ( to8_cart_vpage >= 4 || offset >= 8000 ) return;
  thom_dirty_gpl( to8_cart_vpage, offset / 40, offset % 40 );
}
