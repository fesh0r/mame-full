#include "inputx.h"

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
void at_keyboard_init(AT_KEYBOARD_TYPE type);
void at_keyboard_reset(void);
void at_keyboard_set_scan_code_set(int set);
void at_keyboard_set_input_port_base(int base);

QUEUE_CHARS( at_keyboard );
ACCEPT_CHAR( at_keyboard );
CHARQUEUE_EMPTY( at_keyboard );

/*
#define KEYBOARD_ON 1
#define PS2_MOUSE_ON 1
*/
#define PC_KEYB_HELPER(bit,text,key1,key2) \
	PORT_BITX( bit, 0x0000, IPT_KEYBOARD, text, key1, key2 )

#define PC_KEYBOARD \
    PORT_START  /* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	PORT_KEY1( 0x0002, 0x0000, IP_NAME_DEFAULT, KEYCODE_ESC,        CODE_NONE, 26 )			/* Esc                         01  81 */\
	PORT_KEY2( 0x0004, 0x0000, IP_NAME_DEFAULT, KEYCODE_1,          CODE_NONE, '1', '!' )	/* 1                           02  82 */\
	PORT_KEY2( 0x0008, 0x0000, IP_NAME_DEFAULT, KEYCODE_2,          CODE_NONE, '2', '@' )	/* 2                           03  83 */\
	PORT_KEY2( 0x0010, 0x0000, IP_NAME_DEFAULT, KEYCODE_3,          CODE_NONE, '3', '#' )	/* 3                           04  84 */\
	PORT_KEY2( 0x0020, 0x0000, IP_NAME_DEFAULT, KEYCODE_4,          CODE_NONE, '4', '$' )	/* 4                           05  85 */\
	PORT_KEY2( 0x0040, 0x0000, IP_NAME_DEFAULT, KEYCODE_5,          CODE_NONE, '5', '%' )	/* 5                           06  86 */\
	PORT_KEY2( 0x0080, 0x0000, IP_NAME_DEFAULT, KEYCODE_6,          CODE_NONE, '6', '^' )	/* 6                           07  87 */\
	PORT_KEY2( 0x0100, 0x0000, IP_NAME_DEFAULT, KEYCODE_7,          CODE_NONE, '7', '&' )	/* 7                           08  88 */\
	PORT_KEY2( 0x0200, 0x0000, IP_NAME_DEFAULT, KEYCODE_8,          CODE_NONE, '8', '*' )	/* 8                           09  89 */\
	PORT_KEY2( 0x0400, 0x0000, IP_NAME_DEFAULT, KEYCODE_9,          CODE_NONE, '9', '(' )	/* 9                           0A  8A */\
	PORT_KEY2( 0x0800, 0x0000, IP_NAME_DEFAULT, KEYCODE_0,          CODE_NONE, '0', ')' )	/* 0                           0B  8B */\
	PORT_KEY2( 0x1000, 0x0000, IP_NAME_DEFAULT, KEYCODE_MINUS,      CODE_NONE, '-', '_' )	/* -                           0C  8C */\
	PORT_KEY2( 0x2000, 0x0000, IP_NAME_DEFAULT, KEYCODE_EQUALS,     CODE_NONE, '=', '+' )	/* =                           0D  8D */\
	PORT_KEY1( 0x4000, 0x0000, IP_NAME_DEFAULT, KEYCODE_BACKSPACE,  CODE_NONE, 8 )			/* Backspace                   0E  8E */\
	PORT_KEY1( 0x8000, 0x0000, IP_NAME_DEFAULT, KEYCODE_TAB,        CODE_NONE, 9 )			/* Tab                         0F  8F */\
		\
	PORT_START	/* IN5 */\
	PORT_KEY1( 0x0001, 0x0000, IP_NAME_DEFAULT, KEYCODE_Q,          CODE_NONE, 'Q' ) /* Q                           10  90 */\
	PORT_KEY1( 0x0002, 0x0000, IP_NAME_DEFAULT, KEYCODE_W,          CODE_NONE, 'W' ) /* W                           11  91 */\
	PORT_KEY1( 0x0004, 0x0000, IP_NAME_DEFAULT, KEYCODE_E,          CODE_NONE, 'E' ) /* E                           12  92 */\
	PORT_KEY1( 0x0008, 0x0000, IP_NAME_DEFAULT, KEYCODE_R,          CODE_NONE, 'R' ) /* R                           13  93 */\
	PORT_KEY1( 0x0010, 0x0000, IP_NAME_DEFAULT, KEYCODE_T,          CODE_NONE, 'T' ) /* T                           14  94 */\
	PORT_KEY1( 0x0020, 0x0000, IP_NAME_DEFAULT, KEYCODE_Y,          CODE_NONE, 'Y' ) /* Y                           15  95 */\
	PORT_KEY1( 0x0040, 0x0000, IP_NAME_DEFAULT, KEYCODE_U,          CODE_NONE, 'U' ) /* U                           16  96 */\
	PORT_KEY1( 0x0080, 0x0000, IP_NAME_DEFAULT, KEYCODE_I,          CODE_NONE, 'I' ) /* I                           17  97 */\
	PORT_KEY1( 0x0100, 0x0000, IP_NAME_DEFAULT, KEYCODE_O,          CODE_NONE, 'O' ) /* O                           18  98 */\
	PORT_KEY1( 0x0200, 0x0000, IP_NAME_DEFAULT, KEYCODE_P,          CODE_NONE, 'P' ) /* P                           19  99 */\
	PORT_KEY2( 0x0400, 0x0000, IP_NAME_DEFAULT, KEYCODE_OPENBRACE,  CODE_NONE, '[', '{' ) /* [                           1A  9A */\
	PORT_KEY2( 0x0800, 0x0000, IP_NAME_DEFAULT, KEYCODE_CLOSEBRACE, CODE_NONE, ']', '}' ) /* ]                           1B  9B */\
	PORT_KEY1( 0x1000, 0x0000, IP_NAME_DEFAULT, KEYCODE_ENTER,      CODE_NONE, 13  ) /* Enter                       1C  9C */\
	PORT_KEY1( 0x2000, 0x0000, IP_NAME_DEFAULT, KEYCODE_LCONTROL,   CODE_NONE, UCHAR_MAMEKEY(LCONTROL) )      /* Left Ctrl                   1D  9D */\
	PORT_KEY1( 0x4000, 0x0000, IP_NAME_DEFAULT, KEYCODE_A,          CODE_NONE, 'A' ) /* A                           1E  9E */\
	PORT_KEY1( 0x8000, 0x0000, IP_NAME_DEFAULT, KEYCODE_S,          CODE_NONE, 'S' ) /* S                           1F  9F */\
		\
	PORT_START	/* IN6 */\
	PORT_KEY1( 0x0001, 0x0000, IP_NAME_DEFAULT, KEYCODE_D,          CODE_NONE, 'D' ) /* D                           20  A0 */\
	PORT_KEY1( 0x0002, 0x0000, IP_NAME_DEFAULT, KEYCODE_F,          CODE_NONE, 'F' ) /* F                           21  A1 */\
	PORT_KEY1( 0x0004, 0x0000, IP_NAME_DEFAULT, KEYCODE_G,          CODE_NONE, 'G' ) /* G                           22  A2 */\
	PORT_KEY1( 0x0008, 0x0000, IP_NAME_DEFAULT, KEYCODE_H,          CODE_NONE, 'H' ) /* H                           23  A3 */\
	PORT_KEY1( 0x0010, 0x0000, IP_NAME_DEFAULT, KEYCODE_J,          CODE_NONE, 'J' ) /* J                           24  A4 */\
	PORT_KEY1( 0x0020, 0x0000, IP_NAME_DEFAULT, KEYCODE_K,          CODE_NONE, 'K' ) /* K                           25  A5 */\
	PORT_KEY1( 0x0040, 0x0000, IP_NAME_DEFAULT, KEYCODE_L,          CODE_NONE, 'L' ) /* L                           26  A6 */\
	PORT_KEY2( 0x0080, 0x0000, IP_NAME_DEFAULT, KEYCODE_COLON,      CODE_NONE, ';', ':' ) /* ;                           27  A7 */\
	PORT_KEY2( 0x0100, 0x0000, IP_NAME_DEFAULT, KEYCODE_QUOTE,      CODE_NONE, '\'', '\"' ) /* '                           28  A8 */\
	PORT_KEY2( 0x0200, 0x0000, IP_NAME_DEFAULT, KEYCODE_TILDE,      CODE_NONE, '`', '~' ) /* `                           29  A9 */\
	PORT_KEY1( 0x0400, 0x0000, IP_NAME_DEFAULT, KEYCODE_LSHIFT,     CODE_NONE, UCHAR_MAMEKEY(LSHIFT) ) /* Left Shift                  2A  AA */\
	PORT_KEY2( 0x0800, 0x0000, IP_NAME_DEFAULT, KEYCODE_BACKSLASH,  CODE_NONE, '\\', '|' ) /* \                           2B  AB */\
	PORT_KEY1( 0x1000, 0x0000, IP_NAME_DEFAULT, KEYCODE_Z,          CODE_NONE, 'Z' ) /* Z                           2C  AC */\
	PORT_KEY1( 0x2000, 0x0000, IP_NAME_DEFAULT, KEYCODE_X,          CODE_NONE, 'X' ) /* X                           2D  AD */\
	PORT_KEY1( 0x4000, 0x0000, IP_NAME_DEFAULT, KEYCODE_C,          CODE_NONE, 'C' ) /* C                           2E  AE */\
	PORT_KEY1( 0x8000, 0x0000, IP_NAME_DEFAULT, KEYCODE_V,          CODE_NONE, 'V' ) /* V                           2F  AF */\
		\
	PORT_START	/* IN7 */\
	PORT_KEY1( 0x0001, 0x0000, IP_NAME_DEFAULT, KEYCODE_B,          CODE_NONE, 'B' ) /* B                           30  B0 */\
	PORT_KEY1( 0x0002, 0x0000, IP_NAME_DEFAULT, KEYCODE_N,          CODE_NONE, 'N' ) /* N                           31  B1 */\
	PORT_KEY1( 0x0004, 0x0000, IP_NAME_DEFAULT, KEYCODE_M,          CODE_NONE, 'M' ) /* M                           32  B2 */\
	PORT_KEY2( 0x0008, 0x0000, IP_NAME_DEFAULT, KEYCODE_COMMA,      CODE_NONE, ',', '<' ) /* ,                           33  B3 */\
	PORT_KEY2( 0x0010, 0x0000, IP_NAME_DEFAULT, KEYCODE_STOP,       CODE_NONE, '.', '>' ) /* .                           34  B4 */\
	PORT_KEY2( 0x0020, 0x0000, IP_NAME_DEFAULT, KEYCODE_SLASH,      CODE_NONE, '/', '?' ) /* /                           35  B5 */\
	PORT_KEY1( 0x0040, 0x0000, IP_NAME_DEFAULT, KEYCODE_RSHIFT,     CODE_NONE, UCHAR_MAMEKEY(RSHIFT) ) /* Right Shift                 36  B6 */\
	PC_KEYB_HELPER( 0x0080, "KP * (PrtScr)",KEYCODE_ASTERISK,   CODE_NONE ) /* Keypad *  (PrtSc)           37  B7 */\
	PC_KEYB_HELPER( 0x0100, "Alt",          KEYCODE_LALT,       CODE_NONE ) /* Left Alt                    38  B8 */\
	PC_KEYB_HELPER( 0x0200, "Space",        KEYCODE_SPACE,      CODE_NONE ) /* Space                       39  B9 */\
	PC_KEYB_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK,   CODE_NONE ) /* Caps Lock                   3A  BA */\
	PC_KEYB_HELPER( 0x0800, "F1",           KEYCODE_F1,         CODE_NONE ) /* F1                          3B  BB */\
	PC_KEYB_HELPER( 0x1000, "F2",           KEYCODE_F2,         CODE_NONE ) /* F2                          3C  BC */\
	PC_KEYB_HELPER( 0x2000, "F3",           KEYCODE_F3,         CODE_NONE ) /* F3                          3D  BD */\
	PC_KEYB_HELPER( 0x4000, "F4",           KEYCODE_F4,         CODE_NONE ) /* F4                          3E  BE */\
	PC_KEYB_HELPER( 0x8000, "F5",           KEYCODE_F5,         CODE_NONE ) /* F5                          3F  BF */\
		\
	PORT_START	/* IN8 */\
	PC_KEYB_HELPER( 0x0001, "F6",           KEYCODE_F6,         CODE_NONE )     /* F6                          40  C0 */\
	PC_KEYB_HELPER( 0x0002, "F7",           KEYCODE_F7,         CODE_NONE )     /* F7                          41  C1 */\
	PC_KEYB_HELPER( 0x0004, "F8",           KEYCODE_F8,         CODE_NONE )     /* F8                          42  C2 */\
	PC_KEYB_HELPER( 0x0008, "F9",           KEYCODE_F9,         CODE_NONE )     /* F9                          43  C3 */\
	PC_KEYB_HELPER( 0x0010, "F10",          KEYCODE_F10,        CODE_NONE )     /* F10                         44  C4 */\
	PC_KEYB_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK,    CODE_NONE )     /* Num Lock                    45  C5 */\
	PC_KEYB_HELPER( 0x0040, "ScrLock",      KEYCODE_SCRLOCK,    CODE_NONE )     /* Scroll Lock                 46  C6 */\
	PC_KEYB_HELPER( 0x0080, "KP 7 (Home)",  KEYCODE_7_PAD,      KEYCODE_HOME )  /* Keypad 7  (Home)            47  C7 */\
	PC_KEYB_HELPER( 0x0100, "KP 8 (Up)",    KEYCODE_8_PAD,      KEYCODE_UP )    /* Keypad 8  (Up arrow)        48  C8 */\
	PC_KEYB_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD,      KEYCODE_PGUP)   /* Keypad 9  (PgUp)            49  C9 */\
	PC_KEYB_HELPER( 0x0400, "KP -",         KEYCODE_MINUS_PAD,  CODE_NONE )     /* Keypad -                    4A  CA */\
	PC_KEYB_HELPER( 0x0800, "KP 4 (Left)",  KEYCODE_4_PAD,      KEYCODE_LEFT )  /* Keypad 4  (Left arrow)      4B  CB */\
	PC_KEYB_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD,      CODE_NONE )     /* Keypad 5                    4C  CC */\
	PC_KEYB_HELPER( 0x2000, "KP 6 (Right)", KEYCODE_6_PAD,      KEYCODE_RIGHT ) /* Keypad 6  (Right arrow)     4D  CD */\
	PC_KEYB_HELPER( 0x4000, "KP +",         KEYCODE_PLUS_PAD,   CODE_NONE )     /* Keypad +                    4E  CE */\
	PC_KEYB_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD,      KEYCODE_END )   /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START	/* IN9 */\
	PC_KEYB_HELPER( 0x0001, "KP 2 (Down)",  KEYCODE_2_PAD,      KEYCODE_DOWN )   /* Keypad 2  (Down arrow)      50  D0 */\
	PC_KEYB_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD,      KEYCODE_PGDN )   /* Keypad 3  (PgDn)            51  D1 */\
	PC_KEYB_HELPER( 0x0004, "KP 0 (Ins)",   KEYCODE_0_PAD,      KEYCODE_INSERT ) /* Keypad 0  (Ins)             52  D2 */\
	PC_KEYB_HELPER( 0x0008, "KP . (Del)",   KEYCODE_DEL_PAD,    KEYCODE_DEL )    /* Keypad .  (Del)             53  D3 */\
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )\
	PC_KEYB_HELPER( 0x0040, "(84/102)\\",   KEYCODE_BACKSLASH2, CODE_NONE )      /* Backslash 2                 56  D6 */\
	PORT_BIT ( 0xff80, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN10 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN11 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

#define AT_KEYB_HELPER(bit, text, key1) \
	PORT_BITX( bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, text, key1, CODE_NONE )

#define AT_KEYBOARD \
	PORT_START	/* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	AT_KEYB_HELPER( 0x0002, "Esc",          KEYCODE_ESC         ) /* Esc                         01  81 */\
	AT_KEYB_HELPER( 0x0004, "1 !",          KEYCODE_1           ) /* 1                           02  82 */\
	AT_KEYB_HELPER( 0x0008, "2 @",          KEYCODE_2           ) /* 2                           03  83 */\
	AT_KEYB_HELPER( 0x0010, "3 #",          KEYCODE_3           ) /* 3                           04  84 */\
	AT_KEYB_HELPER( 0x0020, "4 $",          KEYCODE_4           ) /* 4                           05  85 */\
	AT_KEYB_HELPER( 0x0040, "5 %",          KEYCODE_5           ) /* 5                           06  86 */\
	AT_KEYB_HELPER( 0x0080, "6 ^",          KEYCODE_6           ) /* 6                           07  87 */\
	AT_KEYB_HELPER( 0x0100, "7 &",          KEYCODE_7           ) /* 7                           08  88 */\
	AT_KEYB_HELPER( 0x0200, "8 *",          KEYCODE_8           ) /* 8                           09  89 */\
	AT_KEYB_HELPER( 0x0400, "9 (",          KEYCODE_9           ) /* 9                           0A  8A */\
	AT_KEYB_HELPER( 0x0800, "0 )",          KEYCODE_0           ) /* 0                           0B  8B */\
	AT_KEYB_HELPER( 0x1000, "- _",          KEYCODE_MINUS       ) /* -                           0C  8C */\
	AT_KEYB_HELPER( 0x2000, "= +",          KEYCODE_EQUALS      ) /* =                           0D  8D */\
	AT_KEYB_HELPER( 0x4000, "<--",          KEYCODE_BACKSPACE   ) /* Backspace                   0E  8E */\
	AT_KEYB_HELPER( 0x8000, "Tab",          KEYCODE_TAB         ) /* Tab                         0F  8F */\
		\
	PORT_START	/* IN5 */\
	AT_KEYB_HELPER( 0x0001, "Q",            KEYCODE_Q           ) /* Q                           10  90 */\
	AT_KEYB_HELPER( 0x0002, "W",            KEYCODE_W           ) /* W                           11  91 */\
	AT_KEYB_HELPER( 0x0004, "E",            KEYCODE_E           ) /* E                           12  92 */\
	AT_KEYB_HELPER( 0x0008, "R",            KEYCODE_R           ) /* R                           13  93 */\
	AT_KEYB_HELPER( 0x0010, "T",            KEYCODE_T           ) /* T                           14  94 */\
	AT_KEYB_HELPER( 0x0020, "Y",            KEYCODE_Y           ) /* Y                           15  95 */\
	AT_KEYB_HELPER( 0x0040, "U",            KEYCODE_U           ) /* U                           16  96 */\
	AT_KEYB_HELPER( 0x0080, "I",            KEYCODE_I           ) /* I                           17  97 */\
	AT_KEYB_HELPER( 0x0100, "O",            KEYCODE_O           ) /* O                           18  98 */\
	AT_KEYB_HELPER( 0x0200, "P",            KEYCODE_P           ) /* P                           19  99 */\
	AT_KEYB_HELPER( 0x0400, "[ {",          KEYCODE_OPENBRACE   ) /* [                           1A  9A */\
	AT_KEYB_HELPER( 0x0800, "] }",          KEYCODE_CLOSEBRACE  ) /* ]                           1B  9B */\
	AT_KEYB_HELPER( 0x1000, "Enter",        KEYCODE_ENTER       ) /* Enter                       1C  9C */\
	AT_KEYB_HELPER( 0x2000, "L-Ctrl",       KEYCODE_LCONTROL    ) /* Left Ctrl                   1D  9D */\
	AT_KEYB_HELPER( 0x4000, "A",            KEYCODE_A           ) /* A                           1E  9E */\
	AT_KEYB_HELPER( 0x8000, "S",            KEYCODE_S           ) /* S                           1F  9F */\
		\
	PORT_START	/* IN6 */\
	AT_KEYB_HELPER( 0x0001, "D",            KEYCODE_D           ) /* D                           20  A0 */\
	AT_KEYB_HELPER( 0x0002, "F",            KEYCODE_F           ) /* F                           21  A1 */\
	AT_KEYB_HELPER( 0x0004, "G",            KEYCODE_G           ) /* G                           22  A2 */\
	AT_KEYB_HELPER( 0x0008, "H",            KEYCODE_H           ) /* H                           23  A3 */\
	AT_KEYB_HELPER( 0x0010, "J",            KEYCODE_J           ) /* J                           24  A4 */\
	AT_KEYB_HELPER( 0x0020, "K",            KEYCODE_K           ) /* K                           25  A5 */\
	AT_KEYB_HELPER( 0x0040, "L",            KEYCODE_L           ) /* L                           26  A6 */\
	AT_KEYB_HELPER( 0x0080, "; :",          KEYCODE_COLON       ) /* ;                           27  A7 */\
	AT_KEYB_HELPER( 0x0100, "' \"",         KEYCODE_QUOTE       ) /* '                           28  A8 */\
	AT_KEYB_HELPER( 0x0200, "` ~",          KEYCODE_TILDE       ) /* `                           29  A9 */\
	AT_KEYB_HELPER( 0x0400, "L-Shift",      KEYCODE_LSHIFT      ) /* Left Shift                  2A  AA */\
	AT_KEYB_HELPER( 0x0800, "\\ |",         KEYCODE_BACKSLASH   ) /* \                           2B  AB */\
	AT_KEYB_HELPER( 0x1000, "Z",            KEYCODE_Z           ) /* Z                           2C  AC */\
	AT_KEYB_HELPER( 0x2000, "X",            KEYCODE_X           ) /* X                           2D  AD */\
	AT_KEYB_HELPER( 0x4000, "C",            KEYCODE_C           ) /* C                           2E  AE */\
	AT_KEYB_HELPER( 0x8000, "V",            KEYCODE_V           ) /* V                           2F  AF */\
		\
	PORT_START	/* IN7 */\
	AT_KEYB_HELPER( 0x0001, "B",            KEYCODE_B           ) /* B                           30  B0 */\
	AT_KEYB_HELPER( 0x0002, "N",            KEYCODE_N           ) /* N                           31  B1 */\
	AT_KEYB_HELPER( 0x0004, "M",            KEYCODE_M           ) /* M                           32  B2 */\
	AT_KEYB_HELPER( 0x0008, ", <",          KEYCODE_COMMA       ) /* ,                           33  B3 */\
	AT_KEYB_HELPER( 0x0010, ". >",          KEYCODE_STOP        ) /* .                           34  B4 */\
	AT_KEYB_HELPER( 0x0020, "/ ?",          KEYCODE_SLASH       ) /* /                           35  B5 */\
	AT_KEYB_HELPER( 0x0040, "R-Shift",      KEYCODE_RSHIFT      ) /* Right Shift                 36  B6 */\
	AT_KEYB_HELPER( 0x0080, "KP * (PrtScr)",KEYCODE_ASTERISK    ) /* Keypad *  (PrtSc)           37  B7 */\
	AT_KEYB_HELPER( 0x0100, "Alt",          KEYCODE_LALT        ) /* Left Alt                    38  B8 */\
	AT_KEYB_HELPER( 0x0200, "Space",        KEYCODE_SPACE       ) /* Space                       39  B9 */\
	AT_KEYB_HELPER( 0x0400, "Caps",         KEYCODE_CAPSLOCK    ) /* Caps Lock                   3A  BA */\
	AT_KEYB_HELPER( 0x0800, "F1",           KEYCODE_F1          ) /* F1                          3B  BB */\
	AT_KEYB_HELPER( 0x1000, "F2",           KEYCODE_F2          ) /* F2                          3C  BC */\
	AT_KEYB_HELPER( 0x2000, "F3",           KEYCODE_F3          ) /* F3                          3D  BD */\
	AT_KEYB_HELPER( 0x4000, "F4",           KEYCODE_F4          ) /* F4                          3E  BE */\
	AT_KEYB_HELPER( 0x8000, "F5",           KEYCODE_F5          ) /* F5                          3F  BF */\
		\
	PORT_START	/* IN8 */\
	AT_KEYB_HELPER( 0x0001, "F6",           KEYCODE_F6          ) /* F6                          40  C0 */\
	AT_KEYB_HELPER( 0x0002, "F7",           KEYCODE_F7          ) /* F7                          41  C1 */\
	AT_KEYB_HELPER( 0x0004, "F8",           KEYCODE_F8          ) /* F8                          42  C2 */\
	AT_KEYB_HELPER( 0x0008, "F9",           KEYCODE_F9          ) /* F9                          43  C3 */\
	AT_KEYB_HELPER( 0x0010, "F10",          KEYCODE_F10         ) /* F10                         44  C4 */\
	AT_KEYB_HELPER( 0x0020, "NumLock",      KEYCODE_NUMLOCK     ) /* Num Lock                    45  C5 */\
	AT_KEYB_HELPER( 0x0040, "ScrLock",      KEYCODE_SCRLOCK     ) /* Scroll Lock                 46  C6 */\
	AT_KEYB_HELPER( 0x0080, "KP 7 (Home)",  KEYCODE_7_PAD       ) /* Keypad 7  (Home)            47  C7 */\
	AT_KEYB_HELPER( 0x0100, "KP 8 (Up)",    KEYCODE_8_PAD       ) /* Keypad 8  (Up arrow)        48  C8 */\
	AT_KEYB_HELPER( 0x0200, "KP 9 (PgUp)",  KEYCODE_9_PAD       ) /* Keypad 9  (PgUp)            49  C9 */\
	AT_KEYB_HELPER( 0x0400, "KP -",         KEYCODE_MINUS_PAD   ) /* Keypad -                    4A  CA */\
	AT_KEYB_HELPER( 0x0800, "KP 4 (Left)",  KEYCODE_4_PAD       ) /* Keypad 4  (Left arrow)      4B  CB */\
	AT_KEYB_HELPER( 0x1000, "KP 5",         KEYCODE_5_PAD       ) /* Keypad 5                    4C  CC */\
	AT_KEYB_HELPER( 0x2000, "KP 6 (Right)", KEYCODE_6_PAD       ) /* Keypad 6  (Right arrow)     4D  CD */\
	AT_KEYB_HELPER( 0x4000, "KP +",         KEYCODE_PLUS_PAD    ) /* Keypad +                    4E  CE */\
	AT_KEYB_HELPER( 0x8000, "KP 1 (End)",   KEYCODE_1_PAD       ) /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START	/* IN9 */\
	AT_KEYB_HELPER( 0x0001, "KP 2 (Down)",  KEYCODE_2_PAD       ) /* Keypad 2  (Down arrow)      50  D0 */\
	AT_KEYB_HELPER( 0x0002, "KP 3 (PgDn)",  KEYCODE_3_PAD       ) /* Keypad 3  (PgDn)            51  D1 */\
	AT_KEYB_HELPER( 0x0004, "KP 0 (Ins)",   KEYCODE_0_PAD       ) /* Keypad 0  (Ins)             52  D2 */\
	AT_KEYB_HELPER( 0x0008, "KP . (Del)",   KEYCODE_DEL_PAD     ) /* Keypad .  (Del)             53  D3 */\
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )\
	AT_KEYB_HELPER( 0x0040, "(84/102)\\",   KEYCODE_BACKSLASH2  ) /* Backslash 2                 56  D6 */\
	AT_KEYB_HELPER( 0x0080, "(MF2)F11",		KEYCODE_F11         ) /* F11                         57  D7 */\
	AT_KEYB_HELPER( 0x0100, "(MF2)F12",		KEYCODE_F12         ) /* F12                         58  D8 */\
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN10 */\
	AT_KEYB_HELPER( 0x0001, "(MF2)KP Enter",		KEYCODE_ENTER_PAD   ) /* PAD Enter                   60  e0 */\
	AT_KEYB_HELPER( 0x0002, "(MF2)Right Control",	KEYCODE_RCONTROL    ) /* Right Control               61  e1 */\
	AT_KEYB_HELPER( 0x0004, "(MF2)KP /",			KEYCODE_SLASH_PAD   ) /* PAD Slash                   62  e2 */\
	AT_KEYB_HELPER( 0x0008, "(MF2)PRTSCR",			KEYCODE_PRTSCR      ) /* Print Screen                63  e3 */\
	AT_KEYB_HELPER( 0x0010, "(MF2)ALTGR",			KEYCODE_RALT        ) /* ALTGR                       64  e4 */\
	AT_KEYB_HELPER( 0x0020, "(MF2)Home",			KEYCODE_HOME        ) /* Home                        66  e6 */\
	AT_KEYB_HELPER( 0x0040, "(MF2)Cursor Up",		KEYCODE_UP          ) /* Up                          67  e7 */\
	AT_KEYB_HELPER( 0x0080, "(MF2)Page Up",			KEYCODE_PGUP        ) /* Page Up                     68  e8 */\
	AT_KEYB_HELPER( 0x0100, "(MF2)Cursor Left",		KEYCODE_LEFT        ) /* Left                        69  e9 */\
	AT_KEYB_HELPER( 0x0200, "(MF2)Cursor Right",	KEYCODE_RIGHT       ) /* Right                       6a  ea */\
	AT_KEYB_HELPER( 0x0400, "(MF2)End",				KEYCODE_END         ) /* End                         6b  eb */\
	AT_KEYB_HELPER( 0x0800, "(MF2)Cursor Down",		KEYCODE_DOWN        ) /* Down                        6c  ec */\
	AT_KEYB_HELPER( 0x1000, "(MF2)Page Down",		KEYCODE_PGDN        ) /* Page Down                   6d  ed */\
	AT_KEYB_HELPER( 0x2000, "(MF2)Insert",			KEYCODE_INSERT      ) /* Insert                      6e  ee */\
	AT_KEYB_HELPER( 0x4000, "(MF2)Delete",			KEYCODE_DEL         ) /* Delete                      6f  ef */\
	AT_KEYB_HELPER( 0x8000, "(MF2)Pause",			KEYCODE_PAUSE       ) /* Pause                       65  e5 */\
	PORT_START	/* IN11 */\
	AT_KEYB_HELPER( 0x0001, "Print Screen", KEYCODE_PRTSCR           ) /* Print Screen alternate      77  f7 */\
	PORT_BIT ( 0xfffe, 0x0000, IPT_UNUSED )

#if 0
	AT_KEYB_HELPER( 0x2000, "Left Win",     CODE_NONE           ) /* Left Win                    7d  fd */
	AT_KEYB_HELPER( 0x4000, "Right Win",    CODE_NONE           ) /* Right Win                   7e  fe */
	AT_KEYB_HELPER( 0x8000, "Menu",         CODE_NONE           ) /* Menu                        7f  ff */
#endif

#ifdef __cplusplus
}
#endif

