#include "blit.h"

#define FUNC_NAME(name) name##_15_15_direct
#define SRC_DEPTH  15
#define DEST_DEPTH 15
#include "blit_defs.h"
#include "blit_normal.h"
#include "blit_effect.h"
#include "advance/scale2x.h"
#include "blit_undefs.h"
