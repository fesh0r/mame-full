#ifndef MAMECE_H
#define MAMECE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <aygshell.h>
#include <unistd.h>

// --------------------------------------------------------------------------
// Standard C library redefines/augmentations

#define sntprintf	_sntprintf
#define tcsrchr		_tcsrchr
#define snprintf	_snprintf
#define tcscpy		_tcscpy
#define tcslen		_tcslen
#define tcscat		_tcscat

char *strdup(const char *s);

// --------------------------------------------------------------------------
// Windows API redefines

#define IsIconic(window)	(0)
#define MENU_HEIGHT 26

// --------------------------------------------------------------------------
// Malloc redefines

size_t outofmemory_occured(void);
void *mamece_malloc(size_t sz);
void *mamece_realloc(void *ptr, size_t sz);

#define malloc	mamece_malloc
#define realloc	mamece_realloc

// --------------------------------------------------------------------------
// Key options

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
	BOOL		enable_throttle;	// Enable Throttling
	BOOL		rotate_left;		// Rotate the Screen to the Left
	BOOL		rotate_right;		// Rotate the Screen to the Right
};

void setup_paths();
int play_game(int game_index, struct ui_options *opts);
void get_mame_root(TCHAR *buffer, int buflen);

#ifdef MESS
LPTSTR get_mess_output(void);
#endif

#ifdef _X86_
#define WINCE_EMULATION
#endif

#ifdef __cplusplus
};
#endif

#endif // MAMECE_H
