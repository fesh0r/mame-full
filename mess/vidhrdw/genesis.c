#if 1

/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Modified the GfxElement Def...  please check ;-)
  	Ben

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "machine/genesis.h"
#include "vidhrdw/genesis.h"

/* skeleton code included from closet.h */
#ifdef GARETHS_LITTLE_SECRET
#define DIM >>3
#else
#define DIM /*quack!*/
#endif

#define BLOCK_SHIFT				3

#define PAL 					1
#define NTSC 0
#define FIFO_EMPTY 0
#define FIFO_FULL 1
#define DMA_BUSY 1
#define DMA_FREE 0
#define MODE_VRAM_WRITE			1
#define MODE_CRAM_WRITE			3
#define MODE_VSRAM_WRITE		5
#define MODE_VRAM_READ			0
#define MODE_CRAM_READ			8
#define MODE_VSRAM_READ			4
#define MODE_VRAM_WRITE_DMA		0x21
#define MODE_CRAM_WRITE_DMA		0x23
#define MODE_VSRAM_WRITE_DMA	0x25
#define MODE_VRAM_READ_DMA		0x20
#define MODE_CRAM_READ_DMA		0x28
#define MODE_VSRAM_READ_DMA		0x24


#define DMA_ROM_VRAM			0
#define DMA_RAM_VRAM			1
#define DMA_VRAM_FILL			2
#define DMA_VRAM_COPY			3

char vdp_dma_mode 			=	DMA_FREE;
char vdp_id; /* above, same with two below */
char *vdp_data_ptr;
int vdp_address;
int vdp_ctrl_status			=	2;
int vdp_h_interrupt 		=	0;
int	hv_counter_stopped 		=	0;
int	vdp_display_enable  	=	0;
int	vdp_v_interrupt			=	0;
int	vdp_dma_enable			=	0;
int vdp_vram_fill			=	0;
int	vdp_30cell_enable		=	1;
int vdp_display_height 		=	224;
int	vdp_fifo_full			=	0;
int vdp_hv_status			=	0;
char *	vdp_pattern_scroll_a=	0;
char *	vdp_pattern_window	=	0;
char *	vdp_pattern_scroll_b=	0;
unsigned char *	vdp_pattern_sprite	=	0;
int	vdp_background_colour 	=	0;
int	vdp_background_palette	=	0;
int	vdp_h_interrupt_reg		=	0;
int	vdp_scrollmode			=	0;
int	vdp_screenmode			=	0;
char *	vdp_h_scroll_addr	=	0;
int	vdp_auto_increment		=	0;
int	vdp_h_scrollsize		=	0;
int vdp_interlace			=	0;
int	vdp_h_width				=	256;
int	vdp_v_scrollsize		=	0;
int	vdp_h_scrollmode		=	0;
int	vdp_v_scrollmode		=	0;
int	vdp_scroll_height		=	224;
int	vdp_window_h			=	0;
int	vdp_window_v			=	0;
int	vdp_dma_length			=	0;
int	vdp_dma_source			=	0;
int	vdp_dma_busy			=	0;
int vdp_pal_mode			=	NTSC;
int numberofsprites			=	0;


unsigned short vdp_cram[0x1000]; /* 64 x 9 bits rounded up */
unsigned char vdp_vram[0x10000]; /* 64K VRAM */
short vdp_vsram[0x1000]; /* 40 x 10 bits scroll RAM, rounded up */

unsigned char colours2[]={		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                        		0,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  								0,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
                        		0,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
                        		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                        		0,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  								0,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
                        		0,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
                        		0,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
                        		0,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
                        		0,161,162,163,163,165,166,167,168,169,170,171,172,173,174,175,
                        		0,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
                        		0,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
                        		0,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
                        		0,161,162,163,163,165,166,167,168,169,170,171,172,173,174,175,
                        		0,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
              			  };
unsigned char * current_dma_src = 0;
unsigned char * current_dma_dest = 0;
unsigned char * current_dma_end = 0;
int current_dma_vram_dest = 0;
int current_dma_increment = 0;
int current_dma_id = 0;

//struct mame_bitmap *scroll_a;
//struct mame_bitmap *scroll_b;
//struct mame_bitmap *bitmap_vram;
//struct mame_bitmap *bitmap_sprite;
struct mame_bitmap *bitmap2;

struct mame_bitmap *spritelayer;

unsigned short colours[256];

char dirty_colour[64];

short dirty_attribute_a[16384];
short dirty_attribute_b[16384];

char *tile_changed_1 = NULL, *tile_changed_2 = NULL;

struct GfxElement scroll_element =
{
	8, 8,
	0,
	2048,
	16,      /* something wrong */
	0,
	0,
	0,
	64,
	0
};

typedef struct
{
	struct mame_bitmap *bitmap;
	int x;
	int y;
	int attribute;
	int width;
	int height;
	int x_end;
	int y_end;
} GENESIS_SPRITE;





PALETTE_INIT( genesis )
{
	int i;

	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
		palette_set_color(i, ((i & 1) >> 0) * 0xff,
							 ((i & 2) >> 1) * 0xff,
							 ((i & 4) >> 2) * 0xff);
}


WRITE16_HANDLER ( genesis_videoram1_w )
{
	logerror("what is this doing? %x, %x\n", offset, data);
	//offset = data;
}

VIDEO_START( genesis )
{
	if (video_start_generic() != 0)
		return 1;

	/* create scrollA and scrollB playfields */

	/*
	if ((scroll_a = auto_bitmap_alloc(1024,1024)) == 0)
		return 1;

	if ((scroll_b = auto_bitmap_alloc(1024,1024)) == 0)
		bitmap_free(scroll_a);
	*/

	if ((spritelayer = auto_bitmap_alloc(2500,2500)) == 0)
		return 1;

	if ((bitmap2 = auto_bitmap_alloc(320,224)) == 0)
		return 1;

	/*
	if ((bitmap_vram = auto_bitmap_alloc(8,18000)) == 0)
		return 1;
	*/

	/*
	if ((bitmap_sprite = auto_bitmap_alloc(64,64)) == 0)
		return 1;
	*/

	if ((tile_changed_1 = auto_malloc(0x1000)) == 0)
		return 1;

	if ((tile_changed_2 = auto_malloc(0x1000)) == 0)
		return 1;


	/* mark all layers and colours as pure filth */

	memset(dirty_attribute_a, -1, (128*128)*sizeof(short));
	memset(dirty_attribute_b, -1, (128*128)*sizeof(short));


	memset(tile_changed_1, 1, 0x1000);
	memset(tile_changed_2, 1, 0x1000);

	memset(dirty_colour, 1, 64);

	/* clear the VRAM too */

	memset(vdp_vram, 0, 0x10000);


	/* some standard startup values */

	//	scroll_a->width = scroll_b->width = 512;
	//	scroll_a->height = scroll_b->height =	512;


	vdp_ctrl_status			=	2;
	vdp_h_interrupt 		=	0;
	hv_counter_stopped 		=	0;
	vdp_display_enable  	=	0;
	vdp_v_interrupt			=	0;
	vdp_dma_enable			=	0;
	vdp_vram_fill			=	0;
	vdp_30cell_enable		=	1;
	vdp_display_height 		=	224;
	vdp_fifo_full			=	0;
	vdp_hv_status			=	0;
	vdp_pattern_scroll_a	=	0;
	vdp_pattern_window		=	0;
	vdp_pattern_scroll_b	=	0;
	vdp_pattern_sprite		=	0;
	vdp_background_colour 	=	0;
	vdp_background_palette	=	0;
	vdp_h_interrupt_reg		=	0;
	vdp_scrollmode			=	0;
	vdp_screenmode			=	0;
	vdp_h_scroll_addr		=	0;
	vdp_auto_increment		=	0;
	vdp_h_scrollsize		=	0;
	vdp_interlace			=	0;
	vdp_h_width				=	256;
	vdp_v_scrollsize		=	0;
	vdp_h_scrollmode		=	0;
	vdp_v_scrollmode		=	0;
	vdp_scroll_height		=	224;
	vdp_window_h			=	0;
	vdp_window_v			=	0;
	vdp_dma_length			=	0;
	vdp_dma_source			=	0;
	vdp_dma_busy			=	0;
	vdp_pal_mode			=	NTSC;
	numberofsprites			=	0;



	return 0;
}


static unsigned char *get_dma_dest_address(int id)
{
	switch (id)
	{
		case MODE_VRAM_READ:
		case MODE_VRAM_WRITE:
		case MODE_VRAM_WRITE_DMA:
			return &vdp_vram[0];
			break;
		case MODE_CRAM_READ:
		case MODE_CRAM_WRITE:
		case MODE_CRAM_WRITE_DMA:
			return (unsigned char *)&vdp_cram[0];
			break;
		case MODE_VSRAM_READ:
		case MODE_VSRAM_WRITE:
		case MODE_VSRAM_WRITE_DMA:
			return (unsigned char *)&vdp_vsram[0];
			break;
		default:
			logerror("Unknown get_dma_dest_address id %x!!\n", id);
	}
	return NULL;
}

READ16_HANDLER ( genesis_vdp_data_r )
{
	int data = 0;		 /* don't worry about this for now, really doesn't happen */

   	logerror("reading from... %x\n", (((vdp_address & 0xfffffffe)+ (int)&vdp_vram[0])) );

	switch (vdp_id)
	{
		case MODE_VRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_vram) ));
			break;
		case MODE_VSRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_vsram)));
			break;
		case MODE_CRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_cram) ));
			break;
		default:
			logerror("unknown vdp port read type %x\n", vdp_id);
	}

   /*	if ((offset == .5) || (offset == 1.5))	  */
		vdp_address += vdp_auto_increment;

	return data;
}

WRITE16_HANDLER ( genesis_vdp_data_w )
{
  	int tempsource = 0;
  	int temp_vdp_address = vdp_address;
	/*logerror("vdp data w offset = %x\n", offset);*/

	/* need a special case for byte writes...? */

	if (vdp_dma_enable && (vdp_id & 0x20))
	{
		if (offset == 0 || offset == 1)
		{

			COMBINE_DATA(&vdp_vram_fill);
			temp_vdp_address = vdp_address;
//			logerror("DMA VRAM FILL, dest %x, fill %x, length %x, real dest %x, id %x, inc %x\n", vdp_address, vdp_vram_fill, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
			/* now do the rest of the DMA fill */

			for (tempsource = 0; tempsource < (vdp_dma_length*2); tempsource++)
			{
				if ( ((tempsource & 1) == 0)  )
				{
					*(get_dma_dest_address(vdp_id)+temp_vdp_address) = (vdp_vram_fill >> 8) & 0xff;
				}
				else
				{
					*(get_dma_dest_address(vdp_id)+temp_vdp_address + 1) = vdp_vram_fill & 0xff;
					temp_vdp_address = (temp_vdp_address+vdp_auto_increment) & 0xffff;
				}
			}

			if (vdp_id == MODE_CRAM_WRITE_DMA)
			{
				 logerror("*** %x-%x\n", temp_vdp_address, vdp_dma_length*2);
				memset(dirty_colour + (temp_vdp_address), 1, (vdp_dma_length*2));
			}


			/* now do the VRAM-converted version */
			if (vdp_id == MODE_VRAM_WRITE_DMA)
			{
				for (tempsource = 0; tempsource < (vdp_dma_length*2); tempsource++)
				{
					int sx, sy;

					sy = (temp_vdp_address << 1) >> 3;
					sx = (temp_vdp_address << 1) & 7;

					if ( ((tempsource & 1) == 0)  )
					{

					      	//bitmap_vram->line[sy][sx]     = (vdp_vram_fill >> 12) & 0x0f;
						//bitmap_vram->line[sy][sx + 1] = (vdp_vram_fill >>  8) & 0x0f;
					  	tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;
					}
					else
					{
						//bitmap_vram->line[sy][sx]     = (vdp_vram_fill >> 4) & 0x0f;
						//bitmap_vram->line[sy][sx + 1] = (vdp_vram_fill     ) & 0x0f;
					  	tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;


						temp_vdp_address = (temp_vdp_address+vdp_auto_increment) & 0xffff;
					}
				}
 			}

		}
		return;
	}
	  /*	logerror("%x",vdp_vram_fill);*/

	/*  if (first_access && (offset == .0 || offset == 1.5))
		logerror("misaligned\n"); */
		 /*  	logerror("would write %x to... %x\n", data, ((vdp_address+ 0*//*(int)&vdp_vram[0]*//*) +(  (offset & 0.5))) );*/
	if ((vdp_address & 1)) logerror("!");

	switch (vdp_id)
	{
		case MODE_VRAM_WRITE:
			{
			int sx, sy;

			sy = ((vdp_address/*+(offset & 0.5)*/)<<1) >> 3;
			sx = ((vdp_address/*+(offset & 0.5)*/)<<1) & 7;
		  	COMBINE_DATA((UINT16*)(vdp_address+(int)vdp_vram));

	 		//bitmap_vram->line[sy][sx]     = (data >> 12) & 0x0f;
	 		//bitmap_vram->line[sy][sx + 1] = (data >>  8) & 0x0f;
			//bitmap_vram->line[sy][sx + 2] = (data >>  4) & 0x0f;
	 		//bitmap_vram->line[sy][sx + 3] = (data      ) & 0x0f;
/*	 		printf("SY:%d\n",sy); */
			if (sy < 16384) tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;

			}
			break;
		case MODE_VSRAM_WRITE:
			COMBINE_DATA((UINT16*)(vdp_address+(int)vdp_vsram));
			break;

		case MODE_CRAM_WRITE:
			COMBINE_DATA((UINT16*)(vdp_address+(int)vdp_cram));
			dirty_colour[vdp_address>>1] = 1;
			//logerror("%x\n", vdp_address);
			break;

		default:
			logerror("unknown vdp port write type %x\n", vdp_id);
	}

   //	if ((offset == .5 || offset == 1.5) /*|| (data_width == 1)*/)
		vdp_address += vdp_auto_increment;

	if (vdp_auto_increment == 1) logerror("1");
/*		first_access=0;*/
}



