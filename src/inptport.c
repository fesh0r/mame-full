/***************************************************************************

  inptport.c

  Input ports handling

TODO:	remove the 1 analog device per port limitation
		support for inputports producing interrupts
		support for extra "real" hardware (PC throttle's, spinners etc)

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "config.h"

#ifdef MESS
#include "inputx.h"
#endif


/***************************************************************************

	Extern declarations

***************************************************************************/

extern void *record;
extern void *playback;

extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];
extern unsigned int lastcoin[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];

static unsigned short input_port_value[MAX_INPUT_PORTS];
static unsigned short input_vblank[MAX_INPUT_PORTS];



/***************************************************************************

	Local variables

***************************************************************************/

/* Assuming a maxium of one analog input device per port BW 101297 */
static struct InputPort *input_analog[MAX_INPUT_PORTS];
static int input_analog_current_value[MAX_INPUT_PORTS],input_analog_previous_value[MAX_INPUT_PORTS];
static int input_analog_init[MAX_INPUT_PORTS];
static int input_analog_scale[MAX_INPUT_PORTS];

static InputCode analogjoy_input[OSD_MAX_JOY_ANALOG][MAX_ANALOG_AXES];	/* [player#][mame axis#] array */

static int mouse_delta_axis[OSD_MAX_JOY_ANALOG][MAX_ANALOG_AXES];
static int lightgun_delta_axis[OSD_MAX_JOY_ANALOG][MAX_ANALOG_AXES];
static int analog_current_axis[OSD_MAX_JOY_ANALOG][MAX_ANALOG_AXES];
static int analog_previous_axis[OSD_MAX_JOY_ANALOG][MAX_ANALOG_AXES];

/***************************************************************************

  Configuration load/save

***************************************************************************/

/* this must match the enum in inptport.h */
const char ipdn_defaultstrings[][MAX_DEFSTR_LEN] =
{
	"Off",
	"On",
	"No",
	"Yes",
	"Lives",
	"Bonus Life",
	"Difficulty",
	"Demo Sounds",
	"Coinage",
	"Coin A",
	"Coin B",
	"9 Coins/1 Credit",
	"8 Coins/1 Credit",
	"7 Coins/1 Credit",
	"6 Coins/1 Credit",
	"5 Coins/1 Credit",
	"4 Coins/1 Credit",
	"3 Coins/1 Credit",
	"8 Coins/3 Credits",
	"4 Coins/2 Credits",
	"2 Coins/1 Credit",
	"5 Coins/3 Credits",
	"3 Coins/2 Credits",
	"4 Coins/3 Credits",
	"4 Coins/4 Credits",
	"3 Coins/3 Credits",
	"2 Coins/2 Credits",
	"1 Coin/1 Credit",
	"4 Coins/5 Credits",
	"3 Coins/4 Credits",
	"2 Coins/3 Credits",
	"4 Coins/7 Credits",
	"2 Coins/4 Credits",
	"1 Coin/2 Credits",
	"2 Coins/5 Credits",
	"2 Coins/6 Credits",
	"1 Coin/3 Credits",
	"2 Coins/7 Credits",
	"2 Coins/8 Credits",
	"1 Coin/4 Credits",
	"1 Coin/5 Credits",
	"1 Coin/6 Credits",
	"1 Coin/7 Credits",
	"1 Coin/8 Credits",
	"1 Coin/9 Credits",
	"Free Play",
	"Cabinet",
	"Upright",
	"Cocktail",
	"Flip Screen",
	"Service Mode",
	"Pause",
	"Test",
	"Tilt",
	"Version",
	"Region",
	"International",
	"Japan",
	"USA",
	"Europe",
	"Asia",
	"World",
	"Hispanic",
	"Language",
	"English",
	"Japanese",
	"German",
	"French",
	"Italian",
	"Spanish",
	"Very Easy",
	"Easy",
	"Normal",
	"Medium",
	"Hard",
	"Harder",
	"Hardest",
	"Very Hard",
	"Very Low",
	"Low",
	"High",
	"Higher",
	"Highest",
	"Very High",
	"Players",
	"Controls",
	"Dual",
	"Single",
	"Game Time",
	"Continue Price",
	"Controller",
	"Light Gun",
	"Joystick",
	"Trackball",
	"Continues",
	"Allow Continue",
	"Level Select",
	"Infinite",
	"Stereo",
	"Mono",
	"Unused",
	"Unknown"
};

struct ipd inputport_defaults[] =
{
	{ IPT_UI_CONFIGURE,			0, "Config Menu",		{ SEQ_DEF_1(KEYCODE_TAB) } },
	{ IPT_UI_ON_SCREEN_DISPLAY, 0, "On Screen Display",	{ SEQ_DEF_1(KEYCODE_TILDE) } },
	{ IPT_UI_PAUSE,				0, "Pause",				{ SEQ_DEF_1(KEYCODE_P) } },
	{ IPT_UI_RESET_MACHINE,		0, "Reset Game",		{ SEQ_DEF_1(KEYCODE_F3) } },
	{ IPT_UI_SHOW_GFX,			0, "Show Gfx",			{ SEQ_DEF_1(KEYCODE_F4) } },
	{ IPT_UI_FRAMESKIP_DEC,		0, "Frameskip Dec",		{ SEQ_DEF_1(KEYCODE_F8) } },
	{ IPT_UI_FRAMESKIP_INC,		0, "Frameskip Inc",		{ SEQ_DEF_1(KEYCODE_F9) } },
	{ IPT_UI_THROTTLE,			0, "Throttle",			{ SEQ_DEF_1(KEYCODE_F10) } },
	{ IPT_UI_SHOW_FPS,			0, "Show FPS",			{ SEQ_DEF_5(KEYCODE_F11, CODE_NOT, KEYCODE_LCONTROL, CODE_NOT, KEYCODE_LSHIFT) } },
	{ IPT_UI_SHOW_PROFILER,		0, "Show Profiler",		{ SEQ_DEF_2(KEYCODE_F11, KEYCODE_LSHIFT) } },
#ifdef MESS
	{ IPT_UI_TOGGLE_UI,			0, "UI Toggle",			{ SEQ_DEF_1(KEYCODE_SCRLOCK) } },
#endif
	{ IPT_UI_SNAPSHOT,			0, "Save Snapshot",		{ SEQ_DEF_1(KEYCODE_F12) } },
	{ IPT_UI_TOGGLE_CHEAT,		0, "Toggle Cheat",		{ SEQ_DEF_1(KEYCODE_F6) } },
	{ IPT_UI_UP,				0, "UI Up",				{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP) } },
	{ IPT_UI_DOWN,				0, "UI Down",			{ SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_UI_LEFT,				0, "UI Left",			{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT) } },
	{ IPT_UI_RIGHT,				0, "UI Right",			{ SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_UI_SELECT,			0, "UI Select",			{ SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_1_BUTTON1) } },
	{ IPT_UI_CANCEL,			0, "UI Cancel",			{ SEQ_DEF_1(KEYCODE_ESC) } },
	{ IPT_UI_PAN_UP,			0, "Pan Up",			{ SEQ_DEF_3(KEYCODE_PGUP, CODE_NOT, KEYCODE_LSHIFT) } },
	{ IPT_UI_PAN_DOWN,			0, "Pan Down",			{ SEQ_DEF_3(KEYCODE_PGDN, CODE_NOT, KEYCODE_LSHIFT) } },
	{ IPT_UI_PAN_LEFT,			0, "Pan Left",			{ SEQ_DEF_2(KEYCODE_PGUP, KEYCODE_LSHIFT) } },
	{ IPT_UI_PAN_RIGHT,			0, "Pan Right",			{ SEQ_DEF_2(KEYCODE_PGDN, KEYCODE_LSHIFT) } },
	{ IPT_UI_TOGGLE_DEBUG,      0, "Toggle Debugger",	{ SEQ_DEF_1(KEYCODE_F5) } },
	{ IPT_UI_SAVE_STATE,        0, "Save State",		{ SEQ_DEF_2(KEYCODE_F7, KEYCODE_LSHIFT) } },
	{ IPT_UI_LOAD_STATE,        0, "Load State",		{ SEQ_DEF_3(KEYCODE_F7, CODE_NOT, KEYCODE_LSHIFT) } },
	{ IPT_UI_ADD_CHEAT,			0, "Add Cheat",			{ SEQ_DEF_1(KEYCODE_A) } },
	{ IPT_UI_DELETE_CHEAT,		0, "Delete Cheat",		{ SEQ_DEF_1(KEYCODE_D) } },
	{ IPT_UI_SAVE_CHEAT,		0, "Save Cheat",		{ SEQ_DEF_1(KEYCODE_S) } },
	{ IPT_UI_WATCH_VALUE,		0, "Watch Value",		{ SEQ_DEF_1(KEYCODE_W) } },
	{ IPT_UI_EDIT_CHEAT,		0, "Edit Cheat",		{ SEQ_DEF_1(KEYCODE_E) } },
	{ IPT_UI_TOGGLE_CROSSHAIR,	0, "Toggle Crosshair",	{ SEQ_DEF_1(KEYCODE_F1) } },
	{ IPT_START1,				0, "1 Player Start",	{ SEQ_DEF_3(KEYCODE_1, CODE_OR, JOYCODE_1_START) } },
	{ IPT_START2,				0, "2 Players Start",	{ SEQ_DEF_3(KEYCODE_2, CODE_OR, JOYCODE_2_START) } },
	{ IPT_START3,				0, "3 Players Start",	{ SEQ_DEF_3(KEYCODE_3, CODE_OR, JOYCODE_3_START) } },
	{ IPT_START4,				0, "4 Players Start",	{ SEQ_DEF_3(KEYCODE_4, CODE_OR, JOYCODE_4_START) } },
	{ IPT_START5,				0, "5 Players Start",	{ SEQ_DEF_0 } },
	{ IPT_START6,				0, "6 Players Start",	{ SEQ_DEF_0 } },
	{ IPT_START7,				0, "7 Players Start",	{ SEQ_DEF_0 } },
	{ IPT_START8,				0, "8 Players Start",	{ SEQ_DEF_0 } },
	{ IPT_COIN1,				0, "Coin 1",			{ SEQ_DEF_3(KEYCODE_5, CODE_OR, JOYCODE_1_SELECT) } },
	{ IPT_COIN2,				0, "Coin 2",			{ SEQ_DEF_3(KEYCODE_6, CODE_OR, JOYCODE_2_SELECT) } },
	{ IPT_COIN3,				0, "Coin 3",			{ SEQ_DEF_3(KEYCODE_7, CODE_OR, JOYCODE_3_SELECT) } },
	{ IPT_COIN4,				0, "Coin 4",			{ SEQ_DEF_3(KEYCODE_8, CODE_OR, JOYCODE_4_SELECT) } },
	{ IPT_COIN5,				0, "Coin 5",			{ SEQ_DEF_0 } },
	{ IPT_COIN6,				0, "Coin 6",			{ SEQ_DEF_0 } },
	{ IPT_COIN7,				0, "Coin 7",			{ SEQ_DEF_0 } },
	{ IPT_COIN8,				0, "Coin 8",			{ SEQ_DEF_0 } },
	{ IPT_SERVICE1,				0, "Service 1",    		{ SEQ_DEF_1(KEYCODE_9) } },
	{ IPT_SERVICE2,				0, "Service 2",    		{ SEQ_DEF_1(KEYCODE_0) } },
	{ IPT_SERVICE3,				0, "Service 3",     	{ SEQ_DEF_1(KEYCODE_MINUS) } },
	{ IPT_SERVICE4,				0, "Service 4",     	{ SEQ_DEF_1(KEYCODE_EQUALS) } },
#ifndef MESS
	{ IPT_TILT, 				0, "Tilt",			   	{ SEQ_DEF_1(KEYCODE_T) } },
#else
	{ IPT_TILT, 				0, "Tilt",			   	{ SEQ_DEF_0 } },
