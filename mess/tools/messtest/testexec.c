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
static int test_flags;
static int screenshot_num;
static UINT64 runtime_hash;

static void message(enum messtest_messagetype msgtype, const char *fmt, ...);



static void dump_screenshot(void)
{
	mame_file *fp;
	char buf[128];

	/* if we are at runtime, dump a screenshot */
	snprintf(buf, sizeof(buf) / sizeof(buf[0]),
		(screenshot_num >= 0) ? "_%s_%d.png" : "_%s.png",
		current_testcase->name, screenshot_num);
	fp = mame_fopen(Machine->gamedrv->name, buf, FILETYPE_SCREENSHOT, 1);
	if (fp)
	{
		save_screen_snapshot_as(fp, artwork_get_ui_bitmap());
		mame_fclose(fp);
		message(MSG_INFO, "Saved screenshot as %s", buf);
	}

	if (screenshot_num >= 0)
		screenshot_num++;
}



static void message(enum messtest_messagetype msgtype, const char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	printf("%-12s %s %s\n", current_testcase->name, (msgtype ? "***" : "..."), buf);

	/* did we abort? */
	if ((msgtype == MSG_FAILURE) && (state != STATE_ABORTED))
	{
		state = STATE_ABORTED;
		final_time = timer_get_time();
		if (final_time > 0.0)
			dump_screenshot();
	}
}



enum messtest_result run_test(const struct messtest_testcase *testcase, int flags, struct messtest_results *results)
{
	int driver_num;
	enum messtest_result rc;
	clock_t begin_time;
	double real_run_time;
	int done;

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
	test_flags = flags;
	screenshot_num = 0;
	runtime_hash = 0;

	/* set up options */
	memset(&options, 0, sizeof(options));
	options.skip_disclaimer = 1;
	options.skip_gameinfo = 1;
	options.skip_warnings = 1;
	options.disable_normal_ui = 1;
	options.ram = testcase->ram;

	/* preload any needed images */
	while(current_command->command_type == MESSTEST_COMMAND_IMAGE_PRELOAD)
	{
		options.image_files[options.image_count].name = current_command->u.image_args.filename;
		options.image_files[options.image_count].type = current_command->u.image_args.device_type;
		options.image_count++;
		current_command++;
	}

	/* perform the test */
	message(MSG_INFO, "Beginning test (driver '%s')", testcase->driver);
	begin_time = clock();
	run_game(driver_num);
	real_run_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

	/* what happened? */
	switch(state) {
	case STATE_ABORTED:
		message(MSG_FAILURE, "Test aborted");
		rc = MESSTEST_RESULT_RUNTIMEFAILURE;
		break;

	case STATE_DONE:
		message(MSG_INFO, "Test succeeded (real time %.2f; emu time %.2f [%i%%])",
			real_run_time, final_time, (int) ((final_time / real_run_time) * 100));
		rc = MESSTEST_RESULT_SUCCESS;
		break;

	default:
		state = STATE_ABORTED;
		message(MSG_FAILURE, "Abnormal termination");
		rc = MESSTEST_RESULT_STARTFAILURE;
		break;
	}

	if (results)
	{
		results->rc = rc;
		results->runtime_hash = runtime_hash;
	}
	return rc;
}



int osd_trying_to_quit(void)
{
	return (state == STATE_ABORTED) || (state == STATE_DONE);
}



static void find_switch(const char *switch_name, const char *switch_setting,
	int switch_type, int switch_setting_type,
	struct InputPort **in_switch, struct InputPort **in_switch_setting)
{
	struct InputPort *in;

	*in_switch = NULL;
	*in_switch_setting = NULL;

	/* find switch with the name */
	in = Machine->input_ports;
	while(in->type != IPT_END)
	{
		if ((in->type & ~IPF_MASK) == switch_type && (in->type & IPF_UNUSED) == 0
			&& input_port_name(in) && !stricmp(input_port_name(in), switch_name))
			break;
		in++;
	}
	if (in->type == IPT_END)
		return;
	*in_switch = in;

	/* find the setting */
	in++;
	while((in->type & ~IPF_MASK) == switch_setting_type)
	{
		if ((in->type & IPF_UNUSED) == 0 && input_port_name(in) && !stricmp(input_port_name(in), switch_setting))
			break;
		in++;
	}
	if ((in->type & ~IPF_MASK) != switch_setting_type)
		return;
	*in_switch_setting = in;
}



