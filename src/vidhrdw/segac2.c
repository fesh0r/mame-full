/******************************************************************************/
/* vidhrdw\segac2.c                                                           */
/******************************************************************************/

/*** Required *****************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/*** Function Prototypes ******************************************************/

static int  segac2_vdp_data_read (void);
static void segac2_vdp_data_write(int data);
static int  segac2_vdp_control_read (void);
static void segac2_vdp_control_write(int data);
static void vdp_dma_68k(void);
static void vdp_dma_fill(int);
static void vdp_dma_copy(void);

static void segac2_drawline(struct osd_bitmap *bitmap, int line);
static int vdp_gethscroll(int plane, int line);
static int vdp_getvscroll(int plane, int line);
static void draw8pix(unsigned char *bm, unsigned int patno, unsigned char patflip, unsigned int patcol, unsigned char patline);
static void draw8pixs(unsigned char *bm, unsigned int patno, unsigned char patflip, unsigned int patcol, unsigned char patline);



/*** Driver Variables *********************************************************/

/* EXTERNAL (in drivers\segac2.c) */
extern char palbank, palscr, palspr;

/* LOCAL */
static unsigned char* segac2_vdp_vram;
static unsigned char* segac2_vdp_vsram;
static unsigned char* segac2_vdp_regs;

/* vram bases */
static unsigned int segac2_vdp_scrollabase ,segac2_vdp_scrollbbase;
static unsigned int segac2_vdp_windowbase, segac2_vdp_spritebase;
static unsigned int segac2_vdp_hscrollbase;

/* other vdp variables */
static unsigned char  segac2_vdp_hscrollmode, segac2_vdp_vscrollmode;
static unsigned char  segac2_vdp_cmdpart;
static unsigned char  segac2_vdp_code;
static unsigned int   segac2_vdp_address;
static unsigned int   segac2_vdp_dmafill;
static unsigned char  scrollheight, scrollwidth;
static unsigned char  bgcol, bgpal;

/*** General Video Functions **************************************************/

int segac2_vh_start(void)
{
	/* Allocate VRAM etc.. this isn't in 68k Address Space */
	segac2_vdp_vram = malloc(0x10000);
	segac2_vdp_vsram = malloc(0x80);
	segac2_vdp_regs = malloc(0x20);

	palbank = 0;

	segac2_vdp_cmdpart = 0;
	segac2_vdp_code = 0;
	segac2_vdp_address = 0;

	return 0;
}

void segac2_vh_stop(void)
{
	free(segac2_vdp_vram);
	free(segac2_vdp_vsram);
	free(segac2_vdp_regs);

}

void segac2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int line;
	int offs;
	int extraoffs;

	extraoffs =(0x200*palbank) + (0x40*palscr);

	palette_init_used_colors();

	for (offs=(0+extraoffs); offs<(0x40+extraoffs); offs++)
		if (!(offs%16))
			palette_used_colors[offs] = PALETTE_COLOR_TRANSPARENT;
		else
			palette_used_colors[offs] = PALETTE_COLOR_USED;

	extraoffs =(0x200*palbank) + (0x40*palspr);

	for (offs=(0x100+extraoffs); offs<(0x140+extraoffs); offs++)
		if (!(offs%16))
			palette_used_colors[offs] = PALETTE_COLOR_TRANSPARENT;
		else
			palette_used_colors[offs] = PALETTE_COLOR_USED;

    palette_used_colors[bgcol + bgpal*16] = PALETTE_COLOR_USED;

	palette_recalc();

	for (line = 0;line < 224 ; line++ )
	{
	segac2_drawline(bitmap, line);
	}
}

/*** Functions to Emulate the Functionality of the VDP*************************/

READ_HANDLER( segac2_vdp_r )
{
	switch (offset)
	{
		case 0x00:
		case 0x02:
			return (segac2_vdp_data_read());  /* Read Data */
			break;
		case 0x04:
		case 0x06:
			return (segac2_vdp_control_read());  /* Status Register */
			break;
		default:
			logerror("PC:%06x: Unknown VDP Read Offset %02x\n",cpu_get_pc(), offset);
	}
	return (0x00);
}

WRITE_HANDLER( segac2_vdp_w )
{
	switch (offset)
	{
		case 0x00:
		case 0x02:
			segac2_vdp_data_write(data);
			break;
		case 0x04:
		case 0x06:
			segac2_vdp_control_write(data);
			break;
		default:
			logerror("PC:%06x: Unknown VDP Write Offset %02x Data %04x\n",cpu_get_pc(),offset,data);			
	}
}

