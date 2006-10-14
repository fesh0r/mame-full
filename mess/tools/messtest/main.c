/*********************************************************************

	main.c

	MESS testing main module

*********************************************************************/

#include <time.h>

#include "core.h"
#include "hashfile.h"
#include "options.h"
#include "../imgtool/imgtool.h"

#ifdef WIN32
#include <windows.h>
#include "windows/glob.h"
#endif /* WIN32 */

extern int mame_validitychecks(int game);

static int test_count, failure_count;

static const options_entry messtest_opts[] =
{
	{ "" },
	{ "dumpscreenshots;ds",		"0",	OPTION_BOOLEAN,	"always dump screenshots" },
	{ "preservedir;pd",			"0",	OPTION_BOOLEAN,	"preserve current directory" },
	{ "rdtsc",					"0",	OPTION_BOOLEAN, "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",				"0",	0,				"thread priority for the main game thread; range from -15 to 1" },

	// file and directory options
	{ NULL,                       NULL,       OPTION_HEADER,     "PATH AND DIRECTORY OPTIONS" },
#ifndef MESS
	{ "rompath;rp",               "roms",     0,                 "path to ROMsets and hard disk images" },
#else
	{ "biospath;bp",              "bios",     0,                 "path to BIOS sets" },
	{ "softwarepath;swp",         "software", 0,                 "path to software" },
	{ "hash_directory;hash",      "hash",     0,                 "path to hash files" },
#endif
	{ "samplepath;sp",            "samples",  0,                 "path to samplesets" },
#ifdef __WIN32__
	{ "inipath",                  ".;ini",    0,                 "path to ini files" },
#else
	{ "inipath",                  "$HOME/.mame;.;ini", 0,        "path to ini files" },
#endif
	{ "cfg_directory",            "cfg",      0,                 "directory to save configurations" },
	{ "nvram_directory",          "nvram",    0,                 "directory to save nvram contents" },
	{ "memcard_directory",        "memcard",  0,                 "directory to save memory card contents" },
	{ "input_directory",          "inp",      0,                 "directory to save input device logs" },
	{ "hiscore_directory",        "hi",       0,                 "directory to save hiscores" },
	{ "state_directory",          "sta",      0,                 "directory to save states" },
	{ "artpath;artwork_directory","artwork",  0,                 "path to artwork files" },
	{ "snapshot_directory",       "snap",     0,                 "directory to save screenshots" },
	{ "diff_directory",           "diff",     0,                 "directory to save hard drive image difference files" },
	{ "ctrlrpath;ctrlr_directory","ctrlr",    0,                 "path to controller definitions" },
	{ "comment_directory",        "comments", 0,                 "directory to save debugger comments" },
	{ "cheat_file",               "cheat.dat",0,                 "cheat filename" },
	{ NULL }
};



/*************************************
 *
 *	Main and argument parsing/handling
 *
 *************************************/

static void handle_arg(const char *arg)
{
	int this_test_count;
	int this_failure_count;
	struct messtest_options opts;

	/* setup options */
	memset(&opts, 0, sizeof(opts));
	opts.script_filename = arg;
	if (options_get_bool("preservedir"))
		opts.preserve_directory = 1;
	if (options_get_bool("dumpscreenshots"))
		opts.dump_screenshots = 1;

	if (messtest(&opts, &this_test_count, &this_failure_count))
		exit(-1);

	test_count += this_test_count;
	failure_count += this_failure_count;
}



#ifdef WIN32
static void win_expand_wildcards(int *argc, char **argv[])
{
	int i;
	glob_t g;

	memset(&g, 0, sizeof(g));

	for (i = 0; i < *argc; i++)
		glob((*argv)[i], (g.gl_pathc > 0) ? GLOB_APPEND|GLOB_NOCHECK : GLOB_NOCHECK, NULL, &g);

	*argc = g.gl_pathc;
	*argv = g.gl_pathv;
}
#endif /* WIN32 */



int main(int argc, char *argv[])
{
	int result = -1;
	clock_t begin_time;
	double elapsed_time;

#ifdef WIN32
	/* expand wildcards so '*' can be used; this is not UNIX */
	win_expand_wildcards(&argc, &argv);
#else
	{
		/* this is for XMESS */
		extern const char *cheatfile;
		extern const char *db_filename;

		cheatfile = db_filename = NULL;
	}
#endif /* WIN32 */

	test_count = 0;
	failure_count = 0;

	/* since the cpuintrf and sndintrf structures are filled dynamically now, we
	 * have to init first */
	cpuintrf_init(NULL);
	sndintrf_init(NULL);
	
	/* register options */
	options_add_entries(messtest_opts);
	options_set_option_callback("", handle_arg);

	/* run MAME's validity checks; if these fail cop out now */
	/* NPW 16-Sep-2006 - commenting this out because this cannot be run outside of MAME */
	//if (mame_validitychecks(-1))
	//	goto done;
	/* run Imgtool's validity checks; if these fail cop out now */
	if (imgtool_validitychecks())
		goto done;

	begin_time = clock();

	/* parse the commandline */
	if (options_parse_command_line(argc, argv))
	{
		fprintf(stderr, "Error while parsing cmdline\n");
		goto done;
	}

	if (test_count > 0)
	{
		elapsed_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

		fprintf(stderr, "Tests complete; %i test(s), %i failure(s), elapsed time %.2f\n",
			test_count, failure_count, elapsed_time);
	}
	else
	{
		fprintf(stderr, "Usage: %s [test1] [test2]...\n", argv[0]);
	}
	result = failure_count;

done:
	return result;
}

