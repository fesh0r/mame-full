#ifndef MAMECE_H
#define MAMECE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <aygshell.h>
#include <unistd.h>

// --------------------------------------------------------------------------

#define sntprintf	_sntprintf
#define tcsrchr		_tcsrchr
#define snprintf	_snprintf
#define tcscpy		_tcscpy

char *strdup(const char *s);

// --------------------------------------------------------------------------
// Malloc redefine

size_t outofmemory_occured(void);
void *mamece_malloc(size_t sz);
void *mamece_realloc(void *ptr, size_t sz);

#define malloc	mamece_malloc
#define realloc	mamece_realloc

// --------------------------------------------------------------------------

// Utility functions
int __ascii2widelen(const char *asciistr);
WCHAR *__ascii2wide(WCHAR *dest, const char *asciistr);
int __wide2asciilen(const WCHAR *widestr);
char *__wide2ascii(char *dest, const WCHAR *widestr);

#define A2W(asciistr)	__ascii2wide(_alloca(__ascii2widelen(asciistr) * sizeof(WCHAR)), (asciistr))
#define W2A(widestr)	__wide2ascii(_alloca(__wide2asciilen(widestr) * sizeof(char)), (widestr))

#ifdef UNICODE
#define A2T(asciistr)	A2W(asciistr)
#define T2A(widestr)	W2A(widestr)
#else
#error Non-Unicode not supported
#endif

// --------------------------------------------------------------------------
// Windows API redefines

#define IsIconic(window)	(0)

// --------------------------------------------------------------------------

// our dialog/configured options //
struct ui_options
{
	BOOL		enable_sound;		// Enable sound in games
	BOOL		show_all_games;		// Should we show all games?
	BOOL		show_clones;		// Don't show clone games
	BOOL		show_framerate;		// Display Frame Rate
	BOOL		show_profiler;		// Display the profiler
	BOOL		enable_flicker;		// Enable Flicker
	BOOL		enable_translucency;// Enable Translucency
	BOOL		enable_antialias;	// Enable AntiAlias
	BOOL		enable_dirtyline;	// Enable Dirty Line Drawing
	BOOL		disable_throttle;	// Disable Throttling
	BOOL		rotate_left;		// Rotate the Screen to the Left
	BOOL		rotate_right;		// Rotate the Screen to the Right
};

void setup_paths();
int play_game(int game_index, struct ui_options *opts);

// --------------------------------------------------------------------------
// GAPI stuff
//
// Because the morons who wrote GAPI did not understand that not everybody
// uses C++

struct gx_keylist
{
	short vkUp;
	short vkDown;
	short vkLeft;
	short vkRight;
	short vkA;
	short vkB;
	short vkC;
	short vkStart;
};

struct gx_display_properties
{
	DWORD cxWidth;
	DWORD cyHeight;
	long cbxPitch;
	long cbyPitch;
	long cBPP;
	DWORD ffFormat;
};

int gx_open_input(void);
int gx_close_input(void);
int gx_open_display(HWND hWnd);
int gx_close_display(void);
void *gx_begin_draw(void);
int gx_end_draw(void);
void gx_get_default_keys(struct gx_keylist *keylist);
void gx_get_display_properties(struct gx_display_properties *properties);
void gx_blit(struct osd_bitmap *bitmap, int update, int orientation, UINT32 *palette_16bit_lookup, UINT32 *palette_32bit_lookup);

#ifdef __cplusplus
};
#endif

#endif // MAMECE_H
