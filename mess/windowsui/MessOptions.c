#define WIN32_LEAN_AND_MEAN

static int default_mess_column_width[] = { 186, 230, 88, 84, 84, 68 };
static int default_mess_column_shown[] = {   1,   1,  1,  1,  1,  1 };
static int default_mess_column_order[] = {   0,   1,  2,  3,  4,  5 };

static void MessColumnEncodeString(void* data, char *str);
static void MessColumnDecodeString(const char* str, void* data);
static void MessColumnDecodeWidths(const char* str, void* data);

#include "windowsui/options.c"

static void MessColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, MESS_COLUMN_MAX);
}

static void MessColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, MESS_COLUMN_MAX);
}

static void MessColumnDecodeWidths(const char* str, void* data)
{
    if (settings.view == VIEW_REPORT)
        MessColumnDecodeString(str, data);
}

void SetDefaultSoftware(const char *name)
{
   if (settings.default_software != NULL)
    {
        free(settings.default_software);
        settings.default_software = NULL;
    }

    settings.default_software = strdup(name ? name : "");
}

const char *GetDefaultSoftware(void)
{
	assert((sizeof(default_mess_column_width) / sizeof(default_mess_column_width[0])) == MESS_COLUMN_MAX);
	assert((sizeof(default_mess_column_shown) / sizeof(default_mess_column_shown[0])) == MESS_COLUMN_MAX);
	assert((sizeof(default_mess_column_order) / sizeof(default_mess_column_order[0])) == MESS_COLUMN_MAX);
    return settings.default_software ? settings.default_software : "";
}

void SetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        settings.mess_column_width[i] = width[i];
}

void GetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        width[i] = settings.mess_column_width[i];
}

void SetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess_column_order[i] = order[i];
}

void GetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        order[i] = settings.mess_column_order[i];
}

void SetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess_column_shown[i] = shown[i];
}

void GetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        shown[i] = settings.mess_column_shown[i];
}

const char* GetSoftwareDirs(void)
{
    return settings.softwaredirs;
}

void SetSoftwareDirs(const char* paths)
{
    if (settings.softwaredirs != NULL)
    {
        free(settings.softwaredirs);
        settings.softwaredirs = NULL;
    }

    if (paths != NULL)
        settings.softwaredirs = strdup(paths);
}

const char *GetCrcDir(void)
{
	return settings.crcdir;
}

void SetCrcDir(const char *crcdir)
{
    if (settings.crcdir != NULL)
    {
        free(settings.crcdir);
        settings.crcdir = NULL;
    }

    if (crcdir != NULL)
        settings.crcdir = strdup(crcdir);
}

BOOL GetUseNewUI(int num_game)
{
    assert(0 <= num_game && num_game < num_games);

    return game[num_game].use_new_ui;
}

