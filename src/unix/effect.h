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

/* from video.c, needed to scale the display according to the requirements of the effect */
extern int normal_widthscale, normal_heightscale, normal_yarbsize;

/* called from config.c to set scale parameters */
void effect_init1();

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
void effect_init2(int src_depth, int dst_depth, int dst_width);

/*** effect function pointers (use these) ***/
EXTERN_EFFECT void (*effect_scale2x_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_scale2x_direct_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

EXTERN_EFFECT void (*effect_hq2x_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_hq2x_direct_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

EXTERN_EFFECT void (*effect_lq2x_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_lq2x_direct_func)
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

EXTERN_EFFECT void (*effect_scan2_func)(void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_scan2_direct_func)(void *dst0, void *dst1, const void *src, unsigned count);

EXTERN_EFFECT void (*effect_rgbstripe_func)(void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_rgbstripe_direct_func)(void *dst0, void *dst1, const void *src, unsigned count);

EXTERN_EFFECT void (*effect_rgbscan_func)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_rgbscan_direct_func)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

EXTERN_EFFECT void (*effect_scan3_func)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_scan3_direct_func)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

EXTERN_EFFECT void (*effect_6tap_clear_func)(unsigned count);
EXTERN_EFFECT void (*effect_6tap_addline_func)(const void *src0, unsigned count, const void *lookup);
EXTERN_EFFECT void (*effect_6tap_addline_direct_func)(const void *src0, unsigned count);
EXTERN_EFFECT void (*effect_6tap_render_func)(void *dst0, void *dst1, unsigned count);

/********************************************/

void effect_scale2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_scale2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_scale2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_scale2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_scale2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_scale2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_scale2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_hq2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_hq2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_hq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_hq2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_hq2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_hq2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_lq2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_lq2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_lq2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

void effect_lq2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_lq2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup);

void effect_lq2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count);

/*****************************/

void effect_scan2_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_scan2_16_16_direct(void *dst0, void *dst1, const void *src, unsigned count);
void effect_scan2_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_scan2_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_scan2_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_scan2_32_32_direct(void *dst0, void *dst1, const void *src, unsigned count);
void effect_scan2_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count);

/*****************************/

void effect_rgbstripe_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_rgbstripe_16_16_direct(void *dst0, void *dst1, const void *src, unsigned count);
void effect_rgbstripe_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_rgbstripe_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_rgbstripe_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup);
void effect_rgbstripe_32_32_direct(void *dst0, void *dst1, const void *src, unsigned count);
void effect_rgbstripe_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count);

/*****************************/

void effect_rgbscan_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_rgbscan_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_rgbscan_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_rgbscan_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_rgbscan_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_rgbscan_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

/*****************************/

void effect_scan3_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_scan3_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_scan3_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_scan3_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup);
void effect_scan3_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);
void effect_scan3_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count);

/*****************************/

#ifdef EFFECT_MMX_ASM
/* These are in effect_asm.asm */
extern void effect_6tap_clear_asm(unsigned count);
extern void effect_6tap_addline_16_32(const void *src0, unsigned count, const void *lookup);
extern void effect_6tap_addline_32_32_direct(const void *src0, unsigned count);
extern void effect_6tap_render_32(void *dst0, void *dst1, unsigned count);
#else
void effect_6tap_addline_16_32(const void *src0, unsigned count, const void *lookup);
void effect_6tap_addline_32_32_direct(const void *src0, unsigned count);
void effect_6tap_render_32(void *dst0, void *dst1, unsigned count);
#endif

/* these are only implemented in C */
void effect_6tap_clear(unsigned count);
void effect_6tap_addline_16_16(const void *src0, unsigned count, const void *lookup);
void effect_6tap_addline_16_16_direct(const void *src0, unsigned count);
void effect_6tap_render_16(void *dst0, void *dst1, unsigned count);

/*****************************/

EXTERN_EFFECT void (*rotate_func)(void *dst, struct mame_bitmap *bitamp, int y);
void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y);
void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y);

#endif /* __EFFECT_H */
