#include <time.h>

#include "inputx.h"
#include "messtest.h"

enum messtest_state
{
	STATE_READY,
	STATE_INCOMMAND,
	STATE_ABORTED,
	STATE_DONE
};

enum messtest_messagetype
{
	MSG_INFO,
	MSG_FAILURE,
	MSG_PREFAILURE
};

static const struct messtest_testcase *current_testcase;
static enum messtest_state state;
static double wait_target;
static double final_time;
static const struct messtest_command *current_command;
static int abort_test;



static void message(enum messtest_messagetype msgtype, const char *fmt, ...)
{
	char buf[1024];
	va_list va;
	mame_file *fp;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	printf("%-12s %s %s\n", current_testcase->name, (msgtype ? "***" : "..."), buf);

	/* did we abort? */
	if ((msgtype == MSG_FAILURE) && (state != STATE_ABORTED))
	{
		state = STATE_ABORTED;
		final_time = timer_get_time();

		/* if we are at runtime, dump a screenshot */
		if (final_time > 0.0)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "test_%s.png", current_testcase->name);
			fp = mame_fopen(Machine->gamedrv->name, buf, FILETYPE_SCREENSHOT, 1);
			if (fp)
			{
				save_screen_snapshot_as(fp, artwork_get_ui_bitmap());
				mame_fclose(fp);
				message(MSG_INFO, "Saved screenshot as %s", buf);
			}
		}
	}
}



enum messtest_result run_test(const struct messtest_testcase *testcase)
{
	int driver_num;
	enum messtest_result result;
	clock_t begin_time;
	double real_run_time;

	current_testcase = testcase;

	/* lookup driver */
	for (driver_num = 0; drivers[driver_num]; driver_num++)
	{
		if (!strcmpi(testcase->driver, drivers[driver_num]->name))
			break;
	}

	/* cannot find driver? */
	if (!drivers[driver_num])
	{
		message(MSG_PREFAILURE, "Cannot find driver '%s'", testcase->driver);
		return MESSTEST_RESULT_STARTFAILURE;
	}

	/* prepare testing state */
	current_command = testcase->commands;
	state = STATE_READY;

	/* set up options */
	memset(&options, 0, sizeof(options));
	options.skip_disclaimer = 1;
	options.skip_gameinfo = 1;
	options.skip_warnings = 1;
	options.disable_normal_ui = 1;

	/* perform the test */
	message(MSG_INFO, "Beginning test (driver '%s')", testcase->driver);
	begin_time = clock();
	run_game(driver_num);
	real_run_time = ((double) clock()) / CLOCKS_PER_SEC;

	/* what happened? */
	switch(state) {
	case STATE_ABORTED:
		message(MSG_FAILURE, "Test aborted");
		result = MESSTEST_RESULT_RUNTIMEFAILURE;
		break;

	case STATE_DONE:
		message(MSG_INFO, "Test succeeded (real time %.2f; emu time %.2f [%i%%])",
			real_run_time, final_time, (int) ((final_time / real_run_time) * 100));
		result = MESSTEST_RESULT_SUCCESS;
		break;

	default:
		message(MSG_FAILURE, "Abnormal termination");
		result = MESSTEST_RESULT_STARTFAILURE;
		break;
	}

	return result;
}



int osd_trying_to_quit(void)
{
	return (state == STATE_ABORTED) || (state == STATE_DONE);
}



void osd_update_video_and_audio(struct mame_display *display)
{
	int i;
	offs_t offset, offset_start, offset_end;
	const UINT8 *verify_data;
	size_t verify_data_size;
	const UINT8 *target_data;
	size_t target_data_size;
	double time_limit;
	double current_time;

	/* if we have already aborted or completed, our work is done */
	if ((state == STATE_ABORTED) || (state == STATE_DONE))
		return;

	/* have we hit the time limit? */
	current_time = timer_get_time();
	time_limit = (current_testcase->time_limit != 0.0) ? current_testcase->time_limit
		: TIME_IN_SEC(600);
	if (current_time > time_limit)
	{
		message(MSG_FAILURE, "Time limit of %.2f seconds exceeded", time_limit);
		return;
	}

	switch(current_command->command_type) {
	case MESSTEST_COMMAND_WAIT:
		if (state == STATE_READY)
		{
			/* beginning a wait command */
			wait_target = current_time + current_command->u.wait_time;
			state = STATE_INCOMMAND;
		}
		else
		{
			/* during a wait command */
			state = (current_time >= wait_target) ? STATE_READY : STATE_INCOMMAND;
		}
		break;

	case MESSTEST_COMMAND_INPUT:
		/* post a set of characters to the emulation */
		if (state == STATE_READY)
		{
			if (!inputx_can_post())
			{
				message(MSG_FAILURE, "Natural keyboard input not supported for this driver");
				break;
			}

			inputx_post_utf8(current_command->u.input_chars);
		}
		state = inputx_is_posting() ? STATE_INCOMMAND : STATE_READY;
		break;

	case MESSTEST_COMMAND_VERIFY_MEMORY:
		i = 0;
		offset_start = current_command->u.verify_args.start;
		offset_end = current_command->u.verify_args.end;
		verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
		verify_data_size = current_command->u.verify_args.verify_data_size;
		target_data = mess_ram;
		target_data_size = mess_ram_size;

		/* sanity check the ranges */
		if (!verify_data || (verify_data_size <= 0))
		{
			message(MSG_FAILURE, "Invalid memory region during verify");
			break;
		}
		if (offset_start > offset_end)
		{
			message(MSG_FAILURE, "Invalid verify offset range (0x%x-0x%x)", offset_start, offset_end);
			break;
		}
		if (offset_end >= target_data_size)
		{
			message(MSG_FAILURE, "Verify memory range out of bounds");
			break;
		}

		for (offset = offset_start; offset <= offset_end; offset++)
		{
			if (verify_data[i] != target_data[offset])
			{
				message(MSG_FAILURE, "Failed verification step (0x%x-0x%x)", offset_start, offset_end);
				break;
			}
			i = (i + 1) % verify_data_size;
		}
		break;

	case MESSTEST_COMMAND_END:
		/* at the end of our test */
		state = STATE_DONE;
		final_time = current_time;
		break;
	}

	/* if we are ready for the next command, advance to it */
	if (state == STATE_READY)
		current_command++;
}



char *rompath_extra;

