/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef INPUTX_H
#define INPUTX_H

#include "mame.h"

typedef UINT16 utf16_char_t;
typedef UINT32 unicode_char_t;

/* these are called by the core; they should not be called from FEs */
void inputx_init(void);
void inputx_update(unsigned short *ports);

#ifdef MAME_DEBUG
int inputx_validitycheck(const struct GameDriver *gamedrv);
#endif

/* these can be called from FEs */
int inputx_can_post(void);
int inputx_can_post_key(unicode_char_t ch);

void inputx_post(const unicode_char_t *text);
void inputx_postc(unicode_char_t ch);
void inputx_postn(const unicode_char_t *text, size_t text_len);
void inputx_post_utf16(const utf16_char_t *text);
void inputx_postn_utf16(const utf16_char_t *text, size_t text_len);

#define UCHAR_SHIFT_1		(0xf700)
#define UCHAR_SHIFT_2		(0xf701)
#define UCHAR_MAMEKEY_BEGIN	(0xf702)
#define UCHAR_MAMEKEY_END	(UCHAR_MAMEKEY_BEGIN + __code_key_last)
#define UCHAR_MAMEKEY(code)	(UCHAR_MAMEKEY_BEGIN + KEYCODE_##code)

#define UCHAR_SHIFT_BEGIN	(UCHAR_SHIFT_1)
#define UCHAR_SHIFT_END		(UCHAR_SHIFT_2)

#define PORT_UCHAR(uchar) \
	{ (UINT16) ((uchar) >> 16), (UINT16) ((uchar) >> 0), IPT_UCHAR, 0 },

#define PORT_KEY0(mask,default,name,key1,key2)						\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\

#define PORT_KEY1(mask,default,name,key1,key2,uchar)				\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\
	PORT_UCHAR(uchar)												\

#define PORT_KEY2(mask,default,name,key1,key2,uchar1,uchar2)		\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\
	PORT_UCHAR(uchar1)												\
	PORT_UCHAR(uchar2)												\

#endif /* INPUTX_H */
