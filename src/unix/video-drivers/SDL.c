/***************************************************************************
                                          
 SDL XMAME display driver, based on
 Linux SVGALib adaptation by Phillip Ezolt.

 updated and patched by Ricardo Calixto Quesada (riq@ciudad.com.ar)

 TODO: Test the HERMES code.
       Make other update routines. The only one is 8bpp -> 16bpp 
       Improve performace.
   
***************************************************************************/
#define __SDL_C

/* #define SDL_DEBUG */
/* #define DIRECT_HERMES */

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"
#include "SDL-keytable.h"
#ifdef DIRECT_HERMES 
#include <Hermes/Hermes.h>
#endif /* DIRECT_HERMES */

static int Vid_width;
static int Vid_height;
static int Vid_depth = 8;
static SDL_Surface* Surface;
static SDL_Surface* Offscreen_surface;
static int hardware=1;
static int mode_number=-1;
static int list_modes=0;
SDL_Color *Colors;

#ifdef DIRECT_HERMES
HermesHandle   H_PaletteHandle;
HermesHandle H_ConverterHandle;
int32_t* H_Palette;
static int H_Palette_modified = 0;
#endif

typedef void (*update_func_t)(struct osd_bitmap *bitmap);

update_func_t update_function;

struct rc_option display_opts[] = {
    /* name, shortname, type, dest, deflt, min, max, func, help */
   { "SDL Related",  NULL,    rc_seperator,  NULL,
       NULL,         0,       0,             NULL,
       NULL },
   { "listmodes",    NULL,    rc_bool,    &list_modes,
      "0",           0,       0,             NULL,
      "List all posible full-screen modes" },
   { "modenumber",    NULL,   rc_int,        &mode_number,
      "-1",            0,      0,            NULL,
      "Try to use the 'n' possible full-screen mode" },
   { NULL,           NULL,    rc_end,        NULL,
      NULL,          0,       0,             NULL,
      NULL }
};

void list_sdl_modes(void);
void sdl_update_8bpp_16bpp(struct osd_bitmap *bitmap);

int sysdep_init(void)
{
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf (stderr, "SDL: Error: %s\n",SDL_GetError());
      return OSD_NOT_OK;
   } 
#ifdef DIRECT_HERMES
   Hermes_Init(0);
#endif /* DIRECT_HERMES */
   fprintf (stderr, "SDL: Info: SDL initialized\n");
   atexit (SDL_Quit);
   return OSD_OK;
}

void sysdep_close(void)
{
   SDL_Quit();
}

int sysdep_create_display(int depth)
{
   SDL_Rect** vid_modes;
   const SDL_VideoInfo* video_info;
   int vid_modes_i;
#ifdef DIRECT_HERMES 
   HermesFormat* H_src_format;
   HermesFormat* H_dst_format;
#endif /* DIRECT_HERMES */

   if(list_modes){
      list_sdl_modes();
      exit (OSD_OK);
   }

   video_info = SDL_GetVideoInfo();

#ifdef SDL_DEBUG
   fprintf (stderr,"SDL: Info: Best matching mode: \n");
   fprintf (stderr,"SDL: Info: HW blits %d\n"
      "SDL: Info: SW blits %d\n"
      "SDL: Info: Vid mem %d\n"
      "SDL: Info: Best supported depth %d\n",
      video_info->blit_hw,
      video_info->blit_sw,
      video_info->video_mem,
      video_info->vfmt->BitsPerPixel);
#endif

   Vid_depth = video_info->vfmt->BitsPerPixel;

   vid_modes = SDL_ListModes(NULL,SDL_HWSURFACE);
   vid_modes_i = 0;


   if ( (! vid_modes) || ((long)vid_modes == -1)) {
#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Possible all video modes available\n");
#endif
      hardware=0;
      Vid_height = visual_height*heightscale;
      Vid_width = visual_width*widthscale;
      update_function = &sdl_update_8bpp_16bpp;
   } else {
#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: visual w:%d visual h:%d\n", visual_width, visual_height);
#endif
      hardware=1;
      update_function = &sdl_update_8bpp_16bpp;

      while( *(vid_modes+vid_modes_i) ) {
#ifdef SDL_DEBUG
         fprintf (stderr, "SDL: Info: Found mode %d x %d\n",
            (*(vid_modes+vid_modes_i))->w,
            (*(vid_modes+vid_modes_i))->h);
#endif /* SDL_DEBUG */
   
         /* busco el que mejor se ajusta */
         if( ((*(vid_modes + vid_modes_i))->w < visual_width*widthscale) || ((*(vid_modes + vid_modes_i))->h < visual_height*heightscale)) {
            vid_modes_i--;
            break;
         } else
            vid_modes_i++;
      }

      /* mode_number is a command line option */
      if( mode_number != -1) {
         if( mode_number >vid_modes_i)
            fprintf(stderr, "SDL: The mode number is invalid... ignoring\n");
         else
            vid_modes_i = mode_number;
      }
      if( vid_modes_i<0 ) {
         fprintf(stderr, "SDL: None of the modes match :-(\n");
         Vid_height = visual_height*heightscale;
         Vid_width = visual_width*widthscale;
      } else {
         if(*(vid_modes+vid_modes_i)==NULL) 
            vid_modes_i=vid_modes_i-1;

         Vid_width = (*(vid_modes + vid_modes_i))->w;
         Vid_height = (*(vid_modes + vid_modes_i))->h;
      }
   }

   if(! (Surface = SDL_SetVideoMode(Vid_width, Vid_height,Vid_depth, SDL_HWSURFACE))) {
      fprintf (stderr, "SDL: Error: Setting video mode failed\n");
      SDL_Quit();
      exit (OSD_NOT_OK);
   } else {
      fprintf (stderr, "SDL: Info: Video mode set as %d x %d, depth %d\n", Vid_width, Vid_height, Vid_depth);
   }

#ifndef DIRECT_HERMES
   Offscreen_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,Vid_width,Vid_height,Vid_depth,0,0,0,0); 
   if(Offscreen_surface==NULL) {
      SDL_Quit();
      exit (OSD_NOT_OK);
   }
