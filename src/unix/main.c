/*
 * X-mame main-routine
 */

#define __MAIN_C_
#include "xmame.h"
#include "sysdep/sysdep_display.h"

#if defined HAVE_MPROTECT || defined __QNXNTO__
#include <sys/mman.h>
#endif

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	/* now invoice system-dependent initialization */
#ifdef XMAME_NET
	if (osd_net_init() != OSD_OK)
		return OSD_NOT_OK;
#endif	
	if (osd_input_initpre() != OSD_OK)
		return OSD_NOT_OK;

	return OSD_OK;
}

/*
 * Cleanup routines to be executed when the program is terminated.
 */
void osd_exit(void)
{
#ifdef XMAME_NET
	osd_net_close();
#endif
	free_pathlists();
	osd_input_close();
}

void *osd_alloc_executable(size_t size)
{
	void *addr = malloc(size);
#ifdef HAVE_MPROTECT
	if (addr)
	{
		if(mprotect(addr, size, PROT_READ | PROT_WRITE | PROT_EXEC)!=0)
		{
			free(addr);
			addr = NULL;
		}
	}
#endif
	return addr;
}

void osd_free_executable(void *ptr)
{
	free(ptr);
}

int main(int argc, char **argv)
{
	int res;

#ifdef __QNXNTO__
	printf("info: Trying to enable swapfile.... ");
	munlockall();
	printf("Success!\n");
#endif

	/* some display methods need to do some stuff with root rights */
	if(sysdep_display_init())
		return 1;

	/* to be absolutely safe force giving up root rights here in case
	   a display method doesn't */
	if (setuid(getuid()))
	{
		perror("setuid");
		sysdep_display_exit();
		return OSD_NOT_OK;
	}

	/* Set the title, now auto build from defines from the makefile */
	sprintf(title,"%s (%s) version %s", NAME, DISPLAY_METHOD,
			build_version);

	/* parse configuration file and environment */
	if ((res = config_init(argc, argv)) == 1234)
	{
		/* go for it */
		res = run_game (game_index);
	}

	config_exit();
	sysdep_display_exit();

	return res;
}
