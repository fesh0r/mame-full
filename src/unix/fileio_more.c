#include "xmame.h"

/*============================================================ */
/*	osd_display_loading_rom_message */
/*============================================================ */

/* 
 * Called while loading ROMs.  It is called a last time with name == 0 to 
 * signal that the ROM loading process is finished.  return non-zero to abort 
 * loading
 */
int osd_display_loading_rom_message(const char *name,
		struct rom_load_data *romdata)
{
	static int count = 0;
	
	if (name)
		fprintf(stderr_file,"loading rom %d: %-12s\n", count, name);
	else
		fprintf(stderr_file,"done\n");
	
	fflush(stderr_file);
	count++;

	return 0;
}

#ifdef MESS

int osd_select_file(mess_image *img, char *filename)
{
	return 0;
}

void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
}

#endif