static int segac2_vdp_data_read (void)  /* Games needing Read to Work .. bloxeed (attract) .. puyo puyo .. probably more */
{
	unsigned int read;

	logerror("PC:%06x: VDP data read",cpu_get_pc());
	segac2_vdp_cmdpart = 0; /* Kill 2nd Write Pending Flag */

	switch (segac2_vdp_code & 0x0F)
	{
		case 0x00: /* VRAM read */
			read =  segac2_vdp_vram[segac2_vdp_address] << 8 |
				    segac2_vdp_vram[segac2_vdp_address+1] ;
			break;
		case 0x04: /* VSRAM read */
			read =  segac2_vdp_vsram[segac2_vdp_address] << 8 |
			     segac2_vdp_vsram[segac2_vdp_address+1] ;
			break;
		default:
			logerror("PC:%06x: VDP illegal read type %02x", cpu_get_pc(), segac2_vdp_code);
			read = 0x00;
			break;
	}
	segac2_vdp_address += segac2_vdp_regs[15]; /* I think .. must check */
	return (read);
}

static void segac2_vdp_data_write (int data)
{
	logerror("PC:%06x: VDP data write %04x\n",cpu_get_pc(), data);
	segac2_vdp_cmdpart = 0; /* Kill 2nd Write Pending Flag */

    if(segac2_vdp_dmafill == 1)
    {
        vdp_dma_fill(data);
        segac2_vdp_dmafill = 0;
        return;
    }

	switch (segac2_vdp_code & 0x0F)
	{
		case 0x01: /* VRAM write */
			segac2_vdp_vram[(segac2_vdp_address & 0xFFFE) ^ (segac2_vdp_address & 1)] = (data >> 8) & 0xFF;
            segac2_vdp_vram[((segac2_vdp_address & 0xFFFE) + 1) ^ (segac2_vdp_address & 1)] = (data >> 0) & 0xFF;
			break;
		case 0x05: /* VSRAM write */
            segac2_vdp_vsram[(segac2_vdp_address & 0x7E)] = (data >> 8) & 0xFF;
            segac2_vdp_vsram[(segac2_vdp_address & 0x7E)+1] = (data >> 0) & 0xFF;
			break;
		default: /* Illegal write attempt */
			logerror("PC:%06x: VDP illegal write type %02x data %04x",cpu_get_pc(), segac2_vdp_code, data);
			break;
	}
	segac2_vdp_address += segac2_vdp_regs[15];
}

static int segac2_vdp_control_read (void)
{
	return (0x00);
}

static void segac2_vdp_control_write (int data)
{
	logerror("PC:%06x: VDP control write %04x\n",cpu_get_pc(), data);
	if (!segac2_vdp_cmdpart) /* We're not expecting the 2nd Half of a Command */
	{
		if ((data & 0xC000) == 0x8000) /* Register Setting command */
		{
			unsigned char regnum = (data >> 8) & 0x1F; /* ---R RRRR ---- ---- */
			unsigned char regdat = data & 0xFF;       /* ---- ---- DDDD DDDD */
			/* In many cases we can just throw the Register Info into the segac2_vdp_regs[] array, however in
			   some cases its useful to set a couple of things directly */
			switch (regnum)
			{
				case 0x02: /* Scroll A Name Table Base */
					segac2_vdp_scrollabase = (regdat & 0x38) << 10;
					break;
				case 0x03: /* Window Name Table Base */
					segac2_vdp_windowbase = (regdat & 0x3E) << 10;
					break;
				case 0x04: /* Scroll B Name Table Base */
					segac2_vdp_scrollbbase = (regdat & 0x07) << 13;
					break;
				case 0x05: /* Sprite Table Base */
					segac2_vdp_spritebase = (regdat & 0x7F) << 9;
					break;
				case 0x07: /* BG Colour */
					bgcol = (regdat & 0x0F);
					bgpal = (regdat & 0x30) >> 4;
					break;
				case 0x0B: /* Scroll Modes */
					segac2_vdp_hscrollmode = (regdat & 0x03);
					segac2_vdp_vscrollmode = (regdat & 0x04) >> 2;
					break;
				case 0x0D: /* HScroll Base */
					segac2_vdp_hscrollbase = (regdat & 0x3F) << 10;
					break;
				case 0x10: /* Scroll Size */
					scrollwidth = (regdat & 0x03);
					scrollwidth = (scrollwidth+1) * 32;
					scrollheight = (regdat & 0x30) >> 4;
					scrollheight = (scrollheight+1) * 32;
					break;
			}
			segac2_vdp_regs[regnum] = regdat;  /* Put Data in the Array */
			logerror("PC:%06x: VDP register %02x set to %02x \n",cpu_get_pc(), regnum, segac2_vdp_regs[regnum]);			
		} else { /* Set up the VDP with the First Part of a 2 Part Command */
			segac2_vdp_code = (segac2_vdp_code & 0x3C) | ((data >> 14) & 0x03);
			segac2_vdp_address = (segac2_vdp_address & 0xC000) | (data & 0x3FFF);
			segac2_vdp_cmdpart = 1;
		}
	} else { /* This is the 2nd Part of a a Command */
		segac2_vdp_cmdpart = 0;
		segac2_vdp_code = (segac2_vdp_code & 0x03) | ((data >> 2) & 0x3C);
		segac2_vdp_address = (segac2_vdp_address & 0x3FFF) | ((data << 14) & 0xC000);
		/* DMA Stuff, If theres a DMA Operation Requested, and We Can, We May as Well */
		if((segac2_vdp_code & 0x20) && (segac2_vdp_regs[1] & 0x10))
        {
            switch(segac2_vdp_regs[23] & 0xC0)
            {
                case 0x00: 
                case 0x40: /* VDP DMA */
                    vdp_dma_68k();
                    break;

                case 0x80: /* VRAM fill */
					logerror("PC:%06x: VDP DMA VRAM Fill Enabled\n",cpu_get_pc());			
                    segac2_vdp_dmafill = 1;
                    break;

                case 0xC0: /* VRAM copy */
					logerror("PC:%06x: VDP DMA VRAM Copy Enabled\n",cpu_get_pc());			
                    vdp_dma_copy();
                    break;
            }
        }
	}
}

