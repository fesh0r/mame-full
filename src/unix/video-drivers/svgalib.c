/***************************************************************************

 Linux SVGALib adaptation by Phillip Ezolt pe28+@andrew.cmu.edu
  
***************************************************************************/
#define __SVGALIB_C

#include <math.h>
#include <vga.h>
#include <vgagl.h>
#include "svgainput.h"
#include "sysdep/sysdep_display_priv.h"

static int startx, starty;
static int scaled_visual_width, scaled_visual_height;
static int update_type;
static blit_func_p blit_func;
static unsigned char *video_mem;
static unsigned char *doublebuffer_buffer = NULL;
static int use_linear = 1;
static vga_modeinfo video_modeinfo;

struct rc_option display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
	{ "Svgalib Related", NULL, rc_seperator, NULL, NULL, 0,	0, NULL, NULL },
	{ "linear", NULL, rc_bool, &use_linear, "1", 0, 0, NULL, "Enable/disable use of linear framebuffer (fast)" },
	{ NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

int sysdep_display_init(void)
{
	memset(sysdep_display_properties.mode, 0,
	  SYSDEP_DISPLAY_VIDEO_MODES * sizeof(int));
        sysdep_display_properties.mode[0] =
          SYSDEP_DISPLAY_FULLSCREEN | SYSDEP_DISPLAY_EFFECTS;

        if (vga_init())
            return 1;

	return svga_input_init();
}

void sysdep_display_exit(void)
{
	svga_input_exit();

	/* close svgalib (again) just to be sure */
	vga_setmode(TEXT);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_display_open(const struct sysdep_display_open_params *params)
{
	int i;
	int video_mode = -1;
	int score, best_score = 0;
	vga_modeinfo *my_modeinfo;

	sysdep_display_set_params(params);

	for (i=1; (my_modeinfo=vga_getmodeinfo(i)); i++)
	{
		if (!vga_hasmode(i))
			continue;
			
		switch(my_modeinfo->colors)
		{
		  case 32768:
		    i = 15;
		    break;
		  case 65536:
		    i = 16;
		    break;
		  case 16777216:
		    i = 32;
		    break;
		  default:
		    continue;
		}
		score = mode_match(my_modeinfo->width, my_modeinfo->height, i);
		if (score && score >= best_score)
		{
			best_score = score;
			video_mode = i;
			video_modeinfo = *my_modeinfo;
		}
		fprintf(stderr, "Svgalib: Info: Found videomode %dx%dx%d\n",
				my_modeinfo->width, my_modeinfo->height, my_modeinfo->colors);
	}

	if (best_score == 0)
	{
		fprintf(stderr, "Svgalib: Couldn't find a suitable mode\n");
		return 1;
	}

	fprintf(stderr, "Svgalib: Info: Choose videomode %dx%dx%d\n",
			video_modeinfo.width, video_modeinfo.height, video_modeinfo.colors);

	mode_set_aspect_ratio((double)(video_modeinfo.width)/video_modeinfo.height);
	scaled_visual_width  = sysdep_display_params.width * sysdep_display_params.widthscale;
	scaled_visual_height = sysdep_display_params.yarbsize;
	startx = ((video_modeinfo.width  - scaled_visual_width ) / 2) & ~7;
	starty =  (video_modeinfo.height - scaled_visual_height) / 2;

	vga_setmode(video_mode);

	/* do we have a linear framebuffer ? */
	i = video_modeinfo.width * video_modeinfo.height *
		video_modeinfo.bytesperpixel;
	if (i <= 65536 || 
		(video_modeinfo.flags & IS_LINEAR) ||
		(use_linear && (video_modeinfo.flags && CAPABLE_LINEAR) &&
		 vga_setlinearaddressing() >=  i))
	{
		video_mem  = vga_getgraphmem();
		video_mem += startx * video_modeinfo.bytesperpixel;
		video_mem += starty * video_modeinfo.width *
			video_modeinfo.bytesperpixel;
		update_type=0;
		fprintf(stderr, "Svgalib: Info: Using a linear framebuffer to speed up\n");
	}
	else /* use gl funcs todo the updating */
	{
		gl_setcontextvga(video_mode);

			scaled_visual_height*video_modeinfo.bytesperpixel);
		if (!doublebuffer_buffer)
		{
			fprintf(stderr, "Svgalib: Error: Couldn't allocate doublebuffer buffer\n");
			return 1;
		}
		
		/* do we need to blit to a doublebuffer buffer when using
		   effects or scaling */
		if(effect || (sysdep_display_params.widthscale > 1) ||
		   (sysdep_display_params.yarbsize!=sysdep_display_params.height))
			update_type=2;
		else
			update_type=1;
	}

	fprintf(stderr, "Using a mode with a resolution of %d x %d, starting at %d x %d\n",
			video_modeinfo.width, video_modeinfo.height, startx, starty);

	/* fill the sysdep_display_properties struct */
	sysdep_display_properties.palette_info.fourcc_format = 0;
	switch(video_modeinfo.colors)
	{
	  case 32768:
	    sysdep_display_properties.palette_info.red_mask   = 0x001F;
            sysdep_display_properties.palette_info.green_mask = 0x03E0;
            sysdep_display_properties.palette_info.blue_mask  = 0xEC00;
            break;
          case 65536:
            sysdep_display_properties.palette_info.red_mask   = 0xF800;
            sysdep_display_properties.palette_info.green_mask = 0x07E0;
            sysdep_display_properties.palette_info.blue_mask  = 0x001F;
            break;
          case 16777216:
            sysdep_display_properties.palette_info.red_mask   = 0xFF0000;
            sysdep_display_properties.palette_info.green_mask = 0x00FF00;
            sysdep_display_properties.palette_info.blue_mask  = 0x0000FF;
            break;
	}
	sysdep_display_properties.vector_renderer = NULL;

	/* get a blit func */
	blit_func = sysdep_display_get_blitfunc_doublebuffer(video_modeinfo.bytesperpixel*8);
	if (blit_func == NULL)
	{
		fprintf(stderr, "unsupported bpp: %dbpp\n", video_modeinfo.bytesperpixel*8);
		return 1;
	}

	/* init input */
	if(svga_input_open(NULL, NULL))
		return 1;

	return effect_open();
}

/* shut up the display */
void sysdep_display_close(void)
{
    	effect_close();

	/* close input */
	svga_input_close();

	/* close svgalib */
	vga_setmode(TEXT);

	/* and don't forget to free our other resources */
	if (doublebuffer_buffer)
	{
		free(doublebuffer_buffer);
		doublebuffer_buffer = NULL;
	}
}

/* Update the display. */
int sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_area, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags)
{
  switch(video_update_type)
  {
    case 0: /* linear */
      break;
    case 1: /* gl no scaling/effect/palette lookup/depthconversion */
      break;
    case 2: /* gl scaling/effect/palette lookup/depthconversion */
      break;
  }
}

