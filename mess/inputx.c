#include <ctype.h>
#include <assert.h>
#include <wctype.h>

#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#define NUM_CODES		128
#define NUM_SIMUL_KEYS	2

#ifdef MAME_DEBUG
#define LOG_INPUTX	0
#else
#define LOG_INPUTX	0
#endif

struct InputCode
{
	UINT16 port[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipt[NUM_SIMUL_KEYS];
};

enum
{
	STATUS_KEYDOWN = 1	
};

enum
{
	MODIFIER_SHIFT = 1
};

struct KeyBuffer
{
	int begin_pos;
	int end_pos;
	int status;
	wchar_t buffer[4096];
};

struct InputMapEntry
{
	const char *name;
	int code;
};

/***************************************************************************

	Code assembling

***************************************************************************/

static struct InputMapEntry input_map[] =
{
	{ "UP",			'^' },
	{ "DOWN",		0 },
	{ "LEFT",		8 },
	{ "RIGHT",		9 },
	{ "ENTER",		13 },
	{ "CLEAR",		12 },
	{ "SPACE",		' ' },
	{ "BREAK",		3 },
	{ DEF_STR( Unused ), 	0 },
	{ "L-SHIFT",		0 },
	{ "R-SHIFT",		0 }
};

#if LOG_INPUTX
static const char *charstr(wchar_t ch)
{
	static char buf[3];

	switch(ch) {
	case '\0':	strcpy(buf, "\\0");		break;
	case '\r':	strcpy(buf, "\\r");		break;
	case '\n':	strcpy(buf, "\\n");		break;
	case '\t':	strcpy(buf, "\\t");		break;
	default:
		buf[0] = (char) ch;
		buf[1] = '\0';
		break;
	}
	return buf;
}
#endif

static wchar_t find_code(const char *name, int modifiers)
{
	int i;
	wchar_t code;
	int len = strlen(name);

	if (len == 1)
		code = (modifiers == 0) ? name[0] : 0;
	else if ((len == 4) && isspace(name[1]) && isspace(name[2]))
	{
		if (modifiers & MODIFIER_SHIFT)
			code = isspace(name[3]) ? 0 : name[3];
		else
			code = name[0];
	}
	else
	{
		code = 0;
		for (i = 0; i < sizeof(input_map) / sizeof(input_map[0]); i++)
		{
			if (!strcmp(name, input_map[i].name))
			{
				code = modifiers ? 0 : input_map[i].code;
				break;
			}
		}
	}

	if (code >= NUM_CODES)
		code = 0;
	return code;
}

static void scan_keys(const struct GameDriver *gamedrv, struct InputCode *codes, UINT16 *ports, const struct InputPortTiny **ipts, int keys, int modifiers, int *used_modifiers)
{
	const struct InputPortTiny *ipt;
	UINT16 port = (UINT16) -1;
	wchar_t code;

	assert(keys < NUM_SIMUL_KEYS);

	ipt = gamedrv->input_ports;
	while(ipt->type != IPT_END)
	{
		switch(ipt->type) {
		case IPT_PORT:
			port++;
			break;

		case IPT_KEYBOARD:
			if (!strcmp(ipt->name, "SHIFT") || !strcmp(ipt->name, "R-SHIFT") || !strcmp(ipt->name, "L-SHIFT"))
			{
				/* we've found a shift key */
				if ((*used_modifiers & MODIFIER_SHIFT) == 0)
				{
					ports[keys] = port;
					ipts[keys] = ipt;				
					*used_modifiers |= MODIFIER_SHIFT;
					scan_keys(gamedrv, codes, ports, ipts, keys+1, modifiers | MODIFIER_SHIFT, used_modifiers);
				}
			}
			else
			{
				code = find_code(ipt->name, modifiers);
				if (code > 0)
				{
					memcpy(codes[code].port, ports, sizeof(ports[0]) * keys);
					memcpy(codes[code].ipt, ipts, sizeof(ipts[0]) * keys);
					codes[code].port[keys] = port;
					codes[code].ipt[keys] = ipt;
				}
#if LOG_INPUTX
				if (gamedrv == Machine->gamedrv)
					logerror("inputx: code=%i (%s) port=%i ipt->name='%s'\n", (int) code, charstr(code), port, ipt->name);
#endif
			}
			break;
		}
		ipt++;
	}
}

#define CODE_BUFFER_SIZE	(sizeof(struct InputCode) * NUM_CODES + sizeof(struct KeyBuffer))

static void build_codes(const struct GameDriver *gamedrv, struct InputCode *codes)
{
	UINT16 ports[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipts[NUM_SIMUL_KEYS];
	int used_modifiers;
	int switch_upper;
	wchar_t c;

	memset(codes, 0, CODE_BUFFER_SIZE);

	scan_keys(gamedrv, codes, ports, ipts, 0, 0, &used_modifiers);

	/* special case; scan to see if upper case characters are specified, but not lower case */
	switch_upper = 1;
	for (c = 'A'; c <= 'Z'; c++)
	{
		if (!inputx_can_post_key(c) || inputx_can_post_key(towlower(c)))
		{
			switch_upper = 0;
			break;
		}
	}
	if (switch_upper)
		memcpy(&codes['a'], &codes['A'], sizeof(codes[0]) * 26);
}

/***************************************************************************

	Validity checks

***************************************************************************/

#ifdef MAME_DEBUG
void inputx_validitycheck(const struct GameDriver *gamedrv)
{
	char buf[CODE_BUFFER_SIZE];
	struct InputCode *codes;
	const struct InputPortTiny *ipt;
	int port_count, i, j;

	if (gamedrv->flags & GAME_COMPUTER)
	{
		codes = (struct InputCode *) buf;
		build_codes(gamedrv, codes);

		port_count = 0;
		for (ipt = gamedrv->input_ports; ipt->type != IPT_END; ipt++)
		{
			if (ipt->type == IPT_PORT)
				port_count++;
		}

		for (i = 0; i < NUM_CODES; i++)
		{
			for (j = 0; j < NUM_SIMUL_KEYS; j++)
			{
				assert(codes[i].port[j] >= 0);
				assert(codes[i].port[j] < port_count);
			}
		}
	}
}
#endif

/***************************************************************************

	Core

***************************************************************************/

static struct InputCode *codes;
static void *inputx_timer;

static void inputx_timerproc(int dummy);

void inputx_init(void)
{
	codes = NULL;
	inputx_timer = NULL;

	if (Machine->gamedrv->flags & GAME_COMPUTER)
	{
		codes = (struct InputCode *) auto_malloc(CODE_BUFFER_SIZE);
		if (!codes)
			return;
		build_codes(Machine->gamedrv, codes);

		inputx_timer = timer_alloc(inputx_timerproc);
	}
}

int inputx_can_post(void)
{
	return codes != NULL;
}

static struct KeyBuffer *get_buffer(void)
{
	assert(codes);
	return (struct KeyBuffer *) (codes + NUM_CODES);
}

int inputx_can_post_key(wchar_t ch)
{
	if ((ch >= 128) || !inputx_can_post())
		return 0;

	assert(codes);
	return codes[ch].ipt[0] != NULL;
}

void inputx_wpost(const wchar_t *text)
{
	struct KeyBuffer *keybuf;
	int last_cr = 0;
	int can_post;
	wchar_t ch;

	if (!text[0] || !inputx_can_post())
		return;

	keybuf = get_buffer();

	/* need to start up the timer? */
	if (keybuf->begin_pos == keybuf->end_pos)
	{
		timer_adjust(inputx_timer, 0, 0, TIME_IN_SEC(0.1));
		keybuf->status |= STATUS_KEYDOWN;
	}

	while(*text && (keybuf->end_pos+1 != keybuf->begin_pos))
	{
		ch = *text;

		/* change all eolns to '\r' */
		if ((ch != '\n') || !last_cr)
		{
			if (ch == '\n')
				ch = '\r';
			else
				last_cr = (ch == '\r');

			can_post = inputx_can_post_key(ch);

#if LOG_INPUTX
			logerror("inputx_wpost(): code=%i (%s) port=%i ipt->name='%s'\n", (int) ch, charstr(ch), codes[ch].port[0], codes[ch].ipt[0] ? codes[ch].ipt[0]->name : "<null>");
#endif

			if (can_post)
			{
				keybuf->buffer[keybuf->end_pos++] = ch;
				keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
			}
		}
		else
		{
			last_cr = 0;
		}
		text++;
	}
}

void inputx_post(const char *text)
{
	wchar_t buffer[128];
	size_t i;
	size_t charsz;

	while(*text)
	{
		for (i = 0; *text && i < (sizeof(buffer) / sizeof(buffer[0]))-1; i++)
		{
			charsz = mbtowc(&buffer[i], text, 1);
			if (charsz)
			{
				text += charsz;
			}
			else
			{
				buffer[i] = '?';
				text++;
			}
		}
		buffer[i] = '\0';
		inputx_wpost(buffer);
	}
}

static void inputx_timerproc(int dummy)
{
	struct KeyBuffer *keybuf;

	keybuf = get_buffer();
	if (keybuf->status & STATUS_KEYDOWN)
	{
		keybuf->status &= ~STATUS_KEYDOWN;
	}
	else
	{
		keybuf->status |= STATUS_KEYDOWN;
		keybuf->begin_pos++;
		keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		if (keybuf->begin_pos == keybuf->end_pos)
			timer_reset(inputx_timer, TIME_NEVER);
	}
}

void inputx_update(unsigned short *ports)
{
	const struct KeyBuffer *keybuf;
	const struct InputCode *code;
	const struct InputPortTiny *ipt;
	wchar_t ch;
	int i;
	int value;

	if (inputx_can_post())
	{
		keybuf = get_buffer();
		if ((keybuf->status & STATUS_KEYDOWN) && (keybuf->begin_pos != keybuf->end_pos))
		{
			ch = keybuf->buffer[keybuf->begin_pos];
			code = &codes[ch];

			for (i = 0; code->ipt[i] && (i < sizeof(code->ipt) / sizeof(code->ipt[0])); i++)
			{
				ipt = code->ipt[i];
				value = ports[code->port[i]];

				switch(ipt->default_value) {
				case IP_ACTIVE_LOW:
					value &= ~ipt->mask;
					break;
				case IP_ACTIVE_HIGH:
					value |= ipt->mask;
					break;
				}
				ports[code->port[i]] = value;
			}
		}
	}
}

