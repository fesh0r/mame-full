#ifndef DRAGON_H
#define DRAGON_H

#include "vidhrdw/m6847.h"
#include "includes/rstrbits.h"

#define COCO_CPU_SPEED	(TIME_IN_HZ(894886))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)

#define COCO_DIP_ARTIFACTING		12
#define COCO3_DIP_MONITORTYPE		12
#define COCO3_DIP_MONITORTYPE_MASK	0x08

/* ----------------------------------------------------------------------- *
 * Backdoors into mess/vidhrdw/m6847.c                                     *
 * ----------------------------------------------------------------------- */

int internal_m6847_vh_start(const struct m6847_init_params *params, int dirtyramsize);
void internal_m6847_vh_screenrefresh(struct rasterbits_source *rs,
	struct rasterbits_videomode *rvm, struct rasterbits_frame *rf, int full_refresh,
	UINT32 *pens, UINT8 *vrambase, int skew_up, int border_color, int wf,
	int artifact_value, int artifact_palettebase,
	void (*getcolorrgb)(int c, UINT8 *red, UINT8 *green, UINT8 *blue));
int internal_m6847_vblank(int hsyncs, double trailingedgerow, void (*newlineproc)(void));

/* ----------------------------------------------------------------------- *
 * from vidhrdw/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern void coco3_ram_b1_w (offs_t offset, data8_t data);
extern void coco3_ram_b2_w (offs_t offset, data8_t data);
extern void coco3_ram_b3_w (offs_t offset, data8_t data);
extern void coco3_ram_b4_w (offs_t offset, data8_t data);
extern void coco3_ram_b5_w (offs_t offset, data8_t data);
extern void coco3_ram_b6_w (offs_t offset, data8_t data);
extern void coco3_ram_b7_w (offs_t offset, data8_t data);
extern void coco3_ram_b8_w (offs_t offset, data8_t data);
extern void coco3_ram_b9_w (offs_t offset, data8_t data);
extern void coco3_vh_sethires(int hires);
extern int dragon_vh_start(void);
extern int coco2b_vh_start(void);
extern int coco3_vh_start(void);
extern void coco3_vh_stop(void);
extern void coco3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
//extern WRITE_HANDLER ( dragon_sam_display_offset );
//extern WRITE_HANDLER ( dragon_sam_vdg_mode );
extern WRITE_HANDLER ( coco_ram_w );
extern READ_HANDLER ( coco3_gimevh_r );
extern WRITE_HANDLER ( coco3_gimevh_w );
extern WRITE_HANDLER ( coco3_palette_w );
extern void coco3_vh_blink(void);
extern int coco3_vblank(void);

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
extern int dragon32_pak_load(int id);
extern int dragon64_pak_load(int id);
extern int coco3_pak_load(int id);
extern READ_HANDLER ( dragon_mapped_irq_r );
extern READ_HANDLER ( coco3_mapped_irq_r );
//extern WRITE_HANDLER ( dragon64_sam_himemmap );
//extern WRITE_HANDLER ( coco3_sam_himemmap );
extern READ_HANDLER ( coco3_mmu_r );
extern WRITE_HANDLER ( coco3_mmu_w );
extern READ_HANDLER ( coco3_gime_r );
extern WRITE_HANDLER ( coco3_gime_w );
//extern WRITE_HANDLER ( dragon_sam_speedctrl );
//extern WRITE_HANDLER ( coco3_sam_speedctrl );
//extern WRITE_HANDLER ( dragon_sam_page_mode );
//extern WRITE_HANDLER ( dragon_sam_memory_size );
extern READ_HANDLER ( coco_cartridge_r);
extern WRITE_HANDLER ( coco_cartridge_w );
extern READ_HANDLER ( coco3_cartridge_r);
extern WRITE_HANDLER ( coco3_cartridge_w );
extern int coco_floppy_init(int id);
extern void coco_floppy_exit(int id);
extern WRITE_HANDLER( coco_m6847_hs_w );
extern WRITE_HANDLER( coco_m6847_fs_w );
extern WRITE_HANDLER( coco3_m6847_hs_w );
extern WRITE_HANDLER( coco3_m6847_fs_w );
extern int coco3_mmu_translate(int block, int offset);
extern int dragon_floppy_init(int id);
extern int coco_bitbanger_init (int id);
extern void coco_bitbanger_exit (int id);
extern void coco_bitbanger_output (int id, int data);
extern int coco3_calculate_rows(int *bordertop, int *borderbottom);

/* Returns whether a given piece of logical memory is contiguous or not */
extern int coco3_mmu_ismemorycontiguous(int logicaladdr, int len);

/* Reads logical memory into a buffer */
extern void coco3_mmu_readlogicalmemory(UINT8 *buffer, int logicaladdr, int len);

/* Translates a logical address to a physical address */
extern int coco3_mmu_translatelogicaladdr(int logicaladdr);

#define IO_FLOPPY_COCO \
	{\
		IO_FLOPPY,\
		4,\
		"dsk\0",\
		IO_RESET_NONE,\
		basicdsk_floppy_id,\
		dragon_floppy_init,\
		basicdsk_floppy_exit,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL \
    }

#define IO_CARTRIDGE_COCO(loadproc) \
	{\
		IO_CARTSLOT,\
		1,\
		"rom\0",\
		IO_RESET_ALL,\
        NULL,\
		loadproc,\
		NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL\
    }

#define IO_SNAPSHOT_COCOPAK(loadproc) \
	{\
		IO_SNAPSHOT,\
		1,\
		"pak\0",\
		IO_RESET_ALL,\
        NULL,\
		loadproc,\
		NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL\
    }

#define IO_BITBANGER IO_PRINTER

#define IO_BITBANGER_PORT								\
{														\
	IO_BITBANGER,				/* type */				\
	1,							/* count */				\
	"prn\0",					/* file extensions */	\
	IO_RESET_NONE,				/* reset depth */		\
	NULL,						/* id */				\
	coco_bitbanger_init,		/* init */				\
	coco_bitbanger_exit,		/* exit */				\
	NULL,						/* info */				\
	NULL,						/* open */				\
	NULL,						/* close */				\
	NULL,						/* status */			\
	NULL,						/* seek */				\
	NULL,						/* tell */				\
	NULL,						/* input */				\
	coco_bitbanger_output,		/* output */			\
	NULL,						/* input chunk */		\
	NULL						/* output chunk */		\
}

#endif /* DRAGON_H */
