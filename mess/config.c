/* Config file save/load functions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include "mess/config.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

/* A forward linked list of the contents of a section */
struct config_var {
	struct config_var *next;
	char *name;
	unsigned size;
	unsigned chunk;
	void *data;
};

/* our config handling structure */
struct config_hdl {
	FILE *file;
	const char *section;
	int instance;
	struct config_var *list;
};

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

INLINE int xdigits_only(const char *src)
{
	while (*src && (isxdigit(*src) || isspace(*src)))
		src++;
	return (*src) ? 0 : 1;
}

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

/* free a linked list of state_vars (aka section) */
void config_free_section(void *config)
{
	struct config_hdl *cfg = (struct config_hdl *) config;
	struct config_var *this, *next;

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

void *config_create(const char *name)
{
	struct config_hdl *cfg = NULL;

	if (!name || !*name)
		return cfg;
    cfg = (struct config_hdl *) malloc(sizeof (struct config_hdl));
	if (!cfg)
	{
		LOG((errorlog, "config_create: memory problem\n"));
		return cfg;
	}
	memset(cfg, 0, sizeof(struct config_hdl));
	cfg->file = fopen(name, "w");
	if (!cfg->file)
	{
		LOG((errorlog, "config_create: couldn't create file '%s'\n", name));
		free(cfg);
		return NULL;
	}
	LOG((errorlog, "config_create: created file '%s'\n", name));
    return cfg;
}

void *config_open(const char *name)
{
	struct config_hdl *cfg = NULL;

	if (!name || !*name)
		return cfg;

    cfg = (struct config_hdl *) malloc(sizeof (struct config_hdl));
	if (!cfg)
	{
		LOG((errorlog, "config_open: memory problem\n"));
		return cfg;
	}
	memset(cfg, 0, sizeof(struct config_hdl));
	cfg->file = fopen(name, "r");
	if (!cfg->file)
	{
		LOG((errorlog, "config_open: couldn't open file '%s'\n", name));
        free(cfg);
		return NULL;
	}
	LOG((errorlog, "config_open: opened file '%s'\n", name));
    return cfg;
}

void config_close(void *config)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

	if (!cfg)
		return;
	config_free_section(cfg);
	if (cfg->file)
		fclose(cfg->file);
	free(cfg);
}

/* Output a formatted string to the config file */
static void CLIB_DECL config_printf(void *config, const char *fmt,...)
{
	static char buffer[255 + 1];
	struct config_hdl *cfg = (struct config_hdl *) config;
	va_list arg;
	int length;

	va_start(arg, fmt);
	length = vsprintf(buffer, fmt, arg);
	va_end(arg);

	if (fwrite(buffer, 1, length, cfg->file) != length)
	{
		LOG((errorlog, "config_printf: Error while saving cfg '%s'\n", buffer));
	}
}

void config_save_section(void *config, const char *section, int instance)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

	if (!cfg->section ||
		(cfg->section && findstr(cfg->section, section)) ||
		cfg->instance != instance)
	{
		if (cfg->section)
			config_printf(cfg, "\n");
		cfg->section = section;
		cfg->instance = instance;
		if (instance)
			config_printf(cfg, "[%s.%d]\n", section, instance);
		else
			config_printf(cfg, "[%s]\n", section);
	}
}

void config_save_string(void *config, const char *section, int instance,
					   const char *name, const char *src)
{
	struct config_hdl *cfg = (struct config_hdl *) config;
	unsigned size = strlen(src);

    config_save_section(cfg, section, instance);

	config_printf(cfg, "%s=", name);
	while (size-- > 0)
	{
		switch (*src)
		{
            case '\\':
			case '[':
			case ']':
				config_printf(cfg, "\\%c", *src++);
				break;
			default:
				if (*src < 32 || *src == '\t' || *src == '\\' || *src == '[' || *src == ']' )
					config_printf(cfg, "\\%o", *src++);
				else
					config_printf(cfg, "%c", *src++);
		}
	}
	config_printf(cfg, "\n");
}

