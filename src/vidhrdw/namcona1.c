/*	Namco System NA1/2 Video Hardware
**
**	(Preliminary)
**	scrolling isn't well understood
**	ROZ layer attributes aren't implemented
*/

#include "vidhrdw/generic.h"

/*************************************************************************/

extern int cgang_hack( void );

UINT8 *namcona1_vreg;
UINT8 *namcona1_scroll;
UINT8 *namcona1_workram;

#define CG_COUNT (0x1000)
static UINT8 *dirtychar;
static UINT8 dirtygfx;
static UINT8 *shaperam;
static UINT8 *cgram;

static struct tilemap *tilemap[4];
static int tilemap_palette_bank[4];

static const UINT16 *tilemap_videoram;
static int tilemap_color;

static void tilemap_get_info( int tile_index ){
	int data = tilemap_videoram[tile_index];
	int tile = data&0xfff;
	SET_TILE_INFO( 0,tile,tilemap_color );
	tile_info.mask_data = shaperam+(0x08*tile);
}

/*************************************************************************/

WRITE_HANDLER( namcona1_videoram_w ){
	int oldword = READ_WORD( &videoram[offset] );
	int newword = COMBINE_WORD( oldword,data );
	if( oldword!=newword ){
		WRITE_WORD( &videoram[offset],newword );
		offset/=2;
		tilemap_mark_tile_dirty( tilemap[offset/0x1000], offset&0xfff );
	}
}
READ_HANDLER( namcona1_videoram_r ){
	return READ_WORD( &videoram[offset] );
}

/*************************************************************************/

READ_HANDLER( namcona1_paletteram_r ){
	return READ_WORD(&paletteram[offset]);
}

WRITE_HANDLER( namcona1_paletteram_w ){
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	WRITE_WORD(&paletteram[offset],newword);
	if( !keyboard_pressed( KEYCODE_L ) )
	{
		/* -RRRRRGGGGGBBBBB */
		int r = (newword>>10)&0x1f;
		int g = (newword>>5)&0x1f;
		int b = newword&0x001f;

		palette_change_color(offset/2,
			(r<<3)|(r>>2),
			(g<<3)|(g>>2),
			(b<<3)|(b>>2));
	}
}

/*************************************************************************/

static struct GfxLayout shape_layout = {
	8,8,
	CG_COUNT,
	1, /* 1BPP */
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
	8*8
};

static struct GfxLayout cg_layout = {
	8,8,
	CG_COUNT,
	8, /* 8BPP */
	{ 0,1,2,3,4,5,6,7 },
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7 },
	64*8
};

READ_HANDLER( namcona1_gfxram_r ){
	int type = READ_WORD( &namcona1_vreg[0x0c] );
	if( type == 0x03 ){ /* shaperam */
		if( offset<8*CG_COUNT ){
			return 256*shaperam[offset]+shaperam[offset+1];
		}
		return 0x0000;
	}
	else if( type == 0x02 ){ /* cgram read */
		return 256*cgram[offset]+cgram[offset+1];
	}
	else {
		logerror( "unk gfx read:%02x %08x\n", type, cpu_get_pc() );
		return 0x0000;
	}
}

WRITE_HANDLER( namcona1_gfxram_w ){
	int oldword, newword;
	int type = READ_WORD( &namcona1_vreg[0x0c] );
	switch( type ){
	case 0x03: /* shaperam */
		if( offset<8*CG_COUNT ){
			oldword = 256*shaperam[offset]+shaperam[offset+1];
			newword = COMBINE_WORD(oldword,data);
			if( oldword != newword ){
				dirtygfx = 1;
				dirtychar[offset/8] = 1;
				shaperam[offset] = newword>>8;
				shaperam[offset+1] = newword&0xff;
			}
		}
		else {
			logerror( "bad shaperam write: %08x\n", cpu_get_pc() );
		}
		break;

//	case 0x01: /* knuckleheads? */
//		namcona1_paletteram_w( offset, data );
//		break;

	case 0x02: /* cgram write */
		oldword = 256*cgram[offset]+cgram[offset+1];
		newword = COMBINE_WORD(oldword,data);
		if( oldword != newword ){
			dirtygfx = 1;
			dirtychar[offset/0x40] = 1;
			cgram[offset] = newword>>8;
			cgram[offset+1] = newword&0xff;
		}
		break;

	default:
		logerror( "unk gfx write:%02x %08x\n", type, cpu_get_pc() );
		break;
	}
}

