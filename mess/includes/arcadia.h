#include "driver.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void arcadia_runtime_loader_init(void);
# else
	extern void arcadia_runtime_loader_init(void);
# endif
#endif

int arcadia_video_line(void);
READ_HANDLER(arcadia_vsync_r);

READ_HANDLER(arcadia_video_r);
WRITE_HANDLER(arcadia_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 8
#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 48
extern int arcadia_vh_start(void);
extern void arcadia_vh_stop(void);
extern void arcadia_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

extern struct CustomSound_interface arcadia_sound_interface;
extern int arcadia_custom_start (const struct MachineSound *driver);
extern void arcadia_custom_stop (void);
extern void arcadia_custom_update (void);
extern void arcadia_soundport_w (int mode, int data);
