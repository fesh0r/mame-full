#include <conio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <unistd.h>

void abort(void)
{
	exit(-1);
}

void raise(int signo)
{
	exit(-1);
}

int _getch(void)
{
	return 0;
}

int _kbhit(void)
{
	return 0;
}

// --------------------------------------------------------------------------

#define A2T(asciistr)	ascii2wide(_alloca(ascii2widelen(asciistr) * sizeof(WCHAR)), (asciistr))

static int ascii2widelen(const char *asciistr)
{
	return MultiByteToWideChar(CP_ACP, 0, asciistr, -1, NULL, 0);
}

static WCHAR *ascii2wide(WCHAR *dest, const char *asciistr)
{
	MultiByteToWideChar(CP_ACP, 0, asciistr, -1, dest, ascii2widelen(asciistr));
	return dest;
}

// --------------------------------------------------------------------------

int stat( const char *path, struct stat *buffer )
{
	DWORD dwAttributes;
	int st_mode;

	dwAttributes = GetFileAttributes(A2T(path));
	if (dwAttributes == 0xffffffff)
		return -1;

	st_mode = (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : 0;
	buffer->st_mode = st_mode;
	return 0;
}

clock_t clock(void)
{
	return (clock_t) NULL;
}

time_t time(time_t *t)
{
	return (time_t) NULL;
}

struct tm *localtime(const time_t *t)
{
	return NULL;
}

struct tm *gmtime(const time_t *t)
{
	return NULL;
}

time_t mktime( struct tm *timeptr )
{
	return 0;
}

char *strerror(int errnum)
{
	return "Error";
}

char *strdup(const char *s)
{
	char *news;
	news = malloc(strlen(s) + 1);
	if (news)
		strcpy(news, s);
	return news;
}

void system(const char *command)
{
}

#ifdef DEBUG
void _fail(const char *exp, const char *file, int lineno)
{
}
#endif
