/*********************************************************************

	coco.h

	CoCo/Dragon code

*********************************************************************/

#ifndef DRAGON_H
#define DRAGON_H

#include "vidhrdw/m6847.h"
#include "videomap.h"
#include "devices/snapquik.h"
#include "osdepend.h"

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

void internal_video_update_m6847(int screen, mame_bitmap *bitmap, const rectangle *cliprect, int *do_skip);

/* ----------------------------------------------------------------------- *
 * from vidhrdw/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern int coco3_gimevhreg[8];

void coco3_ram_b1_w (offs_t offset, UINT8 data);
void coco3_ram_b2_w (offs_t offset, UINT8 data);
void coco3_ram_b3_w (offs_t offset, UINT8 data);
void coco3_ram_b4_w (offs_t offset, UINT8 data);
void coco3_ram_b5_w (offs_t offset, UINT8 data);
void coco3_ram_b6_w (offs_t offset, UINT8 data);
void coco3_ram_b7_w (offs_t offset, UINT8 data);
void coco3_ram_b8_w (offs_t offset, UINT8 data);
void coco3_ram_b9_w (offs_t offset, UINT8 data);
void coco3_vh_sethires(int hires);

VIDEO_START( dragon );
VIDEO_START( coco );
VIDEO_START( coco2b );
VIDEO_START( coco3 );
VIDEO_UPDATE( coco3 );

WRITE8_HANDLER ( coco_ram_w );
 READ8_HANDLER ( coco3_gimevh_r );
WRITE8_HANDLER ( coco3_gimevh_w );
WRITE8_HANDLER ( coco3_palette_w );

void coco3_vh_reset(void);
void coco3_vh_blink(void);
int coco3_calculate_rows(int *bordertop, int *borderbottom);

#define REORDERED_VBLANK

/* ----------------------------------------------------------------------- *
 * from machine/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

MACHINE_START( dragon32 );
MACHINE_START( dragon64 );
MACHINE_START( dgnalpha );
MACHINE_START( coco );
MACHINE_START( coco2 );
MACHINE_START( coco3 );

INTERRUPT_GEN( coco3_vh_interrupt );

DEVICE_LOAD(coco_rom);
DEVICE_LOAD(coco3_rom);
DEVICE_UNLOAD(coco_rom);
DEVICE_UNLOAD(coco3_rom);

SNAPSHOT_LOAD ( coco_pak );
SNAPSHOT_LOAD ( coco3_pak );
READ8_HANDLER ( coco3_mmu_r );
WRITE8_HANDLER ( coco3_mmu_w );
READ8_HANDLER ( coco3_gime_r );
WRITE8_HANDLER ( coco3_gime_w );
READ8_HANDLER ( coco_cartridge_r);
WRITE8_HANDLER ( coco_cartridge_w );
READ8_HANDLER ( coco3_cartridge_r);
WRITE8_HANDLER ( coco3_cartridge_w );
int coco_floppy_init(int id);
void coco_floppy_exit(int id);
WRITE8_HANDLER( coco_m6847_hs_w );
WRITE8_HANDLER( coco_m6847_fs_w );
WRITE8_HANDLER( coco3_m6847_hs_w );
WRITE8_HANDLER( coco3_m6847_fs_w );
int coco3_mmu_translate(int bank, int offset);
int coco_bitbanger_init (int id);
READ8_HANDLER( coco_pia_1_r );
READ8_HANDLER( coco3_pia_1_r );
void dragon_sound_update(void);

/* Dragon Alpha AY-8912 */
READ8_HANDLER ( dgnalpha_psg_porta_read );
WRITE8_HANDLER ( dgnalpha_psg_porta_write );

/* Dragon Alpha WD2797 FDC */
READ8_HANDLER(wd2797_r);
WRITE8_HANDLER(wd2797_w);

void coco_set_halt_line(int halt_line);

/* Returns whether a given piece of logical memory is contiguous or not */
int coco3_mmu_ismemorycontiguous(int logicaladdr, int len);

/* Reads logical memory into a buffer */
void coco3_mmu_readlogicalmemory(UINT8 *buffer, int logicaladdr, int len);

/* Translates a logical address to a physical address */
int coco3_mmu_translatelogicaladdr(int logicaladdr);

#define IO_BITBANGER IO_PRINTER
#define IO_VHD IO_HARDDISK

/* CoCo 3 video vars; controlling key aspects of the emulation */
struct coco3_video_vars
{
	UINT8 bordertop_192;
	UINT8 bordertop_199;
	UINT8 bordertop_0;
	UINT8 bordertop_225;
	unsigned int hs_gime_flip : 1;
	unsigned int fs_gime_flip : 1;
	unsigned int hs_pia_flip : 1;
	unsigned int fs_pia_flip : 1;
	UINT16 rise_scanline;
	UINT16 fall_scanline;
};

extern const struct coco3_video_vars coco3_vidvars;

#endif /* DRAGON_H */
