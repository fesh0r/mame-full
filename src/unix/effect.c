/* Video Effect Functions
 *
 * Ben Saylor - bsaylor@macalester.edu
 *
 * Each of these functions copies one line of source pixels, applying an effect.
 * They are called through pointers from blit_core.h.
 * There's one for every bitmap depth / display depth combination,
 * and for direct/non-pallettized where needed.
 *
 *  FIXME: 24-bit, video drivers
 *
 * HISTORY:
 *
 *  2001-10-06:
 *   - minor changes to make -effect option trump -arbheight option <adam@gimp.org>
 *
 *  2001-10-01:
 *   - should now compile with all video drivers,
 *     though effects won't work except with
 *     aforementioned drivers
 *
 *  2001-09-21:
 *   - scan3 effect (deluxe scanlines)
 *   - small fixes & changes
 *
 *  2001-09-19:
 *   - ported to xmame-0.55.1
 *   - fixed some DGA bugs
 *   - confirmed to work with SDL
 *   - added ggi and svgalib support (untested)
 *   - added two basic RGB effects (rgbstripe3x2 and rgbscan2x3)
 *   - removed 8-bit support
 *
 *  2001-09-13:
 *   - added scan2 effect (light scanlines)
 *   - automatically scale as required by effect
 *   - works with x11_window and dga2, and hopefully dga1 and SDL (untested)
 *     (windowed mode is fastest)
 *   - use doublebuffering where requested by video driver
 *   - works on 16-bit and 32-bit displays, others untested
 *     (16-bit recommended - it's faster)
 *   - scale2x smooth scaling effect
 */

#include "xmame.h"
#include "osd_cpu.h"
#include "effect.h"

/* divide R, G, and B to darken pixels */
#define SHADE16_HALF(P)   (((P)>>1) & 0x7bef)
#define SHADE16_FOURTH(P) (((P)>>2) & 0x39e7)
#define SHADE32_HALF(P)   (((P)>>1) & 0x007f7f7f)
#define SHADE32_FOURTH(P) (((P)>>2) & 0x003f3f3f)

/* straight RGB masks */
#define RMASK16(P) ((P) & 0xf800)
#define GMASK16(P) ((P) & 0x07e0)
#define BMASK16(P) ((P) & 0x001f)
#define RMASK32(P) ((P) & 0x00ff0000)
#define GMASK32(P) ((P) & 0x0000ff00)
#define BMASK32(P) ((P) & 0x000000ff)

/* inverse RGB masks */
#define RMASK16_INV(P) ((P) & 0x07ff)
#define GMASK16_INV(P) ((P) & 0xf81f)
#define BMASK16_INV(P) ((P) & 0xffe0)
#define RMASK32_INV(P) ((P) & 0x0000ffff)
#define GMASK32_INV(P) ((P) & 0x00ff00ff)
#define BMASK32_INV(P) ((P) & 0x00ffff00)

/* inverse RGB masks, darkened*/
#define RMASK16_INV_HALF(P) (((P)>>1) & 0x03ef)
#define GMASK16_INV_HALF(P) (((P)>>1) & 0x780f)
#define BMASK16_INV_HALF(P) (((P)>>1) & 0xebe0)
#define RMASK32_INV_HALF(P) (((P)>>1) & 0x00007f7f)
#define GMASK32_INV_HALF(P) (((P)>>1) & 0x007f007f)
#define BMASK32_INV_HALF(P) (((P)>>1) & 0x007f7f00)

/* RGB semi-masks */
#define RMASK16_SEMI(P) ( RMASK16(P) | RMASK16_INV_HALF(P) )
#define GMASK16_SEMI(P) ( GMASK16(P) | GMASK16_INV_HALF(P) )
#define BMASK16_SEMI(P) ( BMASK16(P) | BMASK16_INV_HALF(P) )
#define RMASK32_SEMI(P) ( RMASK32(P) | RMASK32_INV_HALF(P) )
#define GMASK32_SEMI(P) ( GMASK32(P) | GMASK32_INV_HALF(P) )
#define BMASK32_SEMI(P) ( BMASK32(P) | BMASK32_INV_HALF(P) )

