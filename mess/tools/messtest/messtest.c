#include <stdio.h>
#include <ctype.h>

#include "messtest.h"
#include "osdepend.h"
#include "pool.h"
#include "rc.h"

#if 0
/* Include the internal copy of the libexpat library */
#define ELEMENT_TYPE ELEMENT_TYPE_
#include "xml2info/xmlrole.c"
#include "xml2info/xmltok.c"
#include "xml2info/xmlparse.c"
#undef ELEMENT_TYPE
#else
#define XMLPARSEAPI(type) type
#include "xml2info/expat.h"
#endif

/* ----------------------------------------------------------------------- */


enum messtest_phase
{
	STATE_ROOT,
	STATE_TEST,
	STATE_COMMAND,
	STATE_ABORTED
};

struct messtest_state
{
	XML_Parser parser;
	enum messtest_phase phase;
	memory_pool pool;

	const char *script_filename;
	struct messtest_testcase testcase;
	int command_count;

	struct messtest_command current_command;
};



static const XML_Char *find_attribute(const XML_Char **attributes, const XML_Char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



static int append_command(struct messtest_state *state)
{
	struct messtest_command *new_commands;

	new_commands = (struct messtest_command *) pool_realloc(&state->pool,
		state->testcase.commands, (state->command_count + 1) * sizeof(state->testcase.commands[0]));
	if (!new_commands)
		return FALSE;

	state->testcase.commands = new_commands;
	state->testcase.commands[state->command_count++] = state->current_command;
	return TRUE;
}



static void report_parseerror(struct messtest_state *state, const char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	fprintf(stderr, "%s:(%i:%i): %s\n",
		state->script_filename,
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		buf);
	state->phase = STATE_ABORTED;
}



static void parse_offset(const char *s, offs_t *result)
{
	if ((s[0] == '0') && (tolower(s[1]) == 'x'))
		sscanf(&s[2], "%x", result);
	else
		*result = atoi(s);
}



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
	struct messtest_state *state = (struct messtest_state *) data;
	const XML_Char *s;
	const XML_Char *s1;
	const XML_Char *s2;
	const char *attr_name;
	struct messtest_command cmd;

	switch(state->phase) {
	case STATE_ROOT:
		if (!strcmp(tagname, "tests"))
		{
			/* <tests> - used to group tests together */
		}
		else if (!strcmp(tagname, "test"))
		{
			/* <test> - identifies a specific test */

			memset(&state->testcase, 0, sizeof(state->testcase));

			attr_name = "name";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;
			state->testcase.name = pool_strdup(&state->pool, s);
			if (!state->testcase.name)
				goto outofmemory;

			attr_name = "driver";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;
			state->testcase.driver = pool_strdup(&state->pool, s);
			if (!state->testcase.driver)
				goto outofmemory;

			state->phase = STATE_TEST;
			state->testcase.commands = NULL;
			state->command_count = 0;
		}
		else
			goto unknowntag;
		break;

	case STATE_TEST:
		memset(&state->current_command, 0, sizeof(state->current_command));
		if (!strcmp(tagname, "wait"))
		{
			attr_name = "time";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;

			state->current_command.command_type = MESSTEST_COMMAND_WAIT;
			state->current_command.u.wait_time = atof(s);
		}
		else if (!strcmp(tagname, "input"))
		{
			state->current_command.command_type = MESSTEST_COMMAND_INPUT;
		}
		else if (!strcmp(tagname, "memverify"))
		{
			attr_name = "start";
			s1 = find_attribute(attributes, attr_name);
			if (!s1)
				goto missing_attribute;

			attr_name = "end";
			s2 = find_attribute(attributes, attr_name);
			if (!s2)
				goto missing_attribute;

			state->current_command.command_type = MESSTEST_COMMAND_VERIFY_MEMORY;
			parse_offset(s1, &state->current_command.u.verify_args.start);
			parse_offset(s2, &state->current_command.u.verify_args.end);
		}
		else
			goto unknowntag;
		state->phase = STATE_COMMAND;
		break;
	}

	return;

outofmemory:
	report_parseerror(state, "Out of memory");
	return;

missing_attribute:
	report_parseerror(state, "Missing attribute '%s'\n", attr_name);
	return;

unknowntag:
	report_parseerror(state, "Unknown tag '%s'\n", tagname);
	return;
}



