/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for WinCE
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Keyboard.c

 ***************************************************************************/

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

struct tKeyboard_private
{
    BOOL m_key_pressed;
    int  m_DefaultInput;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static char pressed_char;
static DWORD pressed_char_expire;
static struct tKeyboard_private This;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
int win_init_input(void)
{
    memset(&This, 0, sizeof(struct tKeyboard_private));
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
	{ "Button 1",	0,					JOYCODE_1_BUTTON1 },
	{ "Button 2",	0,					JOYCODE_1_BUTTON2 },
	{ "Button 3",	0,					KEYCODE_3 },
	{ "Button 4",	0,					KEYCODE_1 },
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
    { "0 PAD",      VK_NUMPAD0,         KEYCODE_0_PAD },
    { "1 PAD",      VK_NUMPAD1,         KEYCODE_1_PAD },
    { "2 PAD",      VK_NUMPAD2,         KEYCODE_2_PAD },
    { "3 PAD",      VK_NUMPAD3,         KEYCODE_3_PAD },
    { "4 PAD",      VK_NUMPAD4,         KEYCODE_4_PAD },
    { "5 PAD",      VK_NUMPAD5,         KEYCODE_5_PAD },
    { "6 PAD",      VK_NUMPAD6,         KEYCODE_6_PAD },
    { "7 PAD",      VK_NUMPAD7,         KEYCODE_7_PAD },
    { "8 PAD",      VK_NUMPAD8,         KEYCODE_8_PAD },
    { "9 PAD",      VK_NUMPAD9,         KEYCODE_9_PAD },
    { "F1",         VK_F1,              KEYCODE_F1 },
    { "F2",         VK_F2,              KEYCODE_F2 },
    { "F3",         VK_F3,              KEYCODE_F3 },
    { "F4",         VK_F4,              KEYCODE_F4 },
    { "F5",         VK_F5,              KEYCODE_F5 },
    { "F6",         VK_F6,              KEYCODE_F6 },
    { "F7",         VK_F7,              KEYCODE_F7 },
    { "F8",         VK_F8,              KEYCODE_F8 },
    { "F9",         VK_F9,              KEYCODE_F9 },
    { "F10",        VK_F10,             KEYCODE_F10 },
    { "F11",        VK_F11,             KEYCODE_F11 },
    { "F12",        VK_F12,             KEYCODE_F12 },
    { "ESC",        VK_ESCAPE,          KEYCODE_ESC },
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
    { "INS",        VK_INSERT,          KEYCODE_INSERT },
    { "DEL",        VK_DELETE,          KEYCODE_DEL },
    { "HOME",       VK_HOME,            KEYCODE_HOME },
    { "END",        VK_END,             KEYCODE_END },
    { "PGUP",       VK_PRIOR,           KEYCODE_PGUP },
    { "PGDN",       VK_NEXT,            KEYCODE_PGDN },
    { "LEFT",       VK_LEFT,            KEYCODE_LEFT },
    { "RIGHT",      VK_RIGHT,           KEYCODE_RIGHT },
    { "UP",         VK_UP,              KEYCODE_UP },
    { "DOWN",       VK_DOWN,            KEYCODE_DOWN },
    { "/ PAD",      VK_DIVIDE,          KEYCODE_SLASH_PAD },
    { "* PAD",      VK_MULTIPLY,        KEYCODE_ASTERISK },
    { "- PAD",      VK_SUBTRACT,        KEYCODE_MINUS_PAD },
    { "+ PAD",      VK_ADD,             KEYCODE_PLUS_PAD },
    { ". PAD",      VK_DECIMAL,         KEYCODE_DEL_PAD },
    { "PRTSCR",     VK_PRINT,           KEYCODE_PRTSCR },
    { "PAUSE",      VK_PAUSE,           KEYCODE_PAUSE },
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
    { "SCRLOCK",    VK_SCROLL,          KEYCODE_SCRLOCK },
    { "NUMLOCK",    VK_NUMLOCK,         KEYCODE_NUMLOCK },
    { "CAPSLOCK",   VK_CAPITAL,         KEYCODE_CAPSLOCK },
    { 0, 0, 0 } /* end of table */
};

/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo* osd_get_key_list(void)
{
	struct gx_keylist gxkeylist;
	static been_executed = 0;

	if (!been_executed) {
		/* first time? */
		been_executed = 1;
	
		gx_get_default_keys(&gxkeylist);

		keylist[0].code = gxkeylist.vkA;
		keylist[1].code = gxkeylist.vkB;
		keylist[2].code = gxkeylist.vkC;
		keylist[3].code = gxkeylist.vkStart;

		logerror("osd_get_key_list(): vkA=0x%02x vkB=0x%02x vkC=0x%02x vkStart=0x%02x\n", gxkeylist.vkA, gxkeylist.vkB, gxkeylist.vkC, gxkeylist.vkStart);
	}

    return keylist;
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
    int DefaultInfput = This.m_DefaultInput;
	while (defaults->type != IPT_END)
	{
		if (defaults->type == IPT_UI_SELECT)                           seq_set_1(&defaults->seq, KEYCODE_ENTER);
	//	if (defaults->type == IPT_UI_CANCEL)                           seq_set_1(&defaults->seq, KEYCODE_ESC);
		if (defaults->type == IPT_START1)                              seq_set_1(&defaults->seq, KEYCODE_1);
		if (defaults->type == IPT_START2)                              seq_set_1(&defaults->seq, KEYCODE_2);
		if (defaults->type == IPT_COIN1)                               seq_set_1(&defaults->seq, KEYCODE_3);
//			if (defaults->type == IPT_COIN2)                               seq_set_1(&defaults->seq, KEYCODE_4);
//			if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_R);
//			if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_F);
//			if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_D);
//			if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_G);
//			if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_8_PAD);
//			if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_2_PAD);
//			if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_4_PAD);
//			if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_6_PAD);
		if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
		if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_LALT);
		if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_SPACE);
//			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
//			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
//			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
		if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_LALT);
		if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_SPACE);
