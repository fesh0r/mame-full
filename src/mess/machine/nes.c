#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"
#include "mess/machine/nes.h"
#include "mess/machine/nes_mmc.h"

//#define VERBOSE_ERRORS

extern int dirtychar[];
extern unsigned char *dirtybuffer2;
extern unsigned char *dirtybuffer3;
extern unsigned char *dirtybuffer4;
extern unsigned char *nes_io_0;
extern unsigned char *nes_io_1;

void nes_vh_renderscanline (int scanline, int drawline);

int Mirroring;
int four_screen_vram;
int current_scanline;
int current_drawline;
int dirty_char;
char use_vram[512];

unsigned char *NES_ROM;
unsigned char *VROM;
unsigned char *VRAM;
unsigned char CHR_Rom;
unsigned char PRG_Rom;

/* PPU Variables */
int PPU_Control0;		// $2000
int PPU_Control1;		// $2001
int PPU_Status;			// $2002
int PPU_Sprite_Addr;	// $2003

int PPU_Scroll_Y;		// These 2 are accessed from $2005
int PPU_Scroll_X;

int PPU_Addr;			// $2006
int PPU_name_table;
int PPU_one_screen;
int PPU_tile_page;
int PPU_sprite_page;
int PPU_background_color;
int PPU_add;
int PPU_scanline_effects;

static char PPU_FirstRead;
static char PPU_latch;
static char	PPU_addr_toggle;
static char	PPU_scroll_toggle;
static int hblank_occurred;

static int Joypad_Read_1;
static int Joypad_Read_2;

void nes_ppu_w (int offset, int data);

/* local prototypes */
static void Write_PPU (int data);
//static void validate_chars (void);

void nes_init_machine (void)
{
//	M6502_Type = M6502_NES;

	current_scanline = 0;
	current_drawline = 0;
	hblank_occurred = 0;

	/* Reset PPU variables */
	PPU_Control0 = PPU_Control1 = PPU_Status = PPU_Scroll_X = PPU_Scroll_Y = 0;
	PPU_Addr = PPU_Sprite_Addr = 0;
	PPU_name_table = 0x2000;
	PPU_one_screen = 0;
	PPU_tile_page = PPU_sprite_page = PPU_background_color = 0;

	PPU_FirstRead = 1;
	PPU_latch = 0;
	PPU_add = 1;
	PPU_background_color = 0;
	PPU_addr_toggle = 0;
	PPU_scroll_toggle = 0;
	if (!readinputport (2))
		PPU_scanline_effects = 1;
	else PPU_scanline_effects = 0;

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	Reset_Mapper (Mapper);

	/* Reset the joypads */
	Joypad_Read_1 = 0;
	Joypad_Read_2 = 0;

//	if (errorlog) fprintf (errorlog, "MMC_low_base: %06x MMC_high_base: %06x\n", MMC_low_base, MMC_high_base);
}

/* TODO: fix the gamepad routines to be readable by humans */
int nes_IN0_r (int offset)
{
	int Joypad_1;

	Joypad_1 = readinputport (0);
	*nes_io_0 = (Joypad_1 >> (Joypad_Read_1 & 0x07)) & 0x01;
	Joypad_Read_1 ++;
//	if (errorlog) fprintf (errorlog, "Joy 0 r, # in a row: %02x\n", Joypad_Read_1);
	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	return (*nes_io_0 | 0x40);
}

int nes_IN1_r (int offset)
{
	int Joypad_2;

	Joypad_2 = readinputport (1);
	*nes_io_1 = (Joypad_2 >> (Joypad_Read_2 & 0x07)) & 0x01;
	Joypad_Read_2 ++;
//	if (errorlog) fprintf (errorlog, "Joy 1 r, # in a row: %02x\n", Joypad_Read_2);
	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	return (*nes_io_1 | 0x40);
}

void nes_IN0_w (int offset, int data)
{
	/* Toggling bit 0 high then low resets the controller */
	if (((*nes_io_0 & 0x01) == 1) && (data == 0))
		Joypad_Read_1 = 0;
	*nes_io_0 = data;
}

void nes_IN1_w (int offset, int data)
{
	/* Toggling bit 0 high then low resets the controller */
	if (((*nes_io_1 & 0x01) == 1) && (data == 0))
		Joypad_Read_2 = 0;
	*nes_io_1 = data;
}