READ16_HANDLER ( genesis_vdp_ctrl_r )
{
static int  fake_dma_mode = 0;
fake_dma_mode ^=8;
	vdp_ctrl_status = ((vdp_fifo_full ^ 1) << 9)
					+ ((vdp_fifo_full    ) << 8)
					+/*(vdp_dma_enable ? 0 : 0)*/fake_dma_mode
					+ (vdp_pal_mode);
				   	vdp_ctrl_status=rand() & 8;
				 //	cpu_cause_interrupt(0,6);

				   //	cpu_halt(0,0);
				//	logerror("vdp ctrl status offset = %x\n", offset);
	return vdp_ctrl_status;
}

WRITE16_HANDLER ( genesis_vdp_ctrl_w )
{
	static int first_read = 1;
	static int vdp_register, full_vdp_data;
	int vdp_data = COMBINE_DATA(&full_vdp_data);
	//cpu_yield();
	switch (offset)
	{
		case 0:
	  	case 1:
	  	  	 /* logerror("genesis_vdp_ctrl_w %x, %x, %x, %x\n", offset, data, vdp_data, data >>16);*/



			if ((vdp_data & 0xe000) == 0x8000) /* general register write	*/
			{
				first_read = 1;
				vdp_register = (vdp_data >> 8) & 0x1f;
			  	vdp_data = full_vdp_data & 0xff;
				/*logerror("register %d writing %x\n", vdp_register, vdp_data);*/
				switch (vdp_register)
				{
					case 0:	/* register 0, interrupt enable etc */
						vdp_h_interrupt = (vdp_data & 0x10);
						hv_counter_stopped = (vdp_data & 0x02);
						break;
					case 1: /* more misc flags */
						vdp_display_enable  = (vdp_data & 0x40);
						vdp_v_interrupt		= (vdp_data & 0x20);
						vdp_dma_enable		= (vdp_data & 0x10);
						vdp_30cell_enable	= (vdp_data & 0x08);
						vdp_display_height  = vdp_30cell_enable ? 240 : 224;

						break;
					case 2:	/* pattern name table address, scroll A */
						vdp_pattern_scroll_a= (char *)(&vdp_vram[0]+(vdp_data << 10));
//						logerror("scrolla = %x\n", vdp_pattern_scroll_a-(int)&vdp_vram[0]);
//						logerror("VRAM is %x\n", &vdp_vram[0]);
					   //	memset(dirty_attribute_a, -1, (128*128)*sizeof(short));
						break;
					case 3:	/* pattern name table address, window */
						vdp_pattern_window	= (char *)(&vdp_vram[0]+(vdp_data <<10));
//						logerror("window = %x\n", vdp_pattern_window-(int)&vdp_vram[0]);

						break;
					case 4:	/* pattern name table address, scroll B */
						vdp_pattern_scroll_b= (char *)(&vdp_vram[0]+(vdp_data <<13));
					  //	memset(dirty_attribute_b, -1, (128*128)*sizeof(short));

//						logerror("scrollb = %x\n", vdp_pattern_scroll_b-(int)&vdp_vram[0]);
						break;
					case 5:	/* pattern name table address, sprite */
						vdp_pattern_sprite	= (unsigned char *)(&vdp_vram[0]+(vdp_data <<9));
//						logerror("sprite = %x\n", vdp_pattern_sprite-(int)&vdp_vram[0]);
						break;
					case 6: /* nothing */
						break;
					case 7:	/*set background colour */
							vdp_background_colour = (vdp_data & 0xff); /* maps directly to palette */
							dirty_colour[0] = 1;
						break;
					case 8: case 9: /* nothing */
						break;
					case 10: /* horizontal interrupt register */
						vdp_h_interrupt_reg	= vdp_data;
						break;
					case 11: /* mode set register 3 */
						vdp_scrollmode		= vdp_data;
						vdp_h_scrollmode	= (vdp_data & 3);
						vdp_v_scrollmode	= (vdp_data >> 2) & 1;
						logerror("scroll modes %x, %x\n", vdp_h_scrollmode, vdp_v_scrollmode);
						break;
					case 12: /* character mode, interlace etc */
						vdp_screenmode		= vdp_data;
						vdp_h_width = ((vdp_data & 1) ? 320 : 256);
						vdp_interlace = (vdp_data >> 1) & 3;
						logerror("screen width, interlace flag = %d, %d\n", vdp_h_width, vdp_interlace);
						break;
					case 13: /* H scroll data address */
						vdp_h_scroll_addr	= (char *)(&vdp_vram[0]+(vdp_data<<10));
						break;
					case 14: /* nothing */
						logerror("$c10001c register usage @ PC=%08x\n",(UINT32)activecpu_get_pc());
						break;
					case 15: /* autoincrement data */
						vdp_auto_increment	= vdp_data;
						break;
					case 16: /* scroll size */
						vdp_h_scrollsize	= (vdp_data & 3);
						vdp_v_scrollsize	= ((vdp_data >> 4) & 3);
						logerror("initial h scrollsize = %x\n", vdp_h_scrollsize);
						switch (vdp_h_scrollsize)
						{
							case 0: vdp_h_scrollsize = 32; break;
							case 1: vdp_h_scrollsize = 64; break;
							case 2: break;
							case 3: vdp_h_scrollsize = 128/*64*/; break;/* should be 128??? */
						}

						switch (vdp_v_scrollsize)
						{
							case 0: vdp_v_scrollsize = 32; break;
							case 1: vdp_v_scrollsize = 64; break;
							case 2: break;
							case 3: vdp_v_scrollsize = 128; break;
						}
						vdp_scroll_height = vdp_v_scrollsize << 3;
						logerror("scrollsizes are %d, %d\n", vdp_h_scrollsize, vdp_v_scrollsize);

					  //	scroll_a->width = scroll_b->width = vdp_h_scrollsize << 3;
					  //	scroll_a->height = scroll_b->height = vdp_scroll_height;

						break;
					case 17: /* window H position */
						vdp_window_h		= vdp_data;
						break;
					case 18: /* window V position */
						vdp_window_v		= vdp_data;
						break;
					case 19: /* DMA length counter low */
						vdp_dma_length		= (vdp_dma_length & 0xff00) | vdp_data;
						logerror("DMA length low.. length is %x\n", vdp_dma_length);
						break;
					case 20: /* DMA length counter high */
						vdp_dma_length		= (vdp_dma_length & 0xff) | (vdp_data << 8);
						logerror("DMA length high.. length is %x\n", vdp_dma_length);
						break;
					case 21: /* DMA source low  (total is SA1-SA22, thus the extra shift */
						vdp_dma_source		= (vdp_dma_source & 0x7ffe00) | (vdp_data << 1);
						break;
					case 22: /* DMA source mid */
						vdp_dma_source		= (vdp_dma_source & 0x7e01fe) | (vdp_data << 9);
						break;
					case 23: /* DMA source high */
						vdp_dma_source		= (vdp_dma_source & 0x01fffe) | ((vdp_data & 0x7f) << 17) ;

						vdp_dma_mode		= (vdp_data >> 6) & 0x03;
						logerror("23:%x, %x\n",vdp_dma_mode, vdp_dma_source);
						break;
				}
			}
			else
			{
				if (first_read)
				{
					/* logerror("vdp data on first read is %x\n", vdp_data); */
					vdp_address = (vdp_data & 0x3fff);
					vdp_id		= (vdp_data & 0xc000) >> 14;
					first_read	= 0;
				}
				else
				{
				   /*	 logerror("vdp data on second read is %x\n", vdp_data); */
					vdp_address |= ((vdp_data & 0x03) << 14);
					vdp_id |= ( (vdp_data & 0xf0) >> 2);
					first_read	= 1;
					logerror("vdp id is %x\n", vdp_id);

					logerror("address set, %x\n", vdp_address);

					if (vdp_dma_enable && (vdp_id & 0x20))
					{

						/* DMA! */
						switch (vdp_dma_mode)
						{
/*#if 0*/
							case DMA_ROM_VRAM:
//								logerror("DMA ROM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

							  	genesis_initialise_dma(&memory_region(REGION_CPU1)[vdp_dma_source & 0x3fffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

/*#endif*/
								break;
							case DMA_RAM_VRAM:
//								logerror("DMA RAM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

								/*	if (vdp_address+(vdp_dma_length*vdp_auto_increment) > 0xffff) printf("error! sdil: %x %x %x %x\n", vdp_dma_source, vdp_address, vdp_auto_increment, vdp_dma_length);*/

							  	genesis_initialise_dma(&genesis_sharedram[vdp_dma_source & 0xffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

								break;
							case DMA_VRAM_FILL: /* handled at other port :-) */
								logerror("VRAM FILL pending, awaiting fill data set\n");
								break;
							case DMA_VRAM_COPY:
//								logerror("DMA VRAM COPY, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
								/*	tempdest = vdp_vram+vdp_address;
								for (tempsource = 0; tempsource < (vdp_dma_length); tempsource++)
								{
									 *tempdest = *(char *)(vdp_vram+(vdp_dma_source)+tempsource);
									 tempdest += vdp_auto_increment;
								}
								*/
								break;
							default:
								logerror("unknown vdp dma mode type %x\n", vdp_dma_mode);
								break;
						}
					}
				}
		}
	}
}

READ16_HANDLER ( genesis_vdp_hv_r )
{
	return vdp_hv_status;
}

WRITE16_HANDLER ( genesis_vdp_hv_w )
{
}

void genesis_dma_poll (int amount)
{
	int indx = 0, counter = 0;
//	char *old_current_dma_dest = current_dma_dest;

	if (vdp_dma_busy)
	{
	//cpu_yield();
		logerror("poll: src %p, end %p, id %x, vram dest %x, inc %x\n", current_dma_src, current_dma_end, current_dma_id, current_dma_vram_dest, current_dma_increment);

		while (indx < amount && current_dma_src < current_dma_end)
		{
			#ifdef PLSB_FIRST
			*(current_dma_dest+1)	= *(current_dma_src++);
			*(current_dma_dest)		= *(current_dma_src++);
			#else
			*(current_dma_dest)		= *(current_dma_src++);
			*(current_dma_dest+1)	= *(current_dma_src++);
			#endif

			current_dma_dest += current_dma_increment;

		   	counter++;
			indx +=2;
		}

		if (current_dma_src >= current_dma_end)
			vdp_dma_busy = 0;


		if (vdp_id == MODE_CRAM_WRITE_DMA)
			{
				logerror("%x-%x\n", current_dma_vram_dest, counter);
				memset(dirty_colour + (current_dma_vram_dest), 1, counter);
			}


		/* Now generate a modified video image of the VRAM data */

	  //	#if 0
	  	if (current_dma_id == MODE_VRAM_WRITE_DMA)
		{
		  	int offset = 0;

			while (counter--)
			{
				int sx, sy;

				sy = ((current_dma_vram_dest + offset)) >> 3;
				sx = ((current_dma_vram_dest + offset)) & 7;

				/* have to organise endianness here as we're dealing with screen data */

				#ifdef LSB_FIRST

				//bitmap_vram->line[sy][sx + 2] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 3] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 0] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 1] = (*old_current_dma_dest++) & 0x0f;
				//bitmap_vram->line[sy][sx + 6] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 7] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 4] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 5] = (*old_current_dma_dest++) & 0x0f;

				#else

				//bitmap_vram->line[sy][sx + 0] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 1] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 2] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 3] = (*old_current_dma_dest++) & 0x0f;
				//bitmap_vram->line[sy][sx + 4] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 5] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 6] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 7] = (*old_current_dma_dest++) & 0x0f;

				#endif

			 	if ((sy >> 3) < 0xfff) tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;



				offset += 4;
#if 0
				*ptr++		= ((*old_current_dma_src) >> 4) & 0x0f;
				*ptr++		= (*old_current_dma_src++) & 0x0f;

				*ptr++		= ((*old_current_dma_src) >> 4) & 0x0f;
				*ptr++		= (*old_current_dma_src++) & 0x0f;
#endif
				current_dma_vram_dest += (current_dma_increment << 1);

			}
		}
	//	#endif

	}
}


void genesis_initialise_dma (unsigned char *src, int dest, int length, int id, int increment)
{
	current_dma_src = src;
	current_dma_dest = get_dma_dest_address (vdp_id)+dest;
	current_dma_end = current_dma_src+length;
	current_dma_increment = increment;
	current_dma_id = id;
	vdp_dma_busy = 1;
	current_dma_vram_dest = dest << 1;
	genesis_dma_poll(165535);

}


/* defines for compares to the numbers used in the priority table */
#define SPRITE			0x3
#define SCROLL_A		0x2
#define SCROLL_B		0x1
#define BACKGROUND		0x4

const unsigned int priorities[8] =
/* our encoded priority values, based on defines above, lowest nibble is highest priority */
{
	0x041230,
	0x041230,
	0x042310,
	0x042130,
	0x041320,
	0x041230,
	0x043120,
	0x041230
};


/* this function is subject to serious change! */

static void combinelayers(struct mame_bitmap *dest, int startline, int endline)
{
	int x;
	unsigned char y;
	//unsigned char block_a_y, block_b_y;

	short *h_scroll = (short *)vdp_h_scroll_addr;
//	int transparent = Machine->pens[0] * 0x01010101;
	int v_mask = (vdp_v_scrollsize << 3)-1;
	int h_mask = (vdp_h_scrollsize << 3)-1;
	unsigned int priority = 0;
	unsigned char a_shift, b_shift;
	unsigned char *shift;
	int indx, increment;
	unsigned short *sprite_ptr, *output_ptr;
	unsigned short *scroll_a_attribute_linebase;
	unsigned short *scroll_b_attribute_linebase;
//	unsigned char *scroll_a_pixel_addr = 0, *scroll_b_pixel_addr = 0;
	unsigned char *scroll_pixel_addr = 0;
	unsigned char sprite_pixel = 0;
	int scroll_a_attribute=0, scroll_b_attribute=0;
	int scroll_a_x = 0, scroll_a_y = 0, scroll_b_x = 0, scroll_b_y = 0;
	int scroll_x, scroll_y;
	int attribute;
	unsigned char output = 0;
  //	int old_a_attribute=0, old_b_attribute=0;
	//unsigned int oldpriority = 0;


	/* the 'sram' scroll RAM areas contain interleaved scroll offset values for layers A & B */

	/* initial Y scroll values */

	scroll_a_y =  (vdp_vsram[0] ) & v_mask;
	scroll_b_y =  (vdp_vsram[1] ) & v_mask;

	/* here we check to see if vertical scrolling is being used. If not, we can work
	in sections of an entire line length, otherwise, just the length between a vertical
	scroll offset change (16 bytes) */

	if (vdp_v_scrollmode == 0)
		increment = vdp_h_width/*+1*/;
	else
		increment = 16;

	/* the number of lines we're going to render */
  	for (y = startline; y < endline-1; y++)
	{
	 	/*our 'chunks' of the rendered scanline, either 1 *whole* scanline or multiple chunks of 16 bytes */

		for (x = 0; x < vdp_h_width; x+=increment)
		{

			/* calculate the positions within the layers, corresponding to the
			position of the display raster position.
			These are the positions of the source pixels */


		   	/* y values will only change when (x & 0x0f) == 0, IE each iteration of this loop	*/

				switch (vdp_v_scrollmode)
				{
					case 0: /* vertically scroll as a whole */
					scroll_a_y =  vdp_vsram[0];
					scroll_b_y =  vdp_vsram[1];
					break;

					case 1: /* vertically scroll in 2 cell units */
				  	scroll_a_y =  vdp_vsram[ (x>>3)&0xfffe   ];
				  	scroll_b_y =  vdp_vsram[((x>>3)&0xfffe)+1];
					break;
				}

				scroll_a_y = (scroll_a_y+y) & v_mask;
				scroll_b_y = (scroll_b_y+y) & v_mask;

				switch (vdp_h_scrollmode)
				{
					case 0: /* horizontally scroll as a whole */
					scroll_a_x =  (-h_scroll[         0]);
					scroll_b_x =  (-h_scroll[         1]);
					break;
					case 1: /* invalid situation, just drop through I guess  */
					case 2: /* every 8 lines scroll separately */
					scroll_a_x =  (-h_scroll[( y<<1)&0xfff0   ]);
					scroll_b_x =  (-h_scroll[((y<<1)&0xfff0)+1]);
					break;
					case 3: /* every line scrolls separately */
					scroll_a_x =  -h_scroll[ y << 1   ];
					scroll_b_x =  -h_scroll[(y << 1)+1];
					break;
				}

				scroll_a_x = (scroll_a_x+x) & h_mask;
				scroll_b_x = (scroll_b_x+x) & h_mask;

			  /* scroll_a_x and y, and scroll_b_x and y now point to the position in the tile map
			  for layers A & B, for the given set of pixels on the screen we're about to render */
		  	  /* the sprite layer position is about to be set too */

   			// sprite_ptr = &spritelayer->line[y+128][x+128];
			sprite_ptr = &((UINT16**)spritelayer->line)[y+128][x+128];
			output_ptr = &((UINT16**)dest->line)[y][x];
			/* base + (y/8) * (number of width attributes * size of attributes, which is 2 bytes) */
		   	scroll_a_attribute_linebase= (unsigned short *)(vdp_pattern_scroll_a+((scroll_a_y>>3)*(vdp_h_scrollsize<<1)));
		   	scroll_b_attribute_linebase= (unsigned short *)(vdp_pattern_scroll_b+((scroll_b_y>>3)*(vdp_h_scrollsize<<1)));
   		  //	block_a_y = (scroll_a_y & 7) << 2;
		  //	block_b_y = (scroll_b_y & 7)  << 2;


			for (indx = 0; indx < increment; indx++)
			{

				/* read all the source pixels & priorities from their respective layers */

				/* attributes derived from the word along the attribute base for the Y line + X offset of the scroll layer wrapped around and divided by 8 */


				/* the shits are used to calculate whether we're playing with the upper of lower nibble of the
				source byte for our pixel (we're dealing with 4bpp data and outputting to 8bpp or more */
		  	  	a_shift = (scroll_a_x & 1) ? 0 : 4;
		  		b_shift = (scroll_b_x & 1) ? 0 : 4;

	   //			old_a_attribute=scroll_a_attribute;
	   //			old_b_attribute=scroll_b_attribute;

				/* here we read the attributes for the tile within which each layer's pixel-to-be-plotted
				resides - this is needed for colour banking, whether the tile is flipped, etc */

				/* this does not need to be done for the sprite layer currently, as I predecode the actual
				sprite colour and priority when rendering the sprites into the sprite layer */

				scroll_a_attribute = scroll_a_attribute_linebase[scroll_a_x >>3];
				scroll_b_attribute = scroll_b_attribute_linebase[scroll_b_x >>3];

			   	sprite_pixel = *(sprite_ptr++);

				/* blank out the source sprite layer as we go for the next frame */
				*(sprite_ptr-1)=0;

				/* here we encode a priority mask up from the priorities of each layer. The sprite
				layer has had the sprite's priority bit encoded into it's top bit */

				/* the resulting 'number' is used as an index into the priority table to decide which
				pixel gets plotted */

				priority =	(((scroll_a_attribute &0x8000	)>> 13)
					|		(( scroll_b_attribute &0x8000	)>> 14)
 					|		(( sprite_pixel		  &0x80		)>>  7));

				priority = priorities[priority];

				output = 0;

				/* here, the priority information in the table is used as a shift register to calculate
				which layer should have it's pixel read and decoded to see if it's opaque - if it is, this
				is the pixel that will end up on the screen, otherwise we drop through to the layer behind it */

				/* note that the address indirections only occur when it's necessary to indirect, all
				calculations act on an address which will be indirected only when it's absolutely necessary
				to obtain the pixel's value */

			   	while(!(output & 0x0f))
				{
					switch (priority & 0x0f)
					{
						case SPRITE:
							output = (sprite_pixel & 0x7f);
							break;

					 	case BACKGROUND:
							output = 1;//vdp_background_colour;
					 		break;

						case SCROLL_A:
							attribute = scroll_a_attribute;
							scroll_x = (scroll_a_x & 7) >> 1;
							scroll_y = (scroll_a_y & 7) << 2;
							shift = &a_shift;
							goto skip; /* I hate these, but it adds a small amount of speed here */

						case SCROLL_B:
							attribute = scroll_b_attribute;
							scroll_x = (scroll_b_x & 7) >> 1;
							scroll_y = (scroll_b_y & 7) << 2;
							shift = &b_shift;

							skip:

							/*if (tile_changed_1[attribute & 0xfff])
							{
							tile_changed_2[attribute & 0xfff]=0;
							*/
							scroll_pixel_addr = &vdp_vram[(attribute & 0x7ff)<<5];

						  	/* here we calculate the address the pixel should be read from within the
							tile, given the flip settings in the tile's attribute */

					  	 	switch (attribute & 0x1800)
					  	 	{
					  	 		case 0x00: /* no flips */

						   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS(scroll_y+scroll_x);
					  	 			break;
					  	 		case 0x800: /* X flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS(scroll_y+(3-scroll_x));
					  	 			*shift ^=4;
					  	 			break;
					  	 		case 0x1000: /* Y flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS((28-scroll_y)+scroll_x);
					  	 			break;

					  	 		case 0x1800: /* X & Y flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS((28-scroll_y)+(3-scroll_x));
					  	 			*shift ^=4;
					  	  			break;
					  	 	}

						 	output =  (*scroll_pixel_addr >> *shift ) & 0x0f;
						 	output |= ((attribute >> 9) & 0x30);
						 	//}
						 	break;


					}
			  //	if (priority ==oldpriority) break;
				  	priority >>= 4; /* as we carry on in life, our priorities tend to change */

				}

				/* this is a fall through. If all the layer's pixels were opaque,
				we use the 'background colour' instead */


			   if (!priority)
			   		output = vdp_background_colour;

				*(output_ptr++)=Machine->pens[output];

				//if (*output_ptr != output) *output_ptr = output;

				/* increment our X position within the layers accounting for wraparound */
			 	scroll_a_x = ((scroll_a_x + 1) & h_mask);
			 	scroll_b_x = ((scroll_b_x + 1) & h_mask);

				//sprite_ptr++;

			}

		}

	}

}


/* fonky tile plotter - ASMable fairly easily */

INLINE void genesis_plot_tile(struct mame_bitmap *dest, int tilenum, int attribute, int sx, int sy)
{
	/* Bugger! If only I could plot 4 pixels at a time... would it be faster on macs if I
	packed reads into a 32-bit word and wrote that? */

	unsigned char code = ((attribute >> 9) & 0x30) | ((attribute & 0x8000) >> 8);
	int line;
	unsigned short *bm;
	unsigned char *c;
	int flips = (attribute & 0x1800);
	#ifdef LSB_FIRST
	#define OF0 1
	#define OF1 0
	#define OF2 3
	#define OF3 2
	#else
	#define OF0 0
	#define OF1 1
	#define OF2 2
	#define OF3 3
	#endif
	switch (flips)
	{
		case 0x00: // no flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			bm = &((UINT16**)dest->line)[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];
		  		if (!bm[0]) bm[0]=colours2[(c[OF0]>>4) | code];
				if (!bm[1]) bm[1]=colours2[(c[OF0]&0xf) | code];
				if (!bm[2]) bm[2]=colours2[(c[OF1]>>4) | code];
				if (!bm[3]) bm[3]=colours2[(c[OF1]&0xf) | code];
				if (!bm[4]) bm[4]=colours2[(c[OF2]>>4) | code];
				if (!bm[5]) bm[5]=colours2[(c[OF2]&0xf) | code];
				if (!bm[6]) bm[6]=colours2[(c[OF3]>>4) | code];
				if (!bm[7]) bm[7]=colours2[(c[OF3]&0xf) | code];
				c+=4;
				/*
				bm[0]=colours2[c[0] | code];
				bm[1]=colours2[c[1] | code];
				bm[2]=colours2[c[2] | code];
				bm[3]=colours2[c[3] | code];
				bm[4]=colours2[c[4] | code];
				bm[5]=colours2[c[5] | code];
				bm[6]=colours2[c[6] | code];
				bm[7]=colours2[c[7] | code]; */

		}
		break;

		case 0x800: // X flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			bm = &((UINT16**)dest->line)[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];

		   		if (!bm[1]) bm[1]=colours2[(c[OF3]>>4) | code];
				if (!bm[0]) bm[0]=colours2[(c[OF3]&0xf) | code];
				if (!bm[3]) bm[3]=colours2[(c[OF2]>>4) | code];
				if (!bm[2]) bm[2]=colours2[(c[OF2]&0xf) | code];
				if (!bm[5]) bm[5]=colours2[(c[OF1]>>4) | code];
				if (!bm[4]) bm[4]=colours2[(c[OF1]&0xf) | code];
				if (!bm[7]) bm[7]=colours2[(c[OF0]>>4) | code];
				if (!bm[6]) bm[6]=colours2[(c[OF0]&0xf) | code];

				c+=4;
			  /*	bm[0]=colours2[c[7] | code];
				bm[1]=colours2[c[6] | code];
				bm[2]=colours2[c[5] | code];
				bm[3]=colours2[c[4] | code];
				bm[4]=colours2[c[3] | code];
				bm[5]=colours2[c[2] | code];
				bm[6]=colours2[c[1] | code];
				bm[7]=colours2[c[0] | code]; */

		}
		break;

		case 0x1000: // Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = &((UINT16**)dest->line)[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

				if (!bm[0]) bm[0]=colours2[(c[OF0]>>4) | code];
				if (!bm[1]) bm[1]=colours2[(c[OF0]&0xf) | code];
				if (!bm[2]) bm[2]=colours2[(c[OF1]>>4) | code];
				if (!bm[3]) bm[3]=colours2[(c[OF1]&0xf) | code];
				if (!bm[4]) bm[4]=colours2[(c[OF2]>>4) | code];
				if (!bm[5]) bm[5]=colours2[(c[OF2]&0xf) | code];
				if (!bm[6]) bm[6]=colours2[(c[OF3]>>4) | code];
				if (!bm[7]) bm[7]=colours2[(c[OF3]&0xf) | code];
				c-=4;

			/*	bm[0]=colours2[c[0] | code];
				bm[1]=colours2[c[1] | code];
				bm[2]=colours2[c[2] | code];
				bm[3]=colours2[c[3] | code];
				bm[4]=colours2[c[4] | code];
				bm[5]=colours2[c[5] | code];
				bm[6]=colours2[c[6] | code];
				bm[7]=colours2[c[7] | code]; */


		}
		break;

		case 0x1800: // X & Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = &((UINT16**)dest->line)[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

		   		if (!bm[1]) bm[1]=colours2[(c[OF3]>>4) | code];
				if (!bm[0]) bm[0]=colours2[(c[OF3]&0xf) | code];
				if (!bm[3]) bm[3]=colours2[(c[OF2]>>4) | code];
				if (!bm[2]) bm[2]=colours2[(c[OF2]&0xf) | code];
				if (!bm[5]) bm[5]=colours2[(c[OF1]>>4) | code];
				if (!bm[4]) bm[4]=colours2[(c[OF1]&0xf) | code];
				if (!bm[7]) bm[7]=colours2[(c[OF0]>>4) | code];
				if (!bm[6]) bm[6]=colours2[(c[OF0]&0xf) | code];

				c-=4;
			/*	bm[0]=colours2[c[7] | code];
				bm[1]=colours2[c[6] | code];
				bm[2]=colours2[c[5] | code];
				bm[3]=colours2[c[4] | code];
				bm[4]=colours2[c[3] | code];
				bm[5]=colours2[c[2] | code];
				bm[6]=colours2[c[1] | code];
				bm[7]=colours2[c[0] | code]; */

		}
		break;

	}
}

static void plot_sprites(int priority)
{
	#ifdef LSB_FIRST
	#define SIZE 3
	#define NEXT 2
	#else
	#define SIZE 2
	#define NEXT 3
	#endif

	unsigned char *current_sprite;

	current_sprite = vdp_pattern_sprite;
	while(1)
	{
		int indx, x, y, code, attribute, size, blocks_x, blocks_y;

		attribute = *(unsigned short *)(current_sprite+4);


		/* plot from top to bottom, starting with highest priority */



		if ((attribute & 0x8000) == priority)
		{
			code = attribute & 0x7ff;
			size = *(current_sprite+SIZE);
			blocks_x = ((size >> 2) & 3)+1;
			blocks_y = (size & 3)+1;
			//bitmap_sprite->width  =	blocks_x << 3;
			//bitmap_sprite->height = blocks_y << 3;


			y = (*(short *) current_sprite   )&0x3ff;/*-0x80*/;
			x = (*(short *)(current_sprite+6))&0x3ff;/*-0x80*/;

			/* plot unflipped to temporary bufferspace */
			switch (attribute & 0x1800)
			{
				case 0: /* no flips */
					for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}
				break;

				case 0x800: /* X flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}


				break;

				case 0x1000: /* Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;

				case 0x1800: /* X & Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;
			}


		}




	   /*	logerror("addr = %x, sprcount %x\n", current_sprite, numberofsprites);  */
		if (*(current_sprite + NEXT) == 0) break;
		current_sprite = (vdp_pattern_sprite + (*(current_sprite + NEXT)<<3) );

	}
//	while (*(current_sprite + NEXT) != 0);
}
 void genesis_modify_display(int);
/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( genesis )
{
	genesis_modify_display(0);
	copybitmap(bitmap, bitmap2, 0, 0, 0, 0, 0, 0, 0);
}

void genesis_modify_display(int inter)
{

//   	int offs;
   	int pom;
//	int sx,sy;

	void *temp_ptr;


	/* as we're dealing with VRAM memory, which was copied over using *(short *) accsses, the bytes
	will be mangled on littleendian systems to allow this. As a result, we need to take these into
	account when accessing this area byte-wise */


	struct rectangle visiblearea =
	{
		0, 0,
		0, 0
	};

  //	struct rectangle visiblearea =
  //	{
  //		128, 128,
  //		0, 0
  //	};


//	char vdp_h_mask = vdp_h_scrollsize-1;
   //cpu_halt(1,0);
   if (!vdp_v_interrupt || !vdp_display_enable) return;

 //  if (z80running)
 //  {
 //  		cpu_cause_interrupt(1,Z80_NMI_INT);
 //		logerror("Allowing Z80 interrupt\n");
 //  }

	/* Setup the non-constant fields */
	//scroll_element.gfxdata = bitmap_vram;
	visiblearea.max_x = (vdp_h_scrollsize<<3)-1;
	visiblearea.max_y = vdp_scroll_height-1;
	spritelayer->width = visiblearea.max_x + 256;
	spritelayer->height = visiblearea.max_y + 256;
	//scroll_element.colortable = &colours[0];

      /*	genesis_dma_poll(200); */
/*	if (!vdp_display_enable) return;*/


/* organise sprites by priority, into the sprite layer  */

   //	fillbitmap(spritelayer, 0, 0);
   if (inter == 0) {
  	plot_sprites(0x8000);
  	plot_sprites(0);

/* translate palette */

	for (pom = 1; pom < 64; pom++) /* don't set colour 0 here, as it's background */
	{

		if (dirty_colour[pom])
		{
			int colour = vdp_cram[pom];
			palette_set_color(Machine->pens[pom],((colour<<4) & 0xe0) DIM,((colour) & 0xe0) DIM,((colour>>4) & 0xe0) DIM);
			colours[pom] = (pom & 0x0f) ? Machine->pens[pom] : Machine->pens[0];
			dirty_colour[pom] = 0;
		}
	}

	if (dirty_colour[0])
	{
		int colour = vdp_cram[vdp_background_colour];
		palette_set_color(Machine->pens[0],((colour<<4) & 0xe0) DIM,((colour) & 0xe0) DIM,((colour>>4) & 0xe0) DIM);  /* quick background colour set */
		colours[0] = Machine->pens[0];
		dirty_colour[0] = 0;
	}

  #if 0
   /* copy the tiles into the scroll layer A and B buffers */
    sx = 0;
    sy = 0;

	for (pom = 0x0;pom < (vdp_h_scrollsize*vdp_v_scrollsize)<<1; pom+=2)
	{
		int num_a, num_b;
		int attribute_a = *(unsigned short *)(vdp_pattern_scroll_a+pom);
 		int attribute_b = *(unsigned short *)(vdp_pattern_scroll_b+pom);
		offs=pom >> 1;

		if (sx > vdp_h_mask<<3)
		{
			sy+=8;
			sx=0;
		}


		num_a = attribute_a & 0xfff;
		num_b = attribute_b & 0xfff;

		if ((dirty_attribute_a[offs] !=attribute_a) || tile_changed_1[num_a])
		{
		 //	sx = (offs & vdp_h_mask)<<3;
		 //	sy = (offs / vdp_h_scrollsize)<<3;
	   		genesis_plot_tile(scroll_a,
	   						num_a, attribute_a, sx, sy);

			dirty_attribute_a[offs] = attribute_a;
			tile_changed_2[num_a] = 0;
		 //	tile_changed_1[num_a] = 0;

		}

		if ((dirty_attribute_b[offs] !=attribute_b) || tile_changed_1[num_b])
		{

		 //	sx = (offs & vdp_h_mask)<<3;
		 //	sy = (offs / vdp_h_scrollsize)<<3;

		  	genesis_plot_tile(scroll_b,
		  					num_b, attribute_b, sx, sy);

			dirty_attribute_b[offs] = attribute_b;
			tile_changed_2[num_b] = 0;
		 //	tile_changed_1[num_b] = 0;

		}

	 sx+=8;
	}
   #endif

	/* swap the tile changed buffers around */
					  }
  	temp_ptr = tile_changed_1;
  	tile_changed_1 = tile_changed_2;
  	tile_changed_2 = temp_ptr;


	/* combine all the layers adding scroll, wraparound and distortion, into the output bitmap */

   //	combinelayers(bitmap,0,vdp_display_height);
	// combinelayers(bitmap2,inter,inter+2);
	combinelayers(bitmap2,0,vdp_display_height);

 /* mark all tiles as unchanged */
   //	memset(tile_changed_1, 0, 0x1000);
   //	memset(tile_changed_2, 0, 0x1000);


}





#else
#ifdef macintosh
#pragma mark ________Faster but Experimental
#endif
//-----------
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "machine/genesis.h"
#include "vidhrdw/genesis.h"

int z80running;

/* skeleton code included from closet.h */
#ifdef GARETHS_LITTLE_SECRET
#define DIM >>3
#else
#define DIM /*quack!*/
#endif

#define BLOCK_SHIFT				3

#define PAL 					1
#define NTSC 0
#define FIFO_EMPTY 0
#define FIFO_FULL 1
#define DMA_BUSY 1
#define DMA_FREE 0
#define MODE_VRAM_WRITE			1
#define MODE_CRAM_WRITE			3
#define MODE_VSRAM_WRITE		5
#define MODE_VRAM_READ			0
#define MODE_CRAM_READ			8
#define MODE_VSRAM_READ			4
#define MODE_VRAM_WRITE_DMA		0x21
#define MODE_CRAM_WRITE_DMA		0x23
#define MODE_VSRAM_WRITE_DMA	0x25
#define MODE_VRAM_READ_DMA		0x20
#define MODE_CRAM_READ_DMA		0x28
#define MODE_VSRAM_READ_DMA		0x24


#define DMA_ROM_VRAM			0
#define DMA_RAM_VRAM			1
#define DMA_VRAM_FILL			2
#define DMA_VRAM_COPY			3

char vdp_dma_mode 			=	DMA_FREE;
char vdp_id; /* above, same with two below */
char *vdp_data_ptr;
int vdp_address;
int vdp_ctrl_status			=	2;
int vdp_h_interrupt 		=	0;
int	hv_counter_stopped 		=	0;
int	vdp_display_enable  	=	0;
int	vdp_v_interrupt			=	0;
int	vdp_dma_enable			=	0;
int vdp_vram_fill			=	0;
int	vdp_30cell_enable		=	1;
int vdp_display_height 		=	224;
int	vdp_fifo_full			=	0;
int vdp_hv_status			=	0;
char *	vdp_pattern_scroll_a=	0;
char *	vdp_pattern_window	=	0;
char *	vdp_pattern_scroll_b=	0;
char *	vdp_pattern_sprite	=	0;
int	vdp_background_colour 	=	0;
int	vdp_background_palette	=	0;
int	vdp_h_interrupt_reg		=	0;
int	vdp_scrollmode			=	0;
int	vdp_screenmode			=	0;
char *	vdp_h_scroll_addr	=	0;
int	vdp_auto_increment		=	0;
int	vdp_h_scrollsize		=	0;
int vdp_interlace			=	0;
int	vdp_h_width				=	256;
int	vdp_v_scrollsize		=	0;
int	vdp_h_scrollmode		=	0;
int	vdp_v_scrollmode		=	0;
int	vdp_scroll_height		=	224;
int	vdp_window_h			=	0;
int	vdp_window_v			=	0;
int	vdp_dma_length			=	0;
int	vdp_dma_source			=	0;
int	vdp_dma_busy			=	0;
int vdp_pal_mode			=	NTSC;
int numberofsprites			=	0;

short distort_a_x[8]; /* tile distortion factors */
short distort_b_x[8]; /* tile distortion factors */

unsigned short vdp_cram[0x1000]; /* 64 x 9 bits rounded up */
unsigned char vdp_vram[0x10000]; /* 64K VRAM */
short vdp_vsram[0x1000]; /* 40 x 10 bits scroll RAM, rounded up */



unsigned char colours2[]={		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                        		0,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  								0,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
                        		0,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
                        		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                        		0,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  								0,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
                        		0,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
                        		0,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
                        		0,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
                        		0,161,162,163,163,165,166,167,168,169,170,171,172,173,174,175,
                        		0,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
                        		0,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
                        		0,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
                        		0,161,162,163,163,165,166,167,168,169,170,171,172,173,174,175,
                        		0,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
              			  };
unsigned char * current_dma_src = 0;
unsigned char * current_dma_dest = 0;
unsigned char * current_dma_end = 0;
int current_dma_vram_dest = 0;
int current_dma_increment = 0;
int current_dma_id = 0;

struct mame_bitmap *scroll_a;
struct mame_bitmap *scroll_b;
//struct mame_bitmap *bitmap_vram;
//struct mame_bitmap *bitmap_sprite;
struct mame_bitmap *bitmap2;

struct mame_bitmap *spritelayer;

unsigned short colours[256];

char dirty_colour[64];

short dirty_attribute_a[16384];
short dirty_attribute_b[16384];

char *tile_changed_1, *tile_changed_2;

struct GfxElement scroll_element =
	{
		8, 8,
		0,
		2048,
		16,
		0,
		64
	};

typedef struct
{
	struct mame_bitmap *bitmap;
	int x;
	int y;
	int attribute;
	int width;
	int height;
	int x_end;
	int y_end;
} GENESIS_SPRITE;




#ifdef USE_PALETTE_HERE
void genesis_vh_convert_color_prom (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}
}
#endif

void genesis_videoram1_w (int offset, int data)
{
	logerror("what is this doing? %x, %x\n", offset, data);
	offset = data;
}

int genesis_vh_start (void)
{
	if (video_start_generic() != 0)
		return 1;

	/* create scrollA and scrollB playfields */

  	if ((scroll_a = osd_create_bitmap(1024,1024)) == 0)
	{
		return 1;
	}
	if ((scroll_b = osd_create_bitmap(1024,1024)) == 0)
	{
		bitmap_free(scroll_a);
		return 1;
	}

   	if ((spritelayer = osd_create_bitmap(2500,2500)) == 0)
	{
   //		bitmap_free(scroll_a);
   //		bitmap_free(scroll_b);
		return 1;
	}
	if ((bitmap2 = osd_create_bitmap(320,224)) == 0)
	{
   //		bitmap_free(scroll_a);
   //		bitmap_free(scroll_b);
		return 1;
	}



   /*	if ((bitmap_vram = osd_create_bitmap(8,18000)) == 0)
	{
	//	bitmap_free(scroll_a);
	//	bitmap_free(scroll_b);
    	bitmap_free(spritelayer);


	   	return 1;
	}*/

   /*	if ((bitmap_sprite = osd_create_bitmap(64,64)) == 0)
	{
	//	bitmap_free(scroll_a);
	//	bitmap_free(scroll_b);
		bitmap_free(spritelayer);
	//   	bitmap_free(bitmap_vram);
		return 1;
	}*/


	if ((tile_changed_1 = auto_malloc(0x800)) == 0)
	{
	//	bitmap_free(scroll_a);
	//	bitmap_free(scroll_b);
		bitmap_free(spritelayer);
	//	bitmap_free(bitmap_vram);
	//	bitmap_free(bitmap_sprite);
		return 1;
	}

	if ((tile_changed_2 = auto_malloc(0x800)) == 0)
	{
	//	bitmap_free(scroll_a);
	//	bitmap_free(scroll_b);
		bitmap_free(spritelayer);
	//	bitmap_free(bitmap_vram);
	//	bitmap_free(bitmap_sprite);
		return 1;
	}


	/* mark all layers and colours as pure filth */

	memset(dirty_attribute_a, -1, (128*128)*sizeof(short));
	memset(dirty_attribute_b, -1, (128*128)*sizeof(short));


	memset(tile_changed_1, 1, 0x800);
	memset(tile_changed_2, 1, 0x800);

	memset(dirty_colour, 1, 64);

	/* clear the VRAM too */

	memset(vdp_vram, 0, 0x10000);


	/* some standard startup values */

  //	scroll_a->width = scroll_b->width = 512;
  //	scroll_a->height = scroll_b->height =	512;


	vdp_ctrl_status			=	2;
	vdp_h_interrupt 		=	0;
	hv_counter_stopped 		=	0;
	vdp_display_enable  	=	0;
	vdp_v_interrupt			=	0;
	vdp_dma_enable			=	0;
	vdp_vram_fill			=	0;
	vdp_30cell_enable		=	1;
	vdp_display_height 		=	224;
	vdp_fifo_full			=	0;
	vdp_hv_status			=	0;
	vdp_pattern_scroll_a	=	0;
	vdp_pattern_window		=	0;
	vdp_pattern_scroll_b	=	0;
	vdp_pattern_sprite		=	0;
	vdp_background_colour 	=	0;
	vdp_background_palette	=	0;
	vdp_h_interrupt_reg		=	0;
	vdp_scrollmode			=	0;
	vdp_screenmode			=	0;
	vdp_h_scroll_addr		=	0;
	vdp_auto_increment		=	0;
	vdp_h_scrollsize		=	0;
	vdp_interlace			=	0;
	vdp_h_width				=	256;
	vdp_v_scrollsize		=	0;
	vdp_h_scrollmode		=	0;
	vdp_v_scrollmode		=	0;
	vdp_scroll_height		=	224;
	vdp_window_h			=	0;
	vdp_window_v			=	0;
	vdp_dma_length			=	0;
	vdp_dma_source			=	0;
	vdp_dma_busy			=	0;
	vdp_pal_mode			=	NTSC;
	numberofsprites			=	0;



	return 0;
}


void genesis_vh_stop (void)
{
	/* Free everything */
 //	bitmap_free(scroll_a);
 //	bitmap_free(scroll_b);
	bitmap_free(spritelayer);
 //	bitmap_free(bitmap_vram);
 //	bitmap_free(bitmap_sprite);
}

unsigned char *get_dma_dest_address(int id)
{
	switch (id)
	{
		case MODE_VRAM_READ:
		case MODE_VRAM_WRITE:
		case MODE_VRAM_WRITE_DMA:
			return &vdp_vram[0];
			break;
		case MODE_CRAM_READ:
		case MODE_CRAM_WRITE:
		case MODE_CRAM_WRITE_DMA:
			return (unsigned char *)&vdp_cram[0];
			break;
		case MODE_VSRAM_READ:
		case MODE_VSRAM_WRITE:
		case MODE_VSRAM_WRITE_DMA:
			return (unsigned char *)&vdp_vsram[0];
			break;
		default:
			logerror("Unknown get_dma_dest_address id %x!!\n", id);
	}
	return NULL;
}

int genesis_vdp_data_r (int offset)
{
	int data = 0;		 /* don't worry about this for now, really doesn't happen */

   	logerror("reading from... %x\n", (((vdp_address & 0xfffffffe)+ (int)&vdp_vram[0])) );

	switch (vdp_id)
	{
		case MODE_VRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_vram) ));
			break;
		case MODE_VSRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_vsram)));
			break;
		case MODE_CRAM_READ:
			data = /*ENDIANISE*/(*(short *)(((vdp_address & 0xfffffffe)+ vdp_cram) ));
			break;
		default:
			logerror("unknown vdp port read type %x\n", vdp_id);
	}

   /*	if ((offset == 1) || (offset == 3))	  */
		vdp_address += vdp_auto_increment;

	return data;
}

