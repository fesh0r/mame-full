//============================================================
//
//	video.c - Win32 video handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// undef WINNT for ddraw.h to prevent duplicate definition
#undef WINNT
#define DIRECTDRAW_VERSION 0x0300
#include <ddraw.h>

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "mamedbg.h"
#include "vidhrdw/vector.h"
#include "ticker.h"
#include "blit.h"
#include "video.h"
#include "window.h"
#include "rc.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win_poll_input(void);
extern void win_pause_input(int pause);
extern UINT8 win_trying_to_quit;


//============================================================
//	PARAMETERS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS			12



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
int frameskip;
int autoframeskip;

// speed throttling
int throttle = 1;

// palette lookups
UINT8 palette_lookups_invalid;
UINT32 palette_16bit_lookup[65536];
UINT32 palette_32bit_lookup[65536];



//============================================================
//	LOCAL VARIABLES
//============================================================

// core video input parameters
static int video_depth;
static double video_fps;
static int video_attributes;
static int video_orientation;

// derived from video attributes
static int vector_game;
static int rgb_direct;

// current visible area bounds
static int vis_min_x;
static int vis_max_x;
static int vis_min_y;
static int vis_max_y;
static int vis_width;
static int vis_height;

// internal readiness states
static int warming_up;

// timing measurements for throttling
static TICKER last_skipcount0_time;
static TICKER this_frame_base;
static int allow_sleep;

// average FPS calculation
static TICKER start_time;
static TICKER end_time;
static int frames_displayed;
static int frames_to_display;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// game states that invalidate autoframeskip
static int game_was_paused;
static int game_is_paused;
static int debugger_was_visible;

// frameskipping tables
static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
};

static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 1,1,1,1,1,1,1,1,1,1,1,1 },
	{ 2,1,1,1,1,1,1,1,1,1,1,0 },
	{ 2,1,1,1,1,0,2,1,1,1,1,0 },
	{ 2,1,1,0,2,1,1,0,2,1,1,0 },
	{ 2,1,0,2,1,0,2,1,0,2,1,0 },
	{ 2,0,2,1,0,2,0,2,1,0,2,0 },
	{ 2,0,2,0,2,0,2,0,2,0,2,0 },
	{ 2,0,2,0,0,3,0,2,0,0,3,0 },
	{ 3,0,0,3,0,0,3,0,0,3,0,0 },
	{ 4,0,0,0,4,0,0,0,4,0,0,0 },
	{ 6,0,0,0,0,0,6,0,0,0,0,0 },
	{12,0,0,0,0,0,0,0,0,0,0,0 }
};



//============================================================
//	OPTIONS
//============================================================

// prototypes
static int video_set_resolution(struct rc_option *option, const char *arg, int priority);
static int decode_effect(struct rc_option *option, const char *arg, int priority);
static int decode_aspect(struct rc_option *option, const char *arg, int priority);
static void update_visible_area(struct mame_display *display);

// internal variables
static char *resolution;
static char *effect;
static char *aspect;

