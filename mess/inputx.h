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

#endif /* INPUTX_H */
