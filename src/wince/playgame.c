#include <windows.h>
#include <stdio.h>
#include "mamece.h"
#include "driver.h"
#include "..\windows\window.h"

void decompose_rom_sample_path (const char *_rompath, const char *_samplepath);
#ifdef MESS
void decompose_software_path (const char *_softwarepath);
#endif

#define localconcat(s1, s2)	((s1) ? ((s2) ? __localconcat(_alloca(strlen(s1) + strlen(s2) + 1), (s1), (s2)) : (s1)) : (s2))

static char *__localconcat(char *dest, const char *s1, const char *s2)
{
	strcpy(dest, s1);
	strcat(dest, s2);
	return dest;
}

static void get_mame_root(TCHAR *buffer, int buflen)
{
#ifdef _X86_
	// To make things simple when running under emulation
	sntprintf(buffer, buflen, TEXT("\\"));
#else
	GetModuleFileName(NULL, buffer, buflen);
#endif
	tcsrchr(buffer, '\\')[0] = '\0';
}

static void set_mame_dir(char **dir, const char *rootpath, const char *subdir)
{
	char buffer[MAX_PATH];

	snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s\\%s", rootpath, subdir);
	if (*dir)
		free(*dir);
	*dir = strdup(buffer);
}

void setup_paths()
{
	extern char *cfgdir;
	extern char *nvdir;
	extern char *hidir;
	extern char *inpdir;
	extern char *stadir;
	extern char *memcarddir;
	extern char *artworkdir;
	extern char *screenshotdir;
	extern char *cheatfile;
	extern char *history_filename;
	extern char *mameinfo_filename;
	static char *my_cheatfile = NULL;
	static char *my_history_filename = NULL;
	static char *my_mameinfo_filename = NULL;

	TCHAR rootpath_buffer[MAX_PATH];
	char *rootpath;
	char *rompath;
	char *samplepath;

	get_mame_root(rootpath_buffer, sizeof(rootpath_buffer) / sizeof(rootpath_buffer[0]));
	rootpath = T2A(rootpath_buffer);

	set_mame_dir(&cfgdir,				rootpath,	"Cfg");
	set_mame_dir(&nvdir,				rootpath,	"Nvram");
	set_mame_dir(&hidir,				rootpath,	"Hi");
	set_mame_dir(&inpdir,				rootpath,	"Inp");
	set_mame_dir(&stadir,				rootpath,	"Sta");
	set_mame_dir(&memcarddir,			rootpath,	"Memcard");
	set_mame_dir(&artworkdir,			rootpath,	"Artwork");
	set_mame_dir(&screenshotdir,		rootpath,	"Snap");
	set_mame_dir(&my_mameinfo_filename,	rootpath,	"mameinfo.dat");
#ifdef MESS
//	set_mame_dir((char**)&crcdir,		rootpath,	"Crc");
	set_mame_dir(&my_cheatfile,			rootpath,	"cheat.cdb");
	set_mame_dir(&my_history_filename,	rootpath,	"sysinfo.dat");
	decompose_software_path(localconcat(rootpath, "\\Software"));
	rompath = localconcat(rootpath, "\\Bios");
#else
	set_mame_dir(&cheatfile,		rootpath,	"cheat.dat");
	set_mame_dir(&history_filename,	rootpath,	"history.dat");
	rompath = localconcat(rootpath, "\\ROMs");
#endif
	samplepath = localconcat(rootpath, "\\Samples");
	decompose_rom_sample_path(rompath, samplepath);

	cheatfile = my_cheatfile;
	history_filename = my_history_filename;
	mameinfo_filename = my_mameinfo_filename;
}

int play_game(int game_index, struct ui_options *opts)
{
	extern int use_dirty;
	extern int throttle;
	extern float gamma_correct;
	extern int attenuation;
	int original_leds;
	int err;

	options.antialias = opts->enable_antialias;
	options.translucency = opts->enable_translucency;
	options.rol = opts->rotate_left;
	options.ror = opts->rotate_right;
	options.samplerate = opts->enable_sound ? 44100 : 0;
	options.use_samples = 1;
	options.use_filter = 1;
	options.vector_flicker = (float) (opts->enable_flicker ? 255 : 0);
	options.vector_width = win_gfx_width;
	options.vector_height = win_gfx_height;
	options.color_depth = 0;
	options.norotate = 0;
	options.flipx = 0;
	options.flipy = 0;
	options.beam = 0x00010000;
	options.use_artwork = 0;
	options.cheat = 0;
	options.mame_debug = 0;
	options.playback = NULL;
	options.record = NULL;
	gamma_correct = 1;
	attenuation = 0;
	use_dirty = opts->enable_dirtyline;
	throttle = !opts->disable_throttle;
	win_window_mode = 0;

	/* remember the initial LED states */
	original_leds = osd_get_leds();

	#ifdef MESS
	/* Build the CRC database filename */
/*	sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
	if (drivers[game_index]->clone_of->name)
		sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
	else
		pcrcfilename[0] = 0;
  */  #endif

	err = run_game(game_index);

	/* restore the original LED state */
	osd_set_leds(original_leds);

	return err;
}

//============================================================
//	osd_init
//============================================================

int osd_init(void)
{
	extern int win_init_input(void);
	int result;

	result = win_init_window();
	if (result) {
		logerror("win_init_window() failed!\n");
		return result;
	}

	result = win_init_input();
	if (result) {
		logerror("win_init_input() failed!\n");
		return result;
	}

	return 0;
}



//============================================================
//	osd_exit
//============================================================

void osd_exit(void)
{
	extern void win_shutdown_input(void);
	extern void win_shutdown_window(void);

	win_shutdown_input();
	win_shutdown_window();
	osd_set_leds(0);
}



//============================================================
//	logerror
//============================================================

void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);

	{
		char szBuffer[256];
		_vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), text, arg);
		OutputDebugString(A2T(szBuffer));
	}

	va_end(arg);
}
