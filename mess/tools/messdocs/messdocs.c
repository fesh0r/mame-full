#include <stdio.h>
#include "pool.h"
#include "sys/stat.h"
#include "utils.h"

#ifdef _MSC_VER
#include "ui/dirent.h"
#else
#include <dirent.h>
#endif

#if 0
/* Include the internal copy of the libexpat library */
#define ELEMENT_TYPE ELEMENT_TYPE_
#include "xml2info/xmlrole.c"
#include "xml2info/xmltok.c"
#include "xml2info/xmlparse.c"
#undef ELEMENT_TYPE
#else
#define XMLPARSEAPI(type) type
#include "xml2info/expat.h"
#endif

struct messdocs_state
{
	const char *dest_dir;
	memory_pool pool;
	XML_Parser parser;
	int depth;
	int error;
	char *toc_dir;
	FILE *chm_toc;
};


static void process_error(struct messdocs_state *state, const char *tag, const char *msgfmt, ...)
{
	va_list va;
	char buf[512];
	const char *msg;

	if (msgfmt)
	{
		va_start(va, msgfmt);
		vsprintf(buf, msgfmt, va);
		va_end(va);
		msg = buf;
	}
	else
	{
		msg = XML_ErrorString(XML_GetErrorCode(state->parser));
	}

	fprintf(stderr, "%d:%s:%s\n", XML_GetCurrentLineNumber(state->parser), tag ? tag : "", msg);
	state->error = 1;
}