int nes_interrupt (void)
{
	static int first_time_in_vblank = 0;
	int ret;

	ret = M6502_INT_NONE;

	if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
	{
		/* If this mapper supports irqs, check if they are enabled */
		if (uses_irqs)
		{
			/* Decrement & check the IRQ scanline counter */
			if (MMC3_DOIRQ && (--MMC3_IRQ == 0))
			{
				ret = M6502_INT_IRQ;
			}
		}

		/* Set the sprite hit flag */
		if (current_scanline == spriteram[0] + 7)
			PPU_Status |= 0x40;

		/* Render this scanline if appropriate */
		if (PPU_scanline_effects)
		{
			if ((current_scanline >= TOP_VISIBLE_SCANLINE) &&
				(!osd_skip_this_frame()) &&
				(PPU_Control1 & 0x08))

				nes_vh_renderscanline (current_scanline, current_drawline);
		}
	}

	/* Has the vblank started? */
	else if ((current_scanline >= VBLANK_START_SCANLINE) && (first_time_in_vblank == 0))
	{
   		if (errorlog) fprintf (errorlog, "** Vblank started\n");

		/* VBlank in progress, set flag */
		PPU_Status |= 0x80;

		*nes_io_0 &= 0xfd;
		*nes_io_1 &= 0xfd;

//		if (current_scanline == BOTTOM_VISIBLE_SCANLINE + 8)
		{
			/* Check if NMIs are enabled on vblank */
			if (PPU_Control0 & 0x80) ret = M6502_INT_NMI;
		}

		/* This code only needs to be executed once per frame */
		if (first_time_in_vblank == 0)
		{
			first_time_in_vblank = 1;

			/* Reset writes to the scroll register */
			PPU_scroll_toggle = 0;
		}

	}

	/* Increment the scanline pointer & check to see if it's rolled */
	++ current_drawline;
	if ( ++ current_scanline >= SCANLINES_PER_FRAME)
	{
		int i;

		/* vblank is over, start at top of screen again */
		current_scanline = 0;
		current_drawline = 0;
		first_time_in_vblank = 0;
		hblank_occurred = 0;

		/* Clear the vblank & sprite hit flag */
		PPU_Status &= ~0xc0;

   		if (errorlog) fprintf (errorlog, "** New frame\n");

#if 1
		/* TODO: verify - this code assumes games with CHR_Rom won't generate chars on the fly */
		if (CHR_Rom == 0)
		{
			/* Decode any dirty characters */
			for (i = 0; i < 0x200; i ++)
				if (dirtychar[i])
				{
					decodechar(Machine->gfx[1], i, VRAM, Machine->drv->gfxdecodeinfo[1].gfxlayout);
					dirtychar[i] = 0;
					use_vram[i] = 1;
				}
		}
#endif
	}

	if ((ret != M6502_INT_NONE) && (errorlog))
	{
    	fprintf (errorlog, "--- scanline %d", current_scanline);
    	if (ret == M6502_INT_IRQ)
    		fprintf (errorlog, " IRQ\n");
    	else fprintf (errorlog, " NMI\n");
    }
	return ret;
}