static void update_gfx( void ){
	if( dirtygfx ){
		int i;
		for( i = 0;i < CG_COUNT;i++ ){
			if( dirtychar[i] ){
				dirtychar[i] = 0;
				decodechar(Machine->gfx[0],i,cgram,&cg_layout);
				decodechar(Machine->gfx[1],i,shaperam,&shape_layout);
			}
		}
		dirtygfx = 0;

		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}
}

/*************************************************************************/
#ifdef NAMCONA1_DEBUG

FILE *namcona1_dump;
FILE *gfx_in;

static void special( struct osd_bitmap *bitmap ){
	static UINT32 source_baseaddr, dest_baseaddr;
	static UINT32 num_bytes;
	static UINT16 param0,param1,param2;
	static UINT16 param3,param4,param5;

	if( keyboard_pressed( KEYCODE_Z ) ){
		UINT32 source = source_baseaddr;
		int x,y,row,col;
		int i;
		for( i=0; i<256; i++ ){
			palette_change_color(i, i,(i*4)&0xff,(i*8)&0xff );
		}

		for( row=0; row<8; row++ ){
			for( col=0; col<64; col++ ){
				for( y=0; y<8; y++ ){
					int sy = row*9+y;
					UINT16 *dest = (UINT16 *)bitmap->line[sy];
					for( x=0; x<8; x+=2 ){
						if( col<4 ){
							int sx = col*9+x;
							UINT16 data = cpu_readmem32_word( source );
							dest[sx] = Machine->pens[data>>8];
							dest[sx+1] = Machine->pens[data&0xff];
						}
						source+=2;
					} /* next x */
				} /* next y */
			} /* next col */
		} /* next row */
	}

	if( keyboard_pressed( KEYCODE_X ) ){
		while( keyboard_pressed( KEYCODE_X ) ){}
		fscanf( gfx_in,
			"%08x %08x %04x %04x %04x %04x %04x %04x %04x\n",
			&source_baseaddr, &dest_baseaddr,
			&num_bytes,
			&param0,&param1,&param2,
			&param3,&param4,&param5
		);
		logerror( "Monkey = 0x%08x\n",source_baseaddr );
	}
}
#endif

int namcona1_vh_start( void ){
	int i;

#ifdef NAMCONA1_DEBUG
	namcona1_dump = fopen("blit.out", "w");
	gfx_in = fopen("blit.in", "r");
#endif

	for( i=0; i<4; i++ ){
		tilemap[i] = tilemap_create(
			tilemap_get_info,
			tilemap_scan_rows,
			TILEMAP_BITMASK,8,8,64,32 );

		if( tilemap[i]==NULL ) return 1; /* error */
		tilemap_palette_bank[i] = -1;
	}

	shaperam = malloc( CG_COUNT*8 );
	cgram = malloc( CG_COUNT*0x40 );
	dirtychar = malloc( CG_COUNT );

	if( shaperam && cgram && dirtychar ){
		struct GfxElement *gfx0 = decodegfx( cgram,&cg_layout );
		struct GfxElement *gfx1 = decodegfx( shaperam,&shape_layout );

		if( gfx0 && gfx1 ){
			gfx0->colortable = Machine->remapped_colortable;
			gfx0->total_colors = Machine->drv->color_table_len/256;
			Machine->gfx[0] = gfx0;

			gfx1->colortable = Machine->remapped_colortable;
			gfx1->total_colors = Machine->drv->color_table_len/2;
			Machine->gfx[1] = gfx1;

			return 0;
		}
	}

	free( dirtychar );
	free( cgram );
	free( shaperam );
	return -1; /* error */
}

void namcona1_vh_stop( void ){
	free( shaperam );
	free( cgram );
	free( dirtychar );

#ifdef NAMCONA1_DEBUG
	fclose( namcona1_dump );
	fclose( gfx_in );
#endif
}

/*************************************************************************/

