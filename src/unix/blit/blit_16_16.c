#include "blit.h"

#define FUNC_NAME(name) name##_16_16
#define SRC_DEPTH  16
#define DEST_DEPTH 16
#include "blit_defs.h"
#include "blit_normal.h"
#include "blit_effect.h"
#include "advance/scale2x.h"
#include "blit_undefs.h"