#else /* DIRECT_HERMES */
   /* No offscreen surface when using hermes directly */
   H_ConverterHandle = Hermes_ConverterInstance(0);
   H_src_format = Hermes_FormatNew (8,0,0,0,0,HERMES_INDEXED);
   /* TODO: More general destination choosing - uptil
       now only 16 bit */
   H_dst_format = Hermes_FormatNew (16,Surface->format->Rmask,Surface->format->Gmask,Surface->format->Bmask,0,0);
   //  H_dst_format = Hermes_FormatNew (16,5,5,5,0,0);
   if ( ! (Hermes_ConverterRequest(H_ConverterHandle,H_src_format , H_dst_format)) ) {
      fprintf (stderr_file, "Hermes: Info: Converter request failed\n");
      exit (OSD_NOT_OK);
   }
#endif /* DIRECT_HERMES */


   /* Creating event mask */
   SDL_EventState(SDL_KEYUP, SDL_ENABLE);
   SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
   SDL_EnableUNICODE(1);
   
    /* fill the display_palette_info struct */
    memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
    display_palette_info.depth = Vid_depth;
    if (Vid_depth == 8)
         display_palette_info.writable_colors = 256;
    else {
      display_palette_info.red_mask = 0xF800;
      display_palette_info.green_mask = 0x07E0;
      display_palette_info.blue_mask   = 0x001F;
   }

   return OSD_OK;
}


/* Update the display. */
void sdl_update_8bpp_16bpp(struct osd_bitmap *bitmap)
{
#define BLIT_16BPP_HACK
#define INDIRECT current_palette->lookup
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned short
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width

#include "blit.h"

#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
#undef BLIT_16BPP_HACK
}

