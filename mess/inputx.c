#include <ctype.h>
#include <assert.h>
#include <wctype.h>
#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#define NUM_CODES		128
#define NUM_SIMUL_KEYS	(UCHAR_SHIFT_END - UCHAR_SHIFT_BEGIN + 1)

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

struct KeyBuffer
{
	int begin_pos;
	int end_pos;
	int status;
	unicode_char_t buffer[4096];
};

struct CharInfo
{
	unicode_char_t ch;
	const char *name;
	const char *alternate;
};

static const struct CharInfo charinfo[] =
{
	{ 0x0008,					"Backspace",	NULL },		/* Backspace */	
	{ 0x0009,					"Tab",			"    " },	/* Tab */
	{ 0x000c,					"Clear",		NULL },		/* Clear */
	{ 0x000d,					"Enter",		NULL },		/* Enter */
	{ 0x001a,					"Esc",			NULL },		/* Esc */
	{ 0x0061,					NULL,			"A" },		/* a */
	{ 0x0062,					NULL,			"B" },		/* b */
	{ 0x0063,					NULL,			"C" },		/* c */
	{ 0x0064,					NULL,			"D" },		/* d */
	{ 0x0065,					NULL,			"E" },		/* e */
	{ 0x0066,					NULL,			"F" },		/* f */
	{ 0x0067,					NULL,			"G" },		/* g */
	{ 0x0068,					NULL,			"H" },		/* h */
	{ 0x0069,					NULL,			"I" },		/* i */
	{ 0x006a,					NULL,			"J" },		/* j */
	{ 0x006b,					NULL,			"K" },		/* k */
	{ 0x006c,					NULL,			"L" },		/* l */
	{ 0x006d,					NULL,			"M" },		/* m */
	{ 0x006e,					NULL,			"N" },		/* n */
	{ 0x006f,					NULL,			"O" },		/* o */
	{ 0x0070,					NULL,			"P" },		/* p */
	{ 0x0071,					NULL,			"Q" },		/* q */
	{ 0x0072,					NULL,			"R" },		/* r */
	{ 0x0073,					NULL,			"S" },		/* s */
	{ 0x0074,					NULL,			"T" },		/* t */
	{ 0x0075,					NULL,			"U" },		/* u */
	{ 0x0076,					NULL,			"V" },		/* v */
	{ 0x0077,					NULL,			"W" },		/* w */
	{ 0x0078,					NULL,			"X" },		/* x */
	{ 0x0079,					NULL,			"Y" },		/* y */
	{ 0x007a,					NULL,			"Z" },		/* z */
	{ 0x00a0,					NULL,			" " },		/* non breaking space */
	{ 0x00a1,					NULL,			"!" },		/* inverted exclaimation mark */
	{ 0x00a6,					NULL,			"|" },		/* broken bar */
	{ 0x00a9,					NULL,			"(c)" },	/* copyright sign */
	{ 0x00ab,					NULL,			"<<" },		/* left pointing double angle */
	{ 0x00ae,					NULL,			"(r)" },	/* registered sign */
	{ 0x00bb,					NULL,			">>" },		/* right pointing double angle */
	{ 0x00bc,					NULL,			"1/4" },	/* vulgar fraction one quarter */
	{ 0x00bd,					NULL,			"1/2" },	/* vulgar fraction one half */
	{ 0x00be,					NULL,			"3/4" },	/* vulgar fraction three quarters */
	{ 0x00bf,					NULL,			"?" },		/* inverted question mark */
	{ 0x00c0,					NULL,			"A" },		/* 'A' grave */
	{ 0x00c1,					NULL,			"A" },		/* 'A' acute */
	{ 0x00c2,					NULL,			"A" },		/* 'A' circumflex */
	{ 0x00c3,					NULL,			"A" },		/* 'A' tilde */
	{ 0x00c4,					NULL,			"A" },		/* 'A' diaeresis */
	{ 0x00c5,					NULL,			"A" },		/* 'A' ring above */
	{ 0x00c6,					NULL,			"AE" },		/* 'AE' ligature */
	{ 0x00c7,					NULL,			"C" },		/* 'C' cedilla */
	{ 0x00c8,					NULL,			"E" },		/* 'E' grave */
	{ 0x00c9,					NULL,			"E" },		/* 'E' acute */
	{ 0x00ca,					NULL,			"E" },		/* 'E' circumflex */
	{ 0x00cb,					NULL,			"E" },		/* 'E' diaeresis */
	{ 0x00cc,					NULL,			"I" },		/* 'I' grave */
	{ 0x00cd,					NULL,			"I" },		/* 'I' acute */
	{ 0x00ce,					NULL,			"I" },		/* 'I' circumflex */
	{ 0x00cf,					NULL,			"I" },		/* 'I' diaeresis */
	{ 0x00d0,					NULL,			"D" },		/* 'ETH' */
	{ 0x00d1,					NULL,			"N" },		/* 'N' tilde */
	{ 0x00d2,					NULL,			"O" },		/* 'O' grave */
	{ 0x00d3,					NULL,			"O" },		/* 'O' acute */
	{ 0x00d4,					NULL,			"O" },		/* 'O' circumflex */
	{ 0x00d5,					NULL,			"O" },		/* 'O' tilde */
	{ 0x00d6,					NULL,			"O" },		/* 'O' diaeresis */
	{ 0x00d7,					NULL,			"X" },		/* multiplication sign */
	{ 0x00d8,					NULL,			"O" },		/* 'O' stroke */
	{ 0x00d9,					NULL,			"U" },		/* 'U' grave */
	{ 0x00da,					NULL,			"U" },		/* 'U' acute */
	{ 0x00db,					NULL,			"U" },		/* 'U' circumflex */
	{ 0x00dc,					NULL,			"U" },		/* 'U' diaeresis */
	{ 0x00dd,					NULL,			"Y" },		/* 'Y' acute */
	{ 0x00df,					NULL,			"SS" },		/* sharp S */
	{ 0x00e0,					NULL,			"a" },		/* 'a' grave */
	{ 0x00e1,					NULL,			"a" },		/* 'a' acute */
	{ 0x00e2,					NULL,			"a" },		/* 'a' circumflex */
	{ 0x00e3,					NULL,			"a" },		/* 'a' tilde */
	{ 0x00e4,					NULL,			"a" },		/* 'a' diaeresis */
	{ 0x00e5,					NULL,			"a" },		/* 'a' ring above */
	{ 0x00e6,					NULL,			"ae" },		/* 'ae' ligature */
	{ 0x00e7,					NULL,			"c" },		/* 'c' cedilla */
	{ 0x00e8,					NULL,			"e" },		/* 'e' grave */
	{ 0x00e9,					NULL,			"e" },		/* 'e' acute */
	{ 0x00ea,					NULL,			"e" },		/* 'e' circumflex */
	{ 0x00eb,					NULL,			"e" },		/* 'e' diaeresis */
	{ 0x00ec,					NULL,			"i" },		/* 'i' grave */
	{ 0x00ed,					NULL,			"i" },		/* 'i' acute */
	{ 0x00ee,					NULL,			"i" },		/* 'i' circumflex */
	{ 0x00ef,					NULL,			"i" },		/* 'i' diaeresis */
	{ 0x00f0,					NULL,			"d" },		/* 'eth' */
	{ 0x00f1,					NULL,			"n" },		/* 'n' tilde */
	{ 0x00f2,					NULL,			"o" },		/* 'o' grave */
	{ 0x00f3,					NULL,			"o" },		/* 'o' acute */
	{ 0x00f4,					NULL,			"o" },		/* 'o' circumflex */
	{ 0x00f5,					NULL,			"o" },		/* 'o' tilde */
	{ 0x00f6,					NULL,			"o" },		/* 'o' diaeresis */
	{ 0x00f8,					NULL,			"o" },		/* 'o' stroke */
	{ 0x00f9,					NULL,			"u" },		/* 'u' grave */
	{ 0x00fa,					NULL,			"u" },		/* 'u' acute */
	{ 0x00fb,					NULL,			"u" },		/* 'u' circumflex */
	{ 0x00fc,					NULL,			"u" },		/* 'u' diaeresis */
	{ 0x00fd,					NULL,			"y" },		/* 'y' acute */
	{ 0x00ff,					NULL,			"y" },		/* 'y' diaeresis */
	{ 0x2010,					NULL,			"-" },		/* hyphen */
	{ 0x2011,					NULL,			"-" },		/* non-breaking hyphen */
	{ 0x2012,					NULL,			"-" },		/* figure dash */
	{ 0x2013,					NULL,			"-" },		/* en dash */
	{ 0x2014,					NULL,			"-" },		/* em dash */
	{ 0x2015,					NULL,			"-" },		/* horizontal dash */
	{ 0x2018,					NULL,			"\'" },		/* left single quotation mark */
	{ 0x2019,					NULL,			"\'" },		/* right single quotation mark */
	{ 0x201a,					NULL,			"\'" },		/* single low quotation mark */
	{ 0x201b,					NULL,			"\'" },		/* single high reversed quotation mark */
	{ 0x201c,					NULL,			"\"" },		/* left double quotation mark */
	{ 0x201d,					NULL,			"\"" },		/* right double quotation mark */
	{ 0x201e,					NULL,			"\"" },		/* double low quotation mark */
	{ 0x201f,					NULL,			"\"" },		/* double high reversed quotation mark */
	{ 0x2024,					NULL,			"." },		/* one dot leader */
	{ 0x2025,					NULL,			".." },		/* two dot leader */
	{ 0x2026,					NULL,			"..." },	/* horizontal ellipsis */
	{ 0x2047,					NULL,			"??" },		/* double question mark */
	{ 0x2048,					NULL,			"?!" },		/* question exclamation mark */
	{ 0x2049,					NULL,			"!?" },		/* exclamation question mark */
	{ UCHAR_MAMEKEY(ESC),		"Esc",			"\032" },	/* esc key */
	{ UCHAR_MAMEKEY(DEL),		"Delete",		"\010" },	/* delete key */
	{ UCHAR_MAMEKEY(HOME),		"Home",			"\014" },	/* home key */
	{ UCHAR_MAMEKEY(LCONTROL),	"Left Ctrl",	NULL },		/* left control key */
	{ UCHAR_MAMEKEY(RCONTROL),	"Right Ctrl",	NULL },		/* right control key */
	{ UCHAR_MAMEKEY(LSHIFT),	"Left Shift",	NULL },		/* left shift key */
	{ UCHAR_MAMEKEY(RSHIFT),	"Right Shift",	NULL }		/* right shift key */
};