static void vdp_dma_68k(void)
{
    int count;
    int length = (segac2_vdp_regs[19] | segac2_vdp_regs[20] << 8) & 0xFFFF;
    int source = (segac2_vdp_regs[21] << 1 | segac2_vdp_regs[22] << 9 | (segac2_vdp_regs[23] & 0x7F) << 17) & 0xFFFFFE;
    if(!length) length = 0xFFFF;
	logerror("PC:%06x: VDP DMA 68k -> VRAM Enabled | Src %06x | Length %04x | Dest %04x \n",cpu_get_pc(),source,length,segac2_vdp_address);			

    for(count = 0; count < length; count++)
    {
		segac2_vdp_data_write ( cpu_readmem24bew_word(source) );
        source += 2;
    }
}

static void vdp_dma_copy(void)
{
    int count;
    int length = (segac2_vdp_regs[19] | segac2_vdp_regs[20] << 8) & 0xFFFF;
    int source = (segac2_vdp_regs[21] | segac2_vdp_regs[22] << 8) & 0xFFFF;
    if(!length) length = 0xFFFF;

    for(count = 0; count < length; count++)
    {
        segac2_vdp_vram[segac2_vdp_address] = segac2_vdp_vram[source];
        source++;
        segac2_vdp_address += segac2_vdp_regs[15];
    }
}

static void vdp_dma_fill(int data)
{
    int count;
    int length = (segac2_vdp_regs[19] | segac2_vdp_regs[20] << 8) & 0xFFFF;
    if(!length) length = 0xffff;

    for(count = 0; count <= length; count++)
    {
        segac2_vdp_vram[(segac2_vdp_address & 0xFFFF) ^ 1] = (data >> 8) & 0xFF;
        segac2_vdp_address += segac2_vdp_regs[15];
    }
}

/*** Functions to Get a Display on the Screen :) ******************************/

