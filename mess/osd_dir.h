#ifndef __OSD_DIR_H_
#define __OSD_DIR_H_

/* called by the filemanager code to allow the OS to override the file */
/* selecting with it's own code. Return 0 if the MESS core should handle */
/* file selection, -1 if the OS code does nothing or 1 if the OS code */
/* changed a file. */
int osd_select_file (int sel, char *filename);

int osd_num_devices(void);
const char *osd_get_device_name(int i);
void osd_change_device(const char *vol);
void *osd_dir_open(const char *mdirname, const char *filemask);
int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir);
void osd_dir_close(void *dir);
extern void osd_change_directory(const char *path);
extern const char *osd_get_cwd(void);

#endif
