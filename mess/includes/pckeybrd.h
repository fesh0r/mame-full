#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	AT_KEYBOARD_TYPE_PC,
	AT_KEYBOARD_TYPE_AT,
	AT_KEYBOARD_TYPE_MF2
} AT_KEYBOARD_TYPE;

void at_keyboard_polling(void);
int at_keyboard_read(void);
void at_keyboard_write(UINT8 data);
void at_keyboard_init(void);
void pc_keyboard_init(void);
void at_keyboard_set_scan_code_set(int set);
void at_keyboard_set_input_port_base(int base);
void at_keyboard_set_type(AT_KEYBOARD_TYPE type);

/*
#define KEYBOARD_ON 1
#define PS2_MOUSE_ON 1
*/

#define AT_KEYBOARD \
	PORT_START	/* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */\
		\
	PORT_START	/* IN5 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */\
		\
	PORT_START	/* IN6 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"` ~",          KEYCODE_TILDE,      IP_JOY_NONE ) /* `                           29  A9 */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"\\ |",         KEYCODE_BACKSLASH,  IP_JOY_NONE ) /* \                           2B  AB */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */\
		\
	PORT_START	/* IN7 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP * (PrtScr)",KEYCODE_ASTERISK,   IP_JOY_NONE ) /* Keypad *  (PrtSc)           37  B7 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */\
		\
	PORT_START	/* IN8 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"ScrLock",      KEYCODE_SCRLOCK,    IP_JOY_NONE ) /* Scroll Lock                 46  C6 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 7 (Home)",  KEYCODE_7_PAD,      IP_JOY_NONE )/* Keypad 7  (Home)            47  C7 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 8 (Up)",    KEYCODE_8_PAD,      IP_JOY_NONE )  /* Keypad 8  (Up arrow)        48  C8 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_9_PAD,      IP_JOY_NONE ) /* Keypad 9  (PgUp)            49  C9 */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP -",         KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* Keypad -                    4A  CA */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 4 (Left)",  KEYCODE_4_PAD,      IP_JOY_NONE )/* Keypad 4  (Left arrow)      4B  CB */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 6 (Right)", KEYCODE_6_PAD,      IP_JOY_NONE )/* Keypad 6  (Right arrow)     4D  CD */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP +",         KEYCODE_PLUS_PAD,   IP_JOY_NONE ) /* Keypad +                    4E  CE */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_1_PAD,      IP_JOY_NONE ) /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START	/* IN9 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 2 (Down)",  KEYCODE_2_PAD,      IP_JOY_NONE ) /* Keypad 2  (Down arrow)      50  D0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_3_PAD,      IP_JOY_NONE ) /* Keypad 3  (PgDn)            51  D1 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP 0 (Ins)",   KEYCODE_0_PAD,      IP_JOY_NONE ) /* Keypad 0  (Ins)             52  D2 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"KP . (Del)",   KEYCODE_DEL_PAD,    IP_JOY_NONE ) /* Keypad .  (Del)             53  D3 */\
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(84/102)\\",   KEYCODE_BACKSLASH2, IP_JOY_NONE ) /* Backslash 2                 56  D6 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)F11",		KEYCODE_F11,        IP_JOY_NONE ) /* F11                         57  D7 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)F12",		KEYCODE_F12,        IP_JOY_NONE ) /* F12                         58  D8 */\
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN10 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)KP Enter",		KEYCODE_ENTER_PAD,  IP_JOY_NONE ) /* PAD Enter                   60  e0 */\
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Right Control",	KEYCODE_RCONTROL,   IP_JOY_NONE ) /* Right Control               61  e1 */\
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)KP /",			KEYCODE_SLASH_PAD,  IP_JOY_NONE ) /* PAD Slash                   62  e2 */\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)PRTSCR",			KEYCODE_PRTSCR,     IP_JOY_NONE ) /* Print Screen                63  e3 */\
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)ALTGR",			KEYCODE_RALT,       IP_JOY_NONE ) /* ALTGR                       64  e4 */\
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Home",			KEYCODE_HOME,       IP_JOY_NONE ) /* Home                        66  e6 */\
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Cursor Up",		KEYCODE_UP,         IP_JOY_NONE ) /* Up                          67  e7 */\
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Page Up",			KEYCODE_PGUP,       IP_JOY_NONE ) /* Page Up                     68  e8 */\
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Cursor Left",		KEYCODE_LEFT,       IP_JOY_NONE ) /* Left                        69  e9 */\
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Cursor Right",	KEYCODE_RIGHT,      IP_JOY_NONE ) /* Right                       6a  ea */\
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)End",				KEYCODE_END,        IP_JOY_NONE ) /* End                         6b  eb */\
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Cursor Down",		KEYCODE_DOWN,       IP_JOY_NONE ) /* Down                        6c  ec */\
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Page Down",		KEYCODE_PGDN,       IP_JOY_NONE ) /* Page Down                   6d  ed */\
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Insert",			KEYCODE_INSERT,     IP_JOY_NONE ) /* Insert                      6e  ee */\
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Delete",			KEYCODE_DEL,        IP_JOY_NONE ) /* Delete                      6f  ef */\
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"(MF2)Pause",			KEYCODE_PAUSE,      IP_JOY_NONE ) /* Pause                       65  e5 */\
	PORT_START	/* IN11 */\
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Print Screen", KEYCODE_PRTSCR,          IP_JOY_NONE ) /* Print Screen alternate      77  f7 */\
	PORT_BIT ( 0xfffe, 0x0000, IPT_UNUSED )

#if 0
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Left Win",     CODE_NONE,          IP_JOY_NONE ) /* Left Win                    7d  fd */
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Right Win",    CODE_NONE,          IP_JOY_NONE ) /* Right Win                   7e  fe */
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Menu",         CODE_NONE,          IP_JOY_NONE ) /* Menu                        7f  ff */
#endif

#ifdef __cplusplus
}
#endif

