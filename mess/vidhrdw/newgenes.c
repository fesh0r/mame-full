/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "machine/genesis.h"
#include "vidhrdw/genesis.h"

int z80running;

int debug_a_y;
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

unsigned short *Pen;
unsigned char *scroll_a;
unsigned char *scroll_b;
//mame_bitmap *bitmap_vram;
//mame_bitmap *bitmap_sprite;


unsigned char *spritelayer;

unsigned short colours[256];

char dirty_colour[64];

short dirty_attribute_a[16384];
short dirty_attribute_b[16384];

unsigned char zbuffera[1024*128]; /* 1 bit zbuffer for for 128x128 tiles ((128*8) * 128)*/
unsigned char zbufferb[1024*128];
unsigned int table[(256*256)*8];


char *tile_changed_1, *tile_changed_2;

typedef struct
{
	mame_bitmap *bitmap;
	int x;
	int y;
	int attribute;
	int width;
	int height;
	int x_end;
	int y_end;
} GENESIS_SPRITE;





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


void genesis_videoram1_w (int offset, int data)
{
	if (errorlog) fprintf(errorlog, "what is this doing? %x, %x\n", offset, data);
	offset = data;
}

#define PRIORITY_S		3
#define PRIORITY_A		2
#define PRIORITY_B		1
#define PRIORITY_DONE	0

/* list is the priority order of layers A, B and S */
char pri[] =
{
	1, 3, 2, 1, 2, 3,
	2, 1, 3, 2, 1, 3,
	3, 2, 1, 2, 1, 3,
	3, 1, 2, 2, 1, 3
/*	PRIORITY_B, PRIORITY_S, PRIORITY_A,	PRIORITY_S, PRIORITY_B, PRIORITY_A,
	PRIORITY_S, PRIORITY_A, PRIORITY_B, PRIORITY_S, PRIORITY_A, PRIORITY_B,
	PRIORITY_A, PRIORITY_B, PRIORITY_S, PRIORITY_S, PRIORITY_A, PRIORITY_B,
	PRIORITY_A, PRIORITY_S, PRIORITY_B, PRIORITY_S, PRIORITY_A, PRIORITY_B	*/
};

unsigned int generate_priority_entry(int a, int b, int priority_number, int sprite)
{
	int pixel;
	unsigned int value = 0;
	int position;

	a <<=1;

	if (sprite) sprite = 3;

	for (pixel = 0; pixel < 8; pixel++)
	{
		position = ((b>>pixel)&1) | ((a>>pixel)&2);

		value |= (pri[(position*6)+sprite+priority_number]) << (pixel<<1);
	}

	return value;

}

int genesis_vh_start (void)
{
	unsigned int a,b;
	int entry;
	if (video_start_generic() != 0)
		return 1;

	Pen = &Machine->pens[0];

	memset(zbuffera, 0, 1024*128);
	memset(zbufferb, 0, 1024*128);

 	for (a = 0; a < 256; a++)
	{
		for (b = 0; b < 256; b++)
		{
	  		entry = ((a << 8) | b)*8;
			table[entry  ]=	generate_priority_entry(a, b, 0, 0);
			table[entry+1]= generate_priority_entry(a, b, 1, 0);
			table[entry+2]=	generate_priority_entry(a, b, 2, 0);

			table[entry+4]=	generate_priority_entry(a, b, 0, 1);
			table[entry+5]=	generate_priority_entry(a, b, 1, 1);
			table[entry+6]=	generate_priority_entry(a, b, 2, 1);
			if (errorlog) fprintf(errorlog, "%x %x %x\t%x %x %x\n",
			table[entry], table[entry+1], table[entry+2], table[entry+4], table[entry+5], table[entry+6]);
		}
	}

	/* create scrollA and scrollB playfields */

  	if ((scroll_a = auto_malloc(1024*1024)) == 0)
		return 1;
	if ((scroll_b = auto_malloc(1024*1024)) == 0)
		return 1;
   	if ((spritelayer = auto_malloc(512*512)) == 0)
		return 1;

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
	//	bitmap_free(bitmap_vram);
	//	bitmap_free(bitmap_sprite);
		return 1;
	}

	if ((tile_changed_2 = auto_malloc(0x800)) == 0)
	{
	//	bitmap_free(scroll_a);
	//	bitmap_free(scroll_b);
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
			if (errorlog) fprintf(errorlog, "Unknown get_dma_dest_address id %x!!\n", id);
	}
	return NULL;
}

