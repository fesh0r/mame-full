#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "device.h"

enum
{
	MESS_COLUMN_IMAGES,
	MESS_COLUMN_GOODNAME,
	MESS_COLUMN_MANUFACTURER,
	MESS_COLUMN_YEAR,
	MESS_COLUMN_PLAYABLE,
	MESS_COLUMN_CRC,
	MESS_COLUMN_MAX
};

struct mess_specific_options
{
	BOOL   use_new_ui;
	UINT32 ram_size;
	char   *software[IO_COUNT];
	char   *softwaredirs[IO_COUNT];
};

struct mess_specific_game_variables
{
	char *extra_software_paths;
};

struct mess_specific_settings
{
	int      mess_column_width[MESS_COLUMN_MAX];
	int      mess_column_order[MESS_COLUMN_MAX];
	int      mess_column_shown[MESS_COLUMN_MAX];

	char*    softwaredirs;
	char*    hashdir;	
};

void SetMessColumnWidths(int widths[]);
void GetMessColumnWidths(int widths[]);

void SetMessColumnOrder(int order[]);
void GetMessColumnOrder(int order[]);

void SetMessColumnShown(int shown[]);
void GetMessColumnShown(int shown[]);

const char* GetSoftwareDirs(void);
void  SetSoftwareDirs(const char* paths);

void SetCrcDir(const char *dir);
const char *GetCrcDir(void);

void SetSelectedSoftware(int driver_index, int devtype, const char *software);
const char *GetSelectedSoftware(int driver_index, int devtype);

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths);
const char *GetExtraSoftwarePaths(int driver_index);

#endif /* OPTIONSMS_H */

