#include <time.h>

typedef enum {
	MC146818_STANDARD,
	MC146818_IGNORE_CENTURY, // century is NOT set, for systems having other usage of this byte
	MC146818_ENHANCED 
} MC146818_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

// initialise mc146818 emulation, call only once at beginning
void mc146818_init(MC146818_TYPE type);
// loads data from standard nvram file
void mc146818_load(void);
// loads data from file stream
void mc146818_load_stream(void *file);
// set mc146818 to actual time
void mc146818_set_time(void);
// sets realtime clock to given time
void mc146818_set_gmtime(struct tm *tmtime);
// saves data into standard nvram file
void mc146818_save(void);
// saves data into file stream
void mc146818_save_stream(void *file);

extern NVRAM_HANDLER( mc146818 );

READ_HANDLER(mc146818_port_r);
WRITE_HANDLER(mc146818_port_w);

#ifdef __cplusplus
}
#endif