int nes_ppu_r (int offset)
{
	int retVal;

//if (errorlog) fprintf (errorlog, "PPU read, offset: %04x\n", offset);

	switch (offset & 0x07)
	{
		case 0: /* PPU Control 0 */
			return PPU_Control0;

		case 1: /* PPU Control 1 */
	 		return PPU_Control1;

/*
    |  $2002  | PPU Status Register (R)                                  |
    |         |   %vhsw----                                              |
    |         |               v = VBlank Occurance                       |
    |         |                      1 = In VBlank                       |
    |         |               h = Sprite #0 Occurance                    |
    |         |                      1 = VBlank has hit Sprite #0        |
    |         |               s = Scanline Sprite Count                  |
    |         |                      0 = Less than 8 sprites on the      |
    |         |                          current scanline                |
    |         |                      1 = More than 8 sprites on the      |
    |         |                          current scanline                |
    |         |               w = VRAM Write Flag                        |
    |         |                      1 = Writes to VRAM are ignored      |
    |         |                                                          |
    |         | NOTE: If read during HBlank and Bit #7 of $2000 is set   |
    |         |       to 0, then switch to Name Table #0.                |
    |         | NOTE: After a read occurs, $2005 is reset, hence the     |
    |         |       next write to $2005 will be Horizontal.            |
    |         | NOTE: Bit #6 (h) is set to 0 after each VBlank.          |
    |         | NOTE: Bit #6 (h) is not set until the first actual non-  |
    |         |       transparent pixel is drawn. Hence, if you have a   |
    |         |       8x8 sprite which has all pixels transparent except |
    |         |       for pixel #4 (horizontally), Bit #6 will be set    |
    |         |       after the 4th pixel is found & drawn.              |
    |         | NOTE: Bit #7 (v) is set to 0 after read occurs.          |
*/
		case 2: /* PPU Status */
		{
			int retval = PPU_Status;

#ifdef VERBOSE_ERRORS
//		if (errorlog) fprintf (errorlog, "PPU status read, data: %02x\n", PPU_Status);
#endif
			/* Clear the vblank flag - takes effect on the next read */
			PPU_Status &= 0x80;
			/* Clear the PPU latches */
			PPU_addr_toggle = 0;
			PPU_scroll_toggle = 0;

			return (retval);
		}
		case 3: /* Sprite Memory Address */
			return (PPU_Sprite_Addr);

		case 4: /* Sprite Memory Data */
			return (PPU_Sprite_Addr);

		case 5: /* PPU Scroll register */
			/* TODO - this can't be right */
			return 0x00;
//			return (RAM [0x2005]);

		case 6: /* PPU Memory Address */
			/* TODO - this can't be right */
			return (PPU_Addr);

		case 7: /* PPU I/O Register */
#if 1
			/* The first read is invalid - gets reset by writes to $2006 */
			if (PPU_FirstRead)
			{
				PPU_FirstRead --;
				return 0x00;
			}

			PPU_Addr &= 0x3fff;
#ifdef VERBOSE_ERRORS
			if (errorlog) fprintf (errorlog, "PPU read from vram - %04x!\n", PPU_Addr);
#endif
			/* Read the data and increment the videoram pointer */
			retVal = videoram[PPU_Addr];
			/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
			/* but, B-Wings, submath (j) seem to use this location differently... */
			if (CHR_Rom && PPU_Addr < 0x2000)
			{
				int vrom_loc;

				vrom_loc = (nes_vram[PPU_Addr >> 10] * 16) + (PPU_Addr & 0x3ff);
				retVal = VROM [vrom_loc];
			}
			PPU_Addr += PPU_add;
			return (retVal);
#else
			PPU_Addr &= 0x3fff;
			if (PPU_Addr >= 0x3f00) PPU_Addr &= 0x3f1f;

			retVal = PPU_latch;

#ifdef VERBOSE_ERRORS
			if (errorlog) fprintf (errorlog, "PPU read from vram - %04x!\n", PPU_Addr);
#endif
			/* Read the data and increment the videoram pointer */
			PPU_latch = videoram[PPU_Addr];
			/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
			/* but, B-Wings, submath (j) seem to use this location differently... */
			if (CHR_Rom && PPU_Addr < 0x2000)
			{
				int vrom_loc;

				vrom_loc = (nes_vram[PPU_Addr >> 10] * 16) + (PPU_Addr & 0x3ff);
				PPU_latch = VROM [vrom_loc];
			}
			PPU_Addr += PPU_add;
			return (retVal);
#endif
		}

    return 0;
}

