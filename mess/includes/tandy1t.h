extern WRITE8_HANDLER ( pc_t1t_p37x_w );
extern  READ8_HANDLER ( pc_t1t_p37x_r );

extern WRITE8_HANDLER ( tandy1000_pio_w );
extern  READ8_HANDLER(tandy1000_pio_r);

extern NVRAM_HANDLER( tandy1000 );

#define T1000_HELPER(bit,text,key1,key2) \
	PORT_BITX( bit, 0x0000, IPT_KEYBOARD, text, key1, key2 )

#define TANDY1000_KEYB \
	PORT_START	/* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	T1000_HELPER( 0x0002, "Esc",          KEYCODE_ESC,        CODE_NONE ) /* Esc                         01  81 */\
	T1000_HELPER( 0x0004, "1 !",          KEYCODE_1,          CODE_NONE ) /* 1                           02  82 */\
	T1000_HELPER( 0x0008, "2 @",          KEYCODE_2,          CODE_NONE ) /* 2                           03  83 */\
	T1000_HELPER( 0x0010, "3 #",          KEYCODE_3,          CODE_NONE ) /* 3                           04  84 */\
	T1000_HELPER( 0x0020, "4 $",          KEYCODE_4,          CODE_NONE ) /* 4                           05  85 */\
	T1000_HELPER( 0x0040, "5 %",          KEYCODE_5,          CODE_NONE ) /* 5                           06  86 */\
	T1000_HELPER( 0x0080, "6 ^",          KEYCODE_6,          CODE_NONE ) /* 6                           07  87 */\
	T1000_HELPER( 0x0100, "7 &",          KEYCODE_7,          CODE_NONE ) /* 7                           08  88 */\
	T1000_HELPER( 0x0200, "8 *",          KEYCODE_8,          CODE_NONE ) /* 8                           09  89 */\
	T1000_HELPER( 0x0400, "9 (",          KEYCODE_9,          CODE_NONE ) /* 9                           0A  8A */\
	T1000_HELPER( 0x0800, "0 )",          KEYCODE_0,          CODE_NONE ) /* 0                           0B  8B */\
	T1000_HELPER( 0x1000, "- _",          KEYCODE_MINUS,      CODE_NONE ) /* -                           0C  8C */\
	T1000_HELPER( 0x2000, "= +",          KEYCODE_EQUALS,     CODE_NONE ) /* =                           0D  8D */\
	T1000_HELPER( 0x4000, "<--",          KEYCODE_BACKSPACE,  CODE_NONE ) /* Backspace                   0E  8E */\
	T1000_HELPER( 0x8000, "Tab",          KEYCODE_TAB,        CODE_NONE ) /* Tab                         0F  8F */\
		\
	PORT_START	/* IN5 */\
	T1000_HELPER( 0x0001, "Q",            KEYCODE_Q,          CODE_NONE ) /* Q                           10  90 */\
	T1000_HELPER( 0x0002, "W",            KEYCODE_W,          CODE_NONE ) /* W                           11  91 */\
	T1000_HELPER( 0x0004, "E",            KEYCODE_E,          CODE_NONE ) /* E                           12  92 */\
	T1000_HELPER( 0x0008, "R",            KEYCODE_R,          CODE_NONE ) /* R                           13  93 */\
	T1000_HELPER( 0x0010, "T",            KEYCODE_T,          CODE_NONE ) /* T                           14  94 */\
	T1000_HELPER( 0x0020, "Y",            KEYCODE_Y,          CODE_NONE ) /* Y                           15  95 */\
	T1000_HELPER( 0x0040, "U",            KEYCODE_U,          CODE_NONE ) /* U                           16  96 */\
	T1000_HELPER( 0x0080, "I",            KEYCODE_I,          CODE_NONE ) /* I                           17  97 */\
	T1000_HELPER( 0x0100, "O",            KEYCODE_O,          CODE_NONE ) /* O                           18  98 */\
	T1000_HELPER( 0x0200, "P",            KEYCODE_P,          CODE_NONE ) /* P                           19  99 */\
	T1000_HELPER( 0x0400, "[ {",          KEYCODE_OPENBRACE,  CODE_NONE ) /* [                           1A  9A */\
	T1000_HELPER( 0x0800, "] }",          KEYCODE_CLOSEBRACE, CODE_NONE ) /* ]                           1B  9B */\
	T1000_HELPER( 0x1000, "Enter",        KEYCODE_ENTER,      CODE_NONE ) /* Enter                       1C  9C */\
	T1000_HELPER( 0x2000, "L-Ctrl",       KEYCODE_LCONTROL,   CODE_NONE ) /* Left Ctrl                   1D  9D */\
	T1000_HELPER( 0x4000, "A",            KEYCODE_A,          CODE_NONE ) /* A                           1E  9E */\
	T1000_HELPER( 0x8000, "S",            KEYCODE_S,          CODE_NONE ) /* S                           1F  9F */\
		\
	PORT_START	/* IN6 */\
	T1000_HELPER( 0x0001, "D",            KEYCODE_D,          CODE_NONE ) /* D                           20  A0 */\
	T1000_HELPER( 0x0002, "F",            KEYCODE_F,          CODE_NONE ) /* F                           21  A1 */\
	T1000_HELPER( 0x0004, "G",            KEYCODE_G,          CODE_NONE ) /* G                           22  A2 */\
	T1000_HELPER( 0x0008, "H",            KEYCODE_H,          CODE_NONE ) /* H                           23  A3 */\
	T1000_HELPER( 0x0010, "J",            KEYCODE_J,          CODE_NONE ) /* J                           24  A4 */\
	T1000_HELPER( 0x0020, "K",            KEYCODE_K,          CODE_NONE ) /* K                           25  A5 */\
	T1000_HELPER( 0x0040, "L",            KEYCODE_L,          CODE_NONE ) /* L                           26  A6 */\
	T1000_HELPER( 0x0080, "; :",          KEYCODE_COLON,      CODE_NONE ) /* ;                           27  A7 */\
	T1000_HELPER( 0x0100, "' \"",         KEYCODE_QUOTE,      CODE_NONE ) /* '                           28  A8 */\
	T1000_HELPER( 0x0200, "Cursor Up",    KEYCODE_UP,		  CODE_NONE ) /*                             29  A9 */\
	T1000_HELPER( 0x0400, "L-Shift",      KEYCODE_LSHIFT,     CODE_NONE ) /* Left Shift                  2A  AA */\
	T1000_HELPER( 0x0800, "Cursor Left",  KEYCODE_LEFT,		  CODE_NONE ) /*                             2B  AB */\
	T1000_HELPER( 0x1000, "Z",            KEYCODE_Z,          CODE_NONE ) /* Z                           2C  AC */\
	T1000_HELPER( 0x2000, "X",            KEYCODE_X,          CODE_NONE ) /* X                           2D  AD */\
	T1000_HELPER( 0x4000, "C",            KEYCODE_C,          CODE_NONE ) /* C                           2E  AE */\
	T1000_HELPER( 0x8000, "V",            KEYCODE_V,          CODE_NONE ) /* V                           2F  AF */\
		\
	PORT_START	/* IN7 */\
	T1000_HELPER( 0x0001, "B",            KEYCODE_B,          CODE_NONE ) /* B                           30  B0 */\
	T1000_HELPER( 0x0002, "N",            KEYCODE_N,          CODE_NONE ) /* N                           31  B1 */\
	T1000_HELPER( 0x0004, "M",            KEYCODE_M,          CODE_NONE ) /* M                           32  B2 */\
	T1000_HELPER( 0x0008, ", <",          KEYCODE_COMMA,      CODE_NONE ) /* ,                           33  B3 */\
	T1000_HELPER( 0x0010, ". >",          KEYCODE_STOP,       CODE_NONE ) /* .                           34  B4 */\
	T1000_HELPER( 0x0020, "/ ?",          KEYCODE_SLASH,      CODE_NONE ) /* /                           35  B5 */\
	T1000_HELPER( 0x0040, "R-Shift",      KEYCODE_RSHIFT,     CODE_NONE ) /* Right Shift                 36  B6 */\
	T1000_HELPER( 0x0080, "Print",		  CODE_NONE,		  CODE_NONE ) /*                             37  B7 */\
	T1000_HELPER( 0x0100, "Alt",          KEYCODE_LALT,       CODE_NONE ) /* Left Alt                    38  B8 */\
	T1000_HELPER( 0x0200, "Space",        KEYCODE_SPACE,      CODE_NONE ) /* Space                       39  B9 */\
	T1000_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK,   CODE_NONE ) /* Caps Lock                   3A  BA */\
	T1000_HELPER( 0x0800, "F1",           KEYCODE_F1,         CODE_NONE ) /* F1                          3B  BB */\
	T1000_HELPER( 0x1000, "F2",           KEYCODE_F2,         CODE_NONE ) /* F2                          3C  BC */\
	T1000_HELPER( 0x2000, "F3",           KEYCODE_F3,         CODE_NONE ) /* F3                          3D  BD */\
	T1000_HELPER( 0x4000, "F4",           KEYCODE_F4,         CODE_NONE ) /* F4                          3E  BE */\
	T1000_HELPER( 0x8000, "F5",           KEYCODE_F5,         CODE_NONE ) /* F5                          3F  BF */\
		\
	PORT_START	/* IN8 */\
	T1000_HELPER( 0x0001, "F6",           KEYCODE_F6,         CODE_NONE ) /* F6                          40  C0 */\
	T1000_HELPER( 0x0002, "F7",           KEYCODE_F7,         CODE_NONE ) /* F7                          41  C1 */\
	T1000_HELPER( 0x0004, "F8",           KEYCODE_F8,         CODE_NONE ) /* F8                          42  C2 */\
	T1000_HELPER( 0x0008, "F9",           KEYCODE_F9,         CODE_NONE ) /* F9                          43  C3 */\
	T1000_HELPER( 0x0010, "F10",          KEYCODE_F10,        CODE_NONE ) /* F10                         44  C4 */\
	T1000_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK,    CODE_NONE ) /* Num Lock                    45  C5 */\
	T1000_HELPER( 0x0040, "Hold",		  KEYCODE_SCRLOCK,    CODE_NONE ) /*		                     46  C6 */\
	T1000_HELPER( 0x0080, "KP 7 /",		  KEYCODE_7_PAD,      CODE_NONE ) /* Keypad 7                    47  C7 */\
	T1000_HELPER( 0x0100, "KP 8 ~",		  KEYCODE_8_PAD,      CODE_NONE ) /* Keypad 8                    48  C8 */\
	T1000_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD,      CODE_NONE ) /* Keypad 9  (PgUp)            49  C9 */\
	T1000_HELPER( 0x0400, "Cursor Down",  KEYCODE_DOWN,		  CODE_NONE ) /*                             4A  CA */\
	T1000_HELPER( 0x0800, "KP 4 |",		  KEYCODE_4_PAD,	  CODE_NONE ) /* Keypad 4                    4B  CB */\
	T1000_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD,      CODE_NONE ) /* Keypad 5                    4C  CC */\
	T1000_HELPER( 0x2000, "KP 6",		  KEYCODE_6_PAD,	  CODE_NONE ) /* Keypad 6                    4D  CD */\
	T1000_HELPER( 0x4000, "Cursor Right", KEYCODE_RIGHT,	  CODE_NONE ) /*                             4E  CE */\
	T1000_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD,      CODE_NONE ) /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START	/* IN9 */\
	T1000_HELPER( 0x0001, "KP 2 `",		  KEYCODE_2_PAD,	  CODE_NONE ) /* Keypad 2                    50  D0 */\
	T1000_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD,      CODE_NONE ) /* Keypad 3  (PgDn)            51  D1 */\
	T1000_HELPER( 0x0004, "KP 0",		  KEYCODE_0_PAD,      CODE_NONE ) /* Keypad 0                    52  D2 */\
	T1000_HELPER( 0x0008, "KP - (Del)",   KEYCODE_MINUS_PAD,  CODE_NONE ) /* - Delete                    53  D3 */\
	T1000_HELPER( 0x0010, "Break",		  KEYCODE_STOP,       CODE_NONE ) /* Break                       54  D4 */\
	T1000_HELPER( 0x0020, "+ Insert",	  KEYCODE_PLUS_PAD,	  CODE_NONE ) /* + Insert                    55  D5 */\
	T1000_HELPER( 0x0040, ".",			  KEYCODE_DEL_PAD,    CODE_NONE ) /* .                           56  D6 */\
	T1000_HELPER( 0x0080, "Enter",		  KEYCODE_ENTER_PAD,  CODE_NONE ) /* Enter                       57  D7 */\
	T1000_HELPER( 0x0100, "Home",		  KEYCODE_HOME,       CODE_NONE ) /* HOME                        58  D8 */\
	T1000_HELPER( 0x0200, "F11",		  KEYCODE_F11,        CODE_NONE ) /* F11                         59  D9 */\
	T1000_HELPER( 0x0400, "F12",		  KEYCODE_F12,        CODE_NONE ) /* F12                         5a  Da */\
		\
	PORT_START	/* IN10 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN11 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

