/***************************************************************************

	crcfile.c - crc file save/load functions

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "driver.h"
#include "crcfile.h"
#include "fileio.h"


/***************************************************************************
	DEBUGGING
***************************************************************************/

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/***************************************************************************
	CONSTANTS
***************************************************************************/

/* Amount of memory to allocate while reading variable size arrays
 * Tradeoff between calling realloc() all the time and wasting memory */
#define CHUNK_SIZE	4096

/***************************************************************************
	TYPE DEFINITIONS
***************************************************************************/

/* A forward linked list of the contents of a section */
struct crcfile_var
{
	struct crcfile_var *next;
	char *name;
	unsigned size;
	unsigned chunk;
	void *data;
};

/* our config handling structure */
struct _crc_file
{
	mame_file *file;
	const char *section;
	int instance;
	struct crcfile_var *list;
    struct crcfile_var *end_of_list; // to avoid n*(n/2) runtime complexity when appending
};

/***************************************************************************
	xtoul
***************************************************************************/

INLINE unsigned xtoul(char **p, int *size)
{
	unsigned val = 0, digit;

	if (size)
		*size = 0;
	while (isxdigit(**p))
	{
		digit = toupper(**p) - '0';
		if (digit > 9)
			digit -= 7;
		val = (val << 4) | digit;
		if (size)
			(*size)++;
		*p += 1;
	}
	while (isspace(**p))
		*p += 1;
	if (size)
		(*size) >>= 1;
	return val;
}

/***************************************************************************
	ctoul
***************************************************************************/

/* extract next character from the string. un-escape \oct codes */
INLINE unsigned ctoul(char **p)
{
	unsigned val = 0, digit;

    if (**p == '\\')
	{
		*p += 1;
		if (**p >= '0' && **p <= '7')
		{
			while (**p >= '0' && **p <= '7')
			{
				digit = toupper(**p) - '0';
				val = (val << 3) | digit;
				*p += 1;
			}
		}
		else
		{
			val = **p;
			*p += 1;
		}
    }
	else
	{
		val = **p;
		*p += 1;
	}
	return val;
}

/***************************************************************************
	ultox
***************************************************************************/

INLINE char *ultox(unsigned val, unsigned size)
{
	static char buffer[32+1];
	static char digit[] = "0123456789ABCDEF";
	char *p = &buffer[size];
	*p-- = '\0';
	while (size-- > 0)
	{
		*p-- = digit[val & 15];
		val >>= 4;
	}
	return buffer;
}

/***************************************************************************
	xdigits_only
***************************************************************************/

INLINE int xdigits_only(const char *src)
{
	while (*src && (isxdigit(*src) || isspace(*src)))
		src++;
	return (*src) ? 0 : 1;
}

/***************************************************************************
	findstr
***************************************************************************/

INLINE int findstr(const char *dst, const char *src)
{
	while (*src && *dst)
	{
		if (tolower(*src) != tolower(*dst))
			return *dst - *src;
		src++;
		dst++;
	}
	if( *dst || *src )
		return 0;
	return *dst - *src;
}

/***************************************************************************
	crcfile_free_section
***************************************************************************/

/* free a linked list of state_vars (aka section) */
static void crcfile_free_section(crc_file *cfg)
{
	struct crcfile_var *this, *next;

	next = cfg->list;
	while (next)
	{
		if (next->name)
			free(next->name);
		if (next->data)
			free(next->data);
		this = next;
		next = next->next;
		free(this);
	}
	cfg->list = NULL;
}

/***************************************************************************
	crcfile_create
***************************************************************************/

crc_file *crcfile_create(const char *gamename, const char *filename, int filetype)
{
	crc_file *cfg;

    cfg = (crc_file *) malloc(sizeof (struct _crc_file));
	if (!cfg)
	{
		LOG(("crcfile_create: memory problem\n"));
		return cfg;
	}

	memset(cfg, 0, sizeof(*cfg));
	cfg->file = mame_fopen(gamename, filename, filetype, OSD_FOPEN_RW_CREATE);
	if (!cfg->file)
	{
		LOG(("crcfile_create: couldn't create file '%s'\n", name));
		free(cfg);
		return NULL;
	}
	LOG(("crcfile_create: created file '%s'\n", name));
    return cfg;
}

/***************************************************************************
	crcfile_open
***************************************************************************/

