#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"
#include "zlib.h"
#include "image.h"

#define M6502_INT_NONE	0

/* Uncomment this to dump reams of ppu state info to the errorlog */
//#define LOG_PPU

/* Uncomment this to dump info about the inputs to the errorlog */
//#define LOG_JOY

/* Uncomment this to generate prg chunk files when the cart is loaded */
//#define SPLIT_PRG

#define BATTERY_SIZE 0x2000
UINT8 battery_data[BATTERY_SIZE];

static int famicom_image_registered = 0;
struct ppu_struct ppu;
struct nes_struct nes;
struct fds_struct nes_fds;

int ppu_scanlines_per_frame;

UINT8 *ppu_page[4];

int current_scanline;
char use_vram[512];

/* PPU Variables */
int PPU_Control0;		// $2000
int PPU_Control1;		// $2001
int PPU_Status;			// $2002
int PPU_Sprite_Addr;	// $2003

UINT8 PPU_X_fine;

UINT16 PPU_address;		// $2006
UINT8 PPU_address_latch;
UINT16 PPU_refresh_data;
UINT16 PPU_refresh_latch;

int PPU_tile_page;
int PPU_sprite_page;
int PPU_background_color;
int PPU_add;

static UINT8 PPU_data_latch;

static char	PPU_toggle;

static UINT32 in_0[3];
static UINT32 in_1[3];
static UINT32 in_0_shift;
static UINT32 in_1_shift;

//void nes_ppu_w (int offset, int data);

/* local prototypes */
static void init_nes_core (void);
static void ppu_reset (struct ppu_struct *ppu_);
static void Write_PPU (int data);

static mess_image *cartslot_image(void)
{
	return image_from_devtype_and_index(IO_CARTSLOT, 0);
}

static void init_nes_core (void)
{
	/* We set these here in case they weren't set in the cart loader */
	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	battery_ram = nes.wram;

	/* Set up the memory handlers for the mapper */
	switch (nes.mapper)
	{
		case 20:
			nes.slow_banking = 0;
			install_mem_read_handler(0, 0x4030, 0x403f, fds_r);
			install_mem_read_handler(0, 0x6000, 0xdfff, MRA_RAM);
			install_mem_read_handler(0, 0xe000, 0xffff, MRA_ROM);

			install_mem_write_handler(0, 0x4020, 0x402f, fds_w);
			install_mem_write_handler(0, 0x6000, 0xdfff, MWA_RAM);
			install_mem_write_handler(0, 0xe000, 0xffff, MWA_ROM);
			break;
		case 40:
			nes.slow_banking = 1;
			/* Game runs code in between banks, so we do things different */
			install_mem_read_handler(0, 0x6000, 0x7fff, MRA_RAM);
			install_mem_read_handler(0, 0x8000, 0xffff, MRA_ROM);

			install_mem_write_handler(0, 0x6000, 0x7fff, nes_mid_mapper_w);
			install_mem_write_handler(0, 0x8000, 0xffff, nes_mapper_w);
			break;
		default:
			nes.slow_banking = 0;
			install_mem_read_handler(0, 0x6000, 0x7fff, MRA_BANK5);
			install_mem_read_handler(0, 0x8000, 0x9fff, MRA_BANK1);
			install_mem_read_handler(0, 0xa000, 0xbfff, MRA_BANK2);
			install_mem_read_handler(0, 0xc000, 0xdfff, MRA_BANK3);
			install_mem_read_handler(0, 0xe000, 0xffff, MRA_BANK4);
			memory_set_bankhandler_r (1, 0, MRA_BANK1);
			memory_set_bankhandler_r (2, 0, MRA_BANK2);
			memory_set_bankhandler_r (3, 0, MRA_BANK3);
			memory_set_bankhandler_r (4, 0, MRA_BANK4);
			memory_set_bankhandler_r (5, 0, MRA_BANK5);

			install_mem_write_handler(0, 0x6000, 0x7fff, nes_mid_mapper_w);
			install_mem_write_handler(0, 0x8000, 0xffff, nes_mapper_w);
			break;
	}

	/* Set up the mapper callbacks */
	{
		int i = 0;

		while (mmc_list[i].iNesMapper != -1)
		{
			if (mmc_list[i].iNesMapper == nes.mapper)
			{
				mmc_write_low = mmc_list[i].mmc_write_low;
				mmc_read_low = mmc_list[i].mmc_read_low;
				mmc_write_mid = mmc_list[i].mmc_write_mid;
				mmc_write = mmc_list[i].mmc_write;
				ppu_latch = mmc_list[i].ppu_latch;
				mmc_irq = mmc_list[i].mmc_irq;
				break;
			}
			i ++;
		}
		if (mmc_list[i].iNesMapper == -1)
		{
			printf ("Mapper %d is not yet supported, defaulting to no mapper.\n",nes.mapper);
			mmc_write_low = mmc_write_mid = mmc_write = NULL;
			mmc_read_low = NULL;
			ppu_latch = NULL;
			mmc_irq = NULL;
		}
	}

	/* Load a battery file, but only if there's no trainer since they share */
	/* overlapping memory. */
	if (nes.trainer) return;

	/* We need this because battery ram is loaded before the */
	/* memory subsystem is set up. When this routine is called */
	/* everything is ready, so we can just copy over the data */
	/* we loaded before. */
	memcpy (battery_ram, battery_data, BATTERY_SIZE);
}

