#include "driver.h"
#include <ctype.h>

struct KEYLAYOUT {
	UINT16 code;
	UINT8  x,y,w,h;
};

static struct KEYLAYOUT keylayout[] =
{
	{KEYCODE_ESC,		 0, 0, 4,4},	// Escape
	{KEYCODE_F1,		 8, 0, 4,4},	// F1
	{KEYCODE_F2,		12, 0, 4,4},	// F2
	{KEYCODE_F3,		16, 0, 4,4},	// F3
	{KEYCODE_F4,		20, 0, 4,4},	// F4
	{KEYCODE_F5,		26, 0, 4,4},	// F5
	{KEYCODE_F6,		30, 0, 4,4},	// F6
	{KEYCODE_F7,		34, 0, 4,4},	// F7
	{KEYCODE_F8,		38, 0, 4,4},	// F8
	{KEYCODE_F9,		44, 0, 4,4},	// F9
	{KEYCODE_F10,		48, 0, 4,4},	// F10
	{KEYCODE_F11,		52, 0, 4,4},	// F11
	{KEYCODE_F12,		56, 0, 4,4},	// F12
	{KEYCODE_PRTSCR,	62, 0, 4,4},	// PrtScr/SysReq
	{KEYCODE_SCRLOCK,	66, 0, 4,4},	// ScrollLock
	{KEYCODE_PAUSE, 	70, 0, 4,4},	// Pause/Break
	{KEYCODE_TILDE, 	 0, 5, 4,4},	// \ | (^ ø)
	{KEYCODE_1, 		 4, 5, 4,4},	// 1
	{KEYCODE_2, 		 8, 5, 4,4},	// 2
	{KEYCODE_3, 		12, 5, 4,4},	// 3
	{KEYCODE_4, 		16, 5, 4,4},	// 4
	{KEYCODE_5, 		20, 5, 4,4},	// 5
	{KEYCODE_6, 		24, 5, 4,4},	// 6
	{KEYCODE_7, 		28, 5, 4,4},	// 7
	{KEYCODE_8, 		32, 5, 4,4},	// 8
	{KEYCODE_9, 		36, 5, 4,4},	// 9
	{KEYCODE_0, 		40, 5, 4,4},	// 0
	{KEYCODE_MINUS, 	44, 5, 4,4},	// - _ (á ? \)
	{KEYCODE_EQUALS,	48, 5, 4,4},	// = + (' `)
	{KEYCODE_BACKSPACE, 52, 5, 8,4},	// Backspace
	{KEYCODE_INSERT,	62, 5, 4,4},	//+Insert
	{KEYCODE_HOME,		66, 5, 4,4},	//+Home
	{KEYCODE_PGUP,		70, 5, 4,4},	//+PageUp
	{KEYCODE_NUMLOCK,	76, 5, 4,4},	// NumLock
	{KEYCODE_SLASH_PAD, 80, 5, 4,4},	// NumPad ö
	{KEYCODE_ASTERISK,	84, 5, 4,4},	// NumPad ù
	{KEYCODE_MINUS_PAD, 88, 5, 4,4},	// NumPad -
	{KEYCODE_TAB,		 0, 9, 6,4},	// Tab
	{KEYCODE_Q, 		 6, 9, 4,4},	// Q
	{KEYCODE_W, 		10, 9, 4,4},	// W
	{KEYCODE_E, 		14, 9, 4,4},	// E
	{KEYCODE_R, 		18, 9, 4,4},	// R
	{KEYCODE_T, 		22, 9, 4,4},	// T
	{KEYCODE_Y, 		26, 9, 4,4},	// Y
	{KEYCODE_U, 		30, 9, 4,4},	// U
	{KEYCODE_I, 		34, 9, 4,4},	// I
	{KEYCODE_O, 		38, 9, 4,4},	// O
	{KEYCODE_P, 		42, 9, 4,4},	// P
	{KEYCODE_OPENBRACE, 46, 9, 4,4},	// [
	{KEYCODE_CLOSEBRACE,50, 9, 4,4},	// ]
	{KEYCODE_ENTER, 	54, 9, 6,8},	// Enter
	{KEYCODE_DEL,		62, 9, 4,4},	//+Del
	{KEYCODE_END,		66, 9, 4,4},	//+End
	{KEYCODE_PGDN,		70, 9, 4,4},	//+PageDn
	{KEYCODE_7_PAD, 	76, 9, 4,4},	// NumPad 7
	{KEYCODE_8_PAD, 	80, 9, 4,4},	// NumPad 8
	{KEYCODE_9_PAD, 	84, 9, 4,4},	// NumPad 9
	{KEYCODE_CAPSLOCK,	 0,13, 7,4},	// CapsLock
	{KEYCODE_A, 		 7,13, 4,4},	// A
	{KEYCODE_S, 		11,13, 4,4},	// S
	{KEYCODE_D, 		15,13, 4,4},	// D
	{KEYCODE_F, 		19,13, 4,4},	// F
	{KEYCODE_G, 		23,13, 4,4},	// G
	{KEYCODE_H, 		27,13, 4,4},	// H
	{KEYCODE_J, 		31,13, 4,4},	// J
	{KEYCODE_K, 		35,13, 4,4},	// K
	{KEYCODE_L, 		39,13, 4,4},	// L
	{KEYCODE_COLON, 	43,13, 4,4},	// ; :
	{KEYCODE_QUOTE, 	47,13, 4,4},	// ' "
	{KEYCODE_BACKSLASH2,51,13, 4,4},	// ` ~
	{KEYCODE_4_PAD, 	76,13, 4,4},	// NumPad 4
	{KEYCODE_5_PAD, 	80,13, 4,4},	// NumPad 5
	{KEYCODE_6_PAD, 	84,13, 4,4},	// NumPad 6
	{KEYCODE_PLUS_PAD,	88, 9, 4,8},	// NumPad +
	{KEYCODE_LSHIFT,	 0,17, 5,4},	// Left Shift
	{KEYCODE_BACKSLASH,  5,17, 4,4},	// \ |
	{KEYCODE_Z, 		 9,17, 4,4},	// Z
	{KEYCODE_X, 		13,17, 4,4},	// X
	{KEYCODE_C, 		17,17, 4,4},	// C
	{KEYCODE_V, 		21,17, 4,4},	// V
	{KEYCODE_B, 		25,17, 4,4},	// B
	{KEYCODE_N, 		29,17, 4,4},	// N
	{KEYCODE_M, 		33,17, 4,4},	// M
	{KEYCODE_COMMA, 	37,17, 4,4},	// , <
	{KEYCODE_STOP,		41,17, 4,4},	// . >
	{KEYCODE_SLASH, 	45,17, 4,4},	// / ?
	{KEYCODE_RSHIFT,	49,17,11,4},	// Right Shift
	{KEYCODE_UP,		66,17, 4,4},	//+UpArrow
	{KEYCODE_1_PAD, 	76,17, 4,4},	// NumPad 1
	{KEYCODE_2_PAD, 	80,17, 4,4},	// NumPad 2
	{KEYCODE_3_PAD, 	84,17, 4,4},	// NumPad 3
	{KEYCODE_LCONTROL,	 0,21, 6,4},	// Control
	{KEYCODE_LALT,		10,21, 6,4},	// Left Alt
	{KEYCODE_SPACE, 	16,21,28,4},	// SpaceBar
	{KEYCODE_RALT,		44,21, 6,4},	//+Right Alt
	{KEYCODE_RCONTROL,	54,21, 6,4},	//+Right Control
	{KEYCODE_LEFT,		62,21, 4,4},	// LeftArrow
	{KEYCODE_DOWN,		66,21, 4,4},	// DownArrow
	{KEYCODE_RIGHT, 	70,21, 4,4},	// RightArrow
	{KEYCODE_0_PAD, 	76,21, 8,4},	// Numpad 0
	{KEYCODE_DEL_PAD,	84,21, 4,4},	// Numpad ,
	{KEYCODE_ENTER_PAD, 88,17, 4,8},	// NumPad Enter
	{KEYCODE_NONE,		 0, 0, 0,0}
};

