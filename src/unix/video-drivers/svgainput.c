#include <vga.h>
#include <vgakeyboard.h>
/* fix ansi compilation */
#define inline
#include <vgamouse.h>
#undef inline
#include <signal.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include "svgainput.h"
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"

static int console_fd       = -1;
static int leds             =  0;
static int release_signal   =  0;
static int acquire_signal   =  0;
static struct sigaction release_sa;
static struct sigaction oldrelease_sa;
static struct sigaction acquire_sa;
static struct sigaction oldacquire_sa;
static void (*release_function)(void) = NULL;
static void (*acquire_function)(void) = NULL;

static const char scancode_to_unicode[128][2] = {
   { 0,   0   }, /* 0 */
   { 0,   0   },
   { '1', '!' },
   { '2', '@' },
   { '3', '#' },
   { '4', '$' },
   { '5', '%' },
   { '6', '^' },
   { '7', '&' },
   { '8', '*' },
   { '9', '(' }, /* 10 */
   { '0', ')' },
   { '-', '_' },
   { '=', '+' },
   { 0x8, 0x8 },
   { 0,   0   },
   { 'q', 'Q' },
   { 'w', 'W' },
   { 'e', 'E' },
   { 'r', 'R' },
   { 't', 'T' }, /* 20 */
   { 'y', 'Y' },
   { 'u', 'U' },
   { 'i', 'I' },
   { 'o', 'O' },
   { 'p', 'P' },
   { '[', '{' },
   { ']', '}' },
   { 0,   0   },
   { 0,   0   },
   { 'a', 'A' }, /* 30 */
   { 's', 'S' },
   { 'd', 'D' },
   { 'f', 'F' },
   { 'g', 'G' },
   { 'h', 'H' },
   { 'j', 'J' },
   { 'k', 'K' },
   { 'l', 'L' },
   { ';', ':' },
   { '\'', '"' },/* 40 */
   { '`', '~' },
   { 0,   0   },
   { '\\', '|' },
   { 'z', 'Z' },
   { 'x', 'X' },
   { 'c', 'C' },
   { 'v', 'V' },
   { 'b', 'B' },
   { 'n', 'N' },
   { 'm', 'M' }, /* 50 */
   { ',', '<' },
   { '.', '>' },
   { '/', '?' },
   { 0,   0   },
   { '*', '*' },
   { 0,   0   },
   { ' ', ' ' },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }, /* 60 */
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }, /* 70 */
   { '7', '7' },
   { '8', '8' },
   { '9', '9' },
   { '-', '-' },
   { '4', '4' },
   { '5', '5' },
   { '6', '6' },
   { '+', '+' },
   { '1', '1' },
   { '2', '2' }, /* 80 */
   { '3', '3' },
   { '0', '0' },
   { '.', '.' },
   { 0,   0   },
   { 0,   0   },
   { '\\', '|' },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }, /* 90 */
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { '/', '/' },
   { 0,   0   },
   { 0,   0   }, /* 100 */
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }, /* 110 */
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }, /* 120 */
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   },
   { 0,   0   }
};

void release_handler(int n)
{
   if (release_function)
      release_function();
   oldrelease_sa.sa_handler(n);
   sigaction(release_signal, &release_sa, NULL);
   sigaction(acquire_signal, &acquire_sa, NULL);
}

void acquire_handler(int n)
{
   oldacquire_sa.sa_handler(n);
   sigaction(release_signal, &release_sa, NULL);
   sigaction(acquire_signal, &acquire_sa, NULL);
   keyboard_clearstate();
   keyboard_clear();
   if (console_fd >= 0)
      ioctl(console_fd, KDSETLED, leds);
   if (acquire_function)
      acquire_function();
}

void keyboard_handler(int scancode, int press)
{
   static int shift = 0;
   int shift_mask = 0;
   struct keyboard_event event;
   
   switch (scancode)
   {
      case KEY_LSHIFT:
         shift_mask = 0x01;
         break;
      case KEY_RSHIFT:
         shift_mask = 0x02;
         break;
   }
   
   if (press)
      shift |= shift_mask;
   else
      shift &= ~shift_mask;
   
   event.press = press;   
   event.scancode = scancode;
   event.unicode = scancode_to_unicode[scancode][shift? 1:0];
   keyboard_register_event(&event);
}