#define INVALID_CHAR '?'

/***************************************************************************

	Char info lookup

***************************************************************************/

static const struct CharInfo *find_charinfo(unicode_char_t target_char)
{
	int low = 0;
	int high = sizeof(charinfo) / sizeof(charinfo[0]);
	int i;
	unicode_char_t ch;

	/* perform a simple binary search to find the proper alternate */
	while(high > low)
	{
		i = (high + low) / 2;
		ch = charinfo[i].ch;
		if (ch < target_char)
			low = i + 1;
		else if (ch > target_char)
			high = i;
		else
			return &charinfo[i];
	}
	return NULL;
}



/***************************************************************************

	Code assembling

***************************************************************************/

#if LOG_INPUTX
static const char *charstr(unicode_char_t ch)
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

static int scan_keys(const struct GameDriver *gamedrv, struct InputCode *codes, UINT16 *ports, const struct InputPortTiny **ipts, int keys, int shift)
{
	int result = 0;
	const struct InputPortTiny *ipt;
	const struct InputPortTiny *ipt_key = NULL;
	int key_shift = 0;
	UINT16 port = (UINT16) -1;
	unicode_char_t code;

	assert(keys < NUM_SIMUL_KEYS);

	ipt = gamedrv->input_ports;
	while(ipt->type != IPT_END)
	{
		switch(ipt->type & ~IPF_MASK) {
		case IPT_EXTENSION:
			break;

		case IPT_KEYBOARD:
			ipt_key = ipt;
			key_shift = 0;
			break;

		case IPT_UCHAR:
			assert(ipt_key);

			result = 1;
			if (key_shift == shift)
			{
				code = ipt->mask;
				code <<= 16;
				code |= ipt->default_value;

				if ((code >= UCHAR_SHIFT_BEGIN) && (code <= UCHAR_SHIFT_END))
				{
					ports[keys] = port;
					ipts[keys] = ipt_key;
					scan_keys(gamedrv, codes, ports, ipts, keys+1, code - UCHAR_SHIFT_1 + 1);
				}
				else if (code < NUM_CODES)
				{
					memcpy(codes[code].port, ports, sizeof(ports[0]) * keys);
					memcpy((void *) codes[code].ipt, ipts, sizeof(ipts[0]) * keys);
					codes[code].port[keys] = port;
					codes[code].ipt[keys] = ipt_key;
#if LOG_INPUTX
					if (gamedrv == Machine->gamedrv)
						logerror("inputx: code=%i (%s) port=%i ipt->name='%s'\n", (int) code, charstr(code), port, ipt->name);
#endif
				}
			}
			key_shift++;
			break;

		case IPT_PORT:
			port++;
			/* fall through */

		default:
			ipt_key = NULL;
			break;
		}
		ipt++;
	}
	return result;
}

