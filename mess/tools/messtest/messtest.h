#ifndef MESSTEST_H
#define MESSTEST_H

#include "mame.h"
#include "timer.h"

enum messtest_command_type
{
	MESSTEST_COMMAND_END,
	MESSTEST_COMMAND_WAIT,
	MESSTEST_COMMAND_INPUT,
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
			offs_t start;
			offs_t end;
			const void *verify_data;
			size_t verify_data_size;
		} verify_args;
	} u;
};

struct messtest_testcase
{
	char *name;
	char *driver;
	double time_limit;	/* 0.0 = default */
	struct messtest_command *commands;
};



enum messtest_result run_test(const struct messtest_testcase *testcase);



#endif /* MESSTEST_H */