#endif

	{ IPT_JOYSTICK_UP,			1, "P1 Up",				{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP)    } },
	{ IPT_JOYSTICK_DOWN,        1, "P1 Down",        	{ SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN)  } },
	{ IPT_JOYSTICK_LEFT,        1, "P1 Left",        	{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT)  } },
	{ IPT_JOYSTICK_RIGHT,       1, "P1 Right",       	{ SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_BUTTON1,			    1, "P1 Button 1",    	{ SEQ_DEF_5(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1, CODE_OR, JOYCODE_MOUSE_1_BUTTON1) } },
	{ IPT_BUTTON2,			    1, "P1 Button 2",    	{ SEQ_DEF_5(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2, CODE_OR, JOYCODE_MOUSE_1_BUTTON3) } },
	{ IPT_BUTTON3,			    1, "P1 Button 3",    	{ SEQ_DEF_5(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3, CODE_OR, JOYCODE_MOUSE_1_BUTTON2) } },
	{ IPT_BUTTON4,			    1, "P1 Button 4",    	{ SEQ_DEF_3(KEYCODE_LSHIFT, CODE_OR, JOYCODE_1_BUTTON4) } },
	{ IPT_BUTTON5,			    1, "P1 Button 5",    	{ SEQ_DEF_3(KEYCODE_Z, CODE_OR, JOYCODE_1_BUTTON5) } },
	{ IPT_BUTTON6,			    1, "P1 Button 6",    	{ SEQ_DEF_3(KEYCODE_X, CODE_OR, JOYCODE_1_BUTTON6) } },
	{ IPT_BUTTON7,			    1, "P1 Button 7",    	{ SEQ_DEF_3(KEYCODE_C, CODE_OR, JOYCODE_1_BUTTON7) } },
	{ IPT_BUTTON8,			    1, "P1 Button 8",    	{ SEQ_DEF_3(KEYCODE_V, CODE_OR, JOYCODE_1_BUTTON8) } },
	{ IPT_BUTTON9,			    1, "P1 Button 9",    	{ SEQ_DEF_3(KEYCODE_B, CODE_OR, JOYCODE_1_BUTTON9) } },
	{ IPT_BUTTON10,			    1, "P1 Button 10",   	{ SEQ_DEF_3(KEYCODE_N, CODE_OR, JOYCODE_1_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     1, "P1 Right/Up",    	{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_1_BUTTON2) } },
	{ IPT_JOYSTICKRIGHT_DOWN,   1, "P1 Right/Down",  	{ SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_1_BUTTON3) } },
	{ IPT_JOYSTICKRIGHT_LEFT,   1, "P1 Right/Left",  	{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_1_BUTTON1) } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  1, "P1 Right/Right", 	{ SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_1_BUTTON4) } },
	{ IPT_JOYSTICKLEFT_UP,      1, "P1 Left/Up",     	{ SEQ_DEF_3(KEYCODE_E, CODE_OR, JOYCODE_1_UP) } },
	{ IPT_JOYSTICKLEFT_DOWN,    1, "P1 Left/Down",   	{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_JOYSTICKLEFT_LEFT,    1, "P1 Left/Left",   	{ SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_1_LEFT) } },
	{ IPT_JOYSTICKLEFT_RIGHT,   1, "P1 Left/Right",  	{ SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_1_RIGHT) } },

	{ IPT_JOYSTICK_UP,			2, "P2 Up",				{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP)    } },
	{ IPT_JOYSTICK_DOWN,        2, "P2 Down",        	{ SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN)  } },
	{ IPT_JOYSTICK_LEFT,        2, "P2 Left",        	{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT)  } },
	{ IPT_JOYSTICK_RIGHT,       2, "P2 Right",       	{ SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_BUTTON1,			    2, "P2 Button 1",    	{ SEQ_DEF_3(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1) } },
	{ IPT_BUTTON2,			    2, "P2 Button 2",    	{ SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2) } },
	{ IPT_BUTTON3,			    2, "P2 Button 3",    	{ SEQ_DEF_3(KEYCODE_Q, CODE_OR, JOYCODE_2_BUTTON3) } },
	{ IPT_BUTTON4,			    2, "P2 Button 4",    	{ SEQ_DEF_3(KEYCODE_W, CODE_OR, JOYCODE_2_BUTTON4) } },
	{ IPT_BUTTON5,			    2, "P2 Button 5",    	{ SEQ_DEF_1(JOYCODE_2_BUTTON5) } },
	{ IPT_BUTTON6,			    2, "P2 Button 6",    	{ SEQ_DEF_1(JOYCODE_2_BUTTON6) } },
	{ IPT_BUTTON7,			    2, "P2 Button 7",    	{ SEQ_DEF_1(JOYCODE_2_BUTTON7) } },
	{ IPT_BUTTON8,			    2, "P2 Button 8",    	{ SEQ_DEF_1(JOYCODE_2_BUTTON8) } },
	{ IPT_BUTTON9,			    2, "P2 Button 9",    	{ SEQ_DEF_1(JOYCODE_2_BUTTON9) } },
	{ IPT_BUTTON10,			    2, "P2 Button 10",   	{ SEQ_DEF_1(JOYCODE_2_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     2, "P2 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   2, "P2 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   2, "P2 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  2, "P2 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      2, "P2 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    2, "P2 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    2, "P2 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   2, "P2 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			3, "P3 Up",				{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP)    } },
	{ IPT_JOYSTICK_DOWN,        3, "P3 Down",        	{ SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN)  } },
	{ IPT_JOYSTICK_LEFT,        3, "P3 Left",        	{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT)  } },
	{ IPT_JOYSTICK_RIGHT,       3, "P3 Right",       	{ SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_BUTTON1,			    3, "P3 Button 1",    	{ SEQ_DEF_3(KEYCODE_RCONTROL, CODE_OR, JOYCODE_3_BUTTON1) } },
	{ IPT_BUTTON2,			    3, "P3 Button 2",    	{ SEQ_DEF_3(KEYCODE_RSHIFT, CODE_OR, JOYCODE_3_BUTTON2) } },
	{ IPT_BUTTON3,			    3, "P3 Button 3",    	{ SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_3_BUTTON3) } },
	{ IPT_BUTTON4,			    3, "P3 Button 4",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON4) } },
	{ IPT_BUTTON5,			    3, "P3 Button 5",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON5) } },
	{ IPT_BUTTON6,			    3, "P3 Button 6",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON6) } },
	{ IPT_BUTTON7,			    3, "P3 Button 7",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON7) } },
	{ IPT_BUTTON8,			    3, "P3 Button 8",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON8) } },
	{ IPT_BUTTON9,			    3, "P3 Button 9",    	{ SEQ_DEF_1(JOYCODE_3_BUTTON9) } },
	{ IPT_BUTTON10,			    3, "P3 Button 10",   	{ SEQ_DEF_1(JOYCODE_3_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     3, "P3 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   3, "P3 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   3, "P3 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  3, "P3 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      3, "P3 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    3, "P3 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    3, "P3 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   3, "P3 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			4, "P4 Up",				{ SEQ_DEF_3(KEYCODE_8_PAD, CODE_OR, JOYCODE_4_UP) } },
	{ IPT_JOYSTICK_DOWN,        4, "P4 Down",        	{ SEQ_DEF_3(KEYCODE_2_PAD, CODE_OR, JOYCODE_4_DOWN) } },
	{ IPT_JOYSTICK_LEFT,        4, "P4 Left",        	{ SEQ_DEF_3(KEYCODE_4_PAD, CODE_OR, JOYCODE_4_LEFT) } },
	{ IPT_JOYSTICK_RIGHT,       4, "P4 Right",       	{ SEQ_DEF_3(KEYCODE_6_PAD, CODE_OR, JOYCODE_4_RIGHT) } },
	{ IPT_BUTTON1,			    4, "P4 Button 1",    	{ SEQ_DEF_3(KEYCODE_0_PAD, CODE_OR, JOYCODE_4_BUTTON1) } },
	{ IPT_BUTTON2,			    4, "P4 Button 2",    	{ SEQ_DEF_3(KEYCODE_DEL_PAD, CODE_OR, JOYCODE_4_BUTTON2) } },
	{ IPT_BUTTON3,			    4, "P4 Button 3",    	{ SEQ_DEF_3(KEYCODE_ENTER_PAD, CODE_OR, JOYCODE_4_BUTTON3) } },
	{ IPT_BUTTON4,			    4, "P4 Button 4",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON4) } },
	{ IPT_BUTTON5,			    4, "P4 Button 5",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON5) } },
	{ IPT_BUTTON6,			    4, "P4 Button 6",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON6) } },
	{ IPT_BUTTON7,			    4, "P4 Button 7",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON7) } },
	{ IPT_BUTTON8,			    4, "P4 Button 8",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON8) } },
	{ IPT_BUTTON9,			    4, "P4 Button 9",    	{ SEQ_DEF_1(JOYCODE_4_BUTTON9) } },
	{ IPT_BUTTON10,			    4, "P4 Button 10",   	{ SEQ_DEF_1(JOYCODE_4_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     4, "P4 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   4, "P4 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   4, "P4 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  4, "P4 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      4, "P4 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    4, "P4 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    4, "P4 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   4, "P4 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			5, "P5 Up",				{ SEQ_DEF_1(JOYCODE_5_UP) } },
	{ IPT_JOYSTICK_DOWN,        5, "P5 Down",        	{ SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_JOYSTICK_LEFT,        5, "P5 Left",        	{ SEQ_DEF_1(JOYCODE_5_LEFT) } },
	{ IPT_JOYSTICK_RIGHT,       5, "P5 Right",       	{ SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_BUTTON1,			    5, "P5 Button 1",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON1) } },
	{ IPT_BUTTON2,			    5, "P5 Button 2",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON2) } },
	{ IPT_BUTTON3,			    5, "P5 Button 3",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON3) } },
	{ IPT_BUTTON4,			    5, "P5 Button 4",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON4) } },
	{ IPT_BUTTON5,			    5, "P5 Button 5",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON5) } },
	{ IPT_BUTTON6,			    5, "P5 Button 6",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON6) } },
	{ IPT_BUTTON7,			    5, "P5 Button 7",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON7) } },
	{ IPT_BUTTON8,			    5, "P5 Button 8",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON8) } },
	{ IPT_BUTTON9,			    5, "P5 Button 9",    	{ SEQ_DEF_1(JOYCODE_5_BUTTON9) } },
	{ IPT_BUTTON10,			    5, "P5 Button 10",   	{ SEQ_DEF_1(JOYCODE_5_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     5, "P5 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   5, "P5 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   5, "P5 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  5, "P5 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      5, "P5 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    5, "P5 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    5, "P5 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   5, "P5 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			6, "P6 Up",				{ SEQ_DEF_1(JOYCODE_6_UP) } },
	{ IPT_JOYSTICK_DOWN,        6, "P6 Down",        	{ SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_JOYSTICK_LEFT,        6, "P6 Left",        	{ SEQ_DEF_1(JOYCODE_6_LEFT) } },
	{ IPT_JOYSTICK_RIGHT,       6, "P6 Right",       	{ SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_BUTTON1,			    6, "P6 Button 1",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON1) } },
	{ IPT_BUTTON2,			    6, "P6 Button 2",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON2) } },
	{ IPT_BUTTON3,			    6, "P6 Button 3",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON3) } },
	{ IPT_BUTTON4,			    6, "P6 Button 4",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON4) } },
	{ IPT_BUTTON5,			    6, "P6 Button 5",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON5) } },
	{ IPT_BUTTON6,			    6, "P6 Button 6",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON6) } },
	{ IPT_BUTTON7,			    6, "P6 Button 7",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON7) } },
	{ IPT_BUTTON8,			    6, "P6 Button 8",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON8) } },
	{ IPT_BUTTON9,			    6, "P6 Button 9",    	{ SEQ_DEF_1(JOYCODE_6_BUTTON9) } },
	{ IPT_BUTTON10,			    6, "P6 Button 10",   	{ SEQ_DEF_1(JOYCODE_6_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     6, "P6 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   6, "P6 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   6, "P6 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  6, "P6 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      6, "P6 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    6, "P6 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    6, "P6 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   6, "P6 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			7, "P7 Up",				{ SEQ_DEF_1(JOYCODE_7_UP) } },
	{ IPT_JOYSTICK_DOWN,        7, "P7 Down",        	{ SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_JOYSTICK_LEFT,        7, "P7 Left",        	{ SEQ_DEF_1(JOYCODE_7_LEFT) } },
	{ IPT_JOYSTICK_RIGHT,       7, "P7 Right",       	{ SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_BUTTON1,			    7, "P7 Button 1",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON1) } },
	{ IPT_BUTTON2,			    7, "P7 Button 2",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON2) } },
	{ IPT_BUTTON3,			    7, "P7 Button 3",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON3) } },
	{ IPT_BUTTON4,			    7, "P7 Button 4",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON4) } },
	{ IPT_BUTTON5,			    7, "P7 Button 5",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON5) } },
	{ IPT_BUTTON6,			    7, "P7 Button 6",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON6) } },
	{ IPT_BUTTON7,			    7, "P7 Button 7",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON7) } },
	{ IPT_BUTTON8,			    7, "P7 Button 8",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON8) } },
	{ IPT_BUTTON9,			    7, "P7 Button 9",    	{ SEQ_DEF_1(JOYCODE_7_BUTTON9) } },
	{ IPT_BUTTON10,			    7, "P7 Button 10",   	{ SEQ_DEF_1(JOYCODE_7_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     7, "P7 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   7, "P7 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   7, "P7 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  7, "P7 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      7, "P7 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    7, "P7 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    7, "P7 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   7, "P7 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_JOYSTICK_UP,			8, "P8 Up",				{ SEQ_DEF_1(JOYCODE_8_UP) } },
	{ IPT_JOYSTICK_DOWN,        8, "P8 Down",        	{ SEQ_DEF_1(JOYCODE_8_DOWN) } },
	{ IPT_JOYSTICK_LEFT,        8, "P8 Left",        	{ SEQ_DEF_1(JOYCODE_8_LEFT) } },
	{ IPT_JOYSTICK_RIGHT,       8, "P8 Right",       	{ SEQ_DEF_1(JOYCODE_8_RIGHT) } },
	{ IPT_BUTTON1,			    8, "P8 Button 1",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON1) } },
	{ IPT_BUTTON2,			    8, "P8 Button 2",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON2) } },
	{ IPT_BUTTON3,			    8, "P8 Button 3",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON3) } },
	{ IPT_BUTTON4,			    8, "P8 Button 4",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON4) } },
	{ IPT_BUTTON5,			    8, "P8 Button 5",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON5) } },
	{ IPT_BUTTON6,			    8, "P8 Button 6",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON6) } },
	{ IPT_BUTTON7,			    8, "P8 Button 7",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON7) } },
	{ IPT_BUTTON8,			    8, "P8 Button 8",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON8) } },
	{ IPT_BUTTON9,			    8, "P8 Button 9",    	{ SEQ_DEF_1(JOYCODE_8_BUTTON9) } },
	{ IPT_BUTTON10,			    8, "P8 Button 10",   	{ SEQ_DEF_1(JOYCODE_8_BUTTON10) } },
	{ IPT_JOYSTICKRIGHT_UP,     8, "P8 Right/Up",    	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_DOWN,   8, "P8 Right/Down",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_LEFT,   8, "P8 Right/Left",  	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKRIGHT_RIGHT,  8, "P8 Right/Right", 	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_UP,      8, "P8 Left/Up",     	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_DOWN,    8, "P8 Left/Down",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_LEFT,    8, "P8 Left/Left",   	{ SEQ_DEF_0 } },
	{ IPT_JOYSTICKLEFT_RIGHT,   8, "P8 Left/Right",  	{ SEQ_DEF_0 } },

	{ IPT_PEDAL,				1, "P1 Pedal 1",     	{ SEQ_DEF_3(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				2, "P2 Pedal 1", 		{ SEQ_DEF_3(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				3, "P3 Pedal 1",		{ SEQ_DEF_3(KEYCODE_RCONTROL, CODE_OR, JOYCODE_3_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				4, "P4 Pedal 1",		{ SEQ_DEF_1(JOYCODE_4_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				5, "P5 Pedal 1",		{ SEQ_DEF_1(JOYCODE_5_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				6, "P6 Pedal 1",		{ SEQ_DEF_1(JOYCODE_6_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				7, "P7 Pedal 1",		{ SEQ_DEF_1(JOYCODE_7_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL,				8, "P8 Pedal 1",		{ SEQ_DEF_1(JOYCODE_8_BUTTON1), SEQ_DEF_1(KEYCODE_Y) } },

	{ IPT_PEDAL2,				1, "P1 Pedal 2",		{ SEQ_DEF_1(JOYCODE_1_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				2, "P2 Pedal 2",		{ SEQ_DEF_1(JOYCODE_2_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				3, "P3 Pedal 2",		{ SEQ_DEF_1(JOYCODE_3_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				4, "P4 Pedal 2",		{ SEQ_DEF_1(JOYCODE_4_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				5, "P5 Pedal 2",		{ SEQ_DEF_1(JOYCODE_5_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				6, "P6 Pedal 2",		{ SEQ_DEF_1(JOYCODE_6_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				7, "P7 Pedal 2",		{ SEQ_DEF_1(JOYCODE_7_DOWN), SEQ_DEF_1(KEYCODE_Y) } },
	{ IPT_PEDAL2,				8, "P8 Pedal 2",		{ SEQ_DEF_1(JOYCODE_8_DOWN), SEQ_DEF_1(KEYCODE_Y) } },

	{ IPT_PADDLE, 				1, "Paddle",   	    	{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_PADDLE,				2, "Paddle 2",      	{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_PADDLE,				3, "Paddle 3",      	{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_PADDLE,				4, "Paddle 4",      	{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_PADDLE,				5, "Paddle 5",      	{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_PADDLE,				6, "Paddle 6",      	{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_PADDLE,				7, "Paddle 7",      	{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_PADDLE,				8, "Paddle 8",      	{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_PADDLE_V,				1, "Paddle V",			{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_PADDLE_V,				2, "Paddle V 2",		{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_PADDLE_V,				3, "Paddle V 3",		{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_PADDLE_V,				4, "Paddle V 4",		{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_PADDLE_V,				5, "Paddle V 5",		{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_PADDLE_V,				6, "Paddle V 6",		{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_PADDLE_V,				7, "Paddle V 7",		{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_PADDLE_V,				8, "Paddle V 8",		{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },

	{ IPT_DIAL,					1, "Dial",				{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_DIAL,					2, "Dial 2",			{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_DIAL,					3, "Dial 3",			{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_DIAL,					4, "Dial 4",			{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_DIAL,					5, "Dial 5",			{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_DIAL,					6, "Dial 6",			{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_DIAL,					7, "Dial 7",			{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_DIAL,					8, "Dial 8",			{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_DIAL_V,				1, "Dial V",			{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_DIAL_V,				2, "Dial V 2",			{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_DIAL_V,				3, "Dial V 3",			{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_DIAL_V,				4, "Dial V 4",			{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_DIAL_V,				5, "Dial V 5",			{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_DIAL_V,				6, "Dial V 6",			{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_DIAL_V,				7, "Dial V 7",			{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_DIAL_V,				8, "Dial V 8",			{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },

	{ IPT_TRACKBALL_X,			1, "Track X",			{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_TRACKBALL_X,			2, "Track X 2",			{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_TRACKBALL_X,			3, "Track X 3",			{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_TRACKBALL_X,			4, "Track X 4",			{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_TRACKBALL_X,			5, "Track X 5",			{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_TRACKBALL_X,			6, "Track X 6",			{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_TRACKBALL_X,			7, "Track X 7",			{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_TRACKBALL_X,			8, "Track X 8",			{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_TRACKBALL_Y,			1, "Track Y",			{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_TRACKBALL_Y,			2, "Track Y 2",			{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_TRACKBALL_Y,			3, "Track Y 3",			{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_TRACKBALL_Y,			4, "Track Y 4",			{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_TRACKBALL_Y,			5, "Track Y 5",			{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_TRACKBALL_Y,			6, "Track Y 6",			{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_TRACKBALL_Y,			7, "Track Y 7",			{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_TRACKBALL_Y,			8, "Track Y 8",			{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },

	{ IPT_AD_STICK_X,			1, "AD Stick X",		{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_AD_STICK_X,			2, "AD Stick X 2",		{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_AD_STICK_X,			3, "AD Stick X 3",		{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_AD_STICK_X,			4, "AD Stick X 4",		{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_AD_STICK_X,			5, "AD Stick X 5",		{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_AD_STICK_X,			6, "AD Stick X 6",		{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_AD_STICK_X,			7, "AD Stick X 7",		{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_AD_STICK_X,			8, "AD Stick X 8",		{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_AD_STICK_Y,			1, "AD Stick Y",		{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_AD_STICK_Y,			2, "AD Stick Y 2",		{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_AD_STICK_Y,			3, "AD Stick Y 3",		{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_AD_STICK_Y,			4, "AD Stick Y 4",		{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_AD_STICK_Y,			5, "AD Stick Y 5",		{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_AD_STICK_Y,			6, "AD Stick Y 6",		{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_AD_STICK_Y,			7, "AD Stick Y 7",		{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_AD_STICK_Y,			8, "AD Stick Y 8",		{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },

	{ IPT_AD_STICK_Z,			1, "AD Stick Z",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			2, "AD Stick Z 2",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			3, "AD Stick Z 3",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			4, "AD Stick Z 4",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			5, "AD Stick Z 5",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			6, "AD Stick Z 6",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			7, "AD Stick Z 7",		{ SEQ_DEF_0, SEQ_DEF_0 } },
	{ IPT_AD_STICK_Z,			8, "AD Stick Z 8",		{ SEQ_DEF_0, SEQ_DEF_0 } },

	{ IPT_LIGHTGUN_X,			1, "Lightgun X",		{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_LIGHTGUN_X,			2, "Lightgun X 2",		{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_LIGHTGUN_X,			3, "Lightgun X 3",		{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_LIGHTGUN_X,			4, "Lightgun X 4",		{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_LIGHTGUN_X,			5, "Lightgun X 5",		{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_LIGHTGUN_X,			6, "Lightgun X 6",		{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_LIGHTGUN_X,			7, "Lightgun X 7",		{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_LIGHTGUN_X,			8, "Lightgun X 8",		{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_LIGHTGUN_Y,			1, "Lightgun Y",		{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_LIGHTGUN_Y,			2, "Lightgun Y 2",		{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_LIGHTGUN_Y,			3, "Lightgun Y 3",		{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_LIGHTGUN_Y,			4, "Lightgun Y 4",		{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_LIGHTGUN_Y,			5, "Lightgun Y 5",		{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_LIGHTGUN_Y,			6, "Lightgun Y 6",		{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_LIGHTGUN_Y,			7, "Lightgun Y 7",		{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_LIGHTGUN_Y,			8, "Lightgun Y 8",		{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },

#ifdef MESS
	{ IPT_MOUSE_X,				1, "Mouse X",			{ SEQ_DEF_3(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT), SEQ_DEF_3(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT) } },
	{ IPT_MOUSE_X,				2, "Mouse X 2",			{ SEQ_DEF_3(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT), SEQ_DEF_3(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT) } },
	{ IPT_MOUSE_X,				3, "Mouse X 3",			{ SEQ_DEF_3(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT), SEQ_DEF_3(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT) } },
	{ IPT_MOUSE_X,				4, "Mouse X 4",			{ SEQ_DEF_1(JOYCODE_4_LEFT), SEQ_DEF_1(JOYCODE_4_RIGHT) } },
	{ IPT_MOUSE_X,				5, "Mouse X 5",			{ SEQ_DEF_1(JOYCODE_5_LEFT), SEQ_DEF_1(JOYCODE_5_RIGHT) } },
	{ IPT_MOUSE_X,				6, "Mouse X 6",			{ SEQ_DEF_1(JOYCODE_6_LEFT), SEQ_DEF_1(JOYCODE_6_RIGHT) } },
	{ IPT_MOUSE_X,				7, "Mouse X 7",			{ SEQ_DEF_1(JOYCODE_7_LEFT), SEQ_DEF_1(JOYCODE_7_RIGHT) } },
	{ IPT_MOUSE_X,				8, "Mouse X 8",			{ SEQ_DEF_1(JOYCODE_8_LEFT), SEQ_DEF_1(JOYCODE_8_RIGHT) } },

	{ IPT_MOUSE_Y,				1, "Mouse Y",			{ SEQ_DEF_3(KEYCODE_UP, CODE_OR, JOYCODE_1_UP), SEQ_DEF_3(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN) } },
	{ IPT_MOUSE_Y,				2, "Mouse Y 2",			{ SEQ_DEF_3(KEYCODE_R, CODE_OR, JOYCODE_2_UP), SEQ_DEF_3(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN) } },
	{ IPT_MOUSE_Y,				3, "Mouse Y 3",			{ SEQ_DEF_3(KEYCODE_I, CODE_OR, JOYCODE_3_UP), SEQ_DEF_3(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN) } },
	{ IPT_MOUSE_Y,				4, "Mouse Y 4",			{ SEQ_DEF_1(JOYCODE_4_UP), SEQ_DEF_1(JOYCODE_4_DOWN) } },
	{ IPT_MOUSE_Y,				5, "Mouse Y 5",			{ SEQ_DEF_1(JOYCODE_5_UP), SEQ_DEF_1(JOYCODE_5_DOWN) } },
	{ IPT_MOUSE_Y,				6, "Mouse Y 6",			{ SEQ_DEF_1(JOYCODE_6_UP), SEQ_DEF_1(JOYCODE_6_DOWN) } },
	{ IPT_MOUSE_Y,				7, "Mouse Y 7",			{ SEQ_DEF_1(JOYCODE_7_UP), SEQ_DEF_1(JOYCODE_7_DOWN) } },
	{ IPT_MOUSE_Y,				8, "Mouse Y 8",			{ SEQ_DEF_1(JOYCODE_8_UP), SEQ_DEF_1(JOYCODE_8_DOWN) } },
#endif

	{ IPT_UNKNOWN,				0, "UNKNOWN",			{ SEQ_DEF_0 } },
	{ IPT_OSD_RESERVED,			0, "",					{ SEQ_DEF_0 } },
	{ IPT_OSD_RESERVED,			0, "",					{ SEQ_DEF_0 } },
	{ IPT_OSD_RESERVED,			0, "",					{ SEQ_DEF_0 } },
	{ IPT_OSD_RESERVED,			0, "",					{ SEQ_DEF_0 } },
	{ IPT_END,					0, NULL,				{ SEQ_DEF_0 } }	/* returned when there is no match */
};

struct ipd inputport_defaults_backup[sizeof(inputport_defaults)/sizeof(struct ipd)];


struct ik *osd_input_keywords = NULL;

struct ik input_keywords[] =
{
	{ "KEYCODE_A",		   		IKT_STD,		KEYCODE_A },
	{ "KEYCODE_B",		   		IKT_STD,		KEYCODE_B },
	{ "KEYCODE_C",		   		IKT_STD,		KEYCODE_C },
	{ "KEYCODE_D",		   		IKT_STD,		KEYCODE_D },
	{ "KEYCODE_E",		   		IKT_STD,		KEYCODE_E },
	{ "KEYCODE_F",		   		IKT_STD,		KEYCODE_F },
	{ "KEYCODE_G",		   		IKT_STD,		KEYCODE_G },
	{ "KEYCODE_H",		   		IKT_STD,		KEYCODE_H },
	{ "KEYCODE_I",		   		IKT_STD,		KEYCODE_I },
	{ "KEYCODE_J",		   		IKT_STD,		KEYCODE_J },
	{ "KEYCODE_K",		   		IKT_STD,		KEYCODE_K },
	{ "KEYCODE_L",		   		IKT_STD,		KEYCODE_L },
	{ "KEYCODE_M",		   		IKT_STD,		KEYCODE_M },
	{ "KEYCODE_N",		   		IKT_STD,		KEYCODE_N },
	{ "KEYCODE_O",		   		IKT_STD,		KEYCODE_O },
	{ "KEYCODE_P",		   		IKT_STD,		KEYCODE_P },
	{ "KEYCODE_Q",		   		IKT_STD,		KEYCODE_Q },
	{ "KEYCODE_R",		   		IKT_STD,		KEYCODE_R },
	{ "KEYCODE_S",		   		IKT_STD,		KEYCODE_S },
	{ "KEYCODE_T",		   		IKT_STD,		KEYCODE_T },
	{ "KEYCODE_U",		   		IKT_STD,		KEYCODE_U },
	{ "KEYCODE_V",		   		IKT_STD,		KEYCODE_V },
	{ "KEYCODE_W",		   		IKT_STD,		KEYCODE_W },
	{ "KEYCODE_X",		   		IKT_STD,		KEYCODE_X },
	{ "KEYCODE_Y",		   		IKT_STD,		KEYCODE_Y },
	{ "KEYCODE_Z",		   		IKT_STD,		KEYCODE_Z },
	{ "KEYCODE_0",		   		IKT_STD,		KEYCODE_0 },
	{ "KEYCODE_1",		   		IKT_STD,		KEYCODE_1 },
	{ "KEYCODE_2",		   		IKT_STD,		KEYCODE_2 },
	{ "KEYCODE_3",		   		IKT_STD,		KEYCODE_3 },
	{ "KEYCODE_4",		   		IKT_STD,		KEYCODE_4 },
	{ "KEYCODE_5",		   		IKT_STD,		KEYCODE_5 },
	{ "KEYCODE_6",		   		IKT_STD,		KEYCODE_6 },
	{ "KEYCODE_7",		   		IKT_STD,		KEYCODE_7 },
	{ "KEYCODE_8",		   		IKT_STD,		KEYCODE_8 },
	{ "KEYCODE_9",		   		IKT_STD,		KEYCODE_9 },
	{ "KEYCODE_0_PAD",	   		IKT_STD,		KEYCODE_0_PAD },
	{ "KEYCODE_1_PAD",	   		IKT_STD,		KEYCODE_1_PAD },
	{ "KEYCODE_2_PAD",	   		IKT_STD,		KEYCODE_2_PAD },
	{ "KEYCODE_3_PAD",	   		IKT_STD,		KEYCODE_3_PAD },
	{ "KEYCODE_4_PAD",	   		IKT_STD,		KEYCODE_4_PAD },
	{ "KEYCODE_5_PAD",	   		IKT_STD,		KEYCODE_5_PAD },
	{ "KEYCODE_6_PAD",	   		IKT_STD,		KEYCODE_6_PAD },
	{ "KEYCODE_7_PAD",	   		IKT_STD,		KEYCODE_7_PAD },
	{ "KEYCODE_8_PAD",	      	IKT_STD,		KEYCODE_8_PAD },
	{ "KEYCODE_9_PAD",	      	IKT_STD,		KEYCODE_9_PAD },
	{ "KEYCODE_F1",		   		IKT_STD,		KEYCODE_F1 },
	{ "KEYCODE_F2",			  	IKT_STD,		KEYCODE_F2 },
	{ "KEYCODE_F3",			  	IKT_STD,		KEYCODE_F3 },
	{ "KEYCODE_F4",			  	IKT_STD,		KEYCODE_F4 },
	{ "KEYCODE_F5",			  	IKT_STD,		KEYCODE_F5 },
	{ "KEYCODE_F6",			  	IKT_STD,		KEYCODE_F6 },
	{ "KEYCODE_F7",			  	IKT_STD,		KEYCODE_F7 },
	{ "KEYCODE_F8",			  	IKT_STD,		KEYCODE_F8 },
	{ "KEYCODE_F9",			  	IKT_STD,		KEYCODE_F9 },
	{ "KEYCODE_F10",		  	IKT_STD,		KEYCODE_F10 },
	{ "KEYCODE_F11",		  	IKT_STD,		KEYCODE_F11 },
	{ "KEYCODE_F12",		  	IKT_STD,		KEYCODE_F12 },
	{ "KEYCODE_ESC",		  	IKT_STD,		KEYCODE_ESC },
	{ "KEYCODE_TILDE",		  	IKT_STD,		KEYCODE_TILDE },
	{ "KEYCODE_MINUS",		  	IKT_STD,		KEYCODE_MINUS },
	{ "KEYCODE_EQUALS",		  	IKT_STD,		KEYCODE_EQUALS },
	{ "KEYCODE_BACKSPACE",	  	IKT_STD,		KEYCODE_BACKSPACE },
	{ "KEYCODE_TAB",		  	IKT_STD,		KEYCODE_TAB },
	{ "KEYCODE_OPENBRACE",	  	IKT_STD,		KEYCODE_OPENBRACE },
	{ "KEYCODE_CLOSEBRACE",	  	IKT_STD,		KEYCODE_CLOSEBRACE },
	{ "KEYCODE_ENTER",		  	IKT_STD,		KEYCODE_ENTER },
	{ "KEYCODE_COLON",		  	IKT_STD,		KEYCODE_COLON },
	{ "KEYCODE_QUOTE",		  	IKT_STD,		KEYCODE_QUOTE },
	{ "KEYCODE_BACKSLASH",	  	IKT_STD,		KEYCODE_BACKSLASH },
	{ "KEYCODE_BACKSLASH2",	  	IKT_STD,		KEYCODE_BACKSLASH2 },
	{ "KEYCODE_COMMA",		  	IKT_STD,		KEYCODE_COMMA },
	{ "KEYCODE_STOP",		  	IKT_STD,		KEYCODE_STOP },
	{ "KEYCODE_SLASH",		  	IKT_STD,		KEYCODE_SLASH },
	{ "KEYCODE_SPACE",		  	IKT_STD,		KEYCODE_SPACE },
	{ "KEYCODE_INSERT",		  	IKT_STD,		KEYCODE_INSERT },
	{ "KEYCODE_DEL",		  	IKT_STD,		KEYCODE_DEL },
	{ "KEYCODE_HOME",		  	IKT_STD,		KEYCODE_HOME },
	{ "KEYCODE_END",		  	IKT_STD,		KEYCODE_END },
	{ "KEYCODE_PGUP",		  	IKT_STD,		KEYCODE_PGUP },
	{ "KEYCODE_PGDN",		  	IKT_STD,		KEYCODE_PGDN },
	{ "KEYCODE_LEFT",		  	IKT_STD,		KEYCODE_LEFT },
	{ "KEYCODE_RIGHT",		  	IKT_STD,		KEYCODE_RIGHT },
	{ "KEYCODE_UP",			  	IKT_STD,		KEYCODE_UP },
	{ "KEYCODE_DOWN",		  	IKT_STD,		KEYCODE_DOWN },
	{ "KEYCODE_SLASH_PAD",	  	IKT_STD,		KEYCODE_SLASH_PAD },
	{ "KEYCODE_ASTERISK",	  	IKT_STD,		KEYCODE_ASTERISK },
	{ "KEYCODE_MINUS_PAD",	  	IKT_STD,		KEYCODE_MINUS_PAD },
	{ "KEYCODE_PLUS_PAD",	  	IKT_STD,		KEYCODE_PLUS_PAD },
	{ "KEYCODE_DEL_PAD",	  	IKT_STD,		KEYCODE_DEL_PAD },
	{ "KEYCODE_ENTER_PAD",	  	IKT_STD,		KEYCODE_ENTER_PAD },
	{ "KEYCODE_PRTSCR",		  	IKT_STD,		KEYCODE_PRTSCR },
	{ "KEYCODE_PAUSE",		  	IKT_STD,		KEYCODE_PAUSE },
	{ "KEYCODE_LSHIFT",		  	IKT_STD,		KEYCODE_LSHIFT },
	{ "KEYCODE_RSHIFT",		  	IKT_STD,		KEYCODE_RSHIFT },
	{ "KEYCODE_LCONTROL",	  	IKT_STD,		KEYCODE_LCONTROL },
	{ "KEYCODE_RCONTROL",	  	IKT_STD,		KEYCODE_RCONTROL },
	{ "KEYCODE_LALT",		  	IKT_STD,		KEYCODE_LALT },
	{ "KEYCODE_RALT",		  	IKT_STD,		KEYCODE_RALT },
	{ "KEYCODE_SCRLOCK",	  	IKT_STD,		KEYCODE_SCRLOCK },
	{ "KEYCODE_NUMLOCK",	  	IKT_STD,		KEYCODE_NUMLOCK },
	{ "KEYCODE_CAPSLOCK",	  	IKT_STD,		KEYCODE_CAPSLOCK },
	{ "KEYCODE_LWIN",		  	IKT_STD,		KEYCODE_LWIN },
	{ "KEYCODE_RWIN",		  	IKT_STD,		KEYCODE_RWIN },
	{ "KEYCODE_MENU",		  	IKT_STD,		KEYCODE_MENU },

	{ "JOYCODE_1_LEFT",		  	IKT_STD,		JOYCODE_1_LEFT },
	{ "JOYCODE_1_RIGHT",	  	IKT_STD,		JOYCODE_1_RIGHT },
	{ "JOYCODE_1_UP",		  	IKT_STD,		JOYCODE_1_UP },
	{ "JOYCODE_1_DOWN",		  	IKT_STD,		JOYCODE_1_DOWN },
	{ "JOYCODE_1_BUTTON1",	  	IKT_STD,		JOYCODE_1_BUTTON1 },
	{ "JOYCODE_1_BUTTON2",	  	IKT_STD,		JOYCODE_1_BUTTON2 },
	{ "JOYCODE_1_BUTTON3",	  	IKT_STD,		JOYCODE_1_BUTTON3 },
	{ "JOYCODE_1_BUTTON4",	  	IKT_STD,		JOYCODE_1_BUTTON4 },
	{ "JOYCODE_1_BUTTON5",	  	IKT_STD,		JOYCODE_1_BUTTON5 },
	{ "JOYCODE_1_BUTTON6",	  	IKT_STD,		JOYCODE_1_BUTTON6 },
	{ "JOYCODE_1_BUTTON7",	  	IKT_STD,		JOYCODE_1_BUTTON7 },
	{ "JOYCODE_1_BUTTON8",	  	IKT_STD,		JOYCODE_1_BUTTON8 },
	{ "JOYCODE_1_BUTTON9",	  	IKT_STD,		JOYCODE_1_BUTTON9 },
	{ "JOYCODE_1_BUTTON10",	  	IKT_STD,		JOYCODE_1_BUTTON10 },
	{ "JOYCODE_1_START",	  	IKT_STD,		JOYCODE_1_START },
	{ "JOYCODE_1_SELECT",	  	IKT_STD,		JOYCODE_1_SELECT },
	{ "JOYCODE_2_LEFT",		  	IKT_STD,		JOYCODE_2_LEFT },
	{ "JOYCODE_2_RIGHT",	  	IKT_STD,		JOYCODE_2_RIGHT },
	{ "JOYCODE_2_UP",		  	IKT_STD,		JOYCODE_2_UP },
	{ "JOYCODE_2_DOWN",		  	IKT_STD,		JOYCODE_2_DOWN },
	{ "JOYCODE_2_BUTTON1",	  	IKT_STD,		JOYCODE_2_BUTTON1 },
	{ "JOYCODE_2_BUTTON2",	  	IKT_STD,		JOYCODE_2_BUTTON2 },
	{ "JOYCODE_2_BUTTON3",	  	IKT_STD,		JOYCODE_2_BUTTON3 },
	{ "JOYCODE_2_BUTTON4",	  	IKT_STD,		JOYCODE_2_BUTTON4 },
	{ "JOYCODE_2_BUTTON5",	  	IKT_STD,		JOYCODE_2_BUTTON5 },
	{ "JOYCODE_2_BUTTON6",	  	IKT_STD,		JOYCODE_2_BUTTON6 },
	{ "JOYCODE_2_BUTTON7",	  	IKT_STD,		JOYCODE_2_BUTTON7 },
	{ "JOYCODE_2_BUTTON8",	  	IKT_STD,		JOYCODE_2_BUTTON8 },
	{ "JOYCODE_2_BUTTON9",	  	IKT_STD,		JOYCODE_2_BUTTON9 },
	{ "JOYCODE_2_BUTTON10",	  	IKT_STD,		JOYCODE_2_BUTTON10 },
	{ "JOYCODE_2_START",	  	IKT_STD,		JOYCODE_2_START },
	{ "JOYCODE_2_SELECT",	  	IKT_STD,		JOYCODE_2_SELECT },
	{ "JOYCODE_3_LEFT",		  	IKT_STD,		JOYCODE_3_LEFT },
	{ "JOYCODE_3_RIGHT",	  	IKT_STD,		JOYCODE_3_RIGHT },
	{ "JOYCODE_3_UP",		  	IKT_STD,		JOYCODE_3_UP },
	{ "JOYCODE_3_DOWN",		  	IKT_STD,		JOYCODE_3_DOWN },
	{ "JOYCODE_3_BUTTON1",	  	IKT_STD,		JOYCODE_3_BUTTON1 },
	{ "JOYCODE_3_BUTTON2",	  	IKT_STD,		JOYCODE_3_BUTTON2 },
	{ "JOYCODE_3_BUTTON3",	  	IKT_STD,		JOYCODE_3_BUTTON3 },
	{ "JOYCODE_3_BUTTON4",	  	IKT_STD,		JOYCODE_3_BUTTON4 },
	{ "JOYCODE_3_BUTTON5",	  	IKT_STD,		JOYCODE_3_BUTTON5 },
	{ "JOYCODE_3_BUTTON6",	  	IKT_STD,		JOYCODE_3_BUTTON6 },
	{ "JOYCODE_3_BUTTON7",	  	IKT_STD,		JOYCODE_3_BUTTON7 },
	{ "JOYCODE_3_BUTTON8",	  	IKT_STD,		JOYCODE_3_BUTTON8 },
	{ "JOYCODE_3_BUTTON9",	  	IKT_STD,		JOYCODE_3_BUTTON9 },
	{ "JOYCODE_3_BUTTON10",	  	IKT_STD,		JOYCODE_3_BUTTON10 },
	{ "JOYCODE_3_START",	  	IKT_STD,		JOYCODE_3_START },
	{ "JOYCODE_3_SELECT",	  	IKT_STD,		JOYCODE_3_SELECT },
	{ "JOYCODE_4_LEFT",		  	IKT_STD,		JOYCODE_4_LEFT },
	{ "JOYCODE_4_RIGHT",	  	IKT_STD,		JOYCODE_4_RIGHT },
	{ "JOYCODE_4_UP",		  	IKT_STD,		JOYCODE_4_UP },
	{ "JOYCODE_4_DOWN",		  	IKT_STD,		JOYCODE_4_DOWN },
	{ "JOYCODE_4_BUTTON1",	  	IKT_STD,		JOYCODE_4_BUTTON1 },
	{ "JOYCODE_4_BUTTON2",	  	IKT_STD,		JOYCODE_4_BUTTON2 },
	{ "JOYCODE_4_BUTTON3",	  	IKT_STD,		JOYCODE_4_BUTTON3 },
	{ "JOYCODE_4_BUTTON4",	  	IKT_STD,		JOYCODE_4_BUTTON4 },
	{ "JOYCODE_4_BUTTON5",	  	IKT_STD,		JOYCODE_4_BUTTON5 },
	{ "JOYCODE_4_BUTTON6",	  	IKT_STD,		JOYCODE_4_BUTTON6 },
	{ "JOYCODE_4_BUTTON7",	  	IKT_STD,		JOYCODE_4_BUTTON7 },
	{ "JOYCODE_4_BUTTON8",	  	IKT_STD,		JOYCODE_4_BUTTON8 },
	{ "JOYCODE_4_BUTTON9",	  	IKT_STD,		JOYCODE_4_BUTTON9 },
	{ "JOYCODE_4_BUTTON10",	  	IKT_STD,		JOYCODE_4_BUTTON10 },
	{ "JOYCODE_4_START",	  	IKT_STD,		JOYCODE_4_START },
	{ "JOYCODE_4_SELECT",	  	IKT_STD,		JOYCODE_4_SELECT },
	{ "JOYCODE_5_LEFT",		  	IKT_STD,		JOYCODE_5_LEFT },
	{ "JOYCODE_5_RIGHT",	  	IKT_STD,		JOYCODE_5_RIGHT },
	{ "JOYCODE_5_UP",		  	IKT_STD,		JOYCODE_5_UP },
	{ "JOYCODE_5_DOWN",		  	IKT_STD,		JOYCODE_5_DOWN },
	{ "JOYCODE_5_BUTTON1",	  	IKT_STD,		JOYCODE_5_BUTTON1 },
	{ "JOYCODE_5_BUTTON2",	  	IKT_STD,		JOYCODE_5_BUTTON2 },
	{ "JOYCODE_5_BUTTON3",	  	IKT_STD,		JOYCODE_5_BUTTON3 },
	{ "JOYCODE_5_BUTTON4",	  	IKT_STD,		JOYCODE_5_BUTTON4 },
	{ "JOYCODE_5_BUTTON5",	  	IKT_STD,		JOYCODE_5_BUTTON5 },
	{ "JOYCODE_5_BUTTON6",	  	IKT_STD,		JOYCODE_5_BUTTON6 },
	{ "JOYCODE_5_BUTTON7",	  	IKT_STD,		JOYCODE_5_BUTTON7 },
	{ "JOYCODE_5_BUTTON8",	  	IKT_STD,		JOYCODE_5_BUTTON8 },
	{ "JOYCODE_5_BUTTON9",	  	IKT_STD,		JOYCODE_5_BUTTON9 },
	{ "JOYCODE_5_BUTTON10",	  	IKT_STD,		JOYCODE_5_BUTTON10 },
	{ "JOYCODE_5_START",	  	IKT_STD,		JOYCODE_5_START },
	{ "JOYCODE_5_SELECT",	  	IKT_STD,		JOYCODE_5_SELECT },
	{ "JOYCODE_6_LEFT",		  	IKT_STD,		JOYCODE_6_LEFT },
	{ "JOYCODE_6_RIGHT",	  	IKT_STD,		JOYCODE_6_RIGHT },
	{ "JOYCODE_6_UP",		  	IKT_STD,		JOYCODE_6_UP },
	{ "JOYCODE_6_DOWN",		  	IKT_STD,		JOYCODE_6_DOWN },
	{ "JOYCODE_6_BUTTON1",	  	IKT_STD,		JOYCODE_6_BUTTON1 },
	{ "JOYCODE_6_BUTTON2",	  	IKT_STD,		JOYCODE_6_BUTTON2 },
	{ "JOYCODE_6_BUTTON3",	  	IKT_STD,		JOYCODE_6_BUTTON3 },
	{ "JOYCODE_6_BUTTON4",	  	IKT_STD,		JOYCODE_6_BUTTON4 },
	{ "JOYCODE_6_BUTTON5",	  	IKT_STD,		JOYCODE_6_BUTTON5 },
	{ "JOYCODE_6_BUTTON6",	  	IKT_STD,		JOYCODE_6_BUTTON6 },
	{ "JOYCODE_6_BUTTON7",	  	IKT_STD,		JOYCODE_6_BUTTON7 },
	{ "JOYCODE_6_BUTTON8",	  	IKT_STD,		JOYCODE_6_BUTTON8 },
	{ "JOYCODE_6_BUTTON9",	  	IKT_STD,		JOYCODE_6_BUTTON9 },
	{ "JOYCODE_6_BUTTON10",	  	IKT_STD,		JOYCODE_6_BUTTON10 },
	{ "JOYCODE_6_START",	  	IKT_STD,		JOYCODE_6_START },
	{ "JOYCODE_6_SELECT",	  	IKT_STD,		JOYCODE_6_SELECT },
	{ "JOYCODE_7_LEFT",		  	IKT_STD,		JOYCODE_7_LEFT },
	{ "JOYCODE_7_RIGHT",	  	IKT_STD,		JOYCODE_7_RIGHT },
	{ "JOYCODE_7_UP",		  	IKT_STD,		JOYCODE_7_UP },
	{ "JOYCODE_7_DOWN",		  	IKT_STD,		JOYCODE_7_DOWN },
	{ "JOYCODE_7_BUTTON1",	  	IKT_STD,		JOYCODE_7_BUTTON1 },
	{ "JOYCODE_7_BUTTON2",	  	IKT_STD,		JOYCODE_7_BUTTON2 },
	{ "JOYCODE_7_BUTTON3",	  	IKT_STD,		JOYCODE_7_BUTTON3 },
	{ "JOYCODE_7_BUTTON4",	  	IKT_STD,		JOYCODE_7_BUTTON4 },
	{ "JOYCODE_7_BUTTON5",	  	IKT_STD,		JOYCODE_7_BUTTON5 },
	{ "JOYCODE_7_BUTTON6",	  	IKT_STD,		JOYCODE_7_BUTTON6 },
	{ "JOYCODE_7_BUTTON7",	  	IKT_STD,		JOYCODE_7_BUTTON7 },
	{ "JOYCODE_7_BUTTON8",	  	IKT_STD,		JOYCODE_7_BUTTON8 },
	{ "JOYCODE_7_BUTTON9",	  	IKT_STD,		JOYCODE_7_BUTTON9 },
	{ "JOYCODE_7_BUTTON10",	  	IKT_STD,		JOYCODE_7_BUTTON10 },
	{ "JOYCODE_7_START",	  	IKT_STD,		JOYCODE_7_START },
	{ "JOYCODE_7_SELECT",	  	IKT_STD,		JOYCODE_7_SELECT },
	{ "JOYCODE_8_LEFT",		  	IKT_STD,		JOYCODE_8_LEFT },
	{ "JOYCODE_8_RIGHT",	  	IKT_STD,		JOYCODE_8_RIGHT },
	{ "JOYCODE_8_UP",		  	IKT_STD,		JOYCODE_8_UP },
	{ "JOYCODE_8_DOWN",		  	IKT_STD,		JOYCODE_8_DOWN },
	{ "JOYCODE_8_BUTTON1",	  	IKT_STD,		JOYCODE_8_BUTTON1 },
	{ "JOYCODE_8_BUTTON2",	  	IKT_STD,		JOYCODE_8_BUTTON2 },
	{ "JOYCODE_8_BUTTON3",	  	IKT_STD,		JOYCODE_8_BUTTON3 },
	{ "JOYCODE_8_BUTTON4",	  	IKT_STD,		JOYCODE_8_BUTTON4 },
	{ "JOYCODE_8_BUTTON5",	  	IKT_STD,		JOYCODE_8_BUTTON5 },
	{ "JOYCODE_8_BUTTON6",	  	IKT_STD,		JOYCODE_8_BUTTON6 },
	{ "JOYCODE_8_BUTTON7",	  	IKT_STD,		JOYCODE_8_BUTTON7 },
	{ "JOYCODE_8_BUTTON8",	  	IKT_STD,		JOYCODE_8_BUTTON8 },
	{ "JOYCODE_8_BUTTON9",	  	IKT_STD,		JOYCODE_8_BUTTON9 },
	{ "JOYCODE_8_BUTTON10",	  	IKT_STD,		JOYCODE_8_BUTTON10 },
	{ "JOYCODE_8_START",	  	IKT_STD,		JOYCODE_8_START },
	{ "JOYCODE_8_SELECT",	  	IKT_STD,		JOYCODE_8_SELECT },

	{ "MOUSECODE_1_BUTTON1", 	IKT_STD,		JOYCODE_MOUSE_1_BUTTON1 },
	{ "MOUSECODE_1_BUTTON2", 	IKT_STD,		JOYCODE_MOUSE_1_BUTTON2 },
	{ "MOUSECODE_1_BUTTON3", 	IKT_STD,		JOYCODE_MOUSE_1_BUTTON3 },

	{ "KEYCODE_NONE",			IKT_STD,		CODE_NONE },
	{ "CODE_NONE",			  	IKT_STD,		CODE_NONE },
	{ "CODE_OTHER",				IKT_STD,		CODE_OTHER },
	{ "CODE_DEFAULT",			IKT_STD,		CODE_DEFAULT },
	{ "CODE_PREVIOUS",			IKT_STD,		CODE_PREVIOUS },
	{ "CODE_NOT",				IKT_STD,		CODE_NOT },
	{ "CODE_OR",			   	IKT_STD,		CODE_OR },
	{ "!",						IKT_STD,		CODE_NOT },
	{ "|",					   	IKT_STD,		CODE_OR },

	{ "UI_CONFIGURE", 			IKT_IPT,	 	IPT_UI_CONFIGURE },
	{ "UI_ON_SCREEN_DISPLAY",	IKT_IPT,		IPT_UI_ON_SCREEN_DISPLAY },
	{ "UI_PAUSE",				IKT_IPT,		IPT_UI_PAUSE },
	{ "UI_RESET_MACHINE",		IKT_IPT,		IPT_UI_RESET_MACHINE },
	{ "UI_SHOW_GFX",			IKT_IPT,		IPT_UI_SHOW_GFX },
	{ "UI_FRAMESKIP_DEC",		IKT_IPT,		IPT_UI_FRAMESKIP_DEC },
	{ "UI_FRAMESKIP_INC",		IKT_IPT,		IPT_UI_FRAMESKIP_INC },
	{ "UI_THROTTLE",			IKT_IPT,		IPT_UI_THROTTLE },
	{ "UI_SHOW_FPS",			IKT_IPT,		IPT_UI_SHOW_FPS },
	{ "UI_SHOW_PROFILER",		IKT_IPT,		IPT_UI_SHOW_PROFILER },
#ifdef MESS
	{ "UI_TOGGLE_UI",			IKT_IPT,		IPT_UI_TOGGLE_UI },
#endif
	{ "UI_SNAPSHOT",			IKT_IPT,		IPT_UI_SNAPSHOT },
	{ "UI_TOGGLE_CHEAT",		IKT_IPT,		IPT_UI_TOGGLE_CHEAT },
	{ "UI_UP",					IKT_IPT,		IPT_UI_UP },
	{ "UI_DOWN",				IKT_IPT,		IPT_UI_DOWN },
	{ "UI_LEFT",				IKT_IPT,		IPT_UI_LEFT },
	{ "UI_RIGHT",				IKT_IPT,		IPT_UI_RIGHT },
	{ "UI_SELECT",				IKT_IPT,		IPT_UI_SELECT },
	{ "UI_CANCEL",				IKT_IPT,		IPT_UI_CANCEL },
	{ "UI_PAN_UP",				IKT_IPT,		IPT_UI_PAN_UP },
	{ "UI_PAN_DOWN",			IKT_IPT,		IPT_UI_PAN_DOWN },
	{ "UI_PAN_LEFT",			IKT_IPT,		IPT_UI_PAN_LEFT },
	{ "UI_PAN_RIGHT",			IKT_IPT,		IPT_UI_PAN_RIGHT },
	{ "UI_TOGGLE_DEBUG",		IKT_IPT,		IPT_UI_TOGGLE_DEBUG },
	{ "UI_SAVE_STATE",			IKT_IPT,		IPT_UI_SAVE_STATE },
	{ "UI_LOAD_STATE",			IKT_IPT,		IPT_UI_LOAD_STATE },
	{ "UI_ADD_CHEAT",			IKT_IPT,		IPT_UI_ADD_CHEAT },
	{ "UI_DELETE_CHEAT",		IKT_IPT,		IPT_UI_DELETE_CHEAT },
	{ "UI_SAVE_CHEAT",			IKT_IPT,		IPT_UI_SAVE_CHEAT },
	{ "UI_WATCH_VALUE",			IKT_IPT,		IPT_UI_WATCH_VALUE },
	{ "UI_EDIT_CHEAT",			IKT_IPT,		IPT_UI_EDIT_CHEAT },
	{ "UI_TOGGLE_CROSSHAIR",	IKT_IPT,		IPT_UI_TOGGLE_CROSSHAIR },
	{ "START1",					IKT_IPT,		IPT_START1 },
	{ "START2",					IKT_IPT,		IPT_START2 },
	{ "START3",					IKT_IPT,		IPT_START3 },
	{ "START4",					IKT_IPT,		IPT_START4 },
	{ "START5",					IKT_IPT,		IPT_START5 },
	{ "START6",					IKT_IPT,		IPT_START6 },
	{ "START7",					IKT_IPT,		IPT_START7 },
	{ "START8",					IKT_IPT,		IPT_START8 },
	{ "COIN1",					IKT_IPT,		IPT_COIN1 },
	{ "COIN2",					IKT_IPT,		IPT_COIN2 },
	{ "COIN3",					IKT_IPT,		IPT_COIN3 },
	{ "COIN4",					IKT_IPT,		IPT_COIN4 },
	{ "COIN5",					IKT_IPT,		IPT_COIN5 },
	{ "COIN6",					IKT_IPT,		IPT_COIN6 },
	{ "COIN7",					IKT_IPT,		IPT_COIN7 },
	{ "COIN8",					IKT_IPT,		IPT_COIN8 },
	{ "SERVICE1",				IKT_IPT,		IPT_SERVICE1 },
	{ "SERVICE2",				IKT_IPT,		IPT_SERVICE2 },
	{ "SERVICE3",				IKT_IPT,		IPT_SERVICE3 },
	{ "SERVICE4",				IKT_IPT,		IPT_SERVICE4 },
	{ "TILT",					IKT_IPT,		IPT_TILT },

	{ "P1_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 1 },
	{ "P1_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 1 },
	{ "P1_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 1 },
	{ "P1_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 1 },
	{ "P1_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 1 },
	{ "P1_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 1 },
	{ "P1_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 1 },
	{ "P1_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 1 },
	{ "P1_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 1 },
	{ "P1_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 1 },
	{ "P1_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 1 },
	{ "P1_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 1 },
	{ "P1_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 1 },
	{ "P1_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 1 },
	{ "P1_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 1 },
	{ "P1_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 1 },
	{ "P1_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 1 },
	{ "P1_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 1 },
	{ "P1_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 1 },
	{ "P1_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 1 },
	{ "P1_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 1 },
	{ "P1_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 1 },

	{ "P2_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 2 },
	{ "P2_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 2 },
	{ "P2_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 2 },
	{ "P2_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 2 },
	{ "P2_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 2 },
	{ "P2_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 2 },
	{ "P2_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 2 },
	{ "P2_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 2 },
	{ "P2_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 2 },
	{ "P2_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 2 },
	{ "P2_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 2 },
	{ "P2_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 2 },
	{ "P2_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 2 },
	{ "P2_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 2 },
	{ "P2_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 2 },
	{ "P2_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 2 },
	{ "P2_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 2 },
	{ "P2_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 2 },
	{ "P2_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 2 },
	{ "P2_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 2 },
	{ "P2_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 2 },
	{ "P2_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 2 },

	{ "P3_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 3 },
	{ "P3_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 3 },
	{ "P3_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 3 },
	{ "P3_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 3 },
	{ "P3_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 3 },
	{ "P3_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 3 },
	{ "P3_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 3 },
	{ "P3_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 3 },
	{ "P3_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 3 },
	{ "P3_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 3 },
	{ "P3_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 3 },
	{ "P3_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 3 },
	{ "P3_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 3 },
	{ "P3_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 3 },
	{ "P3_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 3 },
	{ "P3_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 3 },
	{ "P3_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 3 },
	{ "P3_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 3 },
	{ "P3_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 3 },
	{ "P3_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 3 },
	{ "P3_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 3 },
	{ "P3_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 3 },

	{ "P4_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 4 },
	{ "P4_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 4 },
	{ "P4_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 4 },
	{ "P4_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 4 },
	{ "P4_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 4 },
	{ "P4_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 4 },
	{ "P4_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 4 },
	{ "P4_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 4 },
	{ "P4_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 4 },
	{ "P4_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 4 },
	{ "P4_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 4 },
	{ "P4_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 4 },
	{ "P4_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 4 },
	{ "P4_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 4 },
	{ "P4_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 4 },
	{ "P4_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 4 },
	{ "P4_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 4 },
	{ "P4_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 4 },
	{ "P4_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 4 },
	{ "P4_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 4 },
	{ "P4_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 4 },
	{ "P4_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 4 },

	{ "P5_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 5 },
	{ "P5_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 5 },
	{ "P5_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 5 },
	{ "P5_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 5 },
	{ "P5_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 5 },
	{ "P5_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 5 },
	{ "P5_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 5 },
	{ "P5_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 5 },
	{ "P5_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 5 },
	{ "P5_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 5 },
	{ "P5_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 5 },
	{ "P5_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 5 },
	{ "P5_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 5 },
	{ "P5_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 5 },
	{ "P5_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 5 },
	{ "P5_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 5 },
	{ "P5_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 5 },
	{ "P5_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 5 },
	{ "P5_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 5 },
	{ "P5_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 5 },
	{ "P5_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 5 },
	{ "P5_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 5 },

	{ "P6_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 6 },
	{ "P6_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 6 },
	{ "P6_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 6 },
	{ "P6_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 6 },
	{ "P6_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 6 },
	{ "P6_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 6 },
	{ "P6_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 6 },
	{ "P6_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 6 },
	{ "P6_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 6 },
	{ "P6_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 6 },
	{ "P6_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 6 },
	{ "P6_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 6 },
	{ "P6_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 6 },
	{ "P6_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 6 },
	{ "P6_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 6 },
	{ "P6_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 6 },
	{ "P6_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 6 },
	{ "P6_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 6 },
	{ "P6_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 6 },
	{ "P6_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 6 },
	{ "P6_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 6 },
	{ "P6_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 6 },

	{ "P7_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 7 },
	{ "P7_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 7 },
	{ "P7_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 7 },
	{ "P7_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 7 },
	{ "P7_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 7 },
	{ "P7_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 7 },
	{ "P7_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 7 },
	{ "P7_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 7 },
	{ "P7_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 7 },
	{ "P7_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 7 },
	{ "P7_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 7 },
	{ "P7_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 7 },
	{ "P7_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 7 },
	{ "P7_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 7 },
	{ "P7_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 7 },
	{ "P7_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 7 },
	{ "P7_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 7 },
	{ "P7_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 7 },
	{ "P7_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 7 },
	{ "P7_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 7 },
	{ "P7_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 7 },
	{ "P7_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 7 },

	{ "P8_JOYSTICK_UP",			IKT_IPT,		IPT_JOYSTICK_UP, 8 },
	{ "P8_JOYSTICK_DOWN",		IKT_IPT,		IPT_JOYSTICK_DOWN, 8 },
	{ "P8_JOYSTICK_LEFT",		IKT_IPT,		IPT_JOYSTICK_LEFT, 8 },
	{ "P8_JOYSTICK_RIGHT",		IKT_IPT,		IPT_JOYSTICK_RIGHT, 8 },
	{ "P8_BUTTON1",				IKT_IPT,		IPT_BUTTON1, 8 },
	{ "P8_BUTTON2",				IKT_IPT,		IPT_BUTTON2, 8 },
	{ "P8_BUTTON3",				IKT_IPT,		IPT_BUTTON3, 8 },
	{ "P8_BUTTON4",				IKT_IPT,		IPT_BUTTON4, 8 },
	{ "P8_BUTTON5",				IKT_IPT,		IPT_BUTTON5, 8 },
	{ "P8_BUTTON6",				IKT_IPT,		IPT_BUTTON6, 8 },
	{ "P8_BUTTON7",				IKT_IPT,		IPT_BUTTON7, 8 },
	{ "P8_BUTTON8",				IKT_IPT,		IPT_BUTTON8, 8 },
	{ "P8_BUTTON9",				IKT_IPT,		IPT_BUTTON9, 8 },
	{ "P8_BUTTON10",			IKT_IPT,		IPT_BUTTON10, 8 },
	{ "P8_JOYSTICKRIGHT_UP",	IKT_IPT,		IPT_JOYSTICKRIGHT_UP, 8 },
	{ "P8_JOYSTICKRIGHT_DOWN",	IKT_IPT,		IPT_JOYSTICKRIGHT_DOWN, 8 },
	{ "P8_JOYSTICKRIGHT_LEFT",	IKT_IPT,		IPT_JOYSTICKRIGHT_LEFT, 8 },
	{ "P8_JOYSTICKRIGHT_RIGHT",	IKT_IPT,		IPT_JOYSTICKRIGHT_RIGHT, 8 },
	{ "P8_JOYSTICKLEFT_UP",		IKT_IPT,		IPT_JOYSTICKLEFT_UP, 8 },
	{ "P8_JOYSTICKLEFT_DOWN",	IKT_IPT,		IPT_JOYSTICKLEFT_DOWN, 8 },
	{ "P8_JOYSTICKLEFT_LEFT",	IKT_IPT,		IPT_JOYSTICKLEFT_LEFT, 8 },
	{ "P8_JOYSTICKLEFT_RIGHT",	IKT_IPT,		IPT_JOYSTICKLEFT_RIGHT, 8 },

	{ "P1_PEDAL",				IKT_IPT,		IPT_PEDAL, 1 },
	{ "P1_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 1 },
	{ "P2_PEDAL",				IKT_IPT,		IPT_PEDAL, 2 },
	{ "P2_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 2 },
	{ "P3_PEDAL",				IKT_IPT,		IPT_PEDAL, 3 },
	{ "P3_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 3 },
	{ "P4_PEDAL",				IKT_IPT,		IPT_PEDAL, 4 },
	{ "P4_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 4 },
	{ "P5_PEDAL",				IKT_IPT,		IPT_PEDAL, 5 },
	{ "P5_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 5 },
	{ "P6_PEDAL",				IKT_IPT,		IPT_PEDAL, 6 },
	{ "P6_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 6 },
	{ "P7_PEDAL",				IKT_IPT,		IPT_PEDAL, 7 },
	{ "P7_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 7 },
	{ "P8_PEDAL",				IKT_IPT,		IPT_PEDAL, 8 },
	{ "P8_PEDAL_EXT",			IKT_IPT_EXT,	IPT_PEDAL, 8 },

	{ "P1_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 1 },
	{ "P1_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 1 },
	{ "P2_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 2 },
	{ "P2_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 2 },
	{ "P3_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 3 },
	{ "P3_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 3 },
	{ "P4_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 4 },
	{ "P4_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 4 },
	{ "P5_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 5 },
	{ "P5_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 5 },
	{ "P6_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 6 },
	{ "P6_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 6 },
	{ "P7_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 7 },
	{ "P7_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 7 },
	{ "P8_PEDAL2",				IKT_IPT,		IPT_PEDAL2, 8 },
	{ "P8_PEDAL2_EXT",			IKT_IPT_EXT,	IPT_PEDAL2, 8 },

	{ "P1_PADDLE",				IKT_IPT,		IPT_PADDLE, 1 },
	{ "P1_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 1 },
	{ "P2_PADDLE",				IKT_IPT,		IPT_PADDLE, 2 },
	{ "P2_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 2 },
	{ "P3_PADDLE",				IKT_IPT,		IPT_PADDLE, 3 },
	{ "P3_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 3 },
	{ "P4_PADDLE",				IKT_IPT,		IPT_PADDLE, 4 },
	{ "P4_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 4 },
	{ "P5_PADDLE",				IKT_IPT,		IPT_PADDLE, 5 },
	{ "P5_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 5 },
	{ "P6_PADDLE",				IKT_IPT,		IPT_PADDLE, 6 },
	{ "P6_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 6 },
	{ "P7_PADDLE",				IKT_IPT,		IPT_PADDLE, 7 },
	{ "P7_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 7 },
	{ "P8_PADDLE",				IKT_IPT,		IPT_PADDLE, 8 },
	{ "P8_PADDLE_EXT",			IKT_IPT_EXT,	IPT_PADDLE, 8 },

	{ "P1_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 1 },
	{ "P1_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 1 },
	{ "P2_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 2 },
	{ "P2_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 2 },
	{ "P3_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 3 },
	{ "P3_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 3 },
	{ "P4_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 4 },
	{ "P4_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 4 },
	{ "P5_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 5 },
	{ "P5_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 5 },
	{ "P6_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 6 },
	{ "P6_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 6 },
	{ "P7_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 7 },
	{ "P7_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 7 },
	{ "P8_PADDLE_V",			IKT_IPT,		IPT_PADDLE_V, 8 },
	{ "P8_PADDLE_V_EXT",		IKT_IPT_EXT,	IPT_PADDLE_V, 8 },

	{ "P1_DIAL",				IKT_IPT,		IPT_DIAL, 1 },
	{ "P1_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 1 },
	{ "P2_DIAL",				IKT_IPT,		IPT_DIAL, 2 },
	{ "P2_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 2 },
	{ "P3_DIAL",				IKT_IPT,		IPT_DIAL, 3 },
	{ "P3_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 3 },
	{ "P4_DIAL",				IKT_IPT,		IPT_DIAL, 4 },
	{ "P4_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 4 },
	{ "P5_DIAL",				IKT_IPT,		IPT_DIAL, 5 },
	{ "P5_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 5 },
	{ "P6_DIAL",				IKT_IPT,		IPT_DIAL, 6 },
	{ "P6_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 6 },
	{ "P7_DIAL",				IKT_IPT,		IPT_DIAL, 7 },
	{ "P7_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 7 },
	{ "P8_DIAL",				IKT_IPT,		IPT_DIAL, 8 },
	{ "P8_DIAL_EXT",			IKT_IPT_EXT,	IPT_DIAL, 8 },

	{ "P1_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 1 },
	{ "P1_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 1 },
	{ "P2_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 2 },
	{ "P2_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 2 },
	{ "P3_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 3 },
	{ "P3_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 3 },
	{ "P4_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 4 },
	{ "P4_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 4 },
	{ "P5_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 5 },
	{ "P5_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 5 },
	{ "P6_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 6 },
	{ "P6_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 6 },
	{ "P7_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 7 },
	{ "P7_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 7 },
	{ "P8_DIAL_V",				IKT_IPT,		IPT_DIAL_V, 8 },
	{ "P8_DIAL_V_EXT",			IKT_IPT_EXT,	IPT_DIAL_V, 8 },

	{ "P1_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 1 },
	{ "P1_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 1 },
	{ "P2_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 2 },
	{ "P2_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 2 },
	{ "P3_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 3 },
	{ "P3_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 3 },
	{ "P4_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 4 },
	{ "P4_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 4 },
	{ "P5_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 5 },
	{ "P5_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 5 },
	{ "P6_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 6 },
	{ "P6_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 6 },
	{ "P7_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 7 },
	{ "P7_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 7 },
	{ "P8_TRACKBALL_X",			IKT_IPT,		IPT_TRACKBALL_X, 8 },
	{ "P8_TRACKBALL_X_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_X, 8 },

	{ "P1_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 1 },
	{ "P1_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 1 },
	{ "P2_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 2 },
	{ "P2_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 2 },
	{ "P3_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 3 },
	{ "P3_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 3 },
	{ "P4_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 4 },
	{ "P4_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 4 },
	{ "P5_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 5 },
	{ "P5_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 5 },
	{ "P6_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 6 },
	{ "P6_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 6 },
	{ "P7_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 7 },
	{ "P7_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 7 },
	{ "P8_TRACKBALL_Y",			IKT_IPT,		IPT_TRACKBALL_Y, 8 },
	{ "P8_TRACKBALL_Y_EXT",		IKT_IPT_EXT,	IPT_TRACKBALL_Y, 8 },

	{ "P1_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 1 },
	{ "P1_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 1 },
	{ "P2_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 2 },
	{ "P2_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 2 },
	{ "P3_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 3 },
	{ "P3_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 3 },
	{ "P4_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 4 },
	{ "P4_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 4 },
	{ "P5_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 5 },
	{ "P5_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 5 },
	{ "P6_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 6 },
	{ "P6_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 6 },
	{ "P7_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 7 },
	{ "P7_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 7 },
	{ "P8_AD_STICK_X",			IKT_IPT,		IPT_AD_STICK_X, 8 },
	{ "P8_AD_STICK_X_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_X, 8 },

	{ "P1_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 1 },
	{ "P1_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 1 },
	{ "P2_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 2 },
	{ "P2_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 2 },
	{ "P3_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 3 },
	{ "P3_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 3 },
	{ "P4_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 4 },
	{ "P4_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 4 },
	{ "P5_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 5 },
	{ "P5_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 5 },
	{ "P6_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 6 },
	{ "P6_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 6 },
	{ "P7_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 7 },
	{ "P7_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 7 },
	{ "P8_AD_STICK_Y",			IKT_IPT,		IPT_AD_STICK_Y, 8 },
	{ "P8_AD_STICK_Y_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Y, 8 },

	{ "P1_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 1 },
	{ "P1_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 1 },
	{ "P2_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 2 },
	{ "P2_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 2 },
	{ "P3_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 3 },
	{ "P3_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 3 },
	{ "P4_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 4 },
	{ "P4_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 4 },
	{ "P5_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 5 },
	{ "P5_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 5 },
	{ "P6_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 6 },
	{ "P6_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 6 },
	{ "P7_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 7 },
	{ "P7_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 7 },
	{ "P8_LIGHTGUN_X",			IKT_IPT,		IPT_LIGHTGUN_X, 8 },
	{ "P8_LIGHTGUN_X_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_X, 8 },

	{ "P1_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 1 },
	{ "P1_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 1 },
	{ "P2_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 2 },
	{ "P2_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 2 },
	{ "P3_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 3 },
	{ "P3_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 3 },
	{ "P4_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 4 },
	{ "P4_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 4 },
	{ "P5_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 5 },
	{ "P5_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 5 },
	{ "P6_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 6 },
	{ "P6_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 6 },
	{ "P7_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 7 },
	{ "P7_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 7 },
	{ "P8_LIGHTGUN_Y",			IKT_IPT,		IPT_LIGHTGUN_Y, 8 },
	{ "P8_LIGHTGUN_Y_EXT",		IKT_IPT_EXT,	IPT_LIGHTGUN_Y, 8 },

	{ "P1_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 1 },
	{ "P1_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 1 },
	{ "P2_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 2 },
	{ "P2_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 2 },
	{ "P3_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 3 },
	{ "P3_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 3 },
	{ "P4_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 4 },
	{ "P4_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 4 },
	{ "P5_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 5 },
	{ "P5_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 5 },
	{ "P6_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 6 },
	{ "P6_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 6 },
	{ "P7_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 7 },
	{ "P7_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 7 },
	{ "P8_AD_STICK_Z",			IKT_IPT,		IPT_AD_STICK_Z, 8 },
	{ "P8_AD_STICK_Z_EXT",		IKT_IPT_EXT,	IPT_AD_STICK_Z, 8 },

#ifdef MESS
	{ "P1_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 1 },
	{ "P1_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 1 },
	{ "P2_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 2 },
	{ "P2_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 2 },
	{ "P3_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 3 },
	{ "P3_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 3 },
	{ "P4_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 4 },
	{ "P4_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 4 },
	{ "P5_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 5 },
	{ "P5_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 5 },
	{ "P6_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 6 },
	{ "P6_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 6 },
	{ "P7_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 7 },
	{ "P7_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 7 },
	{ "P8_MOUSE_X",				IKT_IPT,		IPT_MOUSE_X, 8 },
	{ "P8_MOUSE_X_EXT",			IKT_IPT_EXT,	IPT_MOUSE_X, 8 },

	{ "P1_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 1 },
	{ "P1_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 1 },
	{ "P2_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 2 },
	{ "P2_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 2 },
	{ "P3_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 3 },
	{ "P3_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 3 },
	{ "P4_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 4 },
	{ "P4_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 4 },
	{ "P5_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 5 },
	{ "P5_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 5 },
	{ "P6_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 6 },
	{ "P6_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 6 },
	{ "P7_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 7 },
	{ "P7_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 7 },
	{ "P8_MOUSE_Y",				IKT_IPT,		IPT_MOUSE_Y, 8 },
	{ "P8_MOUSE_Y_EXT",			IKT_IPT_EXT,	IPT_MOUSE_Y, 8 },

	{ "P1_START",				IKT_IPT,		IPT_START, 1 },
	{ "P2_START",				IKT_IPT,		IPT_START, 2 },
	{ "P3_START",				IKT_IPT,		IPT_START, 3 },
	{ "P4_START",				IKT_IPT,		IPT_START, 4 },
	{ "P5_START",				IKT_IPT,		IPT_START, 5 },
	{ "P6_START",				IKT_IPT,		IPT_START, 6 },
	{ "P7_START",				IKT_IPT,		IPT_START, 7 },
	{ "P8_START",				IKT_IPT,		IPT_START, 8 },
	{ "P1_SELECT",				IKT_IPT,		IPT_SELECT, 1 },
	{ "P2_SELECT",				IKT_IPT,		IPT_SELECT, 2 },
	{ "P3_SELECT",				IKT_IPT,		IPT_SELECT, 3 },
	{ "P4_SELECT",				IKT_IPT,		IPT_SELECT, 4 },
	{ "P5_SELECT",				IKT_IPT,		IPT_SELECT, 5 },
	{ "P6_SELECT",				IKT_IPT,		IPT_SELECT, 6 },
	{ "P7_SELECT",				IKT_IPT,		IPT_SELECT, 7 },
	{ "P8_SELECT",				IKT_IPT,		IPT_SELECT, 8 },
#endif /* MESS */

	{ "OSD_1",					IKT_IPT,		IPT_OSD_1 },
	{ "OSD_2",					IKT_IPT,		IPT_OSD_2 },
	{ "OSD_3",					IKT_IPT,		IPT_OSD_3 },
	{ "OSD_4",					IKT_IPT,		IPT_OSD_4 },

	{ "UNKNOWN",				IKT_IPT,		IPT_UNKNOWN },
	{ "END",					IKT_IPT,		IPT_END },

	{ "",						0,	0 }
};

int num_ik = sizeof(input_keywords)/sizeof(struct ik);

/***************************************************************************/
/* Generic IO */

static int readword(mame_file *f,UINT16 *num)
{
	unsigned i;
	int res;

	res = 0;
	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		res <<= 8;
		if (mame_fread(f,&c,1) != 1)
			return -1;
		res |= c;
	}

	*num = res;
	return 0;
}

static void writeword(mame_file *f,UINT16 num)
{
	unsigned i;

	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT16)-1)) & 0xff;
		mame_fwrite(f,&c,1);
		num <<= 8;
	}
}



/***************************************************************************/
/* Load */

static void load_default_keys(void)
{
	config_file *cfg;

	osd_customize_inputport_defaults(inputport_defaults);
	memcpy(inputport_defaults_backup,inputport_defaults,sizeof(inputport_defaults));

	cfg = config_open(NULL);
	if (cfg)
	{
		config_read_default_ports(cfg, inputport_defaults);
		config_close(cfg);
	}
}

static void save_default_keys(void)
{
	config_file *cfg;

	cfg = config_create(NULL);
	if (cfg)
	{
		config_write_default_ports(cfg, inputport_defaults_backup, inputport_defaults);
		config_close(cfg);
			}

	memcpy(inputport_defaults,inputport_defaults_backup,sizeof(inputport_defaults_backup));
}


int load_input_port_settings(void)
{
	config_file *cfg;
	int err;
	struct mixer_config mixercfg;


	load_default_keys();

	cfg = config_open(Machine->gamedrv->name);
	if (cfg)
		{
		err = config_read_ports(cfg, Machine->input_ports_default, Machine->input_ports);
		if (err)
				goto getout;

		err = config_read_coin_and_ticket_counters(cfg, coins, lastcoin, coinlockedout, &dispensed_tickets);
		if (err)
				goto getout;

		err = config_read_mixer_config(cfg, &mixercfg);
		if (err)
			goto getout;

		mixer_load_config(&mixercfg);

getout:
		config_close(cfg);
	}

	/* All analog ports need initialization */
	{
		int i;
		for (i = 0; i < MAX_INPUT_PORTS; i++)
			input_analog_init[i] = 1;
	}

	init_analog_seq();

	update_input_ports();

	/* if we didn't find a saved config, return 0 so the main core knows that it */
	/* is the first time the game is run and it should diplay the disclaimer. */
	return cfg ? 1 : 0;
}

/***************************************************************************/
/* Save */

void save_input_port_settings(void)
{
	config_file *cfg;
	struct mixer_config mixercfg;

	save_default_keys();

	cfg = config_create(Machine->gamedrv->name);
	if (cfg)
		{
		mixer_save_config(&mixercfg);

		config_write_ports(cfg, Machine->input_ports_default, Machine->input_ports);
		config_write_coin_and_ticket_counters(cfg, coins, lastcoin, coinlockedout, dispensed_tickets);
		config_write_mixer_config(cfg, &mixercfg);
		config_close(cfg);
	}
}



/* Note that the following 3 routines have slightly different meanings with analog ports */
const char *input_port_name(const struct InputPort *in)
{
	int i;
	UINT32 type;

	if (in->name != IP_NAME_DEFAULT) return in->name;

 	type = in->type | (in->player << 16);

	for (i = 0; inputport_defaults[i].type != IPT_END; i++)
		if (inputport_defaults[i].type == in->type && (inputport_defaults[i].player == 0 || inputport_defaults[i].player == in->player + 1))
			break;

	return inputport_defaults[i].name;
}

int input_port_active(const struct InputPort *in)
{
	return input_port_name(in) && !in->unused;
}

InputSeq* input_port_type_seq(int type, int indx)
{
	unsigned i;

	for (i = 0; inputport_defaults[i].type != IPT_END; i++)
		if (inputport_defaults[i].type == type)
			break;

	return &inputport_defaults[i].seq[indx];
}

int input_port_seq_count(const struct InputPort *in)
{
	return (in->type > IPT_ANALOG_START && in->type < IPT_ANALOG_END) ? 2 : 1;
}

InputSeq* input_port_seq(struct InputPort *in, int seq)
{
	static InputSeq ip_none = SEQ_DEF_1(CODE_NONE);
	int i;

	while (seq > 0 && seq_get_1(&in->seq[seq]) == CODE_PREVIOUS)
		in--;

	/* if port is disabled, return no key */
	if (in->unused)
		return &ip_none;

	/* does this override the default? */
	if (seq_get_1(&in->seq[seq]) != CODE_DEFAULT)
		return &in->seq[seq];

	for (i = 0; inputport_defaults[i].type != IPT_END; i++)
		if (inputport_defaults[i].type == in->type && (inputport_defaults[i].player == 0 || inputport_defaults[i].player == in->player + 1))
			break;

	return &inputport_defaults[i].seq[seq];
}

void update_analog_port(int port)
{
	struct InputPort *in;
	int current, delta, sensitivity, min, max, default_value;
	int axis, is_stick, is_gun, check_bounds;
	InputSeq* incseq;
	InputSeq* decseq;
	int keydelta;
	int player;

	/* get input definition */
	in = input_analog[port];

	decseq = input_port_seq(in, 0);
	incseq = input_port_seq(in, 1);

	keydelta = in->u.analog.delta;

	switch (in->type)
	{
		case IPT_PADDLE:
			axis = X_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_PADDLE_V:
			axis = Y_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_DIAL:
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_DIAL_V:
			axis = Y_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_TRACKBALL_X:
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_TRACKBALL_Y:
			axis = Y_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_AD_STICK_X:
			axis = X_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_AD_STICK_Y:
			axis = Y_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_AD_STICK_Z:
			axis = Z_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_LIGHTGUN_X:
			axis = X_AXIS; is_stick = 1; is_gun=1; check_bounds = 1; break;
		case IPT_LIGHTGUN_Y:
			axis = Y_AXIS; is_stick = 1; is_gun=1; check_bounds = 1; break;
		case IPT_PEDAL:
			axis = PEDAL_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_PEDAL2:
			axis = Z_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
#ifdef MESS
		case IPT_MOUSE_X:
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_MOUSE_Y:
			axis = Y_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
#endif
		default:
			/* Use some defaults to prevent crash */
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0;
			logerror("Oops, polling non analog device in update_analog_port()????\n");
	}


	sensitivity = in->u.analog.sensitivity;
	min = in->u.analog.min;
	max = in->u.analog.max;
	default_value = in->default_value * 100 / sensitivity;
	/* extremes can be either signed or unsigned */
	if (min > max)
	{
		if (in->mask > 0xff) min = min - 0x10000;
		else min = min - 0x100;
	}

	input_analog_previous_value[port] = input_analog_current_value[port];

	/* if centered go back to the default position */
	/* sticks are handled later... */
	if ((in->u.analog.center) && !is_stick)
		input_analog_current_value[port] = in->default_value * 100 / sensitivity;

	current = input_analog_current_value[port];

	delta = 0;

	player = in->player;

	delta = mouse_delta_axis[player][axis];

	if (seq_pressed(decseq)) delta -= keydelta;

	if (in->type != IPT_PEDAL && in->type != IPT_PEDAL2)
	{
		if (seq_pressed(incseq)) delta += keydelta;
	}
	else
	{
		/* is this cheesy or what? */
		if (!delta && seq_get_1(incseq) == KEYCODE_Y) delta += keydelta;
		delta = -delta;
	}

	if (in->u.analog.reverse)
		delta = -delta;

	if (is_gun)
	{
		/* The OSD lightgun call should return the delta from the middle of the screen
		when the gun is fired (not the absolute pixel value), and 0 when the gun is
		inactive.  We take advantage of this to provide support for other controllers
		in place of a physical lightgun.  When the OSD lightgun returns 0, then control
		passes through to the analog joystick, and mouse, in that order.  When the OSD
		lightgun returns a value it overrides both mouse & analog joystick.

		The value returned by the OSD layer should be -128 to 128, same as analog
		joysticks.

		There is an ugly hack to stop scaling of lightgun returned values.  It really
		needs rewritten...
		*/
		if (axis == X_AXIS) {
			if (lightgun_delta_axis[player][X_AXIS] || lightgun_delta_axis[player][Y_AXIS]) {
				analog_previous_axis[player][X_AXIS]=0;
				analog_current_axis[player][X_AXIS]=lightgun_delta_axis[player][X_AXIS];
				input_analog_scale[port]=0;
				sensitivity=100;
			}
		}
		else
		{
			if (lightgun_delta_axis[player][X_AXIS] || lightgun_delta_axis[player][Y_AXIS]) {
				analog_previous_axis[player][Y_AXIS]=0;
				analog_current_axis[player][Y_AXIS]=lightgun_delta_axis[player][Y_AXIS];
				input_analog_scale[port]=0;
				sensitivity=100;
			}
		}
	}

	if (is_stick)
	{
		int new, prev;

		/* center stick */
		if ((delta == 0) && (in->u.analog.center))
		{
			if (current > default_value)
			delta = -100 / sensitivity;
			if (current < default_value)
			delta = 100 / sensitivity;
		}

		/* An analog joystick which is not at zero position (or has just */
		/* moved there) takes precedence over all other computations */
		/* analog_x/y holds values from -128 to 128 (yes, 128, not 127) */

		new  = analog_current_axis[player][axis];
		prev = analog_previous_axis[player][axis];

		if ((new != 0) || (new-prev != 0))
		{
			delta=0;

			/* for pedals, need to change to possitive number */
			/* and, if needed, reverse pedal input */
			if (in->type == IPT_PEDAL || in->type == IPT_PEDAL2)
			{
				new  = -new;
				prev = -prev;
				if (in->u.analog.reverse)		// a reversed pedal is diff than normal reverse
				{								// 128 = no gas, 0 = all gas
					new  = 128-new;				// the default "new=-new" doesn't handle this
					prev = 128-prev;
				}
			}
			else if (in->u.analog.reverse)
			{
				new  = -new;
				prev = -prev;
			}

			/* apply sensitivity using a logarithmic scale */
			if (in->mask > 0xff)
			{
				if (new > 0)
				{
					current = (pow(new / 32768.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 32768.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
			else
			{
				if (new > 0)
				{
					current = (pow(new / 128.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 128.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
		}
	}

	current += delta;

	if (check_bounds)
	{
		int temp;

		if (current >= 0)
			temp = (current * sensitivity + 50) / 100;
		else
			temp = (-current * sensitivity + 50) / -100;

		if (temp < min)
		{
			if (min >= 0)
				current = (min * 100 + sensitivity/2) / sensitivity;
			else
				current = (-min * 100 + sensitivity/2) / -sensitivity;
		}
		if (temp > max)
		{
			if (max >= 0)
				current = (max * 100 + sensitivity/2) / sensitivity;
			else
				current = (-max * 100 + sensitivity/2) / -sensitivity;
		}
	}

	input_analog_current_value[port] = current;
}

static void scale_analog_port(int port)
{
	struct InputPort *in;
	int delta,current,sensitivity;

profiler_mark(PROFILER_INPUT);
	in = input_analog[port];
	sensitivity = in->u.analog.sensitivity;

	/* apply scaling fairly in both positive and negative directions */
	delta = input_analog_current_value[port] - input_analog_previous_value[port];
	if (delta >= 0)
		delta = cpu_scalebyfcount(delta);
	else
		delta = -cpu_scalebyfcount(-delta);

	current = input_analog_previous_value[port] + delta;

	/* An ugly hack to remove scaling on lightgun ports */
	if (input_analog_scale[port]) {
		/* apply scaling fairly in both positive and negative directions */
		if (current >= 0)
			current = (current * sensitivity + 50) / 100;
		else
			current = (-current * sensitivity + 50) / -100;
	}

	input_port_value[port] &= ~in->mask;
	input_port_value[port] |= current & in->mask;

	if (playback)
		readword(playback,&input_port_value[port]);
	if (record)
		writeword(record,input_port_value[port]);
profiler_mark(PROFILER_END);
}

#define MAX_JOYSTICKS 3
#define MAX_PLAYERS 8
static int mJoyCurrent[MAX_JOYSTICKS*MAX_PLAYERS];
static int mJoyPrevious[MAX_JOYSTICKS*MAX_PLAYERS];
static int mJoy4Way[MAX_JOYSTICKS*MAX_PLAYERS];
/*
The above "Joy" states contain packed bits:
	0001	up
	0010	down
	0100	left
	1000	right
*/

static void
ScanJoysticks( struct InputPort *in )
{
	int i;
	int port = 0;

	/* Save old Joystick state. */
	memcpy( mJoyPrevious, mJoyCurrent, sizeof(mJoyPrevious) );

	/* Initialize bits of mJoyCurrent to zero. */
	memset( mJoyCurrent, 0, sizeof(mJoyCurrent) );

	/* Now iterate over the input port structure to populate mJoyCurrent. */
	while( in->type != IPT_END && port < MAX_INPUT_PORTS )
	{
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type >= IPT_JOYSTICK_UP) &&
				(in->type <= IPT_JOYSTICKLEFT_RIGHT))
			{
				for (i = 0; i < input_port_seq_count(in); i++)
				{
					InputSeq* seq;
					seq = input_port_seq(in, i);
					if( seq_pressed(seq) )
					{
						int joynum,joydir,player;
						player = in->player;

						joynum = player * MAX_JOYSTICKS +
								(in->type - IPT_JOYSTICK_UP) / 4;
						joydir = (in->type - IPT_JOYSTICK_UP) % 4;

						mJoyCurrent[joynum] |= 1<<joydir;
					}
				}
			}
			in++;
		}
		port++;
		if (in->type == IPT_PORT) in++;
	}

	/* Process the joystick states, to filter out illegal combinations of switches. */
	for( i=0; i<MAX_JOYSTICKS*MAX_PLAYERS; i++ )
	{
		if( (mJoyCurrent[i]&0x3)==0x3 ) /* both up and down are pressed */
		{
			mJoyCurrent[i]&=0xc; /* clear up and down */
		}
		if( (mJoyCurrent[i]&0xc)==0xc ) /* both left and right are pressed */
		{
			mJoyCurrent[i]&=0x3; /* clear left and right */
		}

		/* Only update mJoy4Way if the joystick has moved. */
		if( mJoyCurrent[i]!=mJoyPrevious[i] )
		{
			mJoy4Way[i] = mJoyCurrent[i];

			if( (mJoy4Way[i] & 0x3) && (mJoy4Way[i] & 0xc) )
			{
				/* If joystick is pointing at a diagonal, acknowledge that the player moved
				 * the joystick by favoring a direction change.  This minimizes frustration
				 * when using a keyboard for input, and maximizes responsiveness.
				 *
				 * For example, if you are holding "left" then switch to "up" (where both left
				 * and up are briefly pressed at the same time), we'll transition immediately
				 * to "up."
				 *
				 * Under the old "sticky" key implentation, "up" wouldn't be triggered until
				 * left was released.
				 *
				 * Zero any switches that didn't change from the previous to current state.
				 */
				mJoy4Way[i] ^= (mJoy4Way[i] & mJoyPrevious[i]);
			}

			if( (mJoy4Way[i] & 0x3) && (mJoy4Way[i] & 0xc) )
			{
				/* If we are still pointing at a diagonal, we are in an indeterminant state.
				 *
				 * This could happen if the player moved the joystick from the idle position directly
				 * to a diagonal, or from one diagonal directly to an extreme diagonal.
				 *
				 * The chances of this happening with a keyboard are slim, but we still need to
				 * constrain this case.
				 *
				 * For now, just resolve randomly.
				 */
				if( rand()&1 )
				{
					mJoy4Way[i] &= 0x3; /* eliminate horizontal component */
				}
				else
				{
					mJoy4Way[i] &= 0xc; /* eliminate vertical component */
				}
			}
		}
	}
} /* ScanJoysticks */

void update_input_ports(void)
{
	int port,ib;
	struct InputPort *in;

#define MAX_INPUT_BITS 1024
	static int impulsecount[MAX_INPUT_BITS];
	static int waspressed[MAX_INPUT_BITS];

profiler_mark(PROFILER_INPUT);

	/* clear all the values before proceeding */
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		input_port_value[port] = 0;
		input_vblank[port] = 0;
		input_analog[port] = 0;
	}

	in = Machine->input_ports;
	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		logerror("Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else
	{
		in++;
	}

	ScanJoysticks( in ); /* populates mJoyCurrent[] */

	/* scan all the input ports */
	port = 0;
	ib = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		struct InputPort *start;
		/* first of all, scan the whole input port definition and build the */
		/* default value. I must do it before checking for input because otherwise */
		/* multiple keys associated with the same input bit wouldn't work (the bit */
		/* would be reset to its default value by the second entry, regardless if */
		/* the key associated with the first entry was pressed) */
		start = in;
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if (in->type != IPT_DIPSWITCH_SETTING	/* skip dipswitch definitions */
#ifdef MESS
				&& in->type != IPT_CONFIG_SETTING		/* skip config definitions */
#endif /* MESS */
				)
			{
				input_port_value[port] =
						(input_port_value[port] & ~in->mask) | (in->default_value & in->mask);
			}

			in++;
		}

		/* now get back to the beginning of the input port and check the input bits. */
		for (in = start;
			 in->type != IPT_END && in->type != IPT_PORT;
			 in++, ib++)
		{
			if (in->type != IPT_DIPSWITCH_SETTING	/* skip dipswitch definitions */
#ifdef MESS
				&& in->type != IPT_CONFIG_SETTING		/* skip config definitions */
#endif /* MESS */
				)
			{
				if (in->type == IPT_VBLANK)
				{
					input_vblank[port] ^= in->mask;
					input_port_value[port] ^= in->mask;
if (Machine->drv->vblank_duration == 0)
	logerror("Warning: you are using IPT_VBLANK with vblank_duration = 0. You need to increase vblank_duration for IPT_VBLANK to work.\n");
				}
				/* If it's an analog control, handle it appropriately */
				else if ((in->type > IPT_ANALOG_START)
					  && (in->type < IPT_ANALOG_END  )) /* LBO 120897 */
				{
					input_analog[port]=in;
					/* reset the analog port on first access */
					if (input_analog_init[port])
					{
						input_analog_init[port] = 0;
						input_analog_scale[port] = 1;
						input_analog_current_value[port] = input_analog_previous_value[port]
							= in->default_value * 100 / in->u.analog.sensitivity;
					}
				}
				else
				{
					InputSeq* seq;
					seq = input_port_seq(in, 0);
					if (seq_pressed(seq))
					{
#ifdef MESS
						if ((in->type == IPT_KEYBOARD) && osd_keyboard_disabled())
							continue;
#endif

						/* skip if coin input and it's locked out */
						if ((in->type >= IPT_COIN1) &&
							(in->type <= IPT_COIN4) &&
                            coinlockedout[in->type - IPT_COIN1])
						{
							continue;
						}
						if ((in->type >= IPT_COIN5) &&
							(in->type <= IPT_COIN8) &&
                            coinlockedout[in->type - IPT_COIN5 + 4])
						{
							continue;
						}

						if (in->impulse)
						{
							if (waspressed[ib] == 0)
								impulsecount[ib] = in->impulse;
								/* the input bit will be toggled later */
						}
						else if (in->toggle)
						{
							if (waspressed[ib] == 0)
							{
								in->default_value ^= in->mask;
								input_port_value[port] ^= in->mask;
							}
						}
						else if (in->type >= IPT_JOYSTICK_UP &&
								in->type <= IPT_JOYSTICKLEFT_RIGHT)
						{
							int joynum,joydir,mask,player;

							player = in->player;
							joynum = player * MAX_JOYSTICKS +
									(in->type - IPT_JOYSTICK_UP) / 4;

							joydir = (in->type - IPT_JOYSTICK_UP) % 4;

							mask = in->mask;

							if( in->four_way )
							{
								/* apply 4-way joystick constraint */
								if( ((mJoy4Way[joynum]>>joydir)&1) == 0 )
								{
									mask = 0;
								}
							}
							else
							{
								/* filter up+down and left+right */
								if( ((mJoyCurrent[joynum]>>joydir)&1) == 0 )
								{
									mask = 0;
								}
							}

							input_port_value[port] ^= mask;
						} /* joystick */
						else
						{
							input_port_value[port] ^= in->mask;
						}
						waspressed[ib] = 1;
					}
					else
						waspressed[ib] = 0;

					if (in->impulse && impulsecount[ib] > 0)
					{
						impulsecount[ib]--;
						waspressed[ib] = 1;
						input_port_value[port] ^= in->mask;
					}
				}
			}
		}

		port++;
		if (in->type == IPT_PORT) in++;
	}

	if (playback)
	{
		int i;

		ib=0;
		in = Machine->input_ports;
		in++;
		for (i = 0; i < MAX_INPUT_PORTS; i ++)
		{
			readword(playback,&input_port_value[i]);
			if (in->type == IPT_PORT) in++;
		}
	}

#ifdef MESS
	inputx_update(input_port_value);
#endif

	if (record)
	{
		int i;

		for (i = 0; i < MAX_INPUT_PORTS; i ++)
			writeword(record,input_port_value[i]);
	}

profiler_mark(PROFILER_END);
}



/* used the the CPU interface to notify that VBlank has ended, so we can update */
/* IPT_VBLANK input ports. */
void inputport_vblank_end(void)
{
	int port;
	int i;


profiler_mark(PROFILER_INPUT);
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		if (input_vblank[port])
		{
			input_port_value[port] ^= input_vblank[port];
			input_vblank[port] = 0;
		}
	}

	/* update the analog devices */
	for (i = 0;i < OSD_MAX_JOY_ANALOG;i++)
	{
		/* update the analog joystick position */
		int a;
		for (a=0; a<MAX_ANALOG_AXES ; a++)
		{
			analog_previous_axis[i][a] = analog_current_axis[i][a];
		}
		osd_analogjoy_read (i, analog_current_axis[i], analogjoy_input[i]);

		/* update mouse/trackball position */
		osd_trak_read (i, &(mouse_delta_axis[i])[X_AXIS], &(mouse_delta_axis[i])[Y_AXIS]);

		/* update lightgun position, if any */
 		osd_lightgun_read (i, &(lightgun_delta_axis[i])[X_AXIS], &(lightgun_delta_axis[i])[Y_AXIS]);
	}

	for (i = 0;i < MAX_INPUT_PORTS;i++)
	{
		struct InputPort *in;

		in=input_analog[i];
		if (in)
		{
			update_analog_port(i);
		}
	}
profiler_mark(PROFILER_END);
}



int readinputport(int port)
{
	struct InputPort *in;

	/* Update analog ports on demand */
	in=input_analog[port];
	if (in)
	{
		scale_analog_port(port);
	}

	return input_port_value[port];
}



int readinputportbytag(const char *tag)
{
	struct InputPort *in;
	int port = -1;
	
	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		if (in->type == IPT_PORT)
		{
			port++;
			if (in->u.start.tag && !strcmp(in->u.start.tag, tag))
				return readinputport(port);
		}
		in++;
	}
	osd_die("Unable to locate input port '%s'", tag);
	return -1;
}



READ8_HANDLER( input_port_0_r ) { return readinputport(0); }
READ8_HANDLER( input_port_1_r ) { return readinputport(1); }
READ8_HANDLER( input_port_2_r ) { return readinputport(2); }
READ8_HANDLER( input_port_3_r ) { return readinputport(3); }
READ8_HANDLER( input_port_4_r ) { return readinputport(4); }
READ8_HANDLER( input_port_5_r ) { return readinputport(5); }
READ8_HANDLER( input_port_6_r ) { return readinputport(6); }
READ8_HANDLER( input_port_7_r ) { return readinputport(7); }
READ8_HANDLER( input_port_8_r ) { return readinputport(8); }
READ8_HANDLER( input_port_9_r ) { return readinputport(9); }
READ8_HANDLER( input_port_10_r ) { return readinputport(10); }
READ8_HANDLER( input_port_11_r ) { return readinputport(11); }
READ8_HANDLER( input_port_12_r ) { return readinputport(12); }
READ8_HANDLER( input_port_13_r ) { return readinputport(13); }
READ8_HANDLER( input_port_14_r ) { return readinputport(14); }
READ8_HANDLER( input_port_15_r ) { return readinputport(15); }
READ8_HANDLER( input_port_16_r ) { return readinputport(16); }
READ8_HANDLER( input_port_17_r ) { return readinputport(17); }
READ8_HANDLER( input_port_18_r ) { return readinputport(18); }
READ8_HANDLER( input_port_19_r ) { return readinputport(19); }
READ8_HANDLER( input_port_20_r ) { return readinputport(20); }
READ8_HANDLER( input_port_21_r ) { return readinputport(21); }
READ8_HANDLER( input_port_22_r ) { return readinputport(22); }
READ8_HANDLER( input_port_23_r ) { return readinputport(23); }
READ8_HANDLER( input_port_24_r ) { return readinputport(24); }
READ8_HANDLER( input_port_25_r ) { return readinputport(25); }
READ8_HANDLER( input_port_26_r ) { return readinputport(26); }
READ8_HANDLER( input_port_27_r ) { return readinputport(27); }
READ8_HANDLER( input_port_28_r ) { return readinputport(28); }
READ8_HANDLER( input_port_29_r ) { return readinputport(29); }

READ16_HANDLER( input_port_0_word_r ) { return readinputport(0); }
READ16_HANDLER( input_port_1_word_r ) { return readinputport(1); }
READ16_HANDLER( input_port_2_word_r ) { return readinputport(2); }
READ16_HANDLER( input_port_3_word_r ) { return readinputport(3); }
READ16_HANDLER( input_port_4_word_r ) { return readinputport(4); }
READ16_HANDLER( input_port_5_word_r ) { return readinputport(5); }
READ16_HANDLER( input_port_6_word_r ) { return readinputport(6); }
READ16_HANDLER( input_port_7_word_r ) { return readinputport(7); }
READ16_HANDLER( input_port_8_word_r ) { return readinputport(8); }
READ16_HANDLER( input_port_9_word_r ) { return readinputport(9); }
READ16_HANDLER( input_port_10_word_r ) { return readinputport(10); }
READ16_HANDLER( input_port_11_word_r ) { return readinputport(11); }
READ16_HANDLER( input_port_12_word_r ) { return readinputport(12); }
READ16_HANDLER( input_port_13_word_r ) { return readinputport(13); }
READ16_HANDLER( input_port_14_word_r ) { return readinputport(14); }
READ16_HANDLER( input_port_15_word_r ) { return readinputport(15); }
READ16_HANDLER( input_port_16_word_r ) { return readinputport(16); }
READ16_HANDLER( input_port_17_word_r ) { return readinputport(17); }
READ16_HANDLER( input_port_18_word_r ) { return readinputport(18); }
READ16_HANDLER( input_port_19_word_r ) { return readinputport(19); }
READ16_HANDLER( input_port_20_word_r ) { return readinputport(20); }
READ16_HANDLER( input_port_21_word_r ) { return readinputport(21); }
READ16_HANDLER( input_port_22_word_r ) { return readinputport(22); }
READ16_HANDLER( input_port_23_word_r ) { return readinputport(23); }
READ16_HANDLER( input_port_24_word_r ) { return readinputport(24); }
READ16_HANDLER( input_port_25_word_r ) { return readinputport(25); }
READ16_HANDLER( input_port_26_word_r ) { return readinputport(26); }
READ16_HANDLER( input_port_27_word_r ) { return readinputport(27); }
READ16_HANDLER( input_port_28_word_r ) { return readinputport(28); }
READ16_HANDLER( input_port_29_word_r ) { return readinputport(29); }

/***************************************************************************/
/* InputPort conversion */

struct IptInitParams
{
	struct InputPort *ports;
	int max_ports;
	int current_port;
};



struct InputPort *input_port_initialize(void *param, UINT32 type)
{
	/* this function is used within an INPUT_PORT callback to set up a single
	 * port.  Note that this function takes a 32-bit type; this is because it
	 * knows how to interpret legacy type declarations (i.e. - with IPF_*
	 * flags */
	struct IptInitParams *iip;
	struct InputPort *port;
	int i;
	InputCode code;
	 
	iip = (struct IptInitParams *) param;
	if (iip->current_port >= iip->max_ports)
		osd_die("Too many input ports");
	port = &iip->ports[iip->current_port++];
	
	/* set up defaults */
	memset(port, 0, sizeof(*port));
	port->name = IP_NAME_DEFAULT;
	port->type = (UINT8) type;
	
	/* sets up default port codes */
	switch(port->type)
	{
		case IPT_DIPSWITCH_NAME:
		case IPT_DIPSWITCH_SETTING:
#ifdef MESS
		case IPT_CONFIG_NAME:
		case IPT_CONFIG_SETTING:
			code = CODE_NONE;
			break;
#endif

		default:
			code = CODE_DEFAULT;
			break;
	}
	for (i = 0; i < input_port_seq_count(port); i++)
		seq_set_1(&port->seq[i], code);

	return port;
}



struct InputPort *input_port_allocate(void construct_ipt(void *param))
{
	struct InputPort *port;
	unsigned max_ports = 512;
	struct IptInitParams iip;
	int i, j;

	port = (struct InputPort*) auto_malloc(max_ports * sizeof(struct InputPort));
	if (!port)
		return NULL;
	memset(port, 0, max_ports * sizeof(struct InputPort));
	
 	iip.ports = port;
 	iip.max_ports = max_ports;
 	iip.current_port = 0;
 	construct_ipt(&iip);

	i = 0;
	do
	{
		InputCode or3;
		
		for (j = 0; j < input_port_seq_count(&port[i]); j++)
		{
			if (port[i].seq[0][0] != CODE_DEFAULT)
			{
				/* this port overridden the defaults; expand joycodes into mousecodes */
				switch(port[i].seq[j][2])
				{
					case JOYCODE_1_BUTTON1:		or3 = JOYCODE_MOUSE_1_BUTTON1;	break;
					case JOYCODE_1_BUTTON2:		or3 = JOYCODE_MOUSE_1_BUTTON2;	break;
					case JOYCODE_1_BUTTON3:		or3 = JOYCODE_MOUSE_1_BUTTON3;	break;
					case JOYCODE_2_BUTTON1:		or3 = JOYCODE_MOUSE_2_BUTTON1;	break;
					case JOYCODE_2_BUTTON2:		or3 = JOYCODE_MOUSE_2_BUTTON2;	break;
					case JOYCODE_2_BUTTON3:		or3 = JOYCODE_MOUSE_2_BUTTON3;	break;
					case JOYCODE_3_BUTTON1:		or3 = JOYCODE_MOUSE_3_BUTTON1;	break;
					case JOYCODE_3_BUTTON2:		or3 = JOYCODE_MOUSE_3_BUTTON2;	break;
					case JOYCODE_3_BUTTON3:		or3 = JOYCODE_MOUSE_3_BUTTON3;	break;
					case JOYCODE_4_BUTTON1:		or3 = JOYCODE_MOUSE_4_BUTTON1;	break;
					case JOYCODE_4_BUTTON2:		or3 = JOYCODE_MOUSE_4_BUTTON2;	break;
					case JOYCODE_4_BUTTON3:		or3 = JOYCODE_MOUSE_4_BUTTON3;	break;
					default:					or3 = CODE_NONE;				break;
				}

				if (or3 < __code_max)
					seq_set_5(&port[i].seq[j], port[i].seq[j][0], CODE_OR, port[i].seq[j][2], CODE_OR, or3);
			}
			else
			{
				switch (port[i].type)
				{
					case IPT_END :
					case IPT_PORT :
					case IPT_DIPSWITCH_NAME :
					case IPT_DIPSWITCH_SETTING :
#ifdef MESS
					case IPT_CONFIG_NAME :
					case IPT_CONFIG_SETTING :
					case IPT_CATEGORY_NAME :
					case IPT_CATEGORY_SETTING :
#endif /* MESS */
						port[i].seq[j][0] = CODE_NONE;
						break;
				}
			}
		}

#ifdef MESS
		/* process MESS specific extensions to the port */
		inputx_handle_mess_extensions(&port[i]);
#endif /* MESS */
	}
	while(port[i++].type != IPT_END);
  
	return port;
}


void seq_set_string(InputSeq* a, const char *buf)
{
	char *lbuf;
	char *arg = NULL;
	int j;
	struct ik *pik;
	int found;

	// create a locale buffer to be parsed by strtok
	lbuf = malloc (strlen(buf)+1);

	// copy the input string
	strcpy (lbuf, buf);

	for(j=0;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;

	arg = strtok(lbuf, " \t\r\n");
	j = 0;
	while( arg != NULL )
	{
		found = 0;

		pik = input_keywords;

		while (!found && pik->name && pik->name[0] != 0)
		{
			if (strcmp(pik->name,arg) == 0)
			{
				// this entry is only valid if it is a KEYCODE
				if (pik->type == IKT_STD)
				{
					(*a)[j] = pik->val;
					j++;
					found = 1;
				}
			}
			pik++;
		}

		pik = osd_input_keywords;

		if (pik)
		{
			while (!found && pik->name && pik->name[0] != 0)
			{
				if (strcmp(pik->name,arg) == 0)
				{
					switch (pik->type)
					{
						case IKT_STD:
							(*a)[j] = pik->val;
							j++;
							found = 1;
						break;

						case IKT_OSD_KEY:
							(*a)[j] = keyoscode_to_code(pik->val);
							j++;
							found = 1;
						break;

						case IKT_OSD_JOY:
							(*a)[j] = joyoscode_to_code(pik->val);
							j++;
							found = 1;
						break;
					}
				}
				pik++;
			}
		}

		arg = strtok(NULL, " \t\r\n");
	}
	free (lbuf);
}

void init_analog_seq()
{
	struct InputPort *in;
	int player, axis;

/* init analogjoy_input array */
	for (player=0; player<OSD_MAX_JOY_ANALOG; player++)
	{
		for (axis=0; axis<MAX_ANALOG_AXES; axis++)
		{
			analogjoy_input[player][axis] = CODE_NONE;
		}
	}

	in = Machine->input_ports;
	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		logerror("Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else
	{
		in++;
	}

	while (in->type != IPT_END)
	{
		if (in->type != IPT_PORT && ((in->type > IPT_ANALOG_START))
			&& ((in->type < IPT_ANALOG_END)))
		{
			int i, j, invert;
			InputSeq *seq;
			InputCode analog_seq;

			for (i = 0; i < input_port_seq_count(in); i++)
			{
				seq = input_port_seq(in, i);

				invert = 0;
				analog_seq = CODE_NONE;

				for(j=0; j<SEQ_MAX && analog_seq == CODE_NONE; ++j)
				{
					switch ((*seq)[j])
					{
						case CODE_NONE :
							continue;
						case CODE_NOT :
							invert = !invert;
							break;
						case CODE_OR :
							invert = 0;
							break;
						default:
							if (!invert && is_joystick_axis_code((*seq)[j]) )
							{
								analog_seq = return_os_joycode((*seq)[j]);
							}
							invert = 0;
							break;
					}
				}
				if (analog_seq != CODE_NONE)
				{
					player = in->player;
  
					switch (in->type)
					{
						case IPT_DIAL:
						case IPT_PADDLE:
						case IPT_TRACKBALL_X:
						case IPT_LIGHTGUN_X:
						case IPT_AD_STICK_X:
#ifdef MESS
						case IPT_MOUSE_X:
#endif
							axis = X_AXIS;
							break;
						case IPT_DIAL_V:
						case IPT_PADDLE_V:
						case IPT_TRACKBALL_Y:
						case IPT_LIGHTGUN_Y:
						case IPT_AD_STICK_Y:
#ifdef MESS
						case IPT_MOUSE_Y:
#endif
							axis = Y_AXIS;
							break;
						case IPT_AD_STICK_Z:
						case IPT_PEDAL2:
							axis = Z_AXIS;
							break;
						case IPT_PEDAL:
							axis = PEDAL_AXIS;
							break;
						default:
							axis = 0;
							break;
					}

					analogjoy_input[player][axis] = analog_seq;
				}
			}
		}

		in++;
	}

	return;
}