//			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
//			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
//			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER3))             seq_set_1(&defaults->seq, CODE_NONE);

/*			int j;
		for (j = 0; j < SEQ_MAX; ++j)
		{
			if (defaults->seq[j] == KEYCODE_UP)    defaults->seq[j] = VK_UP;
			if (defaults->seq[j] == KEYCODE_DOWN)  defaults->seq[j] = VK_DOWN;
			if (defaults->seq[j] == KEYCODE_LEFT)  defaults->seq[j] = VK_LEFT;
			if (defaults->seq[j] == KEYCODE_RIGHT) defaults->seq[j] = VK_RIGHT;
		}
		if (defaults->type == IPT_UI_SELECT)                           seq_set_1(&defaults->seq, VK_RETURN);
		if (defaults->type == IPT_UI_SELECT)                           seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
		if (defaults->type == IPT_UI_CANCEL)                           seq_set_1(&defaults->seq, VK_ESCAPE);
		if (defaults->type == IPT_UI_UP)                           seq_set_1(&defaults->seq, VK_UP);
		if (defaults->type == IPT_UI_DOWN)                           seq_set_1(&defaults->seq, VK_DOWN);
		if (defaults->type == IPT_UI_LEFT)                           seq_set_1(&defaults->seq, VK_LEFT);
		if (defaults->type == IPT_UI_RIGHT)                           seq_set_1(&defaults->seq, VK_RETURN);
		
		if (defaults->type == IPT_START1)                              seq_set_1(&defaults->seq, VK_MENU);
		if (defaults->type == IPT_START2)                              seq_set_1(&defaults->seq, VK_SPACE);
		if (defaults->type == IPT_COIN1)                               seq_set_1(&defaults->seq, VK_F23);
		if (defaults->type == IPT_COIN2)                               seq_set_1(&defaults->seq, VK_LCONTROL);

		if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1))             seq_set_1(&defaults->seq, VK_LCONTROL);
		if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1))             seq_set_1(&defaults->seq, VK_MENU);
		if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1))             seq_set_1(&defaults->seq, VK_SPACE);
		if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2))             seq_set_1(&defaults->seq, VK_LCONTROL);
		if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2))             seq_set_1(&defaults->seq, VK_MENU);
		if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2))             seq_set_1(&defaults->seq, VK_SPACE);
		if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
		if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2))             seq_set_1(&defaults->seq, CODE_NONE);
*/
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


void press_char(char c)
{
	pressed_char = c;
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
    This.m_key_pressed = FALSE;
    while (1)
    {
       Sleep(1);
       win_process_events_periodic();

       if (This.m_key_pressed)
          break;
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
