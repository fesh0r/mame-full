#include "driver.h"

// define this for debugging sessions
//#define DEBUG

// define this to use digital inputs instead of the slow
// autocentering analog mame joys
#define ANALOG_HACK

extern INTERRUPT_GEN( vc4000_video_line );
extern  READ8_HANDLER(vc4000_vsync_r);

extern  READ8_HANDLER(vc4000_video_r);
extern WRITE8_HANDLER(vc4000_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 8
#define YBOTTOM_SIZE 40
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 48

extern VIDEO_START( vc4000 );
extern VIDEO_UPDATE( vc4000 );

extern struct CustomSound_interface vc4000_sound_interface;
extern int vc4000_custom_start (const struct MachineSound *driver);
extern void vc4000_custom_stop (void);
extern void vc4000_custom_update (void);
extern void vc4000_soundport_w (int mode, int data);
