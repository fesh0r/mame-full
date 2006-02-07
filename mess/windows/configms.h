//============================================================
//
//	configms.h - Win32 MESS specific options
//
//============================================================

#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "windows/rc.h"

extern struct rc_option mess_opts[];
extern int win_write_config;

int write_config (const char* filename, const game_driver *gamedrv);
const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

void win_add_mess_device_options(struct rc_struct *rc, const game_driver *gamedrv);
void win_mess_config_init(void);

#endif /* CONFIGMS_H */