static void pdraw_masked_tile(
		struct osd_bitmap *bitmap,
		int code,
		int color,
		int sx, int sy,
		int flipx, int flipy,
		int priority )
{
	/*
	**	custom blitter for drawing a masked 8x8x8BPP tile
	**	- doesn't yet handle screen orientation (needed particularly for F/A, a vertical game)
	**	- assumes there is an 8 pixel overdraw region on the screen for clipping
	*/
	const struct GfxElement *gfx = Machine->gfx[0];
	const struct GfxElement *mask = Machine->gfx[1];

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if( sx > -8 &&
		sy > -8 &&
		sx < bitmap->width &&
		sy < bitmap->height ) /* all-or-nothing clip */
	{
		const unsigned short *paldata = &gfx->colortable[gfx->color_granularity * color];
		UINT8 *gfx_addr = gfx->gfxdata + code * gfx->char_modulo;
		int gfx_pitch = gfx->line_modulo;

		UINT8 *mask_addr = mask->gfxdata + code * mask->char_modulo;
		int mask_pitch = mask->line_modulo;

		int x,y;

		if( Machine->color_depth == 8 ){
			for( y=0; y<8; y++ ){
				int ypos = sy+(flipy?7-y:y);
				UINT8 *pri = (UINT8 *)priority_bitmap->line[ypos];
				UINT8 *dest = bitmap->line[ypos];
				if( flipx ){
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ ){
						if( mask_addr[x] ){
							if( priority>=pri[-x] ) dest[-x] = paldata[gfx_addr[x]];
							pri[-x] = 0xff;
						}
					}
				}
				else {
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ ){
						if( mask_addr[x] ){
							if( priority>=pri[x] ) dest[x] = paldata[gfx_addr[x]];
							pri[x] = 0xff;
						}
					}
				}
				gfx_addr += gfx_pitch;
				mask_addr += mask_pitch;
			}
		}
		else { /* 16 bit color */
			for( y=0; y<8; y++ ){
				int ypos = sy+(flipy?7-y:y);
				UINT8 *pri = (UINT8 *)priority_bitmap->line[ypos];
				UINT16 *dest = (UINT16 *)bitmap->line[ypos];
				if( flipx ){
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ ){
						if( mask_addr[x] ){ /* sprite pixel is opaque */
							if( priority>=pri[-x] ) dest[-x] = paldata[gfx_addr[x]];
							pri[-x] = 0xff;
						}
					}
				}
				else {
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ ){
						if( mask_addr[x] ){ /* sprite pixel is opaque */
							if( priority>=pri[x] ) dest[x] = paldata[gfx_addr[x]];
							pri[x] = 0xff;
						}
					}
				}
				gfx_addr += gfx_pitch;
				mask_addr += mask_pitch;
			}
		}
	}
}

const UINT8 pri_mask[8] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f };

static void draw_sprites( struct osd_bitmap *bitmap ){
	int which;
	const UINT16 *source = (UINT16 *)spriteram;
	if( READ_WORD(&namcona1_vreg[0x22])&1 ) source += 0x400;

	for( which=0; which<0x100; which++ ){ /* 256 sprites */
		UINT16 ypos = source[0];	/* FHHH???? YYYYYYYY */
		UINT16 tile = source[1];	/* ????TTTT TTTTTTTT */
		UINT16 color = source[2];	/* F?WW???? CCCC?PPP */
		UINT16 xpos = source[3];	/* ???????X XXXXXXXX */

		int priority = pri_mask[color&0x7];
		int width = ((color>>12)&0x3)+1;
		int height = ((ypos>>12)&0x7)+1;
		int flipy = ypos&0x8000;
		int flipx = color&0x8000;
		int row,col;

		int skip = 0;
		if( keyboard_pressed( KEYCODE_T ) ){
			if( color&0x4000 ) skip = 1;
		}

		if( !skip )
		for( row=0; row<height; row++ ){
			int sy = (ypos&0x1ff)-30;
			if( flipy ){
				sy += (height-1-row)*8;
			}
			else {
				sy += row*8;
			}
			for( col=0; col<width; col++ ){
				int sx = (xpos&0x1ff)-10;
				if( flipx ){
					sx += (width-1-col)*8;
				}
				else {
					sx+=col*8;
				}
				pdraw_masked_tile(
					bitmap,
					tile + row*64+col,
					(color>>4)&0xf,
					((sx+16)&0x1ff)-8,
					((sy+8)&0x1ff)-8,
					flipx,flipy,
					priority
				);
			}
		}
		source += 4;
	}
}

static int count_scroll_rows( const UINT16 *source ){
	if( source[0x00]==0 ) return 0;
	if( source[0x1f]==0 ) return 1;
	if( source[0xff]==0 ) return 0x20;
	return 0x100;
}

