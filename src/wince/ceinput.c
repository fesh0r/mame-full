//============================================================
//
//	ceinput.c - WinCE implementation of MAME input routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "driver.h"
#include "mame.h"
#include "inptport.h"
#include "window.h"
#include "rc.h"
#include "mamece.h"
#include "..\windows\window.h"
#include "invokegx.h"

//============================================================
//	PARAMETERS
//============================================================

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
#define VK_IPAQ_CALENDAR		0xC1
#define VK_IPAQ_CONTACTS		0xC2
#define VK_IPAQ_QMENU			0xC3
#define VK_IPAQ_QSTART			0xC4
#define VK_IPAQ_REC				0xC5

#define VK_HP_ACTION			0x0D
#define VK_HP_REC				0x1B
#define VK_HP_ROCKER_UP			0x26
#define VK_HP_ROCKER_DOWN		0x28
#define VK_HP_B1				0x25
#define VK_HP_B2				0x27
#define VK_HP_B3				0xC3
#define VK_HP_B4				0xC4

#define KEYCODE_CEBTN1	KEYCODE_1_PAD
#define KEYCODE_CEBTN2	KEYCODE_2_PAD
#define KEYCODE_CEBTN3	KEYCODE_3_PAD
#define KEYCODE_CEBTN4	KEYCODE_4_PAD
#define KEYCODE_CEBTN5	KEYCODE_5_PAD
#define KEYCODE_CEBTN6	KEYCODE_6_PAD
#define KEYCODE_CEBTN7	KEYCODE_7_PAD
#define KEYCODE_CEBTN8	KEYCODE_8_PAD

//============================================================
//	GLOBAL VARIABLES
//============================================================

struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

UINT8 win_trying_to_quit;

//============================================================
//	LOCAL VARIABLES
//============================================================

// global states
static cycles_t last_poll;

static UINT32 input_map_req[NUMKEYSTATES/32];
static UINT32 input_map[NUMKEYSTATES/32];

// Keyboard Definitions
static struct KeyboardInfo keylist[] =
{
#ifdef ARM
	{ "QStart",		VK_IPAQ_QSTART,		KEYCODE_CEBTN1 },
	{ "QMenu",		VK_IPAQ_QMENU,		KEYCODE_CEBTN2 },
	{ "Contacts",	VK_IPAQ_CONTACTS,	KEYCODE_CEBTN3 },
	{ "Calendar",	VK_IPAQ_CALENDAR,	KEYCODE_CEBTN4 },
	{ "Action",		VK_IPAQ_ACTION,		KEYCODE_CEBTN7 },
	{ "Record",		VK_IPAQ_REC,		KEYCODE_CEBTN8 },
#elif defined(MIPS)
	{ "Slider",		VK_CASSIOPEIA_SLIDER,	KEYCODE_CEBTN8 },
	{ "Escape",		VK_CASSIOPEIA_ESC,		KEYCODE_CEBTN7 },
	{ "Record",		VK_CASSIOPEIA_REC,		KEYCODE_CEBTN1 },
	{ "Button 1",	VK_CASSIOPEIA_B1,		KEYCODE_CEBTN2 },
	{ "Button 2",	VK_CASSIOPEIA_B2,		KEYCODE_CEBTN3 },
	{ "Button 3",	VK_CASSIOPEIA_B3,		KEYCODE_CEBTN4 },
#elif defined(SHx)
	{ "Action",		VK_HP_ACTION,		KEYCODE_CEBTN7 },
	{ "Record",		VK_HP_REC,			KEYCODE_CEBTN8 },
	{ "Button 1",	VK_HP_B1,			KEYCODE_CEBTN1 },
	{ "Button 2",	VK_HP_B2,			KEYCODE_CEBTN2 },
	{ "Button 3",	VK_HP_B3,			KEYCODE_CEBTN3 },
	{ "Button 4",	VK_HP_B4,			KEYCODE_CEBTN4 },
#endif
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

//============================================================
//	win_init_input
//============================================================

int win_init_input(void)
{
    return gx_open_input() ? 0 : -1;
}


//============================================================
//	win_shutdown_input
//============================================================

void win_shutdown_input(void)
{
	gx_close_input();
}


//============================================================
//	osd_get_key_list
//============================================================

const struct KeyboardInfo* osd_get_key_list(void)
{
    return keylist;
}

//============================================================
//	add_seq
//============================================================

static void add_seq(struct ipd *defaults, InputCode code)
{
	InputCode oldcode;
	oldcode = seq_get_1(&defaults->seq);
	seq_set_3(&defaults->seq, oldcode, CODE_OR, code);
}


//============================================================
//	osd_customize_inputport_defaults
//============================================================

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	UINT32 player;

	/* this is the player that gets the UI keys */
	player = IPF_PLAYER1;

