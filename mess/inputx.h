/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef INPUTX_H
#define INPUTX_H

#include "mame.h"

/***************************************************************************

	Constants

***************************************************************************/

/* input classes */
enum
{
	INPUT_CLASS_INTERNAL,
	INPUT_CLASS_KEYBOARD,
	INPUT_CLASS_CONTROLLER,
	INPUT_CLASS_CONFIG,
	INPUT_CLASS_DIPSWITCH,
	INPUT_CLASS_CATEGORIZED,
	INPUT_CLASS_MISC
};

/***************************************************************************

	Macros

***************************************************************************/

#define UCHAR_SHIFT_1		(0xf700)
#define UCHAR_SHIFT_2		(0xf701)
#define UCHAR_MAMEKEY_BEGIN	(0xf702)
#define UCHAR_MAMEKEY_END	(UCHAR_MAMEKEY_BEGIN + __code_key_last)
#define UCHAR_MAMEKEY(code)	(UCHAR_MAMEKEY_BEGIN + KEYCODE_##code)

#define UCHAR_SHIFT_BEGIN	(UCHAR_SHIFT_1)
#define UCHAR_SHIFT_END		(UCHAR_SHIFT_2)

#define PORT_CHAR(ch)												\
	port->keyboard.chars[key++] = (ch);								\

/* config definition */
#define PORT_CONFNAME(mask,default,name) \
	PORT_BIT(mask, default, IPT_CONFIG_NAME)						\
	PORT_NAME(name)													\

#define PORT_CONFSETTING(default,name) \
	PORT_BIT(0, default, IPT_CONFIG_SETTING)						\
	PORT_NAME(name)													\
	
/* categories */
#define PORT_CATEGORY(category_) \
	port->category = (category_);	\

#define PORT_CATEGORY_CLASS(mask,default,name) 						\
	PORT_BIT(mask, default, IPT_CATEGORY_NAME)						\
	PORT_NAME(name)

#define PORT_CATEGORY_ITEM(default,name,category) 					\
	PORT_BIT(0, default, IPT_CATEGORY_SETTING) 						\
	PORT_NAME(name)													\
	PORT_CATEGORY(category)



/***************************************************************************

	Prototypes

***************************************************************************/

/* these are called by the core; they should not be called from FEs */
void inputx_init(void);
void inputx_update(UINT32 *ports);
void inputx_handle_mess_extensions(struct InputPort *ipt);

int inputx_validitycheck(const struct GameDriver *gamedrv);

/* these can be called from FEs */
int inputx_can_post(void);
int inputx_can_post_key(unicode_char_t ch);
int inputx_is_posting(void);
const char *inputx_key_name(unicode_char_t ch);

/* various posting functions; can be called from FEs */
void inputx_post(const unicode_char_t *text);
void inputx_post_rate(const unicode_char_t *text, mame_time rate);
void inputx_postc(unicode_char_t ch);
void inputx_postc_rate(unicode_char_t ch, mame_time rate);
void inputx_postn(const unicode_char_t *text, size_t text_len);
void inputx_postn_rate(const unicode_char_t *text, size_t text_len, mame_time rate);
void inputx_post_utf16(const utf16_char_t *text);
void inputx_post_utf16_rate(const utf16_char_t *text, mame_time rate);
void inputx_postn_utf16(const utf16_char_t *text, size_t text_len);
void inputx_postn_utf16_rate(const utf16_char_t *text, size_t text_len, mame_time rate);
void inputx_post_utf8(const char *text);
void inputx_post_utf8_rate(const char *text, mame_time rate);
void inputx_postn_utf8(const char *text, size_t text_len);
void inputx_postn_utf8_rate(const char *text, size_t text_len, mame_time rate);

/* miscellaneous functions */
int input_classify_port(const struct InputPort *in);
int input_has_input_class(int inputclass);
int input_player_number(const struct InputPort *in);
int input_count_players(void);
int input_category_active(int category);

#endif /* INPUTX_H */
