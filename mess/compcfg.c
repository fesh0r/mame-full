#include <stdio.h>
#include <ctype.h>
#include "snprintf.h"
#include "mess.h"

UINT32 ram_option(const struct GameDriver *gamedrv, unsigned int i)
{
	const struct ComputerConfigEntry *entry = gamedrv->compcfg;
	if (entry)
	{
		while(entry->type != CCE_END)
		{
			if ((entry->type == CCE_RAM) || (entry->type == CCE_RAM_DEFAULT))
			{
				/* found a RAM entry */
				if (i == 0)
					return entry->param;
				i--;
			}
			entry++;
		}
	}
	return 0;
}

UINT32 ram_default(const struct GameDriver *gamedrv)
{
	const struct ComputerConfigEntry *entry = gamedrv->compcfg;
	if (entry)
	{
		while(entry->type != CCE_END)
		{
			if (entry->type == CCE_RAM_DEFAULT)
				return entry->param;
			entry++;
		}
	}
	return 0;
}

int ram_option_count(const struct GameDriver *gamedrv)
{
	const struct ComputerConfigEntry *entry = gamedrv->compcfg;
	int count = 0;

	if (entry)
	{
		while(entry->type != CCE_END)
		{
			if ((entry->type == CCE_RAM) || (entry->type == CCE_RAM_DEFAULT))
				count++;
			entry++;
		}
	}
	return count;
}

int ram_is_valid_option(const struct GameDriver *gamedrv, UINT32 ram)
{
	const struct ComputerConfigEntry *entry = gamedrv->compcfg;

	if (entry)
	{
		while(entry->type != CCE_END)
		{
			if (entry->param == ram)
				return 1;
			entry++;
		}
	}
	return 0;
}

UINT32 ram_parse_string(const char *s)
{
	UINT32 ram;
	char suffix = '\0';

	s += sscanf(s, "%u%c", &ram, &suffix);
	switch(tolower(suffix)) {
	case 'k':
		/* kilobytes */
		ram *= 1024;
		break;

	case 'm':
		/* megabytes */
		ram *= 1024*1024;
		break;

	case '\0':
		/* no suffix */
		break;

	default:
		/* parse failure */
		ram = 0;
		break;
	}
	return ram;
}

const char *ram_string(char *buffer, UINT32 ram)
{
	const char *suffix;

	if ((ram % (1024*1024)) == 0)
	{
		ram /= 1024*1024;
		suffix = "m";
	}
	else if ((ram % 1024) == 0)
	{
		ram /= 1024;
		suffix = "k";
	}
	else
	{
		suffix = "";
	}
	sprintf(buffer, "%u%s", ram, suffix);
	return buffer;
}
