#include "blit.h"

#define FUNC_NAME(name) name##_32_32_direct
#define SRC_DEPTH  32
#define DEST_DEPTH 32
#include "blit_defs.h"
#include "blit_normal.h"
#include "blit_effect.h"
#include "advance/scale2x.h"
#include "advance/hq2x.h"
#include "blit_undefs.h"
