/***************************************************************************

  AY3600.c

  Machine file to handle emulation of the AY-3600 keyboard controller.

  TODO:
    Make the caps lock functional, remove dependency on input_port_1.
    Rename variables from a2 to AY3600
    Find the correct MAGIC_KEY_REPEAT_NUMBER
    Use the keyboard ROM for building a remapping table?

***************************************************************************/

#include "driver.h"

/* This is a horrid hack :( */
#ifndef MAX_KEYS
	#define MAX_KEYS	128 /* for MESS but already done in INPUT.C*/
#endif


static unsigned char a2_key_remap[MAX_KEYS+1][4];
static unsigned int a2_keys[256];

static int keydata_strobe;
static int anykey_clearstrobe;

#define A2_KEY_NORMAL  0
#define A2_KEY_CONTROL 1
#define A2_KEY_SHIFT   2
#define A2_KEY_BOTH    3
#define MAGIC_KEY_REPEAT_NUMBER 40

/***************************************************************************
  AY3600_init
***************************************************************************/
void AY3600_init(void)
{
	/* Set Caps Lock light to ON, since that's how we default it. */
	osd_led_w(1,1);

	/* Init the key remapping table */
	memset(a2_keys,0,sizeof(a2_keys));
	memset(a2_key_remap,0xFF,sizeof(a2_key_remap));
	memcpy(a2_key_remap[KEYCODE_BACKSPACE],		"\x7F\x7F\x7F\x7F",4);
	memcpy(a2_key_remap[KEYCODE_LEFT],			"\x08\x08\x08\x08",4);
	memcpy(a2_key_remap[KEYCODE_TAB],			"\x09\x09\x09\x09",4);
	memcpy(a2_key_remap[KEYCODE_DOWN],			"\x0A\x0A\x0A\x0A",4);
	memcpy(a2_key_remap[KEYCODE_UP],			"\x0B\x0B\x0B\x0B",4);
	memcpy(a2_key_remap[KEYCODE_ENTER],			"\x0D\x0D\x0D\x0D",4);
	memcpy(a2_key_remap[KEYCODE_RIGHT],			"\x15\x15\x15\x15",4);
	memcpy(a2_key_remap[KEYCODE_ESC],			"\x1B\x1B\x1B\x1B",4);
	memcpy(a2_key_remap[KEYCODE_SPACE],			"\x20\x20\x20\x20",4);
	memcpy(a2_key_remap[KEYCODE_QUOTE],			"\x27\x27\x22\x22",4);
	memcpy(a2_key_remap[KEYCODE_COMMA],			"\x2C\x2C\x3C\x3C",4);
	memcpy(a2_key_remap[KEYCODE_MINUS],			"\x2D\x2D\x5F\x1F",4);
	memcpy(a2_key_remap[KEYCODE_STOP],			"\x2E\x2E\x3E\x3E",4);
	memcpy(a2_key_remap[KEYCODE_SLASH],			"\x2F\x2F\x3F\x3F",4);
	memcpy(a2_key_remap[KEYCODE_0],				"\x30\x30\x29\x29",4);
	memcpy(a2_key_remap[KEYCODE_1],				"\x31\x31\x21\x21",4);
	memcpy(a2_key_remap[KEYCODE_2],				"\x32\x00\x40\x00",4);
	memcpy(a2_key_remap[KEYCODE_3],				"\x33\x33\x23\x23",4);
	memcpy(a2_key_remap[KEYCODE_4],				"\x34\x34\x24\x24",4);
	memcpy(a2_key_remap[KEYCODE_5],				"\x35\x35\x25\x25",4);
	memcpy(a2_key_remap[KEYCODE_6],				"\x36\x1E\x5E\x1E",4);
	memcpy(a2_key_remap[KEYCODE_7],				"\x37\x37\x26\x26",4);
	memcpy(a2_key_remap[KEYCODE_8],				"\x38\x38\x2A\x2A",4);
	memcpy(a2_key_remap[KEYCODE_9],				"\x39\x39\x28\x28",4);
	memcpy(a2_key_remap[KEYCODE_COLON],			"\x3B\x3B\x3A\x3A",4);
	memcpy(a2_key_remap[KEYCODE_EQUALS],		"\x3D\x3D\x2B\x2B",4);
	memcpy(a2_key_remap[KEYCODE_OPENBRACE],		"\x5B\x1B\x7B\x1B",4);
	/* No backslash defined in osdepend.h? */
//	memcpy(a2_key_remap[KEYCODE_BACKSLASH],		"\x5C\x1C\x7C\x1C",4);
	memcpy(a2_key_remap[KEYCODE_CLOSEBRACE],	"\x5D\x1D\x7D\x1D",4);
	memcpy(a2_key_remap[KEYCODE_TILDE],			"\x60\x60\x7E\x7E",4);
	memcpy(a2_key_remap[KEYCODE_A],				"\x61\x01\x41\x01",4);
	memcpy(a2_key_remap[KEYCODE_B],				"\x62\x02\x42\x02",4);
	memcpy(a2_key_remap[KEYCODE_C],				"\x63\x03\x43\x03",4);
	memcpy(a2_key_remap[KEYCODE_D],				"\x64\x04\x44\x04",4);
	memcpy(a2_key_remap[KEYCODE_E],				"\x65\x05\x45\x05",4);
	memcpy(a2_key_remap[KEYCODE_F],				"\x66\x06\x46\x06",4);
	memcpy(a2_key_remap[KEYCODE_G],				"\x67\x07\x47\x07",4);
	memcpy(a2_key_remap[KEYCODE_H],				"\x68\x08\x48\x08",4);
	memcpy(a2_key_remap[KEYCODE_I],				"\x69\x09\x49\x09",4);
	memcpy(a2_key_remap[KEYCODE_J],				"\x6A\x0A\x4A\x0A",4);
	memcpy(a2_key_remap[KEYCODE_K],				"\x6B\x0B\x4B\x0B",4);
	memcpy(a2_key_remap[KEYCODE_L],				"\x6C\x0C\x4C\x0C",4);
	memcpy(a2_key_remap[KEYCODE_M],				"\x6D\x0D\x4D\x0D",4);
	memcpy(a2_key_remap[KEYCODE_N],				"\x6E\x0E\x4E\x0E",4);
	memcpy(a2_key_remap[KEYCODE_O],				"\x6F\x0F\x4F\x0F",4);
	memcpy(a2_key_remap[KEYCODE_P],				"\x70\x10\x50\x10",4);
	memcpy(a2_key_remap[KEYCODE_Q],				"\x71\x11\x51\x11",4);
	memcpy(a2_key_remap[KEYCODE_R],				"\x72\x12\x52\x12",4);
	memcpy(a2_key_remap[KEYCODE_S],				"\x73\x13\x53\x13",4);
	memcpy(a2_key_remap[KEYCODE_T],				"\x74\x14\x54\x14",4);
	memcpy(a2_key_remap[KEYCODE_U],				"\x75\x15\x55\x15",4);
	memcpy(a2_key_remap[KEYCODE_V],				"\x76\x16\x56\x16",4);
	memcpy(a2_key_remap[KEYCODE_W],				"\x77\x17\x57\x17",4);
	memcpy(a2_key_remap[KEYCODE_X],				"\x78\x18\x58\x18",4);
	memcpy(a2_key_remap[KEYCODE_Y],				"\x79\x19\x59\x19",4);
	memcpy(a2_key_remap[KEYCODE_Z],				"\x7A\x1A\x5A\x1A",4);
}