void init_nes (void)
{
	ppu_scanlines_per_frame = NTSC_SCANLINES_PER_FRAME;
	init_nes_core ();
}

void init_nespal (void)
{
	ppu_scanlines_per_frame = PAL_SCANLINES_PER_FRAME;
	init_nes_core ();
}

MACHINE_INIT( nes )
{
	current_scanline = 0;

	ppu_reset (&ppu);

	/* Some carts have extra RAM and require it on at startup, e.g. Metroid */
	nes.mid_ram_enable = 1;

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset (nes.mapper);

	/* Reset the serial input ports */
	in_0_shift = 0;
	in_1_shift = 0;
}

MACHINE_STOP( nes )
{
	/* Write out the battery file if necessary */
	if (nes.battery)
		image_battery_save(cartslot_image(), battery_ram, BATTERY_SIZE);
}

static void ppu_reset (struct ppu_struct *_ppu)
{
	/* Reset PPU variables */
	PPU_Control0 = PPU_Control1 = PPU_Status = 0;
	PPU_address_latch = 0;
	PPU_data_latch = 0;
	PPU_address = PPU_Sprite_Addr = 0;
	PPU_tile_page = PPU_sprite_page = PPU_background_color = 0;

	PPU_add = 1;
	PPU_background_color = 0;
	PPU_toggle = 0;

	/* Reset mirroring */
#ifdef NO_MIRRORING
	if (1)
#else
	if (nes.four_screen_vram)
#endif
	{
		ppu_page[0] = &(videoram[0x2000]);
		ppu_page[1] = &(videoram[0x2400]);
		ppu_page[2] = &(videoram[0x2800]);
		ppu_page[3] = &(videoram[0x2c00]);
	}
	else switch (nes.hard_mirroring)
	{
		case 0: ppu_mirror_h(); break;
		case 1: ppu_mirror_v(); break;
	}
}

READ_HANDLER ( nes_IN0_r )
{
	int dip;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	retVal = 0x40;

	retVal |= ((in_0[0] >> in_0_shift) & 0x01);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0x0f)
	{
		case 0x01: /* zapper */
			{
				UINT16 pix;
				retVal |= 0x08;  /* no sprite hit */

				/* If button 1 is pressed, indicate the light gun trigger is pressed */
				retVal |= ((in_0[0] & 0x01) << 4);

				/* Look at the screen and see if the cursor is over a bright pixel */
				pix = read_pixel(Machine->scrbitmap, in_0[1], in_0[2]);
				if ((pix == Machine->pens[0x20]) || (pix == Machine->pens[0x30]) ||
					(pix == Machine->pens[0x33]) || (pix == Machine->pens[0x34]))
				{
					retVal &= ~0x08; /* sprite hit */
				}
			}
			break;
		case 0x02: /* multitap */
			/* Handle data line 1's serial output */
//			retVal |= ((in_0[1] >> in_0_shift) & 0x01) << 1;
			break;
	}

#ifdef LOG_JOY
	logerror ("joy 0 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_0_shift, in_0[0]);
#endif

	in_0_shift ++;
	return retVal;
}

READ_HANDLER ( nes_IN1_r )
{
	int dip;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4017 leaves 0x40 there. */
	retVal = 0x40;

	/* Handle data line 0's serial output */
	retVal |= ((in_1[0] >> in_1_shift) & 0x01);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0xf0)
	{
		case 0x10: /* zapper */
			{
				UINT16 pix;
				retVal |= 0x08;  /* no sprite hit */

				/* If button 1 is pressed, indicate the light gun trigger is pressed */
				retVal |= ((in_1[0] & 0x01) << 4);

				/* Look at the screen and see if the cursor is over a bright pixel */
				pix = read_pixel(Machine->scrbitmap, in_1[1], in_1[2]);
				if ((pix == Machine->pens[0x20]) || (pix == Machine->pens[0x30]) ||
					(pix == Machine->pens[0x33]) || (pix == Machine->pens[0x34]))
				{
					retVal &= ~0x08; /* sprite hit */
				}
			}
			break;
		case 0x20: /* multitap */
			/* Handle data line 1's serial output */
//			retVal |= ((in_1[1] >> in_1_shift) & 0x01) << 1;
			break;
		case 0x30: /* arkanoid dial */
			/* Handle data line 2's serial output */
			retVal |= ((in_1[2] >> in_1_shift) & 0x01) << 3;

			/* Handle data line 3's serial output - bits are reversed */
//			retVal |= ((in_1[3] >> in_1_shift) & 0x01) << 4;
			retVal |= ((in_1[3] << in_1_shift) & 0x80) >> 3;
			break;
	}

