/***************************************************************************
                                          
 This is the SDL XMAME display driver.
 FIrst incarnation by Tadeusz Szczyrba <trevor@pik-nel.pl>,
 based on the Linux SVGALib adaptation by Phillip Ezolt.

 Adapted for a big video reorganisation by Hans de Goede,
 the text below is from before this reorganisation and in many
 ways no longer accurate!

 updated and patched by Ricardo Calixto Quesada (riq@core-sdi.com)

 patched by Patrice Mandin (pmandin@caramail.com)
  modified support for fullscreen modes using SDL and XFree 4
  added toggle fullscreen/windowed mode (Alt + Return)
  added title for the window
  hide mouse cursor in fullscreen mode
  added command line switch to start fullscreen or windowed
  modified the search for the best screen size (SDL modes are sorted by
    Y size)

 patched by Dan Scholnik (scholnik@ieee.org)
  added support for 32bpp XFree86 modes
  new update routines: 8->32bpp & 16->32bpp

 TODO: Test the HERMES code.
       Test the 16bpp->24bpp update routine
       Test the 16bpp->32bpp update routine
       Improve performance.
       Test mouse buttons (which games use them?)

***************************************************************************/
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL.h>
#include "keycodes.h"
#include "SDL-keytable.h"
#include "sysdep/sysdep_display_priv.h"

static int Vid_width;
static int Vid_height;
static int Vid_depth = 8;
static SDL_Surface* Surface;
static SDL_Surface* Offscreen_surface;
static int hardware=1;
static int doublebuf=0;
SDL_Color *Colors=NULL;
static int cursor_state; /* previous mouse cursor state */

blit_func update_function;

static int sdl_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option sysdep_display_opts[] = {
    /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
   { "SDL Related",  NULL,    rc_seperator,  NULL,
       NULL,         0,       0,             NULL,
       NULL },
   { "doublebuf",   NULL,    rc_bool,       &doublebuf,
      "0",           0,       0,             NULL,
      "Use double buffering to reduce flicker/tearing" },
   { "sdlmapkey",	"sdlmk",	rc_use_function,	NULL,
     NULL,		0,			0,		sdl_mapkey,
     "Set a specific key mapping, see xmamerc.dist" },
   { NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
   { NULL,           NULL,    rc_end,        NULL,
      NULL,          0,       0,             NULL,
      NULL }
};

int sysdep_display_init(void)
{
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf (stderr, "SDL: Error: %s\n",SDL_GetError());
      return 1;
   } 
   fprintf (stderr, "SDL: Info: SDL initialized\n");
   atexit (SDL_Quit);
   return 0;
}

void sysdep_close(void)
{
   SDL_Quit();
}

