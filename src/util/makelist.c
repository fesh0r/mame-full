/*****************************************************************************
 *
 *	makelist.c
 *	Utility to (re-)create the enumartions of CPU cores and
 *	sound chips from the rules.mak file in MAME header files.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define VERBOSE 0

#define CPU_DUMMY	"CPU_DUMMY"
#define SND_DUMMY	"SOUND_DUMMY"
#define CPU_COUNT	"CPU_COUNT"
#define SND_COUNT	"SOUND_COUNT"
#define CPU_INTF    "struct cpu_interface cpuintf[]"
#define SND_INTF	"struct snd_interface sndintf[]"
#define WARNING 	"/* !!! This entry does not have a build rule in src/rules.mak !!! */"
struct enum_comment_s
{
	int order;
	int printed_h;
	int printed_c;
	char *enum_name;
	char *comment;
	char *source;
};

static struct enum_comment_s enum_comment[256];
static int enum_comment_count;
static int cpu_or_snd;
#define CPU 	1
#define SOUND	2

static char *dupstr(char *src)
{
	char *dst;
	if (!src)
		return NULL;
	dst = (char *)malloc(strlen(src) + 1);
	if (!dst)
		return NULL;
	strcpy(dst, src);
	return dst;
}

static char *catstr(char *dst, char *src)
{
	if (!src)
		return dst;
	if (!dst)
		return dupstr(src);

    dst = (char *)realloc(dst, strlen(dst) + strlen(src) + 1);
	if (!dst)
		return NULL;
	strcat(dst, src);

    return dst;
}

/* compare strings ignoring whitespace */
static int strcmps(char *dst, char *src)
{
	while (*src && *dst)
	{
		while (*src && isspace(*src))
			src++;
		while (*dst && isspace(*dst))
			dst++;
		if (*dst != *src)
			return *dst - *src;
		src++;
		dst++;
    }
	if (!*dst || !*src)
		return 0;
	return *dst - *src;
}


static int read_enum(char *filename)
{
	FILE *fp;
	int parsing = 0;

	enum_comment_count = 0;
	cpu_or_snd = 0;

	fp = fopen(filename, "r");

	if (!fp)
		return 1;

	while (!feof(fp))
	{
		char line[512] = "";

		fgets(line, sizeof(line), fp);
		if (!line[0])
			break;

		if (strstr(line, CPU_DUMMY))
		{
#if VERBOSE
            printf("parsing CPUs\n");
#endif
            cpu_or_snd = parsing = CPU;
		}
		else
		if (strstr(line, SND_DUMMY))
		{
#if VERBOSE
			printf("parsing SOUNDs\n");
#endif
            cpu_or_snd = parsing = SOUND;
		}
        else
		if (strstr(line, CPU_COUNT))
			parsing = 0;
		else
		if (strstr(line, SND_COUNT))
			parsing = 0;

		if (parsing)
		{
			char *name = strstr(line, (cpu_or_snd == CPU) ? "CPU_" : "SOUND_");
			if (name)
			{
				char c, *delim;

				memset(&enum_comment[enum_comment_count], 0, sizeof(struct enum_comment_s));
				enum_comment[enum_comment_count].order = -1;

				delim = strchr(name, ',');

				if (!delim)
					delim = strchr(name, '\n');
				c = *delim;
				*delim = '\0';
				enum_comment[enum_comment_count].enum_name = dupstr(name);
				*delim++ = c;
				name = strchr(delim, '/');
				if (name)
				{
					delim = strchr(name, '\n');
					if (delim)
						*delim = '\0';
					enum_comment[enum_comment_count].comment = dupstr(name);
				}
				enum_comment_count++;
			}
		}
	}

	fclose(fp);
	return 0;
}

static void add_comment(char *enum_name, char *comment)
{
	int i;

	for (i = 0; i < enum_comment_count; i++)
	{
		if (strcmp(enum_name, enum_comment[i].enum_name) == 0)
		{
			if (enum_comment[i].comment == NULL && strlen(comment))
				enum_comment[i].comment = dupstr(comment);
			return;
		}
	}
	printf("change " __FILE__ ": %s is obsolete (or missing in rules.mak)!\n", enum_name);
}

