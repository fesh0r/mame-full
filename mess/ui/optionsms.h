#ifndef OPTIONSMS_H
#define OPTIONSMS_H

#include "ui/options.h"

void SetDefaultSoftware(const char *name);
const char *GetDefaultSoftware(void);

void SetCrcDir(const char *dir);
const char *GetCrcDir(void);

#endif /* OPTIONSMS_H */