static void end_handler(void *data, const XML_Char *name)
{
	struct messtest_state *state = (struct messtest_state *) data;

	switch(state->phase) {
	case STATE_TEST:
		/* append final end command */
		memset(&state->current_command, 0, sizeof(state->current_command));
		state->current_command.command_type = MESSTEST_COMMAND_END;
		if (!append_command(state))
			goto outofmemory;

		run_test(&state->testcase);
		state->phase = STATE_ROOT;
		break;

	case STATE_COMMAND:
		if (!append_command(state))
			goto outofmemory;
		state->phase = STATE_TEST;
		break;
	}
	return;

outofmemory:
	fprintf(stderr, "Out of memory\n");
	state->phase = STATE_ABORTED;
	return;
}



static int hexdigit(char c)
{
	int result = 0;
	if (isdigit(c))
		result = c - '0';
	else if (isxdigit(c))
		result = toupper(c) - 'A' + 10;
	return result;
}



static void data_handler(void *data, const XML_Char *s, int len)
{
	struct messtest_state *state = (struct messtest_state *) data;
	struct messtest_command *command = &state->current_command;
	UINT8 *bytes;
	char *str;
	int i, bytes_len, line_begin;
	int old_len;

	switch(state->phase) {
	case STATE_COMMAND:
		switch(command->command_type) {
		case MESSTEST_COMMAND_INPUT:
			str = (char *) command->u.input_chars;
			old_len = str ? strlen(str) : 0;
			str = pool_realloc(&state->pool, str, old_len + len + 1);
			if (!str)
				goto outofmemory;
			strncpyz(str + old_len, s, len + 1);
			command->u.input_chars = str;
			break;

		case MESSTEST_COMMAND_VERIFY_MEMORY:
			/* skip over '0x' prefix */
			if ((len >= 2) && (s[0] == '0') && (tolower(s[1]) == 'x'))
			{
				s += 2;
				len -= 2;
			}

			for (i = 0; (i < len) && isxdigit(s[i]); i++)
				;
			bytes_len = i / 2;

			bytes = pool_malloc(&state->pool, bytes_len);
			for (i = 0; i < bytes_len; i++)
				bytes[i] = (hexdigit(s[i*2+0]) << 4) + hexdigit(s[i*2+1]);

			command->u.verify_args.verify_data = bytes;
			command->u.verify_args.verify_data_size = bytes_len;
			break;
		}
	}
	return;

outofmemory:
	fprintf(stderr, "Out of memory\n");
	state->phase = STATE_ABORTED;
	return;
}



void messtest(const char *script_filename)
{
	struct messtest_state state;
	char buf[1024];
	FILE *in;
	int len, done;

	memset(&state, 0, sizeof(state));
	state.script_filename = script_filename;

	/* open the script file */
	in = fopen(script_filename, "r");
	if (!in)
	{
		fprintf(stderr, "%s: Cannot open file\n", script_filename);
		goto done;
	}

	/* since the cpuintrf structure is filled dynamically now, we have to init first */
	cpuintrf_init();

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);
		
		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR)
		{
			report_parseerror(&state, "%s",
				XML_ErrorString(XML_GetErrorCode(state.parser)));
			break;
		}
	}
	while(!done);

done:
	XML_ParserFree(state.parser);
}


/* ---------------------------------------------------------------------- */

extern struct rc_option fileio_opts[];

static struct rc_option opts[] =
{
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

static int handle_arg(char *arg)
{
	messtest(arg);
	return 0;
}

int main(int argc, char *argv[])
{
	struct rc_struct *rc;
	int result = -1;

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

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, handle_arg))
	{
		fprintf(stderr, "Error while parsing cmdline\n");
		goto done;
	}

	result = 0;

done:
	if (rc)
		rc_destroy(rc);
	return result;
}