static void add_defaults(void)
{
	if (strncmp(enum_comment[0].enum_name, "CPU_", 4) == 0)
	{
		add_comment(CPU_DUMMY,"");
		add_comment("CPU_Z80","");
		add_comment("CPU_Z80GB","");
		add_comment("CPU_CDP1802","");
		add_comment("CPU_8080","");
		add_comment("CPU_8085A","");
		add_comment("CPU_M6502","");
		add_comment("CPU_M65C02","");
		add_comment("CPU_M65SC02","");
		add_comment("CPU_M65CE02","");
		add_comment("CPU_M6509","");
		add_comment("CPU_M6510","");
		add_comment("CPU_M6510T","");
		add_comment("CPU_M7501","");
		add_comment("CPU_M8502","");
		add_comment("CPU_N2A03","");
		add_comment("CPU_M4510","");
		add_comment("CPU_H6280","");
		add_comment("CPU_I86","");
		add_comment("CPU_I88","");
		add_comment("CPU_I186","");
		add_comment("CPU_I188","");
		add_comment("CPU_I286","");
		add_comment("CPU_V20","");
		add_comment("CPU_V30","");
		add_comment("CPU_V33","");
		add_comment("CPU_I8035","/* same as CPU_I8039 */");
		add_comment("CPU_I8039","");
		add_comment("CPU_I8048","/* same as CPU_I8039 */");
		add_comment("CPU_N7751","/* same as CPU_I8039 */");
		add_comment("CPU_M6800","/* same as CPU_M6802/CPU_M6808 */");
		add_comment("CPU_M6801","/* same as CPU_M6803 */");
		add_comment("CPU_M6802","/* same as CPU_M6800/CPU_M6808 */");
		add_comment("CPU_M6803","/* same as CPU_M6801 */");
		add_comment("CPU_M6808","/* same as CPU_M6800/CPU_M6802 */");
		add_comment("CPU_HD63701","/* 6808 with some additional opcodes */");
		add_comment("CPU_NSC8105","/* same(?) as CPU_M6802(?) with scrambled opcodes. There is at least one new opcode. */");
		add_comment("CPU_M6805","");
		add_comment("CPU_M68705","/* same as CPU_M6805 */");
		add_comment("CPU_HD63705","/* M6805 family but larger address space, different stack size */");
		add_comment("CPU_HD6309","");
		add_comment("CPU_M6809","");
		add_comment("CPU_KONAMI","");
		add_comment("CPU_M68000","");
		add_comment("CPU_M68010","");
		add_comment("CPU_M68EC020","");
		add_comment("CPU_M68020","");
		add_comment("CPU_T11","");
		add_comment("CPU_S2650","");
		add_comment("CPU_F8","");
		add_comment("CPU_CP1600","");
		add_comment("CPU_TMS34010","");
		add_comment("CPU_TMS9900","");
		add_comment("CPU_TMS9940","");
		add_comment("CPU_TMS9980","");
		add_comment("CPU_TMS9985","");
		add_comment("CPU_TMS9989","");
		add_comment("CPU_TMS9995","");
		add_comment("CPU_TMS99105A","");
		add_comment("CPU_TMS99110A","");
		add_comment("CPU_Z8000","");
		add_comment("CPU_TMS320C10","");
		add_comment("CPU_CCPU","");
		add_comment("CPU_PDP1","");
		add_comment("CPU_ADSP2100","");
		add_comment("CPU_ADSP2105","");
		add_comment("CPU_PSXCPU","");
		add_comment("CPU_SH2","");
		add_comment("CPU_SC61860","");
		add_comment("CPU_ARM","");
		add_comment("CPU_G65816","");
		add_comment("CPU_SPC700","");
	}
	else
	{
		add_comment(SND_DUMMY,"");
		add_comment("SOUND_CUSTOM","");
		add_comment("SOUND_SAMPLES","");
		add_comment("SOUND_DAC","");
		add_comment("SOUND_AY8910","");
		add_comment("SOUND_YM2203","");
		add_comment("SOUND_YM2151","");
		add_comment("SOUND_YM2608","");
		add_comment("SOUND_YM2610","");
		add_comment("SOUND_YM2610B","");
		add_comment("SOUND_YM2612","");
		add_comment("SOUND_YM3438","/* same as YM2612 */");
		add_comment("SOUND_YM2413","/* YM3812 with predefined instruments */");
		add_comment("SOUND_YM3812","");
		add_comment("SOUND_YM3526","/* 100% YM3812 compatible, less features */");
		add_comment("SOUND_YMZ280B","");
		add_comment("SOUND_Y8950","/* YM3526 compatible with delta-T ADPCM */");
		add_comment("SOUND_SN76477","");
		add_comment("SOUND_SN76496","");
		add_comment("SOUND_POKEY","");
		add_comment("SOUND_TIA","/* stripped down Pokey */");
		add_comment("SOUND_NES","");
		add_comment("SOUND_ASTROCADE","/* Custom I/O chip from Bally/Midway */");
		add_comment("SOUND_NAMCO","");
		add_comment("SOUND_TMS36XX","/* currently TMS3615 and TMS3617 */");
		add_comment("SOUND_TMS5110","");
		add_comment("SOUND_TMS5220","");
		add_comment("SOUND_VLM5030","");
		add_comment("SOUND_ADPCM","");
		add_comment("SOUND_OKIM6295","/* ROM-based ADPCM system */");
		add_comment("SOUND_MSM5205","/* CPU-based ADPCM system */");
		add_comment("SOUND_UPD7759","/* ROM-based ADPCM system */");
		add_comment("SOUND_HC55516","/* Harris family of CVSD CODECs */");
		add_comment("SOUND_K005289","/* Konami 005289 */");
		add_comment("SOUND_K007232","/* Konami 007232 */");
		add_comment("SOUND_K051649","/* Konami 051649 */");
		add_comment("SOUND_K053260","/* Konami 053260 */");
		add_comment("SOUND_SEGAPCM","");
		add_comment("SOUND_RF5C68","");
		add_comment("SOUND_CEM3394","");
		add_comment("SOUND_C140","");
		add_comment("SOUND_QSOUND","");
		add_comment("SOUND_SAA1099","");
		add_comment("SOUND_SPEAKER","");
		add_comment("SOUND_WAVE","");
		add_comment("SOUND_BEEP","");
	}
}