#ifdef LOG_JOY
	logerror ("joy 1 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_1_shift, in_1[0]);
#endif

	in_1_shift ++;
	return retVal;
}

WRITE_HANDLER ( nes_IN0_w )
{
	int dip;

	if (data & 0x01) return;
#ifdef LOG_JOY
	logerror ("joy 0 bits read: %d\n", in_0_shift);
#endif

	/* Toggling bit 0 high then low resets both controllers */
	in_0_shift = 0;
	in_1_shift = 0;

	in_0[0] = readinputport (0);

	/* Check the fake dip to see what's connected */
	dip = readinputport (2);

	switch (dip & 0x0f)
	{
		case 0x01: /* zapper */
			in_0[1] = readinputport (3); /* x-axis */
			in_0[2] = readinputport (4); /* y-axis */
			break;

		case 0x02: /* multitap */
			in_0[0] |= (readinputport (8) << 8);
			in_0[0] |= (0x08 << 16); /* OR in the 4-player adapter id, channel 0 */

			/* Optional: copy the data onto the second channel */
//			in_0[1] = in_0[0];
//			in_0[1] |= (0x04 << 16); /* OR in the 4-player adapter id, channel 1 */
			break;
	}

	in_1[0] = readinputport (1);

	switch (dip & 0xf0)
	{
		case 0x10: /* zapper */
			if (dip & 0x01)
			{
				/* zapper is also on port 1, use 2nd player analog inputs */
				in_1[1] = readinputport (5); /* x-axis */
				in_1[2] = readinputport (6); /* y-axis */
			}
			else
			{
				in_1[1] = readinputport (3); /* x-axis */
				in_1[2] = readinputport (4); /* y-axis */
			}
			break;

		case 0x20: /* multitap */
			in_1[0] |= (readinputport (9) << 8);
			in_1[0] |= (0x04 << 16); /* OR in the 4-player adapter id, channel 0 */;

			/* Optional: copy the data onto the second channel */
//			in_1[1] = in_1[0];
//			in_1[1] |= (0x08 << 16); /* OR in the 4-player adapter id, channel 1 */
			break;

		case 0x30: /* arkanoid dial */
			in_1[3] = (UINT8) ((UINT8) readinputport (10) + (UINT8)0x52) ^ 0xff;
//			in_1[3] = readinputport (10) ^ 0xff;
//			in_1[3] = 0x02;

			/* Copy the joypad data onto the third channel */
			in_1[2] = in_1[0] /*& 0x01*/;
			break;
	}
}

WRITE_HANDLER ( nes_IN1_w )
{
	return;
}

void nes_interrupt (void)
{
	static int vblank_started = 0;
	int ret;

	ret = M6502_INT_NONE;

	/* See if a mapper generated an irq */
    if (*mmc_irq != NULL) ret = (*mmc_irq)(current_scanline);

	if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
	{
		/* If background or sprites are enabled, copy the ppu address latch */
		if (PPU_Control1 & 0x18)
		{
			/* Copy only the scroll x-coarse and the x-overflow bit */
			PPU_refresh_data &= ~0x041f;
			PPU_refresh_data |= (PPU_refresh_latch & 0x041f);
		}

#ifdef NEW_SPRITE_HIT
		/* If we're not rendering this frame, fake the sprite hit */
		if (osd_skip_this_frame())
#endif
			if ((current_scanline == spriteram[0] + 7) && (PPU_Control1 & 0x10))
			{
				PPU_Status |= PPU_status_sprite0_hit;
#ifdef LOG_PPU
				logerror ("Sprite 0 hit, scanline: %d\n", current_scanline);
#endif
			}

		/* Render this scanline if appropriate */
		if ((PPU_Control1 & 0x18) /*&& !osd_skip_this_frame()*/)
		{
			nes_vh_renderscanline (current_scanline);
		}
	}

	/* Has the vblank started? */
	else if (current_scanline == BOTTOM_VISIBLE_SCANLINE+1)
	{
   		logerror("** Vblank started\n");

		/* Note: never reset the toggle to the scroll/address latches on vblank */

		/* VBlank in progress, set flag */
		PPU_Status |= PPU_status_vblank;
	}

	else if (current_scanline == NMI_SCANLINE)
	{
		/* Check if NMIs are enabled on vblank */
		if (PPU_Control0 & PPU_c0_NMI)
			ret = IRQ_LINE_NMI;
	}

	/* Increment the scanline pointer & check to see if it's rolled */
	if ( ++ current_scanline == ppu_scanlines_per_frame)
	{
		/* vblank is over, start at top of screen again */
		current_scanline = 0;
		vblank_started = 0;

		/* Clear the vblank & sprite hit flag */
		PPU_Status &= ~(PPU_status_vblank | PPU_status_sprite0_hit);

		/* If background or sprites are enabled, copy the ppu address latch */
		if (PPU_Control1 & 0x18)
			PPU_refresh_data = PPU_refresh_latch;

//if (PPU_refresh_data & 0x400) Debugger ();

   		logerror("** New frame\n");

		/* TODO: verify - this code assumes games with chr chunks won't generate chars on the fly */
		/* Pinbot seems to use both VROM and VRAM */
		if ((nes.chr_chunks == 0) || (nes.mapper == 119))
		{
			int i;

			/* Decode any dirty characters */
			for (i = 0; i < 0x200; i ++)
				if (dirtychar[i])
				{
					decodechar(Machine->gfx[1], i, nes.vram, Machine->drv->gfxdecodeinfo[1].gfxlayout);
					dirtychar[i] = 0;
					use_vram[i] = 1;
				}
		}
	}

	if ((ret != M6502_INT_NONE))
	{
    	logerror("--- scanline %d", current_scanline);
    	if (ret == M6502_IRQ_LINE)
    		logerror(" IRQ\n");
    	else logerror(" NMI\n");
    }

	switch(ret) {
	case INTERRUPT_NONE:
		break;

	case IRQ_LINE_NMI:
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
		break;

	default:
		cpu_set_irq_line_and_vector(0, ret, HOLD_LINE, ret);
		break;
	}
}