void osd_update_video_and_audio(struct mame_display *display)
{
	int i;
	int device_type;
	int device_slot;
	offs_t offset, offset_start, offset_end;
	const UINT8 *verify_data;
	size_t verify_data_size;
	const UINT8 *target_data;
	size_t target_data_size;
	double time_limit;
	double current_time;
	int region;
	const char *filename;
	char buf[128];
	mess_image *image;
	struct InputPort *switch_name;
	struct InputPort *switch_setting;
	int cpunum;

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

	/* update the runtime hash */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		runtime_hash *= 57;
		runtime_hash ^= cpunum_get_reg(cpunum, REG_PC);	/* TODO - Add more registers? */
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

	case MESSTEST_COMMAND_SCREENSHOT:
		dump_screenshot();
		break;

	case MESSTEST_COMMAND_SWITCH:
		find_switch(current_command->u.switch_args.name, current_command->u.switch_args.value,
			IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING, &switch_name, &switch_setting);

		if (!switch_name || !switch_setting)
		{
			find_switch(current_command->u.switch_args.name, current_command->u.switch_args.value,
				IPT_CONFIG_NAME, IPT_CONFIG_SETTING, &switch_name, &switch_setting);
		}

		if (!switch_name)
		{
			message(MSG_FAILURE, "Cannot find switch named '%s'", current_command->u.switch_args.name);
			break;
		}

		if (!switch_setting)
		{
			message(MSG_FAILURE, "Cannot find setting '%s' on switch '%s'",
				current_command->u.switch_args.value, current_command->u.switch_args.name);
			break;
		}

		switch_name->default_value = switch_setting->default_value;
		break;

	case MESSTEST_COMMAND_IMAGE_PRELOAD:
		message(MSG_FAILURE, "Image preloads must be at the beginning");
		break;

	case MESSTEST_COMMAND_IMAGE_CREATE:
	case MESSTEST_COMMAND_IMAGE_LOAD:
		device_slot = current_command->u.image_args.device_slot;
		device_type = current_command->u.image_args.device_type;

		image = image_from_devtype_and_index(device_type, device_slot);
		if (!image)
		{
			message(MSG_FAILURE, "Image slot '%s %i' does not exist",
				device_typename(device_type), device_slot);
			break;
		}

		filename = current_command->u.image_args.filename;
		if (!filename)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]),	"%s.%s",
				current_testcase->name,
				device_find(Machine->gamedrv, device_type)->file_extensions);
			filename = buf;
		}

		/* actually create or load the image */
		switch(current_command->command_type) {
		case MESSTEST_COMMAND_IMAGE_CREATE:
			if (image_create(image, filename, 0, NULL))
			{
				message(MSG_FAILURE, "Failed to create image '%s'", filename);
				break;
			}
			break;
		
		case MESSTEST_COMMAND_IMAGE_LOAD:
			if (image_load(image, filename))
			{
				message(MSG_FAILURE, "Failed to load image '%s'", filename);
				break;
			}
			break;
		}
		break;

	case MESSTEST_COMMAND_VERIFY_MEMORY:
		i = 0;
		offset_start = current_command->u.verify_args.start;
		offset_end = current_command->u.verify_args.end;
		verify_data = (const UINT8 *) current_command->u.verify_args.verify_data;
		verify_data_size = current_command->u.verify_args.verify_data_size;

		region = current_command->u.verify_args.mem_region;
		if (region)
		{
			target_data = memory_region(region);
			target_data_size = memory_region_length(region);
		}
		else
		{
			target_data = mess_ram;
			target_data_size = mess_ram_size;
		}

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
				message(MSG_FAILURE, "Failed verification step (REGION_%s; 0x%x-0x%x)",
					memory_region_to_string(region), offset_start, offset_end);
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
	{
		/* if we are at the end, and we are dumping screenshots, and we didn't
		 * just dump a screenshot, dump one now
		 */
		if ((test_flags & MESSTEST_ALWAYS_DUMP_SCREENSHOT) &&
			(current_command[0].command_type != MESSTEST_COMMAND_SCREENSHOT) &&
			(current_command[1].command_type == MESSTEST_COMMAND_END))
		{
			dump_screenshot();
		}

		current_command++;
	}
}



char *rompath_extra;

