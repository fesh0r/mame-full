#define WIN32_LEAN_AND_MEAN

#include "options.c"

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
