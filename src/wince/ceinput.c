//============================================================
//
//	ceinput.c - WinCE implementation of MAME input routines
//
//============================================================

#include <windows.h>
#include "driver.h"
#include "mame.h"
#include "inptport.h"
#include "window.h"
#include "rc.h"
#include "mamece.h"
#include "..\windows\window.h"

#define NUMKEYSTATES    256

#define VK_CASSIOPEIA_SLIDER	0x86
#define VK_CASSIOPEIA_ESC		0x87
#define VK_CASSIOPEIA_REC		0xC1
#define VK_CASSIOPEIA_B1		0xC2
#define VK_CASSIOPEIA_B2		0xC3
#define VK_CASSIOPEIA_B3		0xC4

#define VK_CASIO_E115_SLIDER	0x86
#define VK_CASIO_E115_ESC		0xC1
#define VK_CASIO_E115_REC		0xC2
#define VK_CASIO_E115_B1		0xC3
#define VK_CASIO_E115_B2		0xC4
#define VK_CASIO_E115_B3		0x5B

#define VK_IPAQ_ACTION			0x86
#define VK_IPAQ_REC				0x5B
#define VK_IPAQ_B1				0xC1
#define VK_IPAQ_B2				0xC2
#define VK_IPAQ_B3				0xC3
#define VK_IPAQ_B4				0xC4

#define VK_HP_ACTION			0x0D
#define VK_HP_REC				0x1B
#define VK_HP_ROCKER_UP			0x26
#define VK_HP_ROCKER_DOWN		0x28
#define VK_HP_B1				0x25
#define VK_HP_B2				0x27
#define VK_HP_B3				0xC3
#define VK_HP_B4				0xC4

/***************************************************************************
    External variables
 ***************************************************************************/

struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

