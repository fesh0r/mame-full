#include "flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

int pc_floppy_init(int id, mame_file *fp, int open_mode);
void pc_floppy_exit(int id);

#ifdef __cplusplus
}
#endif

