/*
 * X-Mame x11 input code
 *
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include "xmame.h"
#include "devices.h"
#include "x11.h"
#include "xkeyboard.h"

static Cursor xinput_normal_cursor;
static Cursor xinput_invisible_cursor;
static int current_mouse[MOUSE_AXES] = {0,0,0,0,0,0,0,0};
static int xinput_use_winkeys = 0;
static int xinput_mapkey(struct rc_option *option, const char *arg, int priority);
static int xinput_keyboard_grabbed = 0;
static int xinput_mouse_grabbed = 0;
static int xinput_cursors_allocated = 0;
static int xinput_grab_mouse = 0;
static int xinput_grab_keyboard = 0;
static int xinput_show_cursor = 1;
static int xinput_force_grab = 0;

static int xinput_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option x11_input_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11-input related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "grabmouse", "gm", rc_bool, &xinput_grab_mouse, "0", 0, 0, NULL, "Enable/disable mousegrabbing (also alt + pagedown)" },
	{ "grabkeyboard", "gkb", rc_bool, &xinput_grab_keyboard, "0", 0, 0, NULL, "Enable/disable keyboardgrabbing (also alt + pageup)" },
	{ "cursor", "cu", rc_bool, &xinput_show_cursor, "1", 0, 0, NULL, "Show/don't show the cursor" },
	{ "winkeys", "wk", rc_bool, &xinput_use_winkeys, "0", 0, 0, NULL, "Enable/disable mapping of windowskeys under X" },
	{ "mapkey", "mk", rc_use_function, NULL, NULL, 0, 0, xinput_mapkey, "Set a specific key mapping, see xmamerc.dist" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

/*
 * Parse keyboard events
 */