#ifndef DIRECT_HERMES
void sysdep_update_display(struct osd_bitmap *bitmap)
{
   int old_use_dirty = use_dirty;
   SDL_Rect srect = { 0,0,0,0 };
   SDL_Rect drect = { 0,0,0,0 };
   srect.w = Vid_width;
   srect.h = Vid_height;
   drect.w = Vid_width;
   drect.h = Vid_height;

   if (current_palette->lookup_dirty)
      use_dirty = 0;
   
   (*update_function)(bitmap);

   
   if(SDL_BlitSurface (Offscreen_surface, &srect, Surface, &drect)<0) 
      fprintf (stderr,"SDL: Warn: Unsuccessful blitting\n");

   if(hardware==0)
      SDL_UpdateRects(Surface,1, &drect);
   use_dirty = old_use_dirty;
}
#else /* DIRECT_HERMES */
void sysdep_update_display(struct osd_bitmap *bitmap)
{
   int i,j,x,y,w,h;
   int locked =0 ;
   static int first_run = 1;
   int line_amount;

#ifdef SDL_DEBUG
   static int update_count = 0;
   static char* bak_bitmap;
   int corrected = 0;
   int debug = 0;
#endif /* SDL_DEBUG */

   if (H_Palette_modified) {
      Hermes_PaletteInvalidateCache(H_PaletteHandle);
      Hermes_ConverterPalette(H_ConverterHandle,H_PaletteHandle,0);
      H_Palette_modified = 0;
   }
   
#ifdef PANANOIC 
      memset(Offscreen_surface->pixels,'\0' ,Vid_height * Vid_width);
#endif 

   switch   (use_dirty) {
      long line_min;
      long line_max;
      long col_min;
      long col_amount;

      
#ifdef SDL_DEBUG
      int my_off;
#endif       
   case 0:
      /* Not using dirty */
      if (SDL_MUSTLOCK(Surface))
         SDL_LockSurface(Surface);
      
      Hermes_ConverterCopy (H_ConverterHandle, 
               bitmap->line[0] ,
               0, 0 , 
               Vid_width,Vid_height, bitmap->line[1] - bitmap->line[0],
               Surface->pixels, 
               0,0,
               Vid_width, Vid_height, Vid_width <<1 );
      
      SDL_UnlockSurface(Surface);
      SDL_UpdateRect(Surface,0,0,Vid_width,Vid_height);
      break;

   case 1:
      /* Algorithm:
          search through dirty & find max maximal polygon, which 
          we can get to clipping (don't know if 8x8 is enought)
      */
      osd_dirty_merge();
      
   case 2:
      h = (bitmap->height+7) >> 3; /* Divide by 8, up rounding */
      w = (bitmap->width +7) >> 3; /* Divide by 8, up rounding */
      
#ifdef PARANOIC
      /* Rechecking dirty correctness ...*/
      if ( (! first_run) && debug) {
         for (y=0;y<h;y++ ) {
            for (i=0;i<8;i++) {
               int line_off = ((y<<3) + i);
               for (x=0;x<w;x++) {
                  for (j=0;j<8;j++) {
                     int col_off = ((x<<3) + j);
                     if ( *(bak_bitmap + (line_off * (bitmap->line[1]- bitmap->line[0])) + col_off ) != *(*(bitmap->line + line_off) + col_off)) {
                        if (! dirty_blocks[y][x] ) {
                           printf ("Warn!!! Block should be dirty %d, %d, %d - correcting \n",y,x,i);
                           dirty_blocks[y][x] = 1;
                           dirty_lines[y]=1;
                           corrected = 1;
                        }
                     } 
                  }
               }
            }
         }
      } else {
         bak_bitmap = (void*)malloc(w<<3 * h<<3);
      }
      
      if (! corrected) {
         printf ("dirty ok\n");
      }
      
      first_run = 0;
      for (i = 0;i< bitmap->height;i++)
         memcpy(bak_bitmap + (bitmap->line[1] - bitmap->line[0])*i,*(bitmap->line+i),w<<3);
      
#endif /* PARANOIC */

      // #define dirty_lines old_dirty_lines    
      // #define dirty_blocks old_dirty_blocks
      
      for (y=0;y<h;y++) {
         if (dirty_lines[y]) {
            line_min = y<<3;
            line_max = line_min + 8;
            // old_dirty_lines[y]=1;
            for (x=0;x<w;x++) {
               if (dirty_blocks[y][x]) {
                  col_min = x<<3;
                  col_amount = 0;
                  do { 
                     col_amount++; 
                     dirty_blocks[y][x] = 0;
                     x++; 
                  } while (dirty_blocks[y][x]); 

                  dirty_blocks[y][x] = 0;
                  col_amount <<= 3;

                  line_amount = line_max - line_min;
                  /* Trying to use direct hermes library for fast blitting */
                  if (SDL_MUSTLOCK(Surface))
                     SDL_LockSurface(Surface);
               
                  Hermes_ConverterCopy (H_ConverterHandle, 
                     bitmap->line[0] ,
                     col_min, line_min , 
                     col_amount,line_amount, bitmap->line[1] - bitmap->line[0],
                     Surface->pixels, 
                     col_min, line_min, 
                     col_amount ,line_amount, Vid_width <<1 );
               
                  SDL_UnlockSurface(Surface);
                  SDL_UpdateRect(Surface,col_min,line_min,col_amount,line_amount);
               }
            }
            dirty_lines[y] = 0;
         }
      }
      
      /* Vector game .... */
      break;
      return ;
   }
   
   /* TODO - It's the real evil - better to use would be original 
       hermes routines */

#ifdef SDL_DEBUG
   update_count++;
#endif
}
#endif /* DIRECT_HERMES */