void genesis_vdp_data_w (int offset, int data)
{
  	int tempsource = 0;
  	int temp_vdp_address = vdp_address;
	/*logerror("vdp data w offset = %x\n", offset);*/

	/* need a special case for byte writes...? */

	if (vdp_dma_enable && (vdp_id & 0x20))
	{
		if (offset == 0 || offset == 2)
		{

		 vdp_vram_fill = COMBINE_WORD(vdp_vram_fill, data);
			temp_vdp_address = vdp_address;
//			logerror("DMA VRAM FILL, dest %x, fill %x, length %x, real dest %x, id %x, inc %x\n", vdp_address, vdp_vram_fill, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
			/* now do the rest of the DMA fill */

			for (tempsource = 0; tempsource < (vdp_dma_length*2); tempsource++)
			{
				if ( ((tempsource & 1) == 0)  )
				{
					*(get_dma_dest_address(vdp_id)+temp_vdp_address) = (vdp_vram_fill >> 8) & 0xff;
				}
				else
				{
					*(get_dma_dest_address(vdp_id)+temp_vdp_address + 1) = vdp_vram_fill & 0xff;
					temp_vdp_address = (temp_vdp_address+vdp_auto_increment) & 0xffff;
				}
			}

			if (vdp_id == MODE_CRAM_WRITE_DMA)
			{
				 logerror("*** %x-%x\n", temp_vdp_address, vdp_dma_length*2);
				memset(dirty_colour + (temp_vdp_address), 1, (vdp_dma_length*2));
			}


			/* now do the VRAM-converted version */
			if (vdp_id == MODE_VRAM_WRITE_DMA)
			{
				for (tempsource = 0; tempsource < (vdp_dma_length*2); tempsource++)
				{
					int sx, sy;

					sy = (temp_vdp_address << 1) >> 3;
					sx = (temp_vdp_address << 1) & 7;

					if ( ((tempsource & 1) == 0)  )
					{

					      	//bitmap_vram->line[sy][sx]     = (vdp_vram_fill >> 12) & 0x0f;
						//bitmap_vram->line[sy][sx + 1] = (vdp_vram_fill >>  8) & 0x0f;
					  	tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;
					}
					else
					{
						//bitmap_vram->line[sy][sx]     = (vdp_vram_fill >> 4) & 0x0f;
						//bitmap_vram->line[sy][sx + 1] = (vdp_vram_fill     ) & 0x0f;
					  	tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;


						temp_vdp_address = (temp_vdp_address+vdp_auto_increment) & 0xffff;
					}
				}
 			}

		}
		return;
	}
	  /*	logerror("%x",vdp_vram_fill);*/

	/*  if (first_access && (offset == 1 || offset == 3))
		logerror("misaligned\n"); */
		 /*  	logerror("would write %x to... %x\n", data, ((vdp_address+ 0*//*(int)&vdp_vram[0]*//*) +(  (offset & 0x01))) );*/
	if ((vdp_address & 1)) logerror("!");

	switch (vdp_id)
	{
		case MODE_VRAM_WRITE:
			{
			int sx, sy;

			sy = ((vdp_address+(offset & 1))<<1) >> 3;
			sx = ((vdp_address+(offset & 1))<<1) & 7;
		  	COMBINE_WORD_MEM(vdp_address+(int)vdp_vram, data);

	 		//bitmap_vram->line[sy][sx]     = (data >> 12) & 0x0f;
	 		//bitmap_vram->line[sy][sx + 1] = (data >>  8) & 0x0f;
			//bitmap_vram->line[sy][sx + 2] = (data >>  4) & 0x0f;
	 		//bitmap_vram->line[sy][sx + 3] = (data      ) & 0x0f;
			tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;

			}
			break;
		case MODE_VSRAM_WRITE:
			COMBINE_WORD_MEM(vdp_address+(int)vdp_vsram,data);
			break;

		case MODE_CRAM_WRITE:
			COMBINE_WORD_MEM(vdp_address+(int)vdp_cram, data);
			dirty_colour[vdp_address>>1] = 1;
			//logerror("%x\n", vdp_address);
			break;

		default:
			logerror("unknown vdp port write type %x\n", vdp_id);
	}

   //	if ((offset == 1 || offset == 3) /*|| (data_width == 1)*/)
		vdp_address += vdp_auto_increment;

	if (vdp_auto_increment == 1) logerror("1");
/*		first_access=0;*/
}



