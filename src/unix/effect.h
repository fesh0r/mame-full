#ifndef __EFFECT_H
#define __EFFECT_H

#ifdef __EFFECT_C_
#define EXTERN_EFFECT
#else
#define EXTERN_EFFECT extern
#endif

/* effect type */
EXTERN_EFFECT int effect;
enum {EFFECT_NONE, EFFECT_SCALE2X, EFFECT_SCAN2, EFFECT_RGBSTRIPE, EFFECT_RGBSCAN, EFFECT_SCAN3, EFFECT_LQ2X, EFFECT_HQ2X, EFFECT_6TAP2X};
#define EFFECT_LAST EFFECT_6TAP2X

/* buffer for doublebuffering */
EXTERN_EFFECT char *effect_dbbuf;
EXTERN_EFFECT char *rotate_dbbuf0;
EXTERN_EFFECT char *rotate_dbbuf1;
EXTERN_EFFECT char *rotate_dbbuf2;
extern char *_6tap2x_buf0;
extern char *_6tap2x_buf1;
extern char *_6tap2x_buf2;
extern char *_6tap2x_buf3;
extern char *_6tap2x_buf4;
extern char *_6tap2x_buf5;


/* from video.c, needed to scale the display according to the requirements of the effect */
extern int normal_widthscale, normal_heightscale, normal_yarbsize;

/* called from config.c to set scale parameters */
void effect_init1();

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
void effect_init2(int src_depth, int dst_depth, int dst_width);

/*** effect function pointers (use these) ***/
typedef void (*effect_func_p)(void *dst0, void *dst1,
                const void *src, unsigned count);
typedef void (*effect_scale2x_func_p) (void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);
typedef void (*effect_scale3x_func_p)(void *dst0, void *dst1, void *dst2,
                const void *src, unsigned count);
typedef void (*effect_6tap_clear_func_p)(unsigned count);
typedef void (*effect_6tap_addline_func_p)(const void *src0, unsigned count);
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
		unsigned count);

void effect_scale2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_hq2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_lq2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_scan2_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count);
void effect_scan2_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count);

/*****************************/

void effect_rgbstripe_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count);
void effect_rgbstripe_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count);

/*****************************/

void effect_rgbscan_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

/*****************************/

void effect_scan3_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

/*****************************/

void effect_6tap_clear(unsigned count);
void effect_6tap_addline_15(const void *src0, unsigned count);
void effect_6tap_addline_16(const void *src0, unsigned count);
void effect_6tap_addline_32(const void *src0, unsigned count);
void effect_6tap_render_15(void *dst0, void *dst1, unsigned count);
void effect_6tap_render_16(void *dst0, void *dst1, unsigned count);
void effect_6tap_render_32(void *dst0, void *dst1, unsigned count);

/*****************************/

EXTERN_EFFECT void (*rotate_func)(void *dst, struct mame_bitmap *bitamp, int y);
void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y);
void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y);

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

#endif /* __EFFECT_H */