void sysdep_update_keyboard (void)
{
	XEvent				E;
	KeySym				keysym;
	char				keyname[16+1];
	int				mask;
	struct xmame_keyboard_event	event;
	/* grrr some windowmanagers send multiple focus events, this is used to
	   filter them. */
	static int			focus = FALSE;

#ifdef NOT_DEFINED /* x11 */
	if(run_in_root_window && x11_video_mode == X11_WINDOW)
	{
		static int i=0;
		i = ++i % 3;
		switch (i)
		{
			case 0:
				xkey[KEY_O] = 0;
				xkey[KEY_K] = 0;
				break;
			case 1:
				xkey[KEY_O] = 1;
				xkey[KEY_K] = 0;
				break;
			case 2:
				xkey[KEY_O] = 0;
				xkey[KEY_K] = 1;
				break;
		}
	}
	else
#endif

		/* query all events that we have previously requested */
		while ( XPending(display) )
		{
			mask = FALSE;
			event.press = FALSE;

			XNextEvent(display,&E);
			/*  fprintf(stderr_file,"Event: %d\n",E.type); */

			/* we don't have to check x11_video_mode or extensions like xil here,
			   since our eventmask should make sure that we only get the event's matching
			   the current update method */
			switch (E.type)
			{
				/* display events */
				case FocusIn:
					/* check for multiple events and ignore them */
					if (focus) break;
					focus = TRUE;
					/* to avoid some meta-keys to get locked when wm iconify xmame, we must
					   perform a key reset whenever we retrieve keyboard focus */
					xmame_keyboard_clear();
					if (use_mouse &&
					    (xinput_force_grab || xinput_grab_mouse) &&
					    !XGrabPointer(display, window, True,
					      PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
					      GrabModeAsync, GrabModeAsync, window, None, CurrentTime))
					{
						xinput_mouse_grabbed = 1;
						if (xinput_cursors_allocated && xinput_show_cursor)
						  XDefineCursor(display,window,xinput_invisible_cursor);
					}
					break;
				case FocusOut:
					/* check for multiple events and ignore them */
					if (!focus) break;
					focus = FALSE;
					if (xinput_mouse_grabbed)
					{
						XUngrabPointer(display, CurrentTime);
						if (xinput_cursors_allocated && xinput_show_cursor)
						  XDefineCursor(display,window,xinput_normal_cursor);
						xinput_mouse_grabbed = 0;
					}
					break;
				case EnterNotify:
					if (use_private_cmap) XInstallColormap(display,colormap);
					break;	
				case LeaveNotify:
					if (use_private_cmap) XInstallColormap(display,DefaultColormapOfScreen(screen));
					break;	
#ifdef USE_XIL
				case ConfigureNotify:
					update_xil_window_size( E.xconfigure.width, E.xconfigure.height );
					break;
#endif
				/* input events */
				case MotionNotify:
					current_mouse[0] += E.xmotion.x_root;
					current_mouse[1] += E.xmotion.y_root;
					break;
				case ButtonPress:
					mask = TRUE;
#ifdef USE_DGA
					/* Some buggy combination of XFree and virge screwup the viewport
					   on the first mouseclick */
					if(xf86_dga_first_click) { xf86_dga_first_click = 0; xf86_dga_fix_viewport = 1; }
#endif          
				case ButtonRelease:
					mouse_data[0].buttons[E.xbutton.button-1] = mask;
					break;
				case KeyPress:
					event.press = TRUE;
				case KeyRelease:
					/* get bare keysym, for the scancode */
					keysym = XLookupKeysym ((XKeyEvent *) &E, 0);
					/* get key name, using modifiers for the unicode */
					XLookupString ((XKeyEvent *) &E, keyname, 16, NULL, NULL);

					/*	fprintf(stderr, "Keyevent keycode: %04X, keysym: %04X, unicode: %02X\n",
						E.xkey.keycode, (unsigned int)keysym, (unsigned int)keyname[0]); */

					/* look which table should be used */
					if ( (keysym & ~0x1ff) == 0xfe00 )
						event.scancode = extended_code_table[keysym & 0x01ff];
					else if (keysym < 0xff)
						event.scancode = code_table[keysym & 0x00ff];
					else
						event.scancode = 0;

					event.unicode = keyname[0];

					xmame_keyboard_register_event(&event);
					break;
				default:
					/* grrr we can't use case here since the event types for XInput devices
					   aren't hardcoded, since we should have caught anything else above,
					   just asume it's an XInput event */
#ifdef USE_XINPUT_DEVICES
					if (XInputProcessEvent(&E)) break;
#endif
#ifdef X11_JOYSTICK
					process_x11_joy_event(&E);
#endif
					break;
			} /* switch */
		} /* while */
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int xinput_mapkey(struct rc_option *option, const char *arg, int priority)
{
	unsigned int from,to;
	/* ultrix sscanf() requires explicit leading of 0x for hex numbers */
	if ( sscanf(arg,"0x%x,0x%x",&from,&to) == 2)
	{
		/* perform tests */
		/* fprintf(stderr,"trying to map %x to %x\n",from,to); */
		if ( to <= 127 )
		{
			if ( from <= 0x00ff ) 
			{
				code_table[from]=to; return OSD_OK;
			}
			else if ( (from>=0xfe00) && (from<=0xffff) ) 
			{
				extended_code_table[from&0x01ff]=to; return OSD_OK;
			}
		}
		/* stderr_file isn't defined yet when we're called. */
		fprintf(stderr,"Invalid keymapping %s. Ignoring...\n", arg);
	}
	return OSD_NOT_OK;
}

void sysdep_mouse_poll(void)
{
	int i;
	if(x11_video_mode == X11_DGA)
	{
		/* 2 should be MOUSE_AXES but we don't support more
		   than 2 axes at the moment so this is faster */
		for (i=0; i<2; i++)
		{
			mouse_data[0].deltas[i] = current_mouse[i];
			current_mouse[i] = 0;
		}
	}
	else
	{
		Window root,child;
		int root_x, root_y, pos_x, pos_y;
		unsigned int keys_buttons;

		if (!XQueryPointer(display,window, &root,&child, &root_x,&root_y,
					&pos_x,&pos_y,&keys_buttons) )
		{
			mouse_data[0].deltas[0] = 0;
			mouse_data[0].deltas[1] = 0;
			return;
		}

		if ( xinput_mouse_grabbed )
		{
			XWarpPointer(display, None, window, 0, 0, 0, 0,
					visual_width/2, visual_height/2);
			mouse_data[0].deltas[0] = pos_x - visual_width/2;
			mouse_data[0].deltas[1] = pos_y - visual_height/2;
		}
		else
		{
			mouse_data[0].deltas[0] = pos_x - current_mouse[0];
			mouse_data[0].deltas[1] = pos_y - current_mouse[1];
			current_mouse[0] = pos_x;
			current_mouse[1] = pos_y;
		}
	}
}

/*
 * This function creates an invisible cursor.
 *
 * I got the idea and code fragment from in the Apple II+ Emulator
 * version 0.06 for Linux by Aaron Culliney
 * <chernabog@baldmountain.bbn.com>
 *
 * I also found a post from Steve Lamont <spl@szechuan.ucsd.edu> on
 * xforms@bob.usuf2.usuhs.mil.  His comments read:
 *
 * Lifted from unclutter
 * Mark M Martin. cetia 1991 mmm@cetia.fr
 * Version 4 changes by Charles Hannum <mycroft@ai.mit.edu>
 *
 * So I guess this code has been around the block a few times.
 */

static Cursor xinput_create_invisible_cursor (Display * display, Window win)
{
   Pixmap cursormask;
   XGCValues xgc;
   XColor dummycolour;
   Cursor cursor;
   GC gc;

   cursormask = XCreatePixmap (display, win, 1, 1, 1 /*depth */ );
   xgc.function = GXclear;
   gc = XCreateGC (display, cursormask, GCFunction, &xgc);
   XFillRectangle (display, cursormask, gc, 0, 0, 1, 1);
   dummycolour.pixel = 0;
   dummycolour.red = 0;
   dummycolour.flags = 04;
   cursor = XCreatePixmapCursor (display, cursormask, cursormask,
                                 &dummycolour, &dummycolour, 0, 0);
   XFreeGC (display, gc);
   XFreePixmap (display, cursormask);
   return cursor;
}

void sysdep_set_leds(int leds)
{
}

int xinput_open(int force_grab, int event_mask)
{
  xinput_force_grab = force_grab;
  
  /* handle winkey mappings */
  if (xinput_use_winkeys)
  {
  	extended_code_table[XK_Meta_L&0x1FF] = KEY_LWIN; 
  	extended_code_table[XK_Meta_R&0x1FF] = KEY_RWIN; 
  }

  /* mouse grab failing is not really a problem */
  if (use_mouse && (xinput_force_grab || xinput_grab_mouse))
  {
    if(XGrabPointer (display, window, True,
         PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
         GrabModeAsync, GrabModeAsync, window, None, CurrentTime))
      fprintf(stderr_file, "Warning mouse grab failed\n");
    else
      xinput_mouse_grabbed = 1;
  }

  if (window != DefaultRootWindow(display))
  {
    /* setup the cursor */
    xinput_normal_cursor     = XCreateFontCursor (display, XC_trek);
    xinput_invisible_cursor  = xinput_create_invisible_cursor (display, window);
    xinput_cursors_allocated = 1;

    if (xinput_mouse_grabbed || !xinput_show_cursor || xinput_force_grab)
 	XDefineCursor (display, window, xinput_invisible_cursor);
    else
    	XDefineCursor (display, window, xinput_normal_cursor);

    /* Select event mask */
    event_mask |= FocusChangeMask | KeyPressMask | KeyReleaseMask;
    if (use_mouse)
    {
       event_mask |= ButtonPressMask | ButtonReleaseMask;
    }
    XSelectInput (display, window, event_mask);
  }

  if ((xinput_force_grab == 2) || xinput_grab_keyboard)
  {
    /* keyboard grab failing could be fatal so let the caller know */
    if (XGrabKeyboard(display, window, True, GrabModeAsync, GrabModeAsync,
         CurrentTime))
      return 1;

    xinput_keyboard_grabbed = 1;
  }
  
  return 0;
}

void xinput_close(void)
{
  if (xinput_mouse_grabbed)
  {
     XUngrabPointer (display, CurrentTime);
     xinput_mouse_grabbed = 0;
  }

  if (xinput_keyboard_grabbed)
  {
     XUngrabKeyboard (display, CurrentTime);
     xinput_keyboard_grabbed = 0;
  }

  if(xinput_cursors_allocated)
  {
     XFreeCursor(display, xinput_normal_cursor);
     XFreeCursor(display, xinput_invisible_cursor);
     xinput_cursors_allocated = 0;
  }
}

void xinput_check_hotkeys(void)
{
  if (use_mouse && !xinput_force_grab &&
      code_pressed (KEYCODE_LALT) &&
      code_pressed_memory (KEYCODE_PGDN))
  {
     if (xinput_mouse_grabbed)
     {
        XUngrabPointer (display, CurrentTime);
        if (xinput_cursors_allocated && xinput_show_cursor)
           XDefineCursor (display, window, xinput_normal_cursor);
        xinput_mouse_grabbed = 0;
     }
     else if (!XGrabPointer (display, window, True,
                 PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync, window, None, CurrentTime))
     {
        if (xinput_cursors_allocated && xinput_show_cursor)
           XDefineCursor (display, window, xinput_invisible_cursor);
        xinput_mouse_grabbed = 1;
     }
  }

  /* toggle keyboard grabbing */
  if ((xinput_force_grab!=2) && code_pressed (KEYCODE_LALT) &&
      code_pressed_memory (KEYCODE_PGUP))
  {
    if (xinput_keyboard_grabbed)
    {
      XUngrabKeyboard (display, CurrentTime);
      xinput_keyboard_grabbed = 0;
    }
    else if (!XGrabKeyboard(display, window, True, GrabModeAsync, GrabModeAsync, CurrentTime))
    {
      xinput_keyboard_grabbed = 1;
    }
  }
}
