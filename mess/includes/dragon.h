#ifndef DRAGON_H
#define DRAGON_H

#include "vidhrdw/m6847.h"
#include "videomap.h"
#include "snapquik.h"

#define COCO_CPU_SPEED_HZ		894886	/* 0.894886 MHz */
#define COCO_FRAMES_PER_SECOND	(COCO_CPU_SPEED_HZ / 57.0 / 263)
#define COCO_CPU_SPEED			(TIME_IN_HZ(COCO_CPU_SPEED_HZ))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)

#define COCO_DIP_ARTIFACTING		12
#define COCO3_DIP_MONITORTYPE		12
#define COCO3_DIP_MONITORTYPE_MASK	0x08

/* ----------------------------------------------------------------------- *
 * Backdoors into mess/vidhrdw/m6847.c                                     *
 * ----------------------------------------------------------------------- */

int internal_video_start_m6847(const struct m6847_init_params *params, const struct videomap_interface *videointf,
	int dirtyramsize);

void internal_m6847_frame_callback(struct videomap_framecallback_info *info, int offset, int border_top, int rows);

struct internal_m6847_linecallback_interface
{
	int width_factor;
	charproc_callback charproc;
	UINT16 (*calculate_artifact_color)(UINT16 metacolor, int artifact_mode);
	int (*setup_dynamic_artifact_palette)(int artifact_mode, UINT8 *bgcolor, UINT8 *fgcolor);
};

void internal_m6847_line_callback(struct videomap_linecallback_info *info, const UINT16 *metapalette,
	struct internal_m6847_linecallback_interface *intf);

UINT8 internal_m6847_charproc(UINT32 c, UINT16 *charpalette, const UINT16 *metapalette, int row, int skew);

int internal_m6847_getadjustedscanline(void);
void internal_m6847_vh_interrupt(int scanline, int rise_scanline, int fall_scanline);

void internal_video_update_m6847(struct mame_bitmap *bitmap, const struct rectangle *cliprect);

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

extern VIDEO_START( dragon );
extern VIDEO_START( coco );
extern VIDEO_START( coco2b );
extern VIDEO_START( coco3 );
extern VIDEO_UPDATE( coco3 );

extern WRITE_HANDLER ( coco_ram_w );
extern READ_HANDLER ( coco3_gimevh_r );
extern WRITE_HANDLER ( coco3_gimevh_w );
extern WRITE_HANDLER ( coco3_palette_w );

extern void coco3_vh_reset(void);
extern void coco3_vh_blink(void);
extern int coco3_calculate_rows(int *bordertop, int *borderbottom);

#define REORDERED_VBLANK

/* ----------------------------------------------------------------------- *
 * from machine/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern MACHINE_INIT( dragon32 );
extern MACHINE_INIT( dragon64 );
extern MACHINE_INIT( coco );
extern MACHINE_INIT( coco2 );
extern MACHINE_INIT( coco3 );
extern MACHINE_STOP( coco );

extern INTERRUPT_GEN( coco3_vh_interrupt );
extern int coco_cassette_init(int id, void *fp, int open_mode);
extern int coco3_cassette_init(int id);
extern int coco_rom_load(int id, void *fp, int open_mode);
extern int coco3_rom_load(int id, void *fp, int open_mode);
extern SNAPSHOT_LOAD ( coco_pak );
extern SNAPSHOT_LOAD ( coco3_pak );
extern READ_HANDLER ( dragon_mapped_irq_r );
extern READ_HANDLER ( coco3_mapped_irq_r );
extern READ_HANDLER ( coco3_mmu_r );
extern WRITE_HANDLER ( coco3_mmu_w );
extern READ_HANDLER ( coco3_gime_r );
extern WRITE_HANDLER ( coco3_gime_w );
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
extern int coco3_mmu_translate(int bank, int offset);
extern int dragon_floppy_init(int id, void *fp, int open_mode);
extern int dragon_floppy_id(int id);
extern void dragon_floppy_exit(int id);
extern int coco_vhd_init(int id, void *fp, int open_mode);
extern void coco_vhd_exit(int id);
extern READ_HANDLER(coco_vhd_io_r);
extern WRITE_HANDLER(coco_vhd_io_w);
extern int coco_bitbanger_init (int id);
extern READ_HANDLER( coco_pia_1_r );
extern READ_HANDLER( coco3_pia_1_r );
extern void dragon_sound_update(void);

extern void coco_set_halt_line(int halt_line);

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
		OSD_FOPEN_RW_CREATE_OR_READ,\
		0,\
		dragon_floppy_init,\
		dragon_floppy_exit,\
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

#define IO_BITBANGER IO_PRINTER
#define IO_VHD IO_HARDDISK

#endif /* DRAGON_H */