static void draw_layerROZ( struct osd_bitmap *dest_bitmap)
{
#if 0
	/* this is copied from Namco SystemII, but not integrated, yet */

	const struct GfxElement *rozgfx=Machine->gfx[GFX_ROZ];
	int dest_x,dest_x_delta,dest_x_start,dest_x_end,tmp_x;
	int dest_y,dest_y_delta,dest_y_start,dest_y_end,tmp_y;
	int right_dx,right_dy,down_dx,down_dy,start_x,start_y;
	unsigned short *paldata;
	int colour;

	/* These need to be sign extended for arithmetic useage */
	down_dy  = namcos2_68k_roz_ctrl_r(0x00);
	right_dy = namcos2_68k_roz_ctrl_r(0x02);
	down_dx  = namcos2_68k_roz_ctrl_r(0x04);
	right_dx = namcos2_68k_roz_ctrl_r(0x06);
	start_x  = namcos2_68k_roz_ctrl_r(0x08);
	start_y  = namcos2_68k_roz_ctrl_r(0x0a);

	/* Sign extend the deltas */
	if(right_dx&0x8000) right_dx|=0xffff0000;
	if(right_dy&0x8000) right_dy|=0xffff0000;
	if(down_dx &0x8000) down_dx |=0xffff0000;
	if(down_dy &0x8000) down_dy |=0xffff0000;

	/* Correct to 16 bit fixed point from 8/12 bit */
	right_dx <<=8;
	right_dy <<=8;
	down_dx  <<=8;
	down_dy  <<=8;
	start_x  <<=12;
	start_y  <<=12;

	/* Correction factor is needed for the screen offset */
	start_x+=38*right_dx;
	start_y+=38*right_dy;

	/* Pre-calculate the colour palette array pointer */
	colour=(namcos2_68k_sprite_bank_r(0)>>8)&0x000f;
	paldata = &rozgfx->colortable[rozgfx->color_granularity * colour];

	/* Select correct drawing code based on destination bitmap pixel depth */

	if(dest_bitmap->depth == 16)
	{
		unsigned short *dest_line,pixel;

		if(Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int tmp;
			tmp=right_dx; right_dx=down_dx; down_dx=tmp;
			tmp=right_dy; right_dy=down_dy; down_dy=tmp;
		}

		dest_y_delta=(Machine->orientation & ORIENTATION_FLIP_Y)?-1:1;
		dest_y_start=(Machine->orientation & ORIENTATION_FLIP_Y)?dest_bitmap->height-1:0;
		dest_y_end  =(Machine->orientation & ORIENTATION_FLIP_Y)?-1:dest_bitmap->height;

		dest_x_delta=(Machine->orientation & ORIENTATION_FLIP_X)?-1:1;
		dest_x_start=(Machine->orientation & ORIENTATION_FLIP_X)?dest_bitmap->width-1:0;
		dest_x_end  =(Machine->orientation & ORIENTATION_FLIP_X)?-1:dest_bitmap->width;

		for( dest_y=dest_y_start; dest_y!=dest_y_end; dest_y+=dest_y_delta )
		{
			dest_line = (unsigned short*)dest_bitmap->line[dest_y];
			tmp_x = start_x;
			tmp_y = start_y;

			for( dest_x=dest_x_start; dest_x!=dest_x_end; dest_x+=dest_x_delta )
			{
				int xind=(tmp_x>>16)&ROTATE_MASK_HEIGHT;
				int yind=(tmp_y>>16)&ROTATE_MASK_WIDTH;

				pixel= fetch_rotated_pixel(xind,yind);

				/* Only process non-transparent pixels */
				if(pixel!=0xff)
				{
					/* Now remap the colour space of the pixel and store */
					dest_line[dest_x]=paldata[pixel];
				}
				tmp_x += right_dx;
				tmp_y += right_dy;
			}
			start_x += down_dx;
			start_y += down_dy;
		}
	}
	else
	{
		unsigned char *dest_line,pixel;

		if(Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int tmp;
			tmp=right_dx; right_dx=down_dx; down_dx=tmp;
			tmp=right_dy; right_dy=down_dy; down_dy=tmp;
		}

		dest_y_delta=(Machine->orientation & ORIENTATION_FLIP_Y)?-1:1;
		dest_y_start=(Machine->orientation & ORIENTATION_FLIP_Y)?dest_bitmap->height-1:0;
		dest_y_end  =(Machine->orientation & ORIENTATION_FLIP_Y)?-1:dest_bitmap->height;

		dest_x_delta=(Machine->orientation & ORIENTATION_FLIP_X)?-1:1;
		dest_x_start=(Machine->orientation & ORIENTATION_FLIP_X)?dest_bitmap->width-1:0;
		dest_x_end  =(Machine->orientation & ORIENTATION_FLIP_X)?-1:dest_bitmap->width;

		for( dest_y=dest_y_start; dest_y!=dest_y_end; dest_y+=dest_y_delta )
		{
			dest_line = dest_bitmap->line[dest_y];
			tmp_x = start_x;
			tmp_y = start_y;

			for( dest_x=dest_x_start; dest_x!=dest_x_end; dest_x+=dest_x_delta )
			{
				int xind=(tmp_x>>16)&ROTATE_MASK_HEIGHT;
				int yind=(tmp_y>>16)&ROTATE_MASK_WIDTH;

				pixel= fetch_rotated_pixel(xind,yind);

				/* Only process non-transparent pixels */
				if(pixel!=0xff)
				{
					/* Now remap the colour space of the pixel and store */
					dest_line[dest_x]=paldata[pixel];
				}
				tmp_x += right_dx;
				tmp_y += right_dy;
			}
			start_x += down_dx;
			start_y += down_dy;
		}
	}
#endif
}

