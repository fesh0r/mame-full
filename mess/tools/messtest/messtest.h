#ifndef MESSTEST_H
#define MESSTEST_H

#include "mame.h"
#include "timer.h"

enum messtest_command_type
{
	MESSTEST_COMMAND_END,
	MESSTEST_COMMAND_WAIT,
	MESSTEST_COMMAND_INPUT,
	MESSTEST_COMMAND_IMAGE_CREATE,
	MESSTEST_COMMAND_IMAGE_LOAD,
	MESSTEST_COMMAND_VERIFY_MEMORY
};

enum messtest_result
{
	MESSTEST_RESULT_SUCCESS,
	MESSTEST_RESULT_STARTFAILURE,
	MESSTEST_RESULT_RUNTIMEFAILURE
};

struct messtest_command
{
	enum messtest_command_type command_type;
	union
	{
		double wait_time;
		const char *input_chars;
		struct
		{
			int mem_region;
			offs_t start;
			offs_t end;
			const void *verify_data;
			size_t verify_data_size;
		} verify_args;
		struct
		{
			const char *filename;
			int device_type;
			int device_slot;
		} image_args;
	} u;
};

struct messtest_testcase
{
	char *name;
	char *driver;
	double time_limit;	/* 0.0 = default */
	struct messtest_command *commands;

	/* options */
	UINT32 ram;
};



#define MESSTEST_ALWAYS_DUMP_SCREENSHOT		1


int memory_region_from_string(const char *region_name);
const char *memory_region_to_string(int region);

enum messtest_result run_test(const struct messtest_testcase *testcase, int flags);
int messtest(const char *script_filename, int flags, int *test_count, int *failure_count);

#endif /* MESSTEST_H */
