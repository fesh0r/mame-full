#ifndef nes_h
#define nes_h

/* Uncomment this to see all 4 ppu vram pages at once */
//#define BIG_SCREEN

/* Uncomment this to do away with any form of mirroring */
//#define NO_MIRRORING

#define NEW_SPRITE_HIT

/* Uncomment this to have proper emulation of the color intensity */
/* bits, at the expense of speed (and wonked sprites). */
/* NPW 31-Aug-2001 - Uncommented because this is the only way to get it to work at this point */
#define COLOR_INTENSITY

extern unsigned char nes_palette[3*64];
extern int dirtychar[0x200];
extern UINT32 colortable_mono[4*16];

#define BOTTOM_VISIBLE_SCANLINE   239		/* The bottommost visible scanline */
#define NMI_SCANLINE			  244		/* 244 times Bayou Billy perfectly */
#define NTSC_SCANLINES_PER_FRAME  262
#define PAL_SCANLINES_PER_FRAME   305		/* verify - times Elite perfectly */

extern UINT8 *ppu_page[4];
extern void ppu_mirror_h (void);
extern void ppu_mirror_v (void);
extern void ppu_mirror_low (void);
extern void ppu_mirror_high (void);
extern void ppu_mirror_custom (int page, int address);
extern void ppu_mirror_custom_vrom (int page, int address);

extern int current_scanline;
extern int ppu_scanlines_per_frame;

WRITE_HANDLER ( nes_IN0_w );
WRITE_HANDLER ( nes_IN1_w );

extern int nes_vram[8];
extern int nes_vram_sprite[8];
extern char use_vram[512];

extern unsigned char *battery_ram;

extern int PPU_Control0; /* $2000 */
extern int PPU_Control1; /* $2001 */
extern int PPU_Status; /* $2002 */
extern int PPU_Sprite_Addr; /* $2003 */

extern UINT8 PPU_X_fine;

extern UINT16 PPU_refresh_data; /* $2006 */
extern int PPU_tile_page;
extern int PPU_sprite_page;
extern int PPU_add;
extern int PPU_background_color;
extern int PPU_scanline_effects;

enum {
	PPU_c0_inc = 0x04,
	PPU_c0_spr_select = 0x08,
	PPU_c0_chr_select = 0x10,
	PPU_c0_sprite_size = 0x20,
	PPU_c0_NMI = 0x80,

	PPU_c1_background_L8 = 0x02,
	PPU_c1_sprites_L8 = 0x04,
	PPU_c1_background = 0x08,
	PPU_c1_sprites = 0x10,

	PPU_status_8sprites = 0x20,
	PPU_status_sprite0_hit = 0x40,
	PPU_status_vblank = 0x80
};

struct ppu_struct {
	UINT8 control_0;		/* $2000 */
	UINT8 control_1;		/* $2001 */
	UINT8 status;			/* $2002 */
	UINT16 sprite_address;	/* $2003 */

	UINT16 refresh_data;	/* $2005 */
	UINT16 refresh_latch;
	UINT8 x_fine;

	UINT16 address; 		/* $2006 */
	UINT8 address_latch;

	UINT8 data_latch;		/* $2007 - read */

	UINT16 current_scanline;
	UINT8 *page[4];
	UINT16 scanlines_per_frame;
};

struct nes_struct {
	/* load-time cart variables which remain constant */
	UINT8 trainer;
	UINT8 battery;
	UINT8 prg_chunks;
	UINT8 chr_chunks;

	/* system variables which don't change at run-time */
	UINT16 mapper;
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 slow_banking;

	UINT8 *rom;
	UINT8 *vrom;
	UINT8 *vram;
	UINT8 *wram;

	/* Variables which can change */
	UINT8 mid_ram_enable;
};

extern struct nes_struct nes;

struct fds_struct {
	UINT8 *data;
	UINT8 sides;

	/* Variables which can change */
	UINT8 motor_on;
	UINT8 door_closed;
	UINT8 current_side;
	UINT32 head_position;
	UINT8 status0;
	UINT8 read_mode;
	UINT8 write_reg;
};

extern struct fds_struct nes_fds;

/* protos */

/* drivers/nes.c */
READ_HANDLER ( nes_mirrorram_r );
WRITE_HANDLER ( nes_mirrorram_w );
READ_HANDLER ( nes_bogus_r );
extern struct GfxLayout nes_charlayout;

/* machine/nes.c */
DEVICE_LOAD(nes_cart);
DEVICE_LOAD(nes_disk);
DEVICE_UNLOAD(nes_disk);

MACHINE_INIT( nes );
MACHINE_STOP( nes );

void init_nes (void);
void init_nespal (void);
void nes_interrupt (void);
UINT32 nes_partialcrc(const unsigned char *, size_t);
READ_HANDLER  ( nes_ppu_r );
READ_HANDLER  ( nes_IN0_r );
READ_HANDLER  ( nes_IN1_r );
WRITE_HANDLER ( nes_ppu_w );

WRITE_HANDLER ( nes_low_mapper_w );
READ_HANDLER  ( nes_low_mapper_r );
WRITE_HANDLER ( nes_mid_mapper_w );
READ_HANDLER  ( nes_mid_mapper_r );
WRITE_HANDLER ( nes_mapper_w );

/* vidhrdw/nes.c */
extern PALETTE_INIT( nes );
extern VIDEO_START( nes );
extern VIDEO_STOP( nes );
extern VIDEO_UPDATE( nes );

void nes_vh_renderscanline (int scanline);

WRITE_HANDLER ( nes_vh_sprite_dma_w );

#endif

