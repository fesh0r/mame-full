#include "driver.h"
#include "osdepend.h"
#define NC_NUM_COLOURS 4

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

#define NC200_NUM_COLOURS 4

extern VIDEO_START( nc );
extern VIDEO_UPDATE( nc );
extern PALETTE_INIT( nc );

void nc_set_card_present_state(int);

int nc_pcmcia_card_init(int id);
int nc_pcmcia_card_load(mess_image *img, mame_file *fp, int open_mode);
void nc_pcmcia_card_exit(int id);

int nc_serial_load(int, mame_file *fp, int open_mode);

enum
{
        NC_TYPE_1xx, /* nc100/nc150 */
        NC_TYPE_200  /* nc200 */
};

void nc200_video_set_backlight(int state);


