#ifndef __LYNX_H__
#define __LYNX_H__
#include "driver.h"

int lynx_vh_start(void);
void lynx_vh_stop(void);
void lynx_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

extern UINT16 lynx_granularity;

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

#endif

#ifdef INCLUDE_LYNX_LINE_FUNCTION
	int j, xi, wi, i;
	int b, p, c;

	i=blitter.mem[blitter.bitmap];
	for (xi=blitter.x, p=0, b=0, j=1, wi=0; (j<i);) {
		if (p<bits) {
			b=(b<<8)|blitter.mem[blitter.bitmap+j];
			j++;
			p+=8;
		}
		while (p>=bits) {
			c=(b>>(p-bits))&mask; p-=bits;
			for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
				if ((!trans || (c!=0))
					&&(xi>=0)&&(xi<160)) {
					PLOT_PIXEL(dest, xi, blitter.color[c]);
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
				}
				color=(b>>(p-bits))&mask;p-=bits;
				for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
					if ( (!trans||(color!=0))
						  &&(xi>=0)&&(xi<160)) {
						PLOT_PIXEL(dest, xi, blitter.color[color]);
					}
				}
				wi-=blitter.width;
			}
		} else { // count of same pixels
			if (p<bits) {
				j++;
				if (j>=blitter.mem[blitter.bitmap]) return;
				p+=8;
				b=(b<<8)|blitter.mem[blitter.bitmap+j];
			}
			color=(b>>(p-bits))&mask;p-=bits;
			for (;count; count--) {
				for (;(wi<blitter.width);wi+=0x100, xi+=xdir) {
					if ( (!trans||(color!=0))
						  &&(xi>=0)&&(xi<160)) {
						PLOT_PIXEL(dest, xi, blitter.color[color]);
					}
				}
				wi-=blitter.width;
			}
		}
	}

#endif