static unicode_char_t unicode_tolower(unicode_char_t c)
{
	return (c < 128) ? tolower((char) c) : c;
}

#define CODE_BUFFER_SIZE	(sizeof(struct InputCode) * NUM_CODES)

static int build_codes(const struct GameDriver *gamedrv, struct InputCode *codes, int map_lowercase)
{
	UINT16 ports[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipts[NUM_SIMUL_KEYS];
	int switch_upper;
	unicode_char_t c;

	memset(codes, 0, CODE_BUFFER_SIZE);

	if (!scan_keys(gamedrv, codes, ports, ipts, 0, 0))
		return 0;

	if (map_lowercase)
	{
		/* special case; scan to see if upper case characters are specified, but not lower case */
		switch_upper = 1;
		for (c = 'A'; c <= 'Z'; c++)
		{
			if (!inputx_can_post_key(c) || inputx_can_post_key(unicode_tolower(c)))
			{
				switch_upper = 0;
				break;
			}
		}

		if (switch_upper)
			memcpy(&codes['a'], &codes['A'], sizeof(codes[0]) * 26);
	}
	return 1;
}

/***************************************************************************

	Validity checks

***************************************************************************/

#ifdef MAME_DEBUG
int inputx_validitycheck(const struct GameDriver *gamedrv)
{
	char buf[CODE_BUFFER_SIZE];
	struct InputCode *codes;
	const struct InputPortTiny *ipt;
	int port_count, i, j;
	const char *str;
	int error = 0;
	unicode_char_t last_char = 0;
	const struct CharInfo *ci;

	if (gamedrv)
	{
		if (gamedrv->flags & GAME_COMPUTER)
		{
			codes = (struct InputCode *) buf;
			build_codes(gamedrv, codes, FALSE);

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
					if (codes[i].port[j] >= port_count)
					{
						printf("%s: invalid inputx translation for code %i port %i\n", gamedrv->name, i, j);
						error = 1;
					}
				}
			}
		}
	}
	else
	{
		/* check to make sure that charinfo is in order */
		for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
		{
			if (last_char >= charinfo[i].ch)
			{
				printf("inputx: charinfo is out of order; 0x%08x should be higher than 0x%08x\n", charinfo[i].ch, last_char);
				error = 1;
			}
			last_char = charinfo[i].ch;
		}

		/* check to make sure that I can look up everything on alternate_charmap */
		for (i = 0; i < sizeof(charinfo) / sizeof(charinfo[0]); i++)
		{
			ci = find_charinfo(charinfo[i].ch);
			if (ci != &charinfo[i])
			{
				printf("inputx: expected find_charinfo(0x%08x) to work properly\n", charinfo[i].ch);
				error = 1;
			}
		}
	}
	return error;
}
#endif

