#ifndef DRAGON_H
#define DRAGON_H

/* ----------------------------------------------------------------------- *
 * Backdoors into mess/vidhrdw/m6847.c                                     *
 * ----------------------------------------------------------------------- */

typedef void (*artifactproc)(int *artifactcolors);
void internal_m6847_drawborder(struct osd_bitmap *bitmap, int screenx, int screeny, int pen);
int internal_m6847_vh_start(int maxvram);
void internal_m6847_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh,
	const int *metapalette, UINT8 *vrambase, int vrampos, int vramsize,
	int has_lowercase, int basex, int basey, int wf, artifactproc artifact);
void blitgraphics2(struct osd_bitmap *bitmap, UINT8 *vrambase, int vrampos,
	int vramsize, UINT8 *db, const int *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes);
void blitgraphics4(struct osd_bitmap *bitmap, UINT8 *vrambase, int vrampos,
	int vramsize, UINT8 *db, const int *metapalette, int sizex, int sizey,
	int basex, int basey, int scalex, int scaley, int additionalrowbytes);
void blitgraphics16(struct osd_bitmap *bitmap, UINT8 *vrambase,
	int vrampos, int vramsize, UINT8 *db, int sizex, int sizey, int basex,
	int basey, int scalex, int scaley, int additionalrowbytes);

/* ----------------------------------------------------------------------- *
 * from vidhrdw/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern WRITE_HANDLER ( coco3_ram_b1_w );
extern WRITE_HANDLER ( coco3_ram_b2_w );
extern WRITE_HANDLER ( coco3_ram_b3_w );
extern WRITE_HANDLER ( coco3_ram_b4_w );
extern WRITE_HANDLER ( coco3_ram_b5_w );
extern WRITE_HANDLER ( coco3_ram_b6_w );
extern WRITE_HANDLER ( coco3_ram_b7_w );
extern WRITE_HANDLER ( coco3_ram_b8_w );
extern WRITE_HANDLER ( coco3_ram_b9_w );
extern void coco3_vh_sethires(int hires);
extern int dragon_vh_start(void);
extern int coco3_vh_start(void);
extern void coco3_vh_stop(void);
extern void coco3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( dragon_sam_display_offset );
extern WRITE_HANDLER ( dragon_sam_vdg_mode );
extern int dragon_interrupt(void);
extern WRITE_HANDLER ( coco_ram_w );
extern READ_HANDLER ( coco3_gimevh_r );
extern WRITE_HANDLER ( coco3_gimevh_w );
extern WRITE_HANDLER ( coco3_palette_w );
extern void coco3_vh_blink(void);

/* ----------------------------------------------------------------------- *
 * from machine/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern void dragon32_init_machine(void);
extern void dragon64_init_machine(void);
extern void coco_init_machine(void);
extern void coco3_init_machine(void);
extern void dragon_stop_machine(void);
extern int coco_cassette_init(int id);
extern int coco3_cassette_init(int id);
extern void coco_cassette_exit(int id);
extern int dragon32_rom_load(int id);
extern int dragon64_rom_load(int id);
extern int coco3_rom_load(int id);
extern READ_HANDLER ( dragon_mapped_irq_r );
extern int coco3_mapped_irq_r(int offset);
extern WRITE_HANDLER ( dragon64_sam_himemmap );
extern WRITE_HANDLER ( coco3_sam_himemmap );
extern READ_HANDLER ( coco3_mmu_r );
extern WRITE_HANDLER ( coco3_mmu_w );
extern READ_HANDLER ( coco3_gime_r );
extern WRITE_HANDLER ( coco3_gime_w );
extern WRITE_HANDLER ( dragon_sam_speedctrl );
extern WRITE_HANDLER ( coco3_sam_speedctrl );
extern WRITE_HANDLER ( dragon_sam_page_mode );
extern WRITE_HANDLER ( dragon_sam_memory_size );
extern READ_HANDLER ( coco3_floppy_r);
extern WRITE_HANDLER ( coco3_floppy_w );
extern int coco_floppy_init(int id);
extern void coco_floppy_exit(int id);
extern READ_HANDLER ( coco_floppy_r );
extern WRITE_HANDLER ( coco_floppy_w );
extern READ_HANDLER(dragon_floppy_r);
extern WRITE_HANDLER ( dragon_floppy_w );
extern void coco3_vblank(void);
extern int coco3_mmu_translate(int block, int offset);

/* Returns whether a given piece of logical memory is contiguous or not */
extern int coco3_mmu_ismemorycontiguous(int logicaladdr, int len);

/* Reads logical memory into a buffer */
extern void coco3_mmu_readlogicalmemory(UINT8 *buffer, int logicaladdr, int len);

/* Translates a logical address to a physical address */
extern int coco3_mmu_translatelogicaladdr(int logicaladdr);

#endif /* DRAGON_H */
