/*********************************************************************

	testmess.h

	Code for testing MESS

*********************************************************************/

#ifndef TESTMESS_H
#define TESTMESS_H

#include "core.h"

extern const struct messtest_tagdispatch testmess_dispatch[];

void testmess_start_handler(const char **attributes);
void testmess_end_handler(const void *buffer, size_t size);

#endif /* TESTMESS_H */
