/*********************************************************************

	pc_video.h

	Refactoring of code common to PC video implementations

*********************************************************************/

#ifndef PC_VIDEO_H
#define PC_VIDEO_H

#include "includes/crtc6845.h"

typedef void (*pc_video_update_proc)(struct mame_bitmap *bitmap);

int pc_video_start(struct _CRTC6845 *crtc, CRTC6845_CONFIG *config,
	pc_video_update_proc (*choosevideomode)(int *xfactor, int *yfactor));

VIDEO_UPDATE( pc_video );

#endif /* PC_VIDEO_H */
