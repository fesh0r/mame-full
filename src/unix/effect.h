#ifndef __EFFECT_H
#define __EFFECT_H

#include "sysdep/sysdep_display.h"

#ifdef __EFFECT_C_
#define EXTERN_EFFECT
#else
#define EXTERN_EFFECT extern
#endif

/* buffer for doublebuffering */
extern char *effect_dbbuf;
extern char *rotate_dbbuf0;
extern char *rotate_dbbuf1;
extern char *rotate_dbbuf2;
extern char *_6tap2x_buf0;
extern char *_6tap2x_buf1;
extern char *_6tap2x_buf2;
extern char *_6tap2x_buf3;
extern char *_6tap2x_buf4;
extern char *_6tap2x_buf5;

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
int  effect_open(void);
void effect_close(void);

/*** effect function pointers (use these) ***/
typedef void (*effect_func_p)(void *dst0, void *dst1,
                const void *src, unsigned count, unsigned int *u32lookup);
typedef void (*effect_scale2x_func_p) (void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);
typedef void (*effect_scale3x_func_p)(void *dst0, void *dst1, void *dst2,
                const void *src, unsigned count, unsigned int *u32lookup);
typedef void (*effect_6tap_clear_func_p)(unsigned count);
typedef void (*effect_6tap_addline_func_p)(const void *src0, unsigned count, unsigned int *u32lookup);
typedef void (*effect_6tap_render_func_p)(void *dst0, void *dst1, unsigned count);

EXTERN_EFFECT effect_func_p effect_func;
EXTERN_EFFECT effect_scale2x_func_p effect_scale2x_func;
EXTERN_EFFECT effect_scale3x_func_p effect_scale3x_func;
EXTERN_EFFECT effect_6tap_clear_func_p effect_6tap_clear_func;
EXTERN_EFFECT effect_6tap_addline_func_p effect_6tap_addline_func;
EXTERN_EFFECT effect_6tap_render_func_p effect_6tap_render_func;

/********************************************/

void effect_scale2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

void effect_scale2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_hq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

void effect_hq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_lq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

void effect_lq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_scan2_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan2_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
#ifdef EFFECT_MMX_ASM
void effect_scan2_15_15_direct(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan2_16_15(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan2_16_16(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan2_16_32(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan2_32_32_direct(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
#endif

/*****************************/

void effect_rgbstripe_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);
void effect_rgbstripe_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_rgbscan_16_YUY2(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup);
void effect_rgbscan_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_scan3_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup);
void effect_scan3_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup);

/*****************************/

void effect_6tap_clear(unsigned count);
void effect_6tap_addline_15(const void *src0, unsigned count, unsigned int *u32lookup);
void effect_6tap_addline_16(const void *src0, unsigned count, unsigned int *u32lookup);
void effect_6tap_addline_32(const void *src0, unsigned count, unsigned int *u32lookup);
void effect_6tap_render_15(void *dst0, void *dst1, unsigned count);
void effect_6tap_render_16(void *dst0, void *dst1, unsigned count);
void effect_6tap_render_32(void *dst0, void *dst1, unsigned count);

/*****************************/

EXTERN_EFFECT void (*rotate_func)(void *dst, struct mame_bitmap *bitamp, int y, struct rectangle *bounds);
void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds);
void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds);

/****** usefull macros *******/

/* straight RGB masks */
#define RMASK15(P) ((P) & 0x7c00)
#define GMASK15(P) ((P) & 0x03e0)
#define BMASK15(P) ((P) & 0x001f)
#define RMASK16(P) ((P) & 0xf800)
#define GMASK16(P) ((P) & 0x07e0)
#define BMASK16(P) ((P) & 0x001f)
#define RMASK32(P) ((P) & 0x00ff0000)
#define GMASK32(P) ((P) & 0x0000ff00)
#define BMASK32(P) ((P) & 0x000000ff)

/* inverse RGB masks, darkened*/
#define RMASK15_INV_HALF(P) (((P)>>1) & 0x01ef)
#define GMASK15_INV_HALF(P) (((P)>>1) & 0x3c0f)
#define BMASK15_INV_HALF(P) (((P)>>1) & 0x3de0)
#define RMASK16_INV_HALF(P) (((P)>>1) & 0x03ef)
#define GMASK16_INV_HALF(P) (((P)>>1) & 0x780f)
#define BMASK16_INV_HALF(P) (((P)>>1) & 0xebe0)
#define RMASK32_INV_HALF(P) (((P)>>1) & 0x00007f7f)
#define GMASK32_INV_HALF(P) (((P)>>1) & 0x007f007f)
#define BMASK32_INV_HALF(P) (((P)>>1) & 0x007f7f00)

/* RGB semi-masks */
#define RMASK15_SEMI(P) ( RMASK15(P) | RMASK15_INV_HALF(P) )
#define GMASK15_SEMI(P) ( GMASK15(P) | GMASK15_INV_HALF(P) )
#define BMASK15_SEMI(P) ( BMASK15(P) | BMASK15_INV_HALF(P) )
#define RMASK16_SEMI(P) ( RMASK16(P) | RMASK16_INV_HALF(P) )
#define GMASK16_SEMI(P) ( GMASK16(P) | GMASK16_INV_HALF(P) )
#define BMASK16_SEMI(P) ( BMASK16(P) | BMASK16_INV_HALF(P) )
#define RMASK32_SEMI(P) ( RMASK32(P) | RMASK32_INV_HALF(P) )
#define GMASK32_SEMI(P) ( GMASK32(P) | GMASK32_INV_HALF(P) )
#define BMASK32_SEMI(P) ( BMASK32(P) | BMASK32_INV_HALF(P) )

/* divide R, G, and B to darken pixels */
#define SHADE15_HALF(P)   (((P)>>1) & 0x3def)
#define SHADE15_FOURTH(P) (((P)>>2) & 0x1ce7)
#define SHADE16_HALF(P)   (((P)>>1) & 0x7bef)
#define SHADE16_FOURTH(P) (((P)>>2) & 0x39e7)
#define SHADE32_HALF(P)   (((P)>>1) & 0x007f7f7f)
#define SHADE32_FOURTH(P) (((P)>>2) & 0x003f3f3f)

/* average two pixels */
#define MEAN15(P,Q) ( RMASK15((RMASK15(P)+RMASK15(Q))/2) | GMASK15((GMASK15(P)+GMASK15(Q))/2) | BMASK15((BMASK15(P)+BMASK15(Q))/2) )
#define MEAN16(P,Q) ( RMASK16((RMASK16(P)+RMASK16(Q))/2) | GMASK16((GMASK16(P)+GMASK16(Q))/2) | BMASK16((BMASK16(P)+BMASK16(Q))/2) )
#define MEAN32(P,Q) ( RMASK32((RMASK32(P)+RMASK32(Q))/2) | GMASK32((GMASK32(P)+GMASK32(Q))/2) | BMASK32((BMASK32(P)+BMASK32(Q))/2) )

#endif /* __EFFECT_H */