static int set_order(char *enum_name, int pos)
{
	int i;

	for (i = 0; i < enum_comment_count; i++)
	{
		if (strcmp(enum_name, enum_comment[i].enum_name) == 0)
		{
#if VERBOSE
			printf("found #%02d %-24s @%d\n", pos, enum_name, i);
#endif
            enum_comment[i].order = pos++;
			return pos;
		}
	}
#if VERBOSE
	printf("added #%02d %-24s @%d\n", pos, enum_name, i);
#endif
	memset(&enum_comment[i], 0, sizeof(struct enum_comment_s));
    enum_comment[i].order = pos++;
	enum_comment[i].enum_name = dupstr(enum_name);
	enum_comment_count++;
    return pos;
}

static int parse_makefile(char *filename)
{
	FILE *fp;
	int pos = 0;

	fp = fopen(filename, "r");
	if (!fp)
		return 1;

	if (cpu_or_snd == CPU)
		pos = set_order(CPU_DUMMY, pos);
	else
	if (cpu_or_snd == SOUND)
		pos = set_order(SND_DUMMY, pos);

	while (!feof(fp))
	{
		char line[512], *has, *at = NULL;

		line[0] = '\0';
		fgets(line, sizeof(line), fp);
		if (!line[0])
			break;
		if ((cpu_or_snd == CPU && strncmp(line, "CPU=$(strip", 11)) ||
			(cpu_or_snd == SOUND && strncmp(line, "SOUND=$(strip", 13)))
			continue;
		has = strstr(line, "$(findstring");
		if (!has)
			continue;
		has = has + sizeof("$(findstring");
		at = strchr(has, '@');
		if (has && at)
		{
			char enum_name[32];
			*at = '\0';
			if (cpu_or_snd == CPU)
				sprintf(enum_name, "CPU_%s", has);
			else
			if (cpu_or_snd == SOUND)
				sprintf(enum_name, "SOUND_%s", has);
			pos = set_order(enum_name, pos);
		}
	}

	fclose(fp);
	return 0;
}

static int get_order(char *enum_name)
{
	int i;

	for (i = 0; i < enum_comment_count; i++)
	{
		if (strcmp(enum_name, enum_comment[i].enum_name) == 0)
			return enum_comment[i].order;
	}
	return -1;
}

static int append_source(int order, char *line)
{
	int i;

	for (i = 0; i < enum_comment_count; i++)
    {
		if (enum_comment[i].order == order)
        {
			enum_comment[i].source = catstr(enum_comment[i].source, line);
			return 0;
		}
	}
	return -1;
}

