/***************************************************************************

 Linux SVGALib adaptation by Phillip Ezolt pe28+@andrew.cmu.edu
  
***************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgagl.h>
#include "svgainput.h"
#include "sysdep/sysdep_display_priv.h"

static int startx, starty;
static int scaled_width, scaled_height;
static int video_update_type;
static blit_func_p blit_func;
static unsigned char *video_mem;
static unsigned char *doublebuffer_buffer = NULL;
static int use_linear = 1;
static vga_modeinfo video_modeinfo;

struct rc_option sysdep_display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
	{ "Svgalib Related", NULL, rc_seperator, NULL, NULL, 0,	0, NULL, NULL },
	{ "linear", NULL, rc_bool, &use_linear, "1", 0, 0, NULL, "Enable/disable use of linear framebuffer (fast)" },
	{ NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

int sysdep_display_init(void)
{
	memset(sysdep_display_properties.mode_info, 0,
	  SYSDEP_DISPLAY_VIDEO_MODES * sizeof(int));
        sysdep_display_properties.mode_info[0] =
          SYSDEP_DISPLAY_FULLSCREEN | SYSDEP_DISPLAY_EFFECTS;
        memset(sysdep_display_properties.mode_name, 0,
          SYSDEP_DISPLAY_VIDEO_MODES * sizeof(const char *));
        sysdep_display_properties.mode_name[0] = "SvgaLib";

        if (vga_init())
            return 1;

	return svga_input_init();
}

void sysdep_display_exit(void)
{
	svga_input_exit();
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_display_driver_open(int reopen)
{
	int i;
	int video_mode = -1;
	int score, best_score = 0;
	int depth, bpp;
	vga_modeinfo *my_modeinfo;
	static int firsttime = 1;
	
	if (reopen)
	{
	  sysdep_display_effect_close();
          if (doublebuffer_buffer)
          {
                  free(doublebuffer_buffer);
                  doublebuffer_buffer = NULL;
          }
	}

	/* Find a suitable mode, also determine the max size of the display */
	sysdep_display_properties.max_width  = 0;
	sysdep_display_properties.max_height = 0;

	for (i=1; (my_modeinfo=vga_getmodeinfo(i)); i++)
	{
		if (!vga_hasmode(i))
			continue;
			
		switch(my_modeinfo->colors)
		{
		  case 32768:
		    depth = 15;
		    bpp   = 16;
		    break;
		  case 65536:
		    depth = 16;
		    bpp   = 16;
		    break;
		  case 16777216:
		    depth = 24;
		    bpp   = my_modeinfo->bytesperpixel*8;
		    break;
		  default:
		    continue;
		}
		score = mode_match(my_modeinfo->width, my_modeinfo->height,
		  depth, bpp, 1);
		if (score > best_score)
		{
			best_score = score;
			video_mode = i;
			video_modeinfo = *my_modeinfo;
		}
		/* also determine the max size of the display */
		if (my_modeinfo->width  > sysdep_display_properties.max_width)
		  sysdep_display_properties.max_width  = my_modeinfo->width;
		if (my_modeinfo->height > sysdep_display_properties.max_height)
		  sysdep_display_properties.max_height = my_modeinfo->height;

                if (firsttime)
		  fprintf(stderr, "Svgalib: Info: Found videomode %dx%dx%d\n",
		    my_modeinfo->width, my_modeinfo->height,
		    my_modeinfo->colors);
	}
	firsttime = 0;

	if (best_score == 0)
	{
		fprintf(stderr, "Svgalib: Couldn't find a suitable mode\n");
		return 1;
	}

	fprintf(stderr, "Using videomode %dx%dx%d, starting at %dx%d\n",
			video_modeinfo.width, video_modeinfo.height, video_modeinfo.colors,
			startx, starty);

	/* fill the sysdep_display_properties struct */
	sysdep_display_properties.palette_info.fourcc_format = 0;
	switch(video_modeinfo.colors)
	{
	  case 32768:
	    sysdep_display_properties.palette_info.red_mask   = 0x001F;
            sysdep_display_properties.palette_info.green_mask = 0x03E0;
            sysdep_display_properties.palette_info.blue_mask  = 0xEC00;
            sysdep_display_properties.palette_info.depth      = 15;
            sysdep_display_properties.palette_info.bpp        = 16;
            break;
          case 65536:
            sysdep_display_properties.palette_info.red_mask   = 0xF800;
            sysdep_display_properties.palette_info.green_mask = 0x07E0;
            sysdep_display_properties.palette_info.blue_mask  = 0x001F;
            sysdep_display_properties.palette_info.depth      = 16;
            sysdep_display_properties.palette_info.bpp        = 16;
            break;
          case 16777216:
            sysdep_display_properties.palette_info.red_mask   = 0xFF0000;
            sysdep_display_properties.palette_info.green_mask = 0x00FF00;
            sysdep_display_properties.palette_info.blue_mask  = 0x0000FF;
            sysdep_display_properties.palette_info.depth      = 24;
            sysdep_display_properties.palette_info.bpp        = 
              video_modeinfo.bytesperpixel*8;
            break;
	}
	sysdep_display_properties.vector_renderer = NULL;

	/* get a blit func */
	blit_func = sysdep_display_get_blitfunc_dfb();
	if (blit_func == NULL)
	{
		fprintf(stderr, "Error: unsupported dept/bpp: %d/%dbpp\n",
		  sysdep_display_properties.palette_info.depth,
		  sysdep_display_properties.palette_info.bpp);
		return 1;
	}

	mode_set_aspect_ratio((double)(video_modeinfo.width)/video_modeinfo.height);
        scaled_width  = sysdep_display_params.width * sysdep_display_params.widthscale;
        scaled_height = sysdep_display_params.yarbsize? sysdep_display_params.yarbsize:
          sysdep_display_params.height * sysdep_display_params.heightscale;
	startx = ((video_modeinfo.width  - scaled_width ) / 2) & ~7;
	starty =  (video_modeinfo.height - scaled_height) / 2;

	vga_setmode(video_mode);

	/* do we have a linear framebuffer ? */
	i = video_modeinfo.width * video_modeinfo.height *
          video_modeinfo.bytesperpixel;
	if ((video_modeinfo.flags & IS_LINEAR) ||
	    (use_linear && (video_modeinfo.flags && CAPABLE_LINEAR) &&
             vga_setlinearaddressing() >= i))
	{
          video_mem  = vga_getgraphmem();
          video_mem += startx * video_modeinfo.bytesperpixel;
          video_mem += starty * video_modeinfo.width *
                  video_modeinfo.bytesperpixel;
          video_update_type=0;
          fprintf(stderr, "Svgalib: Info: Using a linear framebuffer to speed up\n");
	}
	else /* use gl funcs todo the updating */
	{
	  gl_setcontextvga(video_mode);
          /* do we need to blit to a doublebuffer buffer because using
             effects, scaling etc. */
	  if (!sysdep_display_blit_dest_bitmap_equals_src_bitmap())
	  {
	    doublebuffer_buffer = malloc(scaled_width*scaled_height*
	      video_modeinfo.bytesperpixel);
            if (!doublebuffer_buffer)
            {
                    fprintf(stderr, "Svgalib: Error: Couldn't allocate doublebuffer buffer\n");
                    return 1;
            }
	    video_update_type=2;
          }
	  else
	    video_update_type=1;
	}

	/* init input */
	if (!reopen)
	{
	  if(svga_input_open(NULL, NULL))
	    return 1;
        }

	return sysdep_display_effect_open();
}