static void segac2_drawline(struct osd_bitmap *bitmap, int line)
{
/*******************************************************************************/
/* Rendering a Line to the Screen                                              */
/*******************************************************************************/

	unsigned int linehscroll;
	unsigned int columnvscroll;
	unsigned char *bm; /* Pointer to where we Draw */
	unsigned char column;
	unsigned int tilebase;
	unsigned int temp;
	unsigned int patno;
	unsigned char patcol;
	unsigned char patflip;
	unsigned char patline;

	unsigned char spritcount;
	unsigned char* spritebase;
	unsigned int spritexpos;
	unsigned int spriteypos;
	unsigned char spriteheight;
	unsigned char spritewidth;
	unsigned int spriteattr;
	unsigned char link;
	int spriterpos;

	unsigned char lowsprites, highsprites;
	unsigned int lowlist[80], highlist[80];

	/* Sprites need to be Drawn in Reverse order .. may as well sort them here */

	link = 0; lowsprites = 0; highsprites = 0;
	for (spritcount=0;spritcount<80;spritcount++)
	{
		spritebase = &segac2_vdp_vram[segac2_vdp_spritebase+ (8*link) ];
		if ((spritebase[4] & 0x8000) != 0x8000) /* High Priority */  /* Ok.. must work out how sprite priority should work.. */
		{ highsprites ++; highlist[highsprites] = link;  }
		else
		{ lowsprites ++; lowlist[lowsprites] = link;  }
		link = (spritebase[3] & 0x7F);
		if (!link) break; /* Link is 0, end of Sprite List */
	}

    /* Clear Line */

	bm = (unsigned char *)bitmap->line[line];
	memset (bm, Machine->pens[bgcol + bgpal*16 + 0x200*palbank], 336); /* Clear Line */

	/* Scroll B Low */

	linehscroll = vdp_gethscroll(2,line);
	bm = (unsigned char *)bitmap->line[line] + ( 8 - (linehscroll % 8) );
	for (column = 0 ; column < 41 ; column ++)
	{
		columnvscroll = vdp_getvscroll(2,column);
		temp = ((((line + columnvscroll) >> 3) ) & (scrollheight-1)   )* scrollwidth; 
		tilebase = segac2_vdp_scrollbbase + 2 * temp;
		temp = ((linehscroll >> 3) + column) & (scrollwidth-1);
		tilebase += 2 * temp;
		temp = segac2_vdp_vram[tilebase] << 8 | segac2_vdp_vram[tilebase+1]; /* Get Tile Info */
		if ((temp & 0x8000) == 0) /* This is Low Priority */
		{
			patno = temp & 0x07FF;
			patflip = (temp & 0x1800) >> 11;
			patcol = (temp & 0x6000) >> 13;
			patline = (columnvscroll + line) % 8;
			draw8pix(bm, patno, patflip, patcol, patline);
		}
		bm+=8;
	}

	/* Scroll A Low */

	linehscroll = vdp_gethscroll(0,line);
	bm = (unsigned char *)bitmap->line[line] + ( 8 - (linehscroll % 8) );

	for (column = 0 ; column < 41 ; column ++)
	{
		columnvscroll = vdp_getvscroll(0,column);
		temp = ((((line + columnvscroll) >> 3) )& (scrollheight-1)) * scrollwidth;
		tilebase = segac2_vdp_scrollabase + 2 * temp;
		temp = ((linehscroll >> 3) + column) & (scrollwidth-1);
		tilebase += 2 * temp;
		temp = segac2_vdp_vram[tilebase] << 8 | segac2_vdp_vram[tilebase+1]; /* Get Tile Info */
		if ((temp & 0x8000) == 0) /* This is Low Priority */
		{
			patno = temp & 0x07FF;
			patflip = (temp & 0x1800) >> 11;
			patcol = (temp & 0x6000) >> 13;
			patline = (columnvscroll + line) % 8;
			draw8pix(bm, patno, patflip, patcol, patline);
		}
		bm+=8;
	}

	/* Sprites Low */

	for (spritcount=lowsprites;spritcount>0;spritcount--)
	{
		spritebase = &segac2_vdp_vram[segac2_vdp_spritebase+8*lowlist[spritcount] ];
		spriteypos = (spritebase[0] & 0x03) << 8 |
					 (spritebase[1]       );
		spritexpos = (spritebase[6] & 0x03) << 8 |
			         (spritebase[7]       );
		spriteheight = ((spritebase[2] & 0x03) + 1) * 8;
		spritewidth = (((spritebase[2] & 0x0C) >> 2) + 1) * 8;
		if( ((line + 0x80) >= spriteypos) && ((line + 0x80) < (spriteypos + spriteheight)) && ((spritexpos + spritewidth) >= 0x80) && (spritexpos < 320+128) )
		{
			spriteattr = (spritebase[4] << 8) |
						 (spritebase[5]     ) ;
				spriterpos = spritexpos - 120;
				patno = spriteattr & 0x07FF;
				patflip = (spriteattr & 0x1800) >> 11;
				patcol = (spriteattr & 0x6000) >> 13;
				patline = (line - (spriteypos - 0x80));
				bm = (unsigned char *)bitmap->line[line] + spriterpos;
				for (column=0;column < (spritewidth >>3) ;column++ )
				{
					if ((spriterpos >=0) && (spriterpos < 328))
					{
						draw8pixs(bm, patno, patflip, patcol, patline);
					}
					spriterpos +=8; bm+=8; patno+= spriteheight>>3;
				}
		
		}
	}


	/* Scroll B High */

	linehscroll = vdp_gethscroll(2,line);
	bm = (unsigned char *)bitmap->line[line] + ( 8 - (linehscroll % 8) );

	for (column = 0 ; column < 41 ; column ++)
	{
		columnvscroll = vdp_getvscroll(2,column);
		temp = ((((line + columnvscroll) >> 3) ) & (scrollheight-1)) * scrollwidth;
		tilebase = segac2_vdp_scrollbbase + 2 * temp;
		temp = ((linehscroll >> 3) + column) & (scrollwidth-1);
		tilebase += 2 * temp;
		temp = segac2_vdp_vram[tilebase] << 8 | segac2_vdp_vram[tilebase+1];
		if ((temp & 0x8000) != 0)
		{
			patno = temp & 0x07FF;
			patflip = (temp & 0x1800) >> 11;
			patcol = (temp & 0x6000) >> 13;
			patline = (columnvscroll + line) % 8;
			draw8pix(bm, patno, patflip, patcol, patline);
		}
		bm+=8;
	}

	/* Scroll A High */

	linehscroll = vdp_gethscroll(0,line);
	bm = (unsigned char *)bitmap->line[line] + ( 8 - (linehscroll % 8) );

	for (column = 0 ; column < 41 ; column ++)
	{
		columnvscroll = vdp_getvscroll(0,column);
		temp = ((((line + columnvscroll) >> 3) )& (scrollheight-1) )* scrollwidth;
		tilebase = segac2_vdp_scrollabase + 2 * temp;
		temp = ((linehscroll >> 3) + column) & (scrollwidth-1);
		tilebase += 2 * temp;
		temp = segac2_vdp_vram[tilebase] << 8 | segac2_vdp_vram[tilebase+1];
		if ((temp & 0x8000) != 0)
		{
			patno = temp & 0x07FF;
			patflip = (temp & 0x1800) >> 11;
			patcol = (temp & 0x6000) >> 13;
			patline = (columnvscroll + line) % 8;
			draw8pix(bm, patno, patflip, patcol, patline);
		}
		bm+=8;
	}

	/* Sprites High */

/*
Sprite Attributes Table (in VRAM)
Each Sprite uses 64-bits of Information (8 bytes)
|  x     |  x     |  x     |  x     |  x     |  x     |  YP9   |  YP8   |
|  YP7   |  YP6   |  YP5   |  YP4   |  YP3   |  YP2   |  YP1   |  YP0   |
|  x     |  x     |  x     |  x     |  HS1   |  HS0   |  VS1   |  VS0   |
|  x     |  LN6   |  LN5   |  LN4   |  LN3   |  LN2   |  LN1   |  LN0   |
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
|  x     |  x     |  x     |  x     |  x     |  x     |  XP9   |  XP8   |
|  XP7   |  XP6   |  XP5   |  XP4   |  XP3   |  XP2   |  XP1   |  XP0   |
YP = Y Position of Sprite
HS = Horizontal Size (Blocks)
VS = Vertical Size (Blocks)
LN = Link Field
PRI = Priority
CP = Colour Palette
VF = VFlip
HF = HFlip
PT = Pattern Number
XP = X Position of Sprite
*/

	for (spritcount=highsprites;spritcount>0;spritcount--)
	{
		spritebase = &segac2_vdp_vram[segac2_vdp_spritebase+8*highlist[spritcount] ];
		spriteypos = (spritebase[0] & 0x03) << 8 |
					 (spritebase[1]       );
		spritexpos = (spritebase[6] & 0x03) << 8 |
			         (spritebase[7]       );
		spriteheight = ((spritebase[2] & 0x03) + 1) * 8;
		spritewidth = (((spritebase[2] & 0x0C) >> 2) + 1) * 8;
		if( ((line + 0x80) >= spriteypos) && ((line + 0x80) < (spriteypos + spriteheight)) && ((spritexpos + spritewidth) >= 0x80) && (spritexpos < 320+128) )
		{
			spriteattr = (spritebase[4] << 8) |
						 (spritebase[5]     ) ;
				spriterpos = spritexpos - 120;
				patno = spriteattr & 0x07FF;
				patflip = (spriteattr & 0x1800) >> 11;
				patcol = (spriteattr & 0x6000) >> 13;
				patline = (line - (spriteypos - 0x80));
				bm = (unsigned char *)bitmap->line[line] + spriterpos;
				for (column=0;column < (spritewidth >>3) ;column++ )
				{
					if ((spriterpos >=0) && (spriterpos < 328))
					{
						draw8pixs(bm, patno, patflip, patcol, patline);
					}
					spriterpos +=8; bm+=8; patno+= spriteheight>>3;
				}
		
		}
	}
}

