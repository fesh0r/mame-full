/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef INPUTX_H
#define INPUTX_H

#include <wchar.h>
#include "mame.h"

/* these are called by the core; they should not be called from FEs */
void inputx_init(void);
void inputx_update(unsigned short *ports);

#ifdef MAME_DEBUG
void inputx_validitycheck(struct GameDriver *gamedrv);
#endif

/* these can be called from FEs */
int inputx_can_post(void);
int inputx_can_post_key(wchar_t ch);
void inputx_wpost(const wchar_t *text);
void inputx_post(const char *text);

#endif /* INPUTX_H */
