#include "pixel_defs.h"
#include "blit.h"

#define FUNC_NAME(name) name##_32_YUY2_direct
#define SRC_DEPTH    32
#define DEST_DEPTH   16
#define RENDER_DEPTH 32
#define GET_YUV_PIXEL(p) lookup[_32TO16_RGB_565(p)]
#include "blit_defs.h"
#include "blit_yuy2.h"
#include "blit_undefs.h"
