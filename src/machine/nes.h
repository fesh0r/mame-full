#ifndef nes_h
#define nes_h

#define TOP_VISIBLE_SCANLINE      0			/* The topmost visible scanline */
#define BOTTOM_VISIBLE_SCANLINE   239		/* The bottommost visible scanline */
#define VBLANK_START_SCANLINE     242
#define SCANLINES_PER_FRAME       262

extern int Mirroring;
extern int four_screen_vram;
extern int current_scanline;
extern int current_drawline;

extern int nes_vram[8]; /* Keep track of 8 .5k vram pages to speed decoding */
extern char use_vram[512];
extern unsigned char *VROM;
extern unsigned char *VRAM;
extern unsigned char CHR_Rom;
extern unsigned char PRG_Rom;

extern int PPU_Control0; /* $2000 */
extern int PPU_Control1; /* $2001 */
extern int PPU_Status; /* $2002 */
extern int PPU_Sprite_Addr; /* $2003 */

extern int PPU_Scroll_Y; /* $2005 */
extern int PPU_Scroll_X;

extern int PPU_Addr; /* $2006 */
extern int PPU_name_table;
extern int PPU_one_screen;
extern int PPU_tile_page;
extern int PPU_sprite_page;
extern int PPU_add;
extern int PPU_background_color;
extern int PPU_scanline_effects;

extern int scroll_adjust;

#endif