int sysdep_display_driver_open(int reopen)
{
  SDL_Rect** vid_modes;
  const SDL_VideoInfo* video_info;
  int i,j;
  int scaled_height = sysdep_display_params.yarbsize?
          sysdep_display_params.yarbsize:
          sysdep_display_params.height*sysdep_display_params.heightscale;
  int scaled_width  = sysdep_display_params.width *
          sysdep_display_params.widthscale;
  int depth, score, best_score = 0;
  int best_width, best_height;
  static int firsttime = 1;

#ifdef SDL_DEBUG
  fprintf (stderr,"SDL: Info: HW blits %d\n"
      "SDL: Info: SW blits %d\n"
      "SDL: Info: Vid mem %d\n"
      "SDL: Info: Best supported depth %d\n",
      video_info->blit_hw,
      video_info->blit_sw,
      video_info->video_mem,
      video_info->vfmt->BitsPerPixel);
#endif
   
  for (i=0; i<5; i++)
  {
    /* We can't ask SDL which pixel formats are available, so we
       try all well known formats + the format preferred by the SDL
       videodriver, also trying the preferred format will ensure that
       we try atleast one available format, so that we will always find
       some modes.
       
       The preferred format is tried last, so that if
       a just as good well known format is found this will be used instead of
       the preffered format. This is done because the effectcode only supports
       well know formats, so if we can choise we want a well known format. */
    switch (i)
    {
      case 0:
        /* rgb 555 */
        pixel_format.palette = NULL;
        pixel_format.BitsPerPixel = 15;
        pixel_format.BytesPerPixel = 2;
        pixel_format.Rmask  = 0x01F << 10;
        pixel_format.Gmask  = 0x01F << 5;
        pixel_format.Bmask  = 0x01F << 0;
        pixel_format.Rshift = 10;
        pixel_format.Gshift = 5;
        pixel_format.Bshift = 0;
        pixel_format.Rloss  = 3;
        pixel_format.Gloss  = 3;
        pixel_format.Bloss  = 3;
        pixel_format.colorkey = 0;
        pixel_format.alpha  = 255;
        depth = 15;
        break;
      case 1:
        /* rgb 565 */
        pixel_format.palette = NULL;
        pixel_format.BitsPerPixel = 16;
        pixel_format.BytesPerPixel = 2;
        pixel_format.Rmask  = 0x01F << 11;
        pixel_format.Gmask  = 0x02F << 5;
        pixel_format.Bmask  = 0x01F << 0;
        pixel_format.Rshift = 11;
        pixel_format.Gshift = 5;
        pixel_format.Bshift = 0;
        pixel_format.Rloss  = 3;
        pixel_format.Gloss  = 2;
        pixel_format.Bloss  = 3;
        pixel_format.colorkey = 0;
        pixel_format.alpha  = 255;
        depth = 16;
        break;
      case 2:
        /* rgb 888 packed*/
        pixel_format.palette = NULL;
        pixel_format.BitsPerPixel = 24;
        pixel_format.BytesPerPixel = 3;
        pixel_format.Rmask  = 0x0FF << 16;
        pixel_format.Gmask  = 0x0FF << 8;
        pixel_format.Bmask  = 0x0FF << 0;
        pixel_format.Rshift = 16;
        pixel_format.Gshift = 8;
        pixel_format.Bshift = 0;
        pixel_format.Rloss  = 0;
        pixel_format.Gloss  = 0;
        pixel_format.Bloss  = 0;
        pixel_format.colorkey = 0;
        pixel_format.alpha  = 255;
        depth = 24;
        break;
      case 3:
        /* rgb 888 sparse */
        pixel_format.palette = NULL;
        pixel_format.BitsPerPixel = 32;
        pixel_format.BytesPerPixel = 4;
        pixel_format.Rmask  = 0x0FF << 16;
        pixel_format.Gmask  = 0x0FF << 8;
        pixel_format.Bmask  = 0x0FF << 0;
        pixel_format.Rshift = 16;
        pixel_format.Gshift = 8;
        pixel_format.Bshift = 0;
        pixel_format.Rloss  = 0;
        pixel_format.Gloss  = 0;
        pixel_format.Bloss  = 0;
        pixel_format.colorkey = 0;
        pixel_format.alpha  = 255;
        depth = 24;
        break;
      case 4:
        video_info = SDL_GetVideoInfo();
        pixel_format = *(video_info->vfmt);
        if (pixel_format.palette || (pixel_format.BitsPerPixel <= 8))
          continue;
        depth = pixel_format.Rshift+pixel_format.Gshift+pixel_format.Bshift;
        break;
    }
    if (sysdep_display_params.fullscreen)
      vid_modes = SDL_ListModes(NULL, SDL_HWSURFACE|SDL_FULLSCREEN);
    else
      vid_modes = SDL_ListModes(&pixel_format, SDL_HWSURFACE);

    if(vid_modes == (SDL_Rect **)-1)
    {
      /* All resolutions available */
      score = mode_match(0, 0, depth, pixel_format.BytesPerPixel*8, 0);
      if (score > best_score)
      {
        base_score  = score;
        best_bpp    = pixel_format.BitsPerPixel;
        best_width  = scaled_width;
        best_height = scaled_height;
      }
    }
    else if (vid_modes)
    {
      for(j=0;vid_modes[j];j++)
      {
        /* assume dfb when there is a list of modes to choise from */
        score = mode_match(vid_modes[j]->w, vid_modes[j]->h, depth,
          pixel_format.BytesPerPixel*8, 1);
        if (score > best_score)
        {
          base_score  = score;
          best_bpp    = pixel_format.BitsPerPixel;
          best_width  = vid_modes[j]->w;
          best_height = vid_modes[j]->h;
        }
        if (firsttime)
          fprintf(stderr, "SDL found mode:%dx%dx%d\n", vid_modes[j]->w,
            vid_modes[j]->h, pixel_format.BitsPerPixel);
      }
    }
  }
  firsttime = 0;
  
  if (best_score == 0)
  {
    fprintf(stderr, "SDL Error: could not find a suitable mode\n");
    return 1;
  }
  
/* todo: fill palette info, get a blit func */  

  /* Set video mode according to flags */
  i = SDL_HWSURFACE;
  if (sysdep_display_params.fullscreen)
    i |= SDL_FULLSCREEN;
  if (doublebuf)
    i |= SDL_DOUBLEBUF;

  if(! (Surface = SDL_SetVideoMode(best_width, best_height, best_depth, i)))
  {
    fprintf (stderr, "SDL: Error: Setting video mode failed\n");
    return 1;
  }

  Offscreen_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,Vid_width,Vid_height,Vid_depth,0,0,0,0); 
  if(Offscreen_surface==NULL) {
     SDL_Quit();
     exit (1);
  }


  /* Creating event mask */
  SDL_EventState(SDL_KEYUP, SDL_ENABLE);
  SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
  SDL_EnableUNICODE(1);
  
   /* fill the display_palette_info struct */
   memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
   if (Vid_depth == 16) {
     display_palette_info.red_mask = 0xF800;
     display_palette_info.green_mask = 0x07E0;
     display_palette_info.blue_mask   = 0x001F;
   }
   else {
     display_palette_info.red_mask   = 0x00FF0000;
     display_palette_info.green_mask = 0x0000FF00;
     display_palette_info.blue_mask  = 0x000000FF;
   };

  /* Hide mouse cursor and save its previous status */
  cursor_state = SDL_ShowCursor(0);

  /* Set window title */
  SDL_WM_SetCaption(title, NULL);

  if (effect_open())
     exit (1);

  return 0;
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int sdl_mapkey(struct rc_option *option, const char *arg, int priority)
{
   unsigned int from, to;
   /* ultrix sscanf() requires explicit leading of 0x for hex numbers */
   if (sscanf(arg, "0x%x,0x%x", &from, &to) == 2)
   {
      /* perform tests */
      /* fprintf(stderr,"trying to map %x to %x\n", from, to); */
      if (from >= SDLK_FIRST && from < SDLK_LAST && to >= 0 && to <= 127)
      {
         klookup[from] = to;
	 return 0;
      }
      /* stderr isn't defined yet when we're called. */
      fprintf(stderr,"Invalid keymapping %s. Ignoring...\n", arg);
   }
   return 1;
}

