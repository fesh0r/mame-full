/*********************************************************************

	messtest.c

	MESS testing code

*********************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "messtest.h"
#include "osdepend.h"
#include "pool.h"
#include "inputx.h"

#include "expat/expat.h"

/* ----------------------------------------------------------------------- */

enum blobparse_state
{
	BLOBSTATE_INITIAL,
	BLOBSTATE_AFTER_0,
	BLOBSTATE_HEX,
	BLOBSTATE_INQUOTES
};

struct messtest_state;

struct messtest_tagdispatch
{
	const char *tag;
	void (*start_handler)(struct messtest_state *state, const XML_Char **attributes);
	void (*end_handler)(struct messtest_state *state);
	void (*data_handler)(struct messtest_state *state, const XML_Char *s, int len);
	struct messtest_tagdispatch *subdispatch;
};

struct messtest_state
{
	XML_Parser parser;
	memory_pool pool;

	struct messtest_tagdispatch *dispatch[32];
	int dispatch_pos;
	unsigned int aborted : 1;

	const char *script_filename;
	struct messtest_testcase testcase;
	int command_count;

	struct messtest_command current_command;

	enum blobparse_state blobstate;

	int test_flags;
	int test_count;
	int failure_count;
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
	state->aborted = 1;
}



static void parse_offset(const char *s, offs_t *result)
{
	if ((s[0] == '0') && (tolower(s[1]) == 'x'))
		sscanf(&s[2], "%x", result);
	else
		*result = atoi(s);
}



static mame_time parse_time(const char *s)
{
	double d = atof(s);
	return double_to_mame_time(d);
}



static void error_missingattribute(struct messtest_state *state, const char *attribute)
{
	report_parseerror(state, "Missing attribute '%s'\n", attribute);
}



static void error_outofmemory(struct messtest_state *state)
{
	report_parseerror(state, "Out of memory\n");
}



static void error_invalidmemregion(struct messtest_state *state, const char *s)
{
	report_parseerror(state, "Invalid memory region '%s'\n", s);
}



static void error_baddevicetype(struct messtest_state *state, const char *s)
{
	report_parseerror(state, "Bad device type '%s'\n", s);
}



static void wait_handler(struct messtest_state *state, const XML_Char **attributes)
{
	const char *s;

	s = find_attribute(attributes, "time");
	if (!s)
	{
		error_missingattribute(state, "time");
		return;
	}

	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_WAIT;
	state->current_command.u.wait_time = mame_time_to_double(parse_time(s));
}



static void input_handler(struct messtest_state *state, const XML_Char **attributes)
{
	/* <input> - inputs natural keyboard data into a system */
	const char *s;
	mame_time rate;

	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_INPUT;
	s = find_attribute(attributes, "rate");
	rate = s ? parse_time(s) : make_mame_time(0, 0);
	state->current_command.u.input_args.rate = rate;
}



static void rawinput_handler(struct messtest_state *state, const XML_Char **attributes)
{
	/* <rawinput> - inputs raw data into a system */
	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_RAWINPUT;
}



static void input_data_handler(struct messtest_state *state, const XML_Char *s, int len)
{
	struct messtest_command *command = &state->current_command;
	char *str;
	int old_len;

	str = (char *) command->u.input_args.input_chars;
	old_len = str ? strlen(str) : 0;
	str = pool_realloc(&state->pool, str, old_len + len + 1);
	if (!str)
	{
		error_outofmemory(state);
		return;
	}
	strncpyz(str + old_len, s, len + 1);
	command->u.input_args.input_chars = str;
}



static void switch_handler(struct messtest_state *state, const XML_Char **attributes)
{
	const char *s1;
	const char *s2;

	/* <switch> - switches a DIP switch/config setting */
	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_SWITCH;

	/* 'name' attribute */
	s1 = find_attribute(attributes, "name");
	if (!s1)
	{
		error_missingattribute(state, "name");
		return;
	}

	/* 'value' attribute */
	s2 = find_attribute(attributes, "value");
	if (!s2)
	{
		error_missingattribute(state, "value");
		return;
	}

	state->current_command.u.switch_args.name =
		pool_strdup(&state->pool, s1);
	state->current_command.u.switch_args.value =
		pool_strdup(&state->pool, s2);
}