void nes_ppu_w (int offset, int data)
{

	switch (offset & 0x07)
	{
/*
    |  $2000  | PPU Control Register #1 (W)                              |
    |         |   %vMsbpiNN                                              |
    |         |               v = Execute NMI on VBlank                  |
    |         |                      1 = Enabled                         |
    |         |               M = PPU Selection (unused)                 |
    |         |                      0 = Master                          |
    |         |                      1 = Slave                           |
    |         |               s = Sprite Size                            |
    |         |                      0 = 8x8                             |
    |         |                      1 = 8x16                            |
    |         |               b = Background Pattern Table Address       |
    |         |                      0 = $0000 (VRAM)                    |
    |         |                      1 = $1000 (VRAM)                    |
    |         |               p = Sprite Pattern Table Address           |
    |         |                      0 = $0000 (VRAM)                    |
    |         |                      1 = $1000 (VRAM)                    |
    |         |               i = PPU Address Increment                  |
    |         |                      0 = Increment by 1                  |
    |         |                      1 = Increment by 32                 |
    |         |              NN = Name Table Address                     |
    |         |                     00 = $2000 (VRAM)                    |
    |         |                     01 = $2400 (VRAM)                    |
    |         |                     10 = $2800 (VRAM)                    |
    |         |                     11 = $2C00 (VRAM)                    |
    |         |                                                          |
    |         | NOTE: Bit #6 (M) has no use, as there is only one (1)    |
    |         |       PPU installed in all forms of the NES and Famicom. |
*/
		case 0: /* PPU Control 0 */
			if (current_scanline < BOTTOM_VISIBLE_SCANLINE)
			{
				if (data != PPU_Control0) PPU_scanline_effects = 1;
			}
			PPU_Control0 = data;

			/* We precalculate these variables for speed purposes */
			PPU_name_table = ((PPU_Control0 & 0x03) << 10) | 0x2000;
			/* The char ram bank points either 0x0000 or 0x1000 (page 0 or page 4) */
			PPU_tile_page = (PPU_Control0 & 0x10) >> 2;
			PPU_sprite_page = (PPU_Control0 & 0x08) >> 1;

			if (PPU_Control0 & 0x04)
				PPU_add = 32;
			else
				PPU_add = 1;
#ifdef VERBOSE_ERRORS
			if (errorlog)
			{
				fprintf (errorlog, "------ scanline: %d -------\n", current_scanline);
				fprintf (errorlog, "PPU_w Name table: %04x\n", PPU_name_table);
				fprintf (errorlog, "PPU_w tile page: %04x\n", PPU_tile_page);
				fprintf (errorlog, "PPU_w sprite page: %04x\n", PPU_sprite_page);
				fprintf (errorlog, "---------------------------\n");
			}
#endif
//			if (errorlog) fprintf (errorlog, "W PPU_Control0: %02x\n", PPU_Control0);
			break;
/*
    |  $2001  | PPU Control Register #2 (W)                              |
    |         |   %fffpcsit                                              |
    |         |             fff = Full Background Colour                 |
    |         |                    000 = Black                           |
    |         |                    001 = Red                             |
    |         |                    010 = Blue                            |
    |         |                    100 = Green                           |
    |         |               p = Sprite Visibility                      |
    |         |                      1 = Display                         |
    |         |               c = Background Visibility                  |
    |         |                      1 = Display                         |
    |         |               s = Sprite Clipping                        |
    |         |                      0 = Sprites not displayed in left   |
    |         |                          8-pixel column                  |
    |         |                      1 = No clipping                     |
    |         |               i = Background Clipping                    |
    |         |                      0 = Background not displayed in     |
    |         |                          left 8-pixel column             |
    |         |                      1 = No clipping                     |
    |         |               t = Display Type                           |
    |         |                      0 = Colour display                  |
    |         |                      1 = Mono-type (B&W) display         |
*/
		case 1: /* PPU Control 1 */
			if (current_scanline < BOTTOM_VISIBLE_SCANLINE)
			{
				if (data != PPU_Control1) PPU_scanline_effects = 1;
			}
			PPU_Control1 = data;
//			if (errorlog) fprintf (errorlog, "W PPU_Control1: %02x\n", PPU_Control1);
			break;
		case 2: /* PPU Status */
			if (current_scanline < BOTTOM_VISIBLE_SCANLINE)
			{
				if (data != PPU_Status) PPU_scanline_effects = 1;
			}
			PPU_Status = data;
//			if (errorlog) fprintf (errorlog, "W PPU_Status: %02x\n", PPU_Status);

			break;
		case 3: /* PPU Sprite Memory Address */
			if (current_scanline < BOTTOM_VISIBLE_SCANLINE)
			{
				if (data != PPU_Sprite_Addr) PPU_scanline_effects = 1;
			}
			PPU_Sprite_Addr = data;
			break;
		case 4: /* PPU Sprite Data */
			if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
			{
				if (spriteram[PPU_Sprite_Addr] != data) PPU_scanline_effects = 1;
			}
			spriteram[PPU_Sprite_Addr] = data;
			PPU_Sprite_Addr ++;
			break;

		case 5:
			if (PPU_scroll_toggle)
			/* Vertical scroll */
			{
				/* Writes to y-scroll are ignored in mid-frame */
				if ((current_scanline > BOTTOM_VISIBLE_SCANLINE) || (hblank_occurred))
//				if (PPU_Status & 0x80)
				{
					if (data != PPU_Scroll_Y)
					{
						if (errorlog) fprintf (errorlog, "    yscroll_w: %d\n", data < 240 ? data : data - 256);
					}
					if (data < 240)
						PPU_Scroll_Y =  data;
					else
						PPU_Scroll_Y = data - 256;
					hblank_occurred = 0;
				}
				else
				{
					if (errorlog)
					{
						fprintf (errorlog, "    yscroll %d ignored (mid-frame @ %d)\n", data, current_scanline);
					}
//					PPU_Scroll_Y = 0;
				}
			}
			/* Horizontal scroll */
			else
			{
				if (data != PPU_Scroll_X)
				{
					if (errorlog) fprintf (errorlog, "    xscroll_w: %d\n", data);
					if (current_scanline <= BOTTOM_VISIBLE_SCANLINE) PPU_scanline_effects = 1;
				}
				PPU_Scroll_X = data;
			}

			PPU_scroll_toggle = !PPU_scroll_toggle;
			break;
		case 6: /* PPU Address Register */
			/* PPU Memory Adress */
			if (PPU_addr_toggle)
			{
				PPU_Addr = (PPU_Addr & 0xFF00) | (data);

				if (!(PPU_Status & 0x80) && (current_scanline < BOTTOM_VISIBLE_SCANLINE) && (readinputport (3)))
				{
					/* If the VBL bit is clear, funkiness happens */
					/*
					hi-byte      lo-byte
					00LLSSVV     VVVHHHHH

					The new H-scroll is set to : HHHHH * 8
					The new V-scroll is set to : (VVVVV * 8) + LL
					The origin nametable (as set in bits 0-1 of 2000) is set to : SS

					Data for the following scanlines is then reset such that it's pulled from scanline 0 and on.
 					Thanks to Marat Fayzullin and Arthur Langereis for this info.
					*/

					PPU_Scroll_X = (PPU_Addr & 0x1f) * 8;
					PPU_Scroll_Y = ((PPU_Addr >> 5) & 0x1f) * 8 + ((PPU_Addr >> 12) & 0x03);
					PPU_name_table = (PPU_Addr & 0x0c00) | 0x2000;
					current_drawline = 0;
					hblank_occurred = 0;
					if (errorlog)
					{
						fprintf (errorlog, "=== Mid-screen change @ scanline %d ===\n", current_scanline);
						fprintf (errorlog, "    PPU_Scroll_X: %d\n", PPU_Scroll_X);
						fprintf (errorlog, "    PPU_Scroll_Y: %d\n", PPU_Scroll_Y);
						fprintf (errorlog, "    PPU_name_table: %04x\n", PPU_name_table);
						fprintf (errorlog, "    (PPU_Addr: %04x)\n", PPU_Addr);
						fprintf (errorlog, "    (PPU_Status: %02x)\n", PPU_Status);
						fprintf (errorlog, "===\n");
					}
				}

			}
			else
			{
				PPU_Addr = (PPU_Addr & 0x00FF) | (data << 8);
			}
			PPU_addr_toggle ^= 0x01;
			/* Reset reads from $2007 */
			PPU_FirstRead = 1;
			break;

		case 7: /* PPU I/O Register */

			/* If PPU writes are turned off, ignore this one */
			if (PPU_Status & 0x10) break;
			if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
			{
//				if (errorlog) fprintf (errorlog, "*** PPU write during hblank: %02x: %02x\n", offset & 0x07, data);
				PPU_scanline_effects = 1;
			}
			Write_PPU (data);
			break;
		}
}

