#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "imgtool.h"
#include "osdutils.h"

struct basictokens {
	UINT16 baseaddress;
	const char **statements;
	int num_statements;
	const char **functions;
	int num_functions;
};

struct filter_basic_state
{
	int position;
	int done;
	int linesize;
	char linebuffer[300];
};

static int sendstring(struct filter_info *fi, const char *s)
{
	int buflen;
	buflen = strlen(s);
	return fi->sendproc(fi, (void *) s, buflen);
}

static int CLIB_DECL sendstringf(struct filter_info *fi, const char *fmt, ...)
{
	va_list va;
	char buffer[256];

	va_start(va, fmt);
	vsprintf(buffer, fmt, va);
	va_end(va);

	return sendstring(fi, buffer);
}

#define filter_basic_write NULL
/*
static int filter_basic_write(const struct basictokens *tokens, struct filter_basic_state *state,
	int (*sendproc)(struct filter_info *fi, void *buf, int buflen), char *buf, int buflen)
{
}
*/

static const char *lookupkeyword(int val, const char *keywords[], int num_keywords)
{
	const char *keyword;

	assert(val & 0x80);

	val &= 0x7f;

	if (val >= num_keywords)
		keyword = NULL;
	else
		keyword = keywords[val];

	return keyword;
}

static void sendline(struct filter_info *fi, char *line, int len)
{
	int textlen;
	int val;
	const char *keyword;
	const struct basictokens *tokens = (const struct basictokens *) fi->filterparam;

	while(len > 0) {
		for (textlen = 0; (textlen < len) && ((line[textlen] & 0x80) == 0); textlen++)
			;
		if (textlen > 0) {
			fi->sendproc(fi, line, textlen);
			line += textlen;
			len -= textlen;
		}
		if (len > 0) {
			/* We are at a keyword */
			val = *line;
			line++;
			len--;
			assert(val & 0x80);
			if (val == 0xff) {
				/* A function */
				if (len > 0) {
					val = *line;
					line++;
					len--;
					keyword = lookupkeyword(val, tokens->functions, tokens->num_functions);
				}
				else {
					/* A weird case */
					keyword = NULL;
				}
			}
			else {
				/* A statement */
				keyword = lookupkeyword(val, tokens->statements, tokens->num_statements);
			}

			/* IIRC, when an unknown token was reached, a ! would be displayed */
			if (!keyword)
				keyword = "!";
			sendstring(fi, keyword);
		}
	}
	sendstring(fi, EOLN);
}

static int filter_basic_read(struct filter_info *fi, void *buf, int buflen)
{
	char b;
	int result = 0;
	int sizetoskip;
	char *bufc = (char *) buf;
	unsigned int nextaddr;
	unsigned int linenumber;
	const struct basictokens *tokens = (const struct basictokens *) fi->filterparam;
	struct filter_basic_state *state = (struct filter_basic_state *) fi->filterstate;

	/* Skip first three bytes */
	if (state->position < 3) {
		sizetoskip = MIN(3 - state->position, buflen);
		bufc += sizetoskip;
		buflen -= sizetoskip;
		result += sizetoskip;
	}

	while(buflen > 0) {
		b = *(bufc++);
		buflen--;

		/* Did we just terminate a line? */
		if ((state->linesize >= 4) && (b == 0)) {
			/* We have a full line! */
			nextaddr = ((unsigned int) (UINT8) state->linebuffer[0]) * 256 + ((unsigned int) (UINT8) state->linebuffer[1]);
			linenumber = ((unsigned int) (UINT8) state->linebuffer[2]) * 256 + ((unsigned int) (UINT8) state->linebuffer[3]);

			if (nextaddr == 0)
				state->done = 1;

			if (!state->done) {
				sendstringf(fi, "%i ", (int) linenumber);
				sendline(fi, state->linebuffer + 4, state->linesize - 4);
			}
			state->linesize = 0;
		}
		else {
			/* Put it into the line buffer (unless the impossible happens and the line is too large for the buffer) */
			if ((state->linesize+1) < (sizeof(state->linebuffer) / sizeof(state->linebuffer[0])))
				state->linebuffer[state->linesize++] = b;
		}
	}

	state->position += result;
	return result;
}

/* ----------------------------------------------------------------------- */

