#include <ctype.h>
#include "imgtool.h"
#include "main.h"

#ifdef MAME_DEBUG

int cmd_testsuite(struct command *c, int argc, char *argv[])
{
	const char *testsuitefile;
	FILE *inifile;
	const struct ImageModule *module = NULL;
	char buffer[1024];
	char buffer2[1024];
	char filename[1024];
	char *filename_base;
	size_t filename_len;
	char *s;
	char *s2;
	const char *directive;
	const char *directive_value;
	IMAGE *img = NULL;
	IMAGEENUM *imgenum;
	imgtool_dirent imgdirent;
	int err = -1, i;
	struct disk_geometry geometry;

	testsuitefile = argv[0];

	/* open the testsuite file */
	inifile = fopen(testsuitefile, "r");
	if (!inifile)
	{
		fprintf(stderr, "*** cannot open testsuite file '%s'\n", testsuitefile);
		goto error;
	}

	/* change to the current directory of the test suite */
	strncpyz(filename, testsuitefile, sizeof(filename));
	s = filename + strlen(filename) - 1;
	while((*s != '/') && (*s != '\\'))
		*(s--) = '\0';
	filename_base = s+1;
	filename_len = filename_base - filename;

	while(!feof(inifile))
	{
		buffer[0] = '\0';
		fgets(buffer, sizeof(buffer) / sizeof(buffer[0]), inifile);

		if (buffer[0] != '\0')
		{
			s = buffer + strlen(buffer) -1;
			while(isspace(*s))
				*(s--) = '\0';
		}
		s = buffer;
		while(isspace(*s))
			s++;

		if (*s == '[')
		{
			s2 = strchr(s, ']');
			if (!s2)
				goto syntaxerror;
			*s2 = '\0';
			module = findimagemodule(s+1);
			if (!module)
			{
				fprintf(stderr, "*** unrecognized imagemodule '%s'\n", s+1);
				goto error;
			}
			imgtool_test(module);
		}
		else if (isalpha(*s))
		{
			directive = s;
			while(isalpha(*s))
				s++;
			while(isspace(*s))
				*(s++) = '\0';

			if (*s != '=')
			{
				fprintf(stderr, "*** expected '='\n");
				goto error;
			}
			*(s++) = '\0';
			while(isspace(*s))
				s++;
			directive_value = s;

			if (!strcmpi(directive, "imagefile"))
			{
				/* imagefile directive */
				if (img)
					img_close(img);
				strncpyz(filename_base, directive_value, filename_len);
				err = img_open(module, filename, OSD_FOPEN_READ, &img);
				if (err)
					goto error;

				fprintf(stdout, "%s\n", filename);
			}
			else if (!strcmpi(directive, "files"))
			{
				/* files directive */
				if (!img)
					goto needimgmodule;
				err = img_beginenum(img, &imgenum);
				if (err)
					goto error;

				memset(&imgdirent, 0, sizeof(imgdirent));
				imgdirent.fname = buffer2;
				imgdirent.fname_len = sizeof(buffer2);
				while(((err = img_nextenum(imgenum, &imgdirent)) != 0) && imgdirent.fname[0])
				{
					i = strlen(buffer2);
					buffer[i++] = ',';
					imgdirent.fname = &buffer[i];
					imgdirent.fname_len = sizeof(buffer2) - i;
				}
				img_closeenum(imgenum);
				if (err)
					goto error;
				i = strlen(buffer2);
				if (i > 0)
					buffer2[i-1] = '\0';

				if (strcmp(directive_value, buffer2))
				{
					fprintf(stderr, "*** expected files '%s', but instead got '%s'", directive_value, buffer2);
					goto error;
				}
			}
			else if (!strcmpi(directive, "geometry"))
			{
				/* geometry directive */
				img_get_geometry(img, &geometry);
				snprintf(buffer2, sizeof(buffer2), "%i-%i/%i/%i-%i/%i", 
					0,	(int) geometry.tracks-1,
					(int) geometry.heads,
					(int) geometry.first_sector_id,
					(int) geometry.sectors - 1 + geometry.first_sector_id,
					(int) geometry.sector_size);

				if (strcmp(directive_value, buffer2))
				{
					fprintf(stderr, "*** expected geometry '%s', but instead got '%s'", directive_value, buffer2);
					goto error;
				}
			}
			else
			{
				fprintf(stderr, "*** unrecognized directive '%s'\n", directive);
				goto done;
			}
		}
	}
	err = 0;
	goto done;

needimgmodule:
	fprintf(stderr, "*** need [format] declaration before any directives\n");
	goto error;

syntaxerror:
	fprintf(stderr, "*** syntax error: \n", buffer);
	goto error;

error:
	if (err)
		reporterror(err, c, module ? module->name : NULL, filename, NULL, NULL, NULL);

done:
	if (inifile)
		fclose(inifile);
	if (img)
		img_close(img);
	return err;
}

#endif /* MAME_DEBUG */
