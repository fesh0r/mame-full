/* First try to get debugging to work under UNIX */

#include "xmame.h"
#include "osd_dbg.h"

#ifdef MAME_DEBUG
#include <curses.h>
#include <term.h>

static int cursor_x = 0;
static int cursor_y = 0;
static int curses_initialised = FALSE;
static int have_color;

static const int unix_color[8] =
{
   COLOR_BLACK,
   COLOR_BLUE,
   COLOR_GREEN,
   COLOR_CYAN,
   COLOR_RED,
   COLOR_MAGENTA,
   COLOR_YELLOW,
   COLOR_WHITE
};

static void sysdep_debug_real_init(void)
{
   int i;
      
   initscr ();
   noecho ();

   if (has_colors())
   {
      start_color ();

      /* we skip colorpair 0 since it's hardcoded as white on black into curses */
      /* this isn't a problem since it's unlikely anyone wants black on black */
      for (i = 1; i < 64; i++)
         init_pair (i, unix_color[i&0x07], unix_color[i>>3]);
      have_color = TRUE;
   }
   else
   {
      have_color = FALSE;
   }
      
   curses_initialised = TRUE;
}

#endif /* ifdef MAME_DEBUG */

int osd_debug_init(void)
{
   /* nothing done here instead it's done on the first call to a debug function,
      to fix the staircasing of the boot messages */
   /* svgalib however seems to not like curses being initialsed after it
      <sigh> */
#if defined MAME_DEBUG && defined svgalib
   sysdep_debug_real_init();
#endif
   return OSD_OK;
}

void osd_debug_close (void)
{
#ifdef MAME_DEBUG
   if (curses_initialised) endwin();
#endif
}


#ifdef MAME_DEBUG

/* Switch to text mode, if possible create this mode at width x height.
   Used to switch to textmode in the dosport.
*/
void osd_set_screen_size (unsigned width, unsigned height)
{
   sysdep_set_text_mode();
}

/* Since we cannot garantee the requested screensize above is set, this function
   is used to get the real screensize */
void osd_get_screen_size (unsigned *width, unsigned *height)
{
   if(!curses_initialised)
   {
      sysdep_debug_real_init();
   }
   if (columns < 80 || lines < 25)
   {
      osd_close_display();
      osd_exit();
      fprintf(stderr,
         "Error: the %s debugger needs a terminal of at least 80x25\n"
         "Resize the terminal and try again\n", NAME);
      exit(1); /* ugly, anyone know a better way ? */
   }
   *width  = columns;
   *height = lines;
}

/* Set the actual display screen, used to switch back to video mode
   in the dos-port */
int osd_set_display(int width,int height,int depth,int attributes,int orientation)
{
   if (sysdep_set_video_mode()!=OSD_OK) return 0;
   return 1;
}

void osd_set_screen_curpos (int x, int y)
{
   if(!curses_initialised)
   {
      sysdep_debug_real_init();
   }
   cursor_x = x;
   cursor_y = y;
   move (y, x);
   refresh ();
}

void osd_put_screen_char (int ch, int attr, int x, int y)
{
   if(!curses_initialised)
   {
      sysdep_debug_real_init();
   }
   if (have_color)
   {
      /* ncurses uses bold to get the bright variants of the 8 basic colors */
      if (attr & 0x08)
         attrset(A_BOLD   | COLOR_PAIR ((attr & 0x07) + ((attr & 0x70) >> 1)));
      else
         attrset(A_NORMAL | COLOR_PAIR ((attr & 0x07) + ((attr & 0x70) >> 1)));   
   }
   else
   {
      if ((attr & 7) == 7 && (attr & 0x70) == 0x40) /* white on red (curr. disassembler line) */
      {
         attrset(A_REVERSE);
      }
      else if (attr & 0x08)
      {
         attrset(A_NORMAL);  /* invert the meaning, else everything is bold */
      }
      else
      {
         attrset(A_BOLD);
      }
   }
   mvaddch (y, x, ch);
   move(cursor_y, cursor_x);
   refresh ();
}

void osd_screen_update(void)
{
}

#endif
