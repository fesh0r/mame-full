#include "driver.h"

/* Drivers for each of the game consoles */
extern struct GameDriver nes_driver;
extern struct GameDriver coleco_driver;
extern struct GameDriver genesis_driver;
/* Drivers for each of the computer systems */
extern struct GameDriver trs80_driver;
extern struct GameDriver trs80_m3_driver;
extern struct GameDriver cgenie_driver;


const struct GameDriver *drivers[] =
{
	/* Game console drivers */
	&nes_driver,
	&coleco_driver,
	&genesis_driver,
	/* Computer system driver */
	&trs80_driver,
	&trs80_m3_driver,
	&cgenie_driver,
	0	/* end of array */
};