// options struct
struct rc_option video_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "autoframeskip", "afs", rc_bool, &autoframeskip, "1", 0, 0, NULL, "skip frames to speed up emulation" },
	{ "frameskip", "fs", rc_int, &frameskip, "0", 0, 12, NULL, "set frameskip explicitly (autoframeskip needs to be off)" },
	{ "waitvsync", NULL, rc_bool, &win_wait_vsync, "0", 0, 0, NULL, "wait for vertical sync (reduces tearing)"},
	{ "triplebuffer", "tb", rc_bool, &win_triple_buffer, "0", 0, 0, NULL, "triple buffering (only if fullscreen)" },
	{ "window", "w", rc_bool, &win_window_mode, "0", 0, 0, NULL, "run in a window/run on full screen" },
	{ "ddraw", "dd", rc_bool, &win_use_ddraw, "1", 0, 0, NULL, "use DirectDraw for rendering" },
	{ "hwstretch", "hws", rc_bool, &win_hw_stretch, "1", 0, 0, NULL, "stretch video using the hardware" },
	{ "resolution", "r", rc_string, &resolution, "auto", 0, 0, video_set_resolution, "set resolution" },
	{ "refresh", NULL, rc_int, &win_gfx_refresh, "0", 0, 0, NULL, "set specific monitor refresh rate" },
	{ "scanlines", "sl", rc_bool, &win_old_scanlines, "0", 0, 0, NULL, "emulate win_old_scanlines" },
	{ "switchres", NULL, rc_bool, &win_switch_res, "1", 0, 0, NULL, "switch resolutions to best fit" },
	{ "switchbpp", NULL, rc_bool, &win_switch_bpp, "1", 0, 0, NULL, "switch color depths to best fit" },
	{ "maximize", "max", rc_bool, &win_start_maximized, "1", 0, 0, NULL, "start out maximized" },
	{ "keepaspect", "ka", rc_bool, &win_keep_aspect, "1", 0, 0, NULL, "enforce aspect ratio" },
	{ "matchrefresh", NULL, rc_bool, &win_match_refresh, "0", 0, 0, NULL, "attempt to match the game's refresh rate" },
	{ "syncrefresh", NULL, rc_bool, &win_sync_refresh, "0", 0, 0, NULL, "syncronize only to the monitor refresh" },
	{ "throttle", NULL, rc_bool, &throttle, "1", 0, 0, NULL, "throttle speed to the game's framerate" },
	{ "full_screen_brightness", "fsb", rc_float, &win_gfx_brightness, "0.0", 0.0, 4.0, NULL, "sets the brightness in full screen mode" },
	{ "frames_to_run", "ftr", rc_int, &frames_to_display, "0", 0, 0, NULL, "sets the number of frames to run within the game" },
	{ "effect", NULL, rc_string, &effect, "none", 0, 0, decode_effect, "specify the blitting effect" },
	{ "screen_aspect", NULL, rc_string, &aspect, "4:3", 0, 0, decode_aspect, "specify an alternate monitor aspect ratio" },
	{ "sleep", NULL, rc_bool, &allow_sleep, "1", 0, 0, NULL, "allow MAME to give back time to the system when it's not needed" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//	video_set_resolution
//============================================================

static int video_set_resolution(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
	}
	else if (sscanf(arg, "%dx%dx%d", &win_gfx_width, &win_gfx_height, &win_gfx_depth) < 2)
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	if ((win_gfx_depth != 0) &&
		(win_gfx_depth != 16) &&
		(win_gfx_depth != 24) &&
		(win_gfx_depth != 32))
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	options.vector_width = win_gfx_width;
	options.vector_height = win_gfx_height;

	option->priority = priority;
	return 0;
}



//============================================================
//	decode_effect
//============================================================

static int decode_effect(struct rc_option *option, const char *arg, int priority)
{
	win_blit_effect = win_lookup_effect(arg);
	if (win_blit_effect == -1)
	{
		fprintf(stderr, "error: invalid value for effect: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}



//============================================================
//	decode_aspect
//============================================================

static int decode_aspect(struct rc_option *option, const char *arg, int priority)
{
	int num, den;

	if (sscanf(arg, "%d:%d", &num, &den) != 2 || num == 0 || den == 0)
	{
		fprintf(stderr, "error: invalid value for aspect ratio: %s\n", arg);
		return -1;
	}
	win_screen_aspect = (double)num / (double)den;

	option->priority = priority;
	return 0;
}



//============================================================
//	osd_create_display
//============================================================

int osd_create_display(int width, int height, int depth, float fps, int attributes, int orientation, UINT32 *rgb_components)
{
	struct mame_display dummy_display;
	int r, g, b;
	
	logerror("width %d, height %d depth %d\n",width,height,depth);

	// copy the parameters into globals for later use
	video_depth			= (depth != 15) ? depth : 16;
	video_fps			= fps;
	video_attributes	= attributes;
	video_orientation	= orientation;

	// clamp the frameskip value to within range
	if (frameskip < 0)
		frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS)
		frameskip = FRAMESKIP_LEVELS - 1;

	// extract useful parameters from the attributes
	vector_game			= ((attributes & VIDEO_TYPE_VECTOR) != 0);
	rgb_direct			= (depth == 15) || (depth == 32);

	// create the window
	if (win_create_window(width, height, video_depth, attributes, orientation))
		return 1;
	
	// initialize the palette to a fixed 5-5-5 mapping
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				int idx = (r << 10) | (g << 5) | b;
				int rr = (r << 3) | (r >> 2);
				int gg = (g << 3) | (g >> 2);
				int bb = (b << 3) | (b >> 2);
				palette_16bit_lookup[idx] = win_color16(rr, gg, bb) * 0x10001;
				palette_32bit_lookup[idx] = win_color32(rr, gg, bb);
			}

	// fill in the resulting RGB components
	if (rgb_components)
	{
		if (video_depth == 32)
		{
			rgb_components[0] = win_color32(0xff, 0x00, 0x00);
			rgb_components[1] = win_color32(0x00, 0xff, 0x00);
			rgb_components[2] = win_color32(0x00, 0x00, 0xff);
		}
		else
		{
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;
		}
	}

	// set visible area to nothing just to initialize it - it will be set by the core
	memset(&dummy_display, 0, sizeof(dummy_display));
	update_visible_area(&dummy_display);

	// indicate for later that we're just beginning
	warming_up = 1;
    return 0;
}



//============================================================
//	osd_close_display
//============================================================

void osd_close_display(void)
{
	// tear down the window
	win_destroy_window();

	// print a final result to the stdout
	if (frames_displayed != 0)
		printf("Average FPS: %f (%d frames)\n", (double)TICKS_PER_SEC / (end_time - start_time) * frames_displayed, frames_displayed);
}



//============================================================
//	osd_skip_this_frame
//============================================================

int osd_skip_this_frame(void)
{
	// skip the current frame?
	return skiptable[frameskip][frameskip_counter];
}



//============================================================
//	osd_get_frameskip
//============================================================

int osd_get_frameskip(void)
{
	return autoframeskip ? -(frameskip + 1) : frameskip;
}



//============================================================
//	check_inputs
//============================================================

static void check_inputs(void)
{
	// increment frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		// if autoframeskip, disable auto and go to 0
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = 0;
		}

		// wrap from maximum to auto
		else if (frameskip == FRAMESKIP_LEVELS - 1)
		{
			frameskip = 0;
			autoframeskip = 1;
		}

		// else just increment
		else
			frameskip++;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS-1;
		}

		// wrap from 0 to auto
		else if (frameskip == 0)
			autoframeskip = 1;

		// else just decrement
		else
			frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		throttle ^= 1;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		win_toggle_full_screen();
}