static int rewrite_header(char *filename)
{
	FILE *fi, *fo;
	char tempname[260], *h;
	int pos = 0, changed = 0, parsing = 0;

	strcpy(tempname, filename);
	h = strrchr(tempname, '.');
	if (!h)
		h = tempname + strlen(tempname);
	strcpy(h, ".$$$");

	fi = fopen(filename, "r");
	fo = fopen(tempname, "w");

	while (!feof(fi))
	{
		char line[512] = "";

		fgets(line, sizeof(line), fi);
		if (!line[0])
			break;

		if (strstr(line, CPU_DUMMY) || strstr(line, SND_DUMMY))
			parsing = 1;
		else
		if (strstr(line, CPU_COUNT) || strstr(line, SND_COUNT))
		{
			/* if we were parsing the enum */
			if (parsing)
			{
				int i, j;
				for (i = 0; i < enum_comment_count; i++)
				{
					for (j = 0; j < enum_comment_count; j++)
					{
						if (enum_comment[j].order != i)
							continue;
						if (i > 0)
							fprintf(fo,"#if (HAS_%s)\n", &enum_comment[j].enum_name[(cpu_or_snd == CPU) ? 4 : 6]);
						if (enum_comment[j].comment)
							fprintf(fo,"\t%s,\t%s\n", enum_comment[j].enum_name, enum_comment[j].comment);
						else
							fprintf(fo,"\t%s,\n", enum_comment[j].enum_name);
						if (i > 0)
							fprintf(fo,"#endif\n");
						enum_comment[j].printed_h = 1;
                    }
				}
				for (j = 0; j < enum_comment_count; j++)
                {
					if (enum_comment[j].printed_h)
						continue;
					fprintf(fo,"#if (HAS_%s)\n", &enum_comment[j].enum_name[(cpu_or_snd == CPU) ? 4 : 6]);
					fprintf(fo, "%s\n", WARNING);
					if (enum_comment[j].comment)
						fprintf(fo,"\t%s,\t%s\n", enum_comment[j].enum_name, enum_comment[j].comment);
					else
						fprintf(fo,"\t%s,\n", enum_comment[j].enum_name);
                    fprintf(fo,"#endif\n");
                }
                parsing = 0;
			}
		}

		if (parsing)
		{
			char *name = strstr(line, (cpu_or_snd == CPU) ? "CPU_" : "SOUND_");
			if (name)
			{
				char *delim = strchr(name, ',');
				if (!delim)
					delim = strchr(name, '\n');
				*delim = '\0';
				/* check if the enum is in correct order already */
				if (get_order(name) != pos)
				{
#if VERBOSE
                    if (changed == 0)
						fprintf(stderr, "%s wrong order from %s on\n", filename, name);
#endif
                    changed = 1;
				}
				pos++;
			}
		}
		else
		{
			fprintf(fo, "%s", line);
		}
	}
	if (pos > 0 && pos != enum_comment_count)
		changed = 1;

	fclose(fi);
	fclose(fo);

	if (changed)
	{
		printf("%s did change\n", filename);
	}
	else
	{
		if (pos == 0)
			printf("%s does not contain %s\n", filename, (cpu_or_snd == CPU) ? CPU_DUMMY : SND_DUMMY);
		else
			printf("%s did not change\n", filename);
	}
	/* change, discard the original file and rename the tempfile */
	if (remove(filename))
	{
		fprintf(stderr, "cannot remove %s\n", filename);
		return 1;
	}
	if (rename(tempname, filename))
	{
		fprintf(stderr, "cannot rename %s to %s\n", tempname, filename);
		return 1;
	}
    return 0;
}