static void new_videoram_w (int offset, int data)
{
	/* Handle dirties inside the 4 vram pages */
	if (PPU_Addr >= 0x2000 && PPU_Addr <= 0x2fff)
	{
		if (PPU_Addr < 0x23c0)
		{
			/* videoram 1 */
			if (videoram[PPU_Addr] != data)
				dirtybuffer[PPU_Addr & 0x3ff] = 1;
		}
		else if (PPU_Addr < 0x2400)
		{
			/* color table 1 */
			if (videoram[PPU_Addr] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_Addr & 0x07) << 2) + ((PPU_Addr & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer[tile] = 1;
					}
			}
		}
		else if (PPU_Addr < 0x27c0)
		{
			if (videoram[PPU_Addr] != data)
				dirtybuffer2[PPU_Addr & 0x3ff] = 1;
		}
		else if (PPU_Addr < 0x2800)
		{
			/* color table 2 */
			if (videoram[PPU_Addr] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_Addr & 0x07) << 2) + ((PPU_Addr & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer2[tile] = 1;
					}
			}
		}
		else if (PPU_Addr < 0x2bc0)
		{
			if (videoram[PPU_Addr] != data)
				dirtybuffer3[PPU_Addr & 0x3ff] = 1;
		}
		else if (PPU_Addr < 0x2c00)
		{
			/* color table 3 */
			if (videoram[PPU_Addr] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_Addr & 0x07) << 2) + ((PPU_Addr & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer3[tile] = 1;
					}
			}
		}
		else if (PPU_Addr < 0x2fc0)
		{
			if (videoram[PPU_Addr] != data)
				dirtybuffer4[PPU_Addr & 0x3ff] = 1;
		}
		else
		{
			/* color table 4 */
			if (videoram[PPU_Addr] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_Addr & 0x07) << 2) + ((PPU_Addr & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer4[tile] = 1;
					}
			}
		}
	}
	videoram[PPU_Addr] = data;
}