//============================================================
//	throttle_speed
//============================================================

static void throttle_speed(void)
{
	static double ticks_per_sleep_msec = 0;
	TICKER target, curr;

	// if we're only syncing to the refresh, bail now
	if (win_sync_refresh)
		return;

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// get the current time and the target time
	curr = ticker();
	target = this_frame_base + (int)((double)frameskip_counter * (double)TICKS_PER_SEC / video_fps);

	// sync
	if (curr - target < 0)
	{
		// initialize the ticks per sleep
		if (ticks_per_sleep_msec == 0)
			ticks_per_sleep_msec = (double)(TICKS_PER_SEC / 1000);

		// loop until we reach the target time
		while (curr - target < 0)
		{
			// if we have enough time to sleep, do it
			// ...but not if we're autoframeskipping and we're behind
			if (allow_sleep && (!autoframeskip || frameskip == 0) &&
				(target - curr) > (TICKER)(ticks_per_sleep_msec * 1.1))
			{
				// keep track of how long we actually slept
				Sleep(1);
				ticks_per_sleep_msec = (ticks_per_sleep_msec * 0.90) + ((double)(ticker() - curr) * 0.10);
			}

			// update the current time
			curr = ticker();
		}
	}

	// idle time done
	profiler_mark(PROFILER_END);
}



//============================================================
//	update_palette
//============================================================

static void update_palette(struct mame_display *display)
{
	int i, j;

	// loop over dirty colors in batches of 32
	for (i = 0; i < display->game_palette_entries; i += 32)
	{
//		UINT32 dirtyflags = palette_lookups_invalid ? ~0 : display->game_palette_dirty[i / 32];
		UINT32 dirtyflags = display->game_palette_dirty[i / 32];
		if (dirtyflags)
		{
			display->game_palette_dirty[i / 32] = 0;
			
			// loop over all 32 bits and update dirty entries
			for (j = 0; j < 32; j++, dirtyflags >>= 1)
				if (dirtyflags & 1)
				{
					// extract the RGB values
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);
					
					// update both lookup tables
					palette_16bit_lookup[i + j] = win_color16(r, g, b) * 0x10001;
					palette_32bit_lookup[i + j] = win_color32(r, g, b);
				}
		}
	}
	
	// reset the invalidate flag
	palette_lookups_invalid = 0;
}



//============================================================
//	update_visible_area
//============================================================

static void update_visible_area(struct mame_display *display)
{
	// copy the new parameters
	vis_min_x = display->game_visible_area.min_x;
	vis_max_x = display->game_visible_area.max_x;
	vis_min_y = display->game_visible_area.min_y;
	vis_max_y = display->game_visible_area.max_y;

	// track these changes
	logerror("set visible area %d-%d %d-%d\n",vis_min_x,vis_max_x,vis_min_y,vis_max_y);

	// compute the visible width and height
	vis_width  = vis_max_x - vis_min_x + 1;
	vis_height = vis_max_y - vis_min_y + 1;

	// tell the UI where it can draw
	set_ui_visarea(vis_min_x, vis_min_y, vis_min_x + vis_width - 1, vis_min_y + vis_height - 1);

	// now adjust the window for the aspect ratio
	if (vis_width > 1 && vis_height > 1)
		win_adjust_window_for_visible(vis_min_x, vis_max_x, vis_min_y, vis_max_y);
}