/***************************************************************************

	Core

***************************************************************************/

static struct InputCode *codes;
static struct KeyBuffer *keybuffer;
static mame_timer *inputx_timer;
static int (*queue_chars)(const unicode_char_t *text, size_t text_len);
static int (*accept_char)(unicode_char_t ch);
static int (*charqueue_empty)(void);
static mame_time current_rate;

static void inputx_timerproc(int dummy);

void inputx_init(void)
{
	struct SystemConfigurationParamBlock params;

	codes = NULL;
	inputx_timer = NULL;
	queue_chars = NULL;
	accept_char = NULL;
	charqueue_empty = NULL;

	/* posting keys directly only makes sense for a computer */
	if (Machine->gamedrv->flags & GAME_COMPUTER)
	{
		/* check driver for QUEUE_CHARS and/or ACCEPT_CHAR callbacks */
		memset(&params, 0, sizeof(params));
		if (Machine->gamedrv->sysconfig_ctor)
			Machine->gamedrv->sysconfig_ctor(&params);
		queue_chars = params.queue_chars;
		accept_char = params.accept_char;
		charqueue_empty = params.charqueue_empty;

		keybuffer = auto_malloc(sizeof(struct KeyBuffer));
		if (!keybuffer)
			goto error;
		memset(keybuffer, 0, sizeof(*keybuffer));

		if (!queue_chars)
		{
			/* only need to compute codes if QUEUE_CHARS not present */
			codes = (struct InputCode *) auto_malloc(CODE_BUFFER_SIZE);
			if (!codes)
				goto error;
			if (!build_codes(Machine->gamedrv, codes, TRUE))
				goto error;
		}

		inputx_timer = mame_timer_alloc(inputx_timerproc);
	}
	return;

error:
	codes = NULL;
	keybuffer = NULL;
	queue_chars = NULL;
	accept_char = NULL;
}

