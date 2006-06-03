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
extern const options_entry fileio_opts[];

static int dump_screenshots;
static int preserve_directory;
static int test_count, failure_count;

static const options_entry messtest_opts[] =
{
	{ "" },
	{ "dumpscreenshots;ds",		"0",	OPTION_BOOLEAN,	"always dump screenshots" },
	{ "preservedir;pd",			"0",	OPTION_BOOLEAN,	"preserve current directory" },
	{ "rdtsc",					"0",	OPTION_BOOLEAN, "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",				"0",	0,				"thread priority for the main game thread; range from -15 to 1" },
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
	if (preserve_directory)
		opts.preserve_directory = 1;
	if (dump_screenshots)
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
	extern int mess_ghost_images;

	mess_ghost_images = 1;

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
	cpuintrf_init();
	sndintrf_init();
	
	/* register options */
	options_add_entries(messtest_opts);
	options_add_entries(fileio_opts);
	options_set_option_callback("", handle_arg);

	/* run MAME's validity checks; if these fail cop out now */
	if (mame_validitychecks(-1))
		goto done;
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

	/* read options */
	dump_screenshots = options_get_bool("dumpscreenshots", TRUE);
	preserve_directory = options_get_bool("preservedir", TRUE);

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