crc_file *crcfile_open(const char *gamename, const char *filename, int filetype)
{
	crc_file *cfg;

    cfg = (struct _crc_file *) malloc(sizeof (struct _crc_file));
	if (!cfg)
	{
		LOG(("crcfile_open: memory problem\n"));
		return cfg;
	}

	memset(cfg, 0, sizeof(struct _crc_file));
	cfg->file = mame_fopen(gamename, filename, filetype, OSD_FOPEN_READ);
	if (!cfg->file)
	{
		LOG(("crcfile_open: couldn't open file '%s'\n", name));
        free(cfg);
		return NULL;
	}

	LOG(("crcfile_open: opened file '%s'\n", name));
    return cfg;
}

/***************************************************************************
	crcfile_close
***************************************************************************/

void crcfile_close(crc_file *cfg)
{
	crcfile_free_section(cfg);
	mame_fclose(cfg->file);
	free(cfg);
}

/***************************************************************************
	crcfile_printf
***************************************************************************/

/* Output a formatted string to the config file */
static void CLIB_DECL crcfile_printf(crc_file *cfg, const char *fmt,...)
{
	static char buffer[255 + 1];
	va_list arg;
	int length;

	va_start(arg, fmt);
	length = vsprintf(buffer, fmt, arg);
	va_end(arg);

	if (mame_fwrite(cfg->file, buffer, length) != length)
	{
		LOG(("crcfile_printf: Error while saving cfg '%s'\n", buffer));
	}
}

/***************************************************************************
	crcfile_save_section
***************************************************************************/

static void crcfile_save_section(crc_file *cfg, const char *section, int instance)
{
	if (!cfg->section ||
		(cfg->section && findstr(cfg->section, section)) ||
		cfg->instance != instance)
	{
		if (cfg->section)
			crcfile_printf(cfg, "\n");
		cfg->section = section;
		cfg->instance = instance;
		if (instance)
			crcfile_printf(cfg, "[%s.%d]\n", section, instance);
		else
			crcfile_printf(cfg, "[%s]\n", section);
	}
}

/***************************************************************************
	crcfile_save_string
***************************************************************************/

void crcfile_save_string(crc_file *cfg, const char *section, int instance,
					   const char *name, const char *src)
{
	unsigned size = strlen(src);

    crcfile_save_section(cfg, section, instance);

	crcfile_printf(cfg, "%s=", name);
	while (size-- > 0)
	{
		switch (*src)
		{
            case '\\':
			case '[':
			case ']':
				crcfile_printf(cfg, "\\%c", *src++);
				break;
			default:
				if (*src < 32 || *src == '\t' || *src == '\\' || *src == '[' || *src == ']' )
					crcfile_printf(cfg, "\\%o", *src++);
				else
					crcfile_printf(cfg, "%c", *src++);
		}
	}
	crcfile_printf(cfg, "\n");
}

/***************************************************************************
	crcfile_save_UINT8
***************************************************************************/

void crcfile_save_UINT8(crc_file *cfg, const char *section, int instance,
					   const char *name, const UINT8 *val, unsigned size)
{
	crcfile_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 16)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 15) == 0)
				crcfile_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			crcfile_printf(cfg, "%s", ultox(*val++, 2));
			if ((++offs & 15) == 0)
				crcfile_printf(cfg, "\n");
			else
				crcfile_printf(cfg, " ");
		}
		if (offs & 15)
			crcfile_printf(cfg, "\n");
	}
	else
	{
		crcfile_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			crcfile_printf(cfg, "%s", ultox(*val++, 2));
			if (size)
				crcfile_printf(cfg, " ");
		}
		crcfile_printf(cfg, "\n");
	}
}

/***************************************************************************
	crcfile_save_INT8
***************************************************************************/

void crcfile_save_INT8(crc_file *config, const char *section, int instance,
					  const char *name, const INT8 *val, unsigned size)
{
	crcfile_save_UINT8(config, section, instance, name, (UINT8 *)val, size);
}

/***************************************************************************
	crcfile_save_UINT16
***************************************************************************/

void crcfile_save_UINT16(crc_file *cfg, const char *section, int instance,
						const char *name, const UINT16 *val, unsigned size)
{
	crcfile_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 8)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 7) == 0)
				crcfile_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			crcfile_printf(cfg, "%s", ultox(*val++, 4));
			if ((++offs & 7) == 0)
				crcfile_printf(cfg, "\n");
			else
				crcfile_printf(cfg, " ");
		}
		if (offs & 7)
			crcfile_printf(cfg, "\n");
	}
	else
	{
		crcfile_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			crcfile_printf(cfg, "%s", ultox(*val++, 4));
			if (size)
				crcfile_printf(cfg, " ");
		}
		crcfile_printf(cfg, "\n");
	}
}

/***************************************************************************
	crcfile_save_INT16
***************************************************************************/

void crcfile_save_INT16(crc_file *config, const char *section, int instance,
					   const char *name, const INT16 *val, unsigned size)
{
	crcfile_save_UINT16(config, section, instance, name, (UINT16 *)val, size);
}

