/*********************************************************************

	testimgt.h

	Imgtool testing code

*********************************************************************/

#ifndef TESTIMGT_H
#define TESTIMGT_H

#include "core.h"

extern const struct messtest_tagdispatch testimgtool_dispatch[];

void testimgtool_start_handler(struct messtest_state *state, const char **attributes);
void testimgtool_end_handler(struct messtest_state *state, const void *buffer, size_t size);

#endif /* TESTIMGT_H */
