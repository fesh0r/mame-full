//============================================================
//
//	video.h - Win32 implementation of MAME video routines
//
//============================================================

#ifndef __WIN_VIDEO__
#define __WIN_VIDEO__


//============================================================
//	PARAMETERS
//============================================================

// maximum video size
#define MAX_VIDEO_WIDTH			1600
#define MAX_VIDEO_HEIGHT		1200



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
extern int			frameskip;
extern int			autoframeskip;

// speed throttling
extern int			throttle;

// palette lookups
extern UINT8		palette_lookups_invalid;
extern UINT32 		palette_16bit_lookup[];
extern UINT32 		palette_32bit_lookup[];


#endif