READ_HANDLER ( nes_ppu_r )
{
	UINT8 retVal=0;
/*
    |  $2002  | PPU Status Register (R)                                  |
    |         |   %vhs-----                                              |
    |         |               v = VBlank Occurance                       |
    |         |                      1 = In VBlank                       |
    |         |               h = Sprite #0 Occurance                    |
    |         |                      1 = VBlank has hit Sprite #0        |
    |         |               s = Scanline Sprite Count                  |
    |         |                      0 = 8 or less sprites on the        |
    |         |                          current scanline                |
    |         |                      1 = More than 8 sprites on the      |
    |         |                          current scanline                |
*/
	switch (offset & 0x07)
	{
		case 0: case 1: case 3: case 5: case 6:
			retVal = 0x00;
			break;

		case 2:
			retVal = PPU_Status;

			/* This is necessary: see W&W1, Gi Joe Atlantis */
			PPU_toggle = 0;

			/* Note that we don't clear the vblank flag - this is correct. */
			/* Many games would break if we did: Dragon Warrior 3, GI Joe Atlantis */
			/* are two. */
			break;

		case 4:
			retVal = spriteram[PPU_Sprite_Addr];
#ifdef LOG_PPU
//	logerror("PPU read (%02x), data: %02x, pc: %04x\n", offset, retVal, activecpu_get_pc ());
#endif
			break;

		case 7:
			retVal = PPU_data_latch;

            if (*ppu_latch != NULL) (*ppu_latch)(PPU_address & 0x3fff);

			if ((PPU_address >= 0x2000) && (PPU_address <= 0x3fef))
				PPU_data_latch = ppu_page[(PPU_address & 0xc00) >> 10][PPU_address & 0x3ff];
			else
				PPU_data_latch = videoram[PPU_address & 0x3fff];

			/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
			/* but, B-Wings, submath (j) seem to use this location differently... */
			if (nes.chr_chunks && (PPU_address & 0x3fff) < 0x2000)
			{
				int vrom_loc;

				vrom_loc = (nes_vram[(PPU_address & 0x1fff) >> 10] * 16) + (PPU_address & 0x3ff);
				PPU_data_latch = nes.vrom [vrom_loc];
			}

#ifdef LOG_PPU
	logerror("PPU read (%02x), data: %02x, ppu_addr: %04x, pc: %04x\n", offset, retVal, PPU_address, activecpu_get_pc ());
#endif
			PPU_address += PPU_add;
			break;
	}

	return retVal;
}

WRITE_HANDLER ( nes_ppu_w )
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
			PPU_Control0 = data;

			PPU_refresh_latch &= ~0x0c00;
			PPU_refresh_latch |= (data & 0x03) << 10;

			/* The char ram bank points either 0x0000 or 0x1000 (page 0 or page 4) */
			PPU_tile_page = (PPU_Control0 & PPU_c0_chr_select) >> 2;
			PPU_sprite_page = (PPU_Control0 & PPU_c0_spr_select) >> 1;

			if (PPU_Control0 & PPU_c0_inc)
				PPU_add = 32;
			else
				PPU_add = 1;