static const char *cocobas_statements[] =
{
	"FOR",		/* 0x80 */
	"GO",		/* 0x81 */
	"REM",		/* 0x82 */
	"'",		/* 0x83 */
	"ELSE",		/* 0x84 */
	"IF",		/* 0x85 */
	"DATA",		/* 0x86 */
	"PRINT",	/* 0x87 */
	"ON",		/* 0x88 */
	"INPUT",	/* 0x89 */
	"END",		/* 0x8a */
	"NEXT",		/* 0x8b */
	"DIM",		/* 0x8c */
	"READ",		/* 0x8d */
	"RUN",		/* 0x8e */
	"RESTORE",	/* 0x8f */
	"RETURN",	/* 0x90 */
	"STOP",		/* 0x91 */
	"POKE",		/* 0x92 */
	"CONT",		/* 0x93 */
	"LIST",		/* 0x94 */
	"CLEAR",	/* 0x95 */
	"NEW",		/* 0x96 */
	"CLOAD",	/* 0x97 */
	"CSAVE",	/* 0x98 */
	"OPEN",		/* 0x99 */
	"CLOSE",	/* 0x9a */
	"LLIST",	/* 0x9b */
	"SET",		/* 0x9c */
	"RESET",	/* 0x9d */
	"CLS",		/* 0x9e */
	"MOTOR",	/* 0x9f */
	"SOUND",	/* 0xa0 */
	"AUDIO",	/* 0xa1 */
	"EXEC",		/* 0xa2 */
	"SKIPF",	/* 0xa3 */
	"TAB(",		/* 0xa4 */
	"TO",		/* 0xa5 */
	"SUB",		/* 0xa6 */
	"THEN",		/* 0xa7 */
	"NOT",		/* 0xa8 */
	"STEP",		/* 0xa9 */
	"OFF",		/* 0xaa */
	"+",		/* 0xab */
	"-",		/* 0xac */
	"*",		/* 0xad */
	"/",		/* 0xae */
	"^",		/* 0xaf */
	"AND",		/* 0xb0 */
	"OR",		/* 0xb1 */
	">",		/* 0xb2 */
	"=",		/* 0xb3 */
	"<",		/* 0xb4 */
	"DEL",		/* 0xb5 */
	"EDIT",		/* 0xb6 */
	"TRON",		/* 0xb7 */
	"TROFF",	/* 0xb8 */
	"DEF",		/* 0xb9 */
	"LET",		/* 0xba */
	"LINE",		/* 0xbb */
	"PCLS",		/* 0xbc */
	"PSET",		/* 0xbd */
	"PRESET",	/* 0xbe */
	"SCREEN",	/* 0xbf */
	"PCLEAR",	/* 0xc0 */
	"COLOR",	/* 0xc1 */
	"CIRCLE",	/* 0xc2 */
	"PAINT",	/* 0xc3 */
	"GET",		/* 0xc4 */
	"PUT",		/* 0xc5 */
	"DRAW",		/* 0xc6 */
	"PCOPY",	/* 0xc7 */
	"PMODE",	/* 0xc8 */
	"PLAY",		/* 0xc9 */
	"DLOAD",	/* 0xca */
	"RENUM",	/* 0xcb */
	"FN",		/* 0xcc */
	"USING",	/* 0xcd */
	"DIR",		/* 0xce */
	"DRIVE",	/* 0xcf */
	"FIELD",	/* 0xd0 */
	"FILES",	/* 0xd1 */
	"KILL",		/* 0xd2 */
	"LOAD",		/* 0xd3 */
	"LSET",		/* 0xd4 */
	"MERGE",	/* 0xd5 */
	"RENAME",	/* 0xd6 */
	"RSET",		/* 0xd7 */
	"SAVE",		/* 0xd8 */
	"WRITE",	/* 0xd9 */
	"VERIFY",	/* 0xda */
	"UNLOAD",	/* 0xdb */
	"DSKINI",	/* 0xdc */
	"BACKUP",	/* 0xdd */
	"COPY",		/* 0xde */
	"DSKI$",	/* 0xdf */
	"DSKO$"		/* 0xe0 */
	"DOS",		/* 0xe1 */
	"WIDTH",	/* 0xe2 */
	"PALETTE",	/* 0xe3 */
	"HSCREEN",	/* 0xe4 */
	"LPOKE",	/* 0xe5 */
	"HCLS",		/* 0xe6 */
	"HCOLOR",	/* 0xe7 */
	"HPAINT",	/* 0xe8 */
	"HCIRCLE",	/* 0xe9 */
	"HLINE",	/* 0xea */
	"HGET",		/* 0xeb */
	"HPUT",		/* 0xec */
	"HBUFF",	/* 0xed */
	"HPRINT",	/* 0xee */
	"ERR",		/* 0xef */
	"BRK",		/* 0xf0 */
	"LOCATE",	/* 0xf1 */
	"HSTAT",	/* 0xf2 */
	"HSET",		/* 0xf3 */
	"HRESET",	/* 0xf4 */
	"HDRAW",	/* 0xf5 */
	"CMP",		/* 0xf6 */
	"RGB",		/* 0xf7 */
	"ATTR"		/* 0xf8 */
};