static void screenshot_handler(struct messtest_state *state, const XML_Char **attributes)
{
	/* <screenshot> - dumps a screenshot */
	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_SCREENSHOT;
}



static void image_handler(struct messtest_state *state, const XML_Char **attributes, enum messtest_command_type command)
{
	const char *s;
	const char *s1;
	const char *s2;
	const char *s3;
	int preload, device_type;

	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = command;

	/* 'preload' attribute */
	s = find_attribute(attributes, "preload");
	preload = s ? atoi(s) : 0;
	if (preload)
		state->current_command.command_type += 2;

	/* 'filename' attribute */
	s1 = find_attribute(attributes, "filename");

	/* 'type' attribute */
	s2 = find_attribute(attributes, "type");
	if (!s2)
	{
		error_missingattribute(state, "type");
		return;
	}

	device_type = device_typeid(s2);
	if (device_type < 0)
	{
		error_baddevicetype(state, s2);
		return;
	}
	
	/* 'slot' attribute */
	s3 = find_attribute(attributes, "slot");

	state->current_command.u.image_args.filename =
		s1 ? pool_strdup(&state->pool, s1) : NULL;
	state->current_command.u.image_args.device_type = device_type;
	state->current_command.u.image_args.device_slot = s3 ? atoi(s3) : 0;
}



static void imagecreate_handler(struct messtest_state *state, const XML_Char **attributes)
{
	/* <imagecreate> - creates an image */
	image_handler(state, attributes, MESSTEST_COMMAND_IMAGE_CREATE);
}



static void imageload_handler(struct messtest_state *state, const XML_Char **attributes)
{
	/* <imageload> - loads an image */
	image_handler(state, attributes, MESSTEST_COMMAND_IMAGE_LOAD);
}



static void memverify_handler(struct messtest_state *state, const XML_Char **attributes)
{
	const char *s1;
	const char *s2;
	const char *s3;
	int region;

	/* <memverify> - verifies that a range of memory contains specific data */
	s1 = find_attribute(attributes, "start");
	if (!s1)
	{
		error_missingattribute(state, "start");
		return;
	}

	s2 = find_attribute(attributes, "end");
	if (!s2)
		s2 = "0";

	s3 = find_attribute(attributes, "region");

	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_VERIFY_MEMORY;
	state->blobstate = BLOBSTATE_INITIAL;
	parse_offset(s1, &state->current_command.u.verify_args.start);
	parse_offset(s2, &state->current_command.u.verify_args.end);

	if (s3)
	{
		region = memory_region_from_string(s3);
		if (region == REGION_INVALID)
			error_invalidmemregion(state, s3);
		state->current_command.u.verify_args.mem_region = region;
	}
}



