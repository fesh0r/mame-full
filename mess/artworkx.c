/*********************************************************************

	artworkx.c

	MESS specific artwork code

*********************************************************************/

#include <ctype.h>

#include "artworkx.h"
#include "image.h"
#include "png.h"


/***************************************************************************

	Constants & macros

***************************************************************************/

#define PNG_FILE		"ctrlr.png"
#define INI_FILE		"ctrlr.ini"

/***************************************************************************

	Local variables

***************************************************************************/

static char *override_artfile;



/**************************************************************************/

void artwork_use_device_art(mess_image *img, const char *defaultartfile)
{
	const char *fname;
	const char *ext;
	int len = -1;

	fname = image_basename(img);
	if (fname)
	{
		ext = strrchr(fname, '.');
		if (ext)
			len = ext - fname;
	}

	override_artfile = malloc((fname ? len + 1 : 0) + strlen(defaultartfile) + 1 + 1);
	if (!override_artfile)
		return;

	if (fname)
	{
		memcpy(override_artfile, fname, len);
		override_artfile[len] = 0;
	}

	strcpy(&override_artfile[len + 1], defaultartfile);
	override_artfile[len + 1 + strlen(defaultartfile) + 1] = '\0';
}

static int mess_activate_artwork(struct osd_create_params *params)
{
	if ((params->width < options.min_width) && (params->height < options.min_height))
	{
		options.artwork_res = 2;
		return TRUE;
	}
	return FALSE;
}

static mame_file *mess_load_artwork_file(const struct GameDriver **driver)
{
	char filename[2048];
	mame_file *artfile = NULL;
	const char *s;

	while (*driver)
	{
		if ((*driver)->name)
		{
			s = override_artfile ? override_artfile : "";
			do
			{
				sprintf(filename, "%s.art", *s ? s : (*driver)->name);
				if (*s)
					s += strlen(s) + 1;
				artfile = mame_fopen((*driver)->name, filename, FILETYPE_ARTWORK, 0);
			}
			while(!artfile && *s);
			if (artfile)
				break;
		}
		*driver = (*driver)->clone_of;
	}
	return artfile;
}

struct artwork_callbacks mess_artwork_callbacks =
{
	mess_activate_artwork,
	mess_load_artwork_file
};

/********************************************************************/

/*-------------------------------------------------
	strip_space - strip leading/trailing spaces
-------------------------------------------------*/

static char *strip_space(char *string)
{
	char *start, *end;

	/* skip over leading space */
	for (start = string; *start && isspace(*start); start++) ;

	/* NULL terminate over trailing space */
	for (end = start + strlen(start) - 1; end > start && isspace(*end); end--) *end = 0;
	return start;
}

void artwork_get_inputscreen_customizations(struct png_info *png,
	struct inputform_customization *customizations, int customizations_length)
{
	mame_file *file;
	char buffer[1000];
	char ipt_name[64];
	char *p;
	int x1, y1, x2, y2;
	struct ik *pik;
	const char *pik_name;

	/* subtract one from the customizations length; so we can place IPT_END */
	customizations_length--;

	/* open the PNG, if available */
	memset(png, 0, sizeof(*png));
	file = mame_fopen(Machine->gamedrv->name, PNG_FILE, FILETYPE_ARTWORK, 0);
	if (file)
	{
		png_read_file(file, png);
		mame_fclose(file);
	}

	/* open the INI file, if available */
	file = mame_fopen(Machine->gamedrv->name, INI_FILE, FILETYPE_ARTWORK, 0);
	if (file)
	{
		/* loop until we run out of lines */
		while (customizations_length && mame_fgets(buffer, sizeof(buffer), file))
		{
			/* strip off any comments */
			p = strstr(buffer, "//");
			if (p)
				*p = 0;

			if (sscanf(buffer, "%64s (%d,%d)-(%d,%d)", ipt_name, &x1, &y1, &x2, &y2) != 5)
				continue;

			for (pik = input_keywords; pik->name[0]; pik++)
			{
				pik_name = pik->name;
				if ((pik_name[0] == 'P') && (pik_name[1] == '1') && (pik_name[2] == '_'))
					pik_name += 3;

				if (!strcmp(ipt_name, pik_name))
				{
					if ((x1 > 0) && (y1 > 0) && (x2 > x1) && (y2 > y1))
					{
						customizations->ipt = pik->val;
						customizations->x = x1;
						customizations->y = y1;
						customizations->width = x2 - x1;
						customizations->height = y2 - y1;
						customizations++;				
						customizations_length--;
					}
					break;
				}
			}
		}
		mame_fclose(file);
	}

	/* terminate list */
	customizations->ipt = IPT_END;
	customizations->x = -1;
	customizations->y = -1;
	customizations->width = -1;
	customizations->height = -1;
}