static const char *cocobas_functions[] =
{
	"SGN",		/* 0xff80 */
	"INT",		/* 0xff81 */
	"ABS",		/* 0xff82 */
	"USR"		/* 0xff83 */
	"RND",		/* 0xff84 */
	"SIN",		/* 0xff85 */
	"PEEK",		/* 0xff86 */
	"LEN",		/* 0xff87 */
	"STR$",		/* 0xff88 */
	"VAL",		/* 0xff89 */
	"ASC",		/* 0xff8a */
	"CHR$",		/* 0xff8b */
	"EOF",		/* 0xff8c */
	"JOYSTK",	/* 0xff8d */
	"LEFT$",	/* 0xff8e */
	"RIGHT$",	/* 0xff8f */
	"MID$",		/* 0xff90 */
	"POINT",	/* 0xff91 */
	"INKEY$",	/* 0xff92 */
	"MEM",		/* 0xff93 */
	"ATN",		/* 0xff94 */
	"COS",		/* 0xff95 */
	"TAN",		/* 0xff96 */
	"EXP",		/* 0xff97 */
	"FIX",		/* 0xff98 */
	"LOG",		/* 0xff99 */
	"POS",		/* 0xff9a */
	"SQR",		/* 0xff9b */
	"HEX$",		/* 0xff9c */
	"VARPTR",	/* 0xff9d */
	"INSTR",	/* 0xff9e */
	"TIMER",	/* 0xff9f */
	"PPOINT",	/* 0xffa0 */
	"STRING$",	/* 0xffa1 */
	"CVN",		/* 0xffa2 */
	"FREE",		/* 0xffa3 */
	"LOC",		/* 0xffa4 */
	"LOF",		/* 0xffa5 */
	"MKN$",		/* 0xffa6 */
	"AS",		/* 0xffa7 */
	"LPEEK",	/* 0xffa8 */
	"BUTTON",	/* 0xffa9 */
	"HPOINT",	/* 0xffaa */
	"ERNO",		/* 0xffab */
	"ERLIN"		/* 0xffac */
};

static const char *dragonbas_statements[] =
{
	"FOR",		/* 0x80 */
	"GO",		/* 0x81 */
	"REM",		/* 0x82 */
	"'",		/* 0x83 */
	"ELSE",		/* 0x84 */
	"IF",		/* 0x85 */
	"DATA",		/* 0x86 */
	"PRINT",	/* 0x87 */
	"ON",		/* 0x88 */
	"INPUT",	/* 0x89 */
	"END",		/* 0x8a */
	"NEXT",		/* 0x8b */
	"DIM",		/* 0x8c */
	"READ",		/* 0x8d */
	"LET",		/* 0x8e */
	"RUN",		/* 0x8f */
	"RESTORE",	/* 0x90 */
	"RETURN",	/* 0x91 */
	"STOP",		/* 0x92 */
	"POKE",		/* 0x93 */
	"CONT",		/* 0x94 */
	"LIST",		/* 0x95 */
	"CLEAR",	/* 0x96 */
	"NEW",		/* 0x97 */
	"DEF",		/* 0x98 */
	"CLOAD",	/* 0x99 */
	"CSAVE",	/* 0x9a */
	"OPEN",		/* 0x9b */
	"CLOSE",	/* 0x9c */
	"LLIST",	/* 0x9d */
	"SET",		/* 0x9e */
	"RESET",	/* 0x9f */
	"CLS",		/* 0xa0 */
	"MOTOR",	/* 0xa1 */
	"SOUND",	/* 0xa2 */
	"AUDIO",	/* 0xa3 */
	"EXEC",		/* 0xa4 */
	"SKIPF",	/* 0xa5 */
	"DEL",		/* 0xa6 */
	"EDIT",		/* 0xa7 */
	"TRON",		/* 0xa8 */
	"TROFF",	/* 0xa9 */
	"LINE",		/* 0xaa */
	"PCLS",		/* 0xab */
	"PSET",		/* 0xac */
	"PRESET",	/* 0xad */
	"SCREEN",	/* 0xae */
	"PCLEAR",	/* 0xaf */
	"COLOR",	/* 0xb0 */
	"CIRCLE",	/* 0xb1 */
	"PAINT",	/* 0xb2 */
	"GET",		/* 0xb3 */
	"PUT",		/* 0xb4 */
	"DRAW",		/* 0xb5 */
	"PCOPY",	/* 0xb6 */
	"PMODE",	/* 0xb7 */
	"PLAY",		/* 0xb8 */
	"DLOAD",	/* 0xb9 */
	"RENUM",	/* 0xba */
	"TAB(",		/* 0xbb */
	"TO",		/* 0xbc */
	"SUB",		/* 0xbd */
	"FN",		/* 0xbe */
	"THEN",		/* 0xbf */
	"NOT",		/* 0xc0 */
	"STEP",		/* 0xc1 */
	"OFF",		/* 0xc2 */
	"+",		/* 0xc3 */
	"-",		/* 0xc4 */
	"*",		/* 0xc5 */
	"/",		/* 0xc6 */
	"^",		/* 0xc7 */
	"AND",		/* 0xc8 */
	"OR",		/* 0xc9 */
	">",		/* 0xca */
	"=",		/* 0xcb */
	"<",		/* 0xcc */
	"USING",	/* 0xcd */
	"DIR",		/* 0xce */
	"DRIVE",	/* 0xcf */
	"FIELD",	/* 0xd0 */
	"FILES",	/* 0xd1 */
	"KILL",		/* 0xd2 */
	"LOAD",		/* 0xd3 */
	"LSET",		/* 0xd4 */
	"MERGE",	/* 0xd5 */
	"RENAME",	/* 0xd6 */
	"RSET",		/* 0xd7 */
	"SAVE",		/* 0xd8 */
	"WRITE",	/* 0xd9 */
	"VERIFY",	/* 0xda */
	"UNLOAD",	/* 0xdb */
	"DSKINI",	/* 0xdc */
	"BACKUP",	/* 0xdd */
	"COPY",		/* 0xde */
	"DSKI$",	/* 0xdf */
	"DSKO$"		/* 0xe0 */
};

