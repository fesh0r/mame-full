
#include "driver.h"

int lynx_vh_start(void);
void lynx_vh_stop(void);
void lynx_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

extern UINT16 lynx_granularity;

typedef struct {
	UINT8 data[0x100];
} MIKEY;
extern MIKEY mikey;
WRITE_HANDLER( lynx_memory_config );
WRITE_HANDLER(mikey_write);
READ_HANDLER(mikey_read);
WRITE_HANDLER(suzy_write);
READ_HANDLER(suzy_read);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
extern "C" void lynx_runtime_loader_init(void);
# else
extern void lynx_runtime_loader_init(void);
# endif
#endif
