#ifndef __LYNX_H__
#define __LYNX_H__
#include "driver.h"

int lynx_vh_start(void);
void lynx_vh_stop(void);
void lynx_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
extern UINT32 lynx_partialcrc(const unsigned char *,unsigned int);

extern int debug_pos;
extern char debug_strings[16][30];


#define PAD_UP 0x80
#define PAD_DOWN 0x40
#define PAD_LEFT 0x20
#define PAD_RIGHT 0x10

extern void lynx_machine_init(void);

extern UINT16 lynx_granularity;
extern int lynx_rotate;

typedef struct {
	UINT8 data[0x100];
} MIKEY;
extern MIKEY mikey;
WRITE_HANDLER( lynx_memory_config );
WRITE_HANDLER(mikey_write);
READ_HANDLER(mikey_read);
WRITE_HANDLER(suzy_write);
READ_HANDLER(suzy_read);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
extern "C" void lynx_runtime_loader_init(void);
# else
extern void lynx_runtime_loader_init(void);
# endif
#endif

void lynx_audio_debug(struct osd_bitmap *bitmap);
void lynx_audio_write(int offset, UINT8 data);
UINT8 lynx_audio_read(int offset);
extern int lynx_custom_start (const struct MachineSound *driver);
extern int lynx2_custom_start (const struct MachineSound *driver);
extern void lynx_custom_stop (void);
extern void lynx_custom_update (void);
#endif

#ifdef INCLUDE_LYNX_LINE_FUNCTION
	int j, xi, wi, i;
	int b, p, color;

	i=blitter.mem[blitter.bitmap];
	blitter.memory_accesses++;
	for (xi=blitter.x, p=0, b=0, j=1, wi=0; (j<i);) {
	    if (p<bits) {
		b=(b<<8)|blitter.mem[blitter.bitmap+j];
		j++;
		p+=8;
		blitter.memory_accesses++;
	    }
	    for (;(p>=bits);) {
		color=blitter.color[(b>>(p-bits))&mask]; p-=bits;
		for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
		    if ((xi>=0)&&(xi<160)) {
			lynx_plot_pixel(blitter.mode, xi, y, color);
		    }
		}
		wi-=blitter.width;
	    }
	}
#endif

#ifdef INCLUDE_LYNX_LINE_RLE_FUNCTION

	int wi, xi;
	int b, p, j;
	int t, count, color;

	for( p=0, j=0, b=0, xi=blitter.x, wi=0; ; ) { // through the rle entries
	    if (p<5+bits) { // under 7 bits no complete entry
		j++;
		if (j>=blitter.mem[blitter.bitmap]) return;
		p+=8;
		b=(b<<8)|blitter.mem[blitter.bitmap+j];
		blitter.memory_accesses++;
	    }
	    t=(b>>(p-1))&1;p--;
	    count=((b>>(p-4))&0xf)+1;p-=4;
	    if (t) { // count of different pixels
		for (;count; count--) {
		    if (p<bits) {
			j++;
			if (j>=blitter.mem[blitter.bitmap]) return;
			p+=8;
			b=(b<<8)|blitter.mem[blitter.bitmap+j];
			blitter.memory_accesses++;
		    }
		    color=blitter.color[(b>>(p-bits))&mask];p-=bits;
		    for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
			if ((xi>=0)&&(xi<160)) {
			    lynx_plot_pixel(blitter.mode, xi, y, color);
			}
		    }
		    wi-=blitter.width;
		}
	    } else { // count of same pixels
		if (count==0) return;
		if (p<bits) {
		    j++;
		    if (j>=blitter.mem[blitter.bitmap]) return;
		    p+=8;
		    b=(b<<8)|blitter.mem[blitter.bitmap+j];
		    blitter.memory_accesses++;
		}
		color=blitter.color[(b>>(p-bits))&mask];p-=bits;
		for (;count; count--) {
		    for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
			if ((xi>=0)&&(xi<160)) {
			    lynx_plot_pixel(blitter.mode, xi, y, color);
			}
		    }
		    wi-=blitter.width;
		}
	    }
	}

#endif