/***************************************************************************
  AY3600_interrupt
***************************************************************************/
void AY3600_interrupt(void)
{
	int switchkey;	/* Normal, Shift, Control, or both */
	int keys;		/* Loop variable to run through keys */
	char any_key_pressed = 0;	/* Have we pressed a key? True/False */
	char caps_lock = 0;
	int curkey;

	static int last_key = 0xff;		/* Necessary for special repeat key behaviour */
	static unsigned int last_time = 65001;	/* Necessary for special repeat key behaviour */

	static unsigned int time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;

	switchkey = A2_KEY_NORMAL;

	/* Check for these special cases because they affect the emulated key codes */

	/* Check caps lock and set LED here */
	if (input_port_1_r(0) & 0x01)
	{
		caps_lock = 1;
		osd_led_w(1,1);
	}
	else
		osd_led_w(1,0);

	/* Shift key check */
	if ((osd_is_key_pressed(KEYCODE_LSHIFT)) || (osd_is_key_pressed(KEYCODE_RSHIFT)))
		switchkey = A2_KEY_SHIFT;

	/* Control key check - only one control key on the left side on the Apple */
	if (osd_is_key_pressed(KEYCODE_LCONTROL))
		switchkey |= A2_KEY_CONTROL;

	/* Run through real keys and see what's being pressed */
	anykey_clearstrobe = 0x00;
	keydata_strobe &= 0x80;
	for (keys=0;keys<=MAX_KEYS;keys++)
	{
		/* Filter out unmapped keys */
		if ((curkey = a2_key_remap[keys][switchkey])!=0xFF)
		{
			if (osd_is_key_pressed(keys))
			{
				any_key_pressed = 1;

				/* Prevent overflow */
				if (a2_keys[curkey]<65000)
					a2_keys[curkey]++;

				/* On every key press, reset the time until repeat and the key to repeat */
				if (a2_keys[curkey]==1)
				{
					time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;
					last_key = curkey;
					last_time = 1;
				}
			}
			else
				a2_keys[curkey]=0;
		}
	}

	/* If no keys have been pressed, reset the repeat time and return */
	if (!any_key_pressed)
	{
		time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;
		last_key = 0xFF;
		last_time = 65001;
		return;
	}
	/* Otherwise, count down the repeat time */
	else
	{
		if (time_until_repeat>0)
			time_until_repeat--;
	}

	/* Even if a key has been released, repeat it if it was the last key pressed */
	/* If we should output a key, set the appropriate Apple II data lines */
	if ((time_until_repeat==0) || (time_until_repeat==(MAGIC_KEY_REPEAT_NUMBER-1)))
	{
		anykey_clearstrobe = keydata_strobe = 0x80;
		keydata_strobe |= last_key;
	}
}

/***************************************************************************
  AY3600_keydata_strobe_r
***************************************************************************/
int AY3600_keydata_strobe_r(void)
{
	return keydata_strobe;
}


/***************************************************************************
  AY3600_anykey_clearstrobe_r
***************************************************************************/
int AY3600_anykey_clearstrobe_r(void)
{
	keydata_strobe &= 0x7F;
	return anykey_clearstrobe;
}


