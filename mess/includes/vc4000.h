#include "driver.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void vc4000_runtime_loader_init(void);
# else
	extern void vc4000_runtime_loader_init(void);
# endif
#endif

int vc4000_video_line(void);
READ_HANDLER(vc4000_vsync_r);

READ_HANDLER(vc4000_video_r);
WRITE_HANDLER(vc4000_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 8
#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 48
extern int vc4000_vh_start(void);
extern void vc4000_vh_stop(void);
extern void vc4000_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

extern struct CustomSound_interface vc4000_sound_interface;
extern int vc4000_custom_start (const struct MachineSound *driver);
extern void vc4000_custom_stop (void);
extern void vc4000_custom_update (void);
extern void vc4000_soundport_w (int mode, int data);