#ifdef LOG_PPU
#if 0
			logerror("------ scanline: %d -------\n", current_scanline);
			logerror("PPU_w Name table: %04x\n", (PPU_refresh_latch & 0xc00) | 0x2000);
			logerror("PPU_w tile page: %04x\n", PPU_tile_page);
			logerror("PPU_w sprite page: %04x\n", PPU_sprite_page);
			logerror("---------------------------\n");
#endif
			logerror("W PPU_Control0: %02x\n", PPU_Control0);
#endif
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
			/* If color intensity has changed, change all the pens */
			if ((data & 0xe0) != (PPU_Control1 & 0xe0))
			{
#ifdef COLOR_INTENSITY
				int i;

				for (i = 0; i <= 0x1f; i ++)
				{
					UINT8 oldColor = videoram[i+0x3f00];

					Machine->gfx[0]->colortable[i] = Machine->pens[oldColor + (data & 0xe0)*2];
				}
#else
#if 0
				int i;
				double r_mod, g_mod, b_mod;

				switch ((data & 0xe0) >> 5)
				{
					case 0: r_mod = 1.0; g_mod = 1.0; b_mod = 1.0; break;
					case 1: r_mod = 1.24; g_mod = .915; b_mod = .743; break;
					case 2: r_mod = .794; g_mod = 1.09; b_mod = .882; break;
					case 3: r_mod = .905; g_mod = 1.03; b_mod = 1.28; break;
					case 4: r_mod = .741; g_mod = .987; b_mod = 1.0; break;
					case 5: r_mod = 1.02; g_mod = .908; b_mod = .979; break;
					case 6: r_mod = 1.02; g_mod = .98; b_mod = .653; break;
					case 7: r_mod = .75; g_mod = .75; b_mod = .75; break;
				}
				for (i = 0; i < 64; i ++)
					palette_set_color (i,
						(double) nes_palette[3*i] * r_mod,
						(double) nes_palette[3*i+1] * g_mod,
						(double) nes_palette[3*i+2] * b_mod);
#endif
#endif
			}
			PPU_Control1 = data;
#ifdef LOG_PPU
			logerror("W PPU_Control1: %02x\n", PPU_Control1);
#endif
			break;
		case 2: /* PPU Status */
#ifdef LOG_PPU
			logerror("W PPU_Status: %02x\n", data);
#endif
			break;
		case 3: /* PPU Sprite Memory Address */
			PPU_Sprite_Addr = data;
			break;
		case 4: /* PPU Sprite Data */
			spriteram[PPU_Sprite_Addr] = data;
			PPU_Sprite_Addr ++;
			PPU_Sprite_Addr &= 0xff;
			break;

		case 5:
			if (PPU_toggle)
			/* (second write) */
			{
				PPU_refresh_latch &= ~0x03e0;
				PPU_refresh_latch |= (data & 0xf8) << 2;

				PPU_refresh_latch &= ~0x7000;
				PPU_refresh_latch |= (data & 0x07) << 12;
#ifdef LOG_PPU
logerror ("write ppu scroll latch (Y): %02x (scanline: %d) refresh_latch: %04x\n", data, current_scanline, PPU_refresh_latch);
#endif
			}
			/* (first write) */
			else
			{
				PPU_refresh_latch &= ~0x1f;
				PPU_refresh_latch |= (data & 0xf8) >> 3;

				PPU_X_fine = data & 0x07;
#ifdef LOG_PPU
logerror ("write ppu scroll latch (X): %02x (scanline: %d) refresh_latch: %04x\n", data, current_scanline, PPU_refresh_latch);
#endif
			}
			PPU_toggle = !PPU_toggle;
			break;

		case 6: /* PPU Address Register */
			/* PPU Memory Adress */
			if (PPU_toggle)
			{
#ifdef LOG_PPU
//if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
	logerror ("write ppu address (low): %02x (%d)\n", data, current_scanline);
#endif
				PPU_address = (PPU_address_latch << 8) | data;

				PPU_refresh_latch &= ~0x00ff;
				PPU_refresh_latch |= data;
				PPU_refresh_data = PPU_refresh_latch;
			}
			else
			{
#ifdef LOG_PPU
//if (current_scanline <= BOTTOM_VISIBLE_SCANLINE)
	logerror ("write ppu address (high): %02x (%d)\n", data, current_scanline);
#endif
				PPU_address_latch = data;

				if (data != 0x3f) /* TODO: remove this hack! */
				{
					PPU_refresh_latch &= ~0xff00;
					PPU_refresh_latch |= (data & 0x3f) << 8;
				}
			}
			PPU_toggle = !PPU_toggle;
			break;

		case 7: /* PPU I/O Register */

			if ((current_scanline <= BOTTOM_VISIBLE_SCANLINE) /*&& (PPU_Control1 & 0x18)*/)
			{
//				logerror("*** PPU write during hblank (%d) ",  current_scanline);
			}
			Write_PPU (data);
			break;
		}
}

