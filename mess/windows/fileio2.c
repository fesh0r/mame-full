#include <stdio.h>
#include "osdepend.h"

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message (const char *name, int current, int total)
{
	if( name )
		fprintf (stdout, "loading %-12s\r", name);
	else
		fprintf (stdout, "                    \r");
	fflush (stdout);

//	if( keyboard_pressed (KEYCODE_LCONTROL) && keyboard_pressed (KEYCODE_C) )
//		return 1;

	return 0;
}