/* average two pixels */
#define MEAN16(P,Q) ( RMASK16((RMASK16(P)+RMASK16(Q))/2) | GMASK16((GMASK16(P)+GMASK16(Q))/2) | BMASK16((BMASK16(P)+BMASK16(Q))/2) )
#define MEAN32(P,Q) ( RMASK32((RMASK32(P)+RMASK32(Q))/2) | GMASK32((GMASK32(P)+GMASK32(Q))/2) | BMASK32((BMASK32(P)+BMASK32(Q))/2) )

/* called from config.c to set scale parameters */
void effect_init1()
{
        int disable_arbscale = 0;

	switch (effect) {
		case EFFECT_SCALE2X:
		case EFFECT_SCAN2:
			normal_widthscale = 2;
			normal_heightscale = 2;
                        disable_arbscale = 1;
			break;
		case EFFECT_RGBSTRIPE:
			normal_widthscale = 3;
			normal_heightscale = 2;
                        disable_arbscale = 1;
			break;
		case EFFECT_RGBSCAN:
			normal_widthscale = 2;
			normal_heightscale = 3;
                        disable_arbscale = 1;
			break;
		case EFFECT_SCAN3:
			normal_widthscale = 3;
			normal_heightscale = 3;
                        disable_arbscale = 1;
			break;
	}

	if (yarbsize && disable_arbscale) {
	  printf("Using effects -- disabling arbitrary scaling\n");
	  yarbsize = 0;
	}
}

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
void effect_init2(int src_depth, int dst_depth, int dst_width)
{
	if (effect) {
		int i;

		printf("Initializing video effect %d: bitmap depth = %d, display depth = %d\n", effect, src_depth, dst_depth);
		effect_dbbuf = malloc(dst_width*normal_heightscale*dst_depth/8);
		for (i=0; i<dst_width*normal_heightscale*dst_depth/8; i++)
			((char *)effect_dbbuf)[i] = 0;
		switch (dst_depth) {
			case 15:
			case 16:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_16;
						effect_scale2x_direct_func	= effect_scale2x_16_16_direct;
						effect_scan2_func		= effect_scan2_16_16;
						effect_scan2_direct_func	= effect_scan2_16_16_direct;
						effect_rgbstripe_func		= effect_rgbstripe_16_16;
						effect_rgbstripe_direct_func	= effect_rgbstripe_16_16_direct;
						effect_rgbscan_func		= effect_rgbscan_16_16;
						effect_rgbscan_direct_func	= effect_rgbscan_16_16_direct;
						effect_scan3_func		= effect_scan3_16_16;
						effect_scan3_direct_func	= effect_scan3_16_16_direct;
						break;
					case 32:
						break;
				}
				break;
			case 24:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_24;
						effect_scan2_func		= effect_scan2_16_24;
						effect_rgbstripe_func		= effect_rgbstripe_16_24;
						effect_rgbscan_func		= effect_rgbscan_16_24;
						effect_scan3_func		= effect_scan3_16_24;
						break;
					case 32:
						break;
				}
				break;
			case 32:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_32;
						effect_scan2_func		= effect_scan2_16_32;
						effect_rgbstripe_func		= effect_rgbstripe_16_32;
						effect_rgbscan_func		= effect_rgbscan_16_32;
						effect_scan3_func		= effect_scan3_16_32;
						break;
					case 32:
						effect_scale2x_direct_func	= effect_scale2x_32_32_direct;
						effect_scan2_direct_func	= effect_scan2_32_32_direct;
						effect_rgbstripe_direct_func	= effect_rgbstripe_32_32_direct;
						effect_rgbscan_direct_func	= effect_rgbscan_32_32_direct;
						effect_scan3_direct_func	= effect_scan3_32_32_direct;
						break;
				}
				break;
		}
	}
}