int inputx_can_post(void)
{
	return queue_chars || codes;
}

static struct KeyBuffer *get_buffer(void)
{
	assert(inputx_can_post());
	return (struct KeyBuffer *) keybuffer;
}

static int can_post_key_directly(unicode_char_t ch)
{
	int rc;

	if (queue_chars)
	{
		rc = accept_char ? accept_char(ch) : TRUE;
	}
	else
	{
		assert(codes);
		rc = ((ch < NUM_CODES) && codes[ch].ipt[0] != NULL);
	}
	return rc;
}

static int can_post_key_alternate(unicode_char_t ch)
{
	const char *s;
	const struct CharInfo *ci;

	ci = find_charinfo(ch);
	s = ci ? ci->alternate : NULL;
	if (!s)
		return 0;

	while(*s)
	{
		if (!can_post_key_directly(*s))
			return 0;
		s++;
	}
	return 1;
}



int inputx_can_post_key(unicode_char_t ch)
{
	
	return inputx_can_post() && (can_post_key_directly(ch) || can_post_key_alternate(ch));
}



static mame_time choose_delay(unicode_char_t ch)
{
	subseconds_t delay = 0;

	if (current_rate.seconds || current_rate.subseconds)
		return current_rate;

	if (queue_chars)
	{
		/* systems with queue_chars can afford a much smaller delay */
		delay = DOUBLE_TO_SUBSECONDS(0.01);
	}
	else
	{
		switch(ch) {
		case '\r':
			delay = DOUBLE_TO_SUBSECONDS(0.2);
			break;

		default:
			delay = DOUBLE_TO_SUBSECONDS(0.05);
			break;
		}
	}
	return make_mame_time(0, delay);
}



static void internal_post_key(unicode_char_t ch)
{
	struct KeyBuffer *keybuf;

	keybuf = get_buffer();

	/* need to start up the timer? */
	if (keybuf->begin_pos == keybuf->end_pos)
	{
		mame_timer_adjust(inputx_timer, choose_delay(ch), 0, time_zero);
		keybuf->status &= ~STATUS_KEYDOWN;
	}

	keybuf->buffer[keybuf->end_pos++] = ch;
	keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
}



