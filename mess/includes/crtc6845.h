#ifndef CRTC6845_H
#define CRTC6845_H

/***************************************************************************

  motorola cathode ray tube controller 6845

  simple version, for a cycle exact framework look at m6845.h

  copyright peter.trauner@jk.uni-linz.ac.at

***************************************************************************/

#include "mame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* opaque structure representing a crtc8645 chip */
struct crtc6845;

/* generic crtc6845 instance */
extern struct crtc6845 *crtc6845;

struct crtc6845_cursor
{
	int on;
	int pos;
	int top;
	int bottom;
};

struct crtc6845_config
{
	int freq;
	void (*cursor_changed)(struct crtc6845_cursor *old);
};

struct crtc6845 *crtc6845_init(struct crtc6845_config *config);

void crtc6845_set_clock(struct crtc6845 *crtc, int freq);

// to be called before drawing screen
void crtc6845_time(struct crtc6845 *crtc);

int crtc6845_get_char_columns(struct crtc6845 *crtc);
int crtc6845_get_char_height(struct crtc6845 *crtc);
int crtc6845_get_char_lines(struct crtc6845 *crtc);
int crtc6845_get_start(struct crtc6845 *crtc);

/* cursor off, cursor on, cursor 16 frames on/off, cursor 32 frames on/off 
	start line, end line */
void crtc6845_get_cursor(struct crtc6845 *crtc, struct crtc6845_cursor *cursor);

data8_t crtc6845_port_r(struct crtc6845 *crtc, int offset);
void crtc6845_port_w(struct crtc6845 *crtc, int offset, data8_t data);

// functions for 
// querying more videodata 
// set lightpen position
// ...
// later
	


/* to be called when writting to port */
WRITE_HANDLER ( crtc6845_0_port_w );

/* to be called when reading from port */
READ_HANDLER ( crtc6845_0_port_r );
	
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

#define CRTC6845_SKEW	(REG(8)&15)

#define CRTC6845_CURSOR_POS ((REG(0xe)<<8)|REG(0xf))

#define CRTC6845_CURSOR_TOP	(REG(0xa)&0x1f)
#define CRTC6845_CURSOR_BOTTOM REG(0xb)


#ifdef __cplusplus
}
#endif

#endif /* CRTC6845_H */