int genesis_vdp_ctrl_r (int offset)
{
static int  fake_dma_mode = 0;
fake_dma_mode ^=8;
	vdp_ctrl_status = ((vdp_fifo_full ^ 1) << 9)
					+ ((vdp_fifo_full    ) << 8)
					+/*(vdp_dma_enable ? 0 : 0)*/fake_dma_mode
					+ (vdp_pal_mode);
				   	vdp_ctrl_status=rand() & 8;
				 //	cpu_cause_interrupt(0,6);

				   //	cpu_halt(0,0);
				//	logerror("vdp ctrl status offset = %x\n", offset);
	return vdp_ctrl_status;
}

void genesis_vdp_ctrl_w (int offset, int data)
{
	static int first_read = 1;
	static int vdp_register, full_vdp_data;
	int vdp_data = full_vdp_data = COMBINE_WORD(full_vdp_data, data);
	//cpu_yield();
	switch (offset)
	{
		case 0:
	  	case 2:
	  	  	 /* logerror("genesis_vdp_ctrl_w %x, %x, %x, %x\n", offset, data, vdp_data, data >>16);*/



			if ((vdp_data & 0xe000) == 0x8000) /* general register write	*/
			{
				first_read = 1;
				vdp_register = (vdp_data >> 8) & 0x1f;
			  	vdp_data = full_vdp_data & 0xff;
				/*logerror("register %d writing %x\n", vdp_register, vdp_data);*/
				switch (vdp_register)
				{
					case 0:	/* register 0, interrupt enable etc */
						vdp_h_interrupt = (vdp_data & 0x10);
						hv_counter_stopped = (vdp_data & 0x02);
						break;
					case 1: /* more misc flags */
						vdp_display_enable  = (vdp_data & 0x40);
						vdp_v_interrupt		= (vdp_data & 0x20);
						vdp_dma_enable		= (vdp_data & 0x10);
						vdp_30cell_enable	= (vdp_data & 0x08);
						vdp_display_height  = vdp_30cell_enable ? 240 : 224;

						break;
					case 2:	/* pattern name table address, scroll A */
						vdp_pattern_scroll_a= (char *)(&vdp_vram[0]+(vdp_data << 10));
//						logerror("scrolla = %x\n", vdp_pattern_scroll_a-(int)&vdp_vram[0]);
//						logerror("VRAM is %x\n", &vdp_vram[0]);
					   //	memset(dirty_attribute_a, -1, (128*128)*sizeof(short));
						break;
					case 3:	/* pattern name table address, window */
						vdp_pattern_window	= (char *)(&vdp_vram[0]+(vdp_data <<10));
//						logerror("window = %x\n", vdp_pattern_window-(int)&vdp_vram[0]);

						break;
					case 4:	/* pattern name table address, scroll B */
						vdp_pattern_scroll_b= (char *)(&vdp_vram[0]+(vdp_data <<13));
					  //	memset(dirty_attribute_b, -1, (128*128)*sizeof(short));

//						logerror("scrollb = %x\n", vdp_pattern_scroll_b-(int)&vdp_vram[0]);
						break;
					case 5:	/* pattern name table address, sprite */
						vdp_pattern_sprite	= (char *)(&vdp_vram[0]+(vdp_data <<9));
//						logerror("sprite = %x\n", vdp_pattern_sprite-(int)&vdp_vram[0]);
						break;
					case 6: /* nothing */
						break;
					case 7:	/*set background colour */
							vdp_background_colour = (vdp_data & 0xff); /* maps directly to palette */
							dirty_colour[0] = 1;
						break;
					case 8: case 9: /* nothing */
						break;
					case 10: /* horizontal interrupt register */
						vdp_h_interrupt_reg	= vdp_data;
						break;
					case 11: /* mode set register 3 */
						vdp_scrollmode		= vdp_data;
						vdp_h_scrollmode	= (vdp_data & 3);
						vdp_v_scrollmode	= (vdp_data >> 2) & 1;
						logerror("scroll modes %x, %x\n", vdp_h_scrollmode, vdp_v_scrollmode);
						break;
					case 12: /* character mode, interlace etc */
						vdp_screenmode		= vdp_data;
						vdp_h_width = ((vdp_data & 1) ? 320 : 256);
						vdp_interlace = (vdp_data >> 1) & 3;
						logerror("screen width, interlace flag = %d, %d\n", vdp_h_width, vdp_interlace);
						break;
					case 13: /* H scroll data address */
						vdp_h_scroll_addr	= (char *)(&vdp_vram[0]+(vdp_data<<10));
						break;
					case 14: /* nothing */
						break;
					case 15: /* autoincrement data */
						vdp_auto_increment	= vdp_data;
						break;
					case 16: /* scroll size */
						vdp_h_scrollsize	= (vdp_data & 3);
						vdp_v_scrollsize	= ((vdp_data >> 4) & 3);
						logerror("initial h scrollsize = %x\n", vdp_h_scrollsize);
						switch (vdp_h_scrollsize)
						{
							case 0: vdp_h_scrollsize = 32; break;
							case 1: vdp_h_scrollsize = 64; break;
							case 2: break;
							case 3: vdp_h_scrollsize = 128/*64*/; break;/* should be 128??? */
						}

						switch (vdp_v_scrollsize)
						{
							case 0: vdp_v_scrollsize = 32; break;
							case 1: vdp_v_scrollsize = 64; break;
							case 2: break;
							case 3: vdp_v_scrollsize = 128; break;
						}
						vdp_scroll_height = vdp_v_scrollsize << 3;
						logerror("scrollsizes are %d, %d\n", vdp_h_scrollsize, vdp_v_scrollsize);

					  //	scroll_a->width = scroll_b->width = vdp_h_scrollsize << 3;
					  //	scroll_a->height = scroll_b->height = vdp_scroll_height;

						break;
					case 17: /* window H position */
						vdp_window_h		= vdp_data;
						break;
					case 18: /* window V position */
						vdp_window_v		= vdp_data;
						break;
					case 19: /* DMA length counter low */
						vdp_dma_length		= (vdp_dma_length & 0xff00) | vdp_data;
						logerror("DMA length low.. length is %x\n", vdp_dma_length);
						break;
					case 20: /* DMA length counter high */
						vdp_dma_length		= (vdp_dma_length & 0xff) | (vdp_data << 8);
						logerror("DMA length high.. length is %x\n", vdp_dma_length);
						break;
					case 21: /* DMA source low  (total is SA1-SA22, thus the extra shift */
						vdp_dma_source		= (vdp_dma_source & 0x7ffe00) | (vdp_data << 1);
						break;
					case 22: /* DMA source mid */
						vdp_dma_source		= (vdp_dma_source & 0x7e01fe) | (vdp_data << 9);
						break;
					case 23: /* DMA source high */
						vdp_dma_source		= (vdp_dma_source & 0x01fffe) | ((vdp_data & 0x7f) << 17) ;

						vdp_dma_mode		= (vdp_data >> 6) & 0x03;
						logerror("23:%x, %x\n",vdp_dma_mode, vdp_dma_source);
						break;
				}
			}
			else
			{
				if (first_read)
				{
					/* logerror("vdp data on first read is %x\n", vdp_data); */
					vdp_address = (vdp_data & 0x3fff);
					vdp_id		= (vdp_data & 0xc000) >> 14;
					first_read	= 0;
				}
				else
				{
				   /*	 logerror("vdp data on second read is %x\n", vdp_data); */
					vdp_address |= ((vdp_data & 0x03) << 14);
					vdp_id |= ( (vdp_data & 0xf0) >> 2);
					first_read	= 1;
					logerror("vdp id is %x\n", vdp_id);

					logerror("address set, %x\n", vdp_address);

					if (vdp_dma_enable && (vdp_id & 0x20))
					{

						/* DMA! */
						switch (vdp_dma_mode)
						{
/*#if 0*/
							case DMA_ROM_VRAM:
//								logerror("DMA ROM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

							  	genesis_initialise_dma(&memory_region(REGION_CPU1)[vdp_dma_source & 0x3fffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

/*#endif*/
								break;
							case DMA_RAM_VRAM:
//								logerror("DMA RAM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

								/*	if (vdp_address+(vdp_dma_length*vdp_auto_increment) > 0xffff) printf("error! sdil: %x %x %x %x\n", vdp_dma_source, vdp_address, vdp_auto_increment, vdp_dma_length);*/

							  	genesis_initialise_dma(&genesis_sharedram[vdp_dma_source & 0xffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

								break;
							case DMA_VRAM_FILL: /* handled at other port :-) */
								logerror("VRAM FILL pending, awaiting fill data set\n");
								break;
							case DMA_VRAM_COPY:
//								logerror("DMA VRAM COPY, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
								/*	tempdest = vdp_vram+vdp_address;
								for (tempsource = 0; tempsource < (vdp_dma_length); tempsource++)
								{
									 *tempdest = *(char *)(vdp_vram+(vdp_dma_source)+tempsource);
									 tempdest += vdp_auto_increment;
								}
								*/
								break;
							default:
								logerror("unknown vdp dma mode type %x\n", vdp_dma_mode);
								break;
						}
					}
				}
		}
	}
}

int genesis_vdp_hv_r (int offset)
{
   			return vdp_hv_status;
}

void genesis_vdp_hv_w (int offset, int data)
{
}

void genesis_dma_poll (int amount)
{
	int indx = 0, counter = 0;
//	char *old_current_dma_dest = current_dma_dest;

	if (vdp_dma_busy)
	{
	//cpu_yield();
		logerror("poll: src %p, end %p, id %x, vram dest %x, inc %x\n", current_dma_src, current_dma_end, current_dma_id, current_dma_vram_dest, current_dma_increment);

		while (indx < amount && current_dma_src < current_dma_end)
		{
			#ifdef PLSB_FIRST
			*(current_dma_dest+1)	= *(current_dma_src++);
			*(current_dma_dest)		= *(current_dma_src++);
			#else
			*(current_dma_dest)		= *(current_dma_src++);
			*(current_dma_dest+1)	= *(current_dma_src++);
			#endif

			current_dma_dest += current_dma_increment;

		   	counter++;
			indx +=2;
		}

		if (current_dma_src >= current_dma_end)
			vdp_dma_busy = 0;


		if (vdp_id == MODE_CRAM_WRITE_DMA)
			{
				logerror("%x-%x\n", current_dma_vram_dest, counter);
				memset(dirty_colour + (current_dma_vram_dest), 1, counter);
			}


		/* Now generate a modified video image of the VRAM data */

	  //	#if 0
	  	if (current_dma_id == MODE_VRAM_WRITE_DMA)
		{
		  	int offset = 0;

			while (counter--)
			{
				int sx, sy;

				sy = ((current_dma_vram_dest + offset)) >> 3;
				sx = ((current_dma_vram_dest + offset)) & 7;

				/* have to organise endianness here as we're dealing with screen data */

				#ifdef LSB_FIRST

				//bitmap_vram->line[sy][sx + 2] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 3] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 0] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 1] = (*old_current_dma_dest++) & 0x0f;
				//bitmap_vram->line[sy][sx + 6] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 7] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 4] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 5] = (*old_current_dma_dest++) & 0x0f;

				#else

				//bitmap_vram->line[sy][sx + 0] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 1] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 2] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 3] = (*old_current_dma_dest++) & 0x0f;
				//bitmap_vram->line[sy][sx + 4] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 5] = (*old_current_dma_dest++) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 6] = ((*old_current_dma_dest) >> 4) & 0x0f;
	 	  		//bitmap_vram->line[sy][sx + 7] = (*old_current_dma_dest++) & 0x0f;

				#endif

			 	if ((sy >> 3) < 0x7ff) tile_changed_1[sy >> BLOCK_SHIFT] = tile_changed_2[sy >> BLOCK_SHIFT] = 1;


				offset += 4;
#if 0
				*ptr++		= ((*old_current_dma_src) >> 4) & 0x0f;
				*ptr++		= (*old_current_dma_src++) & 0x0f;

				*ptr++		= ((*old_current_dma_src) >> 4) & 0x0f;
				*ptr++		= (*old_current_dma_src++) & 0x0f;
#endif
				current_dma_vram_dest += (current_dma_increment << 1);

			}
		}
	//	#endif

	}
}


void genesis_initialise_dma (unsigned char *src, int dest, int length, int id, int increment)
{
	current_dma_src = src;
	current_dma_dest = get_dma_dest_address (vdp_id)+dest;
	current_dma_end = current_dma_src+length;
	current_dma_increment = increment;
	current_dma_id = id;
	vdp_dma_busy = 1;
	current_dma_vram_dest = dest << 1;
	genesis_dma_poll(165535);

}


/* defines for compares to the numbers used in the priority table */
#define SPRITE			0x3
#define SCROLL_A		0x2
#define SCROLL_B		0x1
#define BACKGROUND		0x4

const unsigned int priorities[8] =
/* our encoded priority values, based on defines above, lowest nibble is highest priority */
{
	0x041230,
	0x041230,
	0x042310,
	0x042130,
	0x041320,
	0x041230,
	0x043120,
	0x041230
};


/* this function is subject to serious change! */

void combinelayers(struct mame_bitmap *dest, int startline, int endline)
{
	int x;
	unsigned char y;
	//unsigned char block_a_y, block_b_y;

	short *h_scroll = (short *)vdp_h_scroll_addr;
//	int transparent = Machine->pens[0] * 0x01010101;
	int v_mask = (vdp_v_scrollsize << 3)-1;
	int h_mask = (vdp_h_scrollsize << 3)-1;
	unsigned int priority = 0;
	unsigned char a_shift, b_shift;
	unsigned char *shift;
	int indx, increment;
	unsigned char *sprite_ptr, *output_ptr;
	unsigned short *scroll_a_attribute_linebase;
	unsigned short *scroll_b_attribute_linebase;
   //	unsigned char *scroll_a_pixel_addr = 0, *scroll_b_pixel_addr = 0;
	unsigned char *scroll_pixel_addr = 0;
	unsigned char sprite_pixel = 0;
	int scroll_a_attribute=0, scroll_b_attribute=0;
	int scroll_a_x = 0, scroll_a_y = 0, scroll_b_x = 0, scroll_b_y = 0;
	int scroll_x, scroll_y;
	int attribute;
	unsigned char output = 0;
   //	int old_a_attribute=0, old_b_attribute=0;
	//unsigned int oldpriority = 0;


	/* the 'sram' scroll RAM areas contain interleaved scroll offset values for layers A & B */

	/* initial Y scroll values */

	scroll_a_y =  (vdp_vsram[0] ) & v_mask;
	scroll_b_y =  (vdp_vsram[1] ) & v_mask;

	/* here we check to see if vertical scrolling is being used. If not, we can work
	in sections of an entire line length, otherwise, just the length between a vertical
	scroll offset change (16 bytes) */

	if (vdp_v_scrollmode == 0)
		increment = vdp_h_width/*+1*/;
	else
		increment = 16;

	/* the number of lines we're going to render */
  	for (y = startline; y < endline-1; y++)
	{
	 	/*our 'chunks' of the rendered scanline, either 1 *whole* scanline or multiple chunks of 16 bytes */

		for (x = 0; x < vdp_h_width; x+=increment)
		{

			/* calculate the positions within the layers, corresponding to the
			position of the display raster position.
			These are the positions of the source pixels */


		   	/* y values will only change when (x & 0x0f) == 0, IE each iteration of this loop	*/

				switch (vdp_v_scrollmode)
				{
					case 0: /* vertically scroll as a whole */
					scroll_a_y =  vdp_vsram[0];
					scroll_b_y =  vdp_vsram[1];
					break;

					case 1: /* vertically scroll in 2 cell units */
				  	scroll_a_y =  vdp_vsram[ (x>>3)&0xfffe   ];
				  	scroll_b_y =  vdp_vsram[((x>>3)&0xfffe)+1];
					break;
				}

				scroll_a_y = (scroll_a_y+y) & v_mask;
				scroll_b_y = (scroll_b_y+y) & v_mask;

				switch (vdp_h_scrollmode)
				{
					case 0: /* horizontally scroll as a whole */
					scroll_a_x =  (-h_scroll[         0]);
					scroll_b_x =  (-h_scroll[         1]);
					break;
					case 1: /* invalid situation, just drop through I guess  */
					case 2: /* every 8 lines scroll separately */
					scroll_a_x =  (-h_scroll[( y<<1)&0xfff0   ]);
					scroll_b_x =  (-h_scroll[((y<<1)&0xfff0)+1]);
					break;
					case 3: /* every line scrolls separately */
					scroll_a_x =  -h_scroll[ y << 1   ];
					scroll_b_x =  -h_scroll[(y << 1)+1];
					break;
				}

				scroll_a_x = (scroll_a_x+x) & h_mask;
				scroll_b_x = (scroll_b_x+x) & h_mask;

			  /* scroll_a_x and y, and scroll_b_x and y now point to the position in the tile map
			  for layers A & B, for the given set of pixels on the screen we're about to render */
		  	  /* the sprite layer position is about to be set too */

   			sprite_ptr = &spritelayer->line[y+128][x+128];
			output_ptr = &dest->line[y][x];
			/* base + (y/8) * (number of width attributes * size of attributes, which is 2 bytes) */
		   	scroll_a_attribute_linebase= (unsigned short *)(vdp_pattern_scroll_a+((scroll_a_y>>3)*(vdp_h_scrollsize<<1)));
		   	scroll_b_attribute_linebase= (unsigned short *)(vdp_pattern_scroll_b+((scroll_b_y>>3)*(vdp_h_scrollsize<<1)));
   		  //	block_a_y = (scroll_a_y & 7) << 2;
		  //	block_b_y = (scroll_b_y & 7)  << 2;


			for (indx = 0; indx < increment; indx++)
			{

				/* read all the source pixels & priorities from their respective layers */

				/* attributes derived from the word along the attribute base for the Y line + X offset of the scroll layer wrapped around and divided by 8 */


				/* the shits are used to calculate whether we're playing with the upper of lower nibble of the
				source byte for our pixel (we're dealing with 4bpp data and outputting to 8bpp or more */
		  	  	a_shift = (scroll_a_x & 1) ? 0 : 4;
		  		b_shift = (scroll_b_x & 1) ? 0 : 4;


	   //			old_a_attribute=scroll_a_attribute;
	   //			old_b_attribute=scroll_b_attribute;

				/* here we read the attributes for the tile within which each layer's pixel-to-be-plotted
				resides - this is needed for colour banking, whether the tile is flipped, etc */

				/* this does not need to be done for the sprite layer currently, as I predecode the actual
				sprite colour and priority when rendering the sprites into the sprite layer */

				scroll_a_attribute = scroll_a_attribute_linebase[scroll_a_x >>3];
				scroll_b_attribute = scroll_b_attribute_linebase[scroll_b_x >>3];

			   	sprite_pixel = *(sprite_ptr++);

				/* blank out the source sprite layer as we go for the next frame */
				*(sprite_ptr-1)=0;

				/* here we encode a priority mask up from the priorities of each layer. The sprite
				layer has had the sprite's priority bit encoded into it's top bit */

				/* the resulting 'number' is used as an indx into the priority table to decide which
				pixel gets plotted */

				priority =	(((scroll_a_attribute &0x8000	)>> 13)
					|		(( scroll_b_attribute &0x8000	)>> 14)
 					|		(( sprite_pixel		  &0x80		)>>  7));

				priority = priorities[priority];

				output = 0;

				/* here, the priority information in the table is used as a shift register to calculate
				which layer should have it's pixel read and decoded to see if it's opaque - if it is, this
				is the pixel that will end up on the screen, otherwise we drop through to the layer behind it */

				/* note that the address indirections only occur when it's necessary to indirect, all
				calculations act on an address which will be indirected only when it's absolutely necessary
				to obtain the pixel's value */

			   	while(!(output & 0x0f))
				{
					switch (priority & 0x0f)
					{
						case SPRITE:
							output = (sprite_pixel & 0x7f);
							break;

					 	case BACKGROUND:
							output = 1;//vdp_background_colour;
					 		break;

						case SCROLL_A:
							attribute = scroll_a_attribute;
							scroll_x = (scroll_a_x & 7) >> 1;
							scroll_y = (scroll_a_y & 7) << 2;
							shift = &a_shift;
							goto skip; /* I hate these, but it adds a small amount of speed here */

						case SCROLL_B:
							attribute = scroll_b_attribute;
							scroll_x = (scroll_b_x & 7) >> 1;
							scroll_y = (scroll_b_y & 7) << 2;
							shift = &b_shift;

							skip:

							/*if (tile_changed_1[attribute & 0x7ff])
							{
							tile_changed_2[attribute & 0x7ff]=0;
							*/
							scroll_pixel_addr = &vdp_vram[(attribute & 0x7ff)<<5];

						  	/* here we calculate the address the pixel should be read from within the
							tile, given the flip settings in the tile's attribute */

					  	 	switch (attribute & 0x1800)
					  	 	{
					  	 		case 0x00: /* no flips */

						   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS(scroll_y+scroll_x);
					  	 			break;
					  	 		case 0x800: /* X flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS(scroll_y+(3-scroll_x));
					  	 			*shift ^=4;
					  	 			break;
					  	 		case 0x1000: /* Y flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS((28-scroll_y)+scroll_x);
					  	 			break;

					  	 		case 0x1800: /* X & Y flips */
					  	   			scroll_pixel_addr += ACTUAL_BYTE_ADDRESS((28-scroll_y)+(3-scroll_x));
					  	 			*shift ^=4;
					  	  			break;
					  	 	}

						 	output =  (*scroll_pixel_addr >> *shift ) & 0x0f;
						 	output |= ((attribute >> 9) & 0x30);
						 	//}
						 	break;


					}
			  //	if (priority ==oldpriority) break;
				  	priority >>= 4; /* as we carry on in life, our priorities tend to change */

				}

				/* this is a fall through. If all the layer's pixels were opaque,
				we use the 'background colour' instead */


			   if (!priority)
			   		output = vdp_background_colour;

				*(output_ptr++)=Machine->pens[output];

				//if (*output_ptr != output) *output_ptr = output;

				/* increment our X position within the layers accounting for wraparound */
			 	scroll_a_x = ((++scroll_a_x) & h_mask);
			 	scroll_b_x = ((++scroll_b_x) & h_mask);

				//sprite_ptr++;

			}

		}

	}

}

inline void genesis_plot_distorted_tile(struct mame_bitmap *dest, int tilenum, int attribute, int sx, int sy, short *distort);

void combinelayers2(struct mame_bitmap *dest, int startline, int endline)
{
	int pom, sx = 0, sy = 0, ay = 0, by = 0;//, offs;
	int line = 0;
	short *h_scroll = (short *)vdp_h_scroll_addr;
	unsigned short * attribute_a_address = (unsigned short *)(vdp_pattern_scroll_a);
	unsigned short * attribute_b_address = (unsigned short *)(vdp_pattern_scroll_b);
	int scroll_a_y=0, scroll_b_y=0;
	int h_mask = (vdp_h_scrollsize << 3)-1;
	int v_mask = (vdp_v_scrollsize << 3)-1;
   //	int screen_y=-1;


	short *scroll_base_a, *scroll_base_b;




  // fillbitmap(dest, colours[0], 0);
   copybitmap(dest, spritelayer, 0, 0, -128, -128, 0, 0, 0);



	for (pom = 0x0;pom < (vdp_h_scrollsize*vdp_v_scrollsize) << 1; pom+=2)
	{
		int num_a, num_b;
	  //	offs=pom >> 1;


		if (!(sx & 0x0f))
		{
			switch (vdp_v_scrollmode)
			{
				case 0: /* vertically scroll as a whole */
				scroll_a_y =  vdp_vsram[0];
				scroll_b_y =  vdp_vsram[1];
				break;

				case 1: /* vertically scroll in 2 cell units */
			  	scroll_a_y =  vdp_vsram[ ((sx/*&h_mask*/)>>3)&0xfffe   ];
			  	scroll_b_y =  vdp_vsram[(((sx/*&h_mask*/)>>3)&0xfffe)+1];
				break;


			}
			//logerror("%d, %d\n", scroll_a_y, scroll_b_y);

			scroll_a_y = (-scroll_a_y+(sy /*& v_mask*/)) & v_mask;
			scroll_b_y = (-scroll_b_y+(sy /*& v_mask*/)) & v_mask;
			//logerror("%d, %d\n", scroll_a_y, scroll_b_y);
			switch (vdp_h_scrollmode)
				{
					case 0: /* horizontally scroll as a whole */
					  	if (sx == 0)
						{
							distort_a_x[0] = distort_a_x[1] = distort_a_x[2] = distort_a_x[3] =
							distort_a_x[4] = distort_a_x[5] = distort_a_x[6] = distort_a_x[7] =

							(h_scroll[         0]);

							distort_b_x[0] = distort_b_x[1] = distort_b_x[2] = distort_b_x[3] =
							distort_b_x[4] = distort_b_x[5] = distort_b_x[6] = distort_b_x[7] =
							(h_scroll[         1]);
						}
						break;

					case 1: /* invalid situation, just drop through I guess  */
					case 2: /* every 8 lines scroll separately */
					  	if (sx == 0)
						{
							distort_a_x[0] = distort_a_x[1] = distort_a_x[2] = distort_a_x[3] =
							distort_a_x[4] = distort_a_x[5] = distort_a_x[6] = distort_a_x[7] =
							(h_scroll[  ((scroll_a_y)<<1) & 0xfff0   ]);

							distort_b_x[0] = distort_b_x[1] = distort_b_x[2] = distort_b_x[3] =
							distort_b_x[4] = distort_b_x[5] = distort_b_x[6] = distort_b_x[7] =
						    (h_scroll[( ((scroll_b_y)<<1) & 0xfff0)+1]);
						}
						break;

					case 3: /* every line scrolls separately */
					  	if (sx == 0)
						{
							scroll_base_a = &h_scroll[ scroll_a_y << 1   ];
						   	scroll_base_b = &h_scroll[(scroll_b_y << 1)+1];

							distort_a_x[0] = scroll_base_a[0 ]; distort_b_x[0] = scroll_base_b[0 ];
							distort_a_x[1] = scroll_base_a[2 ]; distort_b_x[1] = scroll_base_b[2 ];
							distort_a_x[2] = scroll_base_a[4 ]; distort_b_x[2] = scroll_base_b[4 ];
							distort_a_x[3] = scroll_base_a[6 ]; distort_b_x[3] = scroll_base_b[6 ];
							distort_a_x[4] = scroll_base_a[8 ]; distort_b_x[4] = scroll_base_b[8 ];
							distort_a_x[5] = scroll_base_a[10]; distort_b_x[5] = scroll_base_b[10];
							distort_a_x[6] = scroll_base_a[12]; distort_b_x[6] = scroll_base_b[12];
							distort_a_x[7] = scroll_base_a[14]; distort_b_x[7] = scroll_base_b[14];
						}
						break;

				}

		}




		num_a = *attribute_a_address & 0x7ff;
		num_b = *attribute_b_address & 0x7ff;



			 genesis_plot_distorted_tile(dest,
	   						num_a, *attribute_a_address, sx, scroll_a_y, distort_a_x);

		   //	dirty_attribute_a[offs] = attribute_a;
		   //	tile_changed_2[num_a] = 0;

		 	genesis_plot_distorted_tile(dest,
		  					num_b, *attribute_b_address, sx, scroll_b_y, distort_b_x);

		   //	dirty_attribute_b[offs] = attribute_b;
		   //	tile_changed_2[num_b] = 0;

		sx += 8;

		if (sx == (h_mask+1) )
		{
			sy+=8;
			line+=(vdp_h_scrollsize << 1);
			sx=0;

			attribute_a_address = (unsigned short *)(vdp_pattern_scroll_a+line);
			attribute_b_address = (unsigned short *)(vdp_pattern_scroll_b+line);
		}
	  	else
	 	{
	 		attribute_a_address ++;
	 		attribute_b_address ++;
		}
	}



}

inline void genesis_plot_distorted_tile(struct mame_bitmap *dest, int tilenum, int attribute, int sx, int sy, short *distort)
{
	/* Bugger! If only I could plot 4 pixels at a time... would it be faster on macs if I
	packed reads into a 32-bit word and wrote that? */

	unsigned char code = ((attribute >> 9) & 0x30);
	int line;
	unsigned char *bm;
	unsigned char *c;
	int flips = (attribute & 0x1800);
	int h_mask = (vdp_h_scrollsize << 3)-1;
	#ifdef LSB_FIRST
	#define OF0 1
	#define OF1 0
	#define OF2 3
	#define OF3 2
	#else
	#define OF0 0
	#define OF1 1
	#define OF2 2
	#define OF3 3
	#endif
  	if (sy > 216) return;
  	if (sy < 0) return;

	switch (flips)
	{
		case 0x00: // no flips
		c=&vdp_vram[tilenum<<5];


		for (line = 0; line < 8; line++)
		{
			if (((sx+distort[line]) & h_mask)> 320) {c+=4;continue;}

			bm = &dest->line[sy+line][(sx+distort[line]) & h_mask];
		  		if (bm[0]==colours[0]) bm[0]=colours[(c[OF0]>>4) | code];
				if (bm[1]==colours[0]) bm[1]=colours[(c[OF0]&0xf) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF1]>>4) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF1]&0xf) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF2]>>4) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF2]&0xf) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF3]>>4) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF3]&0xf) | code];
				c+=4;

		}
		break;

	   	case 0x800: // X flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			if (((sx+distort[line]) & h_mask)> 320) {c+=4;continue;}

			bm = &dest->line[sy+line][(sx+distort[line]) & h_mask];
		   		if (bm[1]==colours[0]) bm[1]=colours[(c[OF3]>>4) | code];
				if (bm[0]==colours[0]) bm[0]=colours[(c[OF3]&0xf) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF2]>>4) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF2]&0xf) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF1]>>4) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF1]&0xf) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF0]>>4) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF0]&0xf) | code];

				c+=4;
		}
		break;

		case 0x1000: // Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
		if (((sx+distort[line]) & h_mask)> 320) {c-=4;continue;}
			bm = &dest->line[sy+line][(sx+distort[line]) & h_mask];
				if (bm[0]==colours[0]) bm[0]=colours[(c[OF0]>>4) | code];
				if (bm[1]==colours[0]) bm[1]=colours[(c[OF0]&0xf) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF1]>>4) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF1]&0xf) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF2]>>4) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF2]&0xf) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF3]>>4) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF3]&0xf) | code];
				c-=4;

		}
		break;

		case 0x1800: // X & Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			if (((sx+distort[line]) & h_mask)> 320) {c-=4;continue;}

			bm = &dest->line[sy+line][(sx+distort[line]) & h_mask];
		   		if (bm[1]==colours[0]) bm[1]=colours[(c[OF3]>>4) | code];
				if (bm[0]==colours[0]) bm[0]=colours[(c[OF3]&0xf) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF2]>>4) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF2]&0xf) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF1]>>4) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF1]&0xf) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF0]>>4) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF0]&0xf) | code];

				c-=4;

		}
		break;

	}
}