/***************************************************************************
	crcfile_save_UINT32
***************************************************************************/

void crcfile_save_UINT32(crc_file *cfg, const char *section, int instance,
						const char *name, const UINT32 *val, unsigned size)
{
	crcfile_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 4)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 3) == 0)
				crcfile_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			crcfile_printf(cfg, "%s", ultox(*val++, 8));
			if ((++offs & 3) == 0)
				crcfile_printf(cfg, "\n");
			else
				crcfile_printf(cfg, " ");
		}
		if (offs & 3)
			crcfile_printf(cfg, "\n");
	}
	else
	{
		crcfile_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			crcfile_printf(cfg, "%s", ultox(*val++, 8));
			if (size)
				crcfile_printf(cfg, " ");
		}
		crcfile_printf(cfg, "\n");
	}
}

/***************************************************************************
	crcfile_save_INT32
***************************************************************************/

void crcfile_save_INT32(crc_file *config, const char *section, int instance,
					   const char *name, const INT32 *val, unsigned size)
{
	crcfile_save_UINT32(config, section, instance, name, (UINT32 *)val, size);
}

/***************************************************************************
	crcfile_load_section
***************************************************************************/

/* load a linked list of crcfile_vars (aka section) */
static void crcfile_load_section(crc_file *cfg, const char *section, int instance)
{
	/* Make the buffer twice as big as it was while saving
	 * the config, so we should always catch a [section] */
	static char buffer[2047+1];
	char searching[63+1], *p, *d;
	int element_size;
	unsigned offs, data;

	if (cfg->section && !findstr(cfg->section, section) &&
		cfg->instance == instance)
		return;						   /* fine, we already got it */

	if (!cfg->list)
		crcfile_free_section(cfg);

	if (instance)
		sprintf(searching, "[%s.%d]", section, instance);
	else
		sprintf(searching, "[%s]", section);
	LOG(("crcfile_load_section: searching for '%s'\n", searching));

	for (;;)
	{
		buffer[0] = '\0';
		mame_fgets(buffer, sizeof(buffer), cfg->file);
		if (!buffer[0])
			return;
		if (findstr(buffer, searching) == 0)
		{
			cfg->section = section;
			cfg->instance = instance;
			/* now read all crcfile_vars until the next section or end of cfg */
			for (;;)
			{
				struct crcfile_var *v;

				buffer[0] = '\0';
                mame_fgets(buffer, sizeof (buffer), cfg->file);
				if (!buffer[0])
					return;

				p = strchr(buffer, '\n');
				if (!p)
					p = strchr(buffer, '\r');
				if (!p)
				{
					LOG(("crcfile_load_section: Line to long in section '%s'\n", searching));
					return;
				}

				*p = '\0';			   /* cut buffer here */
				p = strchr(buffer, '\n');	/* do we still have a CR? */
				if (p)
					*p = '\0';
				p = strchr(buffer, '\r');	/* do we still have a LF? */
				if (p)
					*p = '\0';

				if (*buffer == '[')    /* next section ? */
					return;

				if (*buffer == '\0' || /* empty line or comment ? */
					*buffer == '#' ||
					*buffer == ';')
					continue;

				/* find the crcfile_var data */
				p = strchr(buffer, '=');
				if (!p)
				{
					LOG(("crcfile_load_section: Line contains no '=' character\n"));
					return;
				}

				/* buffer = crcfile_var[.offs], p = data */
				*p++ = '\0';
				while (*p && isspace(*p))
					*p++ = '\0';
				/* is there an offs defined ? */
				d = strchr(buffer, '.');
				if (d)
				{
					/* buffer = crcfile_var, d = offs, p = data */
					*d++ = '\0';
					offs = xtoul(&d, NULL);
					if (offs)
					{
						v = cfg->list;
						while (v && findstr(v->name, buffer))
							v = v->next;
						if (!v)
						{
							LOG(("crcfile_load_section: Invalid variable continuation found '%s.%04X'\n", buffer, offs));
							return;
						}
					}
				}
				else
				{
					d = buffer + strlen(buffer);
					while( d >= buffer && isspace(*--d) )
						*d = '\0';
					offs = 0;
				}
				LOG(("crcfile_load_section: reading %s.%d=%s\n", buffer, offs, p));

                if (cfg->list)
				{
					/* next crcfile_var */

					/* avoids walking through linear list
					   nes.crc has over 8000 entrys
					   for nes 8000*(8000/2)=32000000 operations saved*/
					v=malloc(sizeof(struct crcfile_var));
					cfg->end_of_list->next=v;
					cfg->end_of_list=v;
				}
				else
				{
					/* first crcfile_var */
					cfg->list = malloc(sizeof (struct crcfile_var));
					cfg->end_of_list=cfg->list;
					v = cfg->list;
				}
				if (!v)
				{
					LOG(("crcfile_load_section: out of memory while reading '%s'\n", searching));
					return;
				}
				memset(v, 0, sizeof(struct crcfile_var));
                v->name = malloc(strlen(buffer) + 1);
				if (!v->name)
				{
					LOG(("crcfile_load_section: out of memory while reading '%s'\n", searching));
					return;
				}
				strcpy(v->name, buffer);
				v->size = 0;
				v->data = NULL;

				/* only xdigits and whitespace on this line? */
				if (xdigits_only(p))
				{
					/* convert the line back into data */
					data = xtoul(&p, &element_size);
					do
					{
						v->size++;
						/* need to allocate first/next chunk of memory? */
						if (v->size * element_size >= v->chunk)
						{
							v->chunk += CHUNK_SIZE;
							v->data = realloc(v->data, v->chunk);
						}
						/* check if the (re-)allocation failed */
						if (!v->data)
						{
							LOG(("crcfile_load_section: out of memory while reading '%s'\n", searching));
							return;
						}
						/* store element */
						switch (element_size)
						{
						case 1:
						    *((UINT8 *)v->data + v->size) = data; // v->size incremented before data stored???
						    break;
						case 2: // this is a fault for all cpus which force aligned memory accesses
						    *((UINT16 *)v->data + v->size) = data;
						    break;
						case 4: // this is a fault for all cpus which force aligned memory accesses
						    *((UINT32 *)v->data + v->size) = data;
						    break;
						}
						data = xtoul(&p, NULL);
					} while (*p);
                }
				else
				{
                    element_size = sizeof(char);
					do
					{
						data = ctoul(&p);
						/* need to allocate first/next chunk of memory? */
						if (v->size * element_size >= v->chunk)
						{
							v->chunk += CHUNK_SIZE;
							v->data = realloc(v->data, v->chunk);
						}
						/* check if the (re-)allocation failed */
						if (!v->data)
						{
							LOG(("crcfile_load_section: Out of memory while reading '%s'\n", searching));
							return;
						}
						*((UINT8 *)v->data + v->size) = data;
                        v->size++;
					} while (data);
                }
			}
		}
	}
}