void config_save_UINT8(void *config, const char *section, int instance,
					   const char *name, const UINT8 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

	config_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 16)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 15) == 0)
				config_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			config_printf(cfg, "%s", ultox(*val++, 2));
			if ((++offs & 15) == 0)
				config_printf(cfg, "\n");
			else
				config_printf(cfg, " ");
		}
		if (offs & 15)
			config_printf(cfg, "\n");
	}
	else
	{
		config_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			config_printf(cfg, "%s", ultox(*val++, 2));
			if (size)
				config_printf(cfg, " ");
		}
		config_printf(cfg, "\n");
	}
}

void config_save_INT8(void *config, const char *section, int instance,
					  const char *name, const INT8 *val, unsigned size)
{
	config_save_UINT8(config, section, instance, name, (UINT8 *)val, size);
}

void config_save_UINT16(void *config, const char *section, int instance,
						const char *name, const UINT16 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

	config_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 8)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 7) == 0)
				config_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			config_printf(cfg, "%s", ultox(*val++, 4));
			if ((++offs & 7) == 0)
				config_printf(cfg, "\n");
			else
				config_printf(cfg, " ");
		}
		if (offs & 7)
			config_printf(cfg, "\n");
	}
	else
	{
		config_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			config_printf(cfg, "%s", ultox(*val++, 4));
			if (size)
				config_printf(cfg, " ");
		}
		config_printf(cfg, "\n");
	}
}

void config_save_INT16(void *config, const char *section, int instance,
					   const char *name, const INT16 *val, unsigned size)
{
	config_save_UINT16(config, section, instance, name, (UINT16 *)val, size);
}

void config_save_UINT32(void *config, const char *section, int instance,
						const char *name, const UINT32 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

	config_save_section(cfg, section, instance);

	/* If next is to much for a single line use the dump format */
	if (size > 4)
	{
		unsigned offs = 0;

		while (size-- > 0)
		{
			if ((offs & 3) == 0)
				config_printf(cfg, "%s.%s=", name, ultox(offs, 4));
			config_printf(cfg, "%s", ultox(*val++, 8));
			if ((++offs & 3) == 0)
				config_printf(cfg, "\n");
			else
				config_printf(cfg, " ");
		}
		if (offs & 3)
			config_printf(cfg, "\n");
	}
	else
	{
		config_printf(cfg, "%s=", name);
		while (size-- > 0)
		{
			config_printf(cfg, "%s", ultox(*val++, 8));
			if (size)
				config_printf(cfg, " ");
		}
		config_printf(cfg, "\n");
	}
}

void config_save_INT32(void *config, const char *section, int instance,
					   const char *name, const INT32 *val, unsigned size)
{
	config_save_UINT32(config, section, instance, name, (UINT32 *)val, size);
}