static int buffer_full(void)
{
	struct KeyBuffer *keybuf;
	keybuf = get_buffer();
	return ((keybuf->end_pos + 1) % (sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]))) == keybuf->begin_pos;
}



void inputx_postn_rate(const unicode_char_t *text, size_t text_len, mame_time rate)
{
	int last_cr = 0;
	unicode_char_t ch;
	const char *s;
	const struct CharInfo *ci;

	current_rate = rate;

	if (inputx_can_post())
	{
		while((text_len > 0) && !buffer_full())
		{
			ch = *(text++);
			text_len--;

			/* change all eolns to '\r' */
			if ((ch != '\n') || !last_cr)
			{
				if (ch == '\n')
					ch = '\r';
				else
					last_cr = (ch == '\r');
#if LOG_INPUTX
				logerror("inputx_postn(): code=%i (%s) port=%i ipt->name='%s'\n", (int) ch, charstr(ch), codes[ch].port[0], codes[ch].ipt[0] ? codes[ch].ipt[0]->name : "<null>");
#endif

				if (can_post_key_directly(ch))
				{
					/* we can post this key in the queue directly */
					internal_post_key(ch);
				}
				else if (can_post_key_alternate(ch))
				{
					/* we can post this key with an alternate representation */
					ci = find_charinfo(ch);
					assert(ci && ci->alternate);
					s = ci->alternate;
					while(*s)
						internal_post_key(*(s++));
				}
			}
			else
			{
				last_cr = 0;
			}
		}
	}
}



static void inputx_timerproc(int dummy)
{
	struct KeyBuffer *keybuf;
	mame_time delay;

	keybuf = get_buffer();

	if (queue_chars)
	{
		/* the driver has a queue_chars handler */
		while((keybuf->begin_pos != keybuf->end_pos) && queue_chars(&keybuf->buffer[keybuf->begin_pos], 1))
		{
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);

			if (current_rate.seconds || current_rate.subseconds)
				break;
		}
	}
	else
	{
		/* the driver does not have a queue_chars handler */
		if (keybuf->status & STATUS_KEYDOWN)
		{
			keybuf->status &= ~STATUS_KEYDOWN;
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
		else
		{
			keybuf->status |= STATUS_KEYDOWN;
		}
	}

	/* need to make sure timerproc is called again if buffer not empty */
	if (keybuf->begin_pos != keybuf->end_pos)
	{
		delay = choose_delay(keybuf->buffer[keybuf->begin_pos]);
		mame_timer_adjust(inputx_timer, delay, 0, time_zero);
	}
}



