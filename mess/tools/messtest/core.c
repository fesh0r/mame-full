/*********************************************************************

	core.c

	MESS testing code

*********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <expat.h>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include "strconv.h"
#endif /* WIN32 */

#include "core.h"
#include "testmess.h"
#include "testimgt.h"
#include "osdepend.h"
#include "pool.h"
#include "pile.h"
#include "inputx.h"

/* ----------------------------------------------------------------------- */

enum blobparse_state
{
	BLOBSTATE_INITIAL,
	BLOBSTATE_AFTER_0,
	BLOBSTATE_HEX,
	BLOBSTATE_SINGLEQUOTES,
	BLOBSTATE_DOUBLEQUOTES
};

struct messtest_state
{
	XML_Parser parser;
	memory_pool pool;

	const struct messtest_tagdispatch *dispatch[32];
	int dispatch_pos;
	unsigned int aborted : 1;

	const char *script_filename;
	char *current_testcase_name;

	int test_count;
	int failure_count;

	mess_pile blobpile;
	enum blobparse_state blobstate;
};

static struct messtest_state *state;



void error_report(const char *message)
{
	fprintf(stderr, "%s:(%i:%i): %s\n",
		state->script_filename,
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		message);
	state->aborted = 1;
}



void error_reportf(const char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	error_report(buf);
}



void error_missingattribute(const char *attribute)
{
	error_reportf("Missing attribute '%s'\n", attribute);
}



void error_outofmemory(void)
{
	error_reportf("Out of memory\n");
}



void error_invalidmemregion(const char *s)
{
	error_reportf("Invalid memory region '%s'\n", s);
}



void error_baddevicetype(const char *s)
{
	error_reportf("Bad device type '%s'\n", s);
}



static struct messtest_tagdispatch root_dispatch[] =
{
	{ "tests",			DATA_NONE, NULL, NULL, root_dispatch },
	{ "test",			DATA_NONE, testmess_start_handler, testmess_end_handler, testmess_dispatch },
	{ "imgtooltest",	DATA_NONE, testimgtool_start_handler, testimgtool_end_handler, testimgtool_dispatch },
	{ NULL }
};



static const struct messtest_tagdispatch initial_dispatch = { NULL, DATA_NONE, NULL, NULL, root_dispatch };



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
//	struct messtest_state *state = (struct messtest_state *) data;
	const struct messtest_tagdispatch *dispatch;

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
			error_reportf("Unknown tag '%s'\n", tagname);
			return;
		}

		if (!state->aborted && dispatch->start_handler)
			dispatch->start_handler(attributes);
	}

	state->dispatch[++state->dispatch_pos] = dispatch;
	if (dispatch && dispatch->datatype)
		pile_clear(&state->blobpile);
	state->blobstate = BLOBSTATE_INITIAL;
}



static void end_handler(void *data, const XML_Char *name)
{
//	struct messtest_state *state = (struct messtest_state *) data;
	const struct messtest_tagdispatch *dispatch;
	void *ptr;
	size_t size;

	dispatch = state->dispatch[state->dispatch_pos];
	if (!state->aborted && dispatch && dispatch->end_handler)
	{
		if (dispatch->datatype == DATA_TEXT)
			pile_putc(&state->blobpile, '\0');
		ptr = pile_getptr(&state->blobpile);
		size = pile_size(&state->blobpile);
		dispatch->end_handler(ptr, size);
	}

	if (state->dispatch_pos > 0)
		state->dispatch_pos--;
}



static void data_handler_text(const XML_Char *s, int len)
{
	int i;
	for (i = 0; i < len; i++)
		pile_putc(&state->blobpile, s[i]);
}



static void data_handler_binary(const XML_Char *s, int len)
{
	int i = 0;
	int found;
	char c;

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
			else if (s[i] == '\'')
			{
				state->blobstate = BLOBSTATE_SINGLEQUOTES;
				i++;
			}
			else if (s[i] == '\"')
			{
				state->blobstate = BLOBSTATE_DOUBLEQUOTES;
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
			else
			{
				found = FALSE;
				while(((i + 2) <= len) && isxdigit(s[i]) && isxdigit(s[i+1]))
				{
					c = (hexdigit(s[i]) << 4) | hexdigit(s[i+1]);
					pile_putc(&state->blobpile, c);
					i += 2;
					found = TRUE;
				}
				if (!found)
					goto parseerror;
			}
			break;

		case BLOBSTATE_SINGLEQUOTES:
		case BLOBSTATE_DOUBLEQUOTES:
			if (s[i] == (state->blobstate == BLOBSTATE_SINGLEQUOTES ? '\'' : '\"'))
			{
				state->blobstate = BLOBSTATE_INITIAL;
				i++;
			}
			else
			{
				while(((i + 1) <= len) && (s[i] != '\"'))
				{
					pile_putc(&state->blobpile, s[i++]);
				}
			}
			break;
		}
	}
	return;

