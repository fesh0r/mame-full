#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int ac, char **av)
{
	char dat_filename[128] = "sysinfo.dat";
	char html_filename[128] = "sysinfo.htm";
	char html_directory[128] = "sysinfo";
	char system_filename[128] = "";
	char system_name[128] = "";
	FILE *dat, *html, *html_system = NULL;
	time_t tm;

	tm = time(NULL);

    if( ac > 1 )
	{
		strcpy(dat_filename, av[1]);
		if( ac > 2 )
		{
			strcpy(html_filename, av[2]);
			if( ac > 3 )
			{
				strcpy(html_directory, av[3]);
			}
		}
	}

    dat = fopen(dat_filename, "r");
	if( !dat )
	{
		fprintf(stderr, "cannot open input file '%s'.\n", dat_filename);
		return 1;
	}

	html = fopen(html_filename, "w");
	if( !html )
	{
		fprintf(stderr, "cannot create output file '%s'.\n", html_filename);
		return 1;
    }

	mkdir(html_directory, 0);

    fprintf(html, "<html>\n");
	fprintf(html, "<head>\n");
	fprintf(html, "<title>Contents of %s</title>\n", dat_filename);
	fprintf(html, "</head>\n");
	fprintf(html, "<body>\n");
	fprintf(html, "<h1>Contents of %s</h1>\n", dat_filename);
	fprintf(html, "<hr>\n");

    while( !feof(dat) )
	{
		char line[1024], *eol;
		fgets(line, sizeof(line), dat);
		eol = strchr(line, '\n');
		if( eol )
			*eol = '\0';
		eol = strchr(line, '\r');
		if( eol )
            *eol = '\0';
        if( line[0] != '#' )
		{
			if( line[0] == '$' )
			{
				if( strncasecmp(line + 1, "info", 4) == 0 )
				{
					char *eq = strchr(line, '='), *p;
					strcpy(system_name, eq + 1);
					p = strchr(system_name, '\\');
					if( p )
						*p = '\0';
					/* multiple systems? */
					p = strchr(system_name, ',');
					if( p )
					{
						*p = '\0';
						sprintf(system_filename, "%s/%s.html", html_directory, system_name);
						fprintf(html, "<h4><a href=\"%s\">%s (aka %s)</a></h4>\n", system_filename, system_name, p+1);
                    }
					else
					{
						sprintf(system_filename, "%s/%s.html", html_directory, system_name);
						fprintf(html, "<h4><a href=\"%s\">%s</a></h4>\n", system_filename, system_name);
					}
					html_system = fopen(system_filename, "w");
					if( !html_system )
					{
						fprintf(stderr, "cannot create system_name file '%s'.\n", system_filename);
						return 1;
                    }
					fprintf(html_system, "<html>\n");
                    fprintf(html_system, "<head>\n");
					fprintf(html_system, "<title>Info for %s</title>\n", system_name);
                    fprintf(html_system, "</head>\n");
                    fprintf(html_system, "<body>\n");
					fprintf(html_system, "<h1>Info for %s</h1>\n", system_name);
					fprintf(html_system, "<h4><a href=\"../index.html\">Back to index</a></h4>\n");
                    fprintf(html_system, "<hr>\n");
                }
				else
				if( strncasecmp(line + 1, "bio", 3) == 0 )
                {
					/* that's just fine... */
				}
				else
				if( strncasecmp(line + 1, "end", 3) == 0 )
				{
                    fprintf(html_system, "<hr>\n");
					fprintf(html_system, "<center><font size=-2>created on %s</font></center>\n", ctime(&tm));
                    fprintf(html_system, "</body>\n");
					fprintf(html_system, "</html>\n");
					fclose(html_system);
					html_system = NULL;
				}
			}
			else
			{
				if( html_system )
					fprintf(html_system, "%s<br>\n", line);
			}
		}
	}
	fprintf(html, "<hr>\n");
	fprintf(html, "<center><font size=-2>created on %s</font></center>\n", ctime(&tm));
    fprintf(html, "</body>\n");
    fprintf(html, "</html>\n");
    fclose(html);
	fclose(dat);
	return 0;
}