void namcona1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	int i;
	int priority;
	const UINT16 *vreg = (UINT16 *)namcona1_vreg;
//	int flipscreen = READ_WORD( &namcona1_vreg[0x98] );
	/* cocktail not yet implemented */

	update_gfx();
	palette_init_used_colors();

	for( i=0; i<4; i++ ){
		const UINT16 *source = (UINT16 *)(namcona1_scroll+0x400*i);
		int scroll_rows, scrolly;

		if( cgang_hack() ){
			/* scrolling isn't well understood; this fixes Cosmo Gang */
			scroll_rows = 0;
			scrolly = 0x41e0;
		}
		else {
			scroll_rows = count_scroll_rows( source );
			scrolly = source[0x100];
		}

		tilemap_set_scrolly( tilemap[i], 0, scrolly-0x41e0 );

		if( scroll_rows ){
			int adjust = i*2+0x41c6;
			int line;
			tilemap_set_scroll_rows( tilemap[i], scroll_rows );
			for( line=0; line<scroll_rows; line++ ){
				int value = source[line]-adjust;
				tilemap_set_scrollx( tilemap[i], line, value );
			}
		}
		else {
			tilemap_set_scroll_rows( tilemap[i], 1 );
			tilemap_set_scrollx( tilemap[i], 0, -8 );
		}

		tilemap_videoram = i*0x1000+(UINT16 *)videoram;
		tilemap_color = vreg[0x58+i]&0xf;
		if( tilemap_color!=tilemap_palette_bank[i] ){
			tilemap_mark_all_tiles_dirty( tilemap[i] );
			tilemap_palette_bank[i] = tilemap_color;
		}
		tilemap_update( tilemap[i] );
	}

	if( palette_recalc() ) tilemap_mark_all_pixels_dirty( ALL_TILEMAPS );
	tilemap_render( ALL_TILEMAPS );

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap( bitmap,Machine->pens[0],&Machine->visible_area ); /* ? */

	for( priority = 0; priority<8; priority++ ){
		int which;
		for( which=0; which<4; which++ ){
			if( vreg[0x50+which] == priority ){
				tilemap_draw(
					bitmap,
					tilemap[which],
					pri_mask[priority]<<16
				);
			}
		}
	}

	draw_sprites( bitmap );

#ifdef NAMCONA1_DEBUG
	{
		static int show_vreg;
		if( keyboard_pressed( KEYCODE_N ) ){ /* debug toggle */
			while( keyboard_pressed( KEYCODE_N ) ){}
			show_vreg = !show_vreg;
		}
		if( show_vreg ){ /* onscreen display of video registers */
			for( i=0; i<0x80; i++ ){
				int sy = (i/8)*8;
				int sx = (i%8)*32;
				int data = READ_WORD( &namcona1_vreg[i*2] );
				int digit;
				for( digit=0; digit<4; digit++ ){
					drawgfx( bitmap, Machine->uifont,
						"0123456789abcdef"[data>>12],
						0,
						0,0,
						sx+digit*6,sy,
						&Machine->visible_area,TRANSPARENCY_NONE,0);
					data = (data<<4)&0xffff;
				}
			}
		}
		special( bitmap );
	}
#endif
}