#define XM	92
#define YM	25

static void scale(struct KEYLAYOUT *k, int *x, int *y, int *w, int *h)
{
	*x = Machine->uixmin + Machine->uiwidth * k->x / XM;
	*y = Machine->uiymin + Machine->uiwidth * k->y / XM +
		(Machine->uiheight - Machine->uiwidth * YM / XM) / 2;
	*w = Machine->uiwidth * k->w / XM;
	*h = Machine->uiwidth * k->h / XM;
}

int displaykeyboard(int select)
{
	struct KEYLAYOUT *k = keylayout;
	int white = Machine->uifont->colortable[1];

	osd_clearbitmap(Machine->scrbitmap);
    while (k->code != KEYCODE_NONE)
	{
		struct InputPortTiny *in = (struct InputPortTiny *)Machine->input_ports;
		int i,x,y,w,h;

        scale(k,&x,&y,&w,&h);

		while (in->type != IPT_END)
        {
			if( (in->type & ~IPF_MASK) == IPT_KEYBOARD )
            {
				const char *src = in->name;
				in++;
				if( in->mask == k->code )
				{
					char name[32+1];
					int wt = w / Machine->uifontwidth;

                    if (isdigit(src[0]) &&
						src[1] == '.' &&
						isdigit(src[2]) &&
						src[3] == ':' )
						strncpy(name, src + 5, wt);
					else
						strncpy(name, src, wt);
					name[wt] = '\0';
					wt = strlen(name);
					ui_text(name, x + 1 + (w - wt * Machine->uifontwidth)/2, y + 1 + (h - Machine->uifontheight) / 2);
				}
            }
            in++;
        }

        for (i = 1; i < w-1; i++)
		{
			plot_pixel(Machine->scrbitmap,x+i,y,white);
			plot_pixel(Machine->scrbitmap,x+i,y+h-1,white);
		}

        for (i = 1; i < h-1; i++)
		{
			plot_pixel(Machine->scrbitmap,x,y+i,white);
			plot_pixel(Machine->scrbitmap,x+w-1,y+i,white);
        }

        k++;
    }
	if (code_read_async() == CODE_NONE)
		return 1;
	return 0;
}
