/*********************************************************************

	main.c

	MESS testing main module

*********************************************************************/

#include <time.h>

#include "rc.h"
#include "messtest.h"
#include "hashfile.h"

#ifdef WIN32
#include "glob.h"
#include "parallel.h"
#endif /* WIN32 */

extern struct rc_option fileio_opts[];

static int dump_screenshots;
static int test_count, failure_count;
static int run_differing_ram_test;
static int run_hash_test;

static struct rc_option opts[] =
{
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ "dumpscreenshots",	"ds",	rc_bool,	&dump_screenshots,			"0", 0, 0, NULL,	"always dump screenshots" },
	{ "difframtest",		"drt",	rc_set_int,	&run_differing_ram_test,	NULL, 1, 0, NULL,	"run a differing ram test" },
	{ "hashtest",			"h",	rc_set_int, &run_hash_test,				NULL, 1, 0, NULL,	"test all hash files" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};


/*************************************
 *
 *	Differing RAM test
 *
 *  This test runs every driver that has configurable RAM twice, with
 *  different default RAM values, and compares the runtime hashes.  If these
 *  values differ, that is surely the result of an emulated crash
 *
 *************************************/


static void differing_ram_test(void)
{
	int i;
	struct messtest_results results;
	struct messtest_testcase test;
	struct messtest_command test_commands[2];
	data8_t saved_ram_default_value;
	UINT64 first_hash;
	UINT64 second_hash;
	char test_name[50];
	const struct IODevice *dev;
	int requires_device;

	saved_ram_default_value = mess_ram_default_value;

	/* set up test case */
	memset(test_commands, 0, sizeof(test_commands));
	test_commands[0].command_type = MESSTEST_COMMAND_WAIT;
	test_commands[0].u.wait_time = TIME_IN_SEC(10);
	test_commands[1].command_type = MESSTEST_COMMAND_END;
	memset(&test, 0, sizeof(test));
	test.commands = test_commands;

	for (i = 0; drivers[i]; i++)
	{
		/* only do these tests on drivers that use mess_ram */
		if (ram_option_count(drivers[i]) == 0)
			continue;

		/* do not test drivers that require certain devices to be specified */
		requires_device = FALSE;
		for (dev = device_first(drivers[i]); dev; dev = device_next(drivers[i], dev))
		{
			if (dev->flags & DEVICE_MUST_BE_LOADED)
				requires_device = TRUE;
		}
		if (requires_device)
			continue;

		if (strcmp(drivers[i]->name, "coupe"))
			continue;

		/* fill in test info */
		snprintf(test_name, sizeof(test_name) / sizeof(test_name[0]), "%s_drt", drivers[i]->name);
		test.name = test_name;
		test.driver = drivers[i]->name;

		mess_ram_default_value = 0xCD;
		if (run_test(&test, 0, &results))
			continue;
		first_hash = results.runtime_hash;

		mess_ram_default_value = 0x00;
		if (run_test(&test, 0, &results))
			continue;
		second_hash = results.runtime_hash;

		if (first_hash != second_hash)
		{
			fprintf(stderr, "*** RUNTIME HASHES DIFFER; crash likely (%08x%08x / %08x%08x)\n",
				(unsigned) (first_hash >> 32), (unsigned) (first_hash >> 0),
				(unsigned) (second_hash >> 32), (unsigned) (second_hash >> 0));
		}
	}

	mess_ram_default_value = saved_ram_default_value;
}



/*************************************
 *
 *	Hashfile verification
 *
 *************************************/

static void my_puts(const char *msg)
{
	puts(msg);
}



static void hash_test(void)
{
	int i;

	for (i = 0; drivers[i]; i++)
	{
		printf("%s\n", drivers[i]->name);
		hashfile_verify(drivers[i]->name, my_puts);
	}
}



/*************************************
 *
 *	Main and argument parsing/handling
 *
 *************************************/

static int handle_arg(char *arg)
{
	int this_test_count;
	int this_failure_count;
	int flags = 0;

	/* compute the flags */
	if (dump_screenshots)
		flags |= MESSTEST_ALWAYS_DUMP_SCREENSHOT;

	if (messtest(arg, flags, &this_test_count, &this_failure_count))
		exit(-1);

	test_count += this_test_count;
	failure_count += this_failure_count;
	return 0;
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
	struct rc_struct *rc = NULL;
	int result = -1;
	clock_t begin_time;
	double elapsed_time;
	extern int mess_ghost_images;

	mess_ghost_images = 1;

#ifdef WIN32
	/* expand wildcards so '*' can be used; this is not UNIX */
	win_expand_wildcards(&argc, &argv);

	win_parallel_init();
#else
	{
		/* this is for XMESS */
		extern const char *cheatfile;
		extern const char *db_filename;
		extern const char *history_filename;
		extern const char *mameinfo_filename;

		cheatfile = db_filename = history_filename = mameinfo_filename
			= NULL;
	}
#endif /* WIN32 */

	test_count = 0;
	failure_count = 0;

	/* since the cpuintrf structure is filled dynamically now, we have to init first */
	cpuintrf_init();
	
	/* run MAME's validity checks; if these fail cop out now */
	if (mame_validitychecks())
		goto done;

	/* create rc struct */
	rc = rc_create();
	if (!rc)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	/* register options */
	if (rc_register(rc, opts))
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	begin_time = clock();

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, handle_arg))
	{
		fprintf(stderr, "Error while parsing cmdline\n");
		goto done;
	}

	if (run_differing_ram_test)
		differing_ram_test();
	if (run_hash_test)
		hash_test();

	if (test_count > 0)
	{
		elapsed_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

		fprintf(stderr, "Tests complete; %i test(s), %i failure(s), elapsed time %.2f\n",
			test_count, failure_count, elapsed_time);
	}
	else if (!run_differing_ram_test)
	{
		fprintf(stderr, "Usage: %s [test1] [test2]...\n", argv[0]);
	}
	result = failure_count;

done:
	if (rc)
		rc_destroy(rc);
#ifdef WIN32
	win_parallel_exit();
#endif /* WIN32 */
	return result;
}

