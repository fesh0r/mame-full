#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "snprintf.h"
#include "mess.h"

#define MAX_RAM_OPTIONS	16

static int get_ram_options(const struct GameDriver *gamedrv, UINT32 *ram_options, int max_options, int *default_option)
{
	struct SystemConfigurationParamBlock params;

	memset(&params, 0, sizeof(params));
	params.max_ram_options = max_options;
	params.ram_options = ram_options;

	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&params);

	if (default_option)
		*default_option = params.default_ram_option;
	return params.actual_ram_options;
}

UINT32 ram_option(const struct GameDriver *gamedrv, unsigned int i)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);
	return i >= ram_optcount ? 0 : ram_options[i];
}

UINT32 ram_default(const struct GameDriver *gamedrv)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;
	int default_ram_option;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), &default_ram_option);
	return default_ram_option >= ram_optcount ? 0 : ram_options[default_ram_option];
}

int ram_option_count(const struct GameDriver *gamedrv)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	return get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);
}

int ram_is_valid_option(const struct GameDriver *gamedrv, UINT32 ram)
{
	UINT32 ram_options[MAX_RAM_OPTIONS];
	int ram_optcount;
	int i;

	ram_optcount = get_ram_options(gamedrv, ram_options, sizeof(ram_options) / sizeof(ram_options[0]), NULL);

	for(i = 0; i < ram_optcount; i++)
		if (ram_options[i] == ram)
			return 1;
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
