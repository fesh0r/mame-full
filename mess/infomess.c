/***************************************************************************

    infomess.c

    MESS-specific augmentation to info.c

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"

/* Print a free format string */
static const char *normalize_string(const char* s)
{
	static char buffer[1024];
	char *d = &buffer[0];

	if (s)
	{
		while (*s)
		{
			switch (*s)
			{
				case '\"' : d += sprintf(d, "&quot;"); break;
				case '&'  : d += sprintf(d, "&amp;"); break;
				case '<'  : d += sprintf(d, "&lt;"); break;
				case '>'  : d += sprintf(d, "&gt;"); break;
				default:
					if (*s>=' ' && *s<='~')
						*d++ = *s;
					else
						d += sprintf(d, "&#%d;", (unsigned)(unsigned char)*s);
			}
			++s;
		}
	}
	*d++ = 0;
	return buffer;
}

void print_game_device(FILE* out, const game_driver* game)
{
	const struct IODevice* dev;

	begin_resource_tracking();

	dev = devices_allocate(game);
	if (dev)
	{
		while(dev->type < IO_COUNT)
		{
			if (dev->must_be_loaded)
			{
				fprintf(out, "\t\t<device name=\"%s\" mandatory=\"1\">\n",
					normalize_string(device_typename(dev->type)));
			}
			else
			{
				fprintf(out, "\t\t<device name=\"%s\">\n",
					normalize_string(device_typename(dev->type)));
			}

			if (dev->file_extensions)
			{
				const char* ext = dev->file_extensions;
				while (*ext)
				{
					fprintf(out, "\t\t\t<extension");
					fprintf(out, " name=\"%s\"", normalize_string(ext));
					fprintf(out, "/>\n");
					ext += strlen(ext) + 1;
				}
			}

			fprintf(out, "\t\t</device>\n");

			dev++;
		}
	}
	end_resource_tracking();
}

void print_game_ramoptions(FILE* out, const game_driver* game)
{
	int i, count;
	UINT32 ram;

	count = ram_option_count(game);

	for (i = 0; i < count; i++)
	{
		ram = ram_option(game, i);
		fprintf(out, "\t\t<ramoption>%u</ramoption>\n", ram);
	}
}
