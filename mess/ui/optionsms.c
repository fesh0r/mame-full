#define WIN32_LEAN_AND_MEAN

static void MessColumnEncodeString(void* data, char *str);
static void MessColumnDecodeString(const char* str, void* data);
static void MessColumnDecodeWidths(const char* str, void* data);

#include "ui/options.c"

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
	if (settings.view == VIEW_REPORT || settings.view == VIEW_GROUPED)
		MessColumnDecodeString(str, data);
}

void SetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_width[i] = width[i];
}

void GetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        width[i] = settings.mess.mess_column_width[i];
}

void SetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_order[i] = order[i];
}

void GetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        order[i] = settings.mess.mess_column_order[i];
}

void SetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_shown[i] = shown[i];
}

void GetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        shown[i] = settings.mess.mess_column_shown[i];
}

const char* GetSoftwareDirs(void)
{
    return settings.mess.softwaredirs;
}

void SetSoftwareDirs(const char* paths)
{
	FreeIfAllocated(&settings.mess.softwaredirs);
    if (paths != NULL)
        settings.mess.softwaredirs = strdup(paths);
}

const char *GetCrcDir(void)
{
	return settings.mess.hashdir;
}

void SetCrcDir(const char *hashdir)
{
	FreeIfAllocated(&settings.mess.hashdir);
    if (hashdir != NULL)
        settings.mess.hashdir = strdup(hashdir);
}

BOOL GetUseNewUI(int driver_index)
{
    assert(0 <= driver_index && driver_index < driver_index);
    return GetGameOptions(driver_index, -1)->mess.use_new_ui;
}

void SetSelectedSoftware(int driver_index, int devtype, const char *software)
{
	char *newsoftware;
	options_type *o;

	newsoftware = strdup(software ? software : "");
	if (!newsoftware)
		return;

	o = GetGameOptions(driver_index, -1);
	FreeIfAllocated(&o->mess.software[devtype]);
	o->mess.software[devtype] = newsoftware;
}

const char *GetSelectedSoftware(int driver_index, int devtype)
{
	const char *software;
	software = GetGameOptions(driver_index, -1)->mess.software[devtype];
	return software ? software : "";
}

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths)
{
	char *new_extra_paths = NULL;

	assert(driver_index >= 0);
	assert(driver_index < num_games);

	if (extra_paths && *extra_paths)
	{
		new_extra_paths = strdup(extra_paths);
		if (!new_extra_paths)
			return;
	}
	FreeIfAllocated(&game_variables[driver_index].mess.extra_software_paths);
	game_variables[driver_index].mess.extra_software_paths = new_extra_paths;
}

const char *GetExtraSoftwarePaths(int driver_index)
{
	const char *paths;

	assert(driver_index >= 0);
	assert(driver_index < num_games);

	paths = game_variables[driver_index].mess.extra_software_paths;
	return paths ? paths : "";
}


