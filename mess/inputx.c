#include <ctype.h>
#include <assert.h>

#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#define NUM_CODES		128
#define NUM_SIMUL_KEYS	2

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
	char buffer[4096];
};

struct InputMapEntry
{
	const char *name;
	int code;
};

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

static struct InputCode *codes;
static void *inputx_timer;

static void inputx_timerproc(int dummy);


static int find_code(const char *name, int modifiers)
{
	int i;
	int code;
	int len = strlen(name);

	if (len == 1)
		code = (modifiers == 0) ? name[0] : 0;
	else if ((len == 4) && isspace(name[1]) && isspace(name[2]))
		code = isspace(name[3]) ? 0 : name[(modifiers & MODIFIER_SHIFT) ? 3 : 0];
	else
	{
		code = -1;
		for (i = 0; i < sizeof(input_map) / sizeof(input_map[0]); i++)
		{
			if (!strcmp(name, input_map[i].name))
			{
				code = modifiers ? 0 : input_map[i].code;
				break;
			}
		}
	}
	return code;
}

static void scan_keys(UINT16 *ports, const struct InputPortTiny **ipts, int keys, int modifiers, int *used_modifiers)
{
	const struct InputPortTiny *ipt;
	UINT16 port = (UINT16) -1;
	int code;

	assert(keys < NUM_SIMUL_KEYS);

	ipt = Machine->gamedrv->input_ports;
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
					scan_keys(ports, ipts, keys+1, modifiers | MODIFIER_SHIFT, used_modifiers);
				}
			}
			else
			{
				code = find_code(ipt->name, modifiers);
				if (code < 0)
				{
					/* we've failed to translate this system's codes */
					codes = NULL;
					return;
				}
				if (code > 0)
				{
					memcpy(codes[code].port, ports, sizeof(ports[0]) * keys);
					memcpy(codes[code].ipt, ipts, sizeof(ipt[0]) * keys);
					codes[code].port[keys] = port;
					codes[code].ipt[keys] = ipt;
				}
			}
			break;
		}
		ipt++;
	}
}

void inputx_init(void)
{
	UINT16 ports[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipts[NUM_SIMUL_KEYS];
	size_t buffer_size;
	int used_modifiers;
	int i;
	int switch_upper;

	codes = NULL;
	inputx_timer = NULL;

	if (Machine->gamedrv->flags & GAME_COMPUTER)
	{
		buffer_size = sizeof(struct InputCode) * NUM_CODES + sizeof(struct KeyBuffer);

		codes = (struct InputCode *) auto_malloc(buffer_size);
		if (!codes)
			return;
		memset(codes, 0, buffer_size);

		scan_keys(ports, ipts, 0, 0, &used_modifiers);

		/* special case; scan to see if upper case characters are specified, but not lower case */
		switch_upper = 1;
		for (i = 'A'; i <= 'Z'; i++)
		{
			if (!inputx_can_post_key(i) || inputx_can_post_key(tolower(i)))
			{
				switch_upper = 0;
				break;
			}
		}
		if (switch_upper)
			memcpy(&codes['a'], &codes['A'], sizeof(codes[0]) * 26);

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

int inputx_can_post_key(int ch)
{
	if ((ch < 0) || (ch >= 128) || !inputx_can_post())
		return 0;

	assert(codes);
	return codes[ch].ipt[0] != NULL;
}

void inputx_post(const char *text)
{
	struct KeyBuffer *keybuf;

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
		if (inputx_can_post_key(*text))
		{
			keybuf->buffer[keybuf->end_pos++] = *(text++);
			keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
		else
		{
			text++;
		}
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
	int ch;
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