/* fonky tile plotter - ASMable fairly easily */

inline void genesis_plot_tile(struct mame_bitmap *dest, int tilenum, int attribute, int sx, int sy)
{
	/* Bugger! If only I could plot 4 pixels at a time... would it be faster on macs if I
	packed reads into a 32-bit word and wrote that? */

	unsigned char code = ((attribute >> 9) & 0x30) /*| ((attribute & 0x8000) >> 8)*/;
	int line;
	unsigned char *bm;
	unsigned char *c;
	int flips = (attribute & 0x1800);
	#ifdef LSB_FIRST
	#define OF0 1
	#define OF1 0
	#define OF2 3
	#define OF3 2
	#else
	#define OF0 0
	#define OF1 1
	#define OF2 2
	#define OF3 3
	#endif
	switch (flips)
	{
		case 0x00: // no flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			bm = &dest->line[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];
		  		if (bm[0]==colours[0]) bm[0]=colours[(c[OF0]>>4) | code];
				if (bm[1]==colours[0]) bm[1]=colours[(c[OF0]&0xf) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF1]>>4) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF1]&0xf) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF2]>>4) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF2]&0xf) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF3]>>4) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF3]&0xf) | code];
				c+=4;
				/*
				bm[0]=colours2[c[0] | code];
				bm[1]=colours2[c[1] | code];
				bm[2]=colours2[c[2] | code];
				bm[3]=colours2[c[3] | code];
				bm[4]=colours2[c[4] | code];
				bm[5]=colours2[c[5] | code];
				bm[6]=colours2[c[6] | code];
				bm[7]=colours2[c[7] | code]; */

		}
		break;

		case 0x800: // X flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			bm = &dest->line[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];

		   		if (bm[1]==colours[0]) bm[1]=colours[(c[OF3]>>4) | code];
				if (bm[0]==colours[0]) bm[0]=colours[(c[OF3]&0xf) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF2]>>4) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF2]&0xf) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF1]>>4) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF1]&0xf) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF0]>>4) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF0]&0xf) | code];

				c+=4;
			  /*	bm[0]=colours2[c[7] | code];
				bm[1]=colours2[c[6] | code];
				bm[2]=colours2[c[5] | code];
				bm[3]=colours2[c[4] | code];
				bm[4]=colours2[c[3] | code];
				bm[5]=colours2[c[2] | code];
				bm[6]=colours2[c[1] | code];
				bm[7]=colours2[c[0] | code]; */

		}
		break;

		case 0x1000: // Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = &dest->line[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

				if (bm[0]==colours[0]) bm[0]=colours[(c[OF0]>>4) | code];
				if (bm[1]==colours[0]) bm[1]=colours[(c[OF0]&0xf) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF1]>>4) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF1]&0xf) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF2]>>4) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF2]&0xf) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF3]>>4) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF3]&0xf) | code];
				c-=4;

			/*	bm[0]=colours2[c[0] | code];
				bm[1]=colours2[c[1] | code];
				bm[2]=colours2[c[2] | code];
				bm[3]=colours2[c[3] | code];
				bm[4]=colours2[c[4] | code];
				bm[5]=colours2[c[5] | code];
				bm[6]=colours2[c[6] | code];
				bm[7]=colours2[c[7] | code]; */


		}
		break;

		case 0x1800: // X & Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = &dest->line[sy+line][sx];
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

		   		if (bm[1]==colours[0]) bm[1]=colours[(c[OF3]>>4) | code];
				if (bm[0]==colours[0]) bm[0]=colours[(c[OF3]&0xf) | code];
				if (bm[3]==colours[0]) bm[3]=colours[(c[OF2]>>4) | code];
				if (bm[2]==colours[0]) bm[2]=colours[(c[OF2]&0xf) | code];
				if (bm[5]==colours[0]) bm[5]=colours[(c[OF1]>>4) | code];
				if (bm[4]==colours[0]) bm[4]=colours[(c[OF1]&0xf) | code];
				if (bm[7]==colours[0]) bm[7]=colours[(c[OF0]>>4) | code];
				if (bm[6]==colours[0]) bm[6]=colours[(c[OF0]&0xf) | code];

				c-=4;
			/*	bm[0]=colours2[c[7] | code];
				bm[1]=colours2[c[6] | code];
				bm[2]=colours2[c[5] | code];
				bm[3]=colours2[c[4] | code];
				bm[4]=colours2[c[3] | code];
				bm[5]=colours2[c[2] | code];
				bm[6]=colours2[c[1] | code];
				bm[7]=colours2[c[0] | code]; */

		}
		break;

	}
}