int svga_input_init(void (*release_func)(void), void (*acquire_func)(void))
{
   release_function = release_func;
   acquire_function = acquire_func;
   
   /* newer svgalib's use different signals */
   if (vga_setmode(-1)<0x1410)
   {
      fprintf(stderr_file, "info: svgalib version older then 1.4.1 detected, using old style signals\n");
      release_signal = SIGUSR1;
      acquire_signal = SIGUSR2;
   }
   else
   {
      fprintf(stderr_file, "info: svgalib version 1.4.1 or newer detected, using new style signals\n");
      release_signal = SIGPROF;
      acquire_signal = SIGUNUSED;
   }
  
   /* catch console switch signals to enable / disable the vga pass through */
   memset(&release_sa, 0, sizeof(struct sigaction));
   memset(&acquire_sa, 0, sizeof(struct sigaction));
   release_sa.sa_handler = release_handler;
   acquire_sa.sa_handler = acquire_handler;
   sigaction(release_signal, &release_sa, &oldrelease_sa);
   sigaction(acquire_signal, &acquire_sa, &oldacquire_sa);

   /* init the keyboard */
   if ((console_fd = keyboard_init_return_fd()) < 0)
   {
      fprintf(stderr_file, "Svgalib: Error: Couldn't open keyboard\n");
      return -1;
   }
   keyboard_seteventhandler(keyboard_handler);
   ioctl(console_fd, KDSETLED, leds);

   /* 
      not sure if this is the best site but mouse init routine must be 
      called after video initialization...
   */   
   if(use_mouse)
   {
	if (mouse_init("/dev/mouse", vga_getmousetype(), MOUSE_DEFAULTSAMPLERATE))
	{
		perror("mouse_init");
		fprintf(stderr_file,"SVGALib: failed to open mouse device\n");
		use_mouse=0;
	}
	else
	{
		/* fix ranges and initial position of mouse */
		mouse_setrange_6d(-500,500, -500,500, -500,500, -500,500,
                   -500,500, -500,500, MOUSE_6DIM);
		mouse_setposition_6d(0, 0, 0, 0, 0, 0, MOUSE_6DIM);
	}
   }
   return 0;
}

void svga_input_exit(void)
{
   /* restore the old handlers */
   sigaction(release_signal, &oldrelease_sa, NULL);
   sigaction(acquire_signal, &oldacquire_sa, NULL);

   if (console_fd >= 0)
   {
      ioctl(console_fd, KDSETLED, 8);
      keyboard_close();
   }

   if (use_mouse)
      mouse_close();
}

void sysdep_mouse_poll (void)
{
	int i, mouse_buttons;
	
	mouse_update();
	
	mouse_getposition_6d(&mouse_data[0].deltas[0],
           &mouse_data[0].deltas[1],
           &mouse_data[0].deltas[2],
           &mouse_data[0].deltas[3],
           &mouse_data[0].deltas[4],
           &mouse_data[0].deltas[5]);
	
	/* scale down the delta's to some more sane values */
	for(i=0; i<6; i++)
	   mouse_data[0].deltas[i] /= 20;

	mouse_buttons = mouse_getbutton();

        for(i=0; i<MOUSE_BUTTONS; i++)
        {
           mouse_data[0].buttons[i] = mouse_buttons & (0x01 << i);
        }

	mouse_setposition_6d(0, 0, 0, 0, 0, 0, MOUSE_6DIM);
}

static const int led_flags[3] = {
  LED_NUM,
  LED_CAP,
  LED_SCR
};

void osd_led_w(int led,int on)
{
	if (led>=3) return;
	if (on)
		leds |=  led_flags[led];
	else
		leds &= ~led_flags[led];
	if (console_fd >= 0)
		ioctl(console_fd, KDSETLED, leds);
}
