#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "ui/options.h"

void SetCrcDir(const char *dir);
const char *GetCrcDir(void);

void SetSelectedSoftware(int driver_index, int devtype, const char *software);
const char *GetSelectedSoftware(int driver_index, int devtype);

#endif /* OPTIONSMS_H */

