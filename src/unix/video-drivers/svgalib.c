/***************************************************************************

 Linux SVGALib adaptation by Phillip Ezolt pe28+@andrew.cmu.edu
  
***************************************************************************/
#define __SVGALIB_C

#include <math.h>
#include <vga.h>
#include <vgagl.h>
#include "xmame.h"
#include "svgainput.h"
#include "effect.h"
#include "sysdep/sysdep_display.h"

static int startx, starty;
static int scaled_visual_width, scaled_visual_height;
static int update_function;
static unsigned char *video_mem;
static unsigned char *doublebuffer_buffer = NULL;
static int use_linear = 1;
static vga_modeinfo video_modeinfo;
static void svgalib_update_linear_16bpp(struct mame_bitmap *bitmap);
static void svgalib_update_gl_16bpp(struct mame_bitmap *bitmap);
static void svgalib_update_gl_doublebuf_16bpp(struct mame_bitmap *bitmap);

struct rc_option display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Svgalib Related", NULL, rc_seperator, NULL, NULL, 0,	0, NULL, NULL },
	{ "linear", NULL, rc_bool, &use_linear, "1", 0, 0, NULL, "Enable/disable use of linear framebuffer (fast)" },
	{ NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

struct sysdep_display_prop_struct sysdep_display_properties = { NULL, 0 };

typedef void (*update_func)(struct mame_bitmap *bitmap);

static update_func update_functions[3] = {
	svgalib_update_linear_16bpp,
	svgalib_update_gl_16bpp,
	svgalib_update_gl_doublebuf_16bpp
};

int sysdep_init(void)
{
	vga_init();

	if(svga_input_init())
		return OSD_NOT_OK;

	return OSD_OK;
}

void sysdep_close(void)
{
	svga_input_exit();

	/* close svgalib (again) just to be sure */
	vga_setmode(TEXT);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
	int i;
	int video_mode = -1;
	int score, best_score = 0;
	vga_modeinfo *my_modeinfo;

	for (i=1; (my_modeinfo=vga_getmodeinfo(i)); i++)
	{
		if(my_modeinfo->colors != 32768 &&
		   my_modeinfo->colors != 65536)
			continue;
		if (!vga_hasmode(i))
			continue;
		/* we only want modes which are a multiple of 8 width, due to alignment
		   issues */
		if (my_modeinfo->width & 7)
			continue;
		if (mode_disabled(my_modeinfo->width, my_modeinfo->height, depth))
			continue;
		score = mode_match(my_modeinfo->width, my_modeinfo->height);
		if (score && score >= best_score)
		{
			best_score = score;
			video_mode = i;
			video_modeinfo = *my_modeinfo;
		}
		fprintf(stderr_file, "Svgalib: Info: Found videomode %dx%dx%d\n",
				my_modeinfo->width, my_modeinfo->height, my_modeinfo->colors);
	}

	if (best_score == 0)
	{
		fprintf(stderr_file, "Svgalib: Couldn't find a suitable mode\n");
		return OSD_NOT_OK;
	}

	fprintf(stderr_file, "Svgalib: Info: Choose videomode %dx%dx%d\n",
			video_modeinfo.width, video_modeinfo.height, video_modeinfo.colors);

	mode_fix_aspect((double)(video_modeinfo.width)/video_modeinfo.height);
	scaled_visual_width  = visual_width  * widthscale;
	scaled_visual_height = yarbsize ? yarbsize : visual_height * heightscale;
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
		update_function=0;
		fprintf(stderr_file, "Svgalib: Info: Using a linear framebuffer to speed up\n");
	}
	else /* use gl funcs todo the updating */
	{
		gl_setcontextvga(video_mode);

		/* we might need the doublebuffer_buffer for 1x1 in 16bpp, since it
		   could be paletised */
		if((widthscale > 1) || (heightscale > 1) || yarbsize ||
		   effect || (video_real_depth==16))
		{
			update_function=2;
			doublebuffer_buffer = malloc(scaled_visual_width*scaled_visual_height*
					video_modeinfo.bytesperpixel);
			if (!doublebuffer_buffer)
			{
				fprintf(stderr_file, "Svgalib: Error: Couldn't allocate doublebuffer buffer\n");
				return OSD_NOT_OK;
			}
		}
		else
			update_function=1;
	}

	fprintf(stderr_file, "Using a mode with a resolution of %d x %d, starting at %d x %d\n",
			video_modeinfo.width, video_modeinfo.height, startx, starty);

	/* fill the display_palette_info struct */
	memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
	if (video_modeinfo.colors == 32768)
	{
		display_palette_info.red_mask   = 0x001F;
		display_palette_info.green_mask = 0x03E0;
		display_palette_info.blue_mask  = 0xEC00;
	}
	else
	{
		display_palette_info.red_mask   = 0xF800;
		display_palette_info.green_mask = 0x07E0;
		display_palette_info.blue_mask  = 0x001F;
	}

	effect_init2(depth, depth, scaled_visual_width);

	/* init input */
	if(svga_input_open(NULL, NULL))
		return OSD_NOT_OK;

	return OSD_OK;
}

/* shut up the display */
void sysdep_display_close(void)
{
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
void sysdep_update_display(struct mame_bitmap *bitmap)
{
	update_functions[update_function](bitmap);
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
