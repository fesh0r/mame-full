#ifndef __CRTC6845_H_
#define __CRTC6845_H_
/***************************************************************************

  motorola cathode ray tube controller 6845

  simple version, for a cycle exact framework look at m6845.h

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include "mame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 14 address lines, 
	5 character line address lines */

// support for more chips
// cga and mda could be in a PC at the same time
struct _CRTC6845;
// static instance used by standard handlers, use it to call functions
extern struct _CRTC6845 *crtc6845;

typedef struct {
	int on;
	int pos;
	int top;
	int bottom;
} CRTC6845_CURSOR;

typedef struct {
	int freq;
	void (*cursor_changed)(CRTC6845_CURSOR *old);
} CRTC6845_CONFIG;

void crtc6845_init(struct _CRTC6845 *crtc, CRTC6845_CONFIG *config);
void crtc6845_set_clock(struct _CRTC6845 *crtc, int freq);

// to be called before drawing screen
void crtc6845_time(struct _CRTC6845 *crtc);

int crtc6845_do_full_refresh(struct _CRTC6845 *crtc);

int crtc6845_get_char_columns(struct _CRTC6845 *crtc);
int crtc6845_get_char_height(struct _CRTC6845 *crtc);
int crtc6845_get_char_lines(struct _CRTC6845 *crtc);
int crtc6845_get_start(struct _CRTC6845 *crtc);

/* cursor off, cursor on, cursor 16 frames on/off, cursor 32 frames on/off 
	start line, end line */
void crtc6845_get_cursor(struct _CRTC6845 *crtc, CRTC6845_CURSOR *cursor);

data8_t crtc6845_port_r(struct _CRTC6845 *crtc, int offset);
void crtc6845_port_w(struct _CRTC6845 *crtc, int offset, data8_t data);

// functions for 
// querying more videodata 
// set lightpen position
// ...
// later
	


/* to be called when writting to port */
extern WRITE_HANDLER ( crtc6845_0_port_w );

/* to be called when reading from port */
extern READ_HANDLER ( crtc6845_0_port_r );
	
// for displaying of debug info
void crtc6845_state(void);


/* use these only in emulations of 6845 variants */

#define CRTC6845_COLUMNS (REG(0)+1)
#define CRTC6845_CHAR_COLUMNS (REG(1))
//#define COLUMNS_SYNC_POS (REG(2))
//#define COLUMNS_SYNC_SIZE ((REG(3)&0xf)-1)
//#define LINES_SYNC_SIZE (vdc.reg[3]>>4)
#define CRTC6845_LINES (REG(4)*CRTC6845_CHAR_HEIGHT+REG(5))
#define CRTC6845_CHAR_LINES REG(6)
//#define LINES_SYNC_POS (vdc.reg[7])
#define CRTC6845_CHAR_HEIGHT ((REG(9)&0x1f)+1)

#define CRTC6845_VIDEO_START ((REG(0xc)<<8)|REG(0xd))

#define CRTC6845_INTERLACE_MODE (REG(8)&3)
#define CRTC6845_INTERLACE_SIGNAL 1
#define CRTC6845_INTERLACE 3

#define CRTC6845_CURSOR_MODE (REG(0xa)&0x60)
#define CRTC6845_CURSOR_OFF 0x20
#define CRTC6845_CURSOR_16FRAMES 0x40
#define CRTC6845_CURSOR_32FRAMES 0x60

#define CRTC6845_CURSOR_POS ((REG(0xe)<<8)|REG(0xf))

#define CRTC6845_CURSOR_TOP	(REG(0xa)&0x1f)
#define CRTC6845_CURSOR_BOTTOM REG(0xb)


#ifdef __cplusplus
}
#endif

#endif