/*** Useful Little Functions **************************************************/

static int vdp_gethscroll(int plane, int line)
{
	/* Note: We Expect plane = 0 for Scroll A, plane = 2 for Scroll B */
	unsigned int vramoffset, temp;

	switch (segac2_vdp_hscrollmode)
	{
		case 0x00: /* Overall Scroll */
			temp = segac2_vdp_vram[segac2_vdp_hscrollbase + plane     ] << 8 |
				   segac2_vdp_vram[segac2_vdp_hscrollbase + 1 + plane];
			temp &= 0x3FF;
			return (0x400-temp);
		case 0x01: /* Illegal! */
			return (0x00);
		case 0x02: /* Every 8 Lines */
			line &= 0xF8; /* Lowest 3 bits of Line # don't matter */
			vramoffset = segac2_vdp_hscrollbase + (4 * line) + plane;
			temp = segac2_vdp_vram[vramoffset] << 8 |
				   segac2_vdp_vram[vramoffset + 1];
			temp &= 0x3FF;
			return (0x400-temp);
		case 0x03: /* Every Line */
			vramoffset = segac2_vdp_hscrollbase + (4 * line) + plane;
			temp = segac2_vdp_vram[vramoffset] << 8 |
				   segac2_vdp_vram[vramoffset + 1];
			temp &= 0x3FF;
			return (0x400-temp);
	}
	return (0x00); /* Won't Happen .. Ever */
}

