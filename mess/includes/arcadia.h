#include "driver.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void arcadia_runtime_loader_init(void);
# else
	extern void arcadia_runtime_loader_init(void);
# endif
#endif

// use this for debugging
#if 0
#define arcadia_DEBUG 1
#else
#define arcadia_DEBUG 0
#endif

int arcadia_video_line(void);
READ_HANDLER(arcadia_vsync_r);

READ_HANDLER(arcadia_video_r);
WRITE_HANDLER(arcadia_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 0
//#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#if arcadia_DEBUG
#define XPOS 48
#else
#define XPOS 32
#endif
extern int arcadia_vh_start(void);
extern void arcadia_vh_stop(void);
extern void arcadia_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);

extern struct CustomSound_interface arcadia_sound_interface;
extern int arcadia_custom_start (const struct MachineSound *driver);
extern void arcadia_custom_stop (void);
extern void arcadia_custom_update (void);
extern void arcadia_soundport_w (int mode, int data);