void inputx_update(unsigned short *ports)
{
	const struct KeyBuffer *keybuf;
	const struct InputCode *code;
	const struct InputPortTiny *ipt;
	unicode_char_t ch;
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



const struct InputPortTiny *inputx_handle_mess_extensions(
	const struct InputPortTiny *ext, struct InputPort *dst, int port_size)
{
	char buf[256];
	int i, pos = 0;
	const char *s;
	unicode_char_t ch;

	/* process MESS specific extensions to the port */
	buf[0] = '\0';
	while((ext->type & ~IPF_MASK) == IPT_UCHAR ||
		(ext->type & ~IPF_MASK) == IPT_CATEGORY)
	{
		switch(ext->type & ~IPF_MASK)
		{
			case IPT_CATEGORY:
				for (i = 0; i < port_size; i++)
					dst[i].category = ext->default_value;
				break;
			case IPT_UCHAR:
				ch = ext->mask;
				ch <<= 16;
				ch |= ext->default_value;
				pos += sprintf(&buf[pos], "%s ", inputx_key_name(ch));
				break;
		}
		ext++;
	}

	if (buf[0])
	{
		/* we have a default name for this keyboard port; iterate through the
		 * ports and identify any keyboard ports that use IP_NAME_DEFAULT */
		s = NULL;
		for (i = 0; i < port_size; i++)
		{
			if (dst[i].name == IP_NAME_DEFAULT)
			{
				if (!s)
				{
					/* we have not allocated our copy of buf yet; allocate it
					 * now */
					rtrim(buf);
					s = auto_strdup(buf);
				}
				dst[i].name = s;
			}
		}
	}
	return ext;
}



const char *inputx_key_name(unicode_char_t ch)
{
	static char buf[2];
	const struct CharInfo *ci;
	const char *result;

	ci = find_charinfo(ch);
	result = ci ? ci->name : NULL;

	if (ci && ci->name)
	{
		result = ci->name;
	}
	else
	{
		if ((ch <= 0x7F) && isprint(ch))
		{
			buf[0] = (char) ch;
			buf[1] = '\0';
			result = buf;
		}
		else
			result = "???";
	}
	return result;
}



/* --------------------------------------------------------------------- */

int inputx_is_posting(void)
{
	const struct KeyBuffer *keybuf;
	keybuf = get_buffer();
	return (keybuf->begin_pos != keybuf->end_pos) || (charqueue_empty && !charqueue_empty());
}



/***************************************************************************

	Alternative calls

***************************************************************************/

void inputx_postn(const unicode_char_t *text, size_t text_len)
{
	inputx_postn_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_rate(const unicode_char_t *text, mame_time rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_rate(text, len, rate);
}



void inputx_post(const unicode_char_t *text)
{
	inputx_post_rate(text, make_mame_time(0, 0));
}



void inputx_postc_rate(unicode_char_t ch, mame_time rate)
{
	inputx_postn_rate(&ch, 1, rate);
}



void inputx_postc(unicode_char_t ch)
{
	inputx_postc_rate(ch, make_mame_time(0, 0));
}



void inputx_postn_utf16_rate(const utf16_char_t *text, size_t text_len, mame_time rate)
{
	size_t len = 0;
	unicode_char_t c;
	utf16_char_t w1, w2;
	unicode_char_t buf[256];

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		w1 = *(text++);
		text_len--;

		if ((w1 >= 0xd800) && (w1 <= 0xdfff))
		{
			if (w1 <= 0xDBFF)
			{
				w2 = 0;
				if (text_len > 0)
				{
					w2 = *(text++);
					text_len--;
				}
				if ((w2 >= 0xdc00) && (w2 <= 0xdfff))
				{
					c = w1 & 0x03ff;
					c <<= 10;
					c |= w2 & 0x03ff;
				}
				else
				{
					c = INVALID_CHAR;
				}
			}
			else
			{
				c = INVALID_CHAR;
			}
		}
		else
		{
			c = w1;
		}
		buf[len++] = c;
	}
	inputx_postn_rate(buf, len, rate);
}



void inputx_postn_utf16(const utf16_char_t *text, size_t text_len)
{
	inputx_postn_utf16_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_utf16_rate(const utf16_char_t *text, mame_time rate)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_utf16_rate(text, len, rate);
}



void inputx_post_utf16(const utf16_char_t *text)
{
	inputx_post_utf16_rate(text, make_mame_time(0, 0));
}



void inputx_postn_utf8_rate(const char *text, size_t text_len, mame_time rate)
{
	size_t len = 0;
	unicode_char_t buf[256];
	unicode_char_t c;
	int rc;

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		rc = uchar_from_utf8(&c, text, text_len);
		if (rc < 0)
		{
			rc = 1;
			c = INVALID_CHAR;
		}
		text += rc;
		text_len -= rc;
		buf[len++] = c;
	}
	inputx_postn_rate(buf, len, rate);
}



void inputx_postn_utf8(const char *text, size_t text_len)
{
	inputx_postn_utf8_rate(text, text_len, make_mame_time(0, 0));
}



void inputx_post_utf8_rate(const char *text, mame_time rate)
{
	inputx_postn_utf8_rate(text, strlen(text), rate);
}



void inputx_post_utf8(const char *text)
{
	inputx_post_utf8_rate(text, make_mame_time(0, 0));
}



/***************************************************************************

	Other stuff

	This stuff is here more out of convienience than anything else
***************************************************************************/

/* this function needs to be used with InputPort and InputPortTiny structs,
 * so we have to put the type and name as separate parameters */