static void Write_PPU (int data)
{
	PPU_Addr &= 0x3fff;

	if (PPU_Addr < 0x2000)
	{
		/* This ROM writes to the character gen portion of VRAM */
		dirtychar[PPU_Addr >> 4] = 1;
		VRAM[PPU_Addr] = data;

#if 0
		/* Set VRAM latches for MMC4 */
		if (Mapper == 10)
		{
			if (errorlog)
			{
				if (PPU_Addr >= 0xfd0 && PPU_Addr < 0x1000)
					fprintf (errorlog, "MMC2 vrom switch (0xf00): %02x\n", data);
				if (PPU_Addr >= 0x1fd0 && PPU_Addr < 0x2000)
					fprintf (errorlog, "MMC2 vrom switch (0x1f00): %02x\n", data);
			}
			if (PPU_Addr >= 0x0fd0 && PPU_Addr <= 0x0fdf)
				MMC2_bank0 = 0xfd;
			if (PPU_Addr >= 0x0fe0 && PPU_Addr <= 0x0fef)
				MMC2_bank0 = 0xfe;
			if (PPU_Addr >= 0x1fd0 && PPU_Addr <= 0x1fdf)
				MMC2_bank1 = 0xfd;
			if (PPU_Addr >= 0x1fe0 && PPU_Addr <= 0x1fef)
				MMC2_bank1 = 0xfe;
		}
#endif
		if ((errorlog) && (CHR_Rom != 0))
			fprintf (errorlog, "****** PPU write to vram with CHR_ROM - %04x:%02x!\n", PPU_Addr, data);
	}

	if (Mapper == 9)
	{
		if (data == 0xfd)
		{
			int i;
			MMC2_bank1 = 0xfd;
			for (i = 4; i < 8; i ++)
				nes_vram[i] = MMC2_hibank1_val * 256 + 64*(i-4);
		}
		if (data == 0xfe)
		{
			int i;
			MMC2_bank1 = 0xfe;
			for (i = 4; i < 8; i ++)
				nes_vram[i] = MMC2_hibank2_val * 256 + 64*(i-4);
		}
	}

	/* We're using the funky-fresh experimental renderer */
	if (readinputport (2))
	{
		new_videoram_w (PPU_Addr, data);
		if (!four_screen_vram)
		{
			if (Mirroring & 0x02) /* Vertical */
				new_videoram_w (PPU_Addr ^ 0x800, data);
			if (Mirroring & 0x01) /* Horizontal */
				new_videoram_w (PPU_Addr ^ 0x400, data);
		}
	}

	/* Non-funky-fresh vram handling */
	else
	{
		/* Handle mirrored PPU videoram writes */
		if ((PPU_Addr & 0x2000) && (!four_screen_vram))
		{
			if (Mirroring & 0x02) /* Vertical */
				videoram[PPU_Addr ^ 0x800] = data;
			if (Mirroring & 0x01) /* Horizontal */
				videoram[PPU_Addr ^ 0x400] = data;
		}
		videoram[PPU_Addr] = data;
		/* Writes to $3000-$3eff are mirrors of $2000-$2eff, used by i.e. Trojan */
		if (PPU_Addr >= 0x3000 && PPU_Addr <= 0x3eff)
			videoram[PPU_Addr & 0x2fff] = data;
	}

	/* The only valid background colors are writes to 0x3f00 and 0x3f10 */
	/* and even then, they are mirrors of each other. */
	/* As usual, some games attempt to write values > the number of colors so we must mask the data. */
	if (PPU_Addr >= 0x3f00)
	{
		videoram[PPU_Addr] = data;
		data &= 0x3f; /* TODO: should this be 0x3f or 0x7f? */

		if (PPU_Addr & 0x03)
			Machine->gfx[0]->colortable[PPU_Addr & 0x1f] = Machine->pens[data];

		if ((PPU_Addr & 0x0f) == 0)
		{
			int i;

			PPU_background_color = data;
			for (i = 0; i < 0x20; i += 0x04)
				Machine->gfx[0]->colortable[i] = Machine->pens[data];
		}
	}

//end:
	PPU_Addr += PPU_add;
}

