#ifndef MAMECE_H
#define MAMECE_H

#include <windows.h>

// --------------------------------------------------------------------------

#define sntprintf	_sntprintf
#define tcsrchr		_tcsrchr
#define snprintf	_snprintf

char *strdup(const char *s);

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
void play_game(int game_index, struct ui_options *opts);

#endif // MAMECE_H
