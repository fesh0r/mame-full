#include <time.h>

#include "rc.h"
#include "messtest.h"
#include "parallel.h"
#include "glob.h"

extern struct rc_option fileio_opts[];

static int dump_screenshots;

static struct rc_option opts[] =
{
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ "dumpscreenshots", "ds", rc_bool, &dump_screenshots, "0", 0, 0, NULL, "always dump screenshots" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

static int test_count, failure_count;

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



int main(int argc, char *argv[])
{
	struct rc_struct *rc;
	int result = -1;
	clock_t begin_time;
	double elapsed_time;

	/* expand wildcards so '*' can be used; this is not UNIX */
	win_expand_wildcards(&argc, &argv);

	win_parallel_init();

	test_count = 0;
	failure_count = 0;

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
	if (rc)
		rc_destroy(rc);
	win_parallel_exit();
	return result;
}