static int vdp_getvscroll(int plane, int column)
{
	/* Note: We expect plane = 0 for Scroll A, plane = 2 for Scroll B
	   A Column is 8 Pixels Wide                                     */
	unsigned int vsramoffset,temp;

	switch (segac2_vdp_vscrollmode)
	{
		case 0x00: /* Overall Scroll */
			temp = segac2_vdp_vsram[0+plane] << 8 |
				   segac2_vdp_vsram[1+plane];
			temp &= 0x7FF;
			return (temp);
		case 0x01: /* Column Scroll */
			vsramoffset = (4 * (column >> 1)) + plane;
			temp = segac2_vdp_vsram[vsramoffset    ] << 8 |
				   segac2_vdp_vsram[vsramoffset + 1];
			temp &= 0x7FF;
			return (temp);
	}
	return (0x00); /* Won't Happen .. Ever */
}

/*** The Drawing Routine ******************************************************/

static void draw8pix(unsigned char *bm, unsigned int patno, unsigned char patflip, unsigned int patcol, unsigned char patline)
{
	UINT32 *tp; /* Points to Pattern/Tile Data in VRAM */
	unsigned short *paldata; /* Points to the Machine->pens */
	UINT32 mytile;
	unsigned char col;
	unsigned int colbase;

	colbase = 16 * patcol;
	colbase += (0x200 * palbank) + (0x40 * palscr);

	tp = (UINT32*)segac2_vdp_vram + 8*patno;
	paldata = &Machine->pens[colbase];

	switch (patflip)
	{
		case 0x00: /* No Flip */
			tp += patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[6] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[1] = paldata[col];
			}
			break;
		case 0x01: /* Horizontal Flip */
			tp += patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[1] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[6] = paldata[col];
			}
			break;
		case 0x02: /* Vertical Flip */
			tp += 7 - patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[6] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[1] = paldata[col];
			}
			break;
		case 0x03: /* Both Flip */
			tp += 7 - patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[1] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[6] = paldata[col];
			}
			break;
	}
}

/* Same but used for Sprites ... */

