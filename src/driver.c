#include "driver.h"

/* Drivers for each of the game consoles */
extern struct GameDriver nes_driver;
extern struct GameDriver coleco_driver;
extern struct GameDriver genesis_driver;
extern struct GameDriver vectrex_driver;
extern struct GameDriver vcs5200_driver;
extern struct GameDriver sms_driver;
extern struct GameDriver gamegear_driver;
extern struct GameDriver astrocde_driver;
/* Drivers for each of the computer systems */
extern struct GameDriver trs80_driver;
extern struct GameDriver trs80_m3_driver;
extern struct GameDriver cgenie_driver;
extern struct GameDriver atari800_driver;
extern struct GameDriver apple2e_driver;
extern struct GameDriver apple2ee_driver;
extern struct GameDriver apple2ep_driver;
extern struct GameDriver apple2c_driver;
extern struct GameDriver apple2c0_driver;
extern struct GameDriver apl2cpls_driver;
extern struct GameDriver pdp1_driver;
extern struct GameDriver kaypro_driver;


const struct GameDriver *drivers[] =
{
	/* Game console drivers */
	&nes_driver,
	&coleco_driver,
	&genesis_driver,
	&vectrex_driver,
	&vcs5200_driver,
	&sms_driver,
	&gamegear_driver,
	&astrocde_driver,
	/* Computer system drivers */
	&trs80_driver,
	&trs80_m3_driver,
	&cgenie_driver,
	&atari800_driver,
	&apple2e_driver,
	&apple2ee_driver,
	&apple2ep_driver,
	&apple2c_driver,
	&apple2c0_driver,
	&apl2cpls_driver,
	&pdp1_driver,
	&kaypro_driver,
	0	/* end of array */
};