/* scale2x algorithm (Andrea Mazzoleni, http://advancemame.sourceforge.net):
 *
 * A 9-pixel rectangle is taken from the source bitmap:
 *
 *  a b c
 *  d e f
 *  g h i
 *
 * The central pixel e is expanded into four new pixels,
 *
 *  e0 e1
 *  e2 e3
 *
 * where
 *
 *  e0 = (d == b && b != f && d != h) ? d : e;
 *  e1 = (b == f && b != d && f != h) ? f : e;
 *  e2 = (d == h && d != b && h != f) ? d : e;
 *  e3 = (h == f && d != h && b != f) ? f : e;
 *
 */

void effect_scale2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	while (count) {

		if (((UINT16*)src1)[-1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[1] != ((UINT16*)src0)[0])
			*((UINT16*)dst0) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT16*)dst0) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src0)[0])
			*((UINT16*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT16*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[-1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[1] != ((UINT16*)src2)[0])
			*((UINT16*)dst1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT16*)dst1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src2)[0])
			*((UINT16*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT16*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		++((UINT16*)src0);
		++((UINT16*)src1);
		++((UINT16*)src2);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		--count;
	}
}

void effect_scale2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count)
{
	while (count) {

		if (((UINT16*)src1)[-1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[1] != ((UINT16*)src0)[0])
			*((UINT16*)dst0) = ((UINT16*)src0)[0];
		else	*((UINT16*)dst0) = ((UINT16*)src1)[0];

		if (((UINT16*)src1)[1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src0)[0])
			*((UINT16*)dst0+1) = ((UINT16*)src0)[0];
		else	*((UINT16*)dst0+1) = ((UINT16*)src1)[0];

		if (((UINT16*)src1)[-1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[1] != ((UINT16*)src2)[0])
			*((UINT16*)dst1) = ((UINT16*)src2)[0];
		else	*((UINT16*)dst1) = ((UINT16*)src1)[0];

		if (((UINT16*)src1)[1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src2)[0])
			*((UINT16*)dst1+1) = ((UINT16*)src2)[0];
		else	*((UINT16*)dst1+1) = ((UINT16*)src1)[0];

		++((UINT16*)src0);
		++((UINT16*)src1);
		++((UINT16*)src2);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		--count;
	}
}

/* FIXME: this probably doesn't work right for 24 bit */
void effect_scale2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	while (count) {

		if (((UINT16*)src1)[-1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[1] != ((UINT16*)src0)[0])
			*((UINT32*)dst0) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT32*)dst0) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src0)[0])
			*((UINT32*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT32*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[-1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[1] != ((UINT16*)src2)[0])
			*((UINT32*)dst1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT32*)dst1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src2)[0])
			*((UINT32*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT32*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		++((UINT16*)src0);
		++((UINT16*)src1);
		++((UINT16*)src2);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}

void effect_scale2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	while (count) {

		if (((UINT16*)src1)[-1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[1] != ((UINT16*)src0)[0])
			*((UINT32*)dst0) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT32*)dst0) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src0)[0] && ((UINT16*)src2)[0] != ((UINT16*)src0)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src0)[0])
			*((UINT32*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src0)[0]];
		else	*((UINT32*)dst0+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[-1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[1] != ((UINT16*)src2)[0])
			*((UINT32*)dst1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT32*)dst1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		if (((UINT16*)src1)[1] == ((UINT16*)src2)[0] && ((UINT16*)src0)[0] != ((UINT16*)src2)[0] && ((UINT16*)src1)[-1] != ((UINT16*)src2)[0])
			*((UINT32*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src2)[0]];
		else	*((UINT32*)dst1+1) = ((UINT32 *)lookup)[((UINT16*)src1)[0]];

		++((UINT16*)src0);
		++((UINT16*)src1);
		++((UINT16*)src2);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}

