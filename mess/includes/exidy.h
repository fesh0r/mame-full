#include "driver.h"
#include "osdepend.h"
#define EXIDY_NUM_COLOURS 2

/* 64 chars wide, 30 chars tall */
#define EXIDY_SCREEN_WIDTH        (64*8)
#define EXIDY_SCREEN_HEIGHT       (30*8)

extern VIDEO_START( exidy );
extern VIDEO_UPDATE( exidy );
extern PALETTE_INIT( exidy );

int exidy_cassette_init(mess_image *img, mame_file *fp, int open_mode);