void ppu_mirror_h (void)
{
	if (nes.four_screen_vram) return;

#ifdef LOG_PPU
	logerror ("mirror: horizontal\n");
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[0] = &(videoram[0x2000]);
	ppu_page[1] = &(videoram[0x2000]);
	ppu_page[2] = &(videoram[0x2400]);
	ppu_page[3] = &(videoram[0x2400]);
}

void ppu_mirror_v (void)
{
	if (nes.four_screen_vram) return;

#ifdef LOG_PPU
	logerror ("mirror: vertical\n");
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[0] = &(videoram[0x2000]);
	ppu_page[1] = &(videoram[0x2400]);
	ppu_page[2] = &(videoram[0x2000]);
	ppu_page[3] = &(videoram[0x2400]);
}

void ppu_mirror_low (void)
{
	if (nes.four_screen_vram) return;

#ifdef LOG_PPU
	logerror ("mirror: $2000\n");
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[0] = &(videoram[0x2000]);
	ppu_page[1] = &(videoram[0x2000]);
	ppu_page[2] = &(videoram[0x2000]);
	ppu_page[3] = &(videoram[0x2000]);
}

void ppu_mirror_high (void)
{
	if (nes.four_screen_vram) return;

#ifdef LOG_PPU
	logerror ("mirror: $2400\n");
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[0] = &(videoram[0x2400]);
	ppu_page[1] = &(videoram[0x2400]);
	ppu_page[2] = &(videoram[0x2400]);
	ppu_page[3] = &(videoram[0x2400]);
}

void ppu_mirror_custom (int page, int address)
{
	if (nes.four_screen_vram) return;

	address = (address << 10) | 0x2000;

#ifdef LOG_PPU
	logerror ("mirror custom, page: %d, address: %04x\n", page, address);
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[page] = &(videoram[address]);
}

void ppu_mirror_custom_vrom (int page, int address)
{
	if (nes.four_screen_vram) return;

#ifdef LOG_PPU
	logerror ("mirror custom vrom, page: %d, address: %04x\n", page, address);
#endif

#ifdef NO_MIRRORING
	return;
#endif

	ppu_page[page] = &(nes.vrom[address]);
}