static void draw8pixs(unsigned char *bm, unsigned int patno, unsigned char patflip, unsigned int patcol, unsigned char patline)
{
	UINT32 *tp; /* Points to Pattern/Tile Data in VRAM */
	unsigned short *paldata; /* Points to the Machine->pens */
	UINT32 mytile;
	unsigned char col;
	unsigned int colbase;

	colbase = 16 * patcol;
	colbase += (0x200 * palbank) + (0x40 * palspr) + 0x100;

	tp = (UINT32*)segac2_vdp_vram + 8*patno;
	paldata = &Machine->pens[colbase];

	switch (patflip)
	{
		case 0x00: /* No Flip */
			tp += patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[6] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[1] = paldata[col];
			}
			break;
		case 0x01: /* Horizontal Flip */
			tp += patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[1] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[6] = paldata[col];
			}
			break;
		case 0x02: /* Vertical Flip */
			tp += 7 - patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[6] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[1] = paldata[col];
			}
			break;
		case 0x03: /* Both Flip */
			tp += 7 - patline;
			mytile = tp[0];
			if (mytile)
			{
			col = (mytile>> 28)&0x0f; if (col) bm[1] = paldata[col];
			col = (mytile>> 24)&0x0f; if (col) bm[0] = paldata[col];
			col = (mytile>> 20)&0x0f; if (col) bm[3] = paldata[col];
			col = (mytile>> 16)&0x0f; if (col) bm[2] = paldata[col];
			col = (mytile>> 12)&0x0f; if (col) bm[5] = paldata[col];
			col = (mytile>> 8)&0x0f; if (col) bm[4] = paldata[col];
			col = (mytile>> 4)&0x0f; if (col) bm[7] = paldata[col];
			col = (mytile>> 0)&0x0f; if (col) bm[6] = paldata[col];
			}
			break;
	}
}
/******************************************************************************/
/* General Information (Mainly Genesis Related                                */
/******************************************************************************/
/* Genesis VDP Registers (from sega2.doc)

Reg# : |  Bit7  |  Bit6  |  Bit5  |  Bit4  |  Bit3  |  Bit2  |  Bit1  |  Bit0  |    General Function
--------------------------------------------------------------------------------
0x00 : |  0     |  0     |  0     |  IE1   |  0     |  1     |  M3    |  0     |    Mode Set Register #1
IE = Enabled H Interrupt (Lev 4), M3 = Enable HV Counter Read / Stopped
0x01 : |  0     |  DISP  |  IE0   |  M1    |  M2    |  1     |  0     |  0     |    Mode Set Register #2
DISP = Display Enable, IE0 = Enabled V Interrupt (Lev 6), M1 = DMA Enabled, M2 = 30 Cell Mode
0x02 : |  0     |  0     |  SA15  |  SA14  |  SA13  |  0     |  0     |  0     |    Scroll A Base Address
SA13-15 = Bits 13-15 of the Scroll A Name Table Base Address in VRAM
0x03 : |  0     |  0     |  WD15  |  WD14  |  WD13  |  WD12  |  WD11  |  0     |    Window Base Address
WD11-15 = Bits 11-15 of the Window Name Table Base Address in VRAM
0x04 : |  0     |  0     |  0     |   0    |  0     |  SB15  |  SB14  |  SB13  |    Scroll B Base Address
SB13-15 = Bits 13-15 of the Scroll B Name Table Base Address in VRAM
0x05 : |  0     |  AT15  |  AT14  |  AT13  |  AT12  |  AT11  |  AT10  |  AT9   |    Sprite Table Base Address
AT9=15 = Bits 9-15 of the Sprite Name Table Base Address in VRAM
0x06 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x07 : |  0     |  0     |  CPT1  |  CPT0  |  COL3  |  COL2  |  COL1  |  COL0  |    Background Colour Select
CPT0-1 = Palette Number, COL = Colour in Palette
0x08 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x09 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0A : |  BIT7  |  BIT6  |  BIT5  |  BIT4  |  BIT3  |  BIT2  |  BIT1  |  BIT0  |    H-Interrupt Register
BIT0-7 = Controls Level 4 Interrupt Timing
0x0B : |  0     |  0     |  0     |  0     |  IE2   |  VSCR  |  HSCR  |  LSCR  |    Mode Set Register #3
IE2 = Enable E Interrupt (Lev 2), VSCR = Vertical Scroll Mode, HSCR / LSCR = Horizontal Scroll Mode
0x0C : |  RS0   |  0     |  0     |  0     |  S/TE  |  LSM1  |  LSM0  |  RS1   |    Mode Set Register #4
RS0 / RS1 = Cell Mode, S/TE = Shadow/Hilight Enable, LSM0 / LSM1 = Interlace Mode Setting
0x0D : |  0     |  0     |  HS15  |  HS14  |  HS13  |  HS12  |  HS11  |  HS10  |    HScroll Base Address
HS10-15 = Bits 10-15 of the HScroll Name Table Base Address in VRAM
0x0E : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0F : |  INC7  |  INC6  |  INC5  |  INC4  |  INC3  |  INC2  |  INC1  |  INC0  |    Auto-Increment
INC0-7 = Auto Increment Value (after VRam Access)
0x10 : |  0     |  0     |  VSZ1  |  VSZ0  |  0     |  0     |  HSZ1  |  HSZ0  |    Scroll Size
VSZ0-1 = Vertical Plane Size, HSZ0-1 = Horizontal Plane Size
0x11 : |  RIGT  |  0     |  0     |  WHP5  |  WHP4  |  WHP3  |  WHP2  |  WHP1  |    Window H Position
RIGT = Window Right Side of Base Point, WHP1-5 = Bits1-5 of Window H Point
0x12 : |  DOWN  |  0     |  0     |  WVP4  |  WVP3  |  WVP2  |  WVP1  |  WVP0  |    Window V Position
DOWN = Window Below Base Point, WVP0-4 = Bits0-4 of Window V Point
0x13 : |  LG7   |  LG6   |  LG5   |  LG4   |  LG3   |  LG2   |  LG1   |  LG0   |    DMA Length Counter LOW
0x14 : |  LG15  |  LG14  |  LG13  |  LG12  |  LG11  |  LG10  |  LG9   |  LG8   |    DMA Length Counter HIGH
LG0-15 = Bits 0-15 of DMA Length Counter
0x15 : |  SA8   |  SA7   |  SA6   |  SA5   |  SA4   |  SA3   |  SA2   |  SA1   |    DMA Source Address LOW
0x16 : |  SA16  |  SA15  |  SA14  |  SA13  |  SA12  |  SA11  |  SA10  |  SA9   |    DMA Source Address MID
0x17 : |  DMD1  |  DMD0  |  SA22  |  SA21  |  SA20  |  SA19  |  SA18  |  S17   |    DMA Source Address HIGH
LG0-15 = Bits 1-22 of DMA Source Address
DMD0-1 = DMA Mode

Memory Layouts ...

Scroll Name Table
16-bits are used to Define a Tile in the Scroll Plane
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
PRI = Priority, CP = Colour Palette, VF = VFlip, HF = HFlip, PT0-9 = Tile # in VRAM

HScroll Data  (in VRAM)
0x00 |
0x01 / H Scroll of Plane A (Used in Overall, Cell & Line Modes)
0x02 |
0x03 / H Scroll of Plane B (Used in Overall, Cell & Line Modes)
0x04 |
0x05 / H Scroll of Plane A (Used in Line Mode)
0x06 |
0x07 / H Scroll of Plane B (Used in Line Mode)
... 
0x20 |
0x21 / H Scroll of Plane A (Used in Cell & Line Mode)
0x22 |
0x23 / H Scroll of Plane B (Used in Cell & Line Mode)
etc.. That kinda thing :)
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  x     |  HS9   |  HS8   |
|  HS7   |  HS6   |  HS5   |  HS4   |  HS3   |  HS2   |  HS1   |  HS0   |
HS = HScroll Amount for Overall / Cell / Line depending on Mode.

VScroll Data (in VSRAM)
0x00 |
0x01 / V Scroll of Plane A (Used in Overall & Cell Modes)
0x02 |
0x03 / V Scroll of Plane B (Used in Overall & Cell Modes)
0x04 |
0x05 / V Scroll of Plane A (Used in Cell Mode)
0x06 |
0x07 / V Scroll of Plane B (Used in Cell Modes)
etc..
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  VS10  |  VS9   |  VS8   |
|  VS7   |  VS6   |  VS5   |  VS4   |  VS3   |  VS2   |  VS1   |  VS0   |
VS = HScroll Amount for Overall / Cell / Line depending on Mode.

Sprite Attributes Table (in VRAM)
Each Sprite uses 64-bits of Information (8 bytes)
|  x     |  x     |  x     |  x     |  x     |  x     |  YP9   |  YP8   |
|  YP7   |  YP6   |  YP5   |  YP4   |  YP3   |  YP2   |  YP1   |  YP0   |
|  x     |  x     |  x     |  x     |  HS1   |  HS0   |  VS1   |  VS0   |
|  x     |  LN6   |  LN5   |  LN4   |  LN3   |  LN2   |  LN1   |  LN0   |
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
|  x     |  x     |  x     |  x     |  x     |  x     |  XP9   |  XP8   |
|  XP7   |  XP6   |  XP5   |  XP4   |  XP3   |  XP2   |  XP1   |  XP0   |
YP = Y Position of Sprite
HS = Horizontal Size (Blocks)
VS = Vertical Size (Blocks)
LN = Link Field
PRI = Priority
CP = Colour Palette
VF = VFlip
HF = HFlip
PT = Pattern Number
XP = X Position of Sprite



*/