void effect_scale2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count)
{
	while (count) {

		if (((UINT32*)src1)[-1] == ((UINT32*)src0)[0] && ((UINT32*)src2)[0] != ((UINT32*)src0)[0] && ((UINT32*)src1)[1] != ((UINT32*)src0)[0])
			*((UINT32*)dst0) = ((UINT32*)src0)[0];
		else	*((UINT32*)dst0) = ((UINT32*)src1)[0];

		if (((UINT32*)src1)[1] == ((UINT32*)src0)[0] && ((UINT32*)src2)[0] != ((UINT32*)src0)[0] && ((UINT32*)src1)[-1] != ((UINT32*)src0)[0])
			*((UINT32*)dst0+1) = ((UINT32*)src0)[0];
		else	*((UINT32*)dst0+1) = ((UINT32*)src1)[0];

		if (((UINT32*)src1)[-1] == ((UINT32*)src2)[0] && ((UINT32*)src0)[0] != ((UINT32*)src2)[0] && ((UINT32*)src1)[1] != ((UINT32*)src2)[0])
			*((UINT32*)dst1) = ((UINT32*)src2)[0];
		else	*((UINT32*)dst1) = ((UINT32*)src1)[0];

		if (((UINT32*)src1)[1] == ((UINT32*)src2)[0] && ((UINT32*)src0)[0] != ((UINT32*)src2)[0] && ((UINT32*)src1)[-1] != ((UINT32*)src2)[0])
			*((UINT32*)dst1+1) = ((UINT32*)src2)[0];
		else	*((UINT32*)dst1+1) = ((UINT32*)src1)[0];

		++((UINT32*)src0);
		++((UINT32*)src1);
		++((UINT32*)src2);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}


/**********************************
 * scan2: light 2x2 scanlines
 **********************************/

void effect_scan2_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT16*)dst0) = *((UINT16*)dst0+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
			 
		*((UINT16*)dst1) = *((UINT16*)dst1+1) = SHADE16_HALF( ((UINT32 *)lookup)[*((UINT16*)src)] ) + SHADE16_FOURTH( ((UINT32 *)lookup)[*((UINT16*)src)] );

		++((UINT16*)src);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		--count;
	}
}

void effect_scan2_16_16_direct (void *dst0, void *dst1, const void *src, unsigned count)
{
	while (count) {

		*((UINT16*)dst0) = *((UINT16*)dst0+1) = *((UINT16*)src);
			 
		*((UINT16*)dst1) = *((UINT16*)dst1+1) = SHADE16_HALF( *((UINT16*)src) ) + SHADE16_FOURTH( *((UINT16*)src) );

		++((UINT16*)src);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		--count;
	}
}