static void svgalib_update_linear_16bpp(struct mame_bitmap *bitmap)
{
	int linewidth = video_modeinfo.linewidth/2;
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST video_mem
#define DEST_WIDTH linewidth
#define DOUBLEBUFFER
	if(current_palette->lookup)
	{
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
	}
	else
	{
#include "blit.h"
	}
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef DEST
#undef DEST_WIDTH
#undef DOUBLEBUFFER
}

static void svgalib_update_gl_16bpp(struct mame_bitmap *bitmap)
{
	/* Note: we calculate the real bitmap->width, as used in
	   osd_create_bitmap, this fixes the skewing bug in tempest
	   & others */
	int bitmap_linewidth = ((unsigned char *)bitmap->line[1] - (unsigned char *)bitmap->line[0]) / video_modeinfo.bytesperpixel;
	gl_putboxpart(	startx, starty,
			scaled_visual_width, scaled_visual_height,
			bitmap_linewidth, bitmap->height,
			bitmap->line[0], visual.min_x, visual.min_y);
}

static void svgalib_update_gl_doublebuf_16bpp(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
	gl_putboxpart( \
			startx + X, starty + Y, \
			WIDTH, HEIGHT, \
			scaled_visual_width, scaled_visual_height, \
			doublebuffer_buffer, X, Y );
	if(current_palette->lookup)
	{
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
	}
	else
	{
#include "blit.h"
	}
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef DEST
#undef DEST_WIDTH
#undef PUT_IMAGE
}