/* shut up the display */
void sysdep_display_close(void)
{
   SDL_FreeSurface(Offscreen_surface);
}

/*
 * In 8 bpp we should alloc pallete - some ancient people  
 * are still using 8bpp displays
 */
int sysdep_display_alloc_palette(int totalcolors)
{
   int ncolors;
   int i;
   ncolors = totalcolors;

   fprintf (stderr, "SDL: sysdep_display_alloc_palette();\n");

#ifndef DIRECT_HERMES
   Colors = (SDL_Color*) malloc (totalcolors * sizeof(SDL_Color));
   for (i=0;i<totalcolors;i++) {
      (Colors + i)->r = 0xFF;
      (Colors + i)->g = 0x00;
      (Colors + i)->b = 0x00;
   }
   SDL_SetColors (Offscreen_surface,Colors,0,totalcolors-1);
#else /* DIRECT_HERMES */
   H_PaletteHandle = Hermes_PaletteInstance();
   if ( !(H_Palette = Hermes_PaletteGet(H_PaletteHandle)) ) {
      fprintf (stderr_file, "Hermes: Info: PaletteHandle invalid");
      exit(OSD_NOT_OK);
   }
#endif /* DIRECT_HERMES */

   fprintf (stderr, "SDL: Info: Palette with %d colors allocated\n", totalcolors);
   return 0;
}

int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
   static int warned = 0;

#ifndef DIRECT_HERMES
   (Colors + pen)->r = red;
   (Colors + pen)->g = green;
   (Colors + pen)->b = blue;
   if ( (! SDL_SetColors(Offscreen_surface, Colors + pen, pen,1)) && (! warned)) {
      printf ("Color allocation failed, or > 8 bit display\n");
      warned = 0;
   }
#else /* DIRECT_HERMES */
   *(H_Palette + pen) = (red<<16) | ((green) <<8) | (blue );
   H_Palette_modified = 1; 
#endif 

#ifdef SDL_DEBUG
   fprintf(stderr, "STD: Debug: Pen %d modification: r %d, g %d, b, %d\n", pen, red,green,blue);
#endif /* SDL_DEBUG */
   return 0;
}



void sysdep_mouse_poll (void)
{
/*   fprintf(stderr,"sysdep_mouse_poll()\n"); */
}

/* Keyboard procs */
/* Lighting keyboard leds */
void sysdep_set_leds(int leds) 
{
}

void sysdep_update_keyboard() 
{
   struct keyboard_event kevent;
   SDL_Event event;
   
   if (Surface) {
      while(SDL_PollEvent(&event)) {
         kevent.press = 0;
         
         switch (event.type)
         {
            case SDL_KEYDOWN:
               kevent.press = 1;
            case SDL_KEYUP:
               kevent.scancode = klookup[event.key.keysym.sym];
               kevent.unicode = event.key.keysym.unicode;
               keyboard_register_event(&kevent);
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
               exit(OSD_OK);
               break;
            default:
#ifdef SDL_DEBUG
               fprintf(stderr, "SDL: Debug: Other event\n");
#endif /* SDL_DEBUG */
               break;
         }
      }
   }
}

/* funciones agregadas */
int sysdep_display_16bpp_capable(void)
{
   return (Vid_depth >=16);
}

void list_sdl_modes(void)
{
   SDL_Rect** vid_modes;
   int vid_modes_i;

   vid_modes = SDL_ListModes(NULL,SDL_HWSURFACE);
   vid_modes_i = 0;

   if ( (! vid_modes) || ((long)vid_modes == -1)) {
      printf("This option only works in a full-screen mode (eg: linux's framebuffer)\n");
      return;
   }

   printf("Modes availables:\n");

   while( *(vid_modes+vid_modes_i) ) {
      printf("\t%d) Mode %d x %d\n",
         vid_modes_i,
         (*(vid_modes+vid_modes_i))->w,
         (*(vid_modes+vid_modes_i))->h
         );
   
      vid_modes_i++;
   }
}