/* load a linked list of config_vars (aka section) */
void config_load_section(void *config, const char *section, int instance)
{
	struct config_hdl *cfg = (struct config_hdl *) config;

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
		config_free_section(cfg);

	if (instance)
		sprintf(searching, "[%s.%d]", section, instance);
	else
		sprintf(searching, "[%s]", section);
	LOG((errorlog, "config_load_section: searching for '%s'\n", searching));

	for (;;)
	{
		buffer[0] = '\0';
		fgets(buffer, sizeof(buffer), cfg->file);
		if (!buffer[0])
			return;
		if (findstr(buffer, searching) == 0)
		{
			cfg->section = section;
			cfg->instance = instance;
			/* now read all config_vars until the next section or end of cfg */
			for (;;)
			{
				struct config_var *v;

				buffer[0] = '\0';
                fgets(buffer, sizeof (buffer), cfg->file);
				if (!buffer[0])
					return;

				p = strchr(buffer, '\n');
				if (!p)
					p = strchr(buffer, '\r');
				if (!p)
				{
					LOG((errorlog, "config_load_section: Line to long in section '%s'\n", searching));
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

				/* find the config_var data */
				p = strchr(buffer, '=');
				if (!p)
				{
					LOG((errorlog, "config_load_section: Line contains no '=' character\n"));
					return;
				}

				/* buffer = config_var[.offs], p = data */
				*p++ = '\0';
				while (*p && isspace(*p))
					*p++ = '\0';
				/* is there an offs defined ? */
				d = strchr(buffer, '.');
				if (d)
				{
					/* buffer = config_var, d = offs, p = data */
					*d++ = '\0';
					offs = xtoul(&d, NULL);
					if (offs)
					{
						v = cfg->list;
						while (v && findstr(v->name, buffer))
							v = v->next;
						if (!v)
						{
							LOG((errorlog, "config_load_section: Invalid variable continuation found '%s.%04X'\n", buffer, offs));
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
				LOG((errorlog, "config_load_section: reading %s.%d=%s\n", buffer, offs, p));

                if (cfg->list)
				{
					/* next config_var */
					v = cfg->list;
					while (v->next)
						v = v->next;
					v->next = malloc(sizeof (struct config_var));
					v = v->next;
				}
				else
				{
					/* first config_var */
					cfg->list = malloc(sizeof (struct config_var));
					v = cfg->list;
				}
				if (!v)
				{
					LOG((errorlog, "config_load_section: out of memory while reading '%s'\n", searching));
					return;
				}
				memset(v, 0, sizeof(struct config_var));
                v->name = malloc(strlen(buffer) + 1);
				if (!v->name)
				{
					LOG((errorlog, "config_load_section: out of memory while reading '%s'\n", searching));
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
							if (v->data)
								v->data = realloc(v->data, v->chunk);
							else
								v->data = malloc(v->chunk);
						}
						/* check if the (re-)allocation failed */
						if (!v->data)
						{
							LOG((errorlog, "config_load_section: out of memory while reading '%s'\n", searching));
							return;
						}
						/* store element */
						switch (element_size)
						{
						case 1:
							*((UINT8 *)v->data + v->size) = data;
						case 2:
							*((UINT16 *)v->data + v->size) = data;
						case 4:
							*((UINT32 *)v->data + v->size) = data;
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
							if (v->data)
								v->data = realloc(v->data, v->chunk);
							else
								v->data = malloc(v->chunk);
						}
						/* check if the (re-)allocation failed */
						if (!v->data)
						{
							LOG((errorlog, "config_load_section: Out of memory while reading '%s'\n", searching));
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

void config_load_string(void *s, const char *section, int instance,
					   const char *name, char *dst, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

	v = cfg->list;
	while (v && findstr(v->name, name))
		v = v->next;

	if (v)
	{
		if( size > v->size )
			size = v->size;
		memcpy(dst, v->data, size - 1);
		dst[size-1] = '\0';
		LOG((errorlog, "config_load_string: '%s' is '%s'\n", name, dst));
    }
	else
	{
		LOG((errorlog, "config_load_string: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(dst, 0, size);
	}
}

void config_load_UINT8(void *s, const char *section, int instance,
					   const char *name, UINT8 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_UINT8: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size);
	}
}

void config_load_INT8(void *s, const char *section, int instance,
					  const char *name, INT8 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_INT8: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
        memset(val, 0, size);
	}
}

void config_load_UINT16(void *s, const char *section, int instance,
						const char *name, UINT16 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_UINT16: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 2);
	}
}

void config_load_INT16(void *s, const char *section, int instance,
					   const char *name, INT16 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_INT16: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
        memset(val, 0, size * 2);
	}
}

void config_load_UINT32(void *s, const char *section, int instance,
						const char *name, UINT32 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_UINT32: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 4);
	}
}

void config_load_INT32(void *s, const char *section, int instance,
					   const char *name, INT32 *val, unsigned size)
{
	struct config_hdl *cfg = (struct config_hdl *) s;
	struct config_var *v;

	config_load_section(cfg, section, instance);

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
		LOG((errorlog, "config_load_INT32: variable '%s' not found in section '%s' instance %d\n", name, section, instance));
		memset(val, 0, size * 4);
	}
}