static int categorize_port_type(UINT32 type, const char *name, UINT16 category)
{
	int result;

	if ((type & IPF_MASK) == IPF_UNUSED)
		return INPUT_CLASS_INTERNAL;
	if (category && ((type & ~IPF_MASK) != IPT_CATEGORY_SETTING))
		return INPUT_CLASS_CATEGORIZED;

	switch(type & ~IPF_MASK) {
	case IPT_JOYSTICK_UP:
	case IPT_JOYSTICK_DOWN:
	case IPT_JOYSTICK_LEFT:
	case IPT_JOYSTICK_RIGHT:
	case IPT_JOYSTICKLEFT_UP:
	case IPT_JOYSTICKLEFT_DOWN:
	case IPT_JOYSTICKLEFT_LEFT:
	case IPT_JOYSTICKLEFT_RIGHT:
	case IPT_JOYSTICKRIGHT_UP:
	case IPT_JOYSTICKRIGHT_DOWN:
	case IPT_JOYSTICKRIGHT_LEFT:
	case IPT_JOYSTICKRIGHT_RIGHT:
	case IPT_BUTTON1:
	case IPT_BUTTON2:
	case IPT_BUTTON3:
	case IPT_BUTTON4:
	case IPT_BUTTON5:
	case IPT_BUTTON6:
	case IPT_BUTTON7:
	case IPT_BUTTON8:
	case IPT_BUTTON9:
	case IPT_BUTTON10:
	case IPT_AD_STICK_X:
	case IPT_AD_STICK_Y:
	case IPT_AD_STICK_Z:
	case IPT_TRACKBALL_X:
	case IPT_TRACKBALL_Y:
	case IPT_LIGHTGUN_X:
	case IPT_LIGHTGUN_Y:
	case IPT_MOUSE_X:
	case IPT_MOUSE_Y:
	case IPT_START:
	case IPT_SELECT:
		result = INPUT_CLASS_CONTROLLER;
		break;

	case IPT_KEYBOARD:
		result = INPUT_CLASS_KEYBOARD;
		break;

	case IPT_CONFIG_NAME:
		result = INPUT_CLASS_CONFIG;
		break;

	case IPT_DIPSWITCH_NAME:
		result = INPUT_CLASS_DIPSWITCH;
		break;

	case 0:
		if (name && (name != (const char *) -1))
			result = INPUT_CLASS_MISC;
		else
			result = INPUT_CLASS_INTERNAL;
		break;

	default:
		result = INPUT_CLASS_INTERNAL;
		break;
	}
	return result;
}



int input_classify_port(const struct InputPort *port)
{
	if ((port->type & ~IPF_MASK) == IPT_EXTENSION)
		port--;
	return categorize_port_type(port->type, port->name, port->category);
}



int input_player_number(const struct InputPort *port)
{
	if ((port->type & ~IPF_MASK) == IPT_EXTENSION)
		port--;
	return (port->type & IPF_PLAYERMASK) / IPF_PLAYER2;
}



int input_has_input_class(int inputclass)
{
	struct InputPort *in;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (input_classify_port(in) == inputclass)
			return TRUE;
	}
	return FALSE;
}



int input_count_players(void)
{
	const struct InputPortTiny *in;
	int joystick_count;

	joystick_count = 0;
	for (in = Machine->gamedrv->input_ports; in->type != IPT_END; in++)
	{
		if (categorize_port_type(in->type, in->name, 0) == INPUT_CLASS_CONTROLLER)
		{
			if (joystick_count <= (in->type & IPF_PLAYERMASK) / IPF_PLAYER2)
				joystick_count = (in->type & IPF_PLAYERMASK) / IPF_PLAYER2 + 1;
		}
	}
	return joystick_count;
}



int input_category_active(int category)
{
	const struct InputPort *in;
	const struct InputPort *in_base = NULL;

	assert(category >= 1);

	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		switch(in->type) {
		case IPT_CATEGORY_NAME:
			in_base = in;
			break;

		case IPT_CATEGORY_SETTING:
			if ((in->category == category) && (in_base->default_value == in->default_value))
				return TRUE;
			break;
		}
	}
	return FALSE;
}