/* shut up the display */
void sysdep_display_close(void)
{
    	sysdep_display_effect_close();

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
const char *sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, int keyb_leds, int flags)
{
  svga_input_set_keybleds(keyb_leds);
  switch(video_update_type)
  {
    case 0: /* linear */
      blit_func(bitmap, vis_in_dest_out, dirty_area, palette, video_mem,
        video_modeinfo.linewidth/video_modeinfo.bytesperpixel);
      break;
    case 1: /* gl bitmap equals framebuffer */
      /* Note: we calculate the real bitmap->width, as used in
         osd_create_bitmap, this fixes the skewing bug in tempest
         & others */
      sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area, 3);
      gl_putboxpart(
        startx + vis_in_dest_out->min_x,
        starty + vis_in_dest_out->min_y,
        (vis_in_dest_out->max_x + 1) - vis_in_dest_out->min_x,
        (vis_in_dest_out->max_y + 1) - vis_in_dest_out->min_y,
        ((unsigned char *)bitmap->line[1] - (unsigned char *)bitmap->line[0]) / 
          video_modeinfo.bytesperpixel,
        bitmap->height, bitmap->line[0], dirty_area->min_x, dirty_area->min_y);
      break;
    case 2: /* gl bitmap needs conversion before it can be blitted */
      blit_func(bitmap, vis_in_dest_out, dirty_area, palette,
        doublebuffer_buffer, scaled_width);
      gl_putboxpart(
        startx + vis_in_dest_out->min_x,
        starty + vis_in_dest_out->min_y,
        (vis_in_dest_out->max_x + 1) - vis_in_dest_out->min_x,
        (vis_in_dest_out->max_y + 1) - vis_in_dest_out->min_y,
        scaled_width, scaled_height, doublebuffer_buffer,
        dirty_area->min_x, dirty_area->min_y);
      break;
  }
  return NULL;
}

void sysdep_display_driver_clear_buffer(void)
{
  int line;
  switch(video_update_type)
  {
    case 0: /* linear */
      for (line=0; line<scaled_height; line++)
        memset(video_mem +
          line*video_modeinfo.linewidth/video_modeinfo.bytesperpixel, 0,
          video_modeinfo.linewidth/video_modeinfo.bytesperpixel);
      break;
    case 1: /* gl bitmap equals framebuffer */
      gl_fillbox(startx, starty, scaled_width, scaled_height, 0);
      break;
    case 2: /* gl bitmap needs conversion before it can be blitted */
      memset(doublebuffer_buffer, 0,
        scaled_width * scaled_height * video_modeinfo.bytesperpixel);
      break;
  }
}