static int rewrite_source(char *filename)
{
	FILE *fi, *fo;
	char tempname[260], *h;
	int pos = 0, idx = 0, changed = 0, parsing = 0;

	strcpy(tempname, filename);
	h = strrchr(tempname, '.');
	if (!h)
		h = tempname + strlen(tempname);
	strcpy(h, ".$$$");

	fi = fopen(filename, "r");
	fo = fopen(tempname, "w");

	while (!feof(fi))
	{
		char line[512] = "";

		fgets(line, sizeof(line), fi);
		if (!line[0])
			break;

		/* find the beginning of the structs in the source */
		if (strcmps(line, CPU_INTF) == 0 ||
			strcmps(line, SND_INTF) == 0)
		{
            /* the first entry is not enclosed in #if (HAS...) */
            if (parsing == 0)
				pos++;
			parsing = 1;
		}
		else
		/* end of the structs in the source? */
		if (parsing && strncmp(line, "};", 2) == 0)
		{
			int i, j;
			for (i = 0; i < enum_comment_count; i++)
			{
				for (j = 0; j < enum_comment_count; j++)
				{
					if (enum_comment[j].order != i)
						continue;
					if (i > 0)
						fprintf(fo,"#if (HAS_%s)\n", &enum_comment[j].enum_name[(cpu_or_snd == CPU) ? 4 : 6]);
					if (enum_comment[j].source)
						fprintf(fo, "%s", enum_comment[j].source);
					if (i > 0)
						fprintf(fo,"#endif\n");
					enum_comment[j].printed_c = 1;
                }
			}
			for (j = 0; j < enum_comment_count; j++)
			{
				if (enum_comment[j].printed_c)
					continue;
				fprintf(fo,"#if (HAS_%s)\n", &enum_comment[j].enum_name[(cpu_or_snd == CPU) ? 4 : 6]);
                if (enum_comment[j].source)
				{
					if (strcmps(enum_comment[j].source, WARNING))
						fprintf(fo, "%s\n", WARNING);
					fprintf(fo,"%s", enum_comment[j].source);
				}
				else
				{
					fprintf(fo, "%s\n", WARNING);
                }
				fprintf(fo,"#endif\n");
			}
            parsing = 0;
		}

		if (parsing)
		{
			if (strcmps(line, "#if (HAS_") == 0)
			{
				char name[32], *cp;
				cp = strchr(line + 9, ')');
				if (!cp)
					cp = strchr(line, '\n');
				if (cp)
					*cp = '\0';
                if (cpu_or_snd == CPU)
					sprintf(name, "CPU_%s", line + 9);
				else
					sprintf(name, "SOUND_%s", line + 9);
				/* check if the array is in correct order already */
				idx = get_order(name);
				if (idx != pos)
                {
#if VERBOSE
                    if (changed == 0)
						fprintf(stderr, "%s wrong order from %s on\n", filename, name);
#endif
                    changed = 1;
                }
            }
			else
			if (strncmp(line, "#endif", 6) == 0)
			{
                pos++;
            }
			else
			{
				append_source(idx, line);
            }
		}
		else
		{
			fprintf(fo, "%s", line);
		}
	}
	if (pos > 0 && pos != enum_comment_count)
		changed = 1;

	fclose(fi);
	fclose(fo);

	if (changed)
	{
		printf("%s did change\n", filename);
	}
	else
	{
		if (pos == 0)
			printf("%s does not contain %s\n", filename, (cpu_or_snd == CPU) ? CPU_INTF : SND_INTF);
		else
			printf("%s did not change\n", filename);
    }
	/* change, discard the original file and rename the tempname */
	if (remove(filename))
	{
		fprintf(stderr, "cannot remove %s\n", filename);
		return 1;
	}
	if (rename(tempname, filename))
	{
		fprintf(stderr, "cannot rename %s to %s\n", tempname, filename);
		return 1;
	}
    return 0;
}

int main(int ac, char **av)
{
	char *prog;

	prog = strrchr(av[0], '/');

	if (!prog)
		prog = strrchr(av[0], '\\');
	if (!prog)
		prog = av[0];
	else
		prog++;

	if (ac < 4)
	{
		fprintf(stderr, "usage: %s [cpu|snd]intf.h [cpu|snd]intf.c rules.mak\n", prog);
		fprintf(stderr, "Create the enum of CPUs or sound chips depending on\n");
		fprintf(stderr, "the order of defines in the makefile rules.mak\n");
		return 1;
	}
	if (read_enum(av[1]))
	{
		fprintf(stderr, "cannot open %s\n", av[1]);
		return 1;
	}
	if (!enum_comment_count)
	{
		fprintf(stderr, "didn't find an enum{} in %s\n", av[1]);
		return 1;
	}
	if (parse_makefile(av[3]))
	{
		fprintf(stderr, "cannot open %s\n", av[2]);
		return 1;
	}
	add_defaults();
	if (rewrite_header(av[1]))
	{
		fprintf(stderr, "cannot rewrite %s\n", av[1]);
		return 1;
	}
	if (rewrite_source(av[2]))
	{
		fprintf(stderr, "cannot rewrite %s\n", av[2]);
		return 1;
    }
    return 0;
}