int genesis_vdp_data_r (int offset)
{
	int data = 0;		 /* don't worry about this for now, really doesn't happen */

   	if (errorlog) fprintf(errorlog, "reading from... %x\n", (((vdp_address & 0xfffffffe)+ (int)&vdp_vram[0])) );

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
			if (errorlog) fprintf(errorlog,"unknown vdp port read type %x\n", vdp_id);
	}

   /*	if ((offset == 1) || (offset == 3))	  */
		vdp_address += vdp_auto_increment;

	return data;
}

void genesis_vdp_data_w (int offset, int data)
{
  	int tempsource = 0;
  	int temp_vdp_address = vdp_address;
	/*if (errorlog) fprintf(errorlog, "vdp data w offset = %x\n", offset);*/

	/* need a special case for byte writes...? */

	if (vdp_dma_enable && (vdp_id & 0x20))
	{
		if (offset == 0 || offset == 2)
		{

		 vdp_vram_fill = COMBINE_WORD(vdp_vram_fill, data);
			temp_vdp_address = vdp_address;
//			if (errorlog) fprintf(errorlog,"DMA VRAM FILL, dest %x, fill %x, length %x, real dest %x, id %x, inc %x\n", vdp_address, vdp_vram_fill, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
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
				 if (errorlog) fprintf(errorlog, "*** %x-%x\n", temp_vdp_address, vdp_dma_length*2);
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
	  /*	if (errorlog) fprintf(errorlog,"%x",vdp_vram_fill);*/

	/*  if (first_access && (offset == 1 || offset == 3))
		if (errorlog) fprintf(errorlog, "misaligned\n"); */
		 /*  	if (errorlog) fprintf(errorlog,"would write %x to... %x\n", data, ((vdp_address+ 0*//*(int)&vdp_vram[0]*//*) +(  (offset & 0x01))) );*/
	if ((vdp_address & 1) && errorlog) fprintf(errorlog, "!");

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
			//if (errorlog) fprintf(errorlog, "%x\n", vdp_address);
			break;

		default:
			if (errorlog) fprintf(errorlog,"unknown vdp port write type %x\n", vdp_id);
	}

   //	if ((offset == 1 || offset == 3) /*|| (data_width == 1)*/)
		vdp_address += vdp_auto_increment;

	if (vdp_auto_increment == 1 && errorlog) fprintf(errorlog, "1");
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
				//	if (errorlog) fprintf(errorlog, "vdp ctrl status offset = %x\n", offset);
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
	  	  	 /* if (errorlog) fprintf(errorlog, "genesis_vdp_ctrl_w %x, %x, %x, %x\n", offset, data, vdp_data, data >>16);*/



			if ((vdp_data & 0xe000) == 0x8000) /* general register write	*/
			{
				first_read = 1;
				vdp_register = (vdp_data >> 8) & 0x1f;
			  	vdp_data = full_vdp_data & 0xff;
				/*if (errorlog) fprintf(errorlog,"register %d writing %x\n", vdp_register, vdp_data);*/
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
//						if (errorlog) fprintf(errorlog, "scrolla = %x\n", vdp_pattern_scroll_a-(int)&vdp_vram[0]);
//						if (errorlog) fprintf(errorlog, "VRAM is %x\n", &vdp_vram[0]);
					   //	memset(dirty_attribute_a, -1, (128*128)*sizeof(short));
						break;
					case 3:	/* pattern name table address, window */
						vdp_pattern_window	= (char *)(&vdp_vram[0]+(vdp_data <<10));
//						if (errorlog) fprintf(errorlog, "window = %x\n", vdp_pattern_window-(int)&vdp_vram[0]);

						break;
					case 4:	/* pattern name table address, scroll B */
						vdp_pattern_scroll_b= (char *)(&vdp_vram[0]+(vdp_data <<13));
					  //	memset(dirty_attribute_b, -1, (128*128)*sizeof(short));

//						if (errorlog) fprintf(errorlog, "scrollb = %x\n", vdp_pattern_scroll_b-(int)&vdp_vram[0]);
						break;
					case 5:	/* pattern name table address, sprite */
						vdp_pattern_sprite	= (unsigned char *)(&vdp_vram[0]+(vdp_data <<9));
//						if (errorlog) fprintf(errorlog, "sprite = %x\n", vdp_pattern_sprite-(int)&vdp_vram[0]);
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
						if (errorlog) fprintf(errorlog, "scroll modes %x, %x\n", vdp_h_scrollmode, vdp_v_scrollmode);
						break;
					case 12: /* character mode, interlace etc */
						vdp_screenmode		= vdp_data;
						vdp_h_width = ((vdp_data & 1) ? 320 : 256);
						vdp_interlace = (vdp_data >> 1) & 3;
						if (errorlog) fprintf(errorlog, "screen width, interlace flag = %d, %d\n", vdp_h_width, vdp_interlace);
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
						if (errorlog) fprintf(errorlog, "initial h scrollsize = %x\n", vdp_h_scrollsize);
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
						if (errorlog) fprintf(errorlog, "scrollsizes are %d, %d\n", vdp_h_scrollsize, vdp_v_scrollsize);

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
						if (errorlog) fprintf(errorlog,"DMA length low.. length is %x\n", vdp_dma_length);
						break;
					case 20: /* DMA length counter high */
						vdp_dma_length		= (vdp_dma_length & 0xff) | (vdp_data << 8);
						if (errorlog) fprintf(errorlog,"DMA length high.. length is %x\n", vdp_dma_length);
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
						if (errorlog) fprintf(errorlog,"23:%x, %x\n",vdp_dma_mode, vdp_dma_source);
						break;
				}
			}
			else
			{
				if (first_read)
				{
					/* if (errorlog) fprintf(errorlog,"vdp data on first read is %x\n", vdp_data); */
					vdp_address = (vdp_data & 0x3fff);
					vdp_id		= (vdp_data & 0xc000) >> 14;
					first_read	= 0;
				}
				else
				{
				   /*	 if (errorlog) fprintf(errorlog,"vdp data on second read is %x\n", vdp_data); */
					vdp_address |= ((vdp_data & 0x03) << 14);
					vdp_id |= ( (vdp_data & 0xf0) >> 2);
					first_read	= 1;
					if (errorlog) fprintf(errorlog,"vdp id is %x\n", vdp_id);

					if (errorlog) fprintf(errorlog,"address set, %x\n", vdp_address);

					if (vdp_dma_enable && (vdp_id & 0x20))
					{

						/* DMA! */
						switch (vdp_dma_mode)
						{
/*#if 0*/
							case DMA_ROM_VRAM:
//								if (errorlog) fprintf(errorlog,"DMA ROM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

							  	genesis_initialise_dma(&memory_region(REGION_CPU1)[vdp_dma_source & 0x3fffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

/*#endif*/
								break;
							case DMA_RAM_VRAM:
//								if (errorlog) fprintf(errorlog,"DMA RAM->VRAM, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);

								/*	if (vdp_address+(vdp_dma_length*vdp_auto_increment) > 0xffff) printf("error! sdil: %x %x %x %x\n", vdp_dma_source, vdp_address, vdp_auto_increment, vdp_dma_length);*/

							  	genesis_initialise_dma(&genesis_sharedram[vdp_dma_source & 0xffff], vdp_address,	vdp_dma_length * 2, vdp_id, vdp_auto_increment);

								break;
							case DMA_VRAM_FILL: /* handled at other port :-) */
								if (errorlog) fprintf(errorlog, "VRAM FILL pending, awaiting fill data set\n");
								break;
							case DMA_VRAM_COPY:
//								if (errorlog) fprintf(errorlog,"DMA VRAM COPY, src %x dest %x, length %x, real dest %x, id %x, inc %x\n",  vdp_dma_source, vdp_address, vdp_dma_length, get_dma_dest_address(vdp_id)+vdp_address, vdp_id, vdp_auto_increment);
								/*	tempdest = vdp_vram+vdp_address;
								for (tempsource = 0; tempsource < (vdp_dma_length); tempsource++)
								{
									 *tempdest = *(char *)(vdp_vram+(vdp_dma_source)+tempsource);
									 tempdest += vdp_auto_increment;
								}
								*/
								break;
							default:
								if (errorlog) fprintf(errorlog,"unknown vdp dma mode type %x\n", vdp_dma_mode);
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
		if (errorlog) fprintf(errorlog, "poll: src %p, end %p, id %x, vram dest %x, inc %x\n", current_dma_src, current_dma_end, current_dma_id, current_dma_vram_dest, current_dma_increment);

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
				if (errorlog) fprintf(errorlog, "%x-%x\n", current_dma_vram_dest, counter);
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
				//if (errorlog && (sy >> 3) > 0x800) fprintf(errorlog,2 "ERK! %x\n", (sy >> 3));


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

inline void genesis_plot_layer_tile(unsigned char *dest, unsigned int attribute, unsigned int sx, unsigned int sy)
{
	/* Bugger! If only I could plot 4 pixels at a time... would it be faster on macs if I
	packed reads into a 32-bit word and wrote that? */

	//unsigned char code = (attribute & 0x8000) ? 0xff : 0x00;
	unsigned int code;
	unsigned char *bm;
	unsigned char *c;
	int tilenum;
	int line;
	//int position = ((sy>>3)<<7)+ (sx>>3);
	int flips = (attribute & 0x1800);
	unsigned int firsthalf, secondhalf;
	unsigned char zb;
	unsigned char *zbuffer = (dest == scroll_a) ? zbuffera : zbufferb;
	//mask[position]=code;
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

	/* decide whether we're writing a 1 or a 0 to the zbuffer depending on the scroll layer & priority */

 //	if (dest == scroll_a)
 //	{
 //		attribute = attribute_a;
 //
 //	}
 //	else
 //	{
 //		attribute = attribute_b;
 //	}
 	tilenum = attribute & 0x7ff;

	code = ((attribute >> 9) & 0x30) * 0x01010101;

	if ( (dest == scroll_a && (attribute & 0x8000)) || (dest == scroll_b && (attribute & 0x8000)==0))
	{
		zb = 0xff;
	}
	else
	{
		zb = 0;
	}

	/* now update the zbuffer for this tile */

	zbuffer[((sy+0)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+1)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+2)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+3)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+4)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+5)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+6)<<7)+(sx>>3)]=zb;
	zbuffer[((sy+7)<<7)+(sx>>3)]=zb;


	/* code is the default code to apply to the bits, 1 meaning priority flag set for this layer, 0 otherwise.
	The flag is reset if the pixel was found to be transparent */
	switch (flips)
	{
		case 0x00: // no flips
		default:
		c=&vdp_vram[tilenum<<5];
		bm = dest+(sy<<10)+sx;

		for (line = 0; line < 8; line++)
		{
			//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];
		  	 /*   bm[0]=(c[OF0]>>4)  | code;
			    bm[1]=(c[OF0]&0xf) | code;
			    bm[2]=(c[OF1]>>4)  | code;
			    bm[3]=(c[OF1]&0xf) | code;
			    bm[4]=(c[OF2]>>4)  | code;
			    bm[5]=(c[OF2]&0xf) | code;
			    bm[6]=(c[OF3]>>4)  | code;
			    bm[7]=(c[OF3]&0xf) | code; 	*/
				firsthalf  = ((c[OF0]>>4) | ((c[OF0]&0xf)<<8) | ((c[OF1]>>4)<<16) | ((c[OF1]&0xf)<<24))	| code;
				secondhalf = ((c[OF2]>>4) | ((c[OF2]&0xf)<<8) | ((c[OF3]>>4)<<16) | ((c[OF3]&0xf)<<24))	| code;
				*(int *) bm		= firsthalf;
				*(int *)(bm+4)	= secondhalf;
				c+=4;
				bm+=1024;
			  //	position += vdp_v_scrollsize;
			  //	mask[position]=code;

		}
		break;
	   #if 0
		case 0x800: // X flips
		c=&vdp_vram[tilenum<<5];
		for (line = 0; line < 8; line++)
		{
			bm = dest+((sy+line)<<10)+sx;
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];

		   if !(		bm[1]=(c[OF3]>>4)  ;
		   if !(		bm[0]=(c[OF3]&0xf) ;
		   if !(		bm[3]=(c[OF2]>>4)  ;
		   if !(		bm[2]=(c[OF2]&0xf) ;
		   if !(		bm[5]=(c[OF1]>>4)  ;
		   if !(		bm[4]=(c[OF1]&0xf) ;
		   if !(		bm[7]=(c[OF0]>>4)  ;
		   if !(		bm[6]=(c[OF0]&0xf) ;

				c+=4;

		}
		break;

		case 0x1000: // Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = dest+((sy+line)<<10)+sx;
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

		   if !(		bm[0]=(c[OF0]>>4)  ;
		   if !(		bm[1]=(c[OF0]&0xf) ;
		   if !(		bm[2]=(c[OF1]>>4)  ;
		   if !(		bm[3]=(c[OF1]&0xf) ;
		   if !(		bm[4]=(c[OF2]>>4)  ;
		   if !(		bm[5]=(c[OF2]&0xf) ;
		   if !(		bm[6]=(c[OF3]>>4)  ;
		   if !(		bm[7]=(c[OF3]&0xf) ;
				c-=4;



		}
		break;

		case 0x1800: // X & Y flips
		c=&vdp_vram[tilenum<<5]+28;
		for (line = 0; line < 8; line++)
		{
			bm = dest+((sy+line)<<10)+sx;
		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

		   if !(	bm[1]=(c[OF3]>>4)  ;
		   if !(	bm[0]=(c[OF3]&0xf) ;
		   if !(	bm[3]=(c[OF2]>>4)  ;
		   if !(	bm[2]=(c[OF2]&0xf) ;
		   if !(	bm[5]=(c[OF1]>>4)  ;
		   if !(	bm[4]=(c[OF1]&0xf) ;
		   if !(	bm[7]=(c[OF0]>>4)  ;
		   if !(	bm[6]=(c[OF0]&0xf) ;

				c-=4;

		}
		break;
		#endif

	}
}



/* Oh lordy, another combinelayers */
void combinelayers3(mame_bitmap *dest, int startline, int endline)
{
	int x;
	unsigned char y;
	//unsigned char block_a_y, block_b_y;
	char *temp_ptr;
	short *h_scroll = (short *)vdp_h_scroll_addr;
//	int transparent = Pen[0] * 0x01010101;
	int v_mask = (vdp_v_scrollsize << 3)-1;
	int h_mask = (vdp_h_scrollsize << 3)-1;
	unsigned int priority = 0;
	unsigned char a_shift, b_shift;
	unsigned char *shift;
	int indx, increment;
	unsigned char *sprite_ptr, *output_ptr;
     //	unsigned char *scroll_a_pixel_addr = 0, *scroll_b_pixel_addr = 0;
	unsigned char *scroll_pixel_addr = 0;
	unsigned char *scroll_a_pixel, *scroll_b_pixel;
	unsigned char sprite_pixel = 0;
	int scroll_a_attribute=0, scroll_b_attribute=0;
	unsigned int scroll_a_x = 0, scroll_a_y = 0, scroll_b_x = 0, scroll_b_y = 0;
	int scroll_x, scroll_y;
	int attribute;
	unsigned char output = 0;
	unsigned char output1,output2,output3,output4;
	int offs;
   //	int old_a_attribute=0, old_b_attribute=0;
	//unsigned int oldpriority = 0;

	int  sx = 0;
    int sy = 0;
	int pom;
	int num_a, num_b;

    debug_a_y=0;

   //	if (errorlog)
   //		for (offs = 0; offs < (1024*128); offs++)
   //			fprintf(errorlog, "%x %x\n", zbuffera[offs], zbufferb[offs]);

 	for (pom = 0x0;pom < (vdp_h_scrollsize*vdp_v_scrollsize)<<1; pom+=2)
	{

		int attribute_a = *(unsigned short *)(vdp_pattern_scroll_a+pom);
 		int attribute_b = *(unsigned short *)(vdp_pattern_scroll_b+pom);
		offs=pom >> 1;

		if (sx > h_mask)
		{
			sy+=8;
			sx=0;
		}


		if ((dirty_attribute_a[offs] !=attribute_a) || tile_changed_1[num_a])
		{
		debug_a_y++;
	   		genesis_plot_layer_tile(scroll_a, attribute_a, sx, sy);

			dirty_attribute_a[offs] = attribute_a;
		//	tile_changed_2[num_a] = 0;
		//  	tile_changed_1[num_a] = 0;

		}

		if ((dirty_attribute_b[offs] !=attribute_b) || tile_changed_1[num_b])
		{

	   	  	debug_a_y++;
		  	genesis_plot_layer_tile(scroll_b, attribute_b, sx, sy);

			dirty_attribute_b[offs] = attribute_b;
		//	tile_changed_2[num_b] = 0;
		//  	tile_changed_1[num_b] = 0;

		}

	 sx+=8;
	}
	if (errorlog) fprintf(errorlog, "tiles redrawn = %d\n", debug_a_y);
	/* swap the tile changed buffers around */

 // 	temp_ptr = tile_changed_1;
 // 	tile_changed_1 = tile_changed_2; /* tile_changed_1 will now be zeroed */
 // 	tile_changed_2 = temp_ptr;
 memset(tile_changed_1, 0, 0x800);


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

   		   	sprite_ptr = spritelayer +(y<<9)+x;
			output_ptr = &dest->line[y][x];

		  	scroll_a_pixel = scroll_a+(scroll_a_y<<10);
		  	scroll_b_pixel = scroll_b+(scroll_b_y<<10);

			for (indx = 0; indx < increment; indx+=8)
			{
			unsigned char z;
			unsigned int zza, zzb, zzs;
			unsigned char priority_loop;
			unsigned int *priority_table;
			unsigned int table_offset;
			unsigned char sprite_priority;
			unsigned char output_pixel;

				table_offset = (((((zbuffera[(scroll_a_y<<7)+(( scroll_a_x    )>>3)] << 8)
							 |     (zbuffera[(scroll_a_y<<7)+(((scroll_a_x+8)&h_mask )>>3)])    )
								 >>  (8-(scroll_a_x & 3 ))) & 0xff) << 8);
				table_offset |=(((((zbufferb[(scroll_b_y<<7)+(( scroll_b_x    )>>3)] << 8)
							 |     (zbufferb[(scroll_b_y<<7)+(((scroll_b_x+8)&h_mask )>>3)])    )
								 >>  (8-(scroll_b_x & 3 ))) & 0xff)     );

			 /* 	if (errorlog)
			  	{
			  		fprintf(errorlog, "ax,bx,ay,by,table offset %d, %d, %d, %d, %x, %x\n", scroll_a_x, scroll_b_x,
			  																			   scroll_a_y, scroll_b_y,
			  																			   table_offset,0

			  																			   );
			  		fflush(errorlog);
			  	} */

				priority_table = &table[table_offset*8];

	   //			zza= priority_table[0];
	   //			zzb= priority_table[1];
	   //			zzs= priority_table[2];

			  	for (priority_loop = 0; priority_loop < 16; priority_loop+=2)
				{
					sprite_priority = (sprite_ptr[0] & 0x80) ? 4 : 0;

					z = 0;
					zza= priority_table[0 + sprite_priority] >> (priority_loop)&3;
					zzb= priority_table[1 + sprite_priority] >> (priority_loop)&3;
				   	zzs= priority_table[2 + sprite_priority] >> (priority_loop)&3;
				   //	 zzs=rand();
				   /*	if (errorlog)
					{
						if (((zza&3)==0) && table_offset != 0xffff) fprintf(errorlog, "zza==0 table_offset,val=%x %x %x %x\n", table_offset, priority_table[0], priority_table[1], priority_loop);
						if (((zzb&3)==0) && table_offset != 0xffff) fprintf(errorlog, "zzb==0 table_offset,val=%x %x %x %x\n", table_offset, priority_table[0], priority_table[1], priority_loop);
					}*/

					if (zza>z)
						if (scroll_a_pixel[scroll_a_x] & 0x0f)
						{
							output_pixel = scroll_a_pixel[scroll_a_x];
							z = zza;
						}

					if (zzb>z)
						if (scroll_b_pixel[scroll_b_x] & 0x0f)
						{
							output_pixel = scroll_b_pixel[scroll_b_x];
							z = zzb;
						}

					if (zzs>z)
						if  (sprite_ptr[0] & 0x0f)
						{
							output_pixel = (sprite_ptr[0]&0x7f);
							z = zzs;
						}

					if (z == 0) output_pixel = vdp_background_colour;

					*(output_ptr++) = Pen[output_pixel];
					scroll_a_x = (++scroll_a_x) & h_mask;
					scroll_b_x = (++scroll_b_x) & h_mask;
					sprite_ptr++;
				  //	zza >>=2;
				  //	zzb >>=2;
				  //	zzs >>=2;
				}


				/* here, the priority information in the table is used as a shift register to calculate
				which layer should have it's pixel read and decoded to see if it's opaque - if it is, this
				is the pixel that will end up on the screen, otherwise we drop through to the layer behind it */

				/* note that the address indirections only occur when it's necessary to indirect, all
				calculations act on an address which will be indirected only when it's absolutely necessary
				to obtain the pixel's value */

				/**(int *)output_ptr=(Pen[output1]      )+
								   (Pen[output2] <<  8)+
								   (Pen[output3] << 16)+
								   (Pen[output4] << 24); */



				//output_ptr+=8;
				//if (*output_ptr != output) *output_ptr = output;

				/* increment our X position within the layers accounting for wraparound */
			   //	scroll_a_x+=8;scroll_b_x+=8;
				//sprite_ptr++;
			}

		}

	}

}






/* fonky tile plotter - ASMable fairly easily */

void genesis_plot_sprite_tile(int tilenum, int attribute, int sx, int sy)
{
	/* Bugger! If only I could plot 4 pixels at a time... would it be faster on macs if I
	packed reads into a 32-bit word and wrote that? */

	unsigned char code = ((attribute >> 9) & 0x30) | ((attribute & 0x8000) >> 8);
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

  //	if (sx <0 || sx > 311) return;
  //	if (sy <0 || sy > 215) return;
	switch (flips)
	{
		case 0x00: // no flips
		c=&vdp_vram[tilenum<<5];
		bm = spritelayer+(sy<<9)+sx;

		for (line = 0; line < 8; line++)
		{
		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];
		  		if (!(bm[0]&0x0f)) bm[0]=(c[OF0]>>4) | code;
				if (!(bm[1]&0x0f)) bm[1]=(c[OF0]&0xf) | code;
				if (!(bm[2]&0x0f)) bm[2]=(c[OF1]>>4) | code;
				if (!(bm[3]&0x0f)) bm[3]=(c[OF1]&0xf) | code;
				if (!(bm[4]&0x0f)) bm[4]=(c[OF2]>>4) | code;
				if (!(bm[5]&0x0f)) bm[5]=(c[OF2]&0xf) | code;
				if (!(bm[6]&0x0f)) bm[6]=(c[OF3]>>4) | code;
				if (!(bm[7]&0x0f)) bm[7]=(c[OF3]&0xf) | code;
				c+=4;
				bm += 512;

		}
		break;

		case 0x800: // X flips
		c=&vdp_vram[tilenum<<5];
		bm = spritelayer+(sy<<9)+sx;

		for (line = 0; line < 8; line++)
		{

		//	c  = &bitmap_vram->line[(tilenum<<3)+line][0];

		   		if (!(bm[1]&0x0f)) bm[1]=(c[OF3]>>4) | code;
				if (!(bm[0]&0x0f)) bm[0]=(c[OF3]&0xf) | code;
				if (!(bm[3]&0x0f)) bm[3]=(c[OF2]>>4) | code;
				if (!(bm[2]&0x0f)) bm[2]=(c[OF2]&0xf) | code;
				if (!(bm[5]&0x0f)) bm[5]=(c[OF1]>>4) | code;
				if (!(bm[4]&0x0f)) bm[4]=(c[OF1]&0xf) | code;
				if (!(bm[7]&0x0f)) bm[7]=(c[OF0]>>4) | code;
				if (!(bm[6]&0x0f)) bm[6]=(c[OF0]&0xf) | code;

				c+=4;
				bm += 512;

		}
		break;

		case 0x1000: // Y flips
		c=&vdp_vram[tilenum<<5]+28;
		bm = spritelayer+((sy+7)<<9)+sx;

		for (line = 0; line < 8; line++)
		{

		//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

				if (!(bm[0]&0x0f)) bm[0]=(c[OF0]>>4) | code;
				if (!(bm[1]&0x0f)) bm[1]=(c[OF0]&0xf) | code;
				if (!(bm[2]&0x0f)) bm[2]=(c[OF1]>>4) | code;
				if (!(bm[3]&0x0f)) bm[3]=(c[OF1]&0xf) | code;
				if (!(bm[4]&0x0f)) bm[4]=(c[OF2]>>4) | code;
				if (!(bm[5]&0x0f)) bm[5]=(c[OF2]&0xf) | code;
				if (!(bm[6]&0x0f)) bm[6]=(c[OF3]>>4) | code;
				if (!(bm[7]&0x0f)) bm[7]=(c[OF3]&0xf) | code;
				c-=4;
		   		bm -= 512;


		}
		break;

		case 0x1800: // X & Y flips
		c=&vdp_vram[tilenum<<5]+28;
		bm = spritelayer+((sy+7)<<9)+sx;
		for (line = 0; line < 8; line++)
		{
			//	c  = &bitmap_vram->line[(tilenum<<3)+(7-line)][0];

		   		if (!(bm[1]&0x0f)) bm[1]=(c[OF3]>>4) | code;
				if (!(bm[0]&0x0f)) bm[0]=(c[OF3]&0xf) | code;
				if (!(bm[3]&0x0f)) bm[3]=(c[OF2]>>4) | code;
				if (!(bm[2]&0x0f)) bm[2]=(c[OF2]&0xf) | code;
				if (!(bm[5]&0x0f)) bm[5]=(c[OF1]>>4) | code;
				if (!(bm[4]&0x0f)) bm[4]=(c[OF1]&0xf) | code;
				if (!(bm[7]&0x0f)) bm[7]=(c[OF0]>>4) | code;
				if (!(bm[6]&0x0f)) bm[6]=(c[OF0]&0xf) | code;

				c-=4;
				bm -= 512;

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


			y = ((*(short *) current_sprite   )&0x3ff)-0x80;
			x = ((*(short *)(current_sprite+6))&0x3ff)-0x80;

			/* plot unflipped to temporary bufferspace */
			switch (attribute & 0x1800)
			{
				case 0: /* no flips */
					for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_sprite_tile(code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}
				break;

				case 0x800: /* X flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_sprite_tile(code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((indx % blocks_y)<<3));
					}


				break;

				case 0x1000: /* Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_sprite_tile(code+indx,
								attribute /*& 0xffffe7ff*/,
								x+(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;

				case 0x1800: /* X & Y flips */
				for (indx = 0; indx < (blocks_x * blocks_y); indx ++)
					{
						genesis_plot_sprite_tile(code+indx,
								attribute /*& 0xffffe7ff*/,
								x+((blocks_x-1)<<3)-(( indx / blocks_y)<<3),y+((blocks_y-1)<<3)-((indx % blocks_y)<<3));
					}


				break;
			}


		}




	   /*	if (errorlog) fprintf(errorlog, "addr = %x, sprcount %x\n", current_sprite, numberofsprites);  */
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
//void genesis_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh)
//{
//
//genesis_modify_display(0);
////copybitmap(bitmap, bitmap2, 0, 0, 0, 0, 0, 0, 0);
//copybitmap(bitmap, bitmap2, 0, 0, 0, 0, 0, 0, 0);
//
//}
void genesis_vh_screenrefresh (mame_bitmap *bitmap, int full_refresh)
//void genesis_modify_display(int inter)
{

   	int offs;
   	int pom;
	int sx,sy;

	void *temp_ptr;


	/* as we're dealing with VRAM memory, which was copied over using *(short *) accsses, the bytes
	will be mangled on littleendian systems to allow this. As a result, we need to take these into
	account when accessing this area byte-wise */


	rectangle visiblearea =
	{
		0, 0,
		0, 0
	};

  //	rectangle visiblearea =
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
 //		if (errorlog) fprintf(errorlog, "Allowing Z80 interrupt\n");
 //  }

	/* Setup the non-constant fields */
	//scroll_element.gfxdata = bitmap_vram;
	visiblearea.max_x = (vdp_h_scrollsize<<3)-1;
	visiblearea.max_y = vdp_scroll_height-1;
  //	spritelayer->width = visiblearea.max_x + 256;
  //	spritelayer->height = visiblearea.max_y + 256;
	//scroll_element.colortable = &colours[0];

      /*	genesis_dma_poll(200); */
/*	if (!vdp_display_enable) return;*/


/* organise sprites by priority, into the sprite layer  */

  // 	fillbitmap(spritelayer, colours[0], 0);
   //if (inter == 0)

 // 	plot_sprites(0x8000);
 // 	plot_sprites(0);

/* translate palette */

	if (dirty_colour[0])
	{
		int colour = vdp_cram[vdp_background_colour];
	  	osd_modify_pen(Pen[0],
		((colour<<4) & 0xe0) DIM,
	   	((colour   ) & 0xe0) DIM,
	   	((colour>>4) & 0xe0) DIM);  /* quick background colour set */
	   	colours[0] = Pen[0];
		dirty_colour[0] = 0;
	}


	for (pom = 1; pom < 64; pom++) /* don't set colour 0 here, as it's background */
	{

		if (dirty_colour[pom])
		{
			int colour = vdp_cram[pom];
	   		osd_modify_pen(Pen[pom],
			((colour<<4) & 0xe0) DIM,
	   		((colour   ) & 0xe0) DIM,
	   		((colour>>4) & 0xe0) DIM);
	   		colours[pom] = (pom & 0x0f) ? Pen[pom] : Pen[0];
			dirty_colour[pom] = 0;
		}
	}


	//fillbitmap(spritelayer, 0, 0); /* note this is the value 0, not pen 0 */
	memset(spritelayer, 0, 512*256);
  	plot_sprites(0x8000);
  	plot_sprites(0);




	/* combine all the layers adding scroll, wraparound and distortion, into the output bitmap */

   //	combinelayers(bitmap,0,vdp_display_height);
	// combinelayers(bitmap2,inter,inter+2);
	combinelayers3(bitmap,0,vdp_display_height);


}