UINT8 win_trying_to_quit;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static char pressed_char;
static DWORD pressed_char_expire;
static void *input_map;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
int win_init_input(void)
{
    return gx_open_input() ? 0 : -1;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
void win_shutdown_input(void)
{
	gx_close_input();
}

// Keyboard Definitions
static struct KeyboardInfo keylist[] =
{
	{ "Button 1",	VK_IPAQ_B1,			KEYCODE_F1 },
	{ "Button 2",	VK_IPAQ_B2,			KEYCODE_F2 },
	{ "Button 3",	VK_IPAQ_B3,			KEYCODE_F3 },
	{ "Button 4",	VK_IPAQ_B4,			KEYCODE_F4 },
    { "A",          'A',                KEYCODE_A },
    { "B",          'B',                KEYCODE_B },
    { "C",          'C',                KEYCODE_C },
    { "D",          'D',                KEYCODE_D },
    { "E",          'E',                KEYCODE_E },
    { "F",          'F',                KEYCODE_F },
    { "G",          'G',                KEYCODE_G },
    { "H",          'H',                KEYCODE_H },
    { "I",          'I',                KEYCODE_I },
    { "J",          'J',                KEYCODE_J },
    { "K",          'K',                KEYCODE_K },
    { "L",          'L',                KEYCODE_L },
    { "M",          'M',                KEYCODE_M },
    { "N",          'N',                KEYCODE_N },
    { "O",          'O',                KEYCODE_O },
    { "P",          'P',                KEYCODE_P },
    { "Q",          'Q',                KEYCODE_Q },
    { "R",          'R',                KEYCODE_R },
    { "S",          'S',                KEYCODE_S },
    { "T",          'T',                KEYCODE_T },
    { "U",          'U',                KEYCODE_U },
    { "V",          'V',                KEYCODE_V },
    { "W",          'W',                KEYCODE_W },
    { "X",          'X',                KEYCODE_X },
    { "Y",          'Y',                KEYCODE_Y },
    { "Z",          'Z',                KEYCODE_Z },
    { "0",          '0',                KEYCODE_0 },
    { "1",          '1',		        KEYCODE_1 },
    { "2",          '2',		        KEYCODE_2 },
    { "3",          '3',		        KEYCODE_3 },
    { "4",          '4',                KEYCODE_4 },
    { "5",          '5',                KEYCODE_5 },
    { "6",          '6',                KEYCODE_6 },
    { "7",          '7',                KEYCODE_7 },
    { "8",          '8',                KEYCODE_8 },
    { "9",          '9',                KEYCODE_9 },
    { "~",          0xC0,               KEYCODE_TILDE },
    { "-",          0xBD,               KEYCODE_MINUS },
    { "=",          0xBB,               KEYCODE_EQUALS },
    { "BKSPACE",    VK_BACK,            KEYCODE_BACKSPACE },
    { "TAB",        VK_TAB,             KEYCODE_TAB },
    { "[",          0xDB,               KEYCODE_OPENBRACE },
    { "]",          0xDD,               KEYCODE_CLOSEBRACE },
    { "ENTER",      VK_RETURN,          KEYCODE_ENTER },
    { ";",          0xBA,               KEYCODE_COLON },
    { "'",          0xDE,               KEYCODE_QUOTE },
    { "\\",			0xDC,		        KEYCODE_BACKSLASH },
    { ",",          0xBC,               KEYCODE_COMMA },
    { ".",          0xBE,               KEYCODE_STOP },
    { "/",          0xBF,               KEYCODE_SLASH },
    { "SPACE",      VK_SPACE,           KEYCODE_SPACE },
    { "LEFT",       VK_LEFT,            KEYCODE_LEFT },
    { "RIGHT",      VK_RIGHT,           KEYCODE_RIGHT },
    { "UP",         VK_UP,              KEYCODE_UP },
    { "DOWN",       VK_DOWN,            KEYCODE_DOWN },
    { "LSHIFT",     VK_LSHIFT,          KEYCODE_LSHIFT },
    { "RSHIFT",     VK_RSHIFT,          KEYCODE_RSHIFT },
    { "LCTRL",      VK_LCONTROL,        KEYCODE_LCONTROL },
    { "RCTRL",      VK_RCONTROL,        KEYCODE_RCONTROL },
    { "LALT",       VK_LMENU,           KEYCODE_RALT },
    { "RALT",       VK_RMENU,           KEYCODE_RALT },
    { "SHIFT",      VK_SHIFT,           KEYCODE_LSHIFT },
    { "CTRL",       VK_CONTROL,         KEYCODE_LCONTROL },
    { "ALT",        VK_MENU,            KEYCODE_LALT },
    { "LWIN",       VK_LWIN,            KEYCODE_OTHER },
    { "RWIN",       VK_RWIN,            KEYCODE_OTHER },
    { "MENU",       VK_APPS,            KEYCODE_OTHER },
    { "CAPSLOCK",   VK_CAPITAL,         KEYCODE_CAPSLOCK },
    { 0, 0, 0 } /* end of table */
};

/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo* osd_get_key_list(void)
{
    return keylist;
}

static int ration_input(const struct GameDriver *drv, UINT32 player, int *buttons, int buttons_available)
{
	const struct InputPortTiny *input_ports;
	int button_count = 0;
	int start_count = 0;
	int coin_count = 0;
	UINT32 type;
	int input_id;

	/* clear out the output */
	memset(buttons, 0, sizeof(*buttons) * buttons_available);

	/* go through the input list, and count the number of the appropriate type
	 * of buttons, and identify the arrow buttons if possible
	 */
	input_ports = drv->input_ports;
	while(input_ports->type != IPT_END)
	{
		type = input_ports->type;

		/* right player? */
		if ((type & IPF_PLAYERMASK) == player)
		{
			type &= 0xffff;
			switch(type)
			{
			case IPT_START1:
			case IPT_START2:
			case IPT_START3:
			case IPT_START4:
				start_count = 1;
				break;

			case IPT_COIN1:
			case IPT_COIN2:
			case IPT_COIN3:
			case IPT_COIN4:
				coin_count = 1;
				break;

			case IPT_BUTTON1:
			case IPT_BUTTON2:
			case IPT_BUTTON3:
			case IPT_BUTTON4:
			case IPT_BUTTON5:
			case IPT_BUTTON6:
			case IPT_BUTTON7:
			case IPT_BUTTON8:
			case IPT_BUTTON9:
			case IPT_BUTTON10:
				button_count++;
				break;
			}
		}
		input_ports++;
	}

	/* do we have enough buttons? */
	if ((start_count + coin_count + button_count) > buttons_available)
	{
		logerror("not enough buttons for driver\n");
		return 1;
	}

	/* now time to allocate buttons */
	input_id = IPT_BUTTON1;
	while(button_count--)
		*(buttons++) = input_id++;

	/* now time to allocate coins */
	input_id = IPT_COIN1;
	while(coin_count--)
		*(buttons++) = input_id++;

	/* now time to allocate start buttons */
	input_id = IPT_START1;
	while(start_count--)
		*(buttons++) = input_id++;

	/* done */
	return 0;
}

/*
  inptport.c defines some general purpose defaults for key bindings. They may be
  further adjusted by the OS dependant code to better match the available keyboard,
  e.g. one could map pause to the Pause key instead of P, or snapshot to PrtScr
  instead of F12. Of course the user can further change the settings to anything
  he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys you want.
*/
void osd_customize_inputport_defaults(struct ipd *defaults)
{
	int i, err;
	UINT32 player;
	UINT32 buttons[4];

	/* this is the player that gets the UI keys */
	player = IPF_PLAYER1;

	err = ration_input(Machine->gamedrv, player, buttons, sizeof(buttons) / sizeof(buttons[0]));
	if (err)
		return;

	while (defaults->type != IPT_END)
	{
		if ((defaults->type & IPF_PLAYERMASK) == player)
		{
			for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++)
			{
				if (buttons[i] == (defaults->type & ~IPF_PLAYERMASK))
				{
					seq_set_1(&defaults->seq, KEYCODE_F1 + i);
					break;
				}
			}
		}
		defaults++;
	}
}

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_customize_inputport_defaults().
*/
int osd_is_key_pressed(int keycode)
{
    SHORT state;

	// special case: if we're trying to quit, fake up/down/up/down
	if (keycode == VK_ESCAPE && win_trying_to_quit) {
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

    win_process_events_periodic();

	/* Are we pressing a char received by WM_KEYDOWN? */
	if (keycode == pressed_char) {
		if (pressed_char_expire > GetTickCount())
			return 1;
		/* Ooops... we're expired */
		pressed_char = 0;
	}
	
	state = GetAsyncKeyState(keycode);

    /* If the high-order bit is 1, the key is down; otherwise, it is up */
    if (state & 0x8000)
        return 1;

    return 0;
}


void wince_press_char(int keycode)
{
	pressed_char = keycode;
	pressed_char_expire = GetTickCount() + 100;
}

/*
  wait for the user to press a key and return its code. This function is not
  required to do anything, it is here so we can avoid bogging down multitasking
  systems while using the debugger. If you don't want to or can't support this
  function you can just return OSD_KEY_NONE.
*/
int osd_wait_keypress(void)
{
    while (1)
    {
       Sleep(1);
       win_process_events_periodic();
    }
    return 0;
}

const struct JoystickInfo *osd_get_joy_list(void)
{
	static struct JoystickInfo dummylist;
	memset(&dummylist, 0, sizeof(dummylist));
	return &dummylist;
}

int osd_is_joy_pressed(int joycode)
{
	return 0;
}

int osd_joystick_needs_calibration (void)
{
	return 0;
}

void osd_joystick_start_calibration (void)
{
}

const char *osd_joystick_calibrate_next (void)
{
	return NULL;
}

void osd_joystick_calibrate (void)
{
}

void osd_joystick_end_calibration (void)
{
}

void osd_trak_read(int player,int *deltax,int *deltay)
{
	*deltax = 0;
	*deltay = 0;
}


/* return values in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y)
{
	*analog_x = 0;
	*analog_y = 0;
}

int osd_readkey_unicode(int flush)
{
	return 0;
}

void win_poll_input(void)
{
	HWND focus = GetFocus();
}

void win_pause_input(int paused)
{
}

int is_mouse_captured(void)
{
	return 0;
}