/* Update routines */
void sdl_update_16_to_16bpp (struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width
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
#undef DEST
#undef DEST_WIDTH
#undef SRC_PIXEL
#undef DEST_PIXEL
}

void sysdep_update_display(struct mame_bitmap *bitmap)
{
   SDL_Rect srect = { 0,0,0,0 };
   SDL_Rect drect = { 0,0,0,0 };
   srect.w = Vid_width;
   srect.h = Vid_height;

   /* Center the display */
   drect.x = (Vid_width - visual_width*widthscale ) >> 1;
   drect.y = (Vid_height - visual_height*heightscale ) >> 1;

   drect.w = Vid_width;
   drect.h = Vid_height;

   (*update_function)(bitmap);

   
   if(SDL_BlitSurface (Offscreen_surface, &srect, Surface, &drect)<0) 
      fprintf (stderr,"SDL: Warn: Unsuccessful blitting\n");

   if (doublebuf == 1)
      SDL_Flip(Surface);
   else if(hardware==0)
      SDL_UpdateRects(Surface,1, &drect);
}

/* shut up the display */
void sysdep_display_close(void)
{
   effect_close();

   SDL_FreeSurface(Offscreen_surface);

   /* Restore cursor state */
   SDL_ShowCursor(cursor_state);
}

void sysdep_mouse_poll (void)
{
   int i;
   int x,y;
   Uint8 buttons;

   buttons = SDL_GetRelativeMouseState( &x, &y);
   mouse_data[0].deltas[0] = x;
   mouse_data[0].deltas[1] = y;
   for(i=0;i<MOUSE_BUTTONS;i++) {
      mouse_data[0].buttons[i] = buttons & (0x01 << i);
   }
}

/* Keyboard procs */
/* Lighting keyboard leds */
void sysdep_set_leds(int leds) 
{
}

void sysdep_update_keyboard() 
{
   struct xmame_keyboard_event kevent;
   SDL_Event event;

   if (Surface) {
      while(SDL_PollEvent(&event)) {
         kevent.press = 0;
         
         switch (event.type)
         {
            case SDL_KEYDOWN:
               kevent.press = 1;

               /* ALT-Enter: toggle fullscreen */
               if ( event.key.keysym.sym == SDLK_RETURN )
               {
                  if(event.key.keysym.mod & KMOD_ALT)
                     SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
               }

            case SDL_KEYUP:
               kevent.scancode = klookup[event.key.keysym.sym];
               kevent.unicode = event.key.keysym.unicode;
               xmame_keyboard_register_event(&kevent);
               if(!kevent.scancode)
                  fprintf (stderr, "Unknown symbol 0x%x\n",
                     event.key.keysym.sym);
#ifdef SDL_DEBUG
               fprintf (stderr, "Key %s %ssed\n",
                  SDL_GetKeyName(event.key.keysym.sym),
                  kevent.press? "pres":"relea");
#endif
               break;
            case SDL_QUIT:
               /* Shoult leave this to application */
               exit(0);
               break;

    	    case SDL_JOYAXISMOTION:   
               if (event.jaxis.which < JOY_AXES)
                  joy_data[event.jaxis.which].axis[event.jaxis.axis].val = event.jaxis.value;
#ifdef SDL_DEBUG
               fprintf (stderr,"Axis=%d,value=%d\n",event.jaxis.axis ,event.jaxis.value);
#endif
		break;
	    case SDL_JOYBUTTONDOWN:

	    case SDL_JOYBUTTONUP:
               if (event.jbutton.which < JOY_BUTTONS)
                  joy_data[event.jbutton.which].buttons[event.jbutton.button] = event.jbutton.state;
#ifdef SDL_DEBUG
               fprintf (stderr, "Button=%d,value=%d\n",event.jbutton.button ,event.jbutton.state);
#endif
		break;


            default:
#ifdef SDL_DEBUG
               fprintf(stderr, "SDL: Debug: Other event\n");
#endif /* SDL_DEBUG */
               break;
         }
    joy_evaluate_moves ();
      }
   }
}