void effect_scan2_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0) = *((UINT32*)dst0+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
			 
		*((UINT32*)dst1) = *((UINT32*)dst1+1) = SHADE32_HALF( ((UINT32 *)lookup)[*((UINT16*)src)] ) + SHADE32_FOURTH( ((UINT32 *)lookup)[*((UINT16*)src)] );

		++((UINT16*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}

void effect_scan2_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0) = *((UINT32*)dst0+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
			 
		*((UINT32*)dst1) = *((UINT32*)dst1+1) = SHADE32_HALF( ((UINT32 *)lookup)[*((UINT16*)src)] ) + SHADE32_FOURTH( ((UINT32 *)lookup)[*((UINT16*)src)] );

		++((UINT16*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}

void effect_scan2_32_32_direct (void *dst0, void *dst1, const void *src, unsigned count)
{
	while (count) {

		*((UINT32*)dst0) = *((UINT32*)dst0+1) = *((UINT32*)src);
			 
		*((UINT32*)dst1) = *((UINT32*)dst1+1) = SHADE32_HALF( *((UINT32*)src) ) +  SHADE32_FOURTH( *((UINT32*)src) );

		++((UINT32*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		--count;
	}
}


/**********************************
 * rgbstripe
 **********************************/

void effect_rgbstripe_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT16*)dst0+0) = *((UINT16*)dst1+0) = RMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst0+1) = *((UINT16*)dst1+1) = GMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst0+2) = *((UINT16*)dst1+2) = BMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT16*)dst0 += 3;
		(UINT16*)dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_16_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	while (count) {

		*((UINT16*)dst0+0) = *((UINT16*)dst1+0) = RMASK16_SEMI(*((UINT16*)src));
		*((UINT16*)dst0+1) = *((UINT16*)dst1+1) = GMASK16_SEMI(*((UINT16*)src));
		*((UINT16*)dst0+2) = *((UINT16*)dst1+2) = BMASK16_SEMI(*((UINT16*)src));

		++((UINT16*)src);
		(UINT16*)dst0 += 3;
		(UINT16*)dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst1+0) = RMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+1) = *((UINT32*)dst1+1) = GMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+2) = *((UINT32*)dst1+2) = BMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst1+0) = RMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+1) = *((UINT32*)dst1+1) = GMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+2) = *((UINT32*)dst1+2) = BMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_32_32_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst1+0) = RMASK32_SEMI(*((UINT32*)src));
		*((UINT32*)dst0+1) = *((UINT32*)dst1+1) = GMASK32_SEMI(*((UINT32*)src));
		*((UINT32*)dst0+2) = *((UINT32*)dst1+2) = BMASK32_SEMI(*((UINT32*)src));

		++((UINT32*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		--count;
	}
}


/**********************************
 * rgbscan
 **********************************/

void effect_rgbscan_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT16*)dst0+0) = *((UINT16*)dst0+1) = RMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst1+0) = *((UINT16*)dst1+1) = GMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst2+0) = *((UINT16*)dst2+1) = BMASK16_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		(UINT16*)dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	while (count) {

		*((UINT16*)dst0+0) = *((UINT16*)dst0+1) = RMASK16_SEMI(*((UINT16*)src));
		*((UINT16*)dst1+0) = *((UINT16*)dst1+1) = GMASK16_SEMI(*((UINT16*)src));
		*((UINT16*)dst2+0) = *((UINT16*)dst2+1) = BMASK16_SEMI(*((UINT16*)src));

		++((UINT16*)src);
		(UINT16*)dst0 += 2;
		(UINT16*)dst1 += 2;
		(UINT16*)dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst0+1) = RMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = GMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) = BMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		(UINT32*)dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst0+1) = RMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = GMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) = BMASK32_SEMI(((UINT32 *)lookup)[*((UINT16*)src)]);

		++((UINT16*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		(UINT32*)dst2 += 2;
		--count;
	}
}

void effect_rgbscan_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	while (count) {

		*((UINT32*)dst0+0) = *((UINT32*)dst0+1) = RMASK32_SEMI(*((UINT32*)src));
		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = GMASK32_SEMI(*((UINT32*)src));
		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) = BMASK32_SEMI(*((UINT32*)src));

		++((UINT32*)src);
		(UINT32*)dst0 += 2;
		(UINT32*)dst1 += 2;
		(UINT32*)dst2 += 2;
		--count;
	}
}


/**********************************
 * scan3
 **********************************/

/* All 3 lines are horizontally blurred a little
 * (the last pixel of each three in a line is averaged with the next pixel).
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%.
 */

