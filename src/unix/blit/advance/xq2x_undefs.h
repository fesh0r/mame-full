#include "interp_undefs.h"

#undef MUR
#undef MDR
#undef MDL
#undef MUL

#undef XQ2X_LINE_LOOP_BEGIN 
#undef XQ2X_LINE_LOOP_BEGIN_SWAP_XY
#undef XQ2X_LINE_LOOP_END

#undef XQ2X_NAME
#undef XQ2X_FUNC_NAME

/* this saves us from having to undef these each time in the files using
   the blit macros */
#undef HQ2X_YUVLOOKUP