static void get_blob(struct messtest_state *state, const XML_Char *s, int len,
	 void **blob, size_t *blob_len)
{
	UINT8 *bytes = NULL;
	int bytes_len = 0;
	int i = 0;
	int j, k;

	while(i < len)
	{
		switch(state->blobstate) {
		case BLOBSTATE_INITIAL:
			if (isspace(s[i]))
			{
				/* ignore whitespace */
				i++;
			}
			else if (s[i] == '0')
			{
				state->blobstate = BLOBSTATE_AFTER_0;
				i++;
			}
			else if (s[i] == '\"')
			{
				state->blobstate = BLOBSTATE_INQUOTES;
				i++;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_AFTER_0:
			if (tolower(s[i]) == 'x')
			{
				state->blobstate = BLOBSTATE_HEX;
				i++;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_HEX:
			if (isspace(s[i]))
			{
				state->blobstate = BLOBSTATE_INITIAL;
				i++;
			}
			else if (isxdigit(s[i]))
			{
				/* count the number of hex digits available */
				for (j = i; (j < len) && isxdigit(s[j+0]) && isxdigit(s[j+1]); j += 2)
					;
				bytes_len = (j - i) / 2;
				
				/* allocate the memory */
				bytes = pool_realloc(&state->pool, *blob, *blob_len + bytes_len);
				if (!bytes)
					goto outofmemory;

				/* build the byte array */
				for (k = 0; k < bytes_len; k++)
				{
					bytes[k + *blob_len] =
						(hexdigit(s[i+k*2+0]) << 4) + hexdigit(s[i+k*2+1]);
				}

				i = j;
				*blob = bytes;
				*blob_len += bytes_len;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_INQUOTES:
			if (s[i] == '\"')
			{
				state->blobstate = BLOBSTATE_INITIAL;
				i++;
			}
			else
			{
				/* count the number of quoted chars available */
				for (j = i; (j < len) && (s[j] != '\"'); j++)
					;
				bytes_len = j - i;
				
				/* allocate the memory */
				bytes = pool_realloc(&state->pool, *blob, *blob_len + bytes_len);
				if (!bytes)
					goto outofmemory;

				/* build the byte array */
				memcpy(&bytes[*blob_len], &s[i], bytes_len);

				i = j;
				*blob = bytes;
				*blob_len += bytes_len;
			}
			break;
		}
	}
	return;

outofmemory:
	error_outofmemory(state);
	return;

parseerror:
	error_outofmemory(state);
	return;
}



static void memverify_data_handler(struct messtest_state *state, const XML_Char *s, int len)
{
	struct messtest_command *command = &state->current_command;

	get_blob(state, s, len,
		(void **) &command->u.verify_args.verify_data,
		&command->u.verify_args.verify_data_size);
}



static void test_start_handler(struct messtest_state *state, const XML_Char **attributes)
{
	const char *s;

	memset(&state->testcase, 0, sizeof(state->testcase));

	/* 'driver' attribute */
	s = find_attribute(attributes, "driver");
	if (!s)
	{
		error_missingattribute(state, "driver");
		return;
	}
	state->testcase.driver = pool_strdup(&state->pool, s);
	if (!state->testcase.driver)
	{
		error_outofmemory(state);
		return;
	}

	/* 'name' attribute */
	s = find_attribute(attributes, "name");
	if (s)
	{
		state->testcase.name = pool_strdup(&state->pool, s);
		if (!state->testcase.name)
		{
			error_outofmemory(state);
			return;
		}
	}
	else
	{
		state->testcase.name = state->testcase.driver;
	}

	/* 'ramsize' attribute */
	s = find_attribute(attributes, "ramsize");
	state->testcase.ram = s ? ram_parse_string(s) : 0;

	state->testcase.commands = NULL;
	state->command_count = 0;
}



static void command_end_handler(struct messtest_state *state)
{
	if (!append_command(state))
	{
		error_outofmemory(state);
		return;
	}
}



static void test_end_handler(struct messtest_state *state)
{
	memset(&state->current_command, 0, sizeof(state->current_command));
	state->current_command.command_type = MESSTEST_COMMAND_END;
	if (!append_command(state))
	{
		error_outofmemory(state);
		return;
	}

	if (run_test(&state->testcase, state->test_flags, NULL))
		state->failure_count++;
	state->test_count++;
}



static struct messtest_tagdispatch test_dispatch[] =
{
	{ "wait",			wait_handler,			command_end_handler },
	{ "input",			input_handler,			command_end_handler, input_data_handler },
	{ "rawinput",		rawinput_handler,		command_end_handler, input_data_handler },
	{ "switch",			switch_handler,			command_end_handler },
	{ "screenshot",		screenshot_handler,		command_end_handler },
	{ "imagecreate",	imagecreate_handler,	command_end_handler },
	{ "imageload",		imageload_handler,		command_end_handler },
	{ "memverify",		memverify_handler,		command_end_handler, memverify_data_handler },
	{ NULL }
};



static struct messtest_tagdispatch root_dispatch[] =
{
	{ "tests",			NULL, NULL, NULL, root_dispatch },
	{ "test",			test_start_handler, test_end_handler, NULL, test_dispatch },
	{ NULL }
};



static struct messtest_tagdispatch initial_dispatch = { NULL, NULL, NULL, NULL, root_dispatch };



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
	struct messtest_state *state = (struct messtest_state *) data;
	struct messtest_tagdispatch *dispatch;

	/* try to find the tag */
	dispatch = state->dispatch[state->dispatch_pos]->subdispatch;
	if (dispatch)
	{
		while(dispatch->tag)
		{
			if (!strcmp(tagname, dispatch->tag))
				break;
			dispatch++;
		}
		if (!dispatch->tag)
		{
			report_parseerror(state, "Unknown tag '%s'\n", tagname);
			return;
		}

		if (!state->aborted && dispatch->start_handler)
			dispatch->start_handler(state, attributes);
	}

	state->dispatch[++state->dispatch_pos] = dispatch;
}



static void end_handler(void *data, const XML_Char *name)
{
	struct messtest_state *state = (struct messtest_state *) data;
	struct messtest_tagdispatch *dispatch;

	dispatch = state->dispatch[state->dispatch_pos];
	if (!state->aborted && dispatch && dispatch->end_handler)
		dispatch->end_handler(state);

	if (state->dispatch_pos > 0)
		state->dispatch_pos--;
}



static void data_handler(void *data, const XML_Char *s, int len)
{
	struct messtest_state *state = (struct messtest_state *) data;
	int dispatch_pos;

	dispatch_pos = state->dispatch_pos;
	while((dispatch_pos > 0) && !state->dispatch[state->dispatch_pos])
		dispatch_pos--;

	if (state->dispatch[dispatch_pos]->data_handler)
		state->dispatch[dispatch_pos]->data_handler(data, s, len);
}



/* this external entity handler allows us to do things like this:
 *
 *	<!DOCTYPE tests
 *	[
 *		<!ENTITY mamekey_esc SYSTEM "http://www.mess.org/messtest/">
 *	]>
 */
static int external_entity_handler(XML_Parser parser,
	const XML_Char *context,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId)
{
	XML_Parser extparser = NULL;
	int rc = 0, i;
	char buf[256];
	static const char *mamekey_prefix = "mamekey_";
	input_code_t c;

	buf[0] = '\0';

	/* only supportr our own schema */
	if (strcmp(systemId, "http://www.mess.org/messtest/"))
		goto done;

	extparser = XML_ExternalEntityParserCreate(parser, context, "us-ascii");
	if (!extparser)
		goto done;

	/* does this use the 'mamekey' prefix? */
	if ((strlen(context) > strlen(mamekey_prefix)) && !memcmp(context,
		mamekey_prefix, strlen(mamekey_prefix)))
	{
		context += strlen(mamekey_prefix);
		c = 0;

		/* this is interim until we can come up with a real solution */
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "KEYCODE_%s", context);
		for (i = 0; buf[i]; i++)
			buf[i] = toupper(buf[i]);

		code_init();
		c = token_to_code(buf);
		code_close();

		if (c != CODE_NONE)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "<%s%s>&#%d;</%s%s>",
				mamekey_prefix, context,
				UCHAR_MAMEKEY_BEGIN + c,
				mamekey_prefix, context);

			if (XML_Parse(extparser, buf, strlen(buf), 0) == XML_STATUS_ERROR)
				goto done;
		}
	}

	if (XML_Parse(extparser, NULL, 0, 1) == XML_STATUS_ERROR)
		goto done;

	rc = 1;