void effect_scan3_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT16*)dst0+1) = *((UINT16*)dst0+0) =
			SHADE16_HALF(((UINT32 *)lookup)[*((UINT16*)src)]) + SHADE16_FOURTH(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst0+2) =
			SHADE16_HALF( MEAN16( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) )
			+
			SHADE16_FOURTH( MEAN16( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		*((UINT16*)dst1+0) = *((UINT16*)dst1+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
		*((UINT16*)dst1+2) = MEAN16( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] );

		*((UINT16*)dst2+0) = *((UINT16*)dst2+1) =
			SHADE16_HALF(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT16*)dst2+2) =
			SHADE16_HALF( MEAN16( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		++((UINT16*)src);
		(UINT16*)dst0 += 3;
		(UINT16*)dst1 += 3;
		(UINT16*)dst2 += 3;
		--count;
	}
}

void effect_scan3_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	while (count) {

		*((UINT16*)dst0+1) = *((UINT16*)dst0+0) =
			SHADE16_HALF(*((UINT16*)src)) + SHADE16_FOURTH(*((UINT16*)src));
		*((UINT16*)dst0+2) =
			SHADE16_HALF( MEAN16( *((UINT16*)src), *((UINT16*)src+1) ) )
			+
			SHADE16_FOURTH( MEAN16( *((UINT16*)src), *((UINT16*)src+1) ) );

		*((UINT16*)dst1+0) = *((UINT16*)dst1+1) = *((UINT16*)src);
		*((UINT16*)dst1+2) = MEAN16( *((UINT16*)src), *((UINT16*)src+1) );

		*((UINT16*)dst2+0) = *((UINT16*)dst2+1) =
			SHADE16_HALF(*((UINT16*)src));
		*((UINT16*)dst2+2) =
			SHADE16_HALF( MEAN16( *((UINT16*)src), *((UINT16*)src+1) ) );

		++((UINT16*)src);
		(UINT16*)dst0 += 3;
		(UINT16*)dst1 += 3;
		(UINT16*)dst2 += 3;
		--count;
	}
}

void effect_scan3_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+1) = *((UINT32*)dst0+0) =
			SHADE32_HALF(((UINT32 *)lookup)[*((UINT16*)src)]) + SHADE32_FOURTH(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+2) =
			SHADE32_HALF( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) )
			+
			SHADE32_FOURTH( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
		*((UINT32*)dst1+2) = MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] );

		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) =
			SHADE32_HALF(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst2+2) =
			SHADE32_HALF( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		++((UINT16*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		(UINT32*)dst2 += 3;
		--count;
	}
}

void effect_scan3_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	while (count) {

		*((UINT32*)dst0+1) = *((UINT32*)dst0+0) =
			SHADE32_HALF(((UINT32 *)lookup)[*((UINT16*)src)]) + SHADE32_FOURTH(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst0+2) =
			SHADE32_HALF( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) )
			+
			SHADE32_FOURTH( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = ((UINT32 *)lookup)[*((UINT16*)src)];
		*((UINT32*)dst1+2) = MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] );

		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) =
			SHADE32_HALF(((UINT32 *)lookup)[*((UINT16*)src)]);
		*((UINT32*)dst2+2) =
			SHADE32_HALF( MEAN32( ((UINT32 *)lookup)[*((UINT16*)src)], ((UINT32 *)lookup)[*((UINT16*)src+1)] ) );

		++((UINT16*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		(UINT32*)dst2 += 3;
		--count;
	}
}

void effect_scan3_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	while (count) {

		*((UINT32*)dst0+1) = *((UINT32*)dst0+0) =
			SHADE32_HALF(*((UINT32*)src)) + SHADE32_FOURTH(*((UINT32*)src));
		*((UINT32*)dst0+2) =
			SHADE32_HALF( MEAN32( *((UINT32*)src), *((UINT32*)src+1) ) )
			+
			SHADE32_FOURTH( MEAN32( *((UINT32*)src), *((UINT32*)src+1) ) );

		*((UINT32*)dst1+0) = *((UINT32*)dst1+1) = *((UINT32*)src);
		*((UINT32*)dst1+2) = MEAN32( *((UINT32*)src), *((UINT32*)src+1) );

		*((UINT32*)dst2+0) = *((UINT32*)dst2+1) =
			SHADE32_HALF(*((UINT32*)src));
		*((UINT32*)dst2+2) =
			SHADE32_HALF( MEAN32( *((UINT32*)src), *((UINT32*)src+1) ) );

		++((UINT32*)src);
		(UINT32*)dst0 += 3;
		(UINT32*)dst1 += 3;
		(UINT32*)dst2 += 3;
		--count;
	}
}