extern struct GfxLayout nes_charlayout;

int nes_load_rom (int id)
{
	const char *rom_name = device_filename(IO_CARTSLOT,id);
    FILE *romfile;
	char magic[4];
	char skank[8];
	int local_options;
	char m;
	int Cart_Size;
	int trainer;
	int battery;
	int i;

	if(!rom_name)
	{
		printf("NES requires cartridge!\n");
		return INIT_FAILED;
	}

	if(errorlog) fprintf (errorlog,"Beginning nes_load_rom\n");
	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		if(errorlog) fprintf (errorlog,"image_fopen failed in nes_load_rom.\n");
			return 1;
	}
	if(errorlog) fprintf (errorlog,"Finished osd_Fopen for ROM\n");

	/* Verify the file is in iNES format */
	osd_fread (romfile, magic, 4);
	if(errorlog) fprintf (errorlog,"Cart is in NES format?\n");

	if ((magic[0] != 'N') ||
		(magic[1] != 'E') ||
		(magic[2] != 'S'))
		goto bad;

	osd_fread (romfile, &PRG_Rom, 1);
	osd_fread (romfile, &CHR_Rom, 1);

	Cart_Size = (PRG_Rom << 4) + CHR_Rom * 8;
	if(errorlog) fprintf (errorlog,"Cart file length is %d \n",Cart_Size);

	/* Read the first ROM option byte (offset 6) */
	osd_fread (romfile, &m, 1);
	if(errorlog) fprintf (errorlog,"Read first ROM option BYTE.\n");

	/* Interpret the iNES header flags */
	Mapper = (m & 0xf0) >> 4;
	local_options = m & 0x0f;

	/* Read the second ROM option byte (offset 7) */
	osd_fread (romfile, &m, 1);
	if(errorlog) fprintf (errorlog,"Read second ROM option BYTE.\n");

	/* Check for skanky headers */
	osd_fread (romfile, &skank, 8);
	if(errorlog) fprintf (errorlog,"Checked for skanky headers.\n");

	/* If the header has junk in the unused bytes, assume the extra mapper byte is also invalid */
	/* We only check the first 4 unused bytes for now */
	for (i = 0; i < 4; i ++)
	{
		if (errorlog) fprintf (errorlog, "%02x ", skank[i]);
		if (skank[i] != 0x00)
		{
			if (errorlog) fprintf (errorlog, "(skank: %d)", i);
			m = 0;
		}
	}
	if (errorlog) fprintf (errorlog, "\n");

	Mapper = Mapper | (m & 0xf0);

	/* Set up the Mapper callbacks */
	{
		i = 0;
		mmc_write = 0;

		while (mmc_list[i].iNesMapper != -1)
		{
			if (mmc_list[i].iNesMapper == Mapper)
			{
				mmc_write = mmc_list[i].mmc_write;
			}
			i ++;
		}
		if (mmc_write == 0)
		{
			printf ("Mapper %d is not yet supported, defaulting to no mapper.\n",Mapper);
			mmc_write = mmc_list[0].mmc_write;
		}
	}

	Mirroring = (local_options & 0x01) + 1;
	if (errorlog)
	{
		if (Mirroring == 1) fprintf (errorlog, "Horizontal mirroring\n");
		if (Mirroring == 2) fprintf (errorlog, "Vertical mirroring\n");
	}
	battery = local_options & 0x02;
	if ((errorlog) && (battery)) fprintf (errorlog, "-- Battery found\n");
	trainer = local_options & 0x04;
	/* Check for 4-screen VRAM */
	if (local_options & 0x08) four_screen_vram = 1;
	else four_screen_vram = 0;

	MMC1_extended = 0;
	if (Mapper == 1) MMC1_extended = Cart_Size >> 9;

    /* Allocate memory and set up memory regions */
    if( new_memory_region(REGION_CPU1, 0x10000 + (PRG_Rom+1) * 0x4000) ||
        new_memory_region(REGION_GFX1, (CHR_Rom+1) * 0x2000) ||
        new_memory_region(REGION_GFX2, 0x2000) )
    {
        printf ("Memory allocation failed reading roms!\n");
        goto bad;
    }
	NES_ROM = memory_region(REGION_CPU1);
	VROM = memory_region(REGION_GFX1);
	VRAM = memory_region(REGION_GFX2);

	/* Position past the header */
	osd_fseek (romfile, 16, SEEK_SET);

	/* Load the 0x200 byte trainer at 0x7000 if it exists */
	if (trainer)
	{
		osd_fread (romfile, &NES_ROM[0x7000], 0x200);
		if (errorlog) fprintf (errorlog, "-- Trainer found\n");
	}

	/* Read in the PRG_Rom chunks */

	if (PRG_Rom == 1)
	{
		osd_fread (romfile, &NES_ROM[0x14000], 0x4000);
		/* Mirror this bank into $8000 */
		memcpy (&NES_ROM[0x10000], &NES_ROM [0x14000], 0x4000);
	}
	else
		osd_fread (romfile, &NES_ROM[0x10000], 0x4000 * PRG_Rom);

	if (errorlog) fprintf (errorlog, "**\n");
	if (errorlog) fprintf (errorlog, "Mapper: %d\n", Mapper);
	if (errorlog) fprintf (errorlog, "PRG chunks: %02x, size: %06x\n", PRG_Rom, 0x4000*PRG_Rom);

	/* Read in any CHR_Rom chunks */
	if (CHR_Rom > 0)
	{
		osd_fread (romfile, VROM, 0x2000*CHR_Rom);

		if (CHR_Rom > MAX_GFX_ELEMENTS)
			printf ("Too many CHR-ROM pages!");

		/* Mark each char as not existing in VRAM */
		for (i = 0; i < 512; i ++)
			use_vram[i] = 0;
		/* Calculate the total number of characters to decode */
		nes_charlayout.total = CHR_Rom * 512;
		if (Mapper == 2)
		{
			printf ("Warning: VROM has been found in VRAM-based mapper. Either the mapper is set wrong or the ROM image is incorrect.\n");
		}
	}

	else
	{
		/* Mark each char as existing in VRAM */
		for (i = 0; i < 512; i ++)
			use_vram[i] = 1;
		nes_charlayout.total = 512;
	}

	if (errorlog) fprintf (errorlog, "CHR chunks: %02x, size: %06x\n", CHR_Rom, 0x4000*CHR_Rom);
	if (errorlog) fprintf (errorlog, "**\n");

	/* TODO: load a battery file here */

/*This bit is dumping to DOS! */
	osd_fclose (romfile);
	return 0;

bad:
	if(errorlog) fprintf (errorlog,"BAD section hit during LOAD ROM.\n");
	osd_fclose (romfile);
	return 1;
}

int nes_id_rom (int id)
{
    FILE *romfile;
	unsigned char magic[4];
	int retval;

	if (!(romfile = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0))) return 0;

	retval = 1;
	/* Verify the file is in iNES format */
	osd_fread (romfile, magic, 4);
	if ((magic[0] != 'N') ||
		(magic[1] != 'E') ||
		(magic[2] != 'S'))
		retval = 0;

	osd_fclose (romfile);
	return retval;
}