done:
	if (extparser)
		XML_ParserFree(extparser);
	return rc;
}



int messtest(const char *script_filename, int flags, int *test_count, int *failure_count)
{
	struct messtest_state state;
	char buf[1024];
	char saved_directory[1024];
	FILE *in;
	int len, done;
	int result = -1;
	char *script_directory;

	memset(&state, 0, sizeof(state));
	state.test_flags = flags;
	state.script_filename = script_filename;
	state.dispatch[0] = &initial_dispatch;

	/* open the script file */
	in = fopen(script_filename, "r");
	if (!in)
	{
		fprintf(stderr, "%s: Cannot open file\n", script_filename);
		goto done;
	}

	/* save the current working directory, and change to the test directory */
	script_directory = osd_dirname(script_filename);
	if (script_directory)
	{
		osd_getcurdir(saved_directory, sizeof(saved_directory) / sizeof(saved_directory[0]));
		osd_setcurdir(script_directory);
		free(script_directory);
	}
	else
	{
		saved_directory[0] = '\0';
	}

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);
	XML_SetExternalEntityRefHandler(state.parser, external_entity_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);
		
		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR)
		{
			report_parseerror(&state, "%s",
				XML_ErrorString(XML_GetErrorCode(state.parser)));
			goto done;
		}
	}
	while(!done);

	result = 0;

done:
	/* restore the directory */
	if (saved_directory[0])
		osd_setcurdir(saved_directory);

	/* dispose of the parser */
	if (state.parser)
		XML_ParserFree(state.parser);

	/* write out test and failure counts */
	if (test_count)
		*test_count = state.test_count;
	if (failure_count)
		*failure_count = state.failure_count;
	return result;
}