static const char *dragonbas_functions[] =
{
	"SGN",		/* 0xff80 */
	"INT",		/* 0xff81 */
	"ABS",		/* 0xff82 */
	"POS",		/* 0xff83 */
	"RND",		/* 0xff84 */
	"SQR",		/* 0xff85 */
	"LOG",		/* 0xff86 */
	"EXP",		/* 0xff87 */
	"SIN",		/* 0xff88 */
	"COS",		/* 0xff89 */
	"TAN",		/* 0xff8a */
	"ATN",		/* 0xff8b */
	"PEEK",		/* 0xff8c */
	"LEN",		/* 0xff8d */
	"STR$",		/* 0xff8e */
	"VAL",		/* 0xff8f */
	"ASC",		/* 0xff90 */
	"CHR$",		/* 0xff91 */
	"EOF",		/* 0xff92 */
	"JOYSTK",	/* 0xff93 */
	"FIX",		/* 0xff94 */
	"HEX$",		/* 0xff95 */
	"LEFT$",	/* 0xff96 */
	"RIGHT$",	/* 0xff97 */
	"MID$",		/* 0xff98 */
	"POINT",	/* 0xff99 */
	"INKEY$",	/* 0xff9a */
	"MEM",		/* 0xff9b */
	"VARPTR",	/* 0xff9c */
	"INSTR",	/* 0xff9d */
	"TIMER",	/* 0xff9e */
	"PPOINT",	/* 0xff9f */
	"STRING$",	/* 0xffa0 */
	"USR"		/* 0xffa1 */
	"CVN",		/* 0xffa2 */
	"FREE",		/* 0xffa3 */
	"LOC",		/* 0xffa4 */
	"LOF",		/* 0xffa5 */
	"MKN$"		/* 0xffa6 */
};

/*
		int8     $ff
		int16    <TOTAL LENGTH>
		...
        int16    <PTR_NEXT_LINE>
        int16    <LINE_NUM>
        int8[]   <TOKENISED_DATA>
        int8     $00     End of line delimiter
 */

static void *filter_cocobas_calcparam(const struct ImageModule *imgmod)
{
	static const struct basictokens cocobas_tokens = {
		0x2600,
		cocobas_statements,	sizeof(cocobas_statements) / sizeof(cocobas_statements[0]),
		cocobas_functions,	sizeof(cocobas_functions) / sizeof(cocobas_functions[0]),
	};

	return (void *) &cocobas_tokens;
}

static void *filter_dragonbas_calcparam(const struct ImageModule *imgmod)
{
	static const struct basictokens cocobas_tokens = {
		0x2600,
		dragonbas_statements,	sizeof(dragonbas_statements) / sizeof(dragonbas_statements[0]),
		dragonbas_functions,	sizeof(dragonbas_functions) / sizeof(dragonbas_functions[0]),
	};

	return (void *) &cocobas_tokens;
}

struct filter_module filter_cocobas =
{
	"cocobas",
	"CoCo Tokenized Basic Files",
	filter_cocobas_calcparam,
	filter_cocobas_calcparam,
	filter_basic_read,
	filter_basic_write,
	sizeof(struct filter_basic_state)
};

struct filter_module filter_dragonbas =
{
	"dragonbas",
	"Dragon Tokenized Basic Files",
	filter_dragonbas_calcparam,
	filter_dragonbas_calcparam,
	filter_basic_read,
	filter_basic_write,
	sizeof(struct filter_basic_state)
};