parseerror:
	error_report("Parse Error");
	return;
}



static void data_handler(void *data, const XML_Char *s, int len)
{
//	struct messtest_state *state = (struct messtest_state *) data;
	int dispatch_pos;

	dispatch_pos = state->dispatch_pos;
	while((dispatch_pos > 0) && !state->dispatch[state->dispatch_pos])
		dispatch_pos--;

	switch(state->dispatch[dispatch_pos]->datatype)
	{
		case DATA_NONE:
			break;

		case DATA_TEXT:
			data_handler_text(s, len);
			break;

		case DATA_BINARY:
			data_handler_binary(s, len);
			break;
	}
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



int messtest(const struct messtest_options *opts, int *test_count, int *failure_count)
{
	struct messtest_state the_state;
	char buf[1024];
	char saved_directory[1024];
	FILE *in;
	int len, done;
	int result = -1;
	char *script_directory;

	state = &the_state;

	memset(state, 0, sizeof(*state));
	state->script_filename = opts->script_filename;
	state->dispatch[0] = &initial_dispatch;
	pile_init(&state->blobpile);

	/* open the script file */
	in = fopen(state->script_filename, "r");
	if (!in)
	{
		fprintf(stderr, "%s: Cannot open file\n", state->script_filename);
		goto done;
	}

	/* save the current working directory, and change to the test directory */
	saved_directory[0] = '\0';
	if (!opts->preserve_directory)
	{
		script_directory = osd_dirname(state->script_filename);
		if (script_directory)
		{
			osd_getcurdir(saved_directory, sizeof(saved_directory) / sizeof(saved_directory[0]));
			osd_setcurdir(script_directory);
			free(script_directory);
		}
	}

	state->parser = XML_ParserCreate(NULL);
	if (!state->parser)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	XML_SetUserData(state->parser, &state);
	XML_SetElementHandler(state->parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state->parser, data_handler);
	XML_SetExternalEntityRefHandler(state->parser, external_entity_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);
		
		if (XML_Parse(state->parser, buf, len, done) == XML_STATUS_ERROR)
		{
			error_reportf("%s", XML_ErrorString(XML_GetErrorCode(state->parser)));
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
	if (state->parser)
		XML_ParserFree(state->parser);

	/* write out test and failure counts */
	if (test_count)
		*test_count = state->test_count;
	if (failure_count)
		*failure_count = state->failure_count;
	pile_delete(&state->blobpile);
	return result;
}



void report_message(messtest_messagetype_t msgtype, const char *fmt, ...)
{
	char buf[1024];
	va_list va;
	const char *prefix1;
	const char *prefix2;
	int width = 64;
	int last_space, base, i;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	prefix1 = state->current_testcase_name;
	prefix2 = msgtype ? "***" : "...";

	base = 0;
	last_space = -1;
	i = 0;
	do
	{
		if ((buf[i] == '\0') || (i - base > width))
		{
			if (buf[i] && (last_space > 0))
				buf[last_space] = '\0';
			else
				last_space = i;
			printf("%-12s %-3s %s\n", prefix1, prefix2, &buf[base]);

			base = last_space + 1;
			last_space = -1;
			prefix1 = "";
			prefix2 = "";
		}
		else if (isspace(buf[i]))
		{
			last_space = i;
		}
	}
	while(buf[i++]);

	/* did we abort? */
/*	if ((msgtype == MSG_FAILURE) && (state != STATE_ABORTED))
	{
		state = STATE_ABORTED;
		final_time = timer_get_time();
		if (final_time > 0.0)
			dump_screenshot(); 
	}
*/
}



void report_testcase_begin(const char *testcase_name)
{
	state->current_testcase_name = strdup(testcase_name);
	if (!state->current_testcase_name)
		error_outofmemory();
}



void report_testcase_ran(int failure)
{
	state->test_count++;
	if (failure)
		state->failure_count++;
}



void make_filename_temporary(char *filename, size_t buflen)
{
#ifdef WIN32
	TCHAR tempbuf[MAX_PATH];

	GetTempPath(sizeof(tempbuf) / sizeof(tempbuf[0]), tempbuf);
	_tcscat(tempbuf, U2T(filename));
	DeleteFile(tempbuf);

	snprintf(filename, buflen, "%s", T2U(tempbuf));
#endif /* WIN32 */
}