	while (defaults->type != IPT_END)
	{
		if ((defaults->type & IPF_PLAYERMASK) == player)
		{
				switch(defaults->type & ~IPF_PLAYERMASK)
				{
				case IPT_BUTTON1:
					add_seq(defaults, KEYCODE_CEBTN1);
					break;

				case IPT_BUTTON2:
					add_seq(defaults, KEYCODE_CEBTN2);
					break;

				case IPT_BUTTON3:
					add_seq(defaults, KEYCODE_CEBTN3);
					break;

				case IPT_BUTTON4:
					add_seq(defaults, KEYCODE_CEBTN4);
					break;

				case IPT_BUTTON5:
					add_seq(defaults, KEYCODE_CEBTN5);
					break;

				case IPT_BUTTON6:
					add_seq(defaults, KEYCODE_CEBTN6);
					break;

				case IPT_START1:
					add_seq(defaults, KEYCODE_CEBTN7);
					break;

				case IPT_COIN1:
					add_seq(defaults, KEYCODE_CEBTN8);
					break;
				}
		}
		defaults++;
	}
}


//============================================================
//	win_poll_input
//============================================================

void win_poll_input(void)
{
	HWND focus = GetFocus();
	int i, mapcode;

	// remember when this happened
	last_poll = osd_cycles();

	// periodically process events, in case they're not coming through
	win_process_events_periodic();

	// clear the keymap
	memset(input_map, 0, sizeof(input_map));

	if (focus)
	{
		for (i = 0; i < sizeof(keylist) / sizeof(keylist[0]); i++)
		{
			mapcode = keylist[i].code;

			// GetAsyncKeyState() appears to be an expensive operation, so we
			// try to minimize calls to it.  When we do call it, if the high-
			// order bit is 1, the key is down; otherwise, it is up 
			if ((input_map_req[mapcode / 32] & (1 << (mapcode % 32))) && GetAsyncKeyState(mapcode) & 0x8000)
				input_map[mapcode / 32] |= (1 << (mapcode % 32));
		}
	}

	// clear the requested map
	memset(input_map_req, 0, sizeof(input_map_req));
}


//============================================================
//	osd_is_key_pressed
//============================================================

int osd_is_key_pressed(int keycode)
{
    SHORT state;

	// make sure we've polled recently
	if (osd_cycles() > last_poll + osd_cycles_per_second()/4)
		win_poll_input();

	// special case: if we're trying to quit, fake up/down/up/down
	if (keycode == VK_ESCAPE && win_trying_to_quit) {
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	if ((keycode < 0) || (keycode >= NUMKEYSTATES))
		return 0;

	input_map_req[keycode / 32] |= (1 << keycode % 32);
	return (input_map[keycode / 32] >> (keycode % 32)) & 1;
}


//============================================================
//	osd_get_joy_list
//============================================================

const struct JoystickInfo *osd_get_joy_list(void)
{
	static struct JoystickInfo dummylist;
	memset(&dummylist, 0, sizeof(dummylist));
	return &dummylist;
}


//============================================================
//	osd_is_joy_pressed
//============================================================

int osd_is_joy_pressed(int joycode)
{
	return 0;
}


//============================================================
//	osd_joystick_needs_calibration
//============================================================

int osd_joystick_needs_calibration (void)
{
	return 0;
}


//============================================================
//	osd_joystick_start_calibration
//============================================================

void osd_joystick_start_calibration (void)
{
}


//============================================================
//	osd_joystick_calibrate_next
//============================================================

const char *osd_joystick_calibrate_next (void)
{
	return NULL;
}


//============================================================
//	osd_joystick_calibrate
//============================================================

void osd_joystick_calibrate (void)
{
}


//============================================================
//	osd_joystick_end_calibration
//============================================================

void osd_joystick_end_calibration (void)
{
}


//============================================================
//	osd_trak_read
//============================================================

void osd_trak_read(int player,int *deltax,int *deltay)
{
	*deltax = 0;
	*deltay = 0;
}


//============================================================
//	osd_analogjoy_read
//============================================================

void osd_analogjoy_read(int player,int analog_axis[], InputCode analogjoy_input[])
{
	memset(analog_axis, 0, sizeof(analog_axis[0]) * MAX_ANALOG_AXES);
}


//============================================================
//	osd_readkey_unicode
//============================================================

int osd_readkey_unicode(int flush)
{
	return 0;
}


//============================================================
//	win_pause_input
//============================================================

void win_pause_input(int paused)
{
}

//============================================================
//	osd_lightgun_read
//============================================================

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
}


//============================================================
//	osd_is_joystick_axis_code
//============================================================

int osd_is_joystick_axis_code(int joycode)
{
	return 0;
}


//============================================================
//	osd_set_leds
//============================================================

void osd_set_leds(int state)
{
}


//============================================================
//	osd_get_leds
//============================================================

int osd_get_leds(void)
{
	return 0;
}