void plot_sprites(int priority)
{
	#ifdef LSB_FIRST
	#define SIZE 3
	#define NEXT 2
	#else
	#define SIZE 2
	#define NEXT 3
	#endif

	unsigned char *current_sprite;

	current_sprite = vdp_pattern_sprite;
	while(1)
	{
		int indx, x, y, code, attribute, size, blocks_x, blocks_y;

		attribute = *(unsigned short *)(current_sprite+4);


		/* plot from top to bottom, starting with highest priority */



		if ((attribute & 0x8000) == priority)
		{
			code = attribute & 0x7ff;
			size = *(current_sprite+SIZE);
			blocks_x = ((size >> 2) & 3)+1;
			blocks_y = (size & 3)+1;
			//bitmap_sprite->width  =	blocks_x << 3;
			//bitmap_sprite->height = blocks_y << 3;


			y = (*(short *) current_sprite   )&0x3ff;/*-0x80*/;
			x = (*(short *)(current_sprite+6))&0x3ff;/*-0x80*/;

			/* plot unflipped to temporary bufferspace */
			switch (attribute & 0x1800)
			{
				case 0: /* no flips */
					for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}
				break;

				case 0x800: /* X flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}


				break;

				case 0x1000: /* Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;

				case 0x1800: /* X & Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_tile(spritelayer, code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;
			}


		}




	   /*	logerror("addr = %x, sprcount %x\n", current_sprite, numberofsprites);  */
		if (*(current_sprite + NEXT) == 0) break;
		current_sprite = (vdp_pattern_sprite + (*(current_sprite + NEXT)<<3) );

	}
//	while (*(current_sprite + NEXT) != 0);
}
 void genesis_modify_display(int);
/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( genesis )
{
	genesis_modify_display(0);
	copybitmap(bitmap, bitmap2, 0, 0, 0, 0, 0, 0, 0);
}

void genesis_modify_display(int inter)
{

   	int offs;
   	int pom;
	int sx,sy;

	void *temp_ptr;


	/* as we're dealing with VRAM memory, which was copied over using *(short *) accsses, the bytes
	will be mangled on littleendian systems to allow this. As a result, we need to take these into
	account when accessing this area byte-wise */


	struct rectangle visiblearea =
	{
		0, 0,
		0, 0
	};

  //	struct rectangle visiblearea =
  //	{
  //		128, 128,
  //		0, 0
  //	};


//	char vdp_h_mask = vdp_h_scrollsize-1;
   //cpu_halt(1,0);
   if (!vdp_v_interrupt || !vdp_display_enable) return;

 //  if (z80running)
 //  {
 //  		cpu_cause_interrupt(1,Z80_NMI_INT);
 //		logerror("Allowing Z80 interrupt\n");
 //  }

	/* Setup the non-constant fields */
	//scroll_element.gfxdata = bitmap_vram;
	visiblearea.max_x = (vdp_h_scrollsize<<3)-1;
	visiblearea.max_y = vdp_scroll_height-1;
	spritelayer->width = visiblearea.max_x + 256;
	spritelayer->height = visiblearea.max_y + 256;
	//scroll_element.colortable = &colours[0];

      /*	genesis_dma_poll(200); */
/*	if (!vdp_display_enable) return;*/


/* organise sprites by priority, into the sprite layer  */

  // 	fillbitmap(spritelayer, colours[0], 0);
   if (inter == 0) {
 // 	plot_sprites(0x8000);
 // 	plot_sprites(0);

/* translate palette */

	for (pom = 1; pom < 64; pom++) /* don't set colour 0 here, as it's background */
	{

		if (dirty_colour[pom])
		{
			int colour = vdp_cram[pom];
	   		osd_modify_pen(Machine->pens[pom],
			((colour<<4) & 0xe0) DIM,
	   		((colour   ) & 0xe0) DIM,
	   		((colour>>4) & 0xe0) DIM);
	   		colours[pom] = (pom & 0x0f) ? Machine->pens[pom] : Machine->pens[0];
			dirty_colour[pom] = 0;
		}
	}

	if (dirty_colour[0])
	{
		int colour = vdp_cram[vdp_background_colour];
	  	osd_modify_pen(Machine->pens[0],
		((colour<<4) & 0xe0) DIM,
	   	((colour   ) & 0xe0) DIM,
	   	((colour>>4) & 0xe0) DIM);  /* quick background colour set */
	   	colours[0] = Machine->pens[0];
		dirty_colour[0] = 0;
	}

	fillbitmap(spritelayer, colours[0], 0);

  	plot_sprites(0x8000);
  	plot_sprites(0);


  #if 0
   /* copy the tiles into the scroll layer A and B buffers */
    sx = 0;
    sy = 0;

	for (pom = 0x0;pom < (vdp_h_scrollsize*vdp_v_scrollsize)<<1; pom+=2)
	{
		int num_a, num_b;
		int attribute_a = *(unsigned short *)(vdp_pattern_scroll_a+pom);
 		int attribute_b = *(unsigned short *)(vdp_pattern_scroll_b+pom);
		offs=pom >> 1;

		if (sx > vdp_h_mask<<3)
		{
			sy+=8;
			sx=0;
		}


		num_a = attribute_a & 0x7ff;
		num_b = attribute_b & 0x7ff;

		if ((dirty_attribute_a[offs] !=attribute_a) || tile_changed_1[num_a])
		{
		 //	sx = (offs & vdp_h_mask)<<3;
		 //	sy = (offs / vdp_h_scrollsize)<<3;
	   		genesis_plot_tile(scroll_a,
	   						num_a, attribute_a, sx, sy);

			dirty_attribute_a[offs] = attribute_a;
			tile_changed_2[num_a] = 0;
		 //	tile_changed_1[num_a] = 0;

		}

		if ((dirty_attribute_b[offs] !=attribute_b) || tile_changed_1[num_b])
		{

		 //	sx = (offs & vdp_h_mask)<<3;
		 //	sy = (offs / vdp_h_scrollsize)<<3;

		  	genesis_plot_tile(scroll_b,
		  					num_b, attribute_b, sx, sy);

			dirty_attribute_b[offs] = attribute_b;
			tile_changed_2[num_b] = 0;
		 //	tile_changed_1[num_b] = 0;

		}

	 sx+=8;
	}
   #endif

	/* swap the tile changed buffers around */
					  }
  	temp_ptr = tile_changed_1;
  	tile_changed_1 = tile_changed_2;
  	tile_changed_2 = temp_ptr;


	/* combine all the layers adding scroll, wraparound and distortion, into the output bitmap */

   //	combinelayers(bitmap,0,vdp_display_height);
	// combinelayers(bitmap2,inter,inter+2);
	combinelayers2(bitmap2,0,vdp_display_height);

 /* mark all tiles as unchanged */
   //	memset(tile_changed_1, 0, 0x800);
   //	memset(tile_changed_2, 0, 0x800);


}


#endif