#if 0
static void new_videoram_w (int offset, int data)
{
	/* Handle dirties inside the 4 vram pages */
	if (PPU_address >= 0x2000 && PPU_address <= 0x2fff)
	{
		if (PPU_address < 0x23c0)
		{
			/* videoram 1 */
			if (videoram[PPU_address] != data)
				dirtybuffer[PPU_address & 0x3ff] = 1;
		}
		else if (PPU_address < 0x2400)
		{
			/* color table 1 */
			if (videoram[PPU_address] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_address & 0x07) << 2) + ((PPU_address & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer[tile] = 1;
					}
			}
		}
		else if (PPU_address < 0x27c0)
		{
			if (videoram[PPU_address] != data)
				dirtybuffer2[PPU_address & 0x3ff] = 1;
		}
		else if (PPU_address < 0x2800)
		{
			/* color table 2 */
			if (videoram[PPU_address] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_address & 0x07) << 2) + ((PPU_address & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer2[tile] = 1;
					}
			}
		}
		else if (PPU_address < 0x2bc0)
		{
			if (videoram[PPU_address] != data)
				dirtybuffer3[PPU_address & 0x3ff] = 1;
		}
		else if (PPU_address < 0x2c00)
		{
			/* color table 3 */
			if (videoram[PPU_address] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_address & 0x07) << 2) + ((PPU_address & 0x38) << 4);
				for (y = 0; y < 4; y ++)
					for (x = 0; x < 4; x ++)
					{
						int tile;

						tile = i + (y * 0x20) + x;
						if (tile < 0x3c0) dirtybuffer3[tile] = 1;
					}
			}
		}
		else if (PPU_address < 0x2fc0)
		{
			if (videoram[PPU_address] != data)
				dirtybuffer4[PPU_address & 0x3ff] = 1;
		}
		else
		{
			/* color table 4 */
			if (videoram[PPU_address] != data)
			{
				int x, y, i;

				/* dirty a 4x4 grid */
				i = ((PPU_address & 0x07) << 2) + ((PPU_address & 0x38) << 4);
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
	videoram[PPU_address] = data;
}
#endif

static void Write_PPU (int data)
{
	int tempAddr = PPU_address & 0x3fff;

    if (*ppu_latch != NULL) (*ppu_latch)(tempAddr);

#ifdef LOG_PPU
	logerror ("   Write_PPU %04x: %02x\n", tempAddr, data);
#endif
	if (tempAddr < 0x2000)
	{
		/* This ROM writes to the character gen portion of VRAM */
		dirtychar[tempAddr >> 4] = 1;
		nes.vram[tempAddr] = data;
		videoram[tempAddr] = data;

		if (nes.chr_chunks != 0)
			logerror("****** PPU write to vram with CHR_ROM - %04x:%02x!\n", tempAddr, data);
		goto end;
	}

	/* The only valid background colors are writes to 0x3f00 and 0x3f10 */
	/* and even then, they are mirrors of each other. */
	/* As usual, some games attempt to write values > the number of colors so we must mask the data. */
	if (tempAddr >= 0x3f00)
	{
		videoram[tempAddr] = data;
		data &= 0x3f;

		if (tempAddr & 0x03)
		{
#ifdef COLOR_INTENSITY
			Machine->gfx[0]->colortable[tempAddr & 0x1f] = Machine->pens[data + (PPU_Control1 & 0xe0)*2];
			colortable_mono[tempAddr & 0x1f] = Machine->pens[(data & 0xf0) + (PPU_Control1 & 0xe0)*2];
#else
			Machine->gfx[0]->colortable[tempAddr & 0x1f] = Machine->pens[data];
			colortable_mono[tempAddr & 0x1f] = Machine->pens[data & 0xf0];
#endif
		}

		if ((tempAddr & 0x0f) == 0)
		{
			int i;

			PPU_background_color = data;
			for (i = 0; i < 0x20; i += 0x04)
			{
#ifdef COLOR_INTENSITY
				Machine->gfx[0]->colortable[i] = Machine->pens[data + (PPU_Control1 & 0xe0)*2];
				colortable_mono[i] = Machine->pens[(data & 0xf0) + (PPU_Control1 & 0xe0)*2];
#else
				Machine->gfx[0]->colortable[i] = Machine->pens[data];
				colortable_mono[i] = Machine->pens[data & 0xf0];
#endif
			}
		}
		goto end;
	}

	/* everything else */
	else
	{
		/* Writes to $3000-$3eff are mirrors of $2000-$2eff, used by e.g. Trojan */
		int page = (PPU_address & 0x0c00) >> 10;
		int address = PPU_address & 0x3ff;

		ppu_page[page][address] = data;
	}

end:
	PPU_address += PPU_add;
}

DEVICE_LOAD(nes_cart)
{
	const char *mapinfo;
	int mapint1=0,mapint2=0,mapint3=0,mapint4=0,goodcrcinfo = 0;
	char magic[4];
	char skank[8];
	int local_options = 0;
	char m;
	int i;

	/* Verify the file is in iNES format */
	mame_fread (file, magic, 4);

	if ((magic[0] != 'N') ||
		(magic[1] != 'E') ||
		(magic[2] != 'S'))
		goto bad;

	mapinfo = image_extrainfo(image);
	if (mapinfo)
	{
		if (4 == sscanf(mapinfo,"%d %d %d %d",&mapint1,&mapint2,&mapint3,&mapint4))
		{
			nes.mapper = mapint1;
			local_options = mapint2;
			nes.prg_chunks = mapint3;
			nes.chr_chunks = mapint4;
			logerror("NES.CRC info: %d %d %d %d\n",mapint1,mapint2,mapint3,mapint4);
			goodcrcinfo = 1;
		} else
		{
			logerror("NES: [%s], Invalid mapinfo found\n",mapinfo);
		}
	} else
	{
		logerror("NES: No extrainfo found\n");
	}
	if (!goodcrcinfo)
	{
		mame_fread (file, &nes.prg_chunks, 1);
		mame_fread (file, &nes.chr_chunks, 1);
		/* Read the first ROM option byte (offset 6) */
		mame_fread (file, &m, 1);

		/* Interpret the iNES header flags */
		nes.mapper = (m & 0xf0) >> 4;
		local_options = m & 0x0f;


		/* Read the second ROM option byte (offset 7) */
		mame_fread (file, &m, 1);

		/* Check for skanky headers */
		mame_fread (file, &skank, 8);

		/* If the header has junk in the unused bytes, assume the extra mapper byte is also invalid */
		/* We only check the first 4 unused bytes for now */
		for (i = 0; i < 4; i ++)
		{
			logerror("%02x ", skank[i]);
			if (skank[i] != 0x00)
			{
				logerror("(skank: %d)", i);
//				m = 0;
			}
		}
		logerror("\n");

		nes.mapper = nes.mapper | (m & 0xf0);
	}

	nes.hard_mirroring = local_options & 0x01;
	nes.battery = local_options & 0x02;
	nes.trainer = local_options & 0x04;
	nes.four_screen_vram = local_options & 0x08;

	if (nes.battery) logerror("-- Battery found\n");
	if (nes.trainer) logerror("-- Trainer found\n");
	if (nes.four_screen_vram) logerror("-- 4-screen VRAM\n");

	/* Free the regions that were allocated by the ROM loader */
    free_memory_region (REGION_CPU1);
    free_memory_region (REGION_GFX1);

    /* Allocate them again with the proper size */
    if( new_memory_region(REGION_CPU1, 0x10000 + (nes.prg_chunks+1) * 0x4000,0) ||
        new_memory_region(REGION_GFX1, (nes.chr_chunks+1) * 0x2000,0) )
    {
        printf ("Memory allocation failed reading roms!\n");
        goto bad;
    }

	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	/* Position past the header */
	mame_fseek (file, 16, SEEK_SET);

	/* Load the 0x200 byte trainer at 0x7000 if it exists */
	if (nes.trainer)
	{
		mame_fread (file, &nes.wram[0x1000], 0x200);
	}

	/* Read in the program chunks */
	if (nes.prg_chunks == 1)
	{
		mame_fread (file, &nes.rom[0x14000], 0x4000);
		/* Mirror this bank into $8000 */
		memcpy (&nes.rom[0x10000], &nes.rom [0x14000], 0x4000);
	}
	else
		mame_fread (file, &nes.rom[0x10000], 0x4000 * nes.prg_chunks);

#ifdef SPLIT_PRG
{
	FILE *prgout;
	char outname[255];

	for (i = 0; i < nes.prg_chunks; i ++)
	{
		sprintf (outname, "%s.p%d", battery_name, i);
		prgout = fopen (outname, "wb");
		if (prgout)
		{
			fwrite (&nes.rom[0x10000 + 0x4000 * i], 1, 0x4000, prgout);
			fclose (prgout);
		}
	}
}
#endif

	logerror("**\n");
	logerror("Mapper: %d\n", nes.mapper);
	logerror("PRG chunks: %02x, size: %06x\n", nes.prg_chunks, 0x4000*nes.prg_chunks);

	/* Read in any chr chunks */
	if (nes.chr_chunks > 0)
	{
		mame_fread (file, nes.vrom, 0x2000*nes.chr_chunks);

		/* Mark each char as not existing in VRAM */
		for (i = 0; i < 512; i ++)
			use_vram[i] = 0;
		/* Calculate the total number of characters to decode */
		nes_charlayout.total = nes.chr_chunks * 512;
		if (nes.mapper == 2)
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

	logerror("CHR chunks: %02x, size: %06x\n", nes.chr_chunks, 0x4000*nes.chr_chunks);
	logerror("**\n");

	/* Attempt to load a battery file for this ROM. If successful, we */
	/* must wait until later to move it to the system memory. */
	if (nes.battery)
		image_battery_load(image, battery_data, BATTERY_SIZE);

	famicom_image_registered = 1;
	return INIT_PASS;

bad:
	logerror("BAD section hit during LOAD ROM.\n");
	return INIT_FAIL;
}

// extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

UINT32 nes_partialcrc(const unsigned char *buf, size_t size)
{
	UINT32 crc;
	if (size < 17)
		return 0;
	crc = (UINT32) crc32(0L, &buf[16], size-16);
	logerror("NES Partial CRC: %08lx %d\n", (long) crc, (int)size);
	return crc;
}

DEVICE_LOAD(nes_disk)
{
	unsigned char magic[4];

	/* See if it has a fucking redundant header on it */
	mame_fread (file, magic, 4);
	if ((magic[0] == 'F') &&
		(magic[1] == 'D') &&
		(magic[2] == 'S'))
	{
		/* Skip past the fucking redundant header */
		mame_fseek (file, 0x10, SEEK_SET);
	}
	else
		/* otherwise, point to the start of the image */
		mame_fseek (file, 0, SEEK_SET);

	/* clear some of the cart variables we don't use */
	nes.trainer = 0;
	nes.battery = 0;
	nes.prg_chunks = nes.chr_chunks = 0;

	nes.mapper = 20;
	nes.four_screen_vram = 0;
	nes.hard_mirroring = 0;

	nes_fds.sides = 0;
	nes_fds.data = NULL;

	/* read in all the sides */
	while (!mame_feof (file))
	{
		nes_fds.sides ++;
		nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);
		if (!nes_fds.data)
			return INIT_FAIL;
		mame_fread (file, nes_fds.data + ((nes_fds.sides-1) * 65500), 65500);
	}

	/* adjust for eof */
	nes_fds.sides --;
	nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);

	logerror ("Number of sides: %d\n", nes_fds.sides);

	famicom_image_registered = 1;
	return INIT_PASS;

//bad:
	logerror("BAD section hit during disk load.\n");
	return 1;
}

DEVICE_UNLOAD(nes_disk)
{
	/* TODO: should write out changes here as well */
	nes_fds.data = NULL;
}