//============================================================
//	update_autoframeskip
//============================================================

void update_autoframeskip(void)
{
	// don't adjust frameskip if we're paused or if the debugger was
	// visible this cycle or if we haven't run yet
	if (!game_was_paused && !debugger_was_visible && cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS)
	{
		// if we're too fast, attempt to increase the frameskip
		if (game_speed_percent >= 99.5)
		{
			frameskipadjust++;

			// but only after 3 consecutive frames where we are too fast
			if (frameskipadjust >= 3)
			{
				frameskipadjust = 0;
				if (frameskip > 0) frameskip--;
			}
		}

		// if we're too slow, attempt to increase the frameskip
		else
		{
			// if below 80% speed, be more aggressive
			if (game_speed_percent < 80)
				frameskipadjust -= (90 - game_speed_percent) / 5;

			// if we're close, only force it up to frameskip 8
			else if (frameskip < 8)
				frameskipadjust--;

			// perform the adjustment
			while (frameskipadjust <= -2)
			{
				frameskipadjust += 2;
				if (frameskip < FRAMESKIP_LEVELS - 1)
					frameskip++;
			}
		}
	}

	// clear the other states
	game_was_paused = game_is_paused;
	debugger_was_visible = 0;
}



//============================================================
//	render_frame
//============================================================

static void render_frame(struct mame_bitmap *bitmap, void *vector_dirty_pixels)
{
	TICKER curr;

	// if we're throttling, synchronize
	if (throttle)
		throttle_speed();

	// at the end, we need the current time
	curr = ticker();

	// update stats for the FPS average calculation
	if (start_time == 0)
	{
		// start the timer going 1 second into the game
		if (timer_get_time() > 1.0)
			start_time = curr;
	}
	else
	{
		frames_displayed++;
		if (frames_displayed + 1 == frames_to_display)
			win_trying_to_quit = 1;
		end_time = curr;
	}

	// if we're at the start of a frameskip sequence, compute the speed
	if (frameskip_counter == 0)
		last_skipcount0_time = curr;

	// update the bitmap we're drawing
	profiler_mark(PROFILER_BLIT);
	win_update_video_window(bitmap, vector_dirty_pixels);
	profiler_mark(PROFILER_END);

	// if we're throttling and autoframeskip is on, adjust
	if (throttle && autoframeskip && frameskip_counter == 0)
		update_autoframeskip();
}



//============================================================
//	osd_update_video_and_audio
//============================================================

void osd_update_video_and_audio(struct mame_display *display)
{
	// if this is the first time through, initialize the previous time value
	if (warming_up)
	{
		last_skipcount0_time = ticker() - (int)((double)FRAMESKIP_LEVELS * (double)TICKS_PER_SEC / video_fps);
		warming_up = 0;
	}

	// if this is the first frame in a sequence, adjust the base time for this frame
	if (frameskip_counter == 0)
		this_frame_base = last_skipcount0_time + (int)((double)FRAMESKIP_LEVELS * (double)TICKS_PER_SEC / video_fps);
	
	// if the visible area has changed, update it
	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
		update_visible_area(display);

	// if the debugger focus changed, update it
	if (display->changed_flags & DEBUG_FOCUS_CHANGED)
		win_set_debugger_focus(display->debug_focus);

	// if the game palette has changed, update it
	if (display->changed_flags & GAME_PALETTE_CHANGED)
		update_palette(display);

	// if we're not skipping this frame, draw it
	if (!osd_skip_this_frame() && (display->changed_flags & GAME_BITMAP_CHANGED))
	{
		if (display->changed_flags & VECTOR_PIXELS_CHANGED)
			render_frame(display->game_bitmap, display->vector_dirty_pixels);
		else
			render_frame(display->game_bitmap, NULL);
	}

	// update the debugger
	if (display->changed_flags & DEBUG_BITMAP_CHANGED)
	{
		win_update_debug_window(display->debug_bitmap, display->debug_palette);
		debugger_was_visible = 1;
	}

	// if the LEDs have changed, update them
	if (display->changed_flags & LED_STATE_CHANGED)
		osd_set_leds(display->led_state);

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// check for inputs
	check_inputs();

	// poll the joystick values here
	win_process_events();
	win_poll_input();
}



//============================================================
//	osd_save_snapshot
//============================================================

void osd_save_snapshot(struct mame_bitmap *bitmap, const struct rectangle *bounds)
{
	save_screen_snapshot(bitmap, bounds);
}



//============================================================
//	osd_pause
//============================================================

void osd_pause(int paused)
{
	// note that we were paused during this autoframeskip cycle
	game_is_paused = paused;
	if (game_is_paused)
		game_was_paused = 1;

	// tell the input system
	win_pause_input(paused);
}