/***************************************************************************
	crcfile_load_string
***************************************************************************/

void crcfile_load_string(crc_file *cfg, const char *section, int instance,
					   const char *name, char *dst, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		if( size > v->size )
			size = v->size;
		memcpy(dst, v->data, size - 1);
		dst[size-1] = '\0';
		LOG(("crcfile_load_string: '%s' is '%s'\n", name, dst));
    }
	else
	{
		LOG(("crcfile_load_string: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(dst, 0, size);
	}
}

/***************************************************************************
	crcfile_load_UINT8
***************************************************************************/

void crcfile_load_UINT8(crc_file *cfg, const char *section, int instance,
					   const char *name, UINT8 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((UINT8 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_UINT8: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size);
	}
}

/***************************************************************************
	crcfile_load_INT8
***************************************************************************/

void crcfile_load_INT8(crc_file *cfg, const char *section, int instance,
					  const char *name, INT8 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((INT8 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_INT8: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
        memset(val, 0, size);
	}
}

/***************************************************************************
	crcfile_load_UINT16
***************************************************************************/

void crcfile_load_UINT16(crc_file *cfg, const char *section, int instance,
						const char *name, UINT16 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((UINT16 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_UINT16: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 2);
	}
}

/***************************************************************************
	crcfile_load_INT16
***************************************************************************/

void crcfile_load_INT16(crc_file *cfg, const char *section, int instance,
					   const char *name, INT16 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((INT16 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_INT16: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
        memset(val, 0, size * 2);
	}
}

/***************************************************************************
	crcfile_load_UINT32
***************************************************************************/

void crcfile_load_UINT32(crc_file *cfg, const char *section, int instance,
						const char *name, UINT32 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((UINT32 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_UINT32: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 4);
	}
}

/***************************************************************************
	crcfile_load_INT32
***************************************************************************/

void crcfile_load_INT32(crc_file *cfg, const char *section, int instance,
					   const char *name, INT32 *val, unsigned size)
{
	struct crcfile_var *v;

	crcfile_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		unsigned offs;

		for (offs = 0; offs < size && offs < v->size; offs++)
			*val++ = *((INT32 *)v->data + offs);
	}
	else
	{
		LOG(("crcfile_load_INT32: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 4);
	}
}

