#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "ui/options.h"

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