static const XML_Char *find_attribute(const XML_Char **attributes, const XML_Char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



static void combine_path(char *buf, size_t buflen, const char *relpath)
{
	size_t i;

	for (i = 0; buf[i]; i++)
	{
		if (osd_is_path_separator(buf[i]))
			buf[i] = PATH_SEPARATOR;
	}
	
	while(osd_is_path_separator(buf[--i]))
		;
	i++;
	buf[i++] = PATH_SEPARATOR;
	buf[i] = '\0';

	while(relpath[0])
	{
		/* skip over './' constructs */
		while ((relpath[0] == '.') && osd_is_path_separator(relpath[1]))
			relpath += 2;

		while((relpath[0] == '.') && (relpath[1] == '.') && osd_is_path_separator(relpath[2]))
		{
			while(i && osd_is_path_separator(buf[--i]))
				buf[i] = '\0';
			while(i && !osd_is_path_separator(buf[i]))
				buf[i--] = '\0';
			relpath += 3;
		}

		snprintf(&buf[i], buflen - i, "%s", relpath);
		relpath += strlen(&buf[i]);
		i += strlen(&buf[i]);
	}
}



static char *make_path(const char *dir, const char *filepath)
{
	char buf[1024];
	char *path;

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s", dir);
	combine_path(buf, sizeof(buf) / sizeof(buf[0]), filepath);

	path = malloc(strlen(buf) + 1);
	if (!path)
		return NULL;
	strcpy(path, buf);
	return path;
}



static void copy_file_to_dest(const char *dest_dir, const char *src_dir, const char *filepath)
{
	char *dest_path = NULL;
	char *src_path = NULL;
	int i;

	/* create dest path */
	dest_path = make_path(dest_dir, filepath);
	if (!dest_path)
		goto done;

	/* create source path */
	src_path = make_path(src_dir, filepath);
	if (!src_path)
		goto done;

	/* create partial directories */
	for (i = strlen(dest_dir) + 1; dest_path[i]; i++)
	{
		if (osd_is_path_separator(dest_path[i]))
		{
			dest_path[i] = '\0';
			osd_mkdir(dest_path);
			dest_path[i] = PATH_SEPARATOR;
		}
	}

	osd_copyfile(dest_path, src_path);

done:
	if (dest_path)
		free(dest_path);
	if (src_path)
		free(src_path);
}



static void html_encode(char *buf, size_t len)
{
	/* NYI */
}



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
	const char *title;
	const char *name;
	const char *filepath;
	const char *srcpath;
	const char *destpath;
	char buf[512];
	char sysname[512];
	char sysfilename[512];
	char *datfile_path = NULL;
	char *s;
	FILE *datfile = NULL;
	FILE *sysfile = NULL;
	int lineno = 0;

	struct messdocs_state *state = (struct messdocs_state *) data;

	if (state->depth == 0)
	{
		/* help tag */
		if (strcmp(tagname, "help"))
		{
			process_error(state, NULL, "Expected tag 'help'");
			return;
		}

		title = find_attribute(attributes, "title");
	}
	else if (!strcmp(tagname, "topic"))
	{
		/* topic tag */
		name = find_attribute(attributes, "text");
		filepath = find_attribute(attributes, "filepath");
		fprintf(state->chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
		fprintf(state->chm_toc, "\t\t<param name=\"Name\"  value=\"%s\">\n", name);
		fprintf(state->chm_toc, "\t\t<param name=\"Local\" value=\"%s\">\n", filepath);	
		fprintf(state->chm_toc, "\t\t</OBJECT>\n");
		copy_file_to_dest(state->dest_dir, state->toc_dir, filepath);
	}
	else if (!strcmp(tagname, "folder"))
	{
		/* folder tag */
		name = find_attribute(attributes, "text");
		fprintf(state->chm_toc, "\t<LI> <OBJECT type=\"text/sitemap\">\n");
		fprintf(state->chm_toc, "\t\t<param name=\"Name\" value=\"%s\">\n", name);
		fprintf(state->chm_toc, "\t\t</OBJECT>\n");
		fprintf(state->chm_toc, "\t\t<UL>\n");
	}
	else if (!strcmp(tagname, "file"))
	{
		/* file tag */
		filepath = find_attribute(attributes, "filepath");
		copy_file_to_dest(state->dest_dir, state->toc_dir, filepath);
	}
	else if (!strcmp(tagname, "datfile"))
	{
		/* datfile tag */
		srcpath = find_attribute(attributes, "srcpath");
		destpath = find_attribute(attributes, "destpath");

		datfile_path = make_path(state->toc_dir, srcpath);
		datfile = fopen(datfile_path, "r");
		if (!datfile)
		{
			process_error(state, NULL, "Cannot open datfile '%s'\n", datfile_path);
			return;
		}

		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s", state->dest_dir);
		combine_path(buf, sizeof(buf) / sizeof(buf[0]), destpath);
		osd_mkdir(buf);
		sysname[0] = '\0';

		while(!feof(datfile))
		{
			fgets(buf, sizeof(buf) / sizeof(buf[0]), datfile);
			s = strchr(buf, '\n');
			if (s)
				*s = '\0';
			rtrim(buf);

			switch(buf[0]) {
			case '$':
				if (!strncmp(buf, "$info", 5))
				{
					/* $info */
					s = strchr(buf, '=');
					s = s ? s + 1 : &buf[strlen(buf)];
					_snprintf(sysname, sizeof(sysname), "%s", s);
					_snprintf(sysfilename, sizeof(sysfilename), "%s%c%s%c%s.htm", state->dest_dir, PATH_SEPARATOR, destpath, PATH_SEPARATOR, s);

					if (sysfile)
						fclose(sysfile);
					sysfile = fopen(sysfilename, "w");
					lineno = 0;
				}
				else if (!strncmp(buf, "$bio", 4))
				{
					/* $bio */
				}
				else if (!strncmp(buf, "$end", 4))
				{
					/* $end */
				}
				break;
			case '#':
			case '\0':
				/* comments */
				break;
			default:
				/* normal line */
				if (!sysfile)
					break;

				html_encode(buf, sizeof(buf) / sizeof(buf[0]));
				if (lineno == 0)
				{
					fprintf(sysfile, "<h2>%s</h2>\n", buf);
					fprintf(sysfile, "<p><i>(directory: %s)</i></p>\n", sysname);
				}
				else if (buf[strlen(buf)-1] == ':')
				{
					fprintf(sysfile, "<h3>%s</h3>\n", buf);
				}
				else
				{
					fprintf(sysfile, "%s\n", buf);
				}
				lineno++;
				break;
			}
		}
	}

	state->depth++;

	if (datfile_path)
		free(datfile_path);
	if (datfile)
		fclose(datfile);
	if (sysfile)
		fclose(sysfile);
}



static void end_handler(void *data, const XML_Char *name)
{
	struct messdocs_state *state = (struct messdocs_state *) data;

	state->depth--;

	if (!strcmp(name, "folder"))
	{
		fprintf(state->chm_toc, "\t<UL>\n");
	}
}



static void data_handler(void *data, const XML_Char *s, int len)
{
}



static int rmdir_recursive(const char *dir_path)
{
	DIR *dir;
	struct dirent *ent;
	struct stat s;
	char *newpath;
	char path_sep[2];

	path_sep[0] = PATH_SEPARATOR;
	path_sep[1] = '\0';

	dir = opendir(dir_path);
	if (dir)
	{
		while((ent = readdir(dir)) != NULL)
		{
			if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
			{
				newpath = malloc(strlen(dir_path) + 1 + strlen(ent->d_name) + 1);
				if (!newpath)
					return -1;

				strcpy(newpath, dir_path);
				strcat(newpath, path_sep);
				strcat(newpath, ent->d_name);

				stat(newpath, &s);
				if (S_ISDIR(s.st_mode))
					rmdir_recursive(newpath);
				else
					osd_rmfile(newpath);

				free(newpath);
			}
		}
		closedir(dir);
	}
	osd_rmdir(dir_path);
	return 0;
}



int messdocs(const char *toc_filename, const char *dest_dir)
{
	char buf[4096];
	struct messdocs_state state;
	int len;
	int done;
	FILE *in;
	FILE *chm_hhp;
	int i;
	const char *help_project_filename = "help.hhp";
	const char *help_contents_filename = "help.hhc";
	char *s;
	char path_sep[2];

	path_sep[0] = PATH_SEPARATOR;
	path_sep[1] = '\0';

	memset(&state, 0, sizeof(state));
	pool_init(&state.pool);

	/* open the DOC */
	in = fopen(toc_filename, "r");
	if (!in)
	{
		fprintf(stderr, "Cannot open TOC file '%s'\n", toc_filename);
		goto error;
	}

	/* figure out the TOC directory */
	state.toc_dir = pool_strdup(&state.pool, toc_filename);
	if (!state.toc_dir)
		goto outofmemory;
	for (i = strlen(state.toc_dir) - 1; (i > 0) && !osd_is_path_separator(state.toc_dir[i]); i--)
		state.toc_dir[i] = '\0';

	/* clean the target directory */
	rmdir_recursive(dest_dir);
	osd_mkdir(dest_dir);

	/* create the help project file */
	s = pool_malloc(&state.pool, strlen(dest_dir) + 1 + strlen(help_project_filename) + 1);
	if (!s)
		goto outofmemory;
	strcpy(s, dest_dir);
	strcat(s, path_sep);
	strcat(s, help_project_filename);
	chm_hhp = fopen(s, "w");
	if (!chm_hhp)
	{
		fprintf(stderr, "Cannot open file %s\n", s);
		goto error;
	}

	fprintf(chm_hhp, "[OPTIONS]\n");
	fprintf(chm_hhp, "Compiled file=NYI.chm");
	fprintf(chm_hhp, "Contents file=%s", help_contents_filename);
	fprintf(chm_hhp, "Default topic=NYI\n");
	fprintf(chm_hhp, "Language=0x409 English (United States)");
	fprintf(chm_hhp, "Title=NYI\n");
	fprintf(chm_hhp, "\n");
	fclose(chm_hhp);

	/* create the help contents file */
	s = pool_malloc(&state.pool, strlen(dest_dir) + 1 + strlen(help_project_filename) + 1);
	if (!s)
		goto outofmemory;
	strcpy(s, dest_dir);
	strcat(s, path_sep);
	strcat(s, help_contents_filename);
	state.chm_toc = fopen(s, "w");
	state.dest_dir = dest_dir;
	if (!state.chm_toc)
	{
		fprintf(stderr, "Cannot open file %s\n", s);
		goto error;
	}

	fprintf(state.chm_toc, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\"\n");
	fprintf(state.chm_toc, "<HTML>\n");
	fprintf(state.chm_toc, "<HEAD>\n");
	fprintf(state.chm_toc, "</HEAD>\n");
	fprintf(state.chm_toc, "<BODY>\n");
	fprintf(state.chm_toc, "<OBJECT type=\"text/site properties\">\n");
	fprintf(state.chm_toc, "\t<param name=\"Window Styles\" value=\"0x800625\">\n");
	fprintf(state.chm_toc, "\t<param name=\"ImageType\" value=\"Folder\">\n");
	fprintf(state.chm_toc, "\t<param name=\"Font\" value=\"Arial,8,0\">\n");
	fprintf(state.chm_toc, "</OBJECT>\n");
	fprintf(state.chm_toc, "<UL>\n");

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser)
		goto outofmemory;

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);
		
		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR)
		{
			process_error(&state, NULL, NULL);
			break;
		}
	}
	while(!done);

	fprintf(state.chm_toc, "</UL>\n");
	fprintf(state.chm_toc, "</BODY></HTML>");
	fclose(state.chm_toc);
	pool_exit(&state.pool);

	return state.error ? -1 : 0;

outofmemory:
	fprintf(stderr, "Out of memory");
error:
	return -1;
}



int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s [tocxml] [destdir]\n", argv[0]);
		return -1;
	}

	return messdocs(argv[1], argv[2]);
}



