#include "xmame.h"
#include "driver.h"
#include <math.h>

static int disabled_modes_count = 0;
static int perfect_aspect = 0;

static int mode_disable(struct rc_option *option, const char *s, int priority);

struct rc_option mode_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Mode Selection Related", NULL,	rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "keepaspect",	"ka",			rc_bool,	&normal_use_aspect_ratio,
     "1",		0,			0,		NULL,
     "Try / don't try to keep the aspect ratio of a game when selecting the best videomode" },
   { "perfectaspect",   "pa",                   rc_bool,        &perfect_aspect,
     "0",		0,			0,		NULL,
     "Automaticly set yarbsize to get the perfect aspect ratio" },
   { "displayaspectratio", "dar",		rc_float,	&display_aspect_ratio,
     "1.3333333333333",	0.5,			2.0,		NULL,
     "Set the display aspect ratio of your monitor. This is used for -keepaspect The default = 1.33 (4/3). Use 0.75 (3/4) for a portrait monitor" },
   { "disablemode",	"dm",			rc_use_function, NULL,
     NULL,		0,			0,		mode_disable,
     "Don't use mode XRESxYRESxDEPTH this can be used to disable specific video modes which don't work on your system. The xDEPTH part of the string is optional. This option may be used more then once" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#define MODE_DISABLED_MAX 32

static struct 
{
   int width;
   int height;
   int depth;
} disabled_modes[MODE_DISABLED_MAX];

static int mode_disable(struct rc_option *option, const char *s, int priority)
{
   if (disabled_modes_count == MODE_DISABLED_MAX)
   {
      /* stderr_file doesn't have a valid value yet when we're called ! */
      fprintf(stderr, "OSD: Warning: You can't disable more then %d modes. Mode %s not disabled\n",
          MODE_DISABLED_MAX, s);
      return OSD_OK;
   }
   if (sscanf(s, "%dx%dx%d",
       &disabled_modes[disabled_modes_count].width,
       &disabled_modes[disabled_modes_count].height,
       &disabled_modes[disabled_modes_count].depth) < 2)
      return OSD_NOT_OK;
   switch (disabled_modes[disabled_modes_count].depth)
   {
      case 0:
      case 256:
      case 65536:
         break;
      default:
         /* stderr_file doesn't have a valid value yet when we're called ! */
         fprintf(stderr, "Svgalib: Warning: No such depth: %d. Mode not disabled\n",
            disabled_modes[disabled_modes_count].depth);
         return OSD_NOT_OK;
   }
   disabled_modes_count++;
   return OSD_OK;
}

int mode_disabled(int width, int height, int depth)
{
   int i;
   for(i=0; i<disabled_modes_count; i++)
   {
      if (disabled_modes[i].width  == width &&
          disabled_modes[i].height == height)
      {
         switch (disabled_modes[disabled_modes_count].depth)
         {
            case 0:
               return TRUE;
            case 256:
               if(depth == 8)
                  return TRUE;
               break;
            case 65536:
               if(depth == 16)
                  return TRUE;
               break;
         }
      }
   }
   return FALSE;
}

/* match a given mode to the needed width, height and aspect ratio to
   perfectly display a game.
   This function returns 0 for a not usable mode and 100 for the perfect mode.
*/
int mode_match(int width, int height)
{
  int viswidth, visheight;
  double perfect_width, perfect_height, perfect_aspect;
  double aspect = (double)width / height;
  static int first_time = TRUE;
  
  /* setup yarbsize to get the aspect right */
  mode_fix_aspect(aspect);
  visheight  = yarbsize ? yarbsize : (visual_height * heightscale);
  viswidth   = visual_width  * widthscale;
  
  /* does the game fit at all ? */
  if(width  < viswidth ||
     height < visheight)
    return 0;
   
  if (use_aspect_ratio)
  {
    /* first of all calculate the pixel aspect_ratio the game has */
    double pixel_aspect_ratio = viswidth / (visheight * aspect_ratio);

    perfect_width  = display_aspect_ratio * pixel_aspect_ratio * visheight;
    perfect_height = visheight;
         
    if (perfect_width < viswidth)
    {
      perfect_height *= viswidth / perfect_width;
      perfect_width   = viswidth;
    }

    if (first_time)
    {
      fprintf(stderr_file, "OSD: Info: Ideal mode for this game = %.0fx%.0f\n",
         perfect_width, perfect_height);
      first_time = FALSE;
    }
    perfect_aspect = perfect_width/perfect_height;
  }
  else
  {
    perfect_width  = viswidth;
    perfect_height = visheight;
    perfect_aspect = aspect;
  }

  return ( 100 *
    (perfect_width  / (fabs(width -perfect_width )+perfect_width )) *
    (perfect_height / (fabs(height-perfect_height)+perfect_height)) *
    (perfect_aspect / (fabs(aspect-perfect_aspect)+perfect_aspect)));
}

/* calculate a virtual screen contained within the given dimensions
   which will give the game the correct aspect ratio */
void mode_clip_aspect(int width, int height, int *corr_width, int *corr_height,
  double display_resolution_aspect_ratio)
{
  double ch, cw;

  ch = height;
  
  if (use_aspect_ratio)
  {
    cw = height * aspect_ratio *
      (display_resolution_aspect_ratio/display_aspect_ratio);
    if ((int)(cw + 0.5) > width )
    {
      ch *= width / cw;
      cw  = width;
    }
  }
  else
    cw = width;
    
  *corr_width  = cw + 0.5;
  *corr_height = ch + 0.5;
}

/* calculate a screen with at least the given dimensions
   which will give the game the correct aspect ratio */
void mode_stretch_aspect(int width, int height, int *corr_width, int *corr_height,
  double display_resolution_aspect_ratio)
{
  double ch, cw;

  ch = height;
  
  if (use_aspect_ratio)
  {
    cw = height * aspect_ratio *
      (display_resolution_aspect_ratio/display_aspect_ratio);
    if ((int)(cw+0.5) < width)
    {
      ch *= width / cw;
      cw  = width;
    }
  }
  else
    cw = width;
  
  *corr_width  = cw + 0.5;
  *corr_height = ch + 0.5;
}

void mode_fix_aspect(double display_resolution_aspect_ratio)
{
  if(!use_aspect_ratio || !perfect_aspect)
    return;
    
  yarbsize = (visual_width * widthscale) / 
    (aspect_ratio * (display_resolution_aspect_ratio/display_aspect_ratio));
